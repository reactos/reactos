@echo off
if errorlevel 0 SET errorlevel=0
if errorlevel 1 SET errorlevel=1
if errorlevel 2 SET errorlevel=2
if errorlevel 3 SET errorlevel=3
if errorlevel 4 SET errorlevel=4
if errorlevel 5 SET errorlevel=5
if errorlevel 6 SET errorlevel=6
if errorlevel 7 SET errorlevel=7
if errorlevel 8 SET errorlevel=8
if errorlevel 9 SET errorlevel=9
if errorlevel 10 SET errorlevel=10
if errorlevel 99 SET errorlevel=99
if errorlevel 254 SET errorlevel=254
if errorlevel 255 SET errorlevel=255
echo ERRORLEVEL IS %errorlevel%