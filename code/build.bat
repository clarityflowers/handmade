@echo off


mkdir ..\build
pushd ..\build
DEL /F /S /Q /A ..\build\win32_handmade.ilk
cl -DHANDMADE_WIN32=1 -FAsc -Zi ..\code\win32_handmade.cpp user32.lib Gdi32.lib
popd