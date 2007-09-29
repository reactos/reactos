@set PATH=%PATH%;%ProgramFiles%\Microsoft Visual Studio 8\Common7\IDE
@set NSIS_EXE=%ProgramFiles%\NSIS\makensis.exe
@set PATH=%PATH%;%ProgramFiles%\NSIS
@set FASTDL_PATH=C:\kjk\src\web\fastdl\www

@pushd .
@set VERSION=%1
@IF NOT DEFINED VERSION GOTO VERSION_NEEDED

@rem TODO: check if %NSIS_EXE% file exists, GOTO NSIS_NEEDED if not
@rem @IF NOT EXISTS %NSIS_EXE% GOTO NSIS_NEEDED
devenv ..\sumatrapdf.sln /Rebuild "Release|Win32"
@IF ERRORLEVEL 1 goto BUILD_FAILED
echo Compilation ok!
copy ..\Release\SumatraPDF.exe ..\Release\SumatraPDF-uncomp.exe
upx --best ..\Release\SumatraPDF.exe
@IF ERRORLEVEL 1 goto PACK_FAILED

@makensis installer
@IF ERRORLEVEL 1 goto INSTALLER_FAILED

move SumatraPDF-install.exe ..\Release\SumatraPDF-%VERSION%-install.exe
copy ..\Release\SumatraPDF-%VERSION%-install.exe %FASTDL_PATH%\SumatraPDF-%VERSION%-install.exe

@cd ..\Release
@rem don't bother compressing since our *.exe has already been packed
zip -0 SumatraPDF-%VERSION%.zip SumatraPDF.exe
copy SumatraPDF-%VERSION%.zip %FASTDL_PATH%\SumatraPDF-%VERSION%.zip
@goto END

:INSTALLER_FAILED
echo Installer script failed
@goto END

:PACK_FAILED
echo Failed to pack executable with upx. Do you have upx installed?
@goto END

:BUILD_FAILED
echo Build failed!
@goto END

:VERSION_NEEDED
echo Need to provide version number e.g. build-release.bat 1.0
@goto END

:NSIS_NEEDED
echo NSIS doesn't seem to be installed. Get it from http://nsis.sourceforge.net/Download
@goto END

:END
@popd
