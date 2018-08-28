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

#include "SmapsWatcher.h"

#include <QDebug>
#include <QtCore/QFileInfo>
#include <QTextStream>
#include <QtCore/QDateTime>

SmapsWatcher::SmapsWatcher(QThread *thread, long pid, long period, QString procFs):
  thread(thread),
  period(period),
  smapsFile(QString("%1/%2/smaps").arg(procFs).arg(pid))
{
  timer.moveToThread(thread);
}

SmapsWatcher::~SmapsWatcher()
{
  if (thread != QThread::currentThread()) {
    qWarning() << "Incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  qDebug() << "~SmapsWatcher";
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

void SmapsWatcher::update()
{
  if (thread != QThread::currentThread()) {
    qWarning() << "Incorrect thread;" << thread << "!=" << QThread::currentThread();
  }

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
  bool rangeLine = true;
  QList<SmapsRange> ranges;
  SmapsRange range;
  for (QString line = in.readLine(); !line.isEmpty(); line = in.readLine()) {
    if (rangeLine) {
      //qDebug() << line;
      parseRange(range, line);
      rangeLine = false;
    }else{
      if (line.startsWith("VmFlags:")){
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

  emit snapshot(QDateTime::currentDateTime(), ranges);
}

void SmapsWatcher::init()
{
  timer.setSingleShot(false);
  timer.setInterval(period);

  connect(&timer, SIGNAL(timeout()), this, SLOT(update()));

  timer.start();
}
