@echo off
mkdir foobar
cd foobar
echo file1 > file1

rem Basic test of command line. Note a section prefix per command
rem to resync, as wine does not output anything in these cases yet.
echo --- Test 1
cmd.exe /c echo Line1
cmd.exe /c echo "Line2"
echo --- Test 2
cmd.exe /c echo Test quotes "&" work
echo --- Test 3
cmd.exe /c echo "&"
echo --- Test 4
cmd.exe /c echo "<"
echo --- Test 5
cmd.exe /c echo ">"
echo --- Test 6
cmd.exe /c echo "\"
echo --- Test 7
cmd.exe /c echo "|"
echo --- Test 8
cmd.exe /c echo "`"
echo --- Test 9
cmd.exe /c echo """
echo --- Test 10
echo on > file3
@type file3
@echo off
echo --- Test 11
cmd.exe /c echo on >file3
@type file3
@echo off
echo --- Test 12
cmd.exe /c "echo passed1"
echo --- Test 13
cmd.exe /c " echo passed2 "
echo --- Test 14
cmd.exe /c "dir /ad ..\fooba* /b"
echo --- Test 15
cmd.exe /cecho No whitespace
echo --- Test 16
cmd.exe /c
echo --- Test 17
cmd.exe /c@space@
echo --- Test 18
rem Ensure no interactive prompting when cmd.exe /c or /k
echo file2 > file2
cmd.exe /c copy file1 file2 >nul
echo No prompts or I would not get here1
rem - Try cmd.exe /k as well
cmd.exe /k "copy file1 file2 >nul && exit"
echo No prompts or I would not get here2

rem Nonexistent variable expansion is as per command line, i.e. left as-is
cmd.exe /c echo %%hello1%%
cmd.exe /c echo %%hello2
cmd.exe /c echo %%hello3^:h=t%%
cmd.exe /c echo %%hello4%%%%

rem Cannot issue a call from cmd.exe /c
cmd.exe /c call :hello5

rem %1-9 has no meaning
cmd.exe /c echo one = %%1

rem for loop vars need expanding
cmd.exe /c for /L %%i in (1,1,5) do @echo %%i

rem goto's are ineffective
cmd.exe /c goto :fred
cmd.exe /c goto eof

rem - %var% is expanded at read time, not execute time
set var=11
cmd.exe /c "set var=22 && setlocal && set var=33 && endlocal && echo var contents: %%var%%"

rem - endlocal ineffective on cmd.exe /c lines
cmd.exe /c "set var=22 && setlocal && set var=33 && endlocal && set var"
set var=99

rem - Environment is inherited ok
cmd.exe /c ECHO %%VAR%%

rem - Exit works
cmd.exe /c exit

cd ..
rd foobar /s /q

rem - Temporary batch files
echo @echo 0 > "say.bat"
echo @echo 1 > "say one.bat"
echo @echo 2 > "saytwo.bat"
echo @echo 3 > "say (3).bat"
echo @echo 4 > "say .bat"
echo @echo 5 > "bazbaz(5).bat"

echo ------ Testing invocation of batch files ----------
call say one
call "say one"
call "say"" one"
call "say one
call :setError 0
call say" one"
if errorlevel 2 echo error %ErrorLevel%
call say "one"
call :setError 0
call s"ay one
if errorlevel 2 echo error %ErrorLevel%
call :setError 0
call s"aytwo
if errorlevel 2 echo error %ErrorLevel%
call say (3)
call "say (3)"
call :setError 0
call say" (3)"
if errorlevel 2 echo error %ErrorLevel%
call :setError 0
call say" "(3) prints 4?!
if errorlevel 2 echo error %ErrorLevel%

echo ------ Testing invocation with CMD /C -------------
cmd /c say one
cmd /c "say one"
call :setError 0
cmd /c "say"" one"
if errorlevel 2 echo error %ErrorLevel%
cmd /c "say one
call :setError 0
cmd /c say" one"
if errorlevel 2 echo error %ErrorLevel%
cmd /c say "one"
call :setError 0
cmd /c s"ay one
if errorlevel 2 echo error %ErrorLevel%
call :setError 0
cmd /c s"aytwo
if errorlevel 2 echo error %ErrorLevel%
cmd /c say (3)
call :setError 0
cmd /c say" (3)"
if errorlevel 2 echo error %ErrorLevel%
call :setError 0
cmd /c say" "(3) prints 4?!
if errorlevel 2 echo error %ErrorLevel%
call :setError 0
rem Deliberately invoking a fully qualified batch name containing a bracket
rem should fail, as a bracket is a command delimiter.
cmd /c "bazbaz(5).bat"
if errorlevel 1 echo Passed

echo ---------- Testing CMD /C quoting -----------------
cmd /c @echo "hi"
call :setError 0
cmd /c say" "one
if errorlevel 2 echo error %ErrorLevel%
cmd /c @echo "\"\\"\\\"\\\\" "\"\\"\\\"\\\\"
rem ---- all 5 conditions met, quotes preserved
cmd /c "say one"
rem cond 1 - /s
cmd /s/c "say one"
cmd /s/c ""say one""
rem cond 2 - not 2 quotes
cmd /c "say one
call :setError 0
cmd /c "say"" one"
if errorlevel 2 echo error %ErrorLevel%
rem cond 3 - special char - first test fails on Vista, W2K8!
cmd /c "say (3)"
cmd /c ""say (3)""
rem cond 4 - no spaces (quotes make no difference here)
cmd /c saytwo
cmd /c "saytwo"
cmd /c "saytwo
rem cond 5 - string between quotes must be name of executable
cmd /c "say five"
echo @echo 5 >"say five.bat"
cmd /c "say five"

echo ------- Testing CMD /C qualifier treatment ------------
rem no need for space after /c
cmd /csay one
cmd /c"say one"
rem ignore quote before qualifier
rem FIXME the next command in wine starts a sub-CMD
echo THIS FAILS: cmd "/c"say one
rem ignore anything before /c
rem FIXME the next command in wine starts a sub-CMD
echo THIS FAILS: cmd ignoreme/c say one

echo --------- Testing special characters --------------
echo @echo amp > "say&.bat"
call say&
echo @echo ( > "say(.bat"
call say(
echo @echo ) > "say).bat"
call say)
echo @echo [ > "say[.bat"
call say[
echo @echo ] > "say].bat"
call say]
echo @echo { > "say{.bat"
call say{
echo @echo } > "say}.bat"
call say}
echo @echo = > "say=.bat"
call say=
echo @echo sem > "say;.bat"
call say;
setlocal DisableDelayedExpansion
echo @echo ! > "say!.bat"
call say!
endlocal
setlocal EnableDelayedExpansion
call say!
endlocal
echo @echo %%%% > "say%%.bat"
call say%%
echo @echo ' > "say'.bat"
call say'
echo @echo + > "say+.bat"
call say+
echo @echo com > "say,.bat"
call say,
echo @echo ` > "say`.bat"
call say'
echo @echo ~ > "say~.bat"
call say~

echo --------- Testing parameter passing  --------------
echo @echo 1:%%1,2:%%2 > tell.bat
call tell 1
call tell (1)
call tell 1(2)
call :setError 0
call tell(1)
if errorlevel 2 echo error %ErrorLevel%
call :setError 0
call tell((1))
if errorlevel 2 echo error %ErrorLevel%
call :setError 0
call tell(1)(2)
if errorlevel 2 echo error %ErrorLevel%
call :setError 0
call tell(1);,;(2)
if errorlevel 2 echo error %ErrorLevel%
call :setError 0
call tell;1 2
if errorlevel 2 echo error %ErrorLevel%
call :setError 0
call tell; 1, ;2
if errorlevel 2 echo error %ErrorLevel%
call :setError 0
call tell;1;;2
if errorlevel 2 echo error %ErrorLevel%
call tell "p "1 p" "2
call tell p"1 p";2

echo --------- Testing delimiters and parameter passing  --------------
echo @echo 0:%%0,1:%%1,2:%%2,All:'%%*'> tell.bat
call;tell 1 2
call   tell 1 2
==call==tell==1==2
call tell(1234)
call tell(12(34)
call tell(12;34)
echo --------- Finished  --------------
del tell.bat say*.* bazbaz*.bat
exit
:setError
exit /B %1
