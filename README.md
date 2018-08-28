
# Memory watcher

Goal of this project is provide high level overview about memory consumption of Linux process 
and provide its progress in time. These tools don't provide deep view to heap usage 
like [Gperftools](https://github.com/gperftools/gperftools) or [Heaptrack](https://github.com/KDE/heaptrack), 
but "outside" monitoring of complete **Rss**/**Pss** layout. Heap size may be just fraction of all
used memory, especially when some library manage its memory by anonymous mappings (mmap) and don't use 
libc allocator.

## How it works

`memory-watcher` reads `/proc/<PID>/smaps` file periodically and store information about memory regions 
to SQLite database. This database may be read by custom scripts or provided tools...

Note that Rss and Pss sizes obtained from `smaps` file may differ from Rss visible in `status`, 
`statm` or `getrusage` syscall. As explained 
on StackOverflow ["Linux OS: smaps vs statm"](https://stackoverflow.com/a/30799817/1632737)
and LKML thread ["why smaps Rss is different from statm"](https://lkml.org/lkml/2016/3/30/171), 
this value is read from counters that may be outdated (performance optimisation) 
and most likely compute Rss differently (TODO: explanation). 

### Record tool

This tool records process memory with given period (milliseconds) and store measurement to database. 

```bash
./memory-record PID period-ms
```

Tool will exit on SIGQUIT, SIGINT (Ctrl+C), SIGTERM or SIGHUP signal.

When you want to record memory on small system where installation of Qt would be problematic, 
it is possible to do it via sshfs (sftp-server is required on the remote device).

```bash
mkdir proc
sshfs -odirect_io  root@192.168.1.1:/proc `pwd`/proc
PROCFS=./proc memory-record remote-PID period-ms
```

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
