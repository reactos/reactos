@ECHO OFF

REM     The purpose of the file is to build the INET\OHare tree for Nashville
REM
REM     What needs to be done :
REM             Point to server
REM             Set environment
REM             Run build script (nmake)
REM             Copy out binaries
REM     Additionally :
REM             Report errors
REM

@ECHO ON

REM Set the name of this component
SET COMPONENTNAME=INET\OHare

REM Set the location where we're building from (drive and path)

SET SOURCEDIR=%BLDROOT%\INET\OHare\url

SET FEATURE_HTML_CONTROL=1

REM Set location of the build tools
SET BUILDLOC=\DEV\TOOLS\C1032\BIN

REM Move to the source drive
%SOURCEDRIVE%
CD %SOURCEDIR%

REM This builds the W95 specific url.dll .
REM The built binary is then copied to the binaries dir

tee %BLDROOT%%BUILDLOC%\NMAKE BUILD=all >>build.log

goto skipnt

cd debug
copy url.dll %BLDROOT%\binaries\ohare\debug
copy url.sym %BLDROOT%\binaries\ohare\debug
cd ..
cd retail
copy url.dll %BLDROOT%\binaries\ohare\retail
copy url.sym %BLDROOT%\binaries\ohare\retail
cd ..

REM Only build NT version of url for self-extracting exe
if %BLDPROJ%!=="OPK2" GOTO SKIPNT

REM This builds the NT specific url.DLL.
REM The built binary is then copied to the binaries dir
SET NT_THUNKS=1

delwalk
tee %BLDROOT%%BUILDLOC%\NMAKE BUILD=all >>build.log
cd debug
copy url.dll %BLDROOT%\binaries\ohare\ntdbg
copy url.sym %BLDROOT%\binaries\ohare\ntdbg
cd ..
cd retail
copy url.dll %BLDROOT%\binaries\ohare\ntret
copy url.sym %BLDROOT%\binaries\ohare\ntret
cd ..

SET NT_THUNKS=
:SKIPNT


IF %BUILDENV%==BROKER GOTO DONE

REM CALL "%SCRIPTROOT%\INET\OHARE\DIALMON\dialmon.bat"

:DONE

SET FEATURE_HTML_CONTROL=

%LOCAL%
