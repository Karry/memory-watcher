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


#include "SmapsRange.h"

#include <QDebug>

bool operator<(const SmapsRange::Key& a, const SmapsRange::Key& b) {
  return std::tie(a.processId, a.from, a.to, a.permission, a.name) <
         std::tie(b.processId, b.from, b.to, b.permission, b.name);
}

qulonglong SmapsRange::Key::hash() const {
  return processId.hash() ^
    (qulonglong(from) + qulonglong(to)) ^
    (qulonglong(qHash(permission)) << 32);
}

size_t SmapsRange::Key::sizeBytes() const {
  return to - from;
}

void SmapsRange::Key::debugPrint() const {
  qDebug() << QString::asprintf("%zx-%zx", from, to) << permission
           << name;
}

void SmapsRange::debugPrint() const {
  qDebug() << QString::asprintf("%zx-%zx", key.from, key.to) << key.permission
           << key.name << rss << pss;
}
