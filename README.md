
# Memory watcher

Goal of this project is provide high level overview about memory consumption of Linux process 
and provide its progress in time. These tools don't provide deep view to heap usage like  
[Gperftools](https://github.com/gperftools/gperftools) or [Heaptrack](https://github.com/KDE/heaptrack), 
but "outside" monitoring of complete **Rss**/**Pss** layout. Heap size may be just fraction of all
used memory, especially when some library manage its memory by anonymous mappings (mmap) and don't use 
libc allocator.

## How it works

`memory-watcher` reads `/proc/<PID>/smaps` file periodically and store information about memory regions 
to SQLite database. This database may be read by custom scripts or provided tools...      

### Peak tool

This tool select peak memory (Rss) usage in recording and prints summary.

```bash
# ./memory-peak measurement.db
peak measurement: 481
time:             2018-08-17T15:58:03.942

thread stacks:                                            128 Ki
heap:                                                       0 Ki
anonymous:                                            541 812 Ki

/SYSV00000000                                          41 432 Ki
/dev/shm/org.chromium.rV92cn                           37 640 Ki
/usr/lib/firefox/libxul.so                             34 180 Ki
/usr/lib/x86_64-linux-gnu/libXss.so.1.0.0              15 528 Ki
/dev/shm/org.chromium.ifl8ui                           12 996 Ki
/usr/lib/x86_64-linux-gnu/libpulse.so.0.20.2           11 384 Ki
/usr/lib/x86_64-linux-gnu/libavcodec.so.57.107.100     10 600 Ki
/dev/shm/org.chromium.BUMGU8                            9 172 Ki
/usr/lib/x86_64-linux-gnu/libjpeg.so.8.1.2              5 956 Ki
/usr/lib/firefox/libsoftokn3.so                         3 208 Ki
/usr/lib/x86_64-linux-gnu/libdbusmenu-glib.so.4.0.12    2 044 Ki
/usr/lib/firefox/libmozavutil.so                        1 072 Ki
/usr/lib/firefox/libnspr4.so                            1 064 Ki
/lib/x86_64-linux-gnu/libnss_files-2.27.so              1 048 Ki
/usr/lib/x86_64-linux-gnu/libx265.so.146                1 016 Ki
/lib/x86_64-linux-gnu/libc-2.27.so                        780 Ki
/dev/shm/org.chromium.HWi8uq                              716 Ki
/usr/lib/firefox/libmozsqlite3.so                         608 Ki
/usr/lib/x86_64-linux-gnu/libgtk-3.so.0.2200.30           476 Ki
/usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.25             420 Ki

other mappings:                                         7 700 Ki

sum:                                                  740 980 Ki
```
