::
:: Diverse tests for the CMD echoer and parser.
::
@echo off
setlocal enableextensions


::
:: Simple tests for the CMD echoer.
::
echo --------- Testing CMD echoer ---------
@echo on

if 1==1 echo j1|(echo j2) else echo j3

(echo a 1>&2|echo a 1>&2) 2>&1

echo 1
@echo 2
@@echo 3
@@@echo 4

:echo x1
::echo x2
@:echo y1
@::echo y2
@:::echo y3

@@:echo z1
@@::echo z2

foobar
echo
foobar parameter
echo parameter

toto>NUL
toto> NUL
toto >NUL
toto > NUL

toto>NUL 2>&1
toto> NUL 2>&1
toto >NUL 2>&1
toto > NUL 2>&1

a&b
a& b
a &b
a & b

a||b
a|| b
a ||b
a || b

a&&b
a&& b
a &&b
a && b

:: a|b
:: a| b
:: a |b
:: a | b

if 1==1 (echo lol) else (echo boom)

if 1==1 (
echo lol
) else (
echo boom
)

for /l %%l in (1,1,5) do (echo %%l)

for /l %%l in (1,1,5) do (
echo %%l
)

for /l %%l in (1,1,5) do (@@@echo %%l)

if 1==1 @echo hi
if 1==1 (@echo heh)
if 1==0 (@echo lol) else @echo better
if 1==0 (@echo lol) else (@echo better2)

(a)
(a b)

:: An empty parenthesized block is considered to be an error.
:: ()


::
:: Tests for delayed expansion.
::

@echo off
setlocal enableextensions
setlocal enabledelayedexpansion

echo --------- Testing Delayed Expansion ---------

:: Checking exclamation point escaping
set ENDV= ^(an open-source operating system^)
echo This is ReactOS^^!%ENDV%
echo Hello!
echo Hello!!
echo Hello!!!
echo Hello^^^! "^!"

:: The following tests are adapted from
:: https://ss64.com/nt/delayedexpansion.html

echo "Hello^World"
echo "Hello^World!"

:: Checking expansion
set "_var=first"
set "_var=second" & Echo %_var% !_var!

:: Checking expansion and replacement
set var1=Hello ABC how are you
set var2=ABC
set result=!var1:%var2%=Developer!
echo [!result!]

:: Test of FOR-loop
set COUNT=0
for /l %%v in (1,1,4) do (
  set /A COUNT=!COUNT! + 1
  echo [!COUNT!]
)
echo Total = %COUNT%


set "com[0]=lol0"
set "com[1]=lol2"
set "com[2]=lol4"
set "com[3]=lol6"
set "com[4]=lol8"
set "com[5]=lol10"

for /l %%k in (1,1,5) do (
  echo(!com[%%k]!
  if /I "%%k" equ "5" echo OHLALA
)


::
:: Re-enable the command echoer
::
@echo on

setlocal disabledelayedexpansion

echo %~DP0

set test=abc
set abc=def

echo %
echo %%
echo %%%
echo %%%%

echo %test%
echo %test%%
echo %%test%
echo %%test%%
echo %%%test%%%

echo !test!
echo !!test!!

endlocal
setlocal enabledelayedexpansion

::
:: Regular and Delayed variables
::

echo !
echo !!
echo !!!
echo !!!!
echo !a!
echo !!a!!

set a=b
:: Will display b
echo !!a!!
set b=c
:: Will still display b
echo !!a!!

echo %test%
echo %test%%
echo %%test%
echo %%test%%
echo %%%test%%%

echo %!test!%
echo !%test%!
echo !!test!!
:: That other one is the same as the previous one.
echo !^!test^!!
echo !^^!test^^!!
echo !test!
echo !test!!
echo !!test!
echo !!test!!
echo !!!test!!!


set proj=XYZ
echo !%proj%_folder!
echo !!proj!_folder!

set %proj%_folder=\\server\folder\
echo !%proj%_folder!
echo !!proj!_folder!


::
:: Delayed variables in blocks
::

if 1==1 (
    set "pc=T"
    echo pc == !pc!

    set i=3
    set "!pc!!i!=5"

    echo other pc == !pc! and !pc!!i! == !!pc!!i!!
    echo other pc == !pc! and !pc!!i! == !^!pc^!^!i^!!
    echo other pc == !pc! and !pc!!i! == ^!!pc!!i!^!
    echo other pc == !pc! and !pc!!i! == ^!^!pc^!^!i^!^!
    set "trol=!pc!!i!"
    echo the var was !trol!

    set "!pc!N=!i!"
    echo updated !pc!N == !!pc!N!
    echo updated !pc!N == !^!pc^!N!
    echo updated !pc!N == ^!!pc!N^!
    echo updated !pc!N == ^!^!pc^!N^!
    set "trol=!pc!N"
    echo updated !pc!N == !trol!
)


@echo off

::
:: Finished!
::
echo --------- Finished  --------------
goto :EOF
