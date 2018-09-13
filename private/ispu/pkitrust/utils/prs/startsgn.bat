@echo off
@rem ======================================================================
@rem ======================================================================
@rem
@rem    Microsoft Windows
@rem
@rem    Copyright (c) Microsoft Corporation, 1996 - 1997
@rem
@rem    File:       startsgn.bat
@rem
@rem    Contents:   Microsoft PRS Signing Utilities
@rem
@rem    History:    20-Aug-1997 pberkman   created
@rem     
@rem ======================================================================
@rem ======================================================================

@SETLOCAL ENABLEEXTENSIONS

@set __OLDPATH=%PATH%

@set PATH=\CryptSDK\Bin;%PATH%

@set __INPUTFILE=LIST.TXT

@set __ECHOON=
@set __TEST=
@set __CHECK=
@set __SIGNFLAGS=

@if "%COMPUTERNAME%" == ""      @set COMPUTERNAME=TEST

@rem ======================================================================
@rem    check command line
@rem ======================================================================

    :CheckCmdLine

    @if "%1" == ""                  goto FindSubDirs

    @if /I "%1" == "-?"             goto HelpMe
    @if /I "%1" == "/?"             goto HelpMe

    @if /I "%1" == "-v"             @set __ECHOON=TRUE
    @if /I "%1" == "/v"             @set __ECHOON=TRUE

    @if /I "%1" == "-c"             @set __CHECK=TRUE
    @if /I "%1" == "/c"             @set __CHECK=TRUE

    @if /I "%1" == "-T"             @set __TEST=TRUE
    @if /I "%1" == "/T"             @set __TEST=TRUE

    @shift
    @goto CheckCmdLine

@rem ======================================================================
@rem    help
@rem ======================================================================
:HelpMe

    @echo Usage:  %0 [-v, -c, -t]
    @echo        parameters (optional)
    @echo            -v: verbose
    @echo            -c: run chktrust on each file
    @echo            -t: use test sign

    @goto end_startsgn

@rem ======================================================================
@rem    find subdirs
@rem ======================================================================

    :FindSubDirs

    @if     "%__ECHOON%" == "TRUE"   @echo on

    @if     "%__ECHOON%" == "TRUE"   @set __SIGNFLAGS=%__SIGNFLAGS% -v
    @if     "%__TEST%"   == "TRUE"   @set __SIGNFLAGS=%__SIGNFLAGS% -t
    @if     "%__CHECK%"  == "TRUE"   @set __SIGNFLAGS=%__SIGNFLAGS% -c

    @dir /ad /b > %COMPUTERNAME%.DIR

    FOR /F %%i IN (%COMPUTERNAME%.DIR) DO call :CallSignFiles %%i

    @goto end_startsgn


@rem ======================================================================
@rem    do the actual call
@rem ======================================================================

    :CallSignFiles

    @echo Processing: %1

    @cd %1

    @if not exist "%__INPUTFILE%"   @goto end_call

    @call ..\SIGNFILES.BAT %__SIGNFLAGS% -!


    :end_call

    @cd ..

    @goto :EOF


:end_startsgn

@set PATH=%__OLDPATH%

@set ___OLDPATH=
@set __INPUTFILE=
@set __ECHOON=
@set __TEST=
@set __CHECK=
@set __SIGNFLAGS=

@if exist %COMPUTERNAME%.DIR  del /q %COMPUTERNAME%.DIR
