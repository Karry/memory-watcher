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

#include <QString>
#include <unistd.h>

struct ProcessId {
  using StartTime = unsigned long long;

  pid_t pid{0};
  StartTime startTime{0}; //!< from /proc/<pid>/stat

  ProcessId() = default;
  ProcessId(pid_t pid, const QString &procFs):
    pid(pid), startTime(processStartTime(pid, procFs)) {};

  ProcessId(pid_t pid, StartTime startTime):
    pid(pid), startTime(startTime) {};

  ProcessId(const ProcessId&) = default;
  ProcessId(ProcessId&&) = default;
  ~ProcessId() = default;
  ProcessId& operator=(const ProcessId&) = default;
  ProcessId& operator=(ProcessId&&) = default;

  qulonglong hash() const;

  static StartTime processStartTime(pid_t pid, const QString &procFs);

  friend bool operator<(const ProcessId&, const ProcessId&);
};

