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

#include "MemoryLoader.h"

#include <Storage.h>
#include <Utils.h>

#include <QObject>
#include <QMap>

#include <atomic>

class Feeder : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Feeder)

signals:
  void inserted();

public slots:
  void onSnapshot(ProcessId processId, QDateTime time, QList<SmapsRange> ranges);

public:
  Feeder() = default;
  ~Feeder() = default;

  bool init(QString file);

private:
  Storage storage;
};
