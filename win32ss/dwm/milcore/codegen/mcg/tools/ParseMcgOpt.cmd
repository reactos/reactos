:: You can pass the following options by setting the "McgOpt" env-var before building.
::
:: Available options:
::
::  -debug     - Runs MilCodeGen under Rascal, with a breakpoint set just before
::               ResourceGenerator.Main.
::
::               Side-effect: Leaves files in %temp%.
::               (Seems unavoidable. See mcg\tools\cleartemp.bat.)
::
::
::  -rascal    - Same as -debug.
::  -windbg    - Like -rascal, but uses windbg.exe.
::  -ntsd      - Like -rascal, but uses ntsd.exe.
::  -noRevert  - Do not revert identical files at the end of running this script.
::  -noSd      - Do not use SD support in MilCodeGen (passes -disablesd to the project)
::  -echo      - Turns echo on (for debugging this script). Output goes to buildchk.log.

:ParseArgs
    if {%1}=={} goto :EOF
    echo mcg(0) : warning : Using options from env-var McgOpt: %*

:ParseLoop    
    if {%1}=={} goto :EOF
    if /i "%1"=="-echo" echo on
    if /i "%1"=="-nosd" set _SdFlag=-disableSD
    if /i "%1"=="-noRevert" set _NoRevertFlag=1
    if /i "%1"=="-debug" (
        set Options=-debugMode
        set DebuggerHook=call %~dp0\InvokeRascal.cmd
    )
    if /i "%1"=="-rascal" (
        set Options=-debugMode
        set DebuggerHook=call %~dp0\InvokeRascal.cmd
    )
    if /i "%1"=="-windbg" (
        if not exist %DbgPath%windbg.exe (
            echo mcg^(0^) : warning : windbg.exe was not found.  Set DbgPath env var.
        )
        set Options=-debugMode
        set DebuggerHook=%DbgPath%windbg.exe -g -G -Q
    )
    if /i "%1"=="-ntsd" (
        if not exist %DbgPath%ntsd.exe (
            echo mcg^(0^) : warning : ntsd.exe was not found.  Set DbgPath env var.
        )
        set Options=-debugMode
        set DebuggerHook=%DbgPath%ntsd.exe -g -G
    )
    shift /1
    goto :ParseLoop
