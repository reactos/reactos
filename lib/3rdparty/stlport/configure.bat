@ECHO OFF
REM **************************************************************************
REM *
REM * configure.bat for setting up compiling STLport under Windows
REM * to see available options, call with option --help
REM *
REM * Copyright (C) 2004,2005 Michael Fink
REM *
REM **************************************************************************

REM Attention! Batch file labels only have 8 significant characters!

echo STLport Configuration Tool for Windows
echo.

REM no options at all?
if NOT "%1xyz123" == "xyz123" goto init

echo Please specify at least the compiler you are going to use,
echo use "configure --help" to see the available ones.
goto skp_comp

:init

REM initially create/overwrite config.mak
echo # STLport Configuration Tool for Windows > build\Makefiles\nmake\config.mak
echo # >> build\Makefiles\nmake\config.mak
echo # config.mak generated with command line: >> build\Makefiles\nmake\config.mak
echo # configure %1 %2 %3 %4 %5 %6 %7 %8 %9 >> build\Makefiles\nmake\config.mak
echo # >> build\Makefiles\nmake\config.mak

REM First parameter can only be help or compiler
REM help option
if "%1" == "-?" goto opt_help
if "%1" == "-h" goto opt_help
if "%1" == "/?" goto opt_help
if "%1" == "/h" goto opt_help
if "%1" == "--help" goto opt_help

REM This is necessarily a compiler
goto opt_comp

REM
REM option loop
REM
:loop

REM platform option
if "%1" == "-p" goto opt_plat
if "%1" == "/p" goto opt_plat
if "%1" == "--platform" goto opt_plat

REM cross compiling
if "%1" == "-x" goto opt_x
if "%1" == "/x" goto opt_x
if "%1" == "--cross" goto opt_x

REM C runtime library
if "%1" == "--with-static-rtl" goto opt_srtl
if "%1" == "--with-dynamic-rtl" goto opt_drtl
if "%1" == "--rtl-static" goto opt_srtl
if "%1" == "--rtl-dynamic" goto opt_drtl

REM boost support
if "%1" == "--use-boost" goto opt_bst

REM multithreading support
if "%1" == "--not-thread-safe" goto opt_st
if "%1" == "--without-thread" goto opt_st

REM rtti support
if "%1" == "--no-rtti" goto opt_rtti
if "%1" == "--without-rtti" goto opt_rtti

REM additional compiler options
if "%1" == "--extra-cxxflag" goto opt_xtra

REM library name customization
if "%1" == "--lib-motif" goto opt_motf
if "%1" == "--with-lib-motif" goto opt_motf

REM build without STLport
if "%1" == "--without-stlport" goto no_sport

REM clean rule
if "%1" == "--clean" goto opt_cln

echo Unknown option: %1

:cont_lp
echo.
shift

REM no more options?
if "%1xyz123" == "xyz123" goto end_loop

goto loop


REM **************************************************************************
REM *
REM * Help
REM *
REM **************************************************************************
:opt_help
echo The first parameter must be the compiler name, here are the available
echo keywords:
echo.
echo    msvc6    Microsoft Visual C++ 6.0
echo    msvc7    Microsoft Visual C++ .NET 2002
echo    msvc71   Microsoft Visual C++ .NET 2003
echo    msvc8    Microsoft Visual C++ 2005
echo    msvc9    Microsoft Visual C++ 2008
echo    icl      Intel C++ Compiler
echo    evc3     Microsoft eMbedded Visual C++ 3 (*)
echo    evc4     Microsoft eMbedded Visual C++ .NET (*)
echo    evc8     Microsoft Visual C++ 2005 compiling for CE
echo    evc9     Microsoft Visual C++ 2008 compiling for CE
echo.
echo  (*) For these compilers the target processor is determined automatically.
echo      You must run the WCE*.BAT file you wish to build STLport for before
echo      running configure.
echo.
echo Then the following options are available:
echo.
echo "-p <platform>" or "--platform <platform>"
echo    Build STLport for the specified platform. Not all existing platforms are
echo    available, only the ones that make a difference when building STLport are.
echo    The following keywords are available:
echo    win95    Windows 95 compatible
echo    win98    Windows 98 and up to Windows XP excluded
echo    winxp    Windows XP or later (default)
echo.
echo "-x"
echo    Enables cross-compiling; the result is that all built files that are
echo    normally put under "bin" and "lib" get extra subfolders depending on
echo    the compiler name.
echo.
echo "--with-static-rtl"
echo "--with-dynamic-rtl"
echo    Enables usage of static (libc.lib family) or dynamic (msvcrt.lib family)
echo    C/C++ runtime library when linking with STLport. If you want your appli/dll
echo    to link statically with STLport but using the dynamic C runtime use
echo    --with-dynamic-rtl; if you want to link dynamicaly with STLport but using the
echo    static C runtime use --with-static-rtl. See README.options for details.
echo    Don't forget to signal the link method when building your appli or dll, in
echo    stlport/stl/config/host.h set the following macro depending on the configure
echo    option:
echo    "--with-static-rtl  -> _STLP_USE_DYNAMIC_LIB"
echo    "--with-dynamic-rtl -> _STLP_USE_STATIC_LIB"
echo.
echo "--use-boost <boost install path>"
echo    Request use of boost support (www.boost.org). For the moment only the boost
echo    type_traits library is used to get type information and to implement some
echo    specific workaround not directly implemented by STLport. To use the same
echo    support when using STLport for your application don't forget to define
echo    _STLP_USE_BOOST_SUPPORT in stlport/stl/config/user_config.h file.
echo.
echo "--without-thread"
echo    Per default STLport libraries are built in order to be usable in a multithreaded
echo    context. If you don't need this you can ask for a not thread safe version with
echo    this option.
echo.
echo "--without-rtti"
echo    Remove rtti (run time type information) support if available.
echo.
echo "--extra-cxxflag <additional compilation options>"
echo    Use this option to add any compilation flag to the build system. For instance
echo    it can be used to activate a specific processor optimization depending on your
echo    processor. For Visual C++ .Net 2003, to activate pentium 3 optim you will use:
echo    --extra-cxxflag /G7
echo    If you have several options use several --extra-cxxflag options. For instance
echo    to also force use of wchar_t as an intrinsic type:
echo    --extra-cxxflag /G7 --extra-cxxflag /Zc:wchar_t
echo.
echo "--with-lib-motif <motif>"
echo   Use this option to customize the generated library name. The motif will be used
echo   in the last place before version information, separated by an underscore, ex:
echo   stlportd_MOTIF.5.0.lib
echo   stlportstld_static_MOTIF.5.1.lib
echo   Do not forget to define _STLP_LIB_NAME_MOTIF macro in STLport configuration file
echo   to the same value if you want to keep the auto link feature supported by some
echo   compilers.
echo.
echo "--without-stlport"
echo   Option specially targetting build of the unit tests project without STLport. This
echo   is a good way to challenge the C++ Standard library implementation comming with
echo   your compiler with STLport.
echo.
echo "--clean"
echo    Removes the build configuration file.
goto skp_comp

REM **************************************************************************
REM *
REM * Compiler configuration
REM *
REM **************************************************************************
:opt_comp

if "%1" == "msvc6" goto oc_msvc6
if "%1" == "msvc71" goto oc_msv71
if "%1" == "msvc7" goto oc_msvc7
if "%1" == "msvc8" goto oc_msvc8
if "%1" == "msvc9" goto oc_msvc9
if "%1" == "icl"   goto oc_icl

if "%1" == "evc3" goto oc_evc3
if "%1" == "evc4" goto oc_evc4
if "%1" == "evc8" goto oc_evc8
if "%1" == "evc9" goto oc_evc9

if "%1" == "watcom" goto oc_wtm

echo Unknown compiler: %1
goto oc_end

:oc_msvc6
:oc_wtm
echo Setting compiler: Microsoft Visual C++ 6.0
echo COMPILER_NAME=vc6 >> build\Makefiles\nmake\config.mak
set SELECTED_COMPILER_VERSION=60
goto oc_msvc

:oc_msvc7
echo Setting compiler: Microsoft Visual C++ .NET 2002
echo COMPILER_NAME=vc70 >> build\Makefiles\nmake\config.mak
set SELECTED_COMPILER_VERSION=70
goto oc_msvc

:oc_msv71
echo Setting compiler: Microsoft Visual C++ .NET 2003
echo COMPILER_NAME=vc71 >> build\Makefiles\nmake\config.mak
set SELECTED_COMPILER_VERSION=71
goto oc_msvc

:oc_msvc8
echo Setting compiler: Microsoft Visual C++ 2005
echo COMPILER_NAME=vc8 >> build\Makefiles\nmake\config.mak
set SELECTED_COMPILER_VERSION=80
goto oc_msvc

:oc_msvc9
echo Setting compiler: Microsoft Visual C++ 2008
echo COMPILER_NAME=vc9 >> build\Makefiles\nmake\config.mak
set SELECTED_COMPILER_VERSION=90
goto oc_msvc

:oc_msvc
echo TARGET_OS=x86 >> build\Makefiles\nmake\config.mak
set SELECTED_COMPILER=msvc
echo !include msvc.mak > .\build\lib\Makefile
echo !include msvc.mak > .\build\test\unit\Makefile
echo !include msvc.mak > .\build\test\eh\Makefile
goto oc_end

:oc_icl
echo Setting compiler: Intel C++ Compiler
echo COMPILER_NAME=icl >> build\Makefiles\nmake\config.mak
echo TARGET_OS=x86 >> build\Makefiles\nmake\config.mak
set SELECTED_COMPILER=icl
echo !include icl.mak > .\build\lib\Makefile
echo !include icl.mak > .\build\test\unit\Makefile
echo !include icl.mak > .\build\test\eh\Makefile
goto oc_end

:oc_evc3
echo Setting compiler: Microsoft eMbedded Visual C++ 3
echo COMPILER_NAME=evc3 >> build\Makefiles\nmake\config.mak
rem TODO: branch on OSVERSION like below?
echo CEVERSION=300 >> build\Makefiles\nmake\config.mak
set SELECTED_COMPILER_VERSION=3
goto oc_evc

:oc_evc4
echo Setting compiler: Microsoft eMbedded Visual C++ .NET
echo COMPILER_NAME=evc4 >> build\Makefiles\nmake\config.mak
if "%OSVERSION%"=="" (
    echo OSVERSION not set, assuming target is CE 4.2
    echo CEVERSION=420 >> build\Makefiles\nmake\config.mak
) else if "%OSVERSION%"=="WCE400" (
    echo CEVERSION=400 >> build\Makefiles\nmake\config.mak
) else if "%OSVERSION%"=="WCE420" (
    echo CEVERSION=420 >> build\Makefiles\nmake\config.mak
) else if "%OSVERSION%"=="WCE500" (
    echo CEVERSION=500 >> build\Makefiles\nmake\config.mak
) else (
    echo Unknown value for OSVERSION.
    exit /b 1
)
set SELECTED_COMPILER_VERSION=4
goto oc_evc

:oc_evc8
echo Setting compiler: Microsoft Visual C++ .NET 2005 for Windows CE
echo COMPILER_NAME=evc8 >> build\Makefiles\nmake\config.mak
set SELECTED_COMPILER_VERSION=80
if "%OSVERSION%"=="" (
    echo OSVERSION not set, assuming target is CE 5.0
    echo CEVERSION=500 >> build\Makefiles\nmake\config.mak
) else if "%OSVERSION%"=="WCE400" (
    echo CEVERSION=400 >> build\Makefiles\nmake\config.mak
) else if "%OSVERSION%"=="WCE420" (
    echo CEVERSION=420 >> build\Makefiles\nmake\config.mak
) else if "%OSVERSION%"=="WCE500" (
    echo CEVERSION=500 >> build\Makefiles\nmake\config.mak
) else (
    echo Unknown value for OSVERSION.
    exit /b 1
)
set PLATFORM_SPECIFIED=1
set SELECTED_COMPILER=msvc
echo !include evc.mak > .\build\lib\Makefile
echo !include evc.mak > .\build\test\unit\Makefile
echo !include evc.mak > .\build\test\eh\Makefile
goto proc

:oc_evc9
echo Setting compiler: Microsoft Visual C++ .NET 2008 for Windows CE
echo COMPILER_NAME=evc9 >> build\Makefiles\nmake\config.mak
set SELECTED_COMPILER_VERSION=90
if "%OSVERSION%"=="" (
    echo OSVERSION not set, assuming target is CE 5.0
    echo CEVERSION=500 >> build\Makefiles\nmake\config.mak
) else if "%OSVERSION%"=="WCE400" (
    echo CEVERSION=400 >> build\Makefiles\nmake\config.mak
) else if "%OSVERSION%"=="WCE420" (
    echo CEVERSION=420 >> build\Makefiles\nmake\config.mak
) else if "%OSVERSION%"=="WCE500" (
    echo CEVERSION=500 >> build\Makefiles\nmake\config.mak
) else (
    echo Unknown value for OSVERSION.
    exit /b 1
)
set PLATFORM_SPECIFIED=1
set SELECTED_COMPILER=msvc
echo !include evc.mak > .\build\lib\Makefile
echo !include evc.mak > .\build\test\unit\Makefile
echo !include evc.mak > .\build\test\eh\Makefile
goto proc

:oc_evc
set PLATFORM_SPECIFIED=1
set SELECTED_COMPILER=evc
echo !include evc.mak > .\build\lib\Makefile
echo !include evc.mak > .\build\test\unit\Makefile
echo !include evc.mak > .\build\test\eh\Makefile
goto proc

:oc_end
goto cont_lp


REM **************************************************************************
REM *
REM * Target processor configuration (automatic)
REM *
REM **************************************************************************
:proc

if "%TARGETCPU%" == "ARM" goto pr_arm
if "%TARGETCPU%" == "ARMV4" goto pr_arm
if "%TARGETCPU%" == "ARMV4I" goto pr_arm
if "%TARGETCPU%" == "ARMV4T" goto pr_arm

if "%TARGETCPU%" == "X86" goto pr_x86
REM Type from evc3 and/or PocketPC 2002 SDK reported here
REM to correctly check the platform:
if "%TARGETCPU%" == "X86EMnset CFG=none" goto pr_emul
if "%TARGETCPU%" == "x86" goto pr_x86
if "%TARGETCPU%" == "emulator" goto pr_emul

if "%TARGETCPU%" == "R4100" goto pr_mips
if "%TARGETCPU%" == "R4111" goto pr_mips
if "%TARGETCPU%" == "R4300" goto pr_mips
if "%TARGETCPU%" == "MIPS16" goto pr_mips
if "%TARGETCPU%" == "MIPSII" goto pr_mips
if "%TARGETCPU%" == "MIPSII_FP" goto pr_mips
if "%TARGETCPU%" == "MIPSIV" goto pr_mips
if "%TARGETCPU%" == "MIPSIV_FP" goto pr_mips

if "%TARGETCPU%" == "SH3" goto pr_sh3
if "%TARGETCPU%" == "SH4" goto pr_sh4

:pr_err
echo Unknown target CPU: %TARGETCPU%
goto pr_end

:pr_arm
echo Target processor: ARM
echo TARGET_PROC=arm >> build\Makefiles\nmake\config.mak
echo TARGET_PROC_SUBTYPE=%TARGETCPU% >> build\Makefiles\nmake\config.mak
goto pr_end

:pr_x86
echo Target processor: x86
echo TARGET_PROC=x86 >> build\Makefiles\nmake\config.mak
goto pr_end

:pr_emul
echo Target processor: Emulator
echo TARGET_PROC=x86 >> build\Makefiles\nmake\config.mak
echo TARGET_PROC_SUBTYPE=emulator >> build\Makefiles\nmake\config.mak
goto pr_end

:pr_mips
echo Target processor: MIPS
echo TARGET_PROC=mips >> build\Makefiles\nmake\config.mak
echo TARGET_PROC_SUBTYPE=%TARGETCPU% >> build\Makefiles\nmake\config.mak

goto pr_end

:pr_sh3
echo Target processor: %TARGETCPU%
echo TARGET_PROC=sh3 >> build\Makefiles\nmake\config.mak
goto pr_end

:pr_sh4
echo Target processor: %TARGETCPU%
echo TARGET_PROC=sh4 >> build\Makefiles\nmake\config.mak
goto pr_end

:pr_end
goto oc_end


REM **************************************************************************
REM *
REM * Platform configuration
REM *
REM **************************************************************************
:opt_plat

if "%2" == "win95" goto op_win95
if "%2" == "win98" goto op_win98
if "%2" == "winxp" goto op_winxp

echo Unknown platform: %2
goto op_end

:op_win95
echo Setting platform: Windows 95
echo WINVER=0x0400 >> build\Makefiles\nmake\config.mak
set PLATFORM_SPECIFIED=1
goto op_end

:op_win98
echo Setting platform: Windows 98
echo WINVER=0x0410 >> build\Makefiles\nmake\config.mak
set PLATFORM_SPECIFIED=1
goto op_end

:op_winxp
echo Setting platform: Windows XP
echo WINVER=0x0501 >> build\Makefiles\nmake\config.mak
set PLATFORM_SPECIFIED=1
goto op_end

:op_end
shift

goto cont_lp


REM **************************************************************************
REM *
REM * Cross Compiling option
REM *
REM **************************************************************************

:opt_x
echo Setting up for cross compiling.
echo CROSS_COMPILING=1 >> build\Makefiles\nmake\config.mak
goto cont_lp


REM **************************************************************************
REM *
REM * C runtime library selection
REM *
REM **************************************************************************

:opt_srtl
if "%SELECTED_COMPILER%" == "msvc" goto or_sok
goto or_err

:opt_drtl
if "%SELECTED_COMPILER%" == "msvc" goto or_dok
goto or_err

:or_err
echo Error: Setting C runtime library for compiler other than microsoft ones!
goto or_end

:or_sok
echo Selecting static C runtime library for STLport
echo WITH_STATIC_RTL=1 >> build\Makefiles\nmake\config.mak
goto or_end

:or_dok
echo Selecting dynamic C runtime library for STLport
echo WITH_DYNAMIC_RTL=1 >> build\Makefiles\nmake\config.mak
goto or_end

:or_end
goto cont_lp

REM **************************************************************************
REM *
REM * boost support
REM *
REM **************************************************************************
:opt_bst
REM if (Exists("%2")) goto ob_ok
REM if !("%2" == "") goto ob_ok
goto ob_ok

echo Error: Invalid boost intallation folder ("%2").
goto ob_end

:ob_ok
echo Activating boost support using "%2" path
echo STLP_BUILD_BOOST_PATH="%2" >> build\Makefiles\nmake\config.mak

:ob_end
shift

goto cont_lp

REM **************************************************************************
REM *
REM * Multithreading support
REM *
REM **************************************************************************
:opt_st
echo Removing thread safety support
echo WITHOUT_THREAD=1 >> build\Makefiles\nmake\config.mak
goto cont_lp

REM **************************************************************************
REM *
REM * rtti support
REM *
REM **************************************************************************
:opt_rtti
echo Removing rtti support
echo WITHOUT_RTTI=1 >> build\Makefiles\nmake\config.mak
goto cont_lp

REM **************************************************************************
REM *
REM * Extra compilation flags
REM *
REM **************************************************************************
:opt_xtra
echo Adding '%2' compilation option
if "%ONE_OPTION_ADDED%" == "1" goto ox_n

echo DEFS = %2 >> build\Makefiles\nmake\config.mak
set ONE_OPTION_ADDED=1
goto ox_end

:ox_n
echo DEFS = $(DEFS) %2 >> build\Makefiles\nmake\config.mak

:ox_end
shift
goto cont_lp

REM **************************************************************************
REM *
REM * Library name configuration
REM *
REM **************************************************************************
:opt_motf
echo Using '%2' in generated library names

echo LIB_MOTIF = %2 >> build\Makefiles\nmake\config.mak

shift
goto cont_lp

REM **************************************************************************
REM *
REM * Build without STLport
REM *
REM **************************************************************************
:no_sport
echo Configured to build without STLport

echo WITHOUT_STLPORT=1 >> build\Makefiles\nmake\config.mak

shift
goto cont_lp

REM **************************************************************************
REM *
REM * Clean
REM *
REM **************************************************************************
:opt_cln
del build\Makefiles\nmake\config.mak
echo STLport configuration file removed.
goto skp_comp

REM **************************************************************************
REM *
REM * End loop
REM *
REM **************************************************************************

:end_loop

if "%PLATFORM_SPECIFIED%" == "1" goto comp
echo Setting platform: Windows XP
echo.
echo WINVER=0x0501 >> build\Makefiles\nmake\config.mak

:comp
echo Done configuring STLport.
echo.
echo Go to build/lib folder and type "nmake clean install" to build  and
echo install STLport to the "lib" and "bin" folders.
echo Go to build/test/unit folder and type nmake clean install to
echo build unit tests and install them in bin folder. 
echo.

:skp_comp
set SELECTED_COMPILER=
set SELECTED_COMPILER_VERSION=
set ONE_OPTION_ADDED=
set PLATFORM_SPECIFIED=
