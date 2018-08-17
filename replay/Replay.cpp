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

#include <Utils.h>
#include <ThreadPool.h>

#include <QtCore/QCoreApplication>
#include <QDebug>

#include <iostream>
#include <unordered_map>
#include <signal.h>

Replay::Replay(const QString &db,
               int interval,
               int idStep):
  db(db),
  interval(interval),
  idStep(idStep)
{}

Replay::~Replay()
{
  QCoreApplication::quit();
}

void Replay::run()
{
  if (!storage.init(db)){
    qWarning() << "Failed to open database" << db;
    deleteLater();
    return;
  }
  if (!storage.getMeasuremntRange(currentMeasurement, lastMeasurement)){
    qWarning() << "Failed to get measurement range" << db;
    deleteLater();
    return;
  }

  timer.setSingleShot(false);
  timer.setInterval(interval);
  connect(&timer, SIGNAL(timeout()), this, SLOT(step()));
  timer.start();
}

void Replay::step()
{

  if (!storage.getMeasurement(measurement, currentMeasurement, /*cache*/ true)){
    qWarning() << "Failed to read measurement";
    deleteLater();
    return;
  }

  Utils::clearScreen();
  Utils::printMeasurement(measurement, type);

  if (currentMeasurement == lastMeasurement){
    timer.stop();
    deleteLater();
  }
  currentMeasurement += idStep;
}

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);

  QString db = app.arguments().size() > 1 ? app.arguments()[1] : "measurement.db";

  Replay *replay = new Replay(db);
  QMetaObject::invokeMethod(replay, "run", Qt::QueuedConnection);

  int result = app.exec();
  qDebug() << "Main loop ended...";
  return result;
}
