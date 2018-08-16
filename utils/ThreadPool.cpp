

#include "ThreadPool.h"
#include <QDebug>

ThreadPool::ThreadPool()
{}

ThreadPool::~ThreadPool()
{
}

void ThreadPool::checkClosed()
{
  closeTimeout++;
  if (liveBackgroundThreads == 0) {
    qDebug() << "All threads exited";
    closeCheck.stop();
    emit closed();
  }else if (closeTimeout > 100) {
    qWarning() << "Running" << liveBackgroundThreads << "background threads, TIMEOUT";
    closeCheck.stop();
    emit closed();
  }else {
    qDebug() << "Waiting for" << liveBackgroundThreads << "background threads (" << closeTimeout << ")";
  }
}

void ThreadPool::close()
{
  closeTimeout = 0;
  connect(&closeCheck, SIGNAL(timeout()), this, SLOT(checkClosed()));
  closeCheck.setInterval(100);
  closeCheck.setSingleShot(false);
  closeCheck.start();
  checkClosed();
}

QThread *ThreadPool::makeThread(QString name)
{
  QThread *thread=new QThread();
  thread->setObjectName(name);
  QObject::connect(thread, SIGNAL(finished()),
                   thread, SLOT(deleteLater()));

  connect(thread, SIGNAL(finished()),
          this, SLOT(threadFinished()));

  liveBackgroundThreads++;
  qDebug() << "Create background thread" << thread;

  return thread;
}

void ThreadPool::threadFinished()
{
  qDebug() << "Thread finished" << QThread::currentThread();
  liveBackgroundThreads--;
}


