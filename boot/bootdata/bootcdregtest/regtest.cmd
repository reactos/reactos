@echo off
set WINETEST_DEBUG=0
set WINETEST_PLATFORM=reactos

:: On the first boot, we're started through RunOnce.
:: Add us to the Run key, so we're also started on the next reboot in case ReactOS crashed *and* the registry has been saved.
:: Exit right after that, because Explorer processes the Run key right after RunOnce and therefore picks up regtest.cmd a second time during the first boot.
reg query HKLM\Software\Microsoft\Windows\CurrentVersion\Run /v regtest
if "%errorlevel%"=="1" (
    reg add HKLM\Software\Microsoft\Windows\CurrentVersion\Run /v regtest /t REG_SZ /d "%SystemRoot%\system32\cmd.exe /c regtest.cmd"
    exit 0
)

move C:\ReactOS\bin\redirtest1.dll C:\ReactOS\bin\kernel32test_versioned.dll
move C:\ReactOS\bin\testdata\redirtest2.dll C:\ReactOS\bin\testdata\kernel32test_versioned.dll
if exist "C:\ReactOS\bin\AHKAppTests.cmd" (
    dbgprint "Preparing AHK Application testing suite."
    call C:\ReactOS\bin\AHKAppTests.cmd
    del C:\ReactOS\bin\AHKAppTests.cmd
)

dbgprint --process "ipconfig"
cd C:\ReactOS\bin
start rosautotest /r /s /n
