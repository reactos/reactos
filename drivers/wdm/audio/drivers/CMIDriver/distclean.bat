@echo off
call envars.bat

rd /s /q cpl\objchk_wxp_x86
rd /s /q cpl\objchk_wxp_amd64
rd /s /q cpl\objfre_wxp_x86
rd /s /q cpl\objfre_wxp_amd64
rd /s /q cpl\objchk_wlh_x86
rd /s /q cpl\objchk_wlh_amd64
rd /s /q cpl\objfre_wlh_x86
rd /s /q cpl\objfre_wlh_amd64
del cpl\errors.err
del cpl\build*.*

rd /s /q cmicontrol\objchk_wxp_x86
rd /s /q cmicontrol\objchk_wxp_amd64
rd /s /q cmicontrol\objfre_wxp_x86
rd /s /q cmicontrol\objfre_wxp_amd64
rd /s /q cmicontrol\objchk_wlh_x86
rd /s /q cmicontrol\objchk_wlh_amd64
rd /s /q cmicontrol\objfre_wlh_x86
rd /s /q cmicontrol\objfre_wlh_amd64
del cmicontrol\errors.err
del cmicontrol\build*.*

rd /s /q installer\objchk_wxp_x86
rd /s /q installer\objchk_wxp_amd64
rd /s /q installer\objfre_wxp_x86
rd /s /q installer\objfre_wxp_amd64
rd /s /q installer\objchk_wlh_x86
rd /s /q installer\objchk_wlh_amd64
rd /s /q installer\objfre_wlh_x86
rd /s /q installer\objfre_wlh_amd64
del installer\errors.err
del installer\build*.*

rd /s /q objchk_wxp_x86
rd /s /q objchk_wxp_amd64
rd /s /q objfre_wxp_x86
rd /s /q objfre_wxp_amd64
rd /s /q objchk_wlh_x86
rd /s /q objchk_wlh_amd64
rd /s /q objfre_wlh_x86
rd /s /q objfre_wlh_amd64
del errors.err

rd /s /q release-x86
rd /s /q release-x64
rd /s /q release-x86-WaveRT
rd /s /q release-x64-WaveRT
rd /s /q debug-x86
del *.zip
7z a -tzip CMIDriver-%CMI_VERSION%-src.zip * cmicontrol\* cpl\* installer\*