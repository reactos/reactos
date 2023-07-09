@ECHO OFF

REM     The purpose of the file is to build the ratings tree for Nashville
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
SET COMPONENTNAME=ratings

REM Set the location where we're building from (drive and path)

SET SOURCEDIR=%BLDROOT%\INET\OHare\ratings

REM Set location of the build tools
SET BUILDLOC=\DEV\TOOLS\C1032\BIN

REM Move to the source drive
%SOURCEDRIVE%
CD %SOURCEDIR%

cd common
tee %BLDROOT%%BUILDLOC%\NMAKE retail >build.log
tee %BLDROOT%%BUILDLOC%\NMAKE debug >>build.log

cd ..\msrating
tee %BLDROOT%%BUILDLOC%\NMAKE retail >>build.log
tee %BLDROOT%%BUILDLOC%\NMAKE debug >>build.log
cd ..

IF %BUILDENV%==BROKER GOTO DONE

:DONE

%LOCAL%
