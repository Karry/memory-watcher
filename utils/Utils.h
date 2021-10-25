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

#include "Storage.h"

#include <QThread>
#include <QString>

#include <functional>
#include <unordered_map>

constexpr size_t PageSizeKiB = 4;

struct Mapping
{
  std::string name;
  size_t size;
};

class MeasurementGroups{
public:
  std::vector<Mapping> sortedMappings() const;

public:
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
  static void group(MeasurementGroups &g, const Measurement &measurement, MemoryType type, bool groupSockets = false);
  static void printMeasurement(const Measurement &measurement, MemoryType type);
  static void clearScreen();
  static void registerQtMetatypes();
};
