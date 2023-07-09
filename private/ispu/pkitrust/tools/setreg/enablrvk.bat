@echo off
IF %1==on goto RevokeON
IF %1==oN goto RevokeON
IF %1==On goto RevokeON
IF %1==ON goto RevokeON

IF %1==off goto RevokeOFF
IF %1==ofF goto RevokeOFF
IF %1==oFf goto RevokeOFF
IF %1==oFF goto RevokeOFF
IF %1==Off goto RevokeOFF
IF %1==OfF goto RevokeOFF
IF %1==OFf goto RevokeOFF
IF %1==OFF goto RevokeOFF

echo.
echo SYNTAX: enablrvk [on ^| off]

GOTO DONE

:RevokeOFF

setreg	IgnoreRevocation on		> NUL
setreg	OfflineOKCommercial off 	> NUL
setreg	OfflineOKCommercialNoBadUI off	> NUL

GOTO VIEW

:RevokeON

setreg	IgnoreRevocation off		> NUL
setreg	OfflineOKCommercial on		> NUL
setreg	OfflineOKCommercialNoBadUI on	> NUL

:VIEW

echo.
setreg

:DONE
