
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
