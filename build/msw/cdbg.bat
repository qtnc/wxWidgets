@echo off
mingw32-make -f makefile.gcc SHELL=cmd BUILD=debug SHARED=1 USE_OPENGL=0 VENDOR=mgw CXXFLAGS="-std=gnu++17" 