@rem echo off

@rem the next line reexecutes the script without params if it was called with params, else we'll get false failures
@if not "%1"=="" seta_test.cmd

@rem the next two lines illustrate bug in existing if code
if not "=="=="==" goto failure
if "=="=="==" goto next1
goto failure
:next1
if "1"=="2" goto failure
if not "1"=="1" goto failure
set /a a=1
if not "%a%"=="1" goto failure
set /a b=a
if not "%b%"=="1" goto failure
set /a a=!5
if not "%a%"=="0" goto failure
set /a a=!a
if not "%a%"=="1" goto failure
set /a a=~5
if not "%a%"=="-6" goto failure
set /a a=5,a=-a
if not "%a%"=="-5" goto failure
set /a a=5*7
if not "%a%"=="35" goto failure
set /a a=2000/10
if not "%a%"=="200" goto failure
set /a a=42%%9
if not "%a%"=="6" goto failure
set /a a=5%2
if not "%a%"=="5" goto failure
set /a a=42^%13
if not "%a%"=="423" goto failure
set /a a=7+9
if not "%a%"=="16" goto failure
set /a a=9-7
if not "%a%"=="2" goto failure
set /a a=9^<^<2
if not "%a%"=="36" goto failure
set /a a=36^>^>2
if not "%a%"=="9" goto failure
set /a a=42^&9
if not "%a%"=="8" goto failure
set /a a=32^9
if not "%a%"=="329" goto failure
set /a a=32^^9
if not "%a%"=="41" goto failure
set /a a=10^|22
if not "%a%"=="30" goto failure
set /a a=2,a*=3
if not "%a%"=="6" goto failure
set /a a=11,a/=2
if not "%a%"=="5" goto failure
set /a a=42,a%%=9
if not "%a%"=="6" goto failure
set /a a=7,a+=9
if not "%a%"=="16" goto failure
set /a a=9,a-=7
if not "%a%"=="2" goto failure
set /a a=42,a^&=9
if not "%a%"=="8" goto failure
set /a a=32,a^^=9
if not "%a%"=="41" goto failure
set /a a=10,a^|=22
if not "%a%"=="30" goto failure
set /a a=9,a^<^<=2
if not "%a%"=="36" goto failure
set /a a=36,a^>^>=2
if not "%a%"=="9" goto failure
set /a a=1,2
if not "%a%"=="1" goto failure
set /a a=(a=1,a+2)
if "%a%"=="3" goto success
goto failure

:success
echo SUCCESS!
goto done

:failure
echo FAILURE! remove the echo off and see the last formula that executed before this line

:done
