rem SET CC=c:\devel\mingw\bin\g++.exe
SET CC=g++
%CC% -static -march=native -ffast-math -O2 -masm=intel main.cpp simpleton4.cpp simpleton4asm.cpp -o simpleton.exe
rem 2> log