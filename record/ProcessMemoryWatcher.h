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

#include <ProcessId.h>
#include <SmapsRange.h>

#include <QtCore/QObject>
#include <QThread>
#include <QTimer>
#include <QDateTime>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QFileInfo>

#include <StatM.h>
#include <atomic>

class ProcessMemoryWatcher : public QObject{
  Q_OBJECT
  Q_DISABLE_COPY(ProcessMemoryWatcher)

signals:
  void snapshot(QDateTime time, QList<SmapsRange> ranges, StatM statm);

public slots:
  void init();
  void update();

public:
  ProcessMemoryWatcher(QThread *thread,
                       pid_t pid,
                       long period,
                       std::atomic_int &queueSize,
                       QString procFs = QProcessEnvironment::systemEnvironment().value("PROCFS", "/proc"));

  ~ProcessMemoryWatcher();

private:
  bool readSmaps(QList<SmapsRange> &ranges);
  bool readStatM(StatM &statm);

private:
  ProcessId processId;
  QTimer timer;
  QThread *thread;
  long period;
  QFileInfo smapsFile;
  QFileInfo statmFile;
  QString lastLineStart;
  std::atomic_int &queueSize; // not owning reference
};
