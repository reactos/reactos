@if "%_echo%" == "" echo off
cd ..\..\..\dev\lib
out url.lib
copy ..\..\inet\ohare\url\retail\url.lib
in -c "Latest Win32 private build" url.lib

cd ..\sdk\lib
out url.lib
copy ..\..\..\inet\ohare\url\retail\url.rlb url.lib
in -c "Latest Win32 public build" url.lib

cd ..\..\..\inet\ohare\url
