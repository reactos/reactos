ECHO automatic performance test running
SLEEP 5
set PADPATH=c:\f3\build\win\ship\bin\fm30pad.exe
SET !PERFMAIL=/t sramani /t GaryBu /t RodC /t valh /c benchr

start /wait perf /q

echo /s Performance results (95) %!PERFMAIL% >>c:\build\perf.log
copy c:\build\perf.log \mailout\pending
sleep 5
call BootNET.bat