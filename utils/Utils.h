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

#include "StatM.h"
#include "ProcessId.h"
#include "OomScore.h"
#include "SmapsRange.h"

#include <QDateTime>
#include <QThread>
#include <QString>
#include <QMap>

#include <functional>
#include <unordered_map>

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#define Q_DISABLE_COPY_MOVE Q_DISABLE_COPY
#endif

constexpr size_t PageSizeKiB = 4;

struct MeasurementData {
  qlonglong rangeId{0};
  qlonglong rss{0};
  qlonglong pss{0};
};

struct Range {
  qlonglong from{0};
  qlonglong to{0};
  QString permission;
  QString name;
};

struct Measurement {
  qulonglong id{0};
  qulonglong processId{0};
  QDateTime time;
  OomScore oomScore;
  StatM statm;
  QMap<qulonglong, Range> rangeMap;
  QList<MeasurementData> data;
};

enum ProcessMemoryType {
  // SMaps
  Rss,
  Pss,
  // statm
  StatmRss
};

enum SystemMemoryType {
  MemAvailable, // Kernel estimate how much memory is available before system start swapping.
  MemAvailableComputed // MemFree + Buffers + (Cached - Shmem) + SwapCache + SReclaimable.
};

struct Mapping {
  std::string name;
  size_t size;
};

struct MeasurementGroups {
  std::vector<Mapping> sortedMappings() const;

  size_t threadStacks{0};
  size_t anonymous{0};
  size_t heap{0};
  size_t sockets{0};
  size_t sum{0};

  StatM statm;

  std::unordered_map<std::string, size_t> mappings;
};

class Utils {
public:

  static void catchUnixSignals(std::initializer_list<int> quitSignals,
                               std::function<void(int)> *handler);

  static void cleanSignalCallback();

  static void printMeasurementSmapsLike(const Measurement &measurement);
  static void group(MeasurementGroups &g, const Measurement &measurement, ProcessMemoryType type, bool groupSockets = false);
  static void printMeasurement(const Measurement &measurement, ProcessMemoryType type);
  static void clearScreen();
  static void registerQtMetatypes();
};
