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
#include "String.h"

#include <QCoreApplication>
#include <QProcess>
#include <QtGlobal>

#include <initializer_list>

#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace {
struct ProcessMemory {
  ProcessMemory(const Measurement &m, ProcessMemoryType type):
    m(m) {
    if (type == StatmRss) {
      memory = m.statm.resident;
    } else if (type == Pss) {
      for (auto const &d : m.data) {
        memory += d.pss;
      }
    } else {
      assert(type == Rss);
      for (auto const &d : m.data) {
        memory += d.rss;
      }
    }
  }
  size_t memory{0};
  Measurement m;
};
} // namespace

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

std::string printWithSeparator(size_t n)
{
  if (n < 1000) {
    return QString::asprintf("%d", (int)n).toStdString();
  }
  return printWithSeparator(n / 1000) +
         QString::asprintf(" %03d", (int)(n % 1000)).toStdString();
}

void Utils::printMeasurementSmapsLike(const Measurement &measurement)
{
  for (const auto &d: measurement.data) {
    const Range &r = measurement.rangeMap[d.rangeId];

    std::cout << "Size: " << std::setw(8) << std::right << printWithSeparator((r.to - r.from) / 1024) << " Ki "
              << "Rss: " << std::setw(8) << std::right << printWithSeparator(d.rss) << " Ki "
              << "Pss: " << std::setw(8) << std::right << printWithSeparator(d.pss) << " Ki : "
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
  constexpr int headerIndent = 16;
  constexpr int memoryIndent = 20;

  std::cout << std::setw(headerIndent) << std::left << "pid:" << measurement.pid << std::endl;
  std::cout << std::setw(headerIndent) << std::left << "process id:" << measurement.processId << std::endl;
  std::cout << std::setw(headerIndent) << std::left << "process name:" << measurement.processName.toStdString() << std::endl;

  std::cout << std::setw(headerIndent) << std::left << "measurement:" << measurement.id << std::endl;

#if QT_VERSION >= 0x050800 // Qt::ISODateWithMs was introduced in Qt 5.8
  std::cout << std::setw(headerIndent) << std::left << "time:" << measurement.time.toString(Qt::ISODateWithMs).toStdString() << std::endl;
#else
  std::cout << std::setw(headerIndent) << std::left << "time:" << measurement.time.toString("yyyy-MM-ddTHH:mm:ss.zzz").toStdString() << std::endl;
#endif

  std::cout << std::endl;
  std::cout << "# statm data" << std::endl;
  std::cout << std::setw(indent) << std::left << "size:"
            << std::setw(memoryIndent) << std::right << printWithSeparator(measurement.statm.size) << " Ki"
            << "   // total program size "
            << "(same as VmSize in /proc/[pid]/status)" << std::endl;

  std::cout << std::setw(indent) << std::left << "resident:"
            << std::setw(memoryIndent) << std::right << printWithSeparator(measurement.statm.resident) << " Ki"
            << "   // resident set size "
            << "(same as VmRSS in /proc/[pid]/status)" << std::endl;

  std::cout << std::setw(indent) << std::left << "shared:"
            << std::setw(memoryIndent) << std::right << printWithSeparator(measurement.statm.shared) << " Ki"
            << "   // number of resident shared pages (i.e., backed by a file) "
            << "(same as RssFile+RssShmem in /proc/[pid]/status)" << std::endl;

  std::cout << std::setw(indent) << std::left << "text:"
            << std::setw(memoryIndent) << std::right << printWithSeparator(measurement.statm.text) << " Ki"
            << "   // text (code)" << std::endl;

  std::cout << std::setw(indent) << std::left << "lib:"
            << std::setw(memoryIndent) << std::right << printWithSeparator(measurement.statm.lib) << " Ki"
            << "   // library (unused since Linux 2.6; always 0)" << std::endl;

  std::cout << std::setw(indent) << std::left << "data:"
            << std::setw(memoryIndent) << std::right << printWithSeparator(measurement.statm.data) << " Ki"
            << "   // data + stack" << std::endl;

  std::cout << std::setw(indent) << std::left << "dt:"
            << std::setw(memoryIndent) << std::right << printWithSeparator(measurement.statm.dt) << " Ki"
            << "   // dirty pages (unused since Linux 2.6; always 0)" << std::endl;

  std::cout << std::endl;
  std::cout << "# smaps data (" << (smapsType == Rss ? "Rss" : "Pss") << ")" << std::endl;
  std::cout << std::setw(indent) << std::left << "thread stacks:"
            << std::setw(memoryIndent) << std::right << printWithSeparator(g.threadStacks) << " Ki" << std::endl;
  std::cout << std::setw(indent) << std::left << "heap:"
            << std::setw(memoryIndent) << std::right << printWithSeparator(g.heap) << " Ki" << std::endl;
  std::cout << std::setw(indent) << std::left << "anonymous:"
            << std::setw(memoryIndent) << std::right << printWithSeparator(g.anonymous) << " Ki" << std::endl;

  int i=0;
  size_t other = 0;
  std::cout << std::endl;
  for (const auto &m: g.sortedMappings()){
    if (i < 20){
      std::cout << std::setw(indent) << std::left << (m.name + " ")
                << std::setw(memoryIndent) << std::right << printWithSeparator(m.size) << " Ki" << std::endl;
    } else {
      other += m.size;
    }
    i++;
  }

  std::cout << std::endl;
  std::cout << std::setw(indent) << std::left << "other mappings:"
            << std::setw(memoryIndent) << std::right << printWithSeparator(other) << " Ki" << std::endl;

  std::cout << std::endl;
  std::cout << std::setw(indent) << std::left << "sum:"
            << std::setw(memoryIndent) << std::right << printWithSeparator(g.sum) << " Ki" << std::endl;
}

void Utils::printProcesses(const QDateTime &time,
                           const MemInfo &memInfo,
                           const QList<Measurement> &processes, ProcessMemoryType processType) {
  using namespace std::string_literals;

  std::cout << "Memory at ";
#if QT_VERSION >= 0x050800 // Qt::ISODateWithMs was introduced in Qt 5.8
  std::cout << time.toString(Qt::ISODateWithMs).toStdString() << std::endl;
#else
  std::cout << time.toString("yyyy-MM-ddTHH:mm:ss.zzz").toStdString() << std::endl;
#endif

  auto f = [](size_t s) -> std::string {
    return ByteSizeToString(double(s) * 1024);
  };

  auto p = [](double val, double sum) -> std::string {
    if (sum <= 0) {
      return "0 %"s;
    }
    std::stringstream buffer;
    buffer.setf(std::ios::fixed);
    buffer << std::setprecision(0);
    buffer << ((val / sum) * 100) << "%";
    return buffer.str();
  };

  std::cout << "Memory details: " << f(memInfo.memTotal) << " total, " << f(memInfo.memFree) << " free, "
            << f(memInfo.buffers) << " buffers, " << f(memInfo.cached) << " cached (including shmem)" << std::endl;
  std::cout << "                " << f(memInfo.swapCache) << " swap cache, " << f(memInfo.sReclaimable) << " SLAB reclaimable "
            << f(memInfo.shmem) << " shmem (tmpfs, zram?)" << std::endl;
  std::cout << "Swap:           " << f(memInfo.swapTotal) << " total, " << f(memInfo.swapFree) << " free (" << p(memInfo.swapFree, memInfo.swapTotal) << ")"<< std::endl;
  std::cout << "Available:      " << f(memInfo.memAvailable) << " (" << p(memInfo.memAvailable, memInfo.memTotal) << ") estimated by kernel" << std::endl;
  size_t computedAvailable = memInfo.memFree + memInfo.buffers + (memInfo.cached - memInfo.shmem) + memInfo.swapCache + memInfo.sReclaimable;
  std::cout << "                " << f(computedAvailable) << " (" << p(computedAvailable, memInfo.memTotal) << ")"
            << " computed. It means: MemFree + Buffers + (Cached - Shmem) + SwapCache + SReclaimable" << std::endl;


  std::cout << std::endl;
  std::cout << "Processes memory (" << (processType == StatmRss? "statm RSS" :(processType == Rss ? "smaps Rss" : "smaps Pss")) << "):" << std::endl;
  std::cout << std::endl;

  constexpr int pidIndent = 7;
  constexpr int processIndent = 40;
  constexpr int memoryIndent = 20;
  std::cout << std::setw(pidIndent) << std::right << "PID" << " "
            << std::setw(processIndent) << std::left << "process"
            << std::setw(memoryIndent) << std::right << "size" << " "
            << "(% of total)" << std::endl;

  std::vector<ProcessMemory> procMem;
  procMem.reserve(processes.size());
  for (const auto &p: processes) {
    procMem.emplace_back(p, processType);
  }
  std::sort(procMem.begin(), procMem.end(), [](const auto &a, const auto &b) {
    return a.memory > b.memory;
  });

  auto printProcess = [&](const std::string &pidStr, const std::string &name, size_t memory) {
    std::cout << std::setw(pidIndent) << std::right << pidStr << " "
              << std::setw(processIndent) << std::left << name
              << std::setw(memoryIndent) << std::right << f(memory) << " "
              << "(" << p(memory, memInfo.memTotal) << ")" << std::endl;
  };

  int i = 0;
  size_t otherSize = 0;
  size_t sumSize = 0;
  for (auto const &proc: procMem) {
    if (i < 30) {
      printProcess(std::to_string(proc.m.pid), proc.m.processName.toStdString(), proc.memory);
    } else {
      otherSize += proc.memory;
    }
    sumSize += proc.memory;
    i++;
  }
  std::cout << std::endl;
  printProcess("", "others", otherSize);
  printProcess("", "sum", sumSize);
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