@echo off

if "%1"=="/help" goto USAGE
if "%1"=="/?" goto USAGE
if "%1"=="" goto USAGE
if not exist %1 goto ERROR

tracepdb.exe -f %1 -c > NUL
for /F "tokens=* USEBACKQ" %%F in (`findstr /R "guid(\"{.*}\")," %~n1.mof`) do (
    set guidline=%%F
)

set guid=%guidline:~7,36%
echo | set /p="%guid%" | clip
echo The provider's control GUID is copied to clipboard!

del %~n1.mof > NUL
del *.tmf > NUL

echo The .tmc file content to help with the enabled flags:

type %guid%.tmc
del %guid%.tmc > NUL

goto END

:USAGE

echo getinfo.bat ^<.pdbfile^>

goto END

:ERROR

echo ERROR: file does not exist.

:END
