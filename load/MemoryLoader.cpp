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

#include "MemoryLoader.h"
#include <StatM.h>
#include <Utils.h>

#include <QDebug>
#include <QtCore/QFileInfo>
#include <QTextStream>
#include <QtCore/QDateTime>

MemoryLoader::MemoryLoader(QThread *thread, [[maybe_unused]] long pid, const QString &smapsFile):
  thread(thread),
  smapsFile(smapsFile)
{
  timer.moveToThread(thread);
}

MemoryLoader::~MemoryLoader()
{
  if (thread != QThread::currentThread()) {
    qWarning() << "Incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  qDebug() << "MemoryWatcher";
  timer.stop();
  thread->quit();
}

size_t parseMemory(const QString &line)
{
  QStringList arr = line.split(" ", QString::SkipEmptyParts);
  if (arr.size() == 3){
    return arr[1].toULongLong();
  }
  qWarning() << "Can't parse memory line" << line;
  return 0;
}

void parseRange(SmapsRange &range, const QString &line)
{
  QStringList arr = line.split(" ", QString::SkipEmptyParts);
  if (arr.size() < 5){
    qWarning() << "Can't parse range:" << line;
  }
  QStringList arr2 = arr[0].split("-", QString::SkipEmptyParts);
  if (arr2.size() != 2){
    qWarning() << "Can't parse range:" << line;
  }
  range.from = arr2[0].toULongLong(nullptr, 16);
  range.to = arr2[1].toULongLong(nullptr, 16);
  range.name = arr.size() >= 6 ? arr[5]: "";
  range.permission = arr[1];
  range.rss = 0;
  range.pss = 0;
}

bool MemoryLoader::readSmaps(QList<SmapsRange> &ranges)
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
      parseRange(range, line);
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
  inputFile.close();
  return true;
}

void MemoryLoader::update()
{
  if (thread != QThread::currentThread()) {
    qWarning() << "Incorrect thread;" << thread << "!=" << QThread::currentThread();
  }

  if (!smapsFile.exists()){
    qWarning() << "File" << smapsFile.absoluteFilePath() << "don't exists";
    return;
  }

  QList<SmapsRange> ranges;
  if (!readSmaps(ranges)){
    return;
  }

  emit loaded(QDateTime::currentDateTime(), ranges);
}

void MemoryLoader::init()
{
  timer.setSingleShot(true);
  timer.setInterval(0);

  if (!smapsFile.exists()){
    qWarning() << "File" << smapsFile.absoluteFilePath() << "don't exists";
    return;
  }

  QFile inputFile(smapsFile.absoluteFilePath());
  if (!inputFile.open(QIODevice::ReadOnly)) {
    qWarning() << "Can't open file" << smapsFile.absoluteFilePath();
    return;
  }
  QTextStream in(&inputFile);
  QString lastLine;
  for (QString line = in.readLine(); !line.isEmpty(); line = in.readLine()) {
    lastLine = line;
  }
  inputFile.close();
  if (lastLine.isEmpty()){
    qWarning() << "Last line is empty" << smapsFile.absoluteFilePath();
    return;
  }
  QStringList arr = lastLine.split(":", QString::SkipEmptyParts);
  if (arr.size() != 2){
    qWarning() << "Last line is malformed" << smapsFile.absoluteFilePath();
    return;
  }
  lastLineStart = arr[0];
  qDebug() << "lastLineStart:" << lastLineStart;

  connect(&timer, &QTimer::timeout, this, &MemoryLoader::update);

  timer.start();
}
