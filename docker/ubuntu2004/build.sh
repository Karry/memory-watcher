#!/bin/sh -xe

if [ ! -d /sources ] ; then
  git clone --recursive git@github.com:Karry/memory-watcher.git /sources
fi

mkdir -p /sources/build
cd /sources/build
cmake -DCMAKE_UNITY_BUILD=ON -Wno-dev -G "Ninja" ..
cmake --build . -- all unittests
ctest -j 2 --output-on-failure
