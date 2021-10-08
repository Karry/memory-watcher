/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#pragma once

#include "String.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <list>

class CmdLineScanner
{
private:
  std::vector<std::string> arguments;
  size_t                   nextArg;

public:
  CmdLineScanner(int argc, char* argv[]); // NOLINT

  explicit CmdLineScanner(const std::vector<std::string>& arguments);

  bool HasNextArg() const;
  std::string PeakNextArg() const;
  std::string Advance();
  std::string GetCurrentArg() const;
};


class CmdLineParseResult
{
private:
  bool        hasError;
  std::string errorDescription;

public:
  CmdLineParseResult();

  explicit CmdLineParseResult(const std::string& errorDescription);

  bool Success() const;
  bool HasError() const;

  std::string GetErrorDescription() const;
};

class CmdLineArgParser
{
private:
  std::string optionName;   //<! The unqualified option name
  std::string argumentName; //<! The actual command line argument name

protected:
  std::string GetOptionName() const;
  std::string GetArgumentName() const;

public:
  virtual ~CmdLineArgParser()=default;

  void SetOptionName(const std::string& optionName);
  void SetArgumentName(const std::string& argumentName);

  virtual std::string GetOptionHint() const = 0;
  virtual std::string GetPositionalHint(const std::string& positional) const = 0;
  virtual CmdLineParseResult Parse(CmdLineScanner& scanner) = 0;
};

using CmdLineArgParserRef = std::shared_ptr<CmdLineArgParser>;

class CmdLineFlagArgParser : public CmdLineArgParser
{
public:
  using SetterFunction = std::function<void (const bool &)>;

private:
   SetterFunction setter;

public:
  explicit CmdLineFlagArgParser(SetterFunction&& setter);

  std::string GetOptionHint() const override;
  std::string GetPositionalHint(const std::string& positional) const override;

  CmdLineParseResult Parse(CmdLineScanner& scanner) override;
};

class CmdLineAlternativeFlagArgParser : public CmdLineArgParser
{
public:
  using SetterFunction = std::function<void (const std::string &)>;

private:
  SetterFunction setter;
  std::string    lastArgumentCalled;

public:
  explicit CmdLineAlternativeFlagArgParser(SetterFunction&& setter);

  std::string GetOptionHint() const override;
  std::string GetPositionalHint(const std::string& positional) const override;

  CmdLineParseResult Parse(CmdLineScanner& scanner) override;
};

class CmdLineBoolArgParser : public CmdLineArgParser
{
public:
  using SetterFunction = std::function<void (const bool &)>;

private:
  SetterFunction setter;

public:
  explicit CmdLineBoolArgParser(SetterFunction&& setter);

  std::string GetOptionHint() const override;
  std::string GetPositionalHint(const std::string& positional) const override;

  CmdLineParseResult Parse(CmdLineScanner& scanner) override;
};

class CmdLineStringArgParser : public CmdLineArgParser
{
public:
  using SetterFunction = std::function<void (const std::string &)>;

private:
  SetterFunction setter;

public:
  explicit CmdLineStringArgParser(SetterFunction&& setter);

  std::string GetOptionHint() const override;
  std::string GetPositionalHint(const std::string& positional) const override;

  CmdLineParseResult Parse(CmdLineScanner& scanner) override;
};

class CmdLineStringListArgParser : public CmdLineArgParser
{
public:
  using AppendFunction = std::function<void (const std::string &)>;

private:
  AppendFunction appender;

public:
  explicit CmdLineStringListArgParser(AppendFunction&& appender);

  std::string GetOptionHint() const override;
  std::string GetPositionalHint(const std::string& positional) const override;

  CmdLineParseResult Parse(CmdLineScanner& scanner) override;
};

template<typename N>
class CmdLineNumberArgParser : public CmdLineArgParser
{
public:
  using SetterFunction = std::function<void (const N &)>;

private:
  SetterFunction setter;

public:
  explicit CmdLineNumberArgParser(SetterFunction&& setter)
    : setter(setter)
  {
    // no code
  }

  std::string GetOptionHint() const override
  {
    return "number";
  }

  std::string GetPositionalHint(const std::string& positional) const override
  {
    return positional;
  }

  CmdLineParseResult Parse(CmdLineScanner& scanner) override
  {
    if (!scanner.HasNextArg()) {
      return CmdLineParseResult("Missing value for number argument '"+GetArgumentName()+"'");
    }

    std::string valueString=scanner.Advance();
    N           value;

    if (StringToNumber(valueString,value)) {
      setter(value);
      return CmdLineParseResult();
    }

    return CmdLineParseResult("Value for number argument '"+GetArgumentName()+"' is not a valid number '"+valueString+"'");
  }
};

template<class ...Args>
CmdLineArgParserRef CmdLineFlag(Args&& ...args)
{
  return std::make_shared<CmdLineFlagArgParser>(std::forward<Args>(args)...);
}

template<class ...Args>
CmdLineArgParserRef CmdLineAlternativeFlag(Args&& ...args)
{
  return std::make_shared<CmdLineAlternativeFlagArgParser>(std::forward<Args>(args)...);
}

template<class ...Args>
CmdLineArgParserRef CmdLineBoolOption(Args&& ...args)
{
  return std::make_shared<CmdLineBoolArgParser>(std::forward<Args>(args)...);
}

template<class ...Args>
CmdLineArgParserRef CmdLineStringOption(Args&& ...args)
{
  return std::make_shared<CmdLineStringArgParser>(std::forward<Args>(args)...);
}

template<class ...Args>
CmdLineArgParserRef CmdLineStringListOption(Args&& ...args)
{
  return std::make_shared<CmdLineStringListArgParser>(std::forward<Args>(args)...);
}

template<class T, class ...Args>
CmdLineArgParserRef CmdLineNumberOption(Args&& ...args)
{
  return std::make_shared<CmdLineNumberArgParser<T>>(std::forward<Args>(args)...);
}

template<class ...Args>
CmdLineArgParserRef CmdLineIntOption(Args&& ...args)
{
  return std::make_shared<CmdLineNumberArgParser<int>>(std::forward<Args>(args)...);
}

template<class ...Args>
CmdLineArgParserRef CmdLineUIntOption(Args&& ...args)
{
  return std::make_shared<CmdLineNumberArgParser<unsigned int>>(std::forward<Args>(args)...);
}

template<class ...Args>
CmdLineArgParserRef CmdLineLongOption(Args&& ...args)
{
  return std::make_shared<CmdLineNumberArgParser<long>>(std::forward<Args>(args)...);
}

template<class ...Args>
CmdLineArgParserRef CmdLineULongOption(Args&& ...args)
{
  return std::make_shared<CmdLineNumberArgParser<unsigned long>>(std::forward<Args>(args)...);
}

template<class ...Args>
CmdLineArgParserRef CmdLineSizeTOption(Args&& ...args)
{
  return std::make_shared<CmdLineNumberArgParser<size_t>>(std::forward<Args>(args)...);
}

template<class ...Args>
CmdLineArgParserRef CmdLineDoubleOption(Args&& ...args)
{
  return std::make_shared<CmdLineNumberArgParser<double>>(std::forward<Args>(args)...);
}

class CmdLineParser
{
private:
  struct CmdLineOption
  {
    CmdLineArgParserRef parser;
    std::string         option;
    std::string         argument;
    bool                stopParsing;

    CmdLineOption(const CmdLineArgParserRef& parser,
                  const std::string& option,
                  const std::string& argument,
                  bool stopParsing)
    : parser(parser),
      option(option),
      argument(argument),
      stopParsing(stopParsing)
    {
      // no code
    }
  };

  struct CmdLinePositional
  {
    CmdLineArgParserRef parser;
    std::string         positional;

    CmdLinePositional(const CmdLineArgParserRef& parser,
                      const std::string& positional)
      : parser(parser),
        positional(positional)
    {
      // no code
    }
  };

  struct CmdLineArgHelp
  {
    std::vector<std::string> argTemplates;
    std::string              helpString;

    CmdLineArgHelp(const std::string& argTemplate,
                   const std::string& helpString)
    : helpString(helpString)
    {
      argTemplates.push_back(argTemplate);
    }

    CmdLineArgHelp(const std::vector<std::string>& argTemplates,
                   const std::string& helpString)
    : argTemplates(argTemplates),
      helpString(helpString)
    {
      // no code
    }
  };

private:
  std::string    appName;
  CmdLineScanner scanner;

  std::map<std::string,CmdLineOption> options;
  std::list<CmdLinePositional>        positionals;
  std::list<CmdLineArgHelp>           optionHelps;
  std::list<CmdLineArgHelp>           positionalHelps;

public:
  CmdLineParser(const std::string& appName,
                int argc, char* argv[]); // NOLINT
  CmdLineParser(const std::string& appName,
                const std::vector<std::string>& arguments);

  void AddOption(const CmdLineArgParserRef& parser,
                 const std::string& optionName,
                 const std::string& helpString,
                 bool stopParsing=false);

  void AddOption(const CmdLineArgParserRef& parser,
                 const std::vector<std::string>& optionNames,
                 const std::string& helpString,
                 bool stopParsing=false);

  void AddPositional(const CmdLineArgParserRef& parser,
                     const std::string& helpString,
                     const std::string& argumentName);

  CmdLineParseResult Parse();

  std::string GetHelp(size_t indent=2) const;
};

extern bool ParseBoolArgument(int argc,
                              char* argv[], // NOLINT
                              int& currentIndex,
                              bool& value);

extern bool ParseStringArgument(int argc,
                                char* argv[], // NOLINT
                                int& currentIndex,
                                std::string& value);

extern bool ParseSizeTArgument(int argc,
                               char* argv[], // NOLINT
                               int& currentIndex,
                               size_t& value);
extern bool ParseUInt32Argument(int argc,
                                char* argv[], // NOLINT
                                int& currentIndex,
                                uint32_t& value);

