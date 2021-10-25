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

#include "ProcessId.h"

#include <QString>

struct SmapsRange {
  struct Key {
    ProcessId processId;

    size_t from{0};
    size_t to{0};
    QString permission;
    QString name;

    friend bool operator<(const Key&, const Key&);

    qulonglong hash() const;
    void debugPrint() const;
    size_t sizeBytes() const;
  };

  Key key;

  size_t rss{0}; // Ki
  size_t pss{0}; // Ki

  void debugPrint() const;
};
