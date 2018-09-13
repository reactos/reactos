@echo off
set build=%1
set MSXMLEXE=\nt\drop\retail\msxml.exe
if "%1" == "debug" set MSXMLEXE=\nt\drop\debug\msxml.exe
if not exist %MSXMLEXE% goto nomsxml
dir %MSXMLEXE%

echo Testing XML %build% ...
echo Testing using %MSXMLEXE%

if not exist temp mkdir temp

echo "Testing correct files..."
cd food
for %%f in (*.xsl) do (
	echo ---------------- "%%f"
	%MSXMLEXE% -s "%%f" food.xml -o "..\temp\%%f".htm
	diff  "..\TestOutput\%%f".htm "..\temp\%%f".htm > _lastdiff
	if ERRORLEVEL 1 echo ####### FAILED
)

cd ..
cd auction

for %%f in (*.xsl) do (
	echo ---------------- "%%f"
	%MSXMLEXE% -s "%%f" auction.xml -o "..\temp\%%f".htm
	diff  "..\TestOutput\%%f".htm "..\temp\%%f".htm > _lastdiff
	if ERRORLEVEL 1 echo ####### FAILED
)

cd ..
cd details

for %%f in (*.xsl) do (
	echo ---------------- "%%f"
	%MSXMLEXE% -s "%%f" details.xml -o "..\temp\%%f".htm
	diff  "..\TestOutput\%%f".htm "..\temp\%%f".htm > _lastdiff
	if ERRORLEVEL 1 echo ####### FAILED
)

goto done

:nomsxml
echo You need to build the %MSXMLEXE% first and copy it to this directory.

:error_stop
echo ######### FAILED

:done
cd ..