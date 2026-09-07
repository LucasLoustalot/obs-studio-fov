git submodule update --init --recursive
cmake --preset=windows-x64 -DENABLE_PORTABLE_CONFIG=ON
cd build_x64
cmake --build .
cd ..