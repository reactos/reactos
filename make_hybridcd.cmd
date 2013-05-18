:: Script to create a Hybrid-CD (Boot-CD + Live-CD) for demo purposes.
:: Only run it from the root "reactos" dir (where you would also call "make").
::
:: Written by Colin Finck (2010-03-10)
::

@echo off

:: Ensure that
::   - the user already built Boot-CDs and Live-CDs
::   - put his extra stuff into "hybridcd_extras"
::   - added a copy of mkisofs
if exist "boot\bootcd\." (
    if exist "boot\livecd\." (
        if exist "hybridcd_extras\." (
            if exist "hybridcd_freeldr.ini" (
                if exist "mkisofs.exe" (
                    goto NEXT
                )
            )
        )
    )
)

echo Please make sure to copy this file (make_hybridcd.cmd) and
echo hybridcd_freeldr.ini to the "reactos" subfolder in your build directory.
echo You should run make_hybridcd from there.
echo.
echo Also build regular Boot-CDs and Live-CDs first, and create a directory
echo "hybridcd_extras" (inside the reactos folder). Put everything else
echo that should go in the CD root into that directory.
echo.
echo You also need to put a version of "mkisofs.exe" into the reactos
echo directory. Get one from e.g. "PE Builder" at http://nu2.nu/.
echo Our cdmake doesn't support creating an ISO9660:1999 filesystem, which is
echo important for a universally usable disc.
goto :EOF

:: Create directories and copy the basic stuff there
:NEXT
rd /s /q "hybridcd"
mkdir "hybridcd"
mkdir "hybridcd\live"
mkdir "hybridcd\Profiles"

xcopy /e "boot\bootcd" "hybridcd"
xcopy /e "boot\livecd\reactos" "hybridcd\live"
xcopy /e "boot\livecd\Profiles" "hybridcd\Profiles"

:: Copy our modified "freeldr.ini"
copy /y "hybridcd_freeldr.ini" "hybridcd\freeldr.ini"

:: Copy the extra stuff
xcopy /e "hybridcd_extras" "hybridcd"

:: Create the ISO
mkisofs -iso-level 4 -volid "ReactOS-HybridCD" -b "loader/isoboot.bin" -no-emul-boot -boot-load-size 4 -hide "boot.catalog" -o "hybridcd.iso" "hybridcd"
