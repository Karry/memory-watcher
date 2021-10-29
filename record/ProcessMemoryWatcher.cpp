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

#include "ProcessMemoryWatcher.h"
#include <StatM.h>
#include <Utils.h>
#include <String.h>

#include <QDebug>
#include <QtCore/QFileInfo>
#include <QTextStream>
#include <QtCore/QDateTime>


namespace {

size_t parseMemory(const QString &line) {
  QStringList arr = line.split(" ", SkipEmptyParts);
  if (arr.size() == 3) {
    return arr[1].toULongLong();
  }
  qWarning() << "Can't parse memory line" << line;
  return 0;
}

void parseRange(SmapsRange &range, const QString &line, const ProcessId &processId) {
  QStringList arr = line.split(" ", SkipEmptyParts);
  if (arr.size() < 5) {
    qWarning() << "Can't parse range:" << line;
  }
  QStringList arr2 = arr[0].split("-", SkipEmptyParts);
  if (arr2.size() != 2) {
    qWarning() << "Can't parse range:" << line;
  }
  range.key.processId = processId;
  range.key.from = arr2[0].toULongLong(nullptr, 16);
  range.key.to = arr2[1].toULongLong(nullptr, 16);
  range.key.name = arr.size() >= 6 ? arr[5] : "";
  range.key.permission = arr[1];
  range.rss = 0;
  range.pss = 0;
}
} // namespace


ProcessMemoryWatcher::ProcessMemoryWatcher(QThread *thread,
                                           pid_t pid,
                                           QString procFs):
  processId(pid, procFs),
  thread(thread),
  smapsFile(QString("%1/%2/smaps").arg(procFs).arg(pid)),
  statmFile(QString("%1/%2/statm").arg(procFs).arg(pid)),
  statusFile(QString("%1/%2/status").arg(procFs).arg(pid)),
  oomAdjFile(QString("%1/%2/oom_").arg(procFs).arg(pid)),
  oomScoreFile(QString("%1/%2/oom_").arg(procFs).arg(pid)),
  oomScoreAdjFile(QString("%1/%2/oom_").arg(procFs).arg(pid))
{
  moveToThread(thread);
}

bool ProcessMemoryWatcher::readStatM(StatM &statm) {
  QFile inputFile(statmFile.absoluteFilePath());
  if (!inputFile.open(QIODevice::ReadOnly)) {
    qWarning() << "Can't open file" << statmFile.absoluteFilePath();
    return false;
  }
  QTextStream in(&inputFile);

  QString line = in.readLine();
  if (line.isEmpty()){
    qWarning() << "Can't parse" << statmFile.absoluteFilePath();
    return false;
  }

  QStringList arr = line.split(" ", SkipEmptyParts);
  if (arr.size() < 7){
    qWarning() << "Can't parse" << statmFile.absoluteFilePath();
    return false;
  }
  QVector<size_t> arri;
  arri.reserve(arr.size());
  for (const auto &numStr:arr){
    bool ok;
    arri.push_back(numStr.toULongLong(&ok) * PageSizeKiB);
    if (!ok){
      qWarning() << "Can't parse" << statmFile.absoluteFilePath();
      return false;
    }
  }

  statm.size = arri[0];
  statm.resident = arri[1];
  statm.shared = arri[2];
  statm.text = arri[3];
  statm.lib = arri[4];
  statm.data = arri[5];
  statm.dt = arri[6];

  return true;
}

bool ProcessMemoryWatcher::readInt(const QFileInfo &file, int &value) const {
  if (!file.exists()) {
    return false;
  }

  QFile inputFile(file.absoluteFilePath());
  if (inputFile.open(QIODevice::ReadOnly)) {
    qWarning() << "Can't open file" << file.absoluteFilePath();
    return false;
  }
  QTextStream in(&inputFile);

  QString line = in.readLine();
  if (line.isEmpty()){
    qWarning() << "Can't parse" << file.absoluteFilePath();
    return false;
  }

  bool ok = true;
  int i = line.toInt(&ok);
  if (!ok) {
    qWarning() << "Can't parse" << file.absoluteFilePath() << ":" << line;
    return false;
  }
  value = i;
  return true;
}
OomScore ProcessMemoryWatcher::readOomScore() {
  OomScore result;
  readInt(oomAdjFile, result.adj);
  readInt(oomScoreFile, result.adj);
  readInt(oomScoreAdjFile, result.adj);
  return result;
}

bool ProcessMemoryWatcher::readSmaps(QList<SmapsRange> &ranges)
{
  QFile inputFile(smapsFile.absoluteFilePath());
  if (!inputFile.open(QIODevice::ReadOnly)) {
    qWarning() << "Can't open file" << smapsFile.absoluteFilePath();
    return false;
  }
  QTextStream in(&inputFile);
  bool rangeLine = true;
  SmapsRange range;
  for (QString line = in.readLine(); !line.isEmpty(); line = in.readLine()) {
    if (rangeLine) {
      //qDebug() << line;
      parseRange(range, line, processId);
      rangeLine = false;
    }else{
      if (line.startsWith(lastLineStart)){
        rangeLine = true;
        ranges << range;
        //range.debugPrint();
      } else if (line.startsWith("Rss:")) {
        range.rss = parseMemory(line);
      } else if (line.startsWith("Pss:")) {
        range.pss = parseMemory(line);
      }
    }
  }

  return true;
}

void ProcessMemoryWatcher::update(QDateTime time)
{
  if (thread != QThread::currentThread()) {
    qWarning() << "Incorrect thread;" << thread << "!=" << QThread::currentThread();
  }

  smapsFile.refresh(); // flush cached info, and get updated info
  if (!smapsFile.exists()) {
    // qWarning() << "File" << smapsFile.absoluteFilePath() << "don't exists";
    emit exited(processId);
    return;
  }

  QList<SmapsRange> ranges;
  if (accessible) {
    if (!readSmaps(ranges)) {
      return;
    }
  }

  StatM statm;
  if (!readStatM(statm)){
    return;
  }

  OomScore oomScore = readOomScore();

  emit snapshot(time, processId, ranges, statm, oomScore);
}

bool ProcessMemoryWatcher::initSmaps() {
  if (!smapsFile.exists()){
    qWarning() << "File" << smapsFile.absoluteFilePath() << "don't exists";
    return false;
  }
  QFile inputFile(smapsFile.absoluteFilePath());
  if (!inputFile.open(QIODevice::ReadOnly)) {
    qWarning() << "Can't open file" << smapsFile.absoluteFilePath();
    // not enough privileges?
    return false;
  }
  QTextStream in(&inputFile);
  QString lastLine;
  for (QString line = in.readLine(); !line.isEmpty(); line = in.readLine()) {
    lastLine = line;
  }
  inputFile.close();
  if (lastLine.isEmpty()) {
    // qWarning() << "Last line is empty" << smapsFile.absoluteFilePath();
    // kernel thread ?
    return false;
  }
  QStringList arr = lastLine.split(":", SkipEmptyParts);
  if (arr.size() != 2) {
    qWarning() << "Last line is malformed" << smapsFile.absoluteFilePath();
    return false;
  }
  lastLineStart = arr[0];
  // qDebug() << "lastLineStart:" << lastLineStart;
  return true;
}

QString ProcessMemoryWatcher::readProcessName() const {
  QFile inputFile(statusFile.absoluteFilePath());
  if (!inputFile.open(QIODevice::ReadOnly)) {
    qWarning() << "Can't open file" << statusFile.absoluteFilePath();
    return "";
  }
  QTextStream in(&inputFile);
  QString processName;
  for (QString line = in.readLine(); !line.isEmpty(); line = in.readLine()) {
    if (line.startsWith("Name:")) {
      processName = line.right(line.size() - QString("Name:").size()).trimmed();
      break;
    }
  }
  inputFile.close();
  return processName;
}

void ProcessMemoryWatcher::init()
{
  accessible = initSmaps();
  QString processName = readProcessName();
  if (!processName.isEmpty()) {
    emit initialized(processId, processName);
  }
}
