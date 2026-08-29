@echo off
cls

echo "Retrieving Steinberg VST3 SDK..."
git clone --recursive https://github.com/steinbergmedia/vst3sdk.git --branch v3.7.11_build_10

cd vst3sdk
rmdir /Q /S build
mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release

cd ..
