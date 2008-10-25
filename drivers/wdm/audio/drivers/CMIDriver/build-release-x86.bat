@echo off
call envars.bat
call %CMI_DDKDIR%\bin\setenv %CMI_DDKDIR% fre x86 wxp
cd %CMI_BUILDDIR%
del CMIDriver-%CMI_VERSION%-bin-x86.zip
del installer\objfre_wxp_x86\i386\*.obj
del installer\objfre_wxp_x86\i386\*.exe
del cmicontrol\objfre_wxp_x86\i386\*.obj
del cmicontrol\objfre_wxp_x86\i386\*.exe
del cpl\objfre_wxp_x86\i386\*.obj
del cpl\objfre_wxp_x86\i386\*.exe
del objfre_wxp_x86\i386\*.obj
del objfre_wxp_x86\i386\*.sys
sed -i "s/CMIVERSION.*/CMIVERSION \"%CMI_VERSION%\"/" debug.hpp
sed -i "s/^#define WAVERT/\/\/#define WAVERT/" debug.hpp
nmake /x errors.err
mkdir release-x86
copy objfre_wxp_x86\i386\*.sys release-x86
sed -e "s/CMIVersion/%CMI_VERSION%/" -e "s/CMIReleaseDate/%CMI_RELEASEDATE%/" CM8738-x32.inf >release-x86\CM8738.inf
copy CHANGELOG.txt release-x86
cd cmicontrol
nmake /x errors.err
copy objfre_wxp_x86\i386\cmicontrol.exe ..\release-x86
cd ..\cpl
build -cZ
copy objfre_wxp_x86\i386\cmicpl.cpl ..\release-x86
cd ..\installer
build -cZ
copy objfre_wxp_x86\i386\setup.exe ..\release-x86
cd ..\release-x86
7z a -tzip ..\CMIDriver-%CMI_VERSION%-bin-x86.zip *
cd ..