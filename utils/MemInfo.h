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

/**
 * System memory statistics read from /proc/meminfo
 * See man proc
 */
struct MemInfo {
  size_t memTotal{0};     //!< Total usable RAM (i.e., physical RAM minus a few reserved bits and the kernel binary code).
  size_t memFree{0};      //!< The sum of LowFree+HighFree
  size_t memAvailable{0}; //!< An estimate of how much memory is available for starting new applications, without swapping.
  size_t buffers{0};      //!< Relatively temporary storage for raw disk blocks that shouldn't get tremendously large (20 MB or so).
  size_t cached{0};       //!< In-memory cache for files read from the disk (the page cache).  Doesn't include SwapCached.
                          //!< includes tmpfs filesystems (shmem) - it cannot be reclaimed!
  size_t swapCache{0};    //!< Memory that once was swapped out, is swapped back in but still also is in the swap file.
                          //!< (If memory pressure is high, these pages don't need to be swapped out again because they
                          //!< are already in the swap file. This saves I/O.)
  size_t swapTotal{0};    //!< Total amount of swap space available.
  size_t swapFree{0};     //!< Amount of swap space that is currently unused.
  size_t anonPages{0};    //!< Non-file backed pages mapped into user-space page tables.
  size_t mapped{0};       //!< Files which have been mapped into memory (with mmap(2)), such as libraries.
  size_t shmem{0};        //!< Amount of memory consumed in tmpfs(5) filesystems.
  size_t sReclaimable{0}; //!< Part of Slab, that might be reclaimed, such as caches.
};

Q_DECLARE_METATYPE(MemInfo)
