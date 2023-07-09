@REM =================================================================
@REM ==
@REM ==     mkregres.bat -- ISPU test copier
@REM ==
@REM ==     called from ISPU/MKREGRES.BAT
@REM == 
@REM ==     parameters if called from main regress:
@REM ==            1 = ispu base directory (eg: \nt\private\ispunt)
@REM ==            2 = built directory (eg: \nt\private\ispunt\bin\objd\i386)
@REM ==            3 = target root directory (eg: \\testmachine\share1\ispunt)
@REM ==
@REM =================================================================

    @echo off

    @SETLOCAL ENABLEEXTENSIONS

    @if "%1" == "" goto NeedHelp
    @if "%2" == "" goto NeedHelp
    @if "%3" == "" goto NeedHelp

    @set THISDIR=pkitrust\test
    @set BASEDIR=%1
    @set EXEDIR=%2
    @set TARGETDIR=%3

    @goto DoCopy

    @goto ExitMkRegress

@REM =====================================================
        :NeedHelp
@REM =====================================================
    @echo Usage: mkregres source_test_directory source_exe_directory dest_test_directory
    @goto ExitMkRegress

@REM =====================================================
        :ExitMkRegress
@REM =====================================================
    
    @cd %BASEDIR%

    @ENDLOCAL
    goto :EOF


@REM =====================================================
        :DoCopy
@REM =====================================================

    xcopy %BASEDIR%\%THISDIR%\signing\*.*     %TARGETDIR%\%THISDIR%\signing\*.* /s /v /e
    xcopy %BASEDIR%\%THISDIR%\catalogs\*.*    %TARGETDIR%\%THISDIR%\catalogs\*.* /s /v /e

    copy /b %BASEDIR%\%THISDIR%\*.*              %TARGETDIR%\%THISDIR%\*.*

    copy %EXEDIR%\chktrust.*        %TARGETDIR%\.
    copy %EXEDIR%\makecat.*         %TARGETDIR%\.
    copy %EXEDIR%\signcode.*        %TARGETDIR%\.

    copy %BASEDIR%\grepout.bat      %TARGETDIR%\.
