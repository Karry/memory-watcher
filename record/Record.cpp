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

#include <Utils.h>
#include <ThreadPool.h>

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

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  qRegisterMetaType<QList<SmapsRange>>("QList<SmapsRange>");

  if (app.arguments().size() < 2){
    std::cerr << "Usage:" << std::endl;
    std::cerr << app.applicationName().toStdString() << " PID [period-ms] [database-file]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "  default period is 1 second" << std::endl;
    std::cerr << "  default database file is measurement.${PID}.db" << std::endl;
    return 1;
  }

  long pid = app.arguments()[1].toLong();
  long period = 1000;
  if (app.arguments().size() >= 3){
    period = app.arguments()[2].toLong();
  }

  QString databaseFile;
  if (app.arguments().size() >= 4){
    databaseFile = app.arguments()[3];
  }
  if (databaseFile.isEmpty()) {
    databaseFile = QString("measurement.%1.db").arg(pid);
  }

  Record *record = new Record(pid, period, databaseFile);
  std::function<void(int)> signalCallback = [&](int){ record->close(); };
  Utils::catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP},
                          &signalCallback);

  int result = app.exec();
  Utils::cleanSignalCallback();
  qDebug() << "Main loop ended...";
  return result;
}
