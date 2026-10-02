@echo off

::
:: Runs MilCodeGen under Rascal.
::


::
:: Requirements:
::
:: 1) The machine must have a URT installed which matches what razzle has.
::    One way to find out what razzle has, is to look at the error message returned by:
::       urtrun 2.0 csc
::
:: 2) A matching Rascal must be installed.
::    (If it's not at "%ProgramFiles%\Microsoft Rascal", you must set the "RascalPath" env-var
::     to point to it.)
::
::
:: Side effect:
::    Leaves files in %temp%. These are created by csp.exe when -debugMode is used.
::


::
:: Notes:
::   This script doesn't use the build environment, and it doesn't invoke "urtrun".
::
::   Neither of those work, apparently because Rascal doesn't like COMPLUS_VERSION
::   to be set to "V2.0". If that occurs, then in Rascal's "Help | About", it
::   reports a CLR version of "V2.0" instead of "V2.0.<whatever>", and it fails to invoke
::   csp.exe.
::

setlocal enabledelayedexpansion

:: Set a flag so that GenerateFiles.cmd knows we're invoking it.
:: This allows it to emit an error if someone uses the old way.
set _DebugGenerateFiles=1

:: Enable Rascal debugging and invoke GenerateFiles.cmd
set McgOpt=-debug %McgOpt%
call %*

:: Idea: Clean up the leftover files in %temp%.
:: Would work as follows: call tools\cleartemp.bat
:: But that script may be over-aggressive.

endlocal
