@echo off
call envars.bat
call %CMI_DDKDIR%\bin\setenv %CMI_DDKDIR% fre x86
cd %CMI_BUILDDIR%
del CMIDriver-%CMI_VERSION%-bin-x86-WaveRT.zip
del installer\objfre_wlh_x86\i386\*.obj
del installer\objfre_wlh_x86\i386\*.exe
del cmicontrol\objfre_wlh_x86\i386\*.obj
del cmicontrol\objfre_wlh_x86\i386\*.exe
del cpl\objfre_wlh_x86\i386\*.obj
del cpl\objfre_wlh_x86\i386\*.exe
del objfre_wlh_x86\i386\*.obj
del objfre_wlh_x86\i386\*.sys
sed -i "s/CMIVERSION.*/CMIVERSION \"%CMI_VERSION%\"/" debug.hpp
sed -i "s/^\/\/#define WAVERT/#define WAVERT/" debug.hpp
nmake /x errors.err
mkdir release-x86-WaveRT
copy objfre_wlh_x86\i386\*.sys release-x86-WaveRT
sed -e "s/CMIVersion/%CMI_VERSION%/" -e "s/CMIReleaseDate/%CMI_RELEASEDATE%/" CM8738-x32-WaveRT.inf >release-x86-WaveRT\CM8738.inf
copy CHANGELOG.txt release-x86-WaveRT
cd cmicontrol
nmake /x errors.err
copy objfre_wlh_x86\i386\cmicontrol.exe ..\release-x86-WaveRT
cd ..\cpl
build -cZ
copy objfre_wlh_x86\i386\cmicpl.cpl ..\release-x86-WaveRT
cd ..\installer
build -cZ
copy objfre_wlh_x86\i386\setup.exe ..\release-x86-WaveRT
cd ..\release-x86-WaveRT
7z a -tzip ..\CMIDriver-%CMI_VERSION%-bin-x86-WaveRT.zip *
cd ..