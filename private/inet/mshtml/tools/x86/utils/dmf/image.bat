@ECHO OFF

ECHO Please insert disk for %1 in drive %2
pause
dmfwrite %1 %2

IF ERRORLEVEL 0 GOTO DONE

ECHO !!ERROR - DmfWrite encountered an error writing %1 to drive %2.
pause

:DONE



