git submodule update --init --recursive
cmake --preset=windows-x64 -DCMAKE_BUILD_TYPE=Release -DENABLE_WEBRTC=On -DENABLE_RELOCATABLE=ON -DENABLE_PORTABLE_CONFIG=ON
cd build_x64
cmake --build . --config Release
cd ..