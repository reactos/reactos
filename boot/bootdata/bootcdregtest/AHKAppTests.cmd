@ECHO OFF

if not exist "D:\AHK-Tests" (
    dbgprint "AHK Application testing suite not present, skipping."
    exit /b 0
)

xcopy /Y /H /E D:\AHK-Tests\*.* C:\ReactOS\bin
REM Download Amine's rosautotest from svn
dwnl http://svn.reactos.org/amine/rosautotest.exe C:\ReactOS\system32\rosautotest.exe
dbgprint "....AHK Application testing suite added."
