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

bool operator<(const RangeKey& a, const RangeKey& b)
{
  if (a.from != b.from)
    return a.from < b.from;
  if (a.to != b.to)
    return a.to < b.to;
  return a.permission < b.permission;
}

Feeder::Feeder(std::atomic_int &queueSize):
  queueSize(queueSize)
{
}

Feeder::~Feeder()
{
}

void Feeder::onProcessSnapshot(QDateTime time, QList<SmapsRange> ranges, StatM statm)
{
  QMap<RangeKey, qlonglong> currentRanges;
  queueSize--;

  storage.transaction();

  qlonglong rssSum = 0;
  qlonglong pssSum = 0;
  std::vector<MeasurementData> measurements;
  for (const auto &r:ranges){
    RangeKey key;
    key.from = r.from;
    key.to = r.to;
    key.permission = r.permission;

    qlonglong rangeId;
    auto it = lastRanges.find(key);
    if (it == lastRanges.end()){
      rangeId = storage.insertRange(r.from, r.to, r.permission, r.name);
    }else{
      rangeId = it.value();
    }

    currentRanges[key] = rangeId;
    MeasurementData m;
    m.rangeId = rangeId;
    m.rss = r.rss;
    m.pss = r.pss;
    rssSum += r.rss;
    pssSum += r.pss;
    measurements.push_back(m);
  }
  qlonglong measurementId = storage.insertMeasurement(time, rssSum, pssSum, statm);
  storage.insertData(measurementId, measurements);
  if (!storage.commit()){
    qWarning() << "Failed to commit measurement";
  }

  lastRanges = currentRanges;
}

bool Feeder::init(QString file)
{
  return storage.init(file);
}
