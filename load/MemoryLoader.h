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

#include <SmapsRange.h>

#include <QtCore/QObject>
#include <QThread>
#include <QTimer>
#include <QDateTime>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QFileInfo>
#include <StatM.h>

#include <atomic>

class MemoryLoader : public QObject{
Q_OBJECT
  Q_DISABLE_COPY(MemoryLoader)

signals:
  void loaded(ProcessId processId, QDateTime time, QList<SmapsRange> ranges);

public slots:
  void init();
  void update();

public:
  MemoryLoader(pid_t pid, const QString &smapsFile);

  virtual ~MemoryLoader() = default;

private:
  bool readSmaps(QList<SmapsRange> &ranges);

private:
  QFileInfo smapsFile;
  QString lastLineStart;
  ProcessId processId;
};
