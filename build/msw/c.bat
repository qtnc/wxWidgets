@echo off
mingw32-make -f makefile.gcc SHELL=cmd BUILD=release SHARED=1 MONOLITHIC=0 USE_GUI=1 USE_OPENGL=0 DEBUG_FLAG=0 DEBUG_INFO=0 VENDOR=x86 CFLAGS="-O3 -s" CXXFLAGS="-std=gnu++17 -O3 -s" LDFLAGS="-O3 -s"