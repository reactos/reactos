@echo off
if ("1"=="2") goto failure
if !("1"=="1") goto failure
set /a a=1
if !("%a%"=="1") goto failure
set /a b=a
if !("%b%"=="1") goto failure
set /a a=!5
if !("%a%"=="0") goto failure
set /a a=~5
if !("%a%"=="-6") goto failure
set /a a=5,a=-a
if !("%a%"=="-5") goto failure
set /a a=5*7
if !("%a%"=="35") goto failure
set /a a=2000/10
if !("%a%"=="200") goto failure
set /a a=42%9
if !("%a%"=="6") goto failure
set /a a=7+9
if !("%a%"=="16") goto failure
set /a a=9-7
if !("%a%"=="2") goto failure
set /a a=9^<^<2
if !("%a%"=="36") goto failure
set /a a=36^>^>2
if !("%a%"=="9") goto failure
set /a a=42^&9
if !("%a%"=="8") goto failure
set /a a=32^^9
if !("%a%"=="41") goto failure
set /a a=10^|22
if !("%a%"=="30") goto failure
set /a a=2,a*=3
if !("%a%"=="6") goto failure
set /a a=11,a/=2
if !("%a%"=="5") goto failure
set /a a=42,a%=9
if !("%a%"=="6") goto failure
set /a a=7,a+=9
if !("%a%"=="16") goto failure
set /a a=9,a-=7
if !("%a%"=="2") goto failure
set /a a=42,a^&=9
if !("%a%"=="8") goto failure
set /a a=32,a^^=9
if !("%a%"=="41") goto failure
set /a a=10,a^|=22
if !("%a%"=="30") goto failure
set /a a=9,a^<^<=2
if !("%a%"=="36") goto failure
set /a a=36,a^>^>=2
if !("%a%"=="9") goto failure
set /a a=(1,2)
if !("%a%"=="2") goto failure
set /a a=(a=1,a+2)
if ("%a%"=="3") goto success
goto failure

:success
echo SUCCESS!
goto done

:failure
echo FAILURE! (remove the echo off and see the last formula that executed before this line)

:done
