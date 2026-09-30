@echo off
rem Build the ROM into build\main.gb (run from anywhere). "make.bat run" also starts mGBA.
setlocal enabledelayedexpansion
set GBDK=D:\Emulation\gbdk
set EMULATOR=D:\Emulation\GBA\mGBA\mGBA.exe
cd /d "%~dp0"
if not exist build mkdir build

rem Every .c file in src and assets is part of the ROM. MBC1 cartridge, 8 x 16KB banks:
rem core code in bank 0, maps in bank 1, tiles in bank 2, level-object code in bank 3,
rem music and the sound player in bank 4 (5-7 are free)
set SRC=
for %%f in (src\*.c assets\sprites\*.c assets\tiles\*.c assets\maps\*.c assets\sound\*.c) do set SRC=!SRC! %%f

"%GBDK%\bin\lcc" -Wa-l -Wl-m -Wl-j -DUSE_SFR_FOR_REG -Wl-yt1 -Wl-yo8 ^
  -Isrc -Iassets\sprites -Iassets\tiles -Iassets\maps -Iassets\sound ^
  -o build\main.gb %SRC%
if errorlevel 1 exit /b 1

rem Fail if anything overflowed its bank (e.g. code spilling out of bank 0,
rem where it could be switched away mid-run)
"%GBDK%\bin\romusage" build\main.map -q -R
if errorlevel 1 (
  echo ROM bank overflow: see "%GBDK%\bin\romusage build\main.map -a"
  exit /b 1
)

rem lcc leaves intermediate files next to the project; keep them out of the way
del /q *.lst *.o *.ihx *.cdb *.adb *.asm *.sym 2>nul

if /i "%1"=="run" start "" "%EMULATOR%" "%~dp0build\main.gb"
