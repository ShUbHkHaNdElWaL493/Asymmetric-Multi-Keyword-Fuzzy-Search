#!/bin/bash

cd -- "$( dirname -- "${BASH_SOURCE[0]}" )"

cmake -B build
cmake --build build
./FYP2026_MAIN