
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

Feeder::Feeder()
{
}

Feeder::~Feeder()
{
}

void Feeder::onSnapshot(QDateTime time, QList<SmapsRange> ranges)
{
  QMap<RangeKey, qlonglong> currentRanges;

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
  qlonglong measurementId = storage.insertMeasurement(time, rssSum, pssSum);
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
