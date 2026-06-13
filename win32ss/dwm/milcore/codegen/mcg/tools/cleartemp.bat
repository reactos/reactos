@echo off

rem Removes all subdirectories in %temp% (ignoring errors)
rem
rem This is to clean up the crud left behind when using csp.exe in debug mode
rem (i.e. when running MilCodeGen using "-debug" or "-rascaldebug".)

for /d %%a in (%temp%\*) do (
    if /i not "%%a"=="%temp%\Temporary Internet Files" rd /s /q %%a 2>nul
)
