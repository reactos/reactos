@ECHO OFF

if not exist "D:\AHK-Tests" (
    dbgprint "AHK Application testing suite not present, skipping."
    exit /b 0
)

xcopy /Y /H /E D:\AHK-Tests\*.* C:\ReactOS\bin
dbgprint "....AHK Application testing suite added."