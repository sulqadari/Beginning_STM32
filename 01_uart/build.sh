#!/usr/bin/bash

rm -rf build > /dev/null
mkdir build && cd build

cmake -GNinja ../
cmake --build . -v
