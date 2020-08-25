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


@echo off

::
:: Finished!
::
echo --------- Finished  --------------
goto :EOF
