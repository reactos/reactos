REM create registry keys
regini perfuser.reg
copy /y obj\i386\perfuser.dll %SystemRoot%\system32
copy /y obj\i386\perfuser.pdb %SystemRoot%\system32
REM load the counters, unload the previous ones first
unlodctr perfuser
lodctr names.txt