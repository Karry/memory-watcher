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

Record::Record(long pid, long period):
  watcherThread(threadPool.makeThread("watcher")),
  watcher(new MemoryWatcher(watcherThread, pid, period)),
  feeder(new Feeder())
{
  connect(&threadPool, SIGNAL(closed()), this, SLOT(deleteLater()));

  // init watcher
  watcher->moveToThread(watcherThread);
  QObject::connect(watcherThread, SIGNAL(started()),
                   watcher, SLOT(init()));
  watcherThread->start();

  if (!feeder->init(QString("measurement.%1.db").arg(pid))){
    close();
    return;
  }

  connect(watcher, SIGNAL(snapshot(QDateTime, QList<SmapsRange>)),
          feeder, SLOT(onSnapshot(QDateTime, QList<SmapsRange>)),
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
    std::cerr << app.applicationName().toStdString() << " PID [period-ms]" << std::endl;
    return 1;
  }

  long pid = app.arguments()[1].toLong();
  long period = 1000;
  if (app.arguments().size() >= 3){
    period = app.arguments()[2].toLong();
  }

  Record *record = new Record(pid, period);
  std::function<void(int)> signalCallback = [&](int){ record->close(); };
  Utils::catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP},
                          &signalCallback);

  int result = app.exec();
  Utils::cleanSignalCallback();
  qDebug() << "Main loop ended...";
  return result;
}
