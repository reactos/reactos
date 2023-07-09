@rem set _echo=yes
chcp 437
call %_NTBINDIR%\private\developr\%USERNAME%\razzle.bat
cd %_NTBINDIR%\private\shell\ext\mlang
iebuild %1
