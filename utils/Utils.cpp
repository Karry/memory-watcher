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
#include "MemInfo.h"

#include <QCoreApplication>
#include <QProcess>
#include <QtGlobal>

#include <initializer_list>

#include <signal.h>
#include <unistd.h>
#include <iostream>

static std::function<void(int)> *signalHandler = nullptr;

void Utils::cleanSignalCallback()
{
  signalHandler = nullptr;
}

// TODO: it is not safe to do multiple operations from sigaction!
// It may be invoked from any thread (that don't have blocked signals), at any time.
// Even when mutexes are locked. sigwait with dedicated thread should be used instead.
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
    return QString::asprintf("%d", (int)n);
  }
  return printWithSeparator(n / 1000) +
         QString::asprintf(" %03d", (int)(n % 1000));
}

std::string align(size_t mem, int size = 9)
{
  QString str = printWithSeparator(mem);
  while (size - str.size() > 0) {
    str = " " + str;
  }
  return str.toStdString();
}

void Utils::printMeasurementSmapsLike(const Measurement &measurement)
{
  for (const auto &d: measurement.data) {
    const Range &r = measurement.rangeMap[d.rangeId];

    std::cout << "Size: " << align((r.to - r.from) / 1024, 12) << " Ki "
              << "Rss: " << align(d.rss) << " Ki "
              << "Pss: " << align(d.pss) << " Ki : "
              << QString::asprintf("%zx-%zx", (size_t) r.from, (size_t) r.to).toStdString() << " "
              << r.permission.toStdString() << " " << r.name.toStdString()
              << std::endl;
  }
}

void Utils::group(MeasurementGroups &g, const Measurement &measurement, ProcessMemoryType type, bool groupSockets)
{
  g.threadStacks = 0;
  g.anonymous = 0;
  g.heap = 0;
  g.sum = 0;
  g.mappings.clear();
  g.statm = measurement.statm;

  Range lastElfMapping;
  for (const auto &d: measurement.data){
    const Range &r = measurement.rangeMap[d.rangeId];
    size_t mem = type == Rss ? d.rss : d.pss;

    if (mem==0){
      continue;
    }

    if (r.name.startsWith("[stack")) {
      g.threadStacks += mem;
    } else if (groupSockets && r.name.startsWith("socket:")) {
      g.sockets += mem;
    } else if (r.name == "[heap]") {
      g.heap += mem;
    } else if (r.name.isEmpty()) {

      if (r.from == lastElfMapping.to && r.permission == "rw-p"){
        // assume that this is .bss library area
        auto stdName = lastElfMapping.name.toStdString();
        g.mappings[stdName] = g.mappings[stdName] + mem;
      }else {
        g.anonymous += mem;
      }
    } else {
      auto stdName = r.name.toStdString();

      if (r.permission=="r-xp") {
        lastElfMapping = r; // .text library area
      } else if ((r.permission == "r--p" || r.permission == "rw-p") &&
                 r.name == lastElfMapping.name) {
        // read-only data (constants) and data of library
        lastElfMapping.to = r.to;
      } else {
        // memory mapped, non executable file
        lastElfMapping = Range();
      }

      g.mappings[stdName] = g.mappings[stdName] + mem;
    }
    g.sum += mem;
  }
}

std::vector<Mapping> MeasurementGroups::sortedMappings() const
{
  std::vector<Mapping> sortedMappings;
  sortedMappings.reserve(mappings.size());
  for (const auto &it: mappings){
    Mapping m{it.first, it.second};
    sortedMappings.push_back(m);
  }
  std::sort(sortedMappings.begin(), sortedMappings.end(), [](const Mapping &a, const Mapping &b) {
    return a.size > b.size;
  });
  return sortedMappings;
}

void Utils::printMeasurement(const Measurement &measurement, ProcessMemoryType type)
{
  MeasurementGroups g;
  ProcessMemoryType smapsType = type;
  if (type == StatmRss){
    smapsType = Rss;
  }
  group(g, measurement, smapsType);

  constexpr int indent = 80;

  std::cout << "measurement:      " << measurement.id << std::endl;

#if QT_VERSION >= 0x050800 // Qt::ISODateWithMs was introduced in Qt 5.8
  std::cout << "time:             " << measurement.time.toString(Qt::ISODateWithMs).toStdString() << std::endl;
#else
  std::cout << "time:             " << measurement.time.toString("yyyy-MM-ddTHH:mm:ss.zzz").toStdString() << std::endl;
#endif

  std::cout << std::endl;
  std::cout << "# statm data" << std::endl;
  std::cout << "size:             " << align(measurement.statm.size, indent) << " Ki"
            << "   // total program size "
            << "(same as VmSize in /proc/[pid]/status)" << std::endl;

  std::cout << "resident:         " << align(measurement.statm.resident, indent) << " Ki"
            << "   // resident set size "
            << "(same as VmRSS in /proc/[pid]/status)" << std::endl;

  std::cout << "shared:           " << align(measurement.statm.shared, indent) << " Ki"
            << "   // number of resident shared pages (i.e., backed by a file) "
            << "(same as RssFile+RssShmem in /proc/[pid]/status)" << std::endl;

  std::cout << "text:             " << align(measurement.statm.text, indent) << " Ki"
            << "   // text (code)" << std::endl;

  std::cout << "lib:              " << align(measurement.statm.lib, indent) << " Ki"
            << "   // library (unused since Linux 2.6; always 0)" << std::endl;

  std::cout << "data:             " << align(measurement.statm.data, indent) << " Ki"
            << "   // data + stack" << std::endl;

  std::cout << "dt:               " << align(measurement.statm.dt, indent) << " Ki"
            << "   // dirty pages (unused since Linux 2.6; always 0)" << std::endl;

  std::cout << std::endl;
  std::cout << "# smaps data (" << (smapsType == Rss ? "Rss" : "Pss") << ")" << std::endl;
  std::cout << "thread stacks:    " << align(g.threadStacks, indent) << " Ki" << std::endl;
  std::cout << "heap:             " << align(g.heap, indent) << " Ki" << std::endl;
  std::cout << "anonymous:        " << align(g.anonymous, indent) << " Ki" << std::endl;

  int i=0;
  size_t other = 0;
  std::cout << std::endl;
  for (const auto &m: g.sortedMappings()){
    if (i < 20){
      std::cout << m.name << " " << align(m.size, indent+17 - m.name.size()) << " Ki" << std::endl;
    } else {
      other += m.size;
    }
    i++;
  }

  std::cout << std::endl;
  std::cout << "other mappings:   " << align(other, indent) << " Ki" << std::endl;

  std::cout << std::endl;
  std::cout << "sum:              " << align(g.sum, indent) << " Ki" << std::endl;
}

void Utils::clearScreen()
{
  //system("clear");
  // TODO: remove execution of external binary
  QProcess::execute("clear", QStringList());
}

void Utils::registerQtMetatypes() {
  qRegisterMetaType<StatM>("StatM");
  qRegisterMetaType<ProcessId>("ProcessId");
  qRegisterMetaType<OomScore>("OomScore");
  qRegisterMetaType<QList<SmapsRange>>("QList<SmapsRange>");
  qRegisterMetaType<MemInfo>("MemInfo");
}