
#include "SmapsWatcher.h"
#include "Watcher.h"

#include <Utils.h>
#include <ThreadPool.h>

#include <QtCore/QCoreApplication>
#include <QDebug>

#include <iostream>
#include <signal.h>


void Watcher::close()
{
  qDebug() << "close()";
  watcher->deleteLater();
  feeder->deleteLater();
  threadPool.close();
}

Watcher::Watcher(long pid, long period):
  watcherThread(threadPool.makeThread("watcher")),
  watcher(new SmapsWatcher(watcherThread, pid, period)),
  feeder(new Feeder())
{
  connect(&threadPool, SIGNAL(closed()), this, SLOT(deleteLater()));

  // init watcher
  watcher->moveToThread(watcherThread);
  QObject::connect(watcherThread, SIGNAL(started()),
                   watcher, SLOT(init()));
  watcherThread->start();

  if (!feeder->init("measurement.db")){
    close();
    return;
  }

  connect(watcher, SIGNAL(snapshot(QDateTime, QList<SmapsRange>)),
          feeder, SLOT(onSnapshot(QDateTime, QList<SmapsRange>)),
          Qt::QueuedConnection);

  // for debug
  shutdownTimer.setSingleShot(true);
  shutdownTimer.setInterval(10000);
  connect(&shutdownTimer, SIGNAL(timeout()), this, SLOT(close()));
  shutdownTimer.start();
}

Watcher::~Watcher()
{
  qDebug() << "~Watcher";
  QCoreApplication::quit();
}

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  qRegisterMetaType<QList<SmapsRange>>("QList<SmapsRange>");

  if (app.arguments().size() < 2){
    std::cerr << "Usage:" << std::endl;
    std::cerr << app.applicationName().toStdString() << " PID [period-ms]" << std::endl;
    return 1;
  }

  long pid = app.arguments()[1].toLong();
  long period = 1000;
  if (app.arguments().size() >= 3){
    period = app.arguments()[2].toLong();
  }

  Watcher *watcher = new Watcher(pid, period);
  std::function<void(int)> signalCallback = [&](int){ watcher->close(); };
  Utils::catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP},
                          &signalCallback);

  int result = app.exec();
  Utils::cleanSignalCallback();
  qDebug() << "Main loop ended...";
  return result;
}
