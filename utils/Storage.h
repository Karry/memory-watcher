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

#pragma once

#include "StatM.h"
#include "ProcessId.h"
#include "SmapsRange.h"
#include "OomScore.h"

#include <QtCore/QObject>
#include <QSqlDatabase>
#include <QDateTime>
#include <QMap>

class MeasurementData
{
public:
  qlonglong rangeId{0};
  qlonglong rss{0};
  qlonglong pss{0};
};

class Range {
public:
  qlonglong from{0};
  qlonglong to{0};
  QString permission;
  QString name;
};

class Measurement {
public:
  qlonglong id{0};
  QDateTime time;
  StatM statm;
  QMap<qlonglong, Range> rangeMap;
  QList<MeasurementData> data;
};

enum MemoryType {
  // SMaps
  Rss,
  Pss,
  // statm
  StatmRss
};

class Storage : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Storage)

signals:
public slots:
public:
  Storage() = default;
  ~Storage();

  bool updateSchema();
  bool init(QString file);

  bool insertProcess(const ProcessId &processId, const QString &name);

  qlonglong insertOrUpdateRange(const SmapsRange::Key &range);

  qlonglong insertMeasurement(const ProcessId &processId,
                              const QDateTime &time,
                              qlonglong rss,
                              qlonglong pss,
                              const StatM &statm,
                              const OomScore &oomScore);

  bool insertData(const ProcessId &processId,
                  const QDateTime &time,
                  const QList<SmapsRange> &ranges);

  bool insertData(const std::vector<MeasurementData> &measurements);

  qint64 measurementCount();

  bool getMemoryPeak(Measurement &measurement,
                     MemoryType type = Rss,
                     qint64 from = std::numeric_limits<qint64>::min(),
                     qint64 to = std::numeric_limits<qint64>::max());

  bool getMeasurement(Measurement &measurement, qlonglong &id, bool cacheRanges = false);

  bool getMeasuremntRange(qlonglong &min,
                          qlonglong &max);

  bool getAllRanges(QMap<qlonglong, Range> &rangeMap);

  bool transaction()
  {
    return db.transaction();
  }

  bool commit()
  {
    return db.commit();
  }

  bool rollback()
  {
    return db.rollback();
  }

private:
  bool getMeasurement(Measurement &measurement, QSqlQuery &measurementQuery, bool cacheRanges = false);
  bool getRanges(QMap<qlonglong, Range> &rangeMap, QSqlQuery &sql);

private:
  QSqlDatabase db;
};

