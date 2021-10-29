/*
  Memory watcher
  Copyright (C) 2021 Lukas Karas <lukas.karas@centrum.cz>

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

#include <Utils.h>
#include <MemInfo.h>

#include <QtCore/QObject>
#include <QDateTime>
#include <QFileInfo>

class SystemMemoryWatcher: public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(SystemMemoryWatcher)
signals:
  void systemSnapshot(QDateTime time, MemInfo memInfo);
public slots:
  void update(QDateTime time);
public:
  explicit SystemMemoryWatcher(const QString &procFs);

  virtual ~SystemMemoryWatcher() = default;

private:
  QFileInfo memInfoFile;
};
