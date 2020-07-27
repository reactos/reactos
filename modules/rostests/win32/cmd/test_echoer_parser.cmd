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



:: Parsing FOR, IF and REM commands.
::
:: Note: wrong syntaxes for FOR, IF, REM or parsing operators
:: are considered syntactic errors and batch running stops.
:: For all other commands, execution continues.
::

@echo on

echo --------- Parsing FOR, IF and REM commands ---------

fOr@space@@space@@space@@tab@@tab@  /d@space@@space@@tab@ %%d iN     (*)@space@@space@@tab@ do   eCHo %%d
:for /asdf
:: for /d %%d in (*) do echo %%d
:: for /d %%d in (*) do echo %%~fd
for /d %%d in (*) do echo %%~ed
for /d %%d in (*) do echo %~d0:
fOr@tab@@space@@space@@space@@space@@space@@space@@space@@space@@tab@/l@space@@tab@ %%c@space@@space@@tab@ iN@tab@  (1,1,5)@space@@space@@tab@   Do@space@@space@@tab@@space@@tab@ echo@tab@@space@@space@@space@@space@@tab@@tab@%%c


iF@space@@space@@tab@   457@space@@tab@@space@@tab@neQ@space@@space@@tab@  458@space@@tab@  (@space@@tab@ echo@space@@tab@ %~d0 )  eLSe    (@space@@space@@tab@ echo $~d0@space@@tab@ )

::: if 457 nea 458 (echo %~d0) eLSe (echo $~dp)
:if 457 nea 458 (echo yo) else (echo ya)
:if 457 leq 458 (rem/? /d)
:: if 457 leq 458 ( if 457 nea 458 (echo hi) else (echo yo) )

iF@space@@space@@tab@  "2147483647"@space@@space@@tab@ gEq@space@@tab@@space@@tab@ "2147483648"@space@@tab@  (Echo Larger)@space@@space@@tab@    Else@space@@tab@  (:Echo Smaller)
ecHO sMaLlEr)

iF@space@@space@@tab@eRrOrlevel 0 echo hi!


:: rem@tab@@space@@tab@ /df@space@@space@@tab@ /d

rEM     /v@space@@tab@/d
REm@space@@space@@space@@tab@@tab@  /d
rEm REM2 /d
reM@space@@tab@ /d@space@@tab@ >@space@@tab@ NUL
:reM /?/d

:: These commands, even commented out via the parser comment colon,
:: cause parsing errors even when being commented out.
:: Replace the two '%' by a single one to see the effects.
::
:echo %~f0 %%~dp %~p1
:echo %%~d0 $~dp
:echo %%~dp:
:echo %%~b0



::
:: Parsing random commands
::

echo --------- Parsing random commands ---------

:: Parsing these commands should fail (when being un-commmented).
:@a ( b & |
: a ( b & |

:: If goto fails, batch stops. (Done OK in ROS' cmd)
:goto /asdf whatever

dir > NUL &(b)


setlocal enabledelayedexpansion

echo %~dp0

set SOMEVAR=C:\ReAcToS
rem %SOMEVAR% |

Set "_var=first"
Set "_var=second" & Rem %_var% !_var!
Set "_var=third" & Echo %_var% !_var!

endlocal



::
:: Parsing line continuations, either from parenthesized blocks
:: or via the escape caret.
:: It may be informative to manually run this test under CMD
:: with cmd!fDumpTokens and cmd!fDumpParse flags enabled.
::

echo --------- Parsing line continuations ---------

(
a & b
c
d
)
::
:: Parsed as:
:: '('[ '&'[a, CRLF[b, CRLF[c,d] ] ] ]
::


(
a && b
c
d
)
::
:: Parsed as:
:: '('[ CRLF['&&'[a,b], CRLF[c,d]] ]
::


(a & b)

(
a & b
)


(
a & b
c & d
)


(a & ^
b
c & d
)
::
:: Parsed as:
:: '('[ '&'[a, CRLF[b, '&'[c,d] ] ]
::


(
a & b
c
d
)
::
:: Parsed as:
:: '('[ '&'[a, CRLF[b, CRLF[c,d] ] ] ]
::


(
a && b
c
d
)
::
:: Parsed as:
:: '('[ CRLF['&&'[a,b], CRLF[c,d]] ]
::


(
a || b
c
d
)
::
:: Parsed as:
:: '('[ CRLF['||'[a,b], CRLF[c,d]] ]
::


(
a
b & c
d
)
::
:: Parsed as:
:: '('[ CRLF[a, '&'[b, CRLF[c,d]]] ]
::


(
a
b && c
d
)
::
:: Parsed as:
:: '('[ CRLF[a, CRLF['&&'[b,c], d]] ]
::


(
a
b
c && d
)
::
:: Parsed as:
:: '('[ CRLF[a, CRLF[b, '&&'[c,d]]] ]
::


(
a
b
c & d
)
::
:: Parsed as:
:: '('[ CRLF[a, CRLF[b, '&'[c,d]]] ]
::


(
a
b

c
d
)
::
:: Parsed as:
:: '('[ CRLF[a, CRLF[b, CRLF[c,d] ] ] ]
::



REM foo^
bar^
baz trol^


if 1==1 (echo a) else (echo b)

if 1==1 (echo a
) else (echo b)

if 1==1 (echo a) else (
echo b)

if 1==1 (
echo a
) else (
echo b
)


REM if 1==1 (^
REM echo a
REM ) ^
REM else
REM (^
REM echo b
REM )


REM if 1==1 (^
REM echo a
REM ) ^
REM else^
REM (^
REM echo b
REM )


REM if 1==1 (^
REM echo a
REM ) ^
REM else^
REM (
REM echo b
REM )


if 1==1 (^
echo a
)


if 1==1 (^
@echo a
)



(foo ^
bar
)

(foo ^
&& bar
baz
)

(foo ^
 && bar
baz
)

REM (foo^
REM && bar
REM baz
REM )

(foo^
bar
baz
)


(foo &^
& bar
baz
)


(^
foo
bar
)



(


foo ^
bar
)



(


foo ^
&& bar
baz
)



(


foo ^
 && bar
baz
)




REM (
REM
REM
REM foo^
REM && bar
REM baz
REM )



(


foo^
bar
baz
)



(


foo &^
& bar
baz
)



(


^
foo
bar
)



::
:: Tests for Character Escape and Line Continuation
::

(^"!pc::^=^!^")

(^
"!pc::^=^!^")

(
^"!pc::^=^!^")


(^"!pc::^=^
% New line %
!^")

REM  @   2>&1    (   (  dir)   )    &&     lol


echo & ^
&&lol


REM echo & ^
REM ^
REM &&lol


trol ^
&&lol


trol^
&&lol


REM (echo hi)^
REM &&lol


rem trol(^
line
@rem trol2""^
line2
:trol3^
line3

echo trol^
line

:echo trol^
line



@echo off

::
:: Finished!
::
echo --------- Finished  --------------
goto :EOF
