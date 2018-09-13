md %4
md %4\%5
set s=%2\sources\private\windows\base\ntcrypto\programs\bin\%5
set r=%1\1381%3.wks

for %%i in (lib exp) do cp %s%\rsabase.%%i %4\%5
for %%i in (dll sig) do cp %r%\rsabase.%%i %4\%5
for %%i in (dbg) do cp %r%\symbols\dll\rsabase.%%i %4\%5
