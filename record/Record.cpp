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

#include "MemoryWatcher.h"
#include "Record.h"

#include <CmdLineParsing.h>
#include <ThreadPool.h>
#include <Utils.h>
#include <Version.h>

#include <QtCore/QCoreApplication>
#include <QDebug>

#include <iostream>
#include <signal.h>

void Record::close()
{
  qDebug() << "close()";
  watcher->deleteLater();
  feeder->deleteLater();
  threadPool.close();
}

Record::Record(long pid, long period, QString databaseFile):
  watcherThread(threadPool.makeThread("watcher")),
  watcher(new MemoryWatcher(watcherThread, pid, period, queueSize)),
  feeder(new Feeder(queueSize))
{
  qRegisterMetaType<StatM>();

  connect(&threadPool, &ThreadPool::closed, this, &Record::deleteLater);

  // init watcher
  watcher->moveToThread(watcherThread);
  QObject::connect(watcherThread, &QThread::started,
                   watcher, &MemoryWatcher::init);
  watcherThread->start();

  if (!feeder->init(databaseFile)){
    close();
    return;
  }

  connect(watcher, &MemoryWatcher::snapshot,
          feeder, &Feeder::onSnapshot,
          Qt::QueuedConnection);

  // for debug
  // shutdownTimer.setSingleShot(true);
  // shutdownTimer.setInterval(10000);
  // connect(&shutdownTimer, SIGNAL(timeout()), this, SLOT(close()));
  // shutdownTimer.start();
}

Record::~Record()
{
  qDebug() << "~Watcher";
  QCoreApplication::quit();
}

struct Arguments {
  bool help{false};
  bool version{false};
  long pid{-1};
  long period{1000};
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
                  "pid",
                  "Pid of monitored process, if not defined, all processes are monitored");

    AddOption(CmdLineULongOption([this](const unsigned long &value) {
                    args.period = value;
                  }),
                  "period",
                  "Period of snapshot [ms], default "s + std::to_string(args.period));

    AddOption(CmdLineStringOption([this](const std::string &value){
      args.databaseFile = QString::fromStdString(value);
    }),
              "database-file",
              "Sqlite database file for storing recording. Default is measurement${PID}.db");
  }

  Arguments GetArguments() const {
    return args;
  }
};

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  qRegisterMetaType<QList<SmapsRange>>("QList<SmapsRange>");

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
    args.databaseFile = QString("measurement.%1.db").arg(args.pid);
  }

  Record *record = new Record(args.pid, args.period, args.databaseFile);
  std::function<void(int)> signalCallback = [&](int){ record->close(); };
  Utils::catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP},
                          &signalCallback);

  int result = app.exec();
  Utils::cleanSignalCallback();
  qDebug() << "Main loop ended...";
  return result;
}
