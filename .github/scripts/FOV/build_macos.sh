#!/bin/bash

git submodule update --init --recursive
cmake --preset macos
cd build_macos
cmake --build .
