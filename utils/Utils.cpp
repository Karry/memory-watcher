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

#include "Utils.h"

#include <QCoreApplication>

#include <initializer_list>
#include <signal.h>
#include <unistd.h>

static std::function<void(int)> *signalHandler = nullptr;

void Utils::cleanSignalCallback()
{
  signalHandler = nullptr;
}

void Utils::catchUnixSignals(std::initializer_list<int> quitSignals,
                             std::function<void(int)> *delegateHandler) {

  signalHandler = delegateHandler;

  auto handler = [](int sig) -> void {

    if (signalHandler)
      (*signalHandler)(sig);

    // blocking and not aysnc-signal-safe func are valid
    //printf("\nquit the application by signal(%d).\n", sig);
    //QCoreApplication::quit();
  };


  sigset_t blocking_mask;
  sigemptyset(&blocking_mask);
  for (auto sig : quitSignals)
    sigaddset(&blocking_mask, sig);

  struct sigaction sa;
  sa.sa_handler = handler;
  sa.sa_mask    = blocking_mask;
  sa.sa_flags   = 0;

  for (auto sig : quitSignals)
    sigaction(sig, &sa, nullptr);
}
