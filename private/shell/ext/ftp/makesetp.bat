start /w iexpress /n /q MakeEXE.sed
@rem Delete turd file left over by iexpress
del ~cabpack.cab

start /w %_NTBINDIR%\idw\iexpress.exe /n /q MakeCAB.sed

@rem Propagate the cabs to the parent
REM move MSIEFTP.cab ..
REM move MSIEFTP.exe ..

