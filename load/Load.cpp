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

#include "MemoryLoader.h"
#include "Load.h"

#include <Utils.h>
#include <ThreadPool.h>

#include <QtCore/QCoreApplication>
#include <QDebug>

#include <iostream>
#include <signal.h>


void Load::close()
{
  qDebug() << "close()";
  loader->deleteLater();
  feeder->deleteLater();
  threadPool.close();
}

Load::Load(long pid, const QString &smapsFile, const QString &databaseFile):
  loaderThread(threadPool.makeThread("watcher")),
  loader(new MemoryLoader(loaderThread, pid, smapsFile)),
  feeder(new Feeder())
{
  qRegisterMetaType<StatM>();

  connect(&threadPool, &ThreadPool::closed, this, &Load::deleteLater);

  // init watcher
  loader->moveToThread(loaderThread);
  QObject::connect(loaderThread, &QThread::started,
                   loader, &MemoryLoader::init);

  if (!feeder->init(databaseFile)){
    close();
    return;
  }

  connect(loader, &MemoryLoader::loaded,
          feeder, &Feeder::onSnapshot,
          Qt::QueuedConnection);

  connect(feeder, &Feeder::inserted,
          this, &Load::close);

  loaderThread->start();
  // for debug
  // shutdownTimer.setSingleShot(true);
  // shutdownTimer.setInterval(10000);
  // connect(&shutdownTimer, SIGNAL(timeout()), this, SLOT(close()));
  // shutdownTimer.start();
}

Load::~Load()
{
  qDebug() << "~Watcher";
  QCoreApplication::quit();
}

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  qRegisterMetaType<QList<SmapsRange>>("QList<SmapsRange>");

  if (app.arguments().size() < 3){
    std::cerr << "Usage:" << std::endl;
    std::cerr << app.applicationName().toStdString() << " PID smapsFile [database-file]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "  default database file is measurement.${PID}.db" << std::endl;
    return 1;
  }

  long pid = app.arguments()[1].toLong();
  QString smapsFile = app.arguments()[2];

  QString databaseFile;
  if (app.arguments().size() >= 4){
    databaseFile = app.arguments()[3];
  }
  if (databaseFile.isEmpty()) {
    databaseFile = QString("measurement.%1.db").arg(pid);
  }

  new Load(pid, smapsFile, databaseFile);
  /*
  std::function<void(int)> signalCallback = [&](int){ record->close(); };
  Utils::catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP},
                          &signalCallback);
  */

  int result = app.exec();
  //Utils::cleanSignalCallback();
  qDebug() << "Main loop ended...";
  return result;
}
