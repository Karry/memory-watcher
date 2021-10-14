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

#include "ProcessId.h"

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QTextStream>

ProcessId::StartTime ProcessId::processStartTime(pid_t pid, const QString &procFs) {
  QFile statFile(QString("%1/%2/stat").arg(procFs).arg(pid));
  if (!statFile.open(QIODevice::ReadOnly)) {
    qWarning() << "Can't open file" << statFile;
    return 0;
  }
  QTextStream in(&statFile);

  QString line = in.readLine();
  if (line.isEmpty()){
    qWarning() << "Can't parse" << statFile;
    return 0;
  }

  int execNameEnd = line.lastIndexOf(')');
  if (execNameEnd < 0) {
    qWarning() << "Can't parse" << statFile;
    return 0;
  }

  QStringList arr = line.right((line.size() - execNameEnd) - 1).split(" ", QString::SkipEmptyParts);
  if (arr.size() < 20){
    qWarning() << "Can't parse" << statFile;
    return 0;
  }

  // /proc/[pid]/stat
  // (22) starttime  %llu
  //      The time the process started after system boot.
  //      In kernels before Linux 2.6, this value was expressed in jiffies.
  //      Since Linux 2.6, the value is expressed in clock ticks (divide by sysconf(_SC_CLK_TCK)).
  //      The format for this field was %lu before Linux 2.6.

  // we skipped firt pid and comm columns
  bool ok;
  ProcessId::StartTime result = arr[19].toULongLong(&ok);
  if (!ok) {
    qWarning() << "Can't parse" << statFile;
    return 0;
  }

  return result;
}

bool operator<(const ProcessId &a, const ProcessId &b) {
  return std::tie(a.pid, a.startTime) < std::tie(b.pid, b.startTime);
}


#ifdef UNIT_TESTS

#include <catch2/catch.hpp>

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QRandomGenerator>

TEST_CASE("/proc/<pid>/stat parsing test") {

  quint32 rndVal = QRandomGenerator::global()->generate();
  QDir procFs(QString("%1/proc-%2").arg(QDir::tempPath()).arg(rndVal));
  qDebug() << "test proc fs:" << procFs.path();
  QDir processDir(procFs.path() + "/1");
  REQUIRE(processDir.mkpath(processDir.path()));
  QFile stat(processDir.path() + "/stat");
  REQUIRE(stat.open(QFile::WriteOnly));

  {
    QTextStream stream(&stat);

    stream << "285465 (a(bc) .sh) S 8557 285465 8557 34820 285465 4194304 173 0 2 0 0 0 0 0 20 0 1 0 ";
    stream << "9493934 12677120 851 18446744073709551615 94349822259200 94349822981893 140733215755776 ";
    stream << "0 0 0 65536 4 65538 1 0 0 17 2 0 0 0 0 0 94349823212784 94349823260164 94349840445440 ";
    stream << "140733215764627 140733215764649 140733215764649 140733215768556 0";
    stream << "\n";
  }

  stat.close();

  qDebug() << "test stat file:" << stat.fileName();
  REQUIRE(ProcessId::processStartTime(1, procFs.path()) == 9493934);
  REQUIRE(stat.remove());
  REQUIRE(procFs.removeRecursively());
}

#endif // UNIT_TESTS
