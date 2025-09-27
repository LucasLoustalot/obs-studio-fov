#!/bin/bash

git submodule update --init --recursive
./requirements.sh
cmake --preset=ubuntu
cd build
cmake --build .