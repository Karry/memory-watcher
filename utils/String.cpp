/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include "String.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <thread>
#include <cmath>
#include <iomanip>

bool GetDigitValue(char digit, size_t& result)
{
  switch (digit) {
    case '1':
      result=1;
      return true;
    case '2':
      result=2;
      return true;
    case '3':
      result=3;
      return true;
    case '4':
      result=4;
      return true;
    case '5':
      result=5;
      return true;
    case '6':
      result=6;
      return true;
    case '7':
      result=7;
      return true;
    case '8':
      result=8;
      return true;
    case '9':
      result=9;
      return true;
    case '0':
      result=0;
      return true;
    case 'a':
    case 'A':
      result=10;
      return true;
    case 'b':
    case 'B':
      result=11;
      return true;
    case 'c':
    case 'C':
      result=12;
      return true;
    case 'd':
    case 'D':
      result=13;
      return true;
    case 'e':
    case 'E':
      result=14;
      return true;
    case 'f':
    case 'F':
      result=15;
      return true;
    default:
      return false;
  }
}

bool StringToBool(const char* string, bool& value)
{
  if (std::strcmp(string,"true")==0) {
    value=true;

    return true;
  }
  else if (std::strcmp(string,"false")==0) {
    value=false;

    return true;
  }

  return false;
}

const char* BoolToString(bool value)
{
  if (value) {
    return "true";
  }

  return "false";
}

bool StringToNumber(const char* string, double& value)
{
  std::istringstream stream(string);

  stream.imbue(std::locale("C"));

  stream >> value;

  return stream.eof();
}

bool StringToNumber(const std::string& string, double& value)
{
  std::istringstream stream(string);

  stream.imbue(std::locale("C"));

  stream >> value;

  return stream.eof();
}

std::string ByteSizeToString(double value)
{
  std::stringstream buffer;

  buffer.setf(std::ios::fixed);
  buffer << std::setprecision(1);

  if (value<1.0 && value>-1) {
    buffer << "0 B";
  }
  else if (ceil(value)>=1024.0*1024*1024*1024) {
    buffer << value/(1024.0*1024*1024*1024) << " TiB";
  }
  else if (ceil(value)>=1024.0*1024*1024) {
    buffer << value/(1024.0*1024*1024) << " GiB";
  }
  else if (ceil(value)>=1024.0*1024) {
    buffer << value/(1024.0*1024) << " MiB";
  }
  else if (ceil(value)>=1024.0) {
    buffer << value/1024.0 << " KiB";
  }
  else {
    buffer << std::setprecision(0);
    buffer << value << " B";
  }

  return buffer.str();
}
