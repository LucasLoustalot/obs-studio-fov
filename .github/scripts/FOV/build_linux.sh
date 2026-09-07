#!/bin/bash

git submodule update --init --recursive
cmake --preset=ubuntu -DCMAKE_BUILD_TYPE=Debug -DLINUX_PORTABLE=ON -DENABLE_RELOCATABLE=ON -DENABLE_PORTABLE_CONFIG=ON .
cd build
cmake --build .