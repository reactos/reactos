@echo OSR DDKBUILD.BAT V6.5 - OSR, Open Systems Resources, Inc.
@echo off
rem /////////////////////////////////////////////////////////////////////////////
rem //
rem //    This sofware is supplied for instructional purposes only.
rem //
rem //    OSR Open Systems Resources, Inc. (OSR) expressly disclaims any warranty
rem //    for this software.  THIS SOFTWARE IS PROVIDED  "AS IS" WITHOUT WARRANTY
rem //    OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
rem //    THE IMPLIED WARRANTIES OF MECHANTABILITY OR FITNESS FOR A PARTICULAR
rem //    PURPOSE.  THE ENTIRE RISK ARISING FROM THE USE OF THIS SOFTWARE REMAINS
rem //    WITH YOU.  OSR's entire liability and your exclusive remedy shall not
rem //    exceed the price paid for this material.  In no event shall OSR or its
rem //    suppliers be liable for any damages whatsoever (including, without
rem //    limitation, damages for loss of business profit, business interruption,
rem //    loss of business information, or any other pecuniary loss) arising out
rem //    of the use or inability to use this software, even if OSR has been
rem //    advised of the possibility of such damages.  Because some states/
rem //    jurisdictions do not allow the exclusion or limitation of liability for
rem //    consequential or incidental damages, the above limitation may not apply
rem //    to you.
rem //
rem //    OSR Open Systems Resources, Inc.
rem //    105 Route 101A Suite 19
rem //    Amherst, NH 03031  (603) 595-6500 FAX: (603) 595-6503
rem //    email bugs to: bugs@osr.com
rem //
rem //
rem //    MODULE:
rem //
rem //        ddkbuild.bat 
rem //
rem //    ABSTRACT:
rem //
rem //      This file allows drivers to be build with visual studio and visual studio.net
rem //
rem //    AUTHOR(S):
rem //
rem //        OSR Open Systems Resources, Inc.
rem // 
rem //    REVISION:   V6.5
rem //
rem //      Clean up batch procedure to make it easier to process.
rem //
rem //
rem //    REQUIREMENTS:  Environment variables that must be set.
rem //
rem //		NT4BASE - must be set up by user to point to NT4 DDK. (e.g. D:\NT4DDK )
rem //      W2KBASE - must be set up by user to point to W2K DDK  (e.g D:\Nt50DDK )
rem //      WXPBASE - must be set up by user to point to WXP DDK  (e.g D:\WINDDK\2600)
rem //      WNETBASE - must be set up by user to point to WNET DDK (e.g D:\WINDDK\1830) 
rem //
rem //
rem //    COMMAND FORMAT:
rem //
rem //		ddkbuild -PLATFORM BUILDTYPE DIRECTORY [FLAGS] [-WDF] [-PREFAST]
rem //
rem //              PLATFORM is either 
rem //                   WXP, WXP64, WXP2K - builds using WXP DDK
rem //                   W2K, W2K64,  - builds using W2k DDK
rem //                   WNET, WNET64, WNET2K, WNETXP, WNETXP64 - builds using WNET DDK
rem //                   WNETAMD64  for an AMD64/EM64T WNET build using the WNET DDK
rem //                   NT4  - build using NT4 DDK (NT4 is the default)
rem //              BUILDTYPE - free, checked, chk or fre
rem //				DIRECTORY is the path to the directory to be build.  It can be "."
rem //              FLAGS - build flags e.g. -cZ etc.   
rem //              -WDF  - allows the user to perform a Windows Driver Framework build.
rem //                      this has been tested with the 01.00.5054 version of the 
rem //                      framework.
rem //              -PREFAST - performs a prefast build, if prefast is available.
rem //
rem //	  BROWSE FILES:
rem //	
rem //       This procedure supports the building of BROWSE files to be used by 
rem //       Visual Studio 6 and by Visual Studio.Net  However, the BSCfiles created
rem //       by bscmake for the 2 studios are not compatible. When this command procedure
rem //       runs, it selects the first bscmake.exe found in the path.   So, make
rem //       sure that the correct bscmake.exe is in the path....  
rem // 
rem //       Note that if using Visual Studio.NET the .BSC must be added to the project
rem //       in order for the project to be browsed.
rem //
rem //    COMPILERS:
rem //
rem //        If you are building NT4 you should really
rem //        be using the VC 6 compiler.   Later versions of the DDK now contain the
rem //        compiler and the linker.  This procedure should use the correct compiler.
rem //       
rem //    GENERAL COMMENTS:
rem //        This procedure has been cleaned up to be modular and easy to
rem //		  understand.
rem //
rem //		  As of the Server 2003 SP1 DDK ddkbuild now clears the
rem //        NO_BROWSE_FILE and NO_BINPLACE environment variables so that users
rem //        can use these features.
rem //
rem ///////////////////////////////////////////////////////////////////////////////

set WNETBASE=c:\winddk\3790
set scriptDebug=off
setlocal ENABLEEXTENSIONS

@echo %scriptDebug%

rem //
rem // clear the error code variable
rem //
set error_code=0
set prefast_build=0

rem //
rem // determine what type of build is to be done.
rem //
if /I %1 EQU -NT4       goto NT4Build
if /I %1 EQU -WNET2K    goto WNET2KBuild
if /I %1 EQU -WNETXP    goto WNETXPBuild
if /I %1 EQU -WNETXP64  goto WNETXPBuild
if /I %1 EQU -WNET64    goto WNET64Build
if /I %1 EQU -WNETAMD64 goto WNETAMD64Build
if /I %1 EQU -WNET      goto WNETBuild
if /I %1 EQU -WXP64     goto WXP64Build
if /I %1 EQU -WXP       goto WXPBuild
if /I %1 EQU -WXP2K     goto WXP2KBuild
if /I %1 EQU -W2K64     goto W2K64Build
if /I %1 EQU -W2K       goto W2KBuild
set error_code=1
goto ErrUnKnownBuildType

rem //
rem // NT 4 Build
rem //
:NT4Build

@echo NT4 BUILD using NT4 DDK

set BASEDIR=%NT4BASE%

shift

if "%BASEDIR%"=="" goto ErrNoBASEDIR

set path=%BASEDIR%\bin;%path%

call :SetMode %1
if "%error_code%" NEQ "0" goto ErrBadMode

call :CheckTargets %2
if "%error_code%" NEQ "0" goto ErrNoDir

pushd .
call %BASEDIR%\bin\setenv.bat %BASEDIR% %mode% "%MSDEVDIR%"
popd

@echo %scriptDebug%

goto RegularBuild

rem //
rem // WNET Windows 2000 Build using WNET DDK
rem //

:WNET2KBuild

@echo W2K BUILD using WNET DDK

set BASEDIR=%WNETBASE%

shift

if "%BASEDIR%"=="" goto ErrNoBASEDIR

set path=%BASEDIR%\bin;%path%

call :SetMode %1
if "%error_code%" NEQ "0" goto ErrBadMode

call :CheckTargets %2
if "%error_code%" NEQ "0" goto ErrNoDir

pushd .
call %BASEDIR%\bin\setenv.bat %BASEDIR% W2K %mode% 
popd

@echo %scriptDebug%

goto RegularBuild

rem //
rem // WXP Build using WNET DDK
rem //
:WNETXPBuild

@echo WXP BUILD using WNET DDK

set BASEDIR=%WNETBASE%

shift

if "%BASEDIR%"=="" goto ErrNoBASEDIR

set path=%BASEDIR%\bin;%path%

call :SetMode %1
if "%error_code%" NEQ "0" goto ErrBadMode

call :CheckTargets %2
if "%error_code%" NEQ "0" goto ErrNoDir

pushd .
call %BASEDIR%\bin\setenv.bat %BASEDIR% %mode% WXP 
popd

@echo %scriptDebug%

goto RegularBuild

rem //
rem // WXP 64 bit Build using WNET DDK
rem //
:WNETXP64Build

@echo WXP 64 BIT BUILD using WNET DDK

set BASEDIR=%WNETBASE%

shift

if "%BASEDIR%"=="" goto ErrNoBASEDIR

set path=%BASEDIR%\bin;%path%

call :SetMode %1
if "%error_code%" NEQ "0" goto ErrBadMode

call :CheckTargets %2
if "%error_code%" NEQ "0" goto ErrNoDir

pushd .
call %BASEDIR%\bin\setenv.bat %BASEDIR% %mode% 64 WXP 
popd

@echo %scriptDebug%

goto RegularBuild

rem //
rem // WNET IA64 bit Build using WNET DDK
rem //
:WNET64Build

@echo WNET 64 BIT BUILD using WNET DDK

set BASEDIR=%WNETBASE%

shift

if "%BASEDIR%"=="" goto ErrNoBASEDIR

set path=%BASEDIR%\bin;%path%

call :SetMode %1
if "%error_code%" NEQ "0" goto ErrBadMode

call :CheckTargets %2
if "%error_code%" NEQ "0" goto ErrNoDir

pushd .
call %BASEDIR%\bin\setenv.bat %BASEDIR% %mode% 64 WNET 
popd

@echo %scriptDebug%

goto RegularBuild

rem //
rem // WNET AMD64 bit Build using WNET DDK
rem //
:WNETAMD64Build

@echo WNET 64 BIT BUILD using WNET DDK

set BASEDIR=%WNETBASE%

shift

if "%BASEDIR%"=="" goto ErrNoBASEDIR

set path=%BASEDIR%\bin;%path%

call :SetMode %1
if "%error_code%" NEQ "0" goto ErrBadMode

call :CheckTargets %2
if "%error_code%" NEQ "0" goto ErrNoDir

pushd .
call %BASEDIR%\bin\setenv.bat %BASEDIR% %mode% AMD64 WNET 
popd

@echo %scriptDebug%

goto RegularBuild

rem //
rem // WNET 32 BIT BUILD using WNET DDK
rem //
:WNETBuild

@echo WNET 32 BIT BUILD using WNET DDK

set BASEDIR=%WNETBASE%

shift

if "%BASEDIR%"=="" goto ErrNoBASEDIR

set path=%BASEDIR%\bin;%path%

call :SetMode %1
if "%error_code%" NEQ "0" goto ErrBadMode

call :CheckTargets %2
if "%error_code%" NEQ "0" goto ErrNoDir

pushd .
call %BASEDIR%\bin\setenv.bat %BASEDIR% %mode%
popd

@echo %scriptDebug%

goto RegularBuild

rem //
rem // WXP 64 BIT BUILD using WXP DDK
rem //
:WXP64Build

@echo WXP 64 BIT BUILD using WXP DDK

set BASEDIR=%WXPBASE%

shift

if "%BASEDIR%"=="" goto ErrNoBASEDIR

set path=%BASEDIR%\bin;%path%

call :SetMode %1
if "%error_code%" NEQ "0" goto ErrBadMode

call :CheckTargets %2
if "%error_code%" NEQ "0" goto ErrNoDir

pushd .
call %BASEDIR%\bin\setenv.bat %BASEDIR% %mode% 64
popd

@echo %scriptDebug%

goto RegularBuild

rem //
rem // WXP 32 BIT BUILD using WXP DDK
rem //
:WXPBuild

@echo WXP 32 BIT BUILD using WXP DDK

set BASEDIR=%WXPBASE%

shift

if "%BASEDIR%"=="" goto ErrNoBASEDIR

set path=%BASEDIR%\bin;%path%

call :SetMode %1
if "%error_code%" NEQ "0" goto ErrBadMode

call :CheckTargets %2
if "%error_code%" NEQ "0" goto ErrNoDir

pushd .
call %BASEDIR%\bin\setenv.bat %BASEDIR% %mode% 
popd

@echo %scriptDebug%

goto RegularBuild

rem //
rem // W2K 32 BIT BUILD using WXP DDK
rem //
:WXP2KBuild

@echo W2K 32 BIT BUILD using WXP DDK

set BASEDIR=%WXPBASE%

shift

if "%BASEDIR%"=="" goto ErrNoBASEDIR

set path=%BASEDIR%\bin;%path%

call :SetMode %1
if "%error_code%" NEQ "0" goto ErrBadMode

call :CheckTargets %2
if "%error_code%" NEQ "0" goto ErrNoDir

pushd .
call %BASEDIR%\bin\w2k\set2k.bat %BASEDIR% %mode% 
popd

@echo %scriptDebug%

goto RegularBuild

rem //
rem // W2K 64 BIT BUILD using W2K DDK
rem //
:W2K64Build

@echo W2K 64 BIT BUILD using W2K DDK

set BASEDIR=%W2KBASE%

shift

if "%BASEDIR%"=="" goto ErrNoBASEDIR

set path=%BASEDIR%\bin;%path%

call :SetMode %1
if "%error_code%" NEQ "0" goto ErrBadMode

call :CheckTargets %2
if "%error_code%" NEQ "0" goto ErrNoDir

pushd .
call %BASEDIR%\bin\setenv64.bat %BASEDIR% %mode% 
popd

@echo %scriptDebug%

goto RegularBuild

rem //
rem // W2K 32 BIT BUILD using W2K DDK
rem //
:W2KBuild

@echo W2K 32 BIT BUILD using W2K DDK

set BASEDIR=%W2KBASE%

shift

if "%BASEDIR%"=="" goto ErrNoBASEDIR

set path=%BASEDIR%\bin;%path%

call :SetMode %1
if "%error_code%" NEQ "0" goto ErrBadMode

call :CheckTargets %2
if "%error_code%" NEQ "0" goto ErrNoDir

pushd .
call %BASEDIR%\bin\setenv.bat %BASEDIR% %mode% 
popd

@echo %scriptDebug%

goto RegularBuild

rem //
rem // All builds go here for the rest of the procedure.  Now,
rem // we are getting ready to call build.  The big problem
rem // here is to figure our the name of the buildxxx files being
rem // generated for the different platforms.
rem //
 
:RegularBuild

set NO_BROWSWER_FILE=
set NO_BINPLACE=

set mpFlag=-M
if "%BUILD_ALT_DIR%"=="" goto NT4

rem win2k sets this!
set W2kEXT=%BUILD_ALT_DIR%

set mpFlag=-MI

:NT4

if "%NUMBER_OF_PROCESSORS%"=="" set mpFlag=
if "%NUMBER_OF_PROCESSORS%"=="1" set mpFlag=

rem //
rem // Determine the settings of flags, WDF and PREFAST in other words
rem // what was set for %3 and beyond....
rem // 
@echo build in directory %2 with arguments %3 %4 %5 (basedir %BASEDIR%)

cd /D %~s2
set bflags=-Ze
set bscFlags=""

set buildDirectory=%2

:ContinueParsing
if "%3" == "" goto done
if "%3" == "/a" goto RebuildallFound
if "%3" == "-WDF" goto WDFFound
if "%3" == "-wdf" goto WDFFound
if "%3" == "-PREFAST" goto PrefastFound
if "%3" == "-prefast" goto PrefastFound
set bscFlags=/n
set bflags=%3 -e
shift
goto ContinueParsing

:WDFFound
shift
if "%WDF_ROOT%" == "" goto errNoWdfRoot
pushd .
call %WDF_ROOT%\set_wdf_env.cmd 
popd
set scriptDebug=on
goto ContinueParsing

:PrefastFound
shift
set prefast_build=1
goto ContinueParsing

:RebuildallFound
shift
set bscFlags=/n
set bflags=-cfeZ
goto ContinueParsing

:done

if EXIST build%W2kEXT%.err	 erase build%W2kEXT%.err
if EXIST build%W2kEXT%.wrn   erase build%W2kEXT%.wrn
if EXIST build%W2kEXT%.log	 erase build%W2kEXT%.log
if EXIST prefast%W2kEXT%.log erase prefast%W2kEXT%.log

if "%prefast_build%" NEQ "0" goto RunPrefastBuild
@echo run build %bflags% %mpFlag% for %mode% version in %buildDirectory%
pushd .
build  %bflags% %mpFlag%
goto BuildComplete

:RunPrefastBuild
@echo run prefast build %bflags% %mpFlag% for %mode% version in %buildDirectory%
pushd .
prefast build  %bflags% %mpFlag%
prefast list > prefast%W2kEXT%.log
goto BuildComplete

:BuildComplete
if "%errorlevel%" GTR "0" set error_code=%errorlevel%
popd

@echo %scriptDebug%

rem assume that the onscreen errors are complete!

@echo =============== build warnings ======================
if exist build%W2kEXT%.wrn findstr "warning[^.][DRCLU][0-9]*" build%W2kEXT%.log
if exist build%W2kEXT%.log findstr "error[^.][DRCLU][0-9]*" build%W2kEXT%.log

if "%prefast_build%" == "0" goto SkipPrefastWarnings
@echo =============== prefast warnings ======================
if exist prefast%W2kEXT%.log findstr "warning[^.][CLU]*" prefast%W2kEXT%.log
:SkipPrefastWarnings

@echo. 
@echo. 
@echo build complete

@echo building browse information files

if EXIST buildbrowse.cmd goto doBrowsescript

set sbrlist=sbrList.txt

if not EXIST sbrList%CPU%.txt goto sbrDefault

set sbrlist=sbrList%CPU%.txt

:sbrDefault

if not EXIST %sbrlist% goto end

if %bscFlags% == "" goto noBscFlags

bscmake %bscFlags% @%sbrlist%

goto end

:noBscFlags

bscmake @%sbrlist%

goto end

:doBrowsescript

call buildBrowse %mode%

goto end

rem //
rem //  SetMode
rem //
rem //  Subroutine to validate the mode of the build passed in.
rem //  it must be free, FREE, fre, FRE or checked, CHECKED,
rem //  chk, CHK.   Anything else is an error.
rem //
:SetMode
set mode=
for %%f in (free FREE fre FRE) do if %%f == %1 set mode=free
for %%f in (checked CHECKED chk CHK) do if %%f == %1 set mode=checked
if "%mode%" =="" set error_code=1
goto :EOF

rem //
rem // CheckTargets
rem //
rem // Subroutine to validate that the target directory exists
rem // and that there is either a DIRS or SOURCES and MakeFile in
rem // it.
rem // 
:CheckTargets
if "%1" NEQ "" goto CheckTargets1
set error_code=1
goto :EOF
:CheckTargets1
if exist %1 goto CheckTargets2
set error_code=1
goto :EOF
:CheckTargets2
if not exist %1\DIRS  goto CheckTargets3
set error_code=0
goto :EOF
:CheckTargets3
if exist %1\SOURCES  goto CheckTargets4
set error_code=2
goto :EOF
:CheckTargets4
if exist %1\MAKEFILE  goto CheckTargets5
set error_code=2
goto :EOF
:CheckTargets5
set error_code=0
goto :EOF

rem //
rem //  Error processing code.   Whenever we encounter an
rem //  error in the parameters, we come to one of the following
rem //  labels to output a decent error to help the user
rem //  understand what is wrong.
rem //

:ErrBadMode
@echo -
@echo ERROR: first param must be "checked", "free", "chk" or "fre"
set error_code=1
goto usage

:ErrNoBASEDIR
@echo -
@echo ERROR: NT4BASE, W2KBASE, WXPBASE, or WNETBASE environment variable not set.
@echo ERROR: Environment variable must be set by user according to DDK version installed.
set error_code=1
goto usage

:ErrUnKnownBuildType
@echo -
@echo ERROR: Unknown type of build.  Please recheck parameters.
set error_code=1
goto usage

:ErrNoDir
if "%error_code%" EQU "2" goto ErrNoTarget
@echo -
@echo ERROR: second parameter must be a valid directory
goto usage

:ErrNoTarget
@echo -
@echo ERROR: target directory must contain a SOURCES or DIRS file
goto usage

:errNoWdfRoot
@echo -
@echo ERROR: WDF_ROOT is not defined, are you using 00.01.5054 or later?
goto usage

rem //
rem // Usage output
rem //
:usage
@echo -
@echo -
@echo usage: ddkbuild [-W2K] "checked | free | chk | fre" "directory-to-build" [flags] [-WDF] [-PREFAST]
@echo        -W2K       indicates development system uses W2KBASE environment variable
@echo                   to locate the win2000 ddk
@echo        -W2K64     indicates development sytsem uses W2KBASE environment variable
@echo                   to locate the win2000 IA64 ddk
@echo        -WXP       to indicate WXP Build uses WXPBASE enviornment variable.
@echo        -WXP64     to indicate WXP IA64 bit build, uses WXPBASE
@echo        -WXP2K     to indicate Windows 2000 build using WXP ddk
@echo        -WNET      to indicate Windows .Net builds using WNET ddk
@echo        -WNET64    to indicate Windows .Net 64 bit builds using WNET DDK
@echo        -WNETXP    to indicate Windows XP builds suing WNET DDK
@echo        -WNETXP64  to indicate Windows XP 64 bit builds suing WNET DDK
@echo        -WNETAMD64 to indicate Windows .NET build for AMD64 using WNET DDK
@echo        -WNET2K    to indicate Windows 2000 builds using WNET DDK
@echo        -NT4       to indicate NT4 build using NT4 DDK.
@echo         checked   indicates a checked build
@echo         free      indicates a free build
@echo         chk		indicates a checked build
@echo         fre		indicates a free build
@echo         directory path to build directory, try . (cwd)
@echo         flags     any random flags you think should be passed to build (try /a for clean)
@echo         -WDF      performs a WDF build
@echo         -PREFAST  preforms a PREFAST build
@echo -
@echo         ex: ddkbuild -NT4 checked . (for NT4 BUILD)
@echo         ex: ddkbuild -WXP64 chk .
@echo         ex: ddkbuild -WXP chk c:\projects\myproject
@echo         ex: ddkbuild -WNET64 chk .      (IA64 bit build)
@echo         ex: ddkbuild -WNETAMD64 chk .   (AMD64/EM64T bit build)
@echo         ex: ddkbuild -WNETXP chk . -cZ -WDF
@echo         ex: ddkbuild -WNETXP chk . -cZ -PREFAST
@echo -
@echo         In order for this procedure to work correctly for each platform, it requires
@echo         an environment variable to be set up for certain platforms.   The environment
@echo         variables are as follows:
@echo -
@echo         NT4BASE - You must set this up to do -NT4 builds
@echo         W2KBASE - You must set this up to do -W2K and -W2K64 builds
@echo         WXPBASE - You must set this up to do -WXP, -WXP64, -WXP2K builds
@echo         WNETBASE - You must set this up to do -WNET, -WNET64, -WNETXP, -WNETXP64, 
@echo                    -WNETAMD64, and -WNET2K builds
@echo -
@echo         WDF_ROOT must be set if attempting to do a WDF Build.
@echo -
@echo -
@echo   OSR DDKBUILD.BAT V6.5 - OSR, Open Systems Resources, Inc.
@echo     report any problems found to info@osr.com
@echo  - 

rem goto end

:end

exit /b %error_code%

@echo ddkbuild complete
