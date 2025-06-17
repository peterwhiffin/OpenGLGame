@echo off
pushd build
start x:/rad/raddbg.exe --quit_after_success --auto_run openglgame.exe
popd
