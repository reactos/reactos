@rem ======================================================================
@rem ======================================================================
@rem
@rem    Microsoft Windows
@rem
@rem    Copyright (c) Microsoft Corporation, 1996 - 1997
@rem
@rem    File:       doperf.cmd
@rem
@rem    Contents:   create performance files.
@rem
@rem    History:    05-Oct-1997 pberkman    created
@rem     
@rem ======================================================================
@rem ======================================================================

@echo off

@SETLOCAL ENABLEEXTENSIONS

@set __LOG=doperf.log

@set __ICE=FALSE
@set __ICEDIR=D:\ICEREPORTS

@if "%COMPUTERNAME%" == "PBERKMAN3" @set __ICE=TRUE

@if exist %__LOG% @del %__LOG%

@if "%1" == "" goto StartTests

@if /I "%1" == "WVT_FILE"   call :DoWVT_FILE
@if /I "%1" == "WVT_CAT"    call :DoWVT_CAT
@if /I "%1" == "WVT_CERT"   call :DoWVT_CERT

@goto FinishInit

@rem ======================================================================
        :StartTests
@rem ======================================================================

        call :DoWVT_FILE
        call :DoWVT_CAT
        call :DoWVT_CERT
        call :DoHashMD5
        call :DoHashSHA1
        call :DoAddCat

        @if "%__ICE%" == "TRUE" @copy %__LOG% %__ICEDIR%

        @goto FinishInit

@rem ======================================================================
        :DoWVT_FILE
@rem ======================================================================

    @set __TEST=WVT_FILE

    perftest -twvtfile -n 1 fileset\signed\*.*
    perftest -twvtfile -n 5 fileset\signed\*.*  >> %__LOG%

    goto GenReport

@rem ======================================================================
        :DoWVT_CAT
@rem ======================================================================

    @set __TEST=WVT_CAT

    perftest -twvtcat -n 1
    perftest -twvtcat -n 5  >> %__LOG%

    @goto GenReport

@rem ======================================================================
        :DoWVT_CERT
@rem ======================================================================

    @set __TEST=WVT_CERT

    perftest -twvtcert -n 1
    perftest -twvtcert -n 5  >> %__LOG%

    @goto GenReport

@rem ======================================================================
        :DoHashMD5
@rem ======================================================================

    @set __TEST=CRYPT_HASH

    perftest -tchash -n 1 FILESET\SIGNED\*.*
    perftest -tchash -n 40 FILESET\SIGNED\*.* >> %__LOG%

    @goto GenReport

@rem ======================================================================
        :DoHashSHA1
@rem ======================================================================

    @set __TEST=CRYPT_HASHSHA1

    perftest -tchash -sha1 -n 1 FILESET\SIGNED\*.*
    perftest -tchash -sha1 -n 40 FILESET\SIGNED\*.* >> %__LOG%

    @goto GenReport

@rem ======================================================================
        :DoAddCat
@rem ======================================================================

    @set __TEST=CATADD

    perftest -tcatadd -n 1
    perftest -tcatadd -n 5 >> %__LOG%

    @goto GenReport

@rem ======================================================================
        :GenReport
@rem ======================================================================

    if "%__ICE%" == "FALSE" @goto :EOF

    @pushd %__ICEDIR%

    @if exist perftest.rpt @del perftest.rpt

    @report perftest.mea @icereport.opt
    @copy perftest.rpt %__TEST%.rpt

    @report perftest.mea -DELIMITED @icereport.opt
    @copy perftest.rpt tab_%__TEST%.rpt

    @popd

    @goto :EOF

@rem ======================================================================
        :FinishInit
@rem ======================================================================

    cd ..

    @ENDLOCAL
