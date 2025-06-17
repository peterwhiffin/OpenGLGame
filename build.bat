@echo off
echo Building...
set arg1=%1
pushd build
cl /Feopenglgame /EHsc /Zi /DEBUG /MTd /std:c++17 /Zc:inline /fp:fast /D PETES_EDITOR /D _MBCS /D WIN32 /D _WINDOWS /D _HAS_EXCEPTIONS=0 /D _DEBUG /D JPH_FLOATING_POINT_EXCEPTIONS_ENABLED /D JPH_DEBUG_RENDERER /D JPH_PROFILE_ENABLED /D JPH_OBJECT_STREAM /D JPH_USE_AVX2 /D JPH_USE_AVX /D JPH_USE_SSE4_1 /D JPH_USE_SSE4_2 /D JPH_USE_LZCNT /D JPH_USE_TZCNT /D JPH_USE_F16C /D JPH_USE_FMADD libcmtd.lib glfw3.lib user32.lib gdi32.lib shell32.lib ../src/*.cpp ../src/utils/*.cpp ../src/utils/*.c -I../include -I../../JoltPhysics-5.3.0 -I../../OpenAL-soft/include/AL -I../../imgui-docking /link /libpath:../lib assimp-vc143-mt.lib Jolt.lib OpenAL32.lib /NODEFAULTLIB:libcmt
popd
if "%arg1%" == "r" (run.bat)
