@ECHO OFF

REM     The purpose of the file is to build the CCSHELL tree
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
SET COMPONENTNAME=CCSHELL

REM Set the location where we're building from (drive and path)

SET SOURCEDIR=%BLDROOT%\WIN\SHELL\CCSHELL
       
REM Set location of the build tools
SET BUILDLOC=\DEV\TOOLS\COMMON

REM Move to the source drive
%SOURCEDRIVE%
CD %SOURCEDIR%       


REM This builds the W95 specific SHDOCVW.DLL.
REM The built binary is then copied to the binaries dir

cd shdocvw
delwalk
SET NT_THUNKS=
tee %BLDROOT%%BUILDLOC%\NMAKE debug >>build.log
cd nashvile\debug
copy shdocvw.dll %BLDROOT%\binaries\ohare\debug
copy shdocvw.sym %BLDROOT%\binaries\ohare\debug
cd..\..
tee %BLDROOT%%BUILDLOC%\NMAKE retail >>build.log
cd nashvile\retail
copy shdocvw.dll %BLDROOT%\binaries\ohare\retail
copy shdocvw.sym %BLDROOT%\binaries\ohare\retail
cd..\..

REM Only build NT version of SHDOCVW for self-extracting exe
if %BLDPROJ%!=="OPK2" GOTO SKIPNT

REM This builds the NT specific SHDOCVW.DLL.
REM The built binary is then copied to the binaries dir
SET NT_THUNKS=1
delwalk
tee %BLDROOT%%BUILDLOC%\NMAKE debug >>build.log
cd nashvile\debug
copy shdocvw.dll %BLDROOT%\binaries\ohare\ntdbg
copy shdocvw.sym %BLDROOT%\binaries\ohare\ntdbg
cd..\..
tee %BLDROOT%%BUILDLOC%\NMAKE retail >>build.log
cd nashvile\retail
copy shdocvw.dll %BLDROOT%\binaries\ohare\ntret
copy shdocvw.sym %BLDROOT%\binaries\ohare\ntret
cd..\..

SET NT_THUNKS=
:SKIPNT

%LOCAL%
