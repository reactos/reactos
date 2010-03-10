:: Script to create a Hybrid-CD (Boot-CD + Live-CD) for demo purposes.
:: Only run it from the root "reactos" dir (where you would also call "make").
::
:: Written by Colin Finck (2010-03-10)
::
:: STEP 1 - Prepare the basic files for the CD
::

@echo off

:: Ensure that the user already built Boot-CDs and Live-CDs
if exist "output-i386\bootcd\." (
    if exist "output-i386\livecd\." (
        goto NEXT
    )
)

echo Please build regular Boot-CDs and Live-CDs first!
goto :EOF

:: Create directories and copy our stuff there
:NEXT
rd /s /q "hybridcd"
mkdir "hybridcd"
mkdir "hybridcd\live"
mkdir "hybridcd\Profiles"

xcopy /e "output-i386\bootcd" "hybridcd"
xcopy /e "output-i386\livecd\reactos" "hybridcd\live"
xcopy /e "output-i386\livecd\Profiles" "hybridcd\Profiles"

:: Copy our modified "freeldr.ini"
copy /y "hybridcd_freeldr.ini" "hybridcd\freeldr.ini"


echo The basic stuff has been prepared in the directory "hybridcd".
echo Now add everything else you want into this directory and run
echo "hybridcd_step2" afterwards to create the ISO.