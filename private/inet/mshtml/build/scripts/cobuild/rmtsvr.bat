@echo off
rem usage:
rem  rmtsvr.bat
rem  Monitors a directory and calls batch files found there
rem

if '%_TEST%' == '1' @echo on
path d:\f3qa;%path%
set _COMMUNICATE=\\Groundhog\communicate
d:
cd \forms96
cls
title RMTSVR - Waiting for jobs (D:)
:LOOP
@echo off
if '%_TEST%' == '1' @echo on
for %%t in (%_COMMUNICATE%\*.bat) do call %%t&del %%t
sleep 1
goto loop