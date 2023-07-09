ECHO automatic performance test running
SLEEP 5
set PADPATH=c:\f3\build\win\ship\bin\fm30pad.exe
REM SET !PERFMAIL=/t sramani /t GaryBu /t RodC /t valh /c benchr

start /wait c:\build\perf /q

REM echo /s Performance results (NT) %!PERFMAIL% >>c:\build\perf.log
REM copy c:\build\perf.log \mailout\pending
sleep 5
call BootNET.bat