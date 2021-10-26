#!/bin/sh -e
SRCDIR="$(readlink -f $(dirname $0)/..)"
docker run --rm=true -v $SRCDIR:/sources -it memory-watcher-ubuntu-2004
