#!/bin/bash

rm -rf build

set -e

cmake --preset=ubuntu -DCMAKE_BUILD_TYPE=Debug -DENABLE_WEBRTC=On .

cd build

cmake --build .

sudo cmake --install .