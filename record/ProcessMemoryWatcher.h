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

#include <OomScore.h>
#include <ProcessId.h>
#include <SmapsRange.h>
#include <Utils.h>

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
  Q_DISABLE_COPY_MOVE(ProcessMemoryWatcher)

signals:
  void initialized(ProcessId processId, QString name);

  void snapshot(QDateTime time,
                ProcessId processId,
                QList<SmapsRange> ranges,
                StatM statm,
                OomScore oomScore);

  void exited(ProcessId processId);

public slots:
  void init();
  void update(QDateTime time);

public:
  ProcessMemoryWatcher(QThread *thread,
                       pid_t pid,
                       QString procFs);

  virtual ~ProcessMemoryWatcher() = default;

  ProcessId getProcessId() const {
    return processId;
  }

private:
  bool initSmaps();
  QString readProcessName() const;
  bool readSmaps(QList<SmapsRange> &ranges);
  bool readStatM(StatM &statm);
  bool readInt(const QFileInfo &file, int &value) const;
  OomScore readOomScore();

private:
  ProcessId processId;
  QThread *thread;
  QFileInfo smapsFile;
  QFileInfo statmFile;
  QFileInfo statusFile;
  QFileInfo oomAdjFile;
  QFileInfo oomScoreFile;
  QFileInfo oomScoreAdjFile;
  QString lastLineStart;
  bool accessible{true}; // false when smaps is not accessible (we don't have enough privileges)
};
