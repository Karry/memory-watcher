#!/bin/sh -xe

if [ ! -d /sources ] ; then
  git clone --recursive git@github.com:Karry/memory-watcher.git /sources
fi

BUILD_DIR=${BUILD_DIR:-build-ubuntu2004}

mkdir -p /sources/$BUILD_DIR
cd /sources/$BUILD_DIR
cmake -DCMAKE_UNITY_BUILD=ON -Wno-dev -G "Ninja" ..
cmake --build . -- all unittests
ctest -j $(nproc) --output-on-failure
