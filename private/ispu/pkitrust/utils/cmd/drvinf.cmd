@echo off
@rem ======================================================================
@rem ======================================================================
@rem
@rem    Microsoft Windows
@rem
@rem    Copyright (c) Microsoft Corporation, 1996 - 1997
@rem
@rem    File:       drvinf.cmd
@rem
@rem    Contents:   Microsoft Win98 INF file Catalog generator
@rem
@rem    History:    01-Oct-1997 pberkman    created
@rem     
@rem ======================================================================
@rem ======================================================================

@SETLOCAL ENABLEEXTENSIONS

@set __VSTRING="1:4.10"
@set __CLSCHK="Class"

@if "%COMPUTERNAME%" == "" @set COMPUTERNAME=drvinf

@set __MODFLAG=
@set __USESLM=
@set __INFBASEDIR=
@set __VERBOSEFLG=

:CheckCmdLine

    @if "%1" == ""                  @goto Initialize

    @if /I "%1" == "-?"             @goto HelpMe
    @if /I "%1" == "/?"             @goto HelpMe

    @if /I "%1" == "-v"             @goto SetEcho
    @if /I "%1" == "/v"             @goto SetEcho

    @if /I "%1" == "-M"             @goto SetModFlag
    @if /I "%1" == "/M"             @goto SetModFlag

    @if /I "%1" == "/S"             @goto SetSlmFlag
    @if /I "%1" == "-S"             @goto SetSlmFlag
    
    @set __INFBASEDIR=%1\

:ShiftCmdLine
    @shift
    @goto CheckCmdLine

:SetModFlag
    @set __MODFLAG=-M
    @goto ShiftCmdLine

:SetEcho
    @echo on
    @set __VERBOSEFLG=-V
    @goto ShiftCmdLine

:SetSlmFlag
    @set __USESLM=TRUE
    @goto ShiftCmdLine

:HelpMe
    @echo usage: %0 [params] [INF start directory]
    @echo      params:
    @echo         -V: verbose
    @echo         -M: modify INF files
    @echo         -S: check out/in inf/inx files using SLM
    @goto :EOF

:Initialize

    @dir /b /A-D /L /Oen /S %__INFBASEDIR%*.INF %__INFBASEDIR%*.INX > %COMPUTERNAME%.DIR

    @FOR /F "delims=;;;" %%i IN (%COMPUTERNAME%.DIR) DO @call :ModInfAndCat "%%i" "%%~dpi" "%%~nxi" "%%~ni"

    @goto EndDrvInf


:EndDrvInf
    @del %COMPUTERNAME%.DIR
    
    @ENDLOCAL

    @goto :EOF

:ModInfAndCat
    @rem
    @rem     %1: drive, path, and file
    @rem     %2: drive and path
    @rem     %3: file and ext only
    @rem     %4: file only
    @rem
    @rem @echo %1 %2 %3 %4

        @rem
        @rem     check out the inf/inx just in case we need to add the CatalogFile= ref
        @rem
    @if "%__USESLM%" == "" @goto NoOut
        @pushd %2
        @out -f! %3
        @popd
    :NoOut

        @rem
        @rem     create a cdf from the inf/inx and add the CatalogFile= ref if necessary
        @rem
    @inf2cdf %__MODFLAG% %__VERBOSEFLG% -ICS %__CLSCHK% -S %__VSTRING% %1
    @if ERRORLEVEL 1 @goto BailMakeCat

        @rem
        @rem     check back in the inf/inx.
        @rem
    @if "%__USESLM%" == "" @goto NoIn
        @pushd %2
        @in -f! -c"inf2cdf" %3
        @popd
    :NoIn

        @rem
        @rem     create the catalog file
        @rem
    @makecat %__VERBOSEFLG% %4.CDF

    @if NOT %USERNAME% == "pberkman" @goto NoSigning
    @if exist driver.pvk signcode -v driver.pvk -spc driver.spc -n "Win98 Driver Testing" -i "http://pberkman2/ISPU" -t "http://timestamp.verisign.com/scripts/timstamp.dll" -tr 10 -tw 2 %4.CAT

    :NoSigning

    @goto :EOF

:BailMakeCat

    @if "%__USESLM%" == "" @goto NoIn2
        @pushd %2
        @in -f! -c"inf2cdf" %3
        @popd
    :NoIn2
    
    @goto :EOF

