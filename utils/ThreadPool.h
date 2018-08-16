
#ifndef MEMORY_WATCHER_THREADPOOL_H
#define MEMORY_WATCHER_THREADPOOL_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include <atomic>

class ThreadPool : public QObject{
  Q_OBJECT
  Q_DISABLE_COPY(ThreadPool)

signals:
  void closed();

public slots:
  void threadFinished();
  void checkClosed();

public:
  ThreadPool();
  ~ThreadPool();

  QThread *makeThread(QString name);
  void close();

private:
  std::atomic_int liveBackgroundThreads;
  int closeTimeout{0};
  QTimer closeCheck;
};


#endif //MEMORY_WATCHER_THREADPOOL_H
