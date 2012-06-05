@echo off
set WINETEST_DEBUG=0

if exist "C:\ReactOS\bin\AHKAppTests.cmd" (
    dbgprint "Preparing AHK Application testing suite."
    call C:\ReactOS\bin\AHKAppTests.cmd
    del C:\ReactOS\bin\AHKAppTests.cmd
)

dbgprint --process "ipconfig"
start rosautotest /r /s
