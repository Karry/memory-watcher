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


#include "ThreadPool.h"
#include <QDebug>

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
  connect(&closeCheck, &QTimer::timeout, this, &ThreadPool::checkClosed);
  closeCheck.setInterval(100);
  closeCheck.setSingleShot(false);
  closeCheck.start();
  checkClosed();
}

QThread *ThreadPool::makeThread(QString name)
{
  QThread *thread=new QThread();
  thread->setObjectName(name);
  QObject::connect(thread, &QThread::finished,
                   thread, &QThread::deleteLater);

  connect(thread, &QThread::finished,
          this, &ThreadPool::threadFinished);

  liveBackgroundThreads++;
  qDebug() << "Create background thread" << thread;

  return thread;
}

void ThreadPool::threadFinished()
{
  qDebug() << "Thread finished" << QObject::sender();
  liveBackgroundThreads--;
}


