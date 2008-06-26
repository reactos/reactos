@echo off
call envars.bat
call %CMI_DDKDIR%\bin\setenv %CMI_DDKDIR% fre AMD64
cd %CMI_BUILDDIR%
del CMIDriver-%CMI_VERSION%-bin-x64-WaveRT.zip
del installer\objfre_wlh_AMD64\AMD64\*.obj
del installer\objfre_wlh_AMD64\AMD64\*.exe
del cmicontrol\objfre_wlh_AMD64\AMD64\*.obj
del cmicontrol\objfre_wlh_AMD64\AMD64\*.exe
del cpl\objfre_wlh_AMD64\AMD64\*.obj
del cpl\objfre_wlh_AMD64\AMD64\*.exe
del objfre_wlh_AMD64\AMD64\*.obj
del objfre_wlh_AMD64\AMD64\*.sys
sed -i "s/CMIVERSION.*/CMIVERSION \"%CMI_VERSION%\"/" debug.hpp
sed -i "s/^\/\/#define WAVERT/#define WAVERT/" debug.hpp
nmake /x errors.err
mkdir release-x64-WaveRT
copy objfre_wlh_AMD64\AMD64\*.sys release-x64-WaveRT
sed -e "s/CMIVersion/%CMI_VERSION%/" -e "s/CMIReleaseDate/%CMI_RELEASEDATE%/" CM8738-x64-WaveRT.inf >release-x64-WaveRT\CM8738.inf
copy CHANGELOG.txt release-x64-WaveRT
cd cmicontrol
nmake /x errors.err
copy objfre_wlh_AMD64\AMD64\cmicontrol.exe ..\release-x64-WaveRT
cd ..\cpl
build -cZ
copy objfre_wlh_AMD64\AMD64\cmicpl.cpl ..\release-x64-WaveRT
cd ..\installer
build -cZ
copy objfre_wlh_AMD64\AMD64\setup.exe ..\release-x64-WaveRT
cd ..\release-x64-WaveRT
7z a -tzip ..\CMIDriver-%CMI_VERSION%-bin-x64-WaveRT.zip *
cd ..