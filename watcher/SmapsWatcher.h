
#ifndef MEMORY_WATCHER_SMAPSWATCHER_H
#define MEMORY_WATCHER_SMAPSWATCHER_H


#include <QtCore/QObject>
#include <QThread>
#include <QTimer>
#include <QDateTime>

class SmapsRange {
public:
  void debugPrint();
  size_t sizeBytes();

public:
  size_t from{0};
  size_t to{0};
  QString permission;
  QString name;
  size_t rss{0}; // Ki
  size_t pss{0}; // Ki
};

class SmapsWatcher : public QObject{
  Q_OBJECT
  Q_DISABLE_COPY(SmapsWatcher)

signals:
  void snapshot(QDateTime time, QList<SmapsRange> ranges);

public slots:
  void init();
  void update();

public:
  SmapsWatcher(QThread *thread, long pid, long period);
  ~SmapsWatcher();

private:
  QTimer timer;
  QThread *thread;
  long pid;
  long period;
};


#endif //MEMORY_WATCHER_SMAPSWATCHER_H
