@echo off

IF '%1'=='' GOTO NO_PARAM

ECHO ÿ
ECHO NOTE: MakeDisk.bat generates DMF formatted floppies only.
ECHO       Any floppies formatted 1.44 should be copied seperately.
ECHO ÿ
ECHO WARNING!: Floppies may not be requested in numerical order.
ECHO ÿ

FOR %%I IN (DISK*.DMF) DO CALL image.bat %%I %1
GOTO DONE

:NO_PARAM
ECHO ÿ
ECHO Usage: "makedisk <drive_letter>:"
ECHO          e.g. makedisk A:
ECHO ÿ
GOTO DONE

:DONE
ECHO ÿ

