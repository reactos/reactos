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
if exist "output-i386\bootcd\." (
    if exist "output-i386\livecd\." (
        if exist "hybridcd_extras\." (
            if exist "mkisofs.exe" (
                goto NEXT
            )
        )
    )
)

echo Please build regular Boot-CDs and Live-CDs first!
echo Also create a directory "hybridcd_extras" and put everything else
echo for the CD root into this directory.
echo.
echo You also need to put a version of "mkisofs.exe" into this
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

xcopy /e "output-i386\bootcd" "hybridcd"
xcopy /e "output-i386\livecd\reactos" "hybridcd\live"
xcopy /e "output-i386\livecd\Profiles" "hybridcd\Profiles"

:: Copy our modified "freeldr.ini"
copy /y "hybridcd_freeldr.ini" "hybridcd\freeldr.ini"

:: Copy the extra stuff
xcopy /e "hybridcd_extras" "hybridcd"

:: Create the ISO
mkisofs -iso-level 4 -volid "ReactOS-HybridCD" -b "loader/isoboot.bin" -no-emul-boot -boot-load-size 4 -hide "boot.catalog" -o "hybridcd.iso" "hybridcd"