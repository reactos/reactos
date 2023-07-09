@REM =================================================================
@REM ==
@REM ==     regress.bat -- ISPU regression tests
@REM ==
@REM ==     called from ISPU/REGRESS.BAT
@REM == 
@REM ==     parameters if called from main regress:
@REM ==                 1 = -!
@REM ==                 2 = ispu base directory (eg: \nt\private\ispunt)
@REM ==                 3 = parameters passed in to original regress.bat
@REM ==                 ...
@REM ==
@REM =================================================================

    @echo off

    @SETLOCAL ENABLEEXTENSIONS

    @set THISDIR=pkitrust\tests
    @set BASEDIR=%_NTDRIVE%%_NTROOT%\private\ispu
    @set LOGFILE=%BASEDIR%\regress.out
    
    @set __CalledFromMain=FALSE
    @set _CDB_=
    @set DEBUG_MASK=
    @set DEBUG_PRINT_MASK=

@REM =====================================================
        :PrsCmdLine
@REM =====================================================
    @if /i "%1" == "-!" @goto Param_!
    @if /i "%1" == "-?" @goto NeedHelp
    @if /i "%1" == "-d" @goto Param_d
    @if /i "%1" == "-v" @goto Param_v
    @if /i "%1" == "-l" @goto Param_l
    @if /i "%1" == ""   @goto PrsCmdDone

    @shift

    @goto PrsCmdLine

@REM =====================================================
        :NeedHelp
@REM =====================================================
    @echo Usage: regress [switches]
    @echo    -d      enable all debug_print_masks
    @echo    -v      verbose (don't suppress echo)
    @echo    -l      check for memory leaks (default=no)
    @goto ExitRegress

@REM =====================================================
        :Param_!
@REM =====================================================
    @shift
    @if "%1" == "" @goto NeedHelp
    @set __CalledFromMain=TRUE
    @set BASEDIR=%1
    @set LOGFILE=%BASEDIR%\regress.out
    @shift
    @goto PrsCmdLine

@REM =====================================================
        :Param_v
@REM =====================================================
    @echo on
    @shift
    @goto PrsCmdLine

@REM =====================================================
        :Param_d
@REM =====================================================
    @set DEBUG_PRINT_MASK=0xFFFFFFFF
    @shift
    @goto PrsCmdLine

@REM =====================================================
        :Param_l
@REM =====================================================
    @set _CDB_=cdb -g -G
    @set DEBUG_MASK=0x20
    @shift
    @goto PrsCmdLine

@REM =====================================================
        :PrsCmdDone
@REM =====================================================

    @if NOT "%__CalledFromMain%" == "TRUE" @if exist %LOGFILE% del %LOGFILE%

    @goto StartTests


@REM =====================================================
        :StartTests
@REM =====================================================
    
    @echo ======= START PKITRUST REGRESSIONS =======    >> %LOGFILE%

    @echo ------- SETREG Regressions -------            >> %LOGFILE%

    cd %BASEDIR%\%THISDIR%

    %_CDB_% setreg -q 1 true 2 true 3 false 4 false     >> %LOGFILE%
    %_CDB_% setreg -q 5 true 6 false 7 true 8 false     >> %LOGFILE%
    %_CDB_% setreg -q 9 false 10 false                  >> %LOGFILE%
    %_CDB_% setreg                                      >> %LOGFILE%

    @echo ------- CHKTRUST Regressions -------          >> %LOGFILE%

    @echo ------- CHKTRUST (signing bad) --------       >> %LOGFILE%

    @cd signing
    @cd bad

    @rem --- cert signature
    %_CDB_% chktrust -q -h0x80096004 bcrtsng.cla        >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096004 bs_name.exe        >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096004 btmcrtsg.cla       >> %LOGFILE%

    %_CDB_% chktrust -q -h0x80096004 cert_pcb.exe       >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096004 cert_pcb.cab       >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096004 cert_pcb.cat       >> %LOGFILE%
   @rem %_CDB_% chktrust -q -h0x80096004 cert_pcb.doc       >> %LOGFILE%

    @rem --- counter (timestamp) cert signature
    %_CDB_% chktrust -q -h0x80096003 tscert.exe         >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096003 tscert.cat         >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096003 tscert.cab         >> %LOGFILE%
   @rem  %_CDB_% chktrust -q -h0x80096003 tscert.doc         >> %LOGFILE%

    @rem --- bad object digest
    %_CDB_% chktrust -q -h0x80096010 b_dig.ocx          >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096010 b_dig.cab          >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096010 b_dig.cla          >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096010 b_dig.dll          >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096010 b_dig.exe          >> %LOGFILE%

    %_CDB_% chktrust -q -h0x80096010 sig_pcb.exe        >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096010 sig2_pcb.exe       >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096010 sig3_pcb.exe       >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80096010 sig_pcb.cab        >> %LOGFILE%
   @rem  %_CDB_% chktrust -q -h0x80096010 sig_pcb.doc        >> %LOGFILE%

    @rem --- bad signature hash
    %_CDB_% chktrust -q -h0x800b0100 sig_pcb.cat        >> %LOGFILE%

    %_CDB_% chktrust -q -h0x800b0100 bad_sign.ocx       >> %LOGFILE%
    %_CDB_% chktrust -q -h0x800b0100 bad_sign.dll       >> %LOGFILE%

    @rem --- commercial cert with bad cridentials
    %_CDB_% chktrust -q -h0x80093005 no_creds.cab       >> %LOGFILE%

    @rem --- bad internal content
    %_CDB_% chktrust -q -h0x800b0100 badintct.exe       >> %LOGFILE%

    @rem --- commercial cert with bad financial criteria 
    @rem ------ check taken out of softpub!
    %_CDB_% chktrust -q -h0x0 bfincrit.cab              >> %LOGFILE%
    %_CDB_% chktrust -q -h0x0 fincrit.exe               >> %LOGFILE%

    @rem --- revoked certificate (expired)
    %_CDB_% setreg -q 3 TRUE                            >> %LOGFILE%
    %_CDB_% chktrust -q -h0x800b0101 revoked.exe        >> %LOGFILE%
    %_CDB_% setreg -q 3 FALSE                           >> %LOGFILE%


    @echo ------- CHKTRUST (signing good) --------      >> %LOGFILE%

    @cd ..
    @cd good

    %_CDB_% setreg -q 3 FALSE                           >> %LOGFILE%

    @rem --- Executables
    %_CDB_% chktrust -q -h0x0        good_pcb.exe       >> %LOGFILE%

    @rem --- Catalogs
    %_CDB_% chktrust -q -h0x0        good_pcb.cat       >> %LOGFILE%

    @rem --- CABs
    %_CDB_% chktrust -q -h0x0        good_pcb.cab       >> %LOGFILE%


    @rem --- Structured Storage
   @rem  %_CDB_% chktrust -q -h0x0        b_ok.doc           >> %LOGFILE%
   @rem  %_CDB_% chktrust -q -h0x0        b_ok.xls           >> %LOGFILE%
   @rem  %_CDB_% chktrust -q -h0x0        b_ok.ppt           >> %LOGFILE%

    @rem --- v1 signed files
    %_CDB_% setreg -q 8 TRUE                            >> %LOGFILE%
    %_CDB_% chktrust -q -h0x80092026 b_ok.exe           >> %LOGFILE%
    %_CDB_% setreg -q 8 FALSE                           >> %LOGFILE%

    @rem --- signed using test root
    %_CDB_% setreg -q 1 FALSE                           >> %LOGFILE%
    %_CDB_% chktrust -q -h0x800b010d good.cab           >> %LOGFILE%
    %_CDB_% setreg -q 1 TRUE                            >> %LOGFILE%

    @rem --- TimeStamped
    %_CDB_% chktrust -q -h0x0        timstamp.cab       >> %LOGFILE%


    @echo ------- CHKTRUST (unsigned) --------          >> %LOGFILE%

    @cd ..
    @cd unsigned

    @rem --- 16 bit dll -- no sip
    %_CDB_% chktrust -q -h0x800b0003 ctl3d.dll          >> %LOGFILE%

    @rem --- unsigned supported
    %_CDB_% chktrust -q -h0x800b0100 unsign.cab         >> %LOGFILE%


    @echo ------- CHKTRUST (catalogs) --------          >> %LOGFILE%
    
    @cd %BASEDIR%\%THISDIR%
    @cd catalogs

    @rem --- the testrev.exe has been signed with a revoked cert, 

    %_CDB_% setreg -q 3 TRUE                                                        >> %LOGFILE%
    %_CDB_% chktrust -q -h0x800b010e  -c test.p7s -t TestSignedEXE testrev.exe      >> %LOGFILE%
    %_CDB_% setreg -q 3 FALSE                                                       >> %LOGFILE%

    %_CDB_% chktrust -q -h0x0         -c test.p7s -t TestSignedEXENoAttr test2.exe  >> %LOGFILE%
    %_CDB_% chktrust -q -h0x0         -c test.p7s -t TestUnsignedCAB nosntest.cab   >> %LOGFILE%
    %_CDB_% chktrust -q -h0x0         -c test.p7s -t TestSignedCAB signtest.cab     >> %LOGFILE%
    %_CDB_% chktrust -q -h0x0         -c test.p7s -t TestFlat create.bat            >> %LOGFILE%

    @rem --- not found
    %_CDB_% chktrust -q -h0x800b0100  -c test.p7s -t NotThere create.bat            >> %LOGFILE%
    %_CDB_% chktrust -q -h0x800b0100  -c test.p7s -t NotThere nosntest.cab          >> %LOGFILE%

    @echo ------- CHKTRUST (catalogs DB) --------                                   >> %LOGFILE%

    @rem --- create the catalogs
    %_CDB_% makecat -h0x0 regress.cdf                                               >> %LOGFILE%
    %_CDB_% makecat -h0x0 regress2.cdf                                              >> %LOGFILE%
    %_CDB_% makecat -h0x0 catdb1.cdf                                                >> %LOGFILE%

    @rem --- sign them
    %_CDB_% signcode -spc publish.spc -v publish.pvk -n "Peter's Catalog DB Test" -i "www.microsoft.com" -$ individual regress.cat >> %LOGFILE%
    %_CDB_% signcode -spc publish.spc -v publish.pvk -n "Peter's Catalog DB Test" -i "www.microsoft.com" -$ individual regress2.cat >> %LOGFILE%
    %_CDB_% signcode -spc publish.spc -v publish.pvk -n "Peter's Catalog DB Test" -i "www.microsoft.com" -$ individual catdb1.cat >> %LOGFILE%

    @rem --- add the catalogs to the db
    %_CDB_% chktrust -acl regress.cat                                               >> %LOGFILE%
    %_CDB_% chktrust -acl regress2.cat                                              >> %LOGFILE%
    %_CDB_% chktrust -acl catdb1.cat                                                >> %LOGFILE%

    @rem --- use the db to verify
    %_CDB_% chktrust -ucl -q -h 0x0 testrev.exe                                     >> %LOGFILE%
    %_CDB_% chktrust -ucl -q -h 0x0 create.bat                                      >> %LOGFILE%
    %_CDB_% chktrust -ucl -q -h 0x0 signtest.cab                                    >> %LOGFILE%
    %_CDB_% chktrust -ucl -q -h 0x0 test2.exe                                       >> %LOGFILE%
    


    @echo ------- CHKTRUST (driver signing) --------                                >> %LOGFILE%

    %_CDB_% setreg -q 1 true                                                        >> %LOGFILE%

    @rem
    @rem driver signing provider guid
    @rem
    @set __g={F750E6C3-38EE-11d1-85E5-00C04FC295EE}
    @set __u=1.3.6.1.4.1.311.10.3.5
    @set __p=-n "Microsoft TEST" -i "http://pberkman2" 

    @cd %BASEDIR%\%THISDIR%

    @cd catalogs

    %_CDB_% makecat -h0x0 drvsign.cdf                                                       >> %LOGFILE%
    %_CDB_% makecert -eku %__u% -sv drv.pvk -n "CN=Microsoft TEST Driver Signing" drv.cer   >> %LOGFILE%
    %_CDB_% cert2spc drv.cer drv.spc                                                        >> %LOGFILE%
    %_CDB_% signcode -v drv.pvk -$ individual -spc drv.spc %__p% drvsign.CAT                              >> %LOGFILE%

        @rem -- the driver provider does not allow test roots...
        @rem -- on 4/28/98 keithv gave orders to allow trusting test roots
    @rem %_CDB_% chktrust -h 0x800b010d -g %__g% -c drvsign.cat -t testrev.exe testrev.exe       >> %LOGFILE%
    %_CDB_% chktrust -h 0x0 -g %__g% -c drvsign.cat -t testrev.exe testrev.exe       >> %LOGFILE%
        
        @rem -- "OSAttr" attribute overridden at the item level -- should fail os version
    %_CDB_% chktrust -q -h 0x47F -g %__g% -c drvsign.cat -t signtest.cab signtest.cab       >> %LOGFILE%


    @cd drvsign

    @rem ---- check catalog (good)
    %_CDB_% chktrust -q -h 0x0 -g %__g% win98drv.cat                                        >> %LOGFILE%

    @rem ---- bad digest....
    %_CDB_% chktrust -q -h 0x80096010 -g %__g% -c win98drv.cat -t msports.inf msports.inf   >> %LOGFILE%
    %_CDB_% chktrust -q -h 0x80096010 -g %__g% -c win98drv.cat -t serialui.dll serialui.dll >> %LOGFILE%

    @rem ---- the attr in the catalog is 1:4.10
    %_CDB_% chktrust -q -h 0x47F -g %__g% -c win98drv.cat -t msports.inf good.inf           >> %LOGFILE%

    @set __g=
    @set __p=

    @echo ------- MAKECAT Regressions -------                                           >> %LOGFILE%

    @cd %BASEDIR%\%THISDIR%
    @cd catalogs

    %_CDB_% makecat -h0x0 regress.cdf                                                   >> %LOGFILE%
    %_CDB_% makecat -h0x0 regress2.cdf                                                  >> %LOGFILE%

    %_CDB_% signcode -spc publish.spc -$ individual -v publish.pvk -n "Peter's Catalog Test" -i "www.microsoft.com" regress.cat >> %LOGFILE%

    %_CDB_% chktrust -q -h0x0         -c regress.cat -t TestSignedEXE testrev.exe       >> %LOGFILE%
    %_CDB_% chktrust -q -h0x0         -c regress.cat -t TestSignedEXENoAttr test2.exe   >> %LOGFILE%
    %_CDB_% chktrust -q -h0x0         -c regress.cat -t TestUnsignedCAB nosntest.cab    >> %LOGFILE%
    %_CDB_% chktrust -q -h0x0         -c regress.cat -t TestSignedCAB signtest.cab      >> %LOGFILE%
    %_CDB_% chktrust -q -h0x0         -c regress.cat -t TestFlat create.bat             >> %LOGFILE%


    @echo ------- TRSTPROV Regressions -------                                          >> %LOGFILE%

    @cd %BASEDIR%\%THISDIR%

    @echo ------- trstprov (varying struct sizes) --------                              >> %LOGFILE%

    @set __g={684D31F8-DDBA-11d0-8CCB-00C04FC295EE}

    @regsvr32 /s TProv1.DLL

    @cd signing
    @cd good

    @rem --- cert expired
    %_CDB_% chktrust -q -h0x800b0101 -g %__g% brill.cab                               >> %LOGFILE%
    %_CDB_% chktrust -q -h0x800b0101 -g %__g% b_ok.exe                                >> %LOGFILE%

    %_CDB_% chktrust -q -h0x800b0101 -g %__g% good.cab                                >> %LOGFILE%
    %_CDB_% chktrust -q -h0x0 -g %__g% timstamp.cab                                   >> %LOGFILE%

    @set __g=


    @echo ------- SIGNING (SIP) Regressions -------                                     >> %LOGFILE%

    @cd %BASEDIR%\%THISDIR%\signing\unsigned

    copy /b unsigned.cat test.cat
    %_CDB_% signcode -$ individual -spc ..\..\publish.spc -v ..\..\publish.pvk -n "SIP" -i "www.microsoft.com" test.cat >> %LOGFILE%
    %_CDB_% chktrust -q -h0x0 test.cat                                                  >> %LOGFILE%
    del test.cat

    copy /b unsigned.exe test.exe
    %_CDB_% signcode -$ individual -spc ..\..\publish.spc -v ..\..\publish.pvk -n "SIP" -i "www.microsoft.com" test.exe >> %LOGFILE%
    %_CDB_% chktrust -q -h0x0 test.exe                                                  >> %LOGFILE%
    del test.exe

    copy /b unsigned.cab test.cab
    %_CDB_% signcode -$ individual -spc ..\..\publish.spc -v ..\..\publish.pvk -n "SIP" -i "www.microsoft.com" test.cab >> %LOGFILE%
    %_CDB_% chktrust -q -h0x0 test.cab                                                  >> %LOGFILE%
    del test.cab


    @goto ExitRegress


@REM =====================================================
        :ExitRegress
@REM =====================================================

    @echo ======= END PKITRUST REGRESSIONS =======    >> %LOGFILE%

    @cd %BASEDIR%

    @if "%__CalledFromMain%" == "TRUE" @goto EndGrep

        @call grepout.bat %LOGFILE%

    :EndGrep

    @ENDLOCAL
    @goto :EOF

