r audio player is a portable, lightweight audio player, made mainly for windows, built with c++ using qt 6.10.1, taglib, zlib, and miniaudio. a windows release build (and its source code) of the latest version is always available.

it is currently in early development, and as such many features are missing, incomplete, or bugged, and the source code may go through sudden refactors.

to build; install vcpkg, then zlib and taglib (in that order), then run for example:

cd "C:\Users\PC\Desktop\projs\r audio player"
rmdir /s /q build
mkdir build
cd build

cmake -S "..\r audio player" -B . ^
  -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=C:/Users/PC/Desktop/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows

cmake --build . --config Release
