:: ParseMcgOpt encounters the -debug flag it will redirect InvokeCsp
:: to use this script to launch CSP.exe under Rascal.

if not "%_DebugGenerateFiles%"=="1" (
    call :ComplainAboutOldWay
    goto :eof
)

if "%RascalPath%"=="" set RascalPath=%ProgramFiles%\Microsoft Rascal
"%RascalPath%\rascal.exe" /DebugExe %*
goto :eof

:ComplainAboutOldWay
echo mcg(0) : error : This way of debugging MilCodeGen doesn't work.
echo mcg(0) : error : See readme.txt for updated instructions.
