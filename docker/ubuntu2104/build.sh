#!/bin/sh -xe

if [ ! -d /sources ] ; then
  git clone --recursive https://github.com/Karry/memory-watcher.git /sources
fi

BUILD_DIR=${BUILD_DIR:-build-ubuntu2104}

mkdir -p /sources/$BUILD_DIR
cd /sources/$BUILD_DIR
cmake -DCMAKE_UNITY_BUILD=ON -Wno-dev -G "Ninja" ..
cmake --build . -- all unittests
ctest -j $(nproc) --output-on-failure
