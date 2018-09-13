@if "%_echo%"=="" echo off
REM %1 is the file to parse, where each line is of the form:
REM     <tagName> <tag character constant> <tag index>
REM %2 is the file to redirect output to
if exist %2 del %2
for /F "eol=; tokens=1,2,3" %%I in (%1) do @echo #define TAG_%%I DEFINE_POOLTAG((%%J), %%K) >>%2
for /F "eol=; tokens=1,2,3" %%I in (%1) do @echo DECLARE_POOLTAG(%%I, (%%J), %%K) >> %2

