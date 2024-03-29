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

#include "Replay.h"

#include <CmdLineParsing.h>
#include <Utils.h>
#include <ThreadPool.h>
#include <Version.h>

#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QFileInfo>

#include <iostream>
#include <optional>

struct Arguments {
  bool help{false};
  bool version{false};
  std::optional<long> pid;
  std::optional<qulonglong> processId;
  unsigned long interval{1000};
  QString databaseFile;
  QString processMemoryType{"pss"};
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
                args.interval = value;
              }),
              "interval",
              "Step interval [ms]. Default is "s + std::to_string(args.interval));

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

    AddOption(CmdLineStringOption([this](const std::string &value){
                args.processMemoryType = QString::fromStdString(value);
              }),
              "process-memory",
              "Type of process memory used for sorting."s
                "\n\tpss (default) - Proportional set size as sum of pss values from /proc/[pid]/smaps"s
                "\n\trss - Resident set size as sum of rss values from /proc/[pid]/smaps"s
                "\n\tstatm - Resident set size as provided in /proc/[pid]/statm");

  }

  Arguments GetArguments() const {
    return args;
  }
};


Replay::Replay(const QString &db,
               std::optional<pid_t> pid,
               std::optional<qulonglong> processId,
               unsigned long interval,
               ProcessMemoryType processType):
  db(db),
  pid(pid),
  processId(processId),
  interval(interval),
  type(processType)
{}

Replay::~Replay()
{
  QCoreApplication::quit();
}

void Replay::run()
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

  if (processId.has_value()) {
    if (!storage.getMeasurementTimes(processId.value(), times)) {
      qWarning() << "Failed to get measurement time points" << db;
      deleteLater();
      return;
    }
  } else {
    if (!storage.getMeasurementTimes(times)) {
      qWarning() << "Failed to get measurement time points" << db;
      deleteLater();
      return;
    }
  }

  timer.setSingleShot(false);
  timer.setInterval(interval);
  connect(&timer, &QTimer::timeout, this, &Replay::step);
  timer.start();
  step();
}

void Replay::step()
{
  if (int(cursor) >= times.size()){
    timer.stop();
    deleteLater();
    return;
  }

  if (processId.has_value()) {
    if (!storage.getMeasurementAt(processId.value(), times[cursor], measurement, true)) {
      qWarning() << "Failed to read measurement";
      deleteLater();
      return;
    }

    Utils::clearScreen();
    Utils::printMeasurement(measurement, type);
  } else {
    QDateTime time = times[cursor];
    QList<Measurement> processes;
    MemInfo memInfo;
    if (!storage.getSystemMemoryAt(time, memInfo, processes)) {
      qWarning() << "Failed to read memory peak";
      deleteLater();
      return;
    }

    // Utils::printMeasurementSmapsLike(measurement);
    // std::cout << std::endl << std::endl;
    Utils::clearScreen();
    Utils::printProcesses(time, memInfo, processes, type);
  }

  cursor++;
}

QMap<QString, ProcessMemoryType> memoryTypes {
  {"rss",   ProcessMemoryType::Rss},
  {"pss",   ProcessMemoryType::Pss},
  {"statm", ProcessMemoryType::StatmRss},
};

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
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

  ProcessMemoryType processType{Rss};
  if (!memoryTypes.contains(args.processMemoryType)){
    qWarning() << "Dont understand to memory type" << args.processMemoryType;
    return 1;
  }
  processType = memoryTypes[args.processMemoryType];

  Replay *replay = new Replay(args.databaseFile,
                              args.pid,
                              args.processId,
                              args.interval,
                              processType);

  QMetaObject::invokeMethod(replay, "run", Qt::QueuedConnection);

  int result = app.exec();
  qDebug() << "Main loop ended...";
  return result;
}
