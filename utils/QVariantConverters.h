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

#ifndef QVARIANTCONVERTERS_H
#define QVARIANTCONVERTERS_H

#include <QVariant>
#include <QDateTime>

namespace converters {

inline qlonglong varToLong(const QVariant &var, qlonglong def = -1)
{
  bool ok;
  qlonglong val = var.toLongLong(&ok);
  return ok ? val : def;
}

inline QString varToString(const QVariant &var, QString def = "")
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::QString)) {
    return var.toString();
  }

  return def;
}

inline bool varToBool(const QVariant &var, bool def = false)
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::Bool)) {
    return var.toBool();
  }

  return def;
}

inline QDateTime varToDateTime(const QVariant &var, QDateTime def = QDateTime())
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::QDateTime)) {
    return var.toDateTime();
  }

  return def;
}

inline double varToDouble(const QVariant &var, double def = 0)
{
  if (!var.isNull() &&
      var.isValid() &&
      var.canConvert(QMetaType::Double)) {
    return var.toDouble();
  }

  return def;
}

}

#endif //QVARIANTCONVERTERS_H
