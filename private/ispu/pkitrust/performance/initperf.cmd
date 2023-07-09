@rem ======================================================================
@rem ======================================================================
@rem
@rem    Microsoft Windows
@rem
@rem    Copyright (c) Microsoft Corporation, 1996 - 1997
@rem
@rem    File:       initperf.cmd
@rem
@rem    Contents:   create performance files.
@rem
@rem    History:    05-Oct-1997 pberkman    created
@rem     
@rem ======================================================================
@rem ======================================================================

@echo off

@SETLOCAL ENABLEEXTENSIONS

    @set    __LOG=..\initperf.log

    @cd fileset

    @if "%COMPUTERNAME%" == "" @set COMPUTERNAME=perf

    @if exist tcatnt.fil    @del tcatnt.fil
    @if exist tcatnt.cat    @del tcatnt.cat
    @if exist tcatnt.cdf    @del tcatnt.cdf
    @if exist tsystem.dir   @del tsystem.dir

    @if exist %SystemRoot%\System32\CatRoot rmdir /s /q %SystemRoot%\System32\CatRoot

    @goto AddCDFHeader

@rem ======================================================================
        :AddCDFHeader
@rem ======================================================================
    echo [CatalogHeader]> tcatnt.cdf
    echo Name=tcatnt.cat>> tcatnt.cdf
    echo CATATTR1=0x10010001:OSAttr:1:4.x,2:4.x,2:5.x>> tcatnt.cdf
    echo [CatalogFiles]>> tcatnt.cdf

    @goto AddFiles2CDF

@rem ======================================================================
        :AddFiles2CDF
@rem ======================================================================
    @dir /b /A-D-R /L /Oen %SystemRoot%\System32\*.* > tsystem.dir

    FOR /F "delims=;" %%i IN (tsystem.dir) DO call :Add2CDFFile "%%i"

    @stripqts tcatnt.cdf

    @goto CreateCAT

@rem ======================================================================
        :CreateCAT
@rem ======================================================================
    @makecat -v tcatnt.cdf           >> %__LOG%

    @goto SignCAT

@rem ======================================================================
        :SignCAT
@rem ======================================================================
    @signcode -v driver.pvk -spc driver.spc -n "Performance" -i "http://pberkman2" -t "http://timestamp.verisign.com/scripts/timstamp.dll" -tr 10 -tw 2 tcatnt.cat >> %__LOG%

    @goto FinishInit

@rem ======================================================================
        :Add2CDFFile
@rem ======================================================================
    @rem
    @rem     %1: drive, path, and file
    @rem     %2: drive and path
    @rem     %3: file and ext only
    @rem     %4: file only
    @rem
    @echo processing: 1=%1 >> %__LOG%

    @if not exist "%SystemRoot%\System32\%1" @goto :Error_Loop

    echo %1=%SystemRoot%\System32\%1>>tcatnt.cdf

    @goto :EOF

@rem ======================================================================
        :Error_Loop
@rem ======================================================================
    echo unable to process %1 >> %__LOG%
    @goto :EOF


@rem ======================================================================
        :FinishInit
@rem ======================================================================

    cd ..

    @ENDLOCAL
