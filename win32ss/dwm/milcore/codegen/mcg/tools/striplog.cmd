@echo off
rem Strip uninteresting spew out of the given file (intended for buildchk.log)
rem
rem Particularly useful when comparing logs

if "%1"=="" goto syntax
if "%2"=="" goto syntax
if not exist %1 goto syntax

qgrep -Xv "^PUBLISHLOG:" %1 | qgrep -Xv "^Elapsed time" | qgrep -Xv "\- also opened by"> %2
goto :eof

:syntax
echo striplog ^<infile^> ^<outfile^>
