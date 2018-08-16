
#ifndef MEMORY_WATCHER_WATCHER_H
#define MEMORY_WATCHER_WATCHER_H

#include "SmapsWatcher.h"
#include "Feeder.h"

#include <ThreadPool.h>

#include <QObject>
#include <QThread>

class Watcher : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(Watcher)

public slots:
  void close();

public:
  Watcher(long pid, long period);
  ~Watcher();

private:
  ThreadPool threadPool;
  QThread *watcherThread;
  SmapsWatcher *watcher;
  Feeder *feeder;

  //QTimer shutdownTimer;
};


#endif //MEMORY_WATCHER_WATCHER_H
