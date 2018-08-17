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

#ifndef MEMORY_WATCHER_UTILS_H
#define MEMORY_WATCHER_UTILS_H

#include <QThread>
#include <QString>
#include <functional>

class Utils {
public:

  static void catchUnixSignals(std::initializer_list<int> quitSignals,
                               std::function<void(int)> *handler);

  static void cleanSignalCallback();
};


#endif //MEMORY_WATCHER_UTILS_H
