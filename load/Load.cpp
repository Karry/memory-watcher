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
#include "Load.h"

#include <Utils.h>
#include <Version.h>

#include <QtCore/QCoreApplication>
#include <QDebug>

#include <iostream>
#include <CmdLineParsing.h>

void Load::close()
{
  qDebug() << "close()";
  loader->deleteLater();
  feeder->deleteLater();
  this->deleteLater();
}

Load::Load(long pid, const QString &smapsFile, const QString &databaseFile):
  loader(new MemoryLoader(pid, smapsFile)),
  feeder(new Feeder())
{
  if (!feeder->init(databaseFile)){
    close();
    return;
  }

  connect(loader, &MemoryLoader::loaded,
          feeder, &Feeder::onSnapshot,
          Qt::QueuedConnection);

  connect(feeder, &Feeder::inserted,
          this, &Load::close);

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
  QMetaObject::invokeMethod(loader, "init", Qt::QueuedConnection);
#else
  QMetaObject::invokeMethod(loader, &MemoryLoader::init, Qt::QueuedConnection);
#endif
}

Load::~Load()
{
  qDebug() << "~Load";
  QCoreApplication::quit();
}

struct Arguments {
  bool help{false};
  bool version{false};
  long pid{1};
  QString databaseFile;
  QString smapsFile;
};

class ArgParser: public CmdLineParser {
private:
  Arguments args;

public:
  ArgParser(QCoreApplication *app,
            int argc, char *argv[])
    : CmdLineParser(app->applicationName().toStdString(), argc, argv) {

    using namespace std::string_literals;

    AddOption(CmdLineFlag([this](const bool &value) {
                args.help = value;
              }),
              std::vector<std::string>{"h", "help"},
              "Display help and exits",
              true);

    AddOption(CmdLineFlag([this](const bool &value) {
                args.version = value;
              }),
              std::vector<std::string>{"v", "version"},
              "Display application version and exits",
              false);

    AddOption(CmdLineULongOption([this](const unsigned long &value) {
                args.pid = value;
              }),
              std::vector<std::string>{"p","pid"},
              "Pid of monitored process. Default is 1."s);

    AddOption(CmdLineStringOption([this](const std::string &value){
                args.databaseFile = QString::fromStdString(value);
              }),
              "database-file",
              "Sqlite database file for storing recording. Default is measurement.db");

    AddPositional(CmdLineStringOption([this](const std::string &value){
                    args.smapsFile = QString::fromStdString(value);
                  }),
                  "smaps-file",
                  "smaps file to be loaded to database");
  }

  Arguments GetArguments() const {
    return args;
  }
};

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  Utils::registerQtMetatypes();

  Arguments args;
  {
    ArgParser argParser(&app, argc, argv);

    CmdLineParseResult argResult = argParser.Parse();
    if (argResult.HasError()) {
      std::cerr << "ERROR: " << argResult.GetErrorDescription() << std::endl;
      std::cout << argParser.GetHelp() << std::endl;
      return 1;
    }

    args = argParser.GetArguments();
    if (args.help) {
      std::cout << argParser.GetHelp() << std::endl;
      return 0;
    }
    if (args.version) {
      std::cout << MEMORY_WATCHER_VERSION_STRING << std::endl;
      return 0;
    }
  }

  if (args.databaseFile.isEmpty()) {
    args.databaseFile = QString("measurement.db");
  }

  new Load(args.pid, args.smapsFile, args.databaseFile);

  int result = app.exec();
  qDebug() << "Main loop ended...";
  return result;
}
