@echo off

:: Set this to the path of the 7zip exe file on your local machine.--------------::
::-------------------------------------------------------------------------------::
set z7_path_exe=C:\Program Files\7-Zip\7z.exe

:: Set this to the path of the sysStore exe file on your local machine.----------::
::-------------------------------------------------------------------------------::
set path_to_systore_exe=C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\symstore.exe

:: Set this to the minimum size of an iso file in bytes to be considered valid.--::
::-------------------------------------------------------------------------------::
set min_iso_size=1000000




:: CODE SECTION
if "%DSTORE_LOCATION%"=="" (
    echo.
    echo Symbol store path is not configured on this machine.
    echo.
    echo Should be configured as: %%DSTORE_LOCATION%%
    echo                     "(environment variable.)"
    call :usage %~0
    goto :eof
)

set store_address=%DSTORE_LOCATION%

:: Creating GUID for the push session
for /F "delims=" %%A in ('powershell -command "$([guid]::NewGuid().ToString())"') do set GUID=%%A

:: Saving local file full path for use in :add_local
set localfilepath=%~f1

:: Creating a folder for the push session, that will be later deleted
:: It helps is concurrency of the push sessions
mkdir %GUID%
pushd %GUID%

if "%~1"=="" (
    call :usage %~0
) else if "%~1"=="-u" (
    call :add_url "%~2"
) else if "%~2"=="" (
    call :add_local "%localfilepath%"
) else call :usage %~0

popd
rmdir /s /q %GUID%

goto :eof

:add_url
echo url remote file
echo.

echo Downloading file...

start /wait cmd /c powershell -Command "Start-BitsTransfer -Source %1 -Destination %~nx1"

if not exist "%~nx1" (
    echo.
    echo Download failed for some reason, aborting ...
) else call :add_local "%cd%\%~nx1"

exit /b 0

:: Adding a local file.
:add_local
if not exist "%~1" (
    echo.
    echo File "%~1" does not exist.
    
    exit /b 0
)

if "%~x1"==".pdb" goto add_pdb
if "%~x1"==".zip" goto add_zip
if "%~x1"==".rpm" goto add_rpm
:: Checking if the input is a directory
for %%i in (%1) do if exist %%~si\NUL goto add_dir

echo.
echo File type of "%~nx1" NOT supported.

exit /b 0

:: Handle adding a single pdb symbol file.
:add_pdb
echo pdb file
echo.

pushd "%~dp1"
call :push_pdb "%~1"
popd

exit /b 0

:: Handle adding zip archive, which includes extraction of the zip.
:add_zip
echo zip file
echo.

echo Extracting the zip...
call "%z7_path_exe%" x -o"%~n1" "%~1" > NUL
if ERRORLEVEL 1 (
    echo.
    echo Something went wrong with the extraction, aborting ...
) else (
    echo Searching recursively for pdb files to push...
    echo.

    pushd "%~n1"
    for /d /r %%p in (.) do call :rec_find_pdb "%%p"
    popd

    rmdir /s /q "%~n1"
)

exit /b 0

:: Handle adding rpm archive, which includes extraction of the rpm.
:add_rpm
echo rpm file
echo.

echo Extracting the rpm...
call "%z7_path_exe%" x -o"%~n1" "%~1" > NUL
if ERRORLEVEL 1 (
    echo.
    echo Something went wrong with the extraction, aborting ...
) else (
    pushd "%~n1"

    echo Extracting the cpio...
    call "%z7_path_exe%" x -o"%~n1" "%~n1.cpio" > NUL
    if ERRORLEVEL 1 (
        echo.
        echo Something went wrong with the extraction, aborting ...
    ) else (
        pushd "%~n1"

        echo Searching for the iso package and extracting it...
        for /d /r %%p in (.) do (
            pushd "%%p"
            for %%f in (*) do (
                if "%%~xf"==".iso" (
                    if %%~zf gtr %min_iso_size% (
                        call "%z7_path_exe%" x -o"%%~nf" "%%~f" > NUL
                        echo Searching recursively for pdb files to push...
                        echo.
                        pushd "%%~nf"
                        for /d /r %%p in (.) do call :rec_find_pdb "%%p"
                        popd

                        rmdir /s /q "%%~nf"
                    )
                )
            )
            popd
        )

        popd

        rmdir /s /q "%~n1"
    )

    popd

    rmdir /s /q "%~n1"
)

exit /b 0

:: Handle adding a directory which has pdb symbol files.
:add_dir
echo directory path
echo.

echo Searching recursively for pdb files to push...
echo.

pushd "%~1"
for /d /r %%p in (.) do call :rec_find_pdb "%%p"
popd

exit /b 0

:: Searches recursively for pdb files in all sub-directories.
:rec_find_pdb
pushd "%~1"
for %%f in (*) do (
    if "%%~xf"==".pdb" call :push_pdb "%%~f"
)
popd
exit /b 0

:: Executing the push of the pdb.
:push_pdb
if exist "%~n1.inf" (
    for /f "tokens=2 delims=," %%t in ('findstr /b "DriverVer" "%~n1.inf"') do (
        for /f "tokens=1 delims= " %%v in ('echo %%~t') do (
            call :lock_file
            if  ERRORLEVEL 1 (
                exit /b 1
            )
            echo Pushing %~1 version %%~v to the symbol store.
            if "%DSTORE_BUILD_TAG%"=="" (
                call "%path_to_systore_exe%" add /s "%store_address%" /f "%~1" /t "%~n1" /v "%%~v" /c "eng" > NUL
                if ERRORLEVEL 1 (
                    echo - Last push was not completed successfully, skipping ...
                )
            ) else (
                call "%path_to_systore_exe%" add /s "%store_address%" /f "%~1" /t "%~n1" /v "%%~v" /c "%DSTORE_BUILD_TAG%" > NUL
                if ERRORLEVEL 1 (
                    echo - Last push was not completed successfully, skipping ...
                )
            )
            call :unlock_file
        )
    )
)
exit /b 0

:: Lock file mechanism, works as follows.
:: Creates a lock file with its unique stamp, tries to copy it to the stores
:: location, then compares the lock at the store with its own, if equal then
:: we locked and we continue, if not then we wait for 2 seconds then try again.
:: If we're unable to create a uniquely named file in the store directory we
:: abort the loop and return an error.
:lock_file
:loop
(
echo User : %USERNAME%
echo Domain : %USERDOMAIN%
echo Working Directory : %cd%
echo Date and time : %DATE% %TIME%
echo GUID : %GUID%
) > %GUID%.lock
echo n | copy /-y %GUID%.lock "%store_address%\lock" > NUL

fc %GUID%.lock "%store_address%\lock" > NUL 2>&1

if not ERRORLEVEL 1 (
    exit /b 0
) else (
    copy /y %GUID%.lock "%store_address%\%GUID%.lock" > NUL
    if ERRORLEVEL 1 (
        echo store appears to be broken, aborting
        exit /b 1
    )
    del "%store_address%\%GUID%.lock" > NUL
    echo someone else is pushing, waiting for 2 secs
    %windir%\system32\timeout /t 2 /nobreak > NUL
    goto :loop
)

:unlock_file
ren "%store_address%\lock" %GUID%.lock > NUL
if ERRORLEVEL 1 goto :unlock_file
del /q "%store_address%\%GUID%.lock" > NUL
del /q %GUID%.lock > NUL
exit /b 0

:usage
echo.
echo This script adds a pdb file to the specified symbol store.
echo.
echo  Usage:
echo.
echo  %~1 [-u] [Path]
echo.
echo.
echo  -u [Path]   Download a remote file (zip or rpm) and push pdbs.
echo              NOTE: Path MUST BE a URL.
echo  [Path]      Push pdbs, Path can be pdb, zip or rpm or a folder.
echo              NOTE: If one pdb is chosen, make sure .inf file is
echo                    in the same directory.
echo.
echo USED ENVIRONMENT VARIABLES:
echo.
echo  %%DSTORE_LOCATION%%  - The symbol store path, that pdb files
echo                        will be pushed to.
echo.
echo  %%DSTORE_BUILD_TAG%% - Will serve as a tag for future versions,
echo                         pushed with the pdb as a message.(Optional)
echo                         if not configured, the default is "eng".
exit /b 0
