/*
  Memory watcher
  Copyright (C) 2018 Lukas Karas <karas@avast.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "Chart.h"

#include <Utils.h>
#include <ThreadPool.h>
#include <CmdLineParsing.h>

#include <QApplication>
#include <QDebug>
#include <QtGui/QPainter>
#include <QLineSeries>
#include <QChart>
#include <QChartView>
#include <QAreaSeries>
#include <QFileInfo>
#include <QElapsedTimer>

#include <iostream>
#include <unordered_map>
#include <signal.h>
#include <unordered_set>
#include <optional>
#include <Version.h>

using namespace QtCharts;

namespace {
constexpr int ThreadStacks = 0;
constexpr int Anonymous = 1;
constexpr int Heap = 2;
constexpr int Sockets = 3;
constexpr int OtherMappings = 4;
}


struct Arguments {
  bool help{false};
  bool version{false};
  std::optional<long> pid;
  std::optional<qulonglong> processId;
  QString databaseFile;
};

class ArgParser: public CmdLineParser {
private:
  Arguments args;

public:
  ArgParser(QCoreApplication *app,
            int argc, char *argv[])
    : CmdLineParser(app->applicationName().toStdString(), argc, argv) {

    using namespace std::string_literals;

    AddOption(CmdLineFlag([this](const bool &value) {
                args.help = value;
              }),
              std::vector<std::string>{"h", "help"},
              "Display help and exits",
              true);

    AddOption(CmdLineFlag([this](const bool &value) {
                args.version = value;
              }),
              std::vector<std::string>{"v", "version"},
              "Display application version and exits",
              false);

    AddOption(CmdLineULongOption([this](const unsigned long &value) {
                args.pid = value;
              }),
              std::vector<std::string>{"p","pid"},
              "Pid of analyzed process. "s +
              "If not defined, system wide statistics are displayed."s);

    AddOption(CmdLineULongOption([this](const qulonglong &value) {
                args.processId = value;
              }),
              "process-id",
              "Internal process id (may be used in case that pid is not unique)"s);

    AddOption(CmdLineStringOption([this](const std::string &value){
                args.databaseFile = QString::fromStdString(value);
              }),
              "database-file",
              "Sqlite database file with recording. Default is measurement.db");

  }

  Arguments GetArguments() const {
    return args;
  }
};


Chart::Chart(const QString &db,
             std::optional<pid_t> pid,
             std::optional<qulonglong> processId):
  db(db),
  pid(pid),
  processId(processId)
{
}

Chart::~Chart()
{
  QGuiApplication::quit();
}

void Chart::run()
{
  if (!QFileInfo(db).exists()){
    qWarning() << "File don't exists" << db;
    deleteLater();
    return;
  }

  if (!storage.init(db)){
    qWarning() << "Failed to open database" << db;
    deleteLater();
    return;
  }

  if (!storage.getMeasurementTimes(times)){
    qWarning() << "Failed to get measurement time points" << db;
    deleteLater();
    return;
  }

  if (pid.has_value() || processId.has_value()) {
    if (!processId.has_value()) {
      using ProcessMap = QMap<ProcessId, QString>;
      ProcessMap processes;
      storage.lookupPid(pid.value(), processes);
      if (processes.empty()) {
        qWarning() << "Cannot found pid" << pid.value();
        deleteLater();
        return;
      }
      if (processes.size() > 1) {
        std::cerr << "Not unique pid " << pid.value() << ", try to use --process-id argument." << std::endl;
        std::cout << "ProcessId\tname:" << std::endl;
        for (ProcessMap::const_iterator it = processes.cbegin(); it != processes.cend(); ++it) {
          std::cout << it.key().hash() << '\t' << it.value().toStdString() << std::endl;
        }
        deleteLater();
        return;
      }
      processId = processes.firstKey().hash();
    }
  }
  if (!processId.has_value()){
    qWarning() << "Failed to determine process" << db;
    deleteLater();
    return;
  }

  // chart configuration
  ProcessMemoryType type = ProcessMemoryType::Rss;

  qint64 measurementFrom = 0;
  qint64 measurementTo = times.size();
  qint64 pointCount = std::min((qint64)1000000, measurementTo - measurementFrom);

  if (pointCount == 0){
    qWarning() << "No measurements" << db;
    deleteLater();
    return;
  }

  // qint64 measurementCenter = 354250;
  // qint64 pointCount = 128;
  // qint64 measurementFrom = measurementCenter - pointCount/2;
  // qint64 measurementTo = measurementCenter + pointCount/2;

  // compute steps
  qint64 measurementCount = measurementTo - measurementFrom;
  qint64 stepSize = measurementCount / pointCount;
  std::vector<MeasurementGroups> measurements(pointCount);
  QElapsedTimer time;
  time.start();
  MeasurementGroups peak;
  Measurement measurement;
  for (qint64 step = 0; step < pointCount; step ++){
    storage.getMeasurement(processId.value(),
                           times[std::min(measurementTo - 1, step * stepSize)],
                           measurement,
                           true);

    Utils::group(measurements[step], measurement, type, true);
    if (peak.sum < measurements[step].sum) {
      peak = measurements[step];
    }
  }
  qDebug() << "getting data: " << time.elapsed() << "ms";

  time.restart();

  std::vector<QAreaSeries*> chartSeries;

  QLineSeries *statmRss = new QLineSeries();
  statmRss->setPen(QPen(Qt::red, 4));
  statmRss->setName("StatM RSS");

  for (int i = ThreadStacks; i <= OtherMappings; i++) {
    chartSeries.push_back(new QAreaSeries(new QLineSeries(), new QLineSeries()));
  }
  chartSeries[ThreadStacks]->setName("thread stacks");
  chartSeries[Anonymous]->setName("anonymous");
  chartSeries[Heap]->setName("heap");
  chartSeries[Sockets]->setName("sockets");
  chartSeries[OtherMappings]->setName("other mappings");

  int significantMappingCnt = 20;
  std::vector<std::string> significantMappings;
  std::unordered_set<std::string> significantMappingSet;
  int i=0;
  for (const auto &m: peak.sortedMappings()){
    if (i>=significantMappingCnt){
      break;
    }
    significantMappings.push_back(m.name);
    significantMappingSet.insert(m.name);
    QAreaSeries *s = new QAreaSeries(new QLineSeries(), new QLineSeries());
    s->setName(QString::fromStdString(m.name));
    chartSeries.push_back(s);
    i++;
  }

  for (qint64 step = 0; step < pointCount; step ++){
    size_t seriesOffset = 0;
    const MeasurementGroups &measurement = measurements[step];

    statmRss->append(step, measurement.statm.resident);

    chartSeries[ThreadStacks]->lowerSeries()->append(step, seriesOffset);
    seriesOffset += measurement.threadStacks;
    chartSeries[ThreadStacks]->upperSeries()->append(step, seriesOffset);

    chartSeries[Anonymous]->lowerSeries()->append(step, seriesOffset);
    seriesOffset += measurement.anonymous;
    chartSeries[Anonymous]->upperSeries()->append(step, seriesOffset);

    chartSeries[Heap]->lowerSeries()->append(step, seriesOffset);
    seriesOffset += measurement.heap;
    chartSeries[Heap]->upperSeries()->append(step, seriesOffset);

    chartSeries[Sockets]->lowerSeries()->append(step, seriesOffset);
    seriesOffset += measurement.sockets;
    chartSeries[Sockets]->upperSeries()->append(step, seriesOffset);

    size_t otherMappings = 0;
    for (const auto &mapping: measurement.mappings){
      if (significantMappingSet.find(mapping.first) == significantMappingSet.end()){
        otherMappings += mapping.second;
      }
    }
    chartSeries[OtherMappings]->lowerSeries()->append(step, seriesOffset);
    seriesOffset += otherMappings;
    chartSeries[OtherMappings]->upperSeries()->append(step, seriesOffset);

    int i = OtherMappings +1;
    for (const auto &name: significantMappings){
      chartSeries[i]->lowerSeries()->append(step, seriesOffset);
      const auto &it = measurement.mappings.find(name);
      seriesOffset += (it == measurement.mappings.end()) ? 0 : it->second;
      chartSeries[i]->upperSeries()->append(step, seriesOffset);
      i++;
    }
  }

  QChart *chart = new QChart();
  for (auto &series: chartSeries) {
    chart->addSeries(series);
    series->setPen(QPen(series->brush().color(), 0.0));
  }

  chart->addSeries(statmRss);

  qDebug() << "prepare series: " << time.elapsed() << "ms";

  //chart->setAnimationOptions(QChart::AllAnimations);
  chart->legend()->setAlignment(Qt::AlignRight);
  chart->createDefaultAxes();

  QChartView *chartView = new QChartView(chart);
  chartView->setRenderHint(QPainter::Antialiasing);
  window.setCentralWidget(chartView);

  window.resize(1920, 1000);
  window.show();
}

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  Utils::registerQtMetatypes();

  Arguments args;
  {
    ArgParser argParser(&app, argc, argv);

    CmdLineParseResult argResult = argParser.Parse();
    if (argResult.HasError()) {
      std::cerr << "ERROR: " << argResult.GetErrorDescription() << std::endl;
      std::cout << argParser.GetHelp() << std::endl;
      return 1;
    }

    args = argParser.GetArguments();
    if (args.help) {
      std::cout << argParser.GetHelp() << std::endl;
      return 0;
    }
    if (args.version) {
      std::cout << MEMORY_WATCHER_VERSION_STRING << std::endl;
      return 0;
    }
  }

  if (args.databaseFile.isEmpty()) {
    args.databaseFile = QString("measurement.db");
  }

  Chart *chart = new Chart(args.databaseFile,
                           args.pid,
                           args.processId);
  QMetaObject::invokeMethod(chart, "run", Qt::QueuedConnection);

  app.setQuitOnLastWindowClosed(false);
  QObject::connect(&app, &QApplication::lastWindowClosed, chart, &Chart::deleteLater);

  int result = app.exec();
  qDebug() << "Main loop ended...";
  return result;
}
