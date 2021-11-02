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

#include <QtCore/QCoreApplication>
#include <QDebug>

#include <iostream>

void Load::close()
{
  qDebug() << "close()";
  loader->deleteLater();
  feeder->deleteLater();
  this->deleteLater();
}

Load::Load(long pid, const QString &smapsFile, const QString &databaseFile):
  loader(new MemoryLoader(pid, smapsFile)),
  feeder(new Feeder())
{
  if (!feeder->init(databaseFile)){
    close();
    return;
  }

  connect(loader, &MemoryLoader::loaded,
          feeder, &Feeder::onSnapshot,
          Qt::QueuedConnection);

  connect(feeder, &Feeder::inserted,
          this, &Load::close);

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
  QMetaObject::invokeMethod(loader, "init", Qt::QueuedConnection);
#else
  QMetaObject::invokeMethod(loader, &MemoryLoader::init, Qt::QueuedConnection);
#endif
}

Load::~Load()
{
  qDebug() << "~Load";
  QCoreApplication::quit();
}

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  Utils::registerQtMetatypes();

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

  int result = app.exec();
  qDebug() << "Main loop ended...";
  return result;
}
