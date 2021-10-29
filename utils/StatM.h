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

#include <cstdlib>

#include <QObject>

/**
 * see statm in `man proc`, sizes are in KiB, not pages!
 */
struct StatM {
  size_t size{0};      // total program size
                       // (same as VmSize in /proc/[pid]/status)
  size_t resident{0};  // resident set size
                       // (same as VmRSS in /proc/[pid]/status)
  size_t shared{0};    // number of resident shared pages (i.e., backed by a file)
                       // (same as RssFile+RssShmem in /proc/[pid]/status)
  size_t text{0};      // text (code)
  size_t lib{0};       // library (unused since Linux 2.6; always 0)
  size_t data{0};      // data + stack
  size_t dt{0};        // dirty pages (unused since Linux 2.6; always 0)
};

Q_DECLARE_METATYPE(StatM)
