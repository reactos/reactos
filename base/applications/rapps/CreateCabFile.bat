@echo off

IF "%1"=="" GOTO show_usage
IF "%2"=="" GOTO show_usage
IF "%3"=="" GOTO show_usage

SET CABMAN_CMD=%1
SET UTF16LE_CMD=%2
SET RAPPSDB_PATH=%3

mkdir "%RAPPSDB_PATH%\utf16"

echo Converting txt files to utf16
for %%f in (%RAPPSDB_PATH%\*.txt) do (
     %UTF16LE_CMD% "%RAPPSDB_PATH%\%%~nf.txt" "%RAPPSDB_PATH%\utf16\%%~nf.txt"
)

echo Building rappmgr.cab
%CABMAN_CMD% -M mszip -S "%RAPPSDB_PATH%\rappmgr.cab" "%RAPPSDB_PATH%\utf16\*.txt"

echo Building rappmgr2.cab
%CABMAN_CMD% -M mszip -S "%RAPPSDB_PATH%\rappmgr2.cab" "%RAPPSDB_PATH%\utf16\*.txt" -F icons "%RAPPSDB_PATH%\icons\*.ico"

echo Cleaning up
rmdir /s /q "%RAPPSDB_PATH%\utf16"

echo Done

goto :eof

:show_usage
echo Usage: CreateCabFile.bat path\to\cabman.exe path\to\utf16le.exe path\to\rapps-db
