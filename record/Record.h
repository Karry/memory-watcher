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

#include "MemoryWatcher.h"
#include "Feeder.h"

#include <ThreadPool.h>

#include <QObject>
#include <QThread>

#include <atomic>

class Record : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(Record)

public slots:
  void close();

public:
  Record(long pid, long period, QString databaseFile);
  ~Record();

private:
  ThreadPool threadPool;
  QThread *watcherThread;
  std::atomic_int queueSize{0};
  MemoryWatcher *watcher;
  Feeder *feeder;

  //QTimer shutdownTimer;
};
