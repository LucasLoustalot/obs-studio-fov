#!/bin/bash

git submodule update --init --recursive
cmake --preset=macos-ci
cd build_macos
cmake --build .
