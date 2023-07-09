@echo off
rem
rem FYI only. Gawk cannot be found in standard env.
rem so now I'm retrieving it.
rem gawk -f wm.awk <\nt\private\genx\windows\inc\winuser.w > wm.txt
rem

perl wm.pl < \nt\private\genx\windows\inc\winuser.w > wm.txt
