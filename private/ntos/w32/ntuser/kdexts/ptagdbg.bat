@if "%_echo%"=="" echo off
REM %1 is the file to parse, where each line is of the form:
REM     <tagName> <tag character constant> <tag index>
REM %2 is the file to redirect output to
if exist %2 del %2
echo /* > %2
echo  * This list is derived from ntuser\kernel\ptag.lst and .\ptagdbg.bat >> %2
echo  */ >> %2
echo LPSTR aszTagNames[] = { >> %2

for /F "eol=; tokens=1,2,3" %%I in (%1) do @echo     "TAG_%%I",		// %%J >>%2


echo }; >> %2
