@echo off
mingw32-make -f makefile.gcc SHELL=cmd BUILD=release SHARED=1 USE_OPENGL=0 VENDOR=x86 CFLAGS="-O3 -s" CXXFLAGS="-std=gnu++17 -O3 -s" LDFLAGS="-O3 -s"