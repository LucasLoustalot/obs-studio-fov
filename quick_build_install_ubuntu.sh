#!/bin/bash

set -e

cd build

cmake --build .

sudo cmake --install .