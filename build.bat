call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64

REM cl %1

REM /subsystem:windows,5.1 (32-bit), -EHa-, -MTd
set FLAGS=-Oi -EHsc -WX -W4 -wd4201 -wd4189 -wd4100 -wd4127 -wd4189 -wd4505 -wd4996 -FC -Z7 -nologo
set LINKER_FLAGS=/opt:ref /subsystem:windows
set LIBS=gdi32.lib gdiplus.lib kernel32.lib shell32.lib user32.lib

IF NOT EXIST build mkdir build
pushd build
rc /nologo /fo "resource.res" ..\src\resource.rc
cl -DIVY_DEBUG=1 %FLAGS% ..\src\ivy.cpp /link %LINKER_FLAGS% resource.res %LIBS%
popd

REM -EHa- 
