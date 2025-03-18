git submodule update --init --recursive
cmake --preset=windows-x64
cd build_x64
cmake --build .