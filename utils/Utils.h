
#ifndef MEMORY_WATCHER_UTILS_H
#define MEMORY_WATCHER_UTILS_H

#include <QThread>
#include <QString>
#include <functional>

class Utils {
public:

  static void catchUnixSignals(std::initializer_list<int> quitSignals,
                               std::function<void(int)> *handler);

  static void cleanSignalCallback();
};


#endif //MEMORY_WATCHER_UTILS_H
