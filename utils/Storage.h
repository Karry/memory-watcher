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
#include "Utils.h"
#include "MemInfo.h"

#include <QtCore/QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QMap>

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

  bool insertOrIgnoreProcess(const ProcessId &processId, const QString &name);

  qlonglong insertOrIgnoreRange(const SmapsRange::Key &range);

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

  bool insertSystemMemInfo(const QDateTime &time, const MemInfo &memInfo);

  qint64 measurementCount();

  bool lookupPid(pid_t pid, QMap<ProcessId, QString> &processes);

  bool getProcess(qulonglong processId, pid_t &pid, QString &processName);

  bool getMemoryPeak(qulonglong processId,
                     Measurement &measurement,
                     ProcessMemoryType type = Rss);

  bool getMeasurementAtOrBefore(qulonglong processId,
                                const QDateTime &time,
                                Measurement &measurement,
                                bool cacheRanges);

  bool getMeasurementAt(qulonglong processId,
                        const QDateTime &time,
                        Measurement &measurement,
                        bool cacheRanges = false);

  bool getMeasurement(Measurement &measurement, qlonglong &id, bool cacheRanges = false);

  bool getSystemMemoryPeak(SystemMemoryType memoryType,
                           QDateTime &time,
                           MemInfo &memInfo,
                           QList<Measurement> &processes);

  bool getSystemMemoryAtOrBefore(const QDateTime &time,
                                 QDateTime &exactTime,
                                 MemInfo &memInfo,
                                 QList<Measurement> &processes);

  bool getSystemMemoryAt(const QDateTime &time,
                         MemInfo &memInfo,
                         QList<Measurement> &processes);

  bool getMeasurementTimes(qulonglong processId, QList<QDateTime> &times);

  bool getMeasurementTimes(QList<QDateTime> &times);

  bool getAllRanges(qulonglong processId, QMap<qulonglong, Range> &rangeMap);

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
  bool execAndGetSystemMemory(QSqlQuery &sql, QDateTime &time, MemInfo &memInfo, QList<Measurement> &processes);
  bool execAndGetMeasurement(Measurement &measurement, QSqlQuery &measurementQuery, bool cacheRanges);
  bool getMeasurement(Measurement &measurement, QSqlQuery &measurementQuery, bool cacheRanges);
  bool getRanges(QMap<qulonglong, Range> &rangeMap, QSqlQuery &sql);

private:
  QSqlDatabase db;
  QSqlQuery sqlProcessInsert;
  QSqlQuery sqlRangeInsert;
  QSqlQuery sqlMeasurementInsert;
  QSqlQuery sqlDataInsert;
  QSqlQuery sqlSystemInsert;
};

