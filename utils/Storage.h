

#ifndef MEMORY_WATCHER_STORAGE_H
#define MEMORY_WATCHER_STORAGE_H


#include <QtCore/QObject>
#include <QSqlDatabase>
#include <QDateTime>

class MeasurementData
{
public:
  qlonglong rangeId;
  qlonglong rss;
  qlonglong pss;
};

class Storage : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(Storage)

signals:
public slots:
public:
  Storage();
  ~Storage();

  bool updateSchema();
  bool init(QString file);

  qlonglong insertRange(qlonglong from, qlonglong to, QString permission, QString name);
  qlonglong insertMeasurement(const QDateTime &time, qlonglong rss, qlonglong pss);
  bool insertData(qlonglong measurementId, const std::vector<MeasurementData> &measurements);

  bool insertData(const std::vector<MeasurementData> &measurements);

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
  QSqlDatabase db;
};


#endif //MEMORY_WATCHER_STORAGE_H
