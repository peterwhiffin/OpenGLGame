@echo off
mkdir build
pushd build
cl /Felearnopengl /EHsc /Zi /DEBUG msvcrt.lib glfw3.lib user32.lib gdi32.lib shell32.lib ../src/*.cpp ../src/*.c -I../include /link /libpath:../lib/ assimp-vc143-mt.lib /NODEFAULTLIB:libcmt
learnopengl.exe
popd
