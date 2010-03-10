:: Script to create a Hybrid-CD (Boot-CD + Live-CD) for demo purposes.
:: Only run it from the root "reactos" dir (where you would also call "make").
::
:: Written by Colin Finck (2010-03-10)
::
:: STEP 2 - Create the ISO
::

@echo off

:: Ensure that "mkisofs" exists
if exist "mkisofs.exe" (
    goto NEXT
)

echo mkisofs.exe was not found in the current directory.
echo Please get a correct version for it. (e.g. from "PE Builder" at http://nu2.nu)
echo.
echo Our cdmake doesn't support creating an ISO9660:1999 filesystem, which is
echo important for a universally usable disc.
goto :EOF

:: Use it
:NEXT
mkisofs -iso-level 4 -volid "ReactOS-HybridCD" -b "loader/isoboot.bin" -no-emul-boot -boot-load-size 4 -hide "boot.catalog" -o "hybridcd.iso" "hybridcd"