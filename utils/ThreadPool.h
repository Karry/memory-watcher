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
  ThreadPool() = default;
  ~ThreadPool() = default;

  QThread *makeThread(QString name);
  void close();

private:
  std::atomic_int liveBackgroundThreads{0};
  int closeTimeout{0};
  QTimer closeCheck;
};
