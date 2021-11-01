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
#include "QVariantConverters.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

#include <cassert>

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
    sql.append("(").append( "`id` UNSIGNED BIG INT PRIMARY KEY "); // hash of pid and start_time
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
    sql.append("(").append( "`id` UNSIGNED BIG INT PRIMARY KEY "); // hash of process_id, from, to and permission columns
    sql.append(",").append( "`process_id` UNSIGNED BIG INT NOT NULL REFERENCES process(id) ON DELETE CASCADE ");
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
    sql.append("(").append( "`id` UNSIGNED BIG INT PRIMARY KEY "); // hash of process_id and time
    sql.append(",").append( "`process_id` UNSIGNED BIG INT NOT NULL REFERENCES process(id) ON DELETE CASCADE ");
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
    sql.append("(").append( "`range_id` UNSIGNED BIG INT NOT NULL REFERENCES memory_range(id) ON DELETE CASCADE");
    sql.append(",").append( "`measurement_id` UNSIGNED BIG INT NOT NULL REFERENCES measurement(id) ON DELETE CASCADE");
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

  if (!tables.contains("system_memory")) {
    QString sql("CREATE TABLE `system_memory`");
    sql.append("(").append("`time` datetime NOT NULL ");
    sql.append(",").append("`mem_total` INTEGER NOT NULL ");
    sql.append(",").append("`mem_free` INTEGER NOT NULL ");
    sql.append(",").append("`mem_available` INTEGER NOT NULL ");
    sql.append(",").append("`buffers` INTEGER NOT NULL ");
    sql.append(",").append("`cached` INTEGER NOT NULL ");
    sql.append(",").append("`swap_cache` INTEGER NOT NULL ");
    sql.append(",").append("`swap_total` INTEGER NOT NULL ");
    sql.append(",").append("`swap_free` INTEGER NOT NULL ");
    sql.append(",").append("`anon_pages` INTEGER NOT NULL ");
    sql.append(",").append("`mapped` INTEGER NOT NULL ");
    sql.append(",").append("`shmem` INTEGER NOT NULL ");
    sql.append(",").append("`s_reclaimable` INTEGER NOT NULL ");

    sql.append(");");

    QSqlQuery q = db.exec(sql);
    if (q.lastError().isValid()) {
      qWarning() << "Creating data table failed" << q.lastError();
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

  // following pragmas speed up operations a bit, but are dangerous
  // https://stackoverflow.com/questions/1711631/improve-insert-per-second-performance-of-sqlite
  if (QSqlQuery q = db.exec("PRAGMA synchronous = OFF;");
      q.lastError().isValid()){
    qWarning() << "Setup synchronous writes fails:" << q.lastError();
  }

  if (QSqlQuery q = db.exec("PRAGMA journal_mode = MEMORY;");
      q.lastError().isValid()){
    qWarning() << "Setup memory journal fails:" << q.lastError();
  }

  if (!updateSchema()){
    return false;
  }

  if (QSqlQuery q = db.exec("PRAGMA foreign_keys = ON;");
      q.lastError().isValid()){
    qWarning() << "Enabling foreign keys fails:" << q.lastError();
  }

  bool valid = db.isValid() && db.isOpen();
  if (valid) {
    sqlProcessInsert = QSqlQuery(db);
    sqlProcessInsert.prepare("INSERT OR IGNORE INTO `process` (`id`, `pid`, `start_time`, `name`) VALUES (:id, :pid, :start_time, :name)");

    sqlRangeInsert = QSqlQuery(db);
    sqlRangeInsert.prepare("INSERT OR IGNORE INTO `memory_range` (`id`, `process_id`, `from`, `to`, `permission`, `name`) VALUES (:id, :process_id, :from, :to, :permission, :name)");

    sqlMeasurementInsert = QSqlQuery(db);
    sqlMeasurementInsert.prepare("INSERT INTO `measurement` ("
                                 "  `id`, `process_id`, `time`, `rss_sum`, `pss_sum`,"
                                 "  `oom_adj`, `oom_score`, `oom_score_adj`, "
                                 "  `statm_size`, `statm_resident`, `statm_shared`, `statm_text`, `statm_lib`, `statm_data`, `statm_dt` "
                                 ") VALUES ("
                                 "  :id, :process_id, :time, :rss, :pss, "
                                 "  :oom_adj, :oom_score, :oom_score_adj, "
                                 "  :statm_size, :statm_resident, :statm_shared, :statm_text, :statm_lib, :statm_data, :statm_dt"
                                 ")");

    sqlDataInsert = QSqlQuery(db);
    sqlDataInsert.prepare("INSERT INTO `data` (`range_id`, `measurement_id`, `rss`, `pss`) VALUES (:range_id, :measurement_id, :rss, :pss)");

    sqlSystemInsert = QSqlQuery(db);
    sqlSystemInsert.prepare("INSERT INTO `system_memory` (`time`, `mem_total`, `mem_free`, `mem_available`, `buffers`, `cached`, `swap_cache`, "
                            "   `swap_total`, `swap_free`, `anon_pages`, `mapped`, `shmem`, `s_reclaimable`"
                            ") VALUES (:time, :mem_total, :mem_free, :mem_available, :buffers, :cached, :swap_cache, "
                            "   :swap_total, :swap_free, :anon_pages, :mapped, :shmem, :s_reclaimable)");
  }
  return valid;
}

bool Storage::insertOrIgnoreProcess(const ProcessId &processId, const QString &name) {
  sqlProcessInsert.bindValue(":id", processId.hash());
  sqlProcessInsert.bindValue(":pid", processId.pid);
  sqlProcessInsert.bindValue(":start_time", processId.startTime);
  sqlProcessInsert.bindValue(":name", name);

  sqlProcessInsert.exec();
  if (sqlProcessInsert.lastError().isValid()) {
    qWarning() << "Insert process (pid" << processId.pid <<
               "hash" << processId.hash() << ") failed" << sqlProcessInsert.lastError();
    return false;
  }
  return true;
}

qlonglong Storage::insertOrIgnoreRange(const SmapsRange::Key &range)
{
  sqlRangeInsert.bindValue(":id", range.hash());
  sqlRangeInsert.bindValue(":process_id", range.processId.hash());
  sqlRangeInsert.bindValue(":from", qlonglong(range.from));
  sqlRangeInsert.bindValue(":to", qlonglong(range.to));
  sqlRangeInsert.bindValue(":permission", range.permission);
  sqlRangeInsert.bindValue(":name", range.name);

  sqlRangeInsert.exec();
  if (sqlRangeInsert.lastError().isValid()) {
    qWarning() << "Insert range failed" << sqlRangeInsert.lastError();
    return 0;
  }

  return varToLong(sqlRangeInsert.lastInsertId());
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
  sqlMeasurementInsert.bindValue(":id", measurementHash(processId, time));
  sqlMeasurementInsert.bindValue(":process_id", processId.hash());

  sqlMeasurementInsert.bindValue(":time", time);
  sqlMeasurementInsert.bindValue(":rss", rss);
  sqlMeasurementInsert.bindValue(":pss", pss);

  sqlMeasurementInsert.bindValue(":oom_adj", oomScore.adj);
  sqlMeasurementInsert.bindValue(":oom_score", oomScore.score);
  sqlMeasurementInsert.bindValue(":oom_score_adj", oomScore.scoreAdj);

  sqlMeasurementInsert.bindValue(":statm_size", (qlonglong)statm.size);
  sqlMeasurementInsert.bindValue(":statm_resident", (qlonglong)statm.resident);
  sqlMeasurementInsert.bindValue(":statm_shared", (qlonglong)statm.shared);
  sqlMeasurementInsert.bindValue(":statm_text", (qlonglong)statm.text);
  sqlMeasurementInsert.bindValue(":statm_lib", (qlonglong)statm.lib);
  sqlMeasurementInsert.bindValue(":statm_data", (qlonglong)statm.data);
  sqlMeasurementInsert.bindValue(":statm_dt", (qlonglong)statm.dt);

  sqlMeasurementInsert.exec();
  if (sqlMeasurementInsert.lastError().isValid()) {
    qWarning() << "Insert measurement failed" << sqlMeasurementInsert.lastError();
    return 0;
  }

  return varToLong(sqlMeasurementInsert.lastInsertId());
}

bool Storage::insertData(const ProcessId &processId,
                         const QDateTime &time,
                         const QList<SmapsRange> &ranges)
{
  qlonglong measurementId = measurementHash(processId, time);

  for (const auto &m: ranges) {
    sqlDataInsert.bindValue(":range_id", m.key.hash());
    sqlDataInsert.bindValue(":measurement_id", measurementId);
    sqlDataInsert.bindValue(":rss", qlonglong(m.rss));
    sqlDataInsert.bindValue(":pss", qlonglong(m.pss));

    sqlDataInsert.exec();
    if (sqlDataInsert.lastError().isValid()) {
      qWarning() << "Insert data failed" << sqlDataInsert.lastError();
      return false;
    }
  }

  return true;
}

bool Storage::insertSystemMemInfo(const QDateTime &time, const MemInfo &memInfo) {
  sqlSystemInsert.bindValue(":time", time);
  sqlSystemInsert.bindValue(":mem_total", (qlonglong)memInfo.memTotal);
  sqlSystemInsert.bindValue(":mem_free", (qlonglong)memInfo.memFree);
  sqlSystemInsert.bindValue(":mem_available", (qlonglong)memInfo.memAvailable);
  sqlSystemInsert.bindValue(":buffers", (qlonglong)memInfo.buffers);
  sqlSystemInsert.bindValue(":cached", (qlonglong)memInfo.cached);
  sqlSystemInsert.bindValue(":swap_cache", (qlonglong)memInfo.swapCache);
  sqlSystemInsert.bindValue(":swap_total", (qlonglong)memInfo.swapTotal);
  sqlSystemInsert.bindValue(":swap_free", (qlonglong)memInfo.swapFree);
  sqlSystemInsert.bindValue(":anon_pages", (qlonglong)memInfo.anonPages);
  sqlSystemInsert.bindValue(":mapped", (qlonglong)memInfo.mapped);
  sqlSystemInsert.bindValue(":shmem", (qlonglong)memInfo.shmem);
  sqlSystemInsert.bindValue(":s_reclaimable", (qlonglong)memInfo.sReclaimable);

  sqlSystemInsert.exec();
  if (sqlSystemInsert.lastError().isValid()) {
    qWarning() << "Insert data failed" << sqlSystemInsert.lastError();
    return false;
  }

  return true;
}

bool Storage::getRanges(QMap<qulonglong, Range> &rangeMap, QSqlQuery &sql)
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
    rangeMap[varToULong(sql.value("id"))] = range;
  }
  return true;
}

bool Storage::getAllRanges(qulonglong processId, QMap<qulonglong, Range> &rangeMap)
{
  QSqlQuery sql(db);

  sql.prepare("SELECT * FROM `memory_range` WHERE process_id = :process_id;");
  sql.bindValue(":process_id", processId);
  return getRanges(rangeMap, sql);
}

bool Storage::execAndGetMeasurement(Measurement &measurement, QSqlQuery &measurementQuery, bool cacheRanges) {
  measurementQuery.exec();
  if (measurementQuery.lastError().isValid()) {
    qWarning() << "Select measurement failed" << measurementQuery.lastError();
    return false;
  }
  if (!measurementQuery.next()) {
    qWarning() << "No measurement found";
    return false;
  }
  return getMeasurement(measurement, measurementQuery, cacheRanges);
}

bool Storage::getMeasurement(Measurement &measurement, QSqlQuery &measurementQuery, bool cacheRanges) {
  measurement.id = varToULong(measurementQuery.value("id"));
  measurement.processId = varToULong(measurementQuery.value("process_id"));
  measurement.time = varToDateTime(measurementQuery.value("time"));

  measurement.oomScore.adj = varToLong(measurementQuery.value("oom_adj"));
  measurement.oomScore.score = varToLong(measurementQuery.value("oom_score"));
  measurement.oomScore.scoreAdj = varToLong(measurementQuery.value("oom_score_adj"));

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
      getAllRanges(measurement.processId, measurement.rangeMap);
    }
  }else {
    sql.prepare(
      "SELECT * FROM `memory_range` WHERE `id` IN (SELECT `range_id` FROM `data` WHERE `measurement_id` = :measurement_id);");
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
    data.rangeId = varToULong(sql.value("range_id"));;
    data.rss = varToLong(sql.value("rss"));
    data.pss = varToLong(sql.value("pss"));
    measurement.data << data;
  }

  return true;
}

bool Storage::lookupPid(pid_t pid, QMap<ProcessId, QString> &processes) {
  QSqlQuery sql(db);
  sql.prepare("SELECT * FROM `process` WHERE pid = :pid;");

  sql.bindValue(":pid", pid);
  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Select of process failed" << sql.lastError();
    return false;
  }
  while (sql.next()) {
    ProcessId::StartTime startTime = varToULong(sql.value("start_time"));
    processes[ProcessId(pid, startTime)] = varToString(sql.value("name"));
  }
  return true;
}

bool Storage::getProcess(qulonglong processId, pid_t &pid, QString &processName) {
  QSqlQuery sql(db);
  sql.prepare("SELECT * FROM `process` WHERE id = :id;");

  sql.bindValue(":id", processId);
  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Select of process failed" << sql.lastError();
    return false;
  }
  if (!sql.next()) {
    return false;
  }
  pid = varToULong(sql.value("pid"));
  processName = varToString(sql.value("name"));
  return true;
}

bool Storage::getMemoryPeak(qulonglong processId, Measurement &measurement, ProcessMemoryType type, qint64 from, qint64 to)
{
  if (!getProcess(processId, measurement.pid, measurement.processName)) {
    qWarning() << "Failed to read process details";
    return false;
  }

  QSqlQuery sql(db);

  // measurement
  if (type == Rss) {
    sql.prepare(QString("SELECT * FROM `measurement` WHERE process_id = :process_id AND rss_sum = ")
                .append("(SELECT MAX(rss_sum) FROM `measurement` WHERE process_id = :process_id AND `id` >= :from AND id < :to) AND `id` >= :from AND id < :to LIMIT 1;"));
  }else if (type == StatmRss){
    sql.prepare(QString("SELECT * FROM `measurement` WHERE process_id = :process_id AND statm_resident = ")
                .append("(SELECT MAX(statm_resident) FROM `measurement` WHERE process_id = :process_id AND `id` >= :from AND id < :to) AND `id` >= :from AND id < :to LIMIT 1;"));
  }else{
    assert(type == Pss);
    sql.prepare(QString("SELECT * FROM `measurement` WHERE process_id = :process_id AND pss_sum = ")
                .append("(SELECT MAX(pss_sum) FROM `measurement` WHERE process_id = :process_id AND `id` >= :from AND id < :to) AND `id` >= :from AND id < :to LIMIT 1;"));
  }
  sql.bindValue(":process_id", processId);
  sql.bindValue(":from", from);
  sql.bindValue(":to", to);
  return execAndGetMeasurement(measurement, sql);
}

bool Storage::getSystemMemoryPeak(SystemMemoryType memoryType,
                                  QDateTime &time,
                                  MemInfo &memInfo,
                                  QList<Measurement> &processes) {
  QSqlQuery sql(db);
  if (memoryType == MemAvailable) {
    sql.prepare("SELECT * FROM `system_memory` ORDER BY `mem_available` ASC LIMIT 1;");
  } else {
    assert(memoryType == MemAvailableComputed);
    sql.prepare("SELECT * FROM `system_memory` ORDER BY (`mem_free` + `buffers` + (`cached` - `shmem`) + `swap_cache` + `s_reclaimable`) ASC LIMIT 1;");
  }
  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Select of system_memory failed" << sql.lastError();
    return false;
  }
  if (!sql.next()) {
    return false;
  }

  time = varToDateTime(sql.value("time"));
  memInfo.memTotal = varToULong(sql.value("mem_total"));
  memInfo.memFree = varToULong(sql.value("mem_free"));
  memInfo.memAvailable = varToULong(sql.value("mem_available"));
  memInfo.buffers = varToULong(sql.value("buffers"));
  memInfo.cached = varToULong(sql.value("cached"));
  memInfo.swapCache = varToULong(sql.value("swap_cache"));
  memInfo.anonPages = varToULong(sql.value("anon_pages"));
  memInfo.mapped = varToULong(sql.value("mapped"));
  memInfo.swapTotal = varToULong(sql.value("swap_total"));
  memInfo.swapFree = varToULong(sql.value("swap_free"));
  memInfo.shmem = varToULong(sql.value("shmem"));
  memInfo.sReclaimable = varToULong(sql.value("s_reclaimable"));

  sql.prepare(QString("SELECT `p`.`pid`, `p`.`start_time`, `p`.`name`, `m`.* ")
              .append("FROM `measurement` AS `m` JOIN `process` AS `p` ON `p`.id == `m`.`process_id` WHERE `time` == :time"));
  sql.bindValue(":time", time);
  sql.exec();
  if (sql.lastError().isValid()) {
    qWarning() << "Select of measurement failed" << sql.lastError();
    return false;
  }
  while (sql.next()) {
    Measurement measurement;
    measurement.pid = varToULong(sql.value("pid"));
    measurement.processName = varToString(sql.value("name"));
    if (!getMeasurement(measurement, sql)) {
      qWarning() << "Select of measurement failed" << sql.lastError();
      return false;
    }
    processes << measurement;
  }
  return true;
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
  return execAndGetMeasurement(measurement, sql, cacheRanges);
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
