
#ifndef MEMORY_WATCHER_FEEDER_H
#define MEMORY_WATCHER_FEEDER_H

#include "SmapsWatcher.h"

#include <Storage.h>

#include <QObject>

#include <QMap>

class RangeKey {
public:
  size_t from{0};
  size_t to{0};
  QString permission;
};

bool operator<(const RangeKey&, const RangeKey&);

class Feeder : public QObject{
  Q_OBJECT
  Q_DISABLE_COPY(Feeder)

signals:
public slots:
  void onSnapshot(QDateTime time, QList<SmapsRange> ranges);

public:
  Feeder();
  ~Feeder();

  bool init(QString file);

private:
  Storage storage;
  QMap<RangeKey, qlonglong> lastRanges;
};


#endif //MEMORY_WATCHER_FEEDER_H
