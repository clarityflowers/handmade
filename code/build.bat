@echo off

REM TODO - can we just build both with one exe?

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
IF EXIST win32_handmade.ilk DEL /F /S /Q /A win32_handmade.ilk
cl -MT -nologo -Gm- -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FAsc -Zi -Fmwin32_handmade.map ..\code\win32_handmade.cpp /link -opt:ref -subsystem:windows,5.1  user32.lib Gdi32.lib
popd