@echo off
@rem ======================================================================
@rem ======================================================================
@rem
@rem    Microsoft Windows
@rem
@rem    Copyright (c) Microsoft Corporation, 1996 - 1997
@rem
@rem    File:       signfiles.bat
@rem
@rem    Contents:   Microsoft PRS Signing Utilities
@rem
@rem    History:    20-Aug-1997 pberkman   created
@rem     
@rem ======================================================================
@rem ======================================================================

@SETLOCAL ENABLEEXTENSIONS

@rem ======================================================================
@rem    defaults
@rem ======================================================================

    @set __TSURL=http://timestamp.verisign.com/scripts/timstamp.dll
    @set __TSTRIES=4
    
    @set __IESIGNSPC=IEBetaPub.spc
    @set __IESIGNPVK=IEBetaPub.pvk

    @set __USESPYRUS=
    @set __USEIETESTCERT=
    @set __CHECK=
    @set __TEST=
    @set __KEYNAME=
    @set __SPCFILE=
    @set __PROVNAME=
    @set __PROVTYPE=
    @set __JAVAPARAMS=
    @set __CALLFROMSTART=


@rem ======================================================================
@rem    check command line
@rem ======================================================================

    :CheckCmdLine

    @if "%1" == ""                  @goto Initialize

    @if /I "%1" == "-?"             @goto HelpMe
    @if /I "%1" == "/?"             @goto HelpMe

    @if /I "%1" == "-v"             @echo on
    @if /I "%1" == "/v"             @echo on

    @if /I "%1" == "-T"             @set __TEST=TRUE
    @if /I "%1" == "/T"             @set __TEST=TRUE

    @if /I "%1" == "-C"             @set __CHECK=TRUE
    @if /I "%1" == "/C"             @set __CHECK=TRUE
    
    @if /I "%1" == "-!"             @set __CALLFROMSTART=TRUE
    @if /I "%1" == "/!"             @set __CALLFROMSTART=TRUE

    @if /I "%1" == "-S"             @set __USESPYRUS=TRUE
    @if /I "%1" == "/S"             @set __USESPYRUS=TRUE

    @shift
    @goto CheckCmdLine


@rem ======================================================================
@rem    need help!
@rem ======================================================================

    :HelpMe

    @echo Usage:  %0 [-v, -c, -t]
    @echo        parameters (optional)
    @echo            -v: verbose
    @echo            -c: run chktrust on each file
    @echo            -t: use test sign

    @goto end_signfiles


@rem ======================================================================
@rem    initialization and variable checks
@rem ======================================================================

    :Initialize

    @if not exist "%__INPUTFILE%"       goto ERRNoListFile
    
    @if "%__CALLFROMSTART%" == ""       @echo Processing ...
    @if "%__CALLFROMSTART%" == ""       @set __OLDPATH=%PATH%
    @if "%__CALLFROMSTART%" == ""       @set PATH=\CryptSDK\Bin;%PATH%
    @if "%__CALLFROMSTART%" == ""       @set __INPUTFILE=LIST.TXT

    @if "%__CALLFROMSTART%" == "TRUE"   @set __IESIGNPVK=..\%__IESIGNPVK%
    @if "%__CALLFROMSTART%" == "TRUE"   @set __IESIGNSPC=..\%__IESIGNSPC%

    @if "%COMPUTERNAME%"    == ""       @set COMPUTERNAME=TEST

    prsparse "%__INPUTFILE%" "%COMPUTERNAME%.TXT"


    @if "%__TEST%" == "TRUE"            @goto SetupTest

    @goto SetupHardware


@rem ======================================================================
@rem    the following are relevant to the Hardware set up.
@rem ======================================================================

    :SetupHardware

    :SpyrusBox

    @if NOT "%__USESPYRUS%" == "TRUE" @goto BBNBox

    @set __KEYNAME=MSInternal
    @set __SPCFILE=..\MSInternal.SPC
    @set __PROVNAME=SPYRUS Lynks RSA/DES/CAST3 CSP v1.5
    @set __PROVTYPE=10

    @goto ForLoop


    :BBNBox

    @set __KEYNAME=Nehemiah
    @set __SPCFILE=..\NEHEMIAH.SPC
    @set __PROVNAME=BBN SafeKeyper Crypto Provider V0.1
    @set __PROVTYPE=2

    @goto ForLoop


@rem ======================================================================
@rem    the following are relevant to the test set up.
@rem ======================================================================

    :SetupTest

    @set __KEYNAME=__MS TEST
    @set __SPCFILE=%COMPUTERNAME%.spc
    @set __PROVNAME=Microsoft Base Cryptographic Provider v1.0
    @set __PROVTYPE=1

    @if exist %__IESIGNPVK%     @goto SetupTestIE

    @if exist "%COMPUTERNAME%.cer"  del /q "%COMPUTERNAME%.cer"
    @if exist %__SPCFILE%           del /q %__SPCFILE%

    makecert -sk "%__KEYNAME%" -n "CN=__MS Test" "%COMPUTERNAME%.cer"
    cert2spc "%COMPUTERNAME%.cer" "%__SPCFILE%
    setreg -q 1 TRUE

    @goto ForLoop

@rem ======================================================================
@rem    the following are relevant to the test set up for IE.
@rem ======================================================================

    :SetupTestIE

    @set __USEIETESTCERT=TRUE

    @goto ForLoop


@rem ======================================================================
@rem
@rem    format:
@rem            set comment character (eol) to ';'.
@rem            set the field delimiter to ','.
@rem    goal:
@rem        %i/1: file to be signed
@rem        %j/2: file description
@rem        %k/3: file more info URL
@rem        %l/4: JAVA permisions
@rem        %m/5: PRS attributes
@rem
@rem ======================================================================

    :ForLoop

    FOR /F "eol=; delims=, tokens=1,2,3,4*" %%i IN (%COMPUTERNAME%.TXT) DO call :DoSign %%i %%j %%k %%l %%m

    @goto FinishedOK


@rem ======================================================================
@rem    do the actual signing
@rem ======================================================================
    
    :DoSign

    @if not exist "%1" goto ErrNoFileToSign

    @set __ATTRPARAMS=-j prsattr.dll -jp %5
    @if NOT "%4" == "" @set __ATTRPARAMS=%__ATTRPARAMS% -j javasign.dll -jp %4

    @echo ......name: %1
    @echo ......desc: %2
    @echo .......url: %3
    @echo ......java: %4
    @echo .......prs: %5

    @if     "%__USEIETESTCERT%" == ""  signcode -tr %__TSTRIES% -tw 5 -spc "%__SPCFILE%" -k "%__KEYNAME%" -p "%__PROVNAME%" -y %__PROVTYPE% -n %2 -i %3 -t "%__TSURL%" %__ATTRPARAMS% %1
    @if NOT "%__USEIETESTCERT%" == ""  signcode -tr %__TSTRIES% -tw 5 -spc %__IESIGNSPC% -v %__IESIGNPVK% -n %2 -i %3 -t "%__TSURL%" %__ATTRPARAMS% %1
    
    @if "__CHECK" == "TRUE" chktrust %1

    @goto :EOF


@rem ======================================================================
@rem    errors
@rem ======================================================================

    :ErrNoFileToSign
    @echo ERROR:
    @echo       file to sign not found ("%1").  FATAL!
    @goto end_signfiles

    :ErrNoListFile
    @echo ERROR:
    @echo        no %__INPUTFILE% file found.  FATAL!
    goto end_signfiles


@rem ======================================================================
@rem    final processing for each file in the TXT file
@rem ======================================================================

    :FinishedOK

    @if exist "%COMPUTERNAME%.TXT" del /q "%COMPUTERNAME%.TXT"
    @if exist "%COMPUTERNAME%.SPC" del /q "%COMPUTERNAME%.SPC"
    @if exist "%COMPUTERNAME%.CER" del /q "%COMPUTERNAME%.CER"

    @goto end_signfiles


@rem ======================================================================
@rem    end
@rem ======================================================================

    :end_signfiles

    @if "%__CALLFROMSTART%" == ""  @set PATH=%__OLDPATH%

    @ENDLOCAL

