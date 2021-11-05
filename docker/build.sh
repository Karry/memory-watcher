#!/bin/sh -e
SRCDIR="$(readlink -f $(dirname $0)/..)"
docker run --rm=true -v $SRCDIR:/sources -e CXX=g++-10  -e CC=gcc-10 -e BUILD_DIR=build-ubuntu2004-gcc   -it memory-watcher-ubuntu-2004
docker run --rm=true -v $SRCDIR:/sources -e CXX=clang++ -e CC=clang  -e BUILD_DIR=build-ubuntu2004-clang -it memory-watcher-ubuntu-2004
docker run --rm=true -v $SRCDIR:/sources -e CXX=g++-11  -e CC=gcc-11 -e BUILD_DIR=build-ubuntu2104-gcc   -it memory-watcher-ubuntu-2104
docker run --rm=true -v $SRCDIR:/sources -e CXX=clang++ -e CC=clang  -e BUILD_DIR=build-ubuntu2104-clang -it memory-watcher-ubuntu-2104
