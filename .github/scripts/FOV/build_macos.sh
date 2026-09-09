#!/bin/bash

git submodule update --init --recursive
cmake --preset macos -DCMAKE_BUILD_TYPE=Debug -DENABLE_WEBRTC=On -DENABLE_RELOCATABLE=ON -DENABLE_PORTABLE_CONFIG=ON
cd build_macos
cmake --build . --config Debug
