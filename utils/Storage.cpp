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

#include "Storage.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

#include "QVariantConverters.h"

using namespace converters;

Storage::~Storage()
{
  if (db.isValid()) {
    if (db.isOpen()) {
      db.close();
    }
    db = QSqlDatabase(); // invalidate instance
  }
  qDebug() << "Storage is closed";
}

bool Storage::updateSchema()
{
  QStringList tables = db.tables();

  if (!tables.contains("process")) {

    QString sql("CREATE TABLE `process`");
    sql.append("(").append( "`id` INTEGER PRIMARY KEY "); // hash of pid and start_time
    sql.append(",").append( "`pid` INTEGER NOT NULL "); // system process id
    sql.append(",").append( "`start_time` "); // from /proc/<pid>/stat
    sql.append(",").append( "`name` varchar(255) NULL ");
    sql.append(");");

    QSqlQuery q = db.exec(sql);
    if (q.lastError().isValid()) {
      qWarning() << "Creating table process failed" << q.lastError();
      db.close();
      return false;
    }
  }

  if (!tables.contains("memory_range")) {

    QString sql("CREATE TABLE `memory_range`");
    sql.append("(").append( "`id` INTEGER PRIMARY KEY "); // hash of process_id, from, to and permission columns
    sql.append(",").append( "`process_id` INTEGER NOT NULL REFERENCES process(id) ON DELETE CASCADE ");
    sql.append(",").append( "`from` INTEGER NOT NULL ");
    sql.append(",").append( "`to` INTEGER NOT NULL ");
    sql.append(",").append( "`permission` varchar(255) NULL ");
    sql.append(",").append( "`name` varchar(255) NULL ");
    sql.append(");");

    QSqlQuery q = db.exec(sql);
    if (q.lastError().isValid()) {
      qWarning() << "Creating table memory_range failed" << q.lastError();
      db.close();
      return false;
    }
  }

  if (!tables.contains("measurement")) {

    QString sql("CREATE TABLE `measurement`");
    sql.append("(").append( "`id` INTEGER PRIMARY KEY "); // hash of process_id and time
    sql.append(",").append( "`process_id` INTEGER NOT NULL REFERENCES process(id) ON DELETE CASCADE ");
    sql.append(",").append( "`time` datetime NOT NULL ");
    sql.append(",").append( "`rss_sum` INTEGER NOT NULL ");
    sql.append(",").append( "`pss_sum` INTEGER NOT NULL ");

    sql.append(",").append( "`oom_adj` INTEGER NOT NULL ");
    sql.append(",").append( "`oom_score` INTEGER NOT NULL ");
    sql.append(",").append( "`oom_score_adj` INTEGER NOT NULL ");

    sql.append(",").append( "`statm_size` INTEGER NOT NULL ");
    sql.append(",").append( "`statm_resident` INTEGER NOT NULL ");
    sql.append(",").append( "`statm_shared` INTEGER NOT NULL ");
    sql.append(",").append( "`statm_text` INTEGER NOT NULL ");
    sql.append(",").append( "`statm_lib` INTEGER NOT NULL ");
    sql.append(",").append( "`statm_data` INTEGER NOT NULL ");
    sql.append(",").append( "`statm_dt` INTEGER NOT NULL ");

    sql.append(");");

    QSqlQuery q = db.exec(sql);
    if (q.lastError().isValid()) {
      qWarning() << "Creating measurement table failed" << q.lastError();
      db.close();
      return false;
    }
  }

  if (!tables.contains("data")) {

    QString sql("CREATE TABLE `data`");
    sql.append("(").append( "`range_id` INTEGER NOT NULL REFERENCES memory_range(id) ON DELETE CASCADE");
    sql.append(",").append( "`measurement_id` INTEGER NOT NULL REFERENCES measurement(id) ON DELETE CASCADE");
    sql.append(",").append( "`rss` INTEGER NOT NULL ");
    sql.append(",").append( "`pss` INTEGER NOT NULL ");
    sql.append(");");

    //

    QSqlQuery q = db.exec(sql);
    if (q.lastError().isValid()) {
      qWarning() << "Creating data table failed" << q.lastError();
      db.close();
      return false;
    }

    QSqlQuery q2 = db.exec("CREATE INDEX idx_data_measurement_id ON data(measurement_id);");
    if (q2.lastError().isValid()) {
      qWarning() << "Creating data index failed" << q2.lastError();
      db.close();
      return false;
    }
  }

  return true;
}

bool Storage::init(QString file)
{
  // Find QSLite driver
  db = QSqlDatabase::addDatabase("QSQLITE", "storage");
  if (!db.isValid()){
    qWarning() << "Could not find QSQLITE backend";
    return false;
  }

  db.setDatabaseName(file);
  if (!db.open()){
    qWarning() << "Open database failed" << db.lastError();
    return false;
  }
  qDebug() << "Storage database opened:" << file;
  if (!updateSchema()){
    return false;
  }

  QSqlQuery q = db.exec("PRAGMA foreign_keys = ON;");
  if (q.lastError().isValid()){
    qWarning() << "Enabling foreign keys fails:" << q.lastError();
  }

  return db.isValid() && db.isOpen();
}

bool Storage::insertProcess(const ProcessId &processId, const QString &name) {
  QSqlQuery sql(db);
  sql.prepare("INSERT INTO `process` (`id`, `pid`, `start_time`, `name`) VALUES (:id, :pid, :start_time, :name)");
  sql.bindValue(":id", processId.hash());
  sql.bindValue(":pid", processId.pid);
  sql.bindValue(":start_time", processId.startTime);
  sql.bindValue(":name", name);

  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Insert process failed" << sql.lastError();
    return false;
  }
  return true;
}

qlonglong Storage::insertOrUpdateRange(const SmapsRange::Key &range)
{
  QSqlQuery sql(db);
  sql.prepare("INSERT OR REPLACE INTO `memory_range` (`id`, `process_id`, `from`, `to`, `permission`, `name`) VALUES (:id, :process_id, :from, :to, :permission, :name)");
  sql.bindValue(":id", range.hash());
  sql.bindValue(":process_id", range.processId.hash());
  sql.bindValue(":from", qlonglong(range.from));
  sql.bindValue(":to", qlonglong(range.to));
  sql.bindValue(":permission", range.permission);
  sql.bindValue(":name", range.name);

  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Insert range failed" << sql.lastError();
    return 0;
  }

  return varToLong(sql.lastInsertId());
}

qlonglong measurementHash(const ProcessId &processId,
                          const QDateTime &time) {
  return processId.hash() ^ qHash(time);
}

qlonglong Storage::insertMeasurement(const ProcessId &processId,
                                     const QDateTime &time,
                                     qlonglong rss,
                                     qlonglong pss,
                                     const StatM &statm,
                                     const OomScore &oomScore)
{


  QSqlQuery sql(db);
  sql.prepare("INSERT INTO `measurement` ("
              "  `id`, `process_id`, `time`, `rss_sum`, `pss_sum`,"
              "  `oom_adj`, `oom_score`, `oom_score_adj`, "
              "  `statm_size`, `statm_resident`, `statm_shared`, `statm_text`, `statm_lib`, `statm_data`, `statm_dt` "
              ") VALUES ("
              "  :id, :process_id, :time, :rss, :pss, "
              "  :oom_adj, :oom_score, :oom_score_adj, "
              "  :statm_size, :statm_resident, :statm_shared, :statm_text, :statm_lib, :statm_data, :statm_dt"
              ")");
  db.transaction();

  sql.bindValue(":id", measurementHash(processId, time));
  sql.bindValue(":process_id", processId.hash());

  sql.bindValue(":time", time);
  sql.bindValue(":rss", rss);
  sql.bindValue(":pss", pss);

  sql.bindValue(":oom_adj", oomScore.adj);
  sql.bindValue(":oom_score", oomScore.score);
  sql.bindValue(":oom_score_adj", oomScore.score_adj);

  sql.bindValue(":statm_size", (qlonglong)statm.size);
  sql.bindValue(":statm_resident", (qlonglong)statm.resident);
  sql.bindValue(":statm_shared", (qlonglong)statm.shared);
  sql.bindValue(":statm_text", (qlonglong)statm.text);
  sql.bindValue(":statm_lib", (qlonglong)statm.lib);
  sql.bindValue(":statm_data", (qlonglong)statm.data);
  sql.bindValue(":statm_dt", (qlonglong)statm.dt);

  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Insert measurement failed" << sql.lastError();
    return 0;
  }

  return varToLong(sql.lastInsertId());
}

bool Storage::insertData(const ProcessId &processId,
                         const QDateTime &time,
                         const QList<SmapsRange> &ranges)
{
  qlonglong measurementId = measurementHash(processId, time);
  QSqlQuery sql(db);
  sql.prepare("INSERT INTO `data` (`range_id`, `measurement_id`, `rss`, `pss`) VALUES (:range_id, :measurement_id, :rss, :pss)");
  db.transaction();
  for (const auto &m: ranges) {
    sql.bindValue(":range_id", m.key.hash());
    sql.bindValue(":measurement_id", measurementId);
    sql.bindValue(":rss", qlonglong(m.rss));
    sql.bindValue(":pss", qlonglong(m.pss));

    sql.exec();
    if (sql.lastError().isValid()) {
      qWarning() << "Insert data failed" << sql.lastError();
      return false;
    }
  }

  return true;
}

bool Storage::getRanges(QMap<qlonglong, Range> &rangeMap, QSqlQuery &sql)
{
  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Select ranges failed" << sql.lastError();
    return false;
  }
  rangeMap.clear();
  while (sql.next()){
    Range range;
    range.from = varToLong(sql.value("from"));
    range.to = varToLong(sql.value("to"));
    range.permission = varToString(sql.value("permission"));
    range.name = varToString(sql.value("name"));
    rangeMap[varToLong(sql.value("id"))] = range;
  }
  return true;
}

bool Storage::getAllRanges(QMap<qlonglong, Range> &rangeMap)
{
  QSqlQuery sql(db);

  sql.prepare("SELECT * FROM `range`;");
  return getRanges(rangeMap, sql);
}

bool Storage::getMeasurement(Measurement &measurement, QSqlQuery &measurementQuery, bool cacheRanges)
{
  measurementQuery.exec();
  if (measurementQuery.lastError().isValid()) {
    qWarning() << "Select measurement failed" << measurementQuery.lastError();
    return false;
  }
  if (!measurementQuery.next()){
    qWarning() << "No measurement found";
    return false;
  }
  measurement.id = varToLong(measurementQuery.value("id"));
  measurement.time = varToDateTime(measurementQuery.value("time"));

  measurement.statm.size = varToLong(measurementQuery.value("statm_size"));
  measurement.statm.resident = varToLong(measurementQuery.value("statm_resident"));
  measurement.statm.shared = varToLong(measurementQuery.value("statm_shared"));
  measurement.statm.text = varToLong(measurementQuery.value("statm_text"));
  measurement.statm.lib = varToLong(measurementQuery.value("statm_lib"));
  measurement.statm.data = varToLong(measurementQuery.value("statm_data"));
  measurement.statm.dt = varToLong(measurementQuery.value("statm_dt"));

  QSqlQuery sql(db);

  // ranges
  if (cacheRanges){
    if (measurement.rangeMap.empty()){
      getAllRanges(measurement.rangeMap);
    }
  }else {
    sql.prepare(
      "SELECT * FROM `range` WHERE `id` IN (SELECT `range_id` FROM `data` WHERE `measurement_id` = :measurement_id);");
    sql.bindValue(":measurement_id", measurement.id);

    measurement.rangeMap.clear();
    if (!getRanges(measurement.rangeMap, sql)) {
      qWarning() << "Select ranges failed" << sql.lastError();
      return false;
    }
  }

  // data
  sql.prepare("SELECT * FROM `data` WHERE `measurement_id` = :measurement_id");
  sql.bindValue(":measurement_id", measurement.id);
  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Select ranges failed" << sql.lastError();
    return false;
  }
  measurement.data.clear();
  while (sql.next()) {
    MeasurementData data;
    data.rangeId = varToLong(sql.value("range_id"));;
    data.rss = varToLong(sql.value("rss"));
    data.pss = varToLong(sql.value("pss"));
    measurement.data << data;
  }

  return true;
}

bool Storage::getMemoryPeak(Measurement &measurement, MemoryType type, qint64 from, qint64 to)
{
  QSqlQuery sql(db);

  // measurement
  if (type == Rss) {
    sql.prepare("SELECT * FROM `measurement` WHERE rss_sum = (SELECT MAX(rss_sum) FROM `measurement` WHERE `id` >= :from AND id < :to) AND `id` >= :from AND id < :to LIMIT 1;");
  }else if (type == StatmRss){
    sql.prepare("SELECT * FROM `measurement` WHERE statm_resident = (SELECT MAX(statm_resident) FROM `measurement` WHERE `id` >= :from AND id < :to) AND `id` >= :from AND id < :to LIMIT 1;");
  }else{
    sql.prepare("SELECT * FROM `measurement` WHERE pss_sum = (SELECT MAX(pss_sum) FROM `measurement` WHERE `id` >= :from AND id < :to) AND `id` >= :from AND id < :to LIMIT 1;");
  }
  sql.bindValue(":from", from);
  sql.bindValue(":to", to);
  return getMeasurement(measurement, sql);
}

qint64 Storage::measurementCount()
{
  QSqlQuery sql(db);
  sql.prepare("SELECT COUNT(*) AS `cnt` FROM `measurement`");
  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Select count of measurements failed" << sql.lastError();
    return -1;
  }
  sql.next();
  return varToLong(sql.value("cnt"));
}

bool Storage::getMeasurement(Measurement &measurement, qlonglong &id, bool cacheRanges)
{
  QSqlQuery sql(db);
  sql.prepare("SELECT * FROM `measurement` WHERE `id` = :id;");
  sql.bindValue(":id", id);
  return getMeasurement(measurement, sql, cacheRanges);
}

bool Storage::getMeasuremntRange(qlonglong &min,
                                 qlonglong &max)
{
  QSqlQuery sql(db);
  sql.prepare("SELECT MIN(`id`) AS min, MAX(`id`) AS max FROM `measurement`");
  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Select ranges failed" << sql.lastError();
    return false;
  }
  if (!sql.next()){
    qWarning() << "No result";
    return false;
  }

  min = varToLong(sql.value("min"));
  max = varToLong(sql.value("max"));

  return true;
}
