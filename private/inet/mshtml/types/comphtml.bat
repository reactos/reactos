@echo off
rem Need the next line to clear build.exe status
echo Build_Status Checking for SDK Updates

diff %1 %2 >nul
if errorlevel 1 goto different

rem
rem  This check will not work on Win95 because DIR always returns an errorlevel
rem  of zero. Persons building on Win95 will not get the "file in read/write..."
rem  message.
rem
dir /a:r %2 >nul 2>&1
if errorlevel 1 goto readwrite
goto out

:readwrite
echo comphtml.bat(1) : warning W1001: %2% is read/write. Remember to check it in.
goto out

:different
copy %1 %2 >nul
if errorlevel 1 goto cantcopy
echo comphtml.bat(1) : warning W1000: Updated %2% - Please check it in
goto out

:cantcopy
echo comphtml.bat(1) : error E9999: Need to update %2% - Please check it out

:out

