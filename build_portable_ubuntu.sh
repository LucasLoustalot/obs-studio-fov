#!/bin/bash

rm -rf build

set -e

cmake --preset=ubuntu -DCMAKE_BUILD_TYPE=Debug -DENABLE_WEBRTC=On -DLINUX_PORTABLE=ON .

cd build

cmake --build .

sudo cmake --install .