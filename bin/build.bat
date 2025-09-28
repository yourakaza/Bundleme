@echo off

cmake -B build -S . -G "MinGW Makefiles"
cmake --build build --config Release
