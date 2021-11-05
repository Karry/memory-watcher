# docker build --tag memory-watcher-ubuntu-2104 docker/ubuntu2104
# docker run --rm=true -it memory-watcher-ubuntu-2104
FROM ubuntu:21.04

# disable interactive functions
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && \
    apt-get install -y apt-utils && \
    apt-get install -y --no-install-recommends \
        clang gcc-11 g++-11 libtbb-dev ccache libtool pkg-config cmake ninja-build \
        qttools5-dev-tools qttools5-dev libqt5sql5-sqlite libqt5charts5-dev \
        curl catch2 git ca-certificates && \
    # upgrade OS
    apt-get -y dist-upgrade && \
    apt-get autoremove -y && \
    apt-get clean all

ENV CXX g++-11
ENV CC gcc-11

COPY build.sh /build.sh
CMD /build.sh
