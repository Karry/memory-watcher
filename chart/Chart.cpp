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

#include <QApplication>
#include <QDebug>
#include <QtGui/QPainter>
#include <QLineSeries>
#include <QChart>
#include <QChartView>
#include <QAreaSeries>
#include <QCategoryAxis>
#include <QFileInfo>

#include <iostream>
#include <unordered_map>
#include <signal.h>
#include <unordered_set>

using namespace QtCharts;

Chart::Chart(const QString &db):
  db(db)
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

  // chart configuration
  MemoryType type = MemoryType::Rss;

  qint64 measurementFrom = 0;
  qint64 measurementTo = storage.measurementCount();
  qint64 pointCount = std::min((qint64)1024, measurementTo - measurementFrom);

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
  QTime time;
  time.start();
  MeasurementGroups peak;
  for (qint64 step = 0; step < pointCount; step ++){
    Measurement measurement;
    storage.getMemoryPeak(measurement, type,
      measurementFrom + step * stepSize,
      measurementFrom + (step * stepSize) + stepSize);

    Utils::group(measurements[step], measurement, type, true);
    if (peak.sum < measurements[step].sum) {
      peak = measurements[step];
    }
  }
  qDebug() << "getting data: " << time.elapsed() << "ms";

  time.restart();

  std::vector<QAreaSeries*> chartSeries;
  for (int i = 0; i < 5; i++) {
    chartSeries.push_back(new QAreaSeries(new QLineSeries(), new QLineSeries()));
  }
  chartSeries[0]->setName("thread stacks");
  chartSeries[1]->setName("anonymous");
  chartSeries[2]->setName("heap");
  chartSeries[3]->setName("sockets");
  chartSeries[4]->setName("other mappings");

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
    chartSeries[0]->lowerSeries()->append(step, seriesOffset);
    seriesOffset += measurement.threadStacks;
    chartSeries[0]->upperSeries()->append(step, seriesOffset);

    chartSeries[1]->lowerSeries()->append(step, seriesOffset);
    seriesOffset += measurement.anonymous;
    chartSeries[1]->upperSeries()->append(step, seriesOffset);

    chartSeries[2]->lowerSeries()->append(step, seriesOffset);
    seriesOffset += measurement.heap;
    chartSeries[2]->upperSeries()->append(step, seriesOffset);

    chartSeries[3]->lowerSeries()->append(step, seriesOffset);
    seriesOffset += measurement.sockets;
    chartSeries[3]->upperSeries()->append(step, seriesOffset);

    size_t otherMappings = 0;
    for (const auto &mapping: measurement.mappings){
      if (significantMappingSet.find(mapping.first) == significantMappingSet.end()){
        otherMappings += mapping.second;
      }
    }
    chartSeries[4]->lowerSeries()->append(step, seriesOffset);
    seriesOffset += otherMappings;
    chartSeries[4]->upperSeries()->append(step, seriesOffset);

    int i = 5;
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

  QString db = app.arguments().size() > 1 ? app.arguments()[1] : "measurement.db";

  Chart *peak = new Chart(db);
  QMetaObject::invokeMethod(peak, "run", Qt::QueuedConnection);

  app.setQuitOnLastWindowClosed(false);
  QObject::connect(&app, SIGNAL(lastWindowClosed()), peak, SLOT(deleteLater()));

  int result = app.exec();
  qDebug() << "Main loop ended...";
  return result;
}
