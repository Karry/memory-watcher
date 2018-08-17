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
#include <unordered_map>

#include <signal.h>
#include <unistd.h>
#include <iostream>

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


QString printWithSeparator(size_t n)
{
  if (n < 1000) {
    return QString().sprintf("%d", (int)n);
  }
  return printWithSeparator(n / 1000) +
         QString().sprintf(" %03d", (int)(n % 1000));
}

std::string align(size_t mem, int size = 9)
{
  QString str = printWithSeparator(mem);
  while (size - str.size() > 0) {
    str = " " + str;
  }
  return str.toStdString();
}

struct Mapping
{
  std::string name;
  size_t size;
};

void Utils::printMeasurementSmapsLike(const Measurement &measurement)
{
  for (const auto &d: measurement.data) {
    const Range &r = measurement.rangeMap[d.rangeId];

    std::cout << "Size: " << align(r.to - r.from, 12) << " Ki " <<
             "Rss: " << align(d.rss) << " Ki " <<
             "Pss: " << align(d.pss) << " Ki : " <<
             QString().sprintf("%zx-%zx", (size_t) r.from, (size_t) r.to).toStdString() << " "<<
             r.permission.toStdString() << " " << r.name.toStdString() <<
             std::endl;
  }
}

void Utils::printMeasurement(const Measurement &measurement, MemoryType type)
{
  size_t threadStacks = 0;
  size_t anonymous = 0;
  size_t heap = 0;
  size_t sum = 0;
  std::unordered_map<std::string, size_t> mappings;
  Range lastMapping;
  for (const auto &d: measurement.data){
    const Range &r = measurement.rangeMap[d.rangeId];
    size_t mem = type == Rss ? d.rss : d.pss;

    if (mem==0){
      continue;
    }

    if (r.name.startsWith("[stack")) {
      threadStacks += mem;
    } else if (r.name == "[heap]") {
      heap += mem;
    } else if (r.name.isEmpty()) {

      if (r.from == lastMapping.to && r.permission == "rw-p"){
        // assume that this is .bss library area
        auto stdName = lastMapping.name.toStdString();
        mappings[stdName] = mappings[stdName] + mem;
      }else {
        anonymous += mem;
      }
    } else {
      auto stdName = r.name.toStdString();
      lastMapping = r;
      mappings[stdName] = mappings[stdName] + mem;
    }
    sum += mem;
  }

  QList<Mapping> sortedMappings;
  for (const auto &it: mappings){
    Mapping m{it.first, it.second};
    sortedMappings << m;
  }
  std::sort(sortedMappings.begin(), sortedMappings.end(), [](const Mapping &a, const Mapping &b) {
    return a.size > b.size;
  });

  constexpr int indent = 43;

  std::cout << "peak measurement: " << measurement.id << std::endl;
  std::cout << "time:             " << measurement.time.toString(Qt::ISODateWithMs).toStdString() << std::endl;

  std::cout << std::endl;
  std::cout << "thread stacks:    " << align(threadStacks, indent) << " Ki" << std::endl;
  std::cout << "heap:             " << align(heap, indent) << " Ki" << std::endl;
  std::cout << "anonymous:        " << align(anonymous, indent) << " Ki" << std::endl;

  int i=0;
  size_t other = 0;
  std::cout << std::endl;
  for (const auto &m: sortedMappings){
    if (i < 20){
      std::cout << m.name << " " << align(m.size, 60 - m.name.size()) << " Ki" << std::endl;
    } else {
      other += m.size;
    }
    i++;
  }

  std::cout << std::endl;
  std::cout << "other mappings:   " << align(other, indent) << " Ki" << std::endl;

  std::cout << std::endl;
  std::cout << "sum:              " << align(sum, indent) << " Ki" << std::endl;
}