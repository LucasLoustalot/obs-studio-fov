#!/bin/bash

git submodule update --init --recursive
cmake --preset=ubuntu
cd build
cmake --build .