@echo off
@rem ======================================================================
@rem ======================================================================
@rem
@rem    Microsoft Windows
@rem
@rem    Copyright (c) Microsoft Corporation, 1996 - 1997
@rem
@rem    File:       starttst.bat
@rem
@rem    Contents:   Microsoft Win98 INF file Catalog regression tests
@rem
@rem    History:    05-Oct-1997 pberkman    created
@rem     
@rem ======================================================================
@rem ======================================================================

@SETLOCAL ENABLEEXTENSIONS

    @set    __DELIMCH=*
    @set    __LOG=CATDB.LOG

    @cd test

    @if "%COMPUTERNAME%" == "" @set COMPUTERNAME=tcatdb

    @if exist loop1.cat     @goto BeginTest

    @if exist loop1.fil     @del loop1.fil
    @if exist loop2.fil     @del loop2.fil
    @if exist loop3.fil     @del loop3.fil

    rem
    rem     add catalog headers to CDF
    rem
    echo [CatalogHeader]> loop1.cdf
    echo Name=loop1.cat>> loop1.cdf
    echo CATATTR1=0x10010001:OSAttr:1:4.x,2:4.x,2:5.x>> loop1.cdf
    echo [CatalogFiles]>> loop1.cdf

rem     echo [CatalogHeader]> loop2.cdf
rem     echo Name=loop2.cat>> loop2.cdf
rem     echo CATATTR1=0x10010001:OSAttr:1:4.x,2:4.x,2:5.x>> loop2.cdf
rem     echo [CatalogFiles]>> loop2.cdf
rem 
rem     echo [CatalogHeader]> loop3.cdf
rem     echo Name=loop3.cat>> loop3.cdf
rem     echo CATATTR1=0x10010001:OSAttr:1:4.x,2:4.x,2:5.x>> loop3.cdf
rem     echo [CatalogFiles]>> loop3.cdf

    @dir /b /A-D-R /L /Oen /S %SystemRoot%\System32\Drivers\*.* %SystemRoot%\Inf\*.* > SYSTEM.DIR

    @set __CMDLINE="%%i" "%%~dpi" "%%~nxi" "%%~ni"

    @set __LOOPFILE=loop1
    FOR /F "delims=;" %%i IN (SYSTEM.DIR)            DO call :CreateLoopFile %__CMDLINE%

rem     @set __LOOPFILE=loop2
rem     FOR /F "skip=200 delims=;;;" %%i IN (SYSTEM.DIR)   DO call :CreateLoopFile %__CMDLINE%

rem     @set __LOOPFILE=loop3
rem     FOR /F "skip=500 delims=;;;" %%i IN (SYSTEM.DIR)   DO call :CreateLoopFile %__CMDLINE%

    @stripqts loop1.cdf
rem     @stripqts loop2.cdf
rem     @stripqts loop3.cdf

    rem
    rem     create catalog files
    rem
    echo makecat >> %__LOG%
    @makecat -v loop1.cdf           >> %__LOG%
rem     @makecat -v loop2.cdf
rem     @makecat -v loop3.cdf

    echo signcode >> %__LOG%
    @signcode -v driver.pvk -spc driver.spc -n "Driver Test 1" -i "http://pberkman2/ISPU" -t "http://timestamp.verisign.com/scripts/timstamp.dll" -tr 10 -tw 2 loop1.cat >> %__LOG%
rem     @signcode -v driver.pvk -spc driver.spc -n "Driver Test 2" -i "http://pberkman2/ISPU" -t "http://timestamp.verisign.com/scripts/timstamp.dll" -tr 10 -tw 2 loop2.cat
rem     @signcode -v driver.pvk -spc driver.spc -n "Driver Test 3" -i "http://pberkman2/ISPU" -t "http://timestamp.verisign.com/scripts/timstamp.dll" -tr 10 -tw 2 loop3.cat

    rem
    rem     begin tests
    rem

:BeginTest

    echo tcatdb -a >> %__LOG%
    @tcatdb -A loop1.cat loop1.fil >> %__LOG%
rem     @tcatdb -V -A loop2.cat loop2.fil
rem     @tcatdb -V -A loop3.cat loop3.fil

    echo tcatdb >> %__LOG%
    @tcatdb loop1.fil >> %__LOG%
rem     @tcatdb -V loop2.fil
rem     @tcatdb -V loop3.fil

    @goto EndTest

:CreateLoopFile
    rem
    rem     %1: drive, path, and file
    rem     %2: drive and path
    rem     %3: file and ext only
    rem     %4: file only
    rem
    echo processing: 1=%1 2=%2 3=%3 4=%4

    if not exist "%1" goto :Error_Loop

    echo %3%__DELIMCH%%1%__DELIMCH%0x00000000>>%__LOOPFILE%.fil
    echo %3=%1>>%__LOOPFILE%.cdf

    @goto :EOF

:Error_Loop
    echo unable to process %1 >> %__LOG%
    @goto :EOF


:EndTest

    @ENDLOCAL
