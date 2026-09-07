#!/bin/bash
set -e

read -p "Clean build ? [y/N]: " -n 1 -r
if [[ $REPLY =~ ^[Yy]$ ]]
then
rm -rf build
fi


cmake --preset=ubuntu -DCMAKE_BUILD_TYPE=Release -DENABLE_WEBRTC=On -DLINUX_PORTABLE=ON -DENABLE_RELOCATABLE=ON -DENABLE_PORTABLE_CONFIG=ON .

cd build

cmake --build .

read -p "Run OBS now ? [y/N]: " -n 1 -r
if [[ $REPLY =~ ^[Yy]$ ]]
then
./rundir/Release/bin/obs --portable
else
exit 0
fi

#sudo cmake --install .
