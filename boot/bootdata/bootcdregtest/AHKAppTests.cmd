@ECHO OFF

set DRIVE=
for %%X in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do (if exist %%X:\AHK-Tests set DRIVE=%%X)

if not defined DRIVE (
    dbgprint "AHK Application testing suite not present, skipping."
    exit /b 0
)

xcopy /Y /H /E %DRIVE%:\AHK-Tests\*.* %SystemRoot%\bin
REM Download Amine's rosautotest from svn
dwnl http://svn.reactos.org/amine/rosautotest.exe %SystemRoot%\system32\rosautotest.exe
dbgprint "....AHK Application testing suite added."
