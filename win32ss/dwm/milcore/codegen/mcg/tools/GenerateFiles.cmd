@echo off

::
:: GenerateFiles.cmd runs the MilCodeGen system.
::
:: To run under Rascal (installed at %ProgramFiles%\Microsoft Rascal, or you must set %RascalPath%):
::   DebugMCG.cmd GenerateFiles.cmd
::
:: GenerateFiles.cmd may be invoked through the build process (e.g., bcz)
:: or directly from a razzle window in which case it relies on the paths provided
:: in SetClrPath.cmd
::
:: See also ParseMcgOpt.cmd for supported options.
::

setlocal enabledelayedexpansion

if "%_gf_test%"=="2" goto starttest

set DebuggerHook=

call %~dp0\ParseMcgOpt.cmd %McgOpt%

:starttest

:: Run from parent directory (relative to GenerateFiles.cmd)
pushd %~dp0\..

call %~dp0\SetClrPath.cmd
set OutputDir=%SdxRoot%\wpf

:: Check out generated files
call %~dp0\tf_GeneratedFiles %OutputDir% %~dp0\GeneratedResources.txt edit 

:: Execute the MilCodeGen project
call %~dp0\InvokeCSP.cmd %Options% -rsp:main\Resources.rsp -enableCsPrime -clrdir:%clr_ref_path% -r:%clr_ref_path%\System.Xml.dll -- %_SdFlag% -xmlFile:xml\Resource.xml -xsdFile:xml\Resource.xsd -o:%OutputDir%

:: Revert any generated files which haven't changed
if {%_NoRevertFlag%} EQU {} call %~dp0\tf_GeneratedFiles %OutputDir% %~dp0\GeneratedResources.txt revert -a

popd
endlocal
goto :EOF
