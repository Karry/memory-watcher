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

#include "ProcessMemoryWatcher.h"
#include "Record.h"

#include <CmdLineParsing.h>
#include <ThreadPool.h>
#include <Utils.h>
#include <Version.h>

#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QDirIterator>

#include <iostream>
#include <signal.h>

void Record::close()
{
  qDebug() << "close()";
  timer.stop();
  for (ProcessMemoryWatcher *watcher: watchers.values()) {
    watcher->deleteLater();
  }
  watchers.clear();
  for (auto *t: watcherThreads) {
    t->quit(); // thread is deleted on finish
  }
  watcherThreads.clear();
  threadPool.close();
}

Record::Record(QSet<long> pids,
               long period,
               QString databaseFile,
               QString procFs):
  monitorSystem(pids.empty()),
  procFs(procFs)
{
  connect(&threadPool, &ThreadPool::closed, this, &Record::deleteLater);

  timer.setSingleShot(false);
  timer.setInterval(period);

  connect(&timer, &QTimer::timeout, this, &Record::update);

  timer.start();

  if (!feeder.init(databaseFile)){
    close();
    return;
  }

  watcherThreads.reserve(QThread::idealThreadCount());
  for (int i = 0; i < QThread::idealThreadCount(); i++) {
    QThread *t = threadPool.makeThread(QString("watcher-%1").arg(i));
    t->start();
    watcherThreads.push_back(t);
  }

  if (monitorSystem) {
    updateProcessList();
  } else {
    for (pid_t pid: pids) {
      startProcessMonitor(pid);
    }
  }

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

void Record::startProcessMonitor(pid_t pid) {
  assert(!watcherThreads.empty());
  QThread *watcherThread = watcherThreads[ nextThread++ % watcherThreads.size()];
  ProcessMemoryWatcher *watcher = new ProcessMemoryWatcher(watcherThread, pid, procFs);

  connect(watcher, &ProcessMemoryWatcher::snapshot,
          &feeder, &Feeder::onProcessSnapshot,
          Qt::QueuedConnection);

  connect(this, &Record::updateRequest,
          watcher, &ProcessMemoryWatcher::update,
          Qt::QueuedConnection);

  connect(watcher, &ProcessMemoryWatcher::initialized,
          &feeder, &Feeder::processInitialized,
          Qt::QueuedConnection);

  connect(watcher, &ProcessMemoryWatcher::initialized,
          this, &Record::processInitialized,
          Qt::QueuedConnection);

  connect(watcher, &ProcessMemoryWatcher::exited,
          this, &Record::processExited,
          Qt::QueuedConnection);

  connect(watcherThread, &QThread::finished,
          watcher, &ProcessMemoryWatcher::deleteLater);

  watchers[pid] = watcher;

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
  QMetaObject::invokeMethod(watcher, "init", Qt::QueuedConnection);
#else
  QMetaObject::invokeMethod(watcher, &ProcessMemoryWatcher::init, Qt::QueuedConnection);
#endif
}

void Record::processInitialized(ProcessId processId, QString name) {
  qDebug() << "Process" << processId.pid << "(hash" << processId.hash() << ") initialized:" << name;
}

void Record::processExited(ProcessId processId) {
  auto it = watchers.find(processId.pid);
  if (it != watchers.end()) {
    qDebug() << "Process" << processId.pid << "(hash" << processId.hash() << ") terminated";
    it.value()->deleteLater();
    watchers.remove(processId.pid);
  }
}

void Record::updateProcessList() {
  QDirIterator it(procFs);
  bool ok;
  while (it.hasNext()) {
    QFileInfo dir(it.next());
    auto pid = dir.fileName().toULongLong(&ok);
    if (ok && pid <= std::numeric_limits<pid_t>::max() && !watchers.contains(pid)) {
      startProcessMonitor(pid);
    }
  }
}

void Record::update() {
  if (monitorSystem) {
    updateProcessList();
  }
  qDebug() << "tick, watching" << watchers.size() << "processes";
  emit updateRequest(QDateTime::currentDateTime());
}

struct Arguments {
  bool help{false};
  bool version{false};
  QSet<long> pids;
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
                    args.pids.insert(value);
                  }),
                  std::vector<std::string>{"p","pid"},
                  "Pid of monitored process. May be defined multiple times. "s +
                  "If not defined, all processes are monitored."s);

    AddOption(CmdLineULongOption([this](const unsigned long &value) {
                    args.period = value;
                  }),
                  "period",
                  "Period of snapshot [ms], default "s + std::to_string(args.period));

    AddOption(CmdLineStringOption([this](const std::string &value){
                    args.databaseFile = QString::fromStdString(value);
                  }),
              "database-file",
              "Sqlite database file for storing recording. Default is measurement.db");
  }

  Arguments GetArguments() const {
    return args;
  }
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

  Record *record = new Record(args.pids, args.period, args.databaseFile);
  std::function<void(int)> signalCallback = [&](int){
    Utils::cleanSignalCallback();
    qDebug() << "closing";
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    QMetaObject::invokeMethod(record, "close", Qt::QueuedConnection);
#else
    QMetaObject::invokeMethod(record, &Record::close, Qt::QueuedConnection);
#endif
  };
  Utils::catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP},
                          &signalCallback);

  int result = app.exec();
  qDebug() << "Main loop ended...";
  return result;
}
