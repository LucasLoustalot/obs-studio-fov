#!/bin/bash
set -e

read -p "Clean build ? [y/N]: " -n 1 -r
if [[ $REPLY =~ ^[Yy]$ ]]
then
rm -rf build
fi


cmake --preset=ubuntu -DCMAKE_BUILD_TYPE=Debug -DENABLE_WEBRTC=On -DLINUX_PORTABLE=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo .

cd build

cmake --build .

read -p "Run OBS now ? [y/N]: " -n 1 -r
if [[ $REPLY =~ ^[Yy]$ ]]
then
./rundir/RelWithDebInfo/bin/obs --portable
else
exit 0
fi

#sudo cmake --install .
