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

#include "Feeder.h"

#include <QDebug>

void Feeder::onSnapshot(ProcessId processId, QDateTime time, QList<SmapsRange> ranges)
{
  storage.transaction();

  storage.insertOrIgnoreProcess(processId, "foo");

  qlonglong rssSum = 0;
  qlonglong pssSum = 0;
  for (const auto &r:ranges){
    storage.insertOrIgnoreRange(r.key);
    rssSum += r.rss;
    pssSum += r.pss;

  }
  storage.insertMeasurement(processId, time, rssSum, pssSum, StatM{}, OomScore{});
  storage.insertData(processId, time, ranges);
  if (!storage.commit()){
    qWarning() << "Failed to commit measurement";
  }

  emit inserted();
}

bool Feeder::init(QString file)
{
  return storage.init(file);
}
