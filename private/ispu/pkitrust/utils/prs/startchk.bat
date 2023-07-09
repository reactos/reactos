@echo off
@rem ======================================================================
@rem ======================================================================
@rem
@rem    Microsoft Windows
@rem
@rem    Copyright (c) Microsoft Corporation, 1996 - 1997
@rem
@rem    File:       startchk.bat
@rem
@rem    Contents:   Microsoft PRS Signing Utilities
@rem
@rem    History:    20-Aug-1997 pberkman   created
@rem     
@rem ======================================================================
@rem ======================================================================

@SETLOCAL ENABLEEXTENSIONS

@set PATH=\CryptSDK\Bin;%PATH%

@set __ECHOON=
@set __INPUTFILE=list.txt

@if "%COMPUTERNAME%" == ""      @set COMPUTERNAME=TEST

@rem ======================================================================
@rem    check command line
@rem ======================================================================

    :CheckCmdLine

    @if "%1" == ""                  goto FindSubDirs

    @if /I "%1" == "-v"             @set __ECHOON=TRUE
    @if /I "%1" == "/v"             @set __ECHOON=TRUE

    @shift
    @goto CheckCmdLine


@rem ======================================================================
@rem    find subdirs
@rem ======================================================================

    :FindSubDirs

    @if "%__ECHOON%" == "TRUE"      @echo on

    @dir /ad /b > "%COMPUTERNAME%.DIR"

    FOR /F %%i IN (%COMPUTERNAME%.DIR) DO call :DoCheckFiles %%i

    @goto end_startchk


@rem ======================================================================
@rem    do the actual call
@rem ======================================================================

    :DoCheckFiles

    @cd %1

    @if not exist "%__INPUTFILE%"   @goto done_for

    prsparse "%__INPUTFILE%" "%COMPUTERNAME%.TXT"

    FOR /F "eol=; tokens=1* delims=," %%j IN (%COMPUTERNAME%.TXT) DO call :CheckFile %%j %%k
    
    :done_for

    @cd .. 

    @goto end_startchk


@rem ======================================================================
@rem    check each file
@rem ======================================================================

    :CheckFile

    @chktrust %1

    @goto :EOF


@rem ======================================================================
@rem    errors
@rem ======================================================================

    :ErrNoFileToCheck
    @echo ERROR:
    @echo       file to check not found ("%1").  FATAL!
    @goto :EOF

    :ErrNoListFile
    @echo ERROR:
    @echo        no %__INPUTFILE% file found.  FATAL!
    goto :EOF


:end_startchk

    @ENDLOCAL