@echo off
set WINETEST_DEBUG=0
set WINETEST_PLATFORM=reactos

move C:\ReactOS\bin\redirtest1.dll C:\ReactOS\bin\kernel32test_versioned.dll
move C:\ReactOS\bin\testdata\redirtest2.dll C:\ReactOS\bin\testdata\kernel32test_versioned.dll
if exist "C:\ReactOS\bin\AHKAppTests.cmd" (
    dbgprint "Preparing AHK Application testing suite."
    call C:\ReactOS\bin\AHKAppTests.cmd
    del C:\ReactOS\bin\AHKAppTests.cmd
)

dbgprint --process "ipconfig"
start rosautotest /r /s /n
