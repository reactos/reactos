::
:: PROJECT:     ReactOS ISO Remastering Script
:: LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
:: PURPOSE:     Allows easy remastering of customized ReactOS ISO images.
::              Based on the MKISOFS.EXE utility and the boot/boot_images.cmake
::              script in the ReactOS source tree.
:: COPYRIGHT:   Copyright 2023 Hermès Bélusca-Maïto
::

@echo off
:: title ReactOS ISO Remastering Script

setlocal enabledelayedexpansion

::
:: Customizable settings
::
:: ISO image identifier names
set ISO_MANUFACTURER="ReactOS Project"& :: For both the publisher and the preparer
set ISO_VOLNAME="ReactOS"&              :: For both the Volume ID and the Volume set ID

:: Image names of the MKISOFS and ISOHYBRID tools
set MKISOFS=MKISOFS.EXE
set ISOHYBRID=ISOHYBRID.EXE


::
:: Main script
::
cls
echo *******************************************************************************
echo *                                                                             *
echo *                       ReactOS ISO Remastering Script                        *
echo *                                                                             *
echo *******************************************************************************
echo.

:: Newline macro, see https://stackoverflow.com/a/6379940
(set \n=^
%=Don't remove this line=%
)


:: Verify that we have access to a temporary directory.
:: See https://stackoverflow.com/a/21041546 for more details.
if not defined TEMP goto :TEMP1
if not exist "%TEMP%\" goto :TEMP1
goto :TEMP0
:TEMP1
    if not defined TMP goto :TEMP2
    if not exist "%TMP%\" goto :TEMP2
    goto :TEMP10
    :TEMP2
        echo No temporary directory exists on your system.
        echo Please create one and assign it to the TEMP environment variable.
        echo.
        goto :EOC
    :TEMP10
    set "TEMP=%TMP%"
:TEMP0


:: Try to auto-locate MKISOFS and if not, prompt the user for a directory.
set TOOL_DIR=
set TOOL_PATH=
for /f "delims=" %%f in ('WHERE %MKISOFS% 2^>NUL') do (
    set "TOOL_DIR=%%~dpf" & if "!TOOL_DIR:~-1!"=="\" set "TOOL_DIR=!TOOL_DIR:~0,-1!"
    set "TOOL_PATH=%%f"
    goto :mkisofs_found
)
if not defined TOOL_PATH (
    set /p TOOL_DIR="Please enter the directory path where %MKISOFS% can be found:!\n!"
    echo.
    if "!TOOL_DIR:~-1!"=="\" set "TOOL_DIR=!TOOL_DIR:~0,-1!"
    set "TOOL_PATH=!TOOL_DIR!\%MKISOFS%"
)
:mkisofs_found
set MKISOFS=%TOOL_PATH%


set /p INPUT_DIR="Please enter the path of the directory tree to be copied into the ISO:!\n!"
echo.
set /p OUTPUT_ISO="Please enter the path where the ISO image should be created:!\n!"
echo.


:: Retrieve the full paths to the 'isombr', 'isoboot', 'isobtrt' and 'efisys' files
set isombr_file=loader/isombr.bin
set isoboot_file=loader/isoboot.bin
set isobtrt_file=loader/isobtrt.bin
set efisys_file=loader/efisys.bin

set ISOBOOT_PATH=%isoboot_file%
CHOICE /c 12 /n /m "Please chose the ISO boot file: 1) isoboot.bin ; 2) isobtrt.bin!\n![default: 1]: "
echo.
if errorlevel 2 set ISOBOOT_PATH=%isobtrt_file%
if errorlevel 1 set ISOBOOT_PATH=%isoboot_file%
:: if %ERRORLEVEL% equ 2 set ISOBOOT_PATH=%isobtrt_file%
:: if %ERRORLEVEL% equ 1 set ISOBOOT_PATH=%isoboot_file%
:: :: if %ERRORLEVEL% equ 0 goto :EOC

echo ISO boot file: '%ISOBOOT_PATH%'
echo EFI boot file: '%efisys_file%'
echo.


set DUPLICATES_ONCE=
CHOICE /c YN /n /m "Do you want to store duplicated files only once (reduces the size!\n!of the ISO image) [Y,N]? "
echo.
if %ERRORLEVEL% equ 1 set DUPLICATES_ONCE=-duplicates-once


echo Creating the ISO image...
echo.

:: Create a mkisofs sort file to specify an explicit ordering for the boot files
:: to place them at the beginning of the image (makes ISO image analysis easier).
:: See mkisofs/schilytools/mkisofs/README.sort and boot/boot_images.cmake script
:: in the ReactOS source tree for more details.
:: https://stackoverflow.com/a/29635767
(
    ::echo ${CMAKE_CURRENT_BINARY_DIR}/empty/boot.catalog 4
    echo boot.catalog 4
    :: NOTE: Convert Unix / to backslashes.
    echo %INPUT_DIR%\%isoboot_file:/=\% 3
    echo %INPUT_DIR%\%isobtrt_file:/=\% 2
    echo %INPUT_DIR%\%efisys_file:/=\% 1
) > "%TEMP%\bootfiles.sort"

:: Finally, create the ISO image proper.
"%MKISOFS%" ^
    -o "%OUTPUT_ISO%" -iso-level 4 ^
    -publisher %ISO_MANUFACTURER% -preparer %ISO_MANUFACTURER% ^
    -volid %ISO_VOLNAME% -volset %ISO_VOLNAME% ^
    -eltorito-boot %ISOBOOT_PATH% -no-emul-boot -boot-load-size 4 ^
    -eltorito-alt-boot -eltorito-platform efi -eltorito-boot %efisys_file% -no-emul-boot ^
    -hide boot.catalog -sort "%TEMP%\bootfiles.sort" ^
    %DUPLICATES_ONCE% -no-cache-inodes "%INPUT_DIR%"
:: -graft-points -path-list "some/directory/iso_image.lst"
echo.

if errorlevel 1 (
del "%TEMP%\bootfiles.sort"
    echo An error %ERRORLEVEL% happened while creating the ISO image "%OUTPUT_ISO%".
    goto :EOC
) else (
del "%TEMP%\bootfiles.sort"
    echo The ISO image "%OUTPUT_ISO%" has been successfully created.
)
echo.


:: Check whether ISOHYBRID is also available and if so, propose to post-process
:: the generated ISO image to allow hybrid booting as a CD-ROM or as a hard disk.
set TOOL_PATH=
for /f "delims=" %%f in ('WHERE "%TOOL_DIR%":%ISOHYBRID% 2^>NUL') do (set "TOOL_PATH=%%f" & goto :isohybrid_found)
if not defined TOOL_PATH (
    for /f "delims=" %%f in ('WHERE %ISOHYBRID% 2^>NUL') do (
        set "TOOL_DIR=%%~dpf" & if "!TOOL_DIR:~-1!"=="\" set "TOOL_DIR=!TOOL_DIR:~0,-1!"
        set "TOOL_PATH=%%f"
        goto :isohybrid_found
    )
    if not defined TOOL_PATH (
        echo %ISOHYBRID% patching skipped.
        goto :Success
    )
)
:isohybrid_found
set ISOHYBRID=%TOOL_PATH%


CHOICE /c YN /n /m "Do you want to post-process the ISO image to allow hybrid booting!\n!as a CD-ROM or as a hard disk [Y,N]? "
echo.
if %ERRORLEVEL% neq 1 goto :Success

echo Patching the ISO image...

:: NOTE: Convert Unix / to backslashes.
"%ISOHYBRID%" -b %INPUT_DIR%\%isombr_file:/=\% -t 0x96 "%OUTPUT_ISO%"
echo.

if errorlevel 1 (
    echo An error %ERRORLEVEL% happened while patching the ISO image "%OUTPUT_ISO%".
    goto :EOC
)
:: else (
::     echo The ISO image "%OUTPUT_ISO%" has been successfully patched.
:: )
:: echo.


:Success
echo Success^^!
:EOC
endlocal
echo Press any key to quit...
pause > NUL
exit /b
