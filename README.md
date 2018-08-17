
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
