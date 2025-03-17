#!/bin/bash

git submodule update --init --recursive
cmake --preset=ubuntu
cd build_ubuntu
cmake --build .