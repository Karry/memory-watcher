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

#include "SystemMemoryWatcher.h"

#include <String.h>

#include <QTextStream>
#include <QDebug>

namespace {
size_t parseLine(const QString &line) {
  QStringList arr = line.split(" ", SkipEmptyParts);
  if (arr.size() != 3) {
    qWarning() << "Can't parse memory line" << line;
    return 0;
  }
  if (arr[2] != "kB") {
    qWarning() << "Can't parse memory line" << line;
    return 0;
  }
  bool ok = true;
  size_t val = arr[1].toULongLong(&ok);
  if (!ok) {
    qWarning() << "Can't parse memory line" << line;
    return 0;
  }
  return val;
}
} // namespace

SystemMemoryWatcher::SystemMemoryWatcher(const QString &procFs):
  memInfoFile(QString("%1/meminfo").arg(procFs))
{}

void SystemMemoryWatcher::update(QDateTime time) {
  if (!memInfoFile.exists()) {
    return;
  }

  QFile inputFile(memInfoFile.absoluteFilePath());
  if (!inputFile.open(QIODevice::ReadOnly)) {
    qWarning() << "Can't open file" << memInfoFile.absoluteFilePath();
    return;
  }

  MemInfo memInfo;
  QTextStream in(&inputFile);

  for (QString line = in.readLine(); !line.isEmpty(); line = in.readLine()) {
    if (line.startsWith("MemTotal:")) {
      memInfo.memTotal = parseLine(line);
    } else if (line.startsWith("MemFree:")) {
      memInfo.memFree = parseLine(line);
    } else if (line.startsWith("MemAvailable:")) {
      memInfo.memAvailable = parseLine(line);
    } else if (line.startsWith("Buffers:")) {
      memInfo.buffers = parseLine(line);
    } else if (line.startsWith("Cached:")) {
      memInfo.cached = parseLine(line);
    } else if (line.startsWith("SwapCache:")) {
      memInfo.swapCache = parseLine(line);
    } else if (line.startsWith("SReclaimable:")) {
      memInfo.sReclaimable = parseLine(line);
    }
  }

  emit systemSnapshot(time, memInfo);
}
