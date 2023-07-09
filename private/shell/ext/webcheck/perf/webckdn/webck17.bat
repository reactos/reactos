@echo off

set SERV=%1
if (%1) == () set SERV=http://vincentr-iis

:: Use the following perl script to generate webck17.htm
if (%2) == (htm) goto GENHTM
goto TEST

:GENHTM
perl webck17.per %SERV% > webck17.htm
perl webck17ch.per %SERV% > webck17.cdf
goto DONE

:TEST
cachetst free 100
sleep 10
webckdn -u%SERV%/top17/webck17.htm -r >> results.txt
cachetst free 100
sleep 10
webckdn -u%SERV%/top17/webck17.htm -c -r >> results.txt

:DONE

