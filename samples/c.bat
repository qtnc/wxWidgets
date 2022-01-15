@echo off
mingw32-make -f makefile.gcc SHELL=cmd BUILD=release SHARED=1 USE_OPENGL=0 2>&1 | unix2dos