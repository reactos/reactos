@echo off
echo Nuking mshtml...
set _NUKEALL=
set _NUKEMISC=
set _NUKERETAIL=
set _NUKEDEBUG=
set _NUKEPROFILE=
set _NUKEMETER=
set _QUIET=

if "%1"=="" goto SetAllNotify
for %%x in (dbg debug chk check DBG DEBUG CHK CHECK) do if "%%x" == "%1" set _NUKEDEBUG=1
for %%x in (fre free ret retail FRE FREE RET RETAIL) do if "%%x" == "%1" set _NUKERETAIL=1
for %%x in (profile icap icecap PROFILE ICAP ICECAP) do if "%%x" == "%1" set _NUKEPROFILE=1
for %%x in (meter METER) do if "%%x" == "%1" set _NUKEMETER=1
for %%x in (misc MISC) do if "%%x" == "%1" set _NUKEMISC=1
for %%x in (all ALL) do if "%%x" == "%1" goto SetAll
for %%x in (quiet QUIET) do if "%%x" == "%1" set _QUIET=1
goto StartNuke

:SetAllNotify
echo **} selective nuke enabled with:
echo **}        nuke [build]
echo **}        ie, nuke debug, nuke free, nuke misc, etc.
echo.

:SetAll
set _NUKEALL=1
set _NUKEMISC=1
set _NUKERETAIL=1
set _NUKEDEBUG=1
set _NUKEPROFILE=1
set _NUKEMETER=1
goto StartNuke

:StartNuke
if "%_NUKERETAIL%"=="1" call nukeeach obj
if "%_NUKEDEBUG%"=="1" call nukeeach objd
if "%_NUKEPROFILE%"=="1" call nukeeach objp
if "%_NUKEMETER%"=="1" call nukeeach objm
if "%_NUKEMISC%"=="" goto Quit:
@echo Nuking misc files....
@del /q types\data.cpp 1>nul: 2>nul:
@del /q types\mshtml.idl 1>nul: 2>nul:
@del /q types\mshtml_i.c 1>nul: 2>nul:
@del /q types\mshtml64.tlb 1>nul: 2>nul:
@del /q types\parser.cpp 1>nul: 2>nul:
@del /q types\pdlparse.cpp 1>nul: 2>nul:
@del /q types\___stubs.c 1>nul: 2>nul:
@del /q types\processing 1>nul: 2>nul:
@del /q src\f3\htmlpad\pad_i.c 1>nul: 2>nul:
@del /q src\f3\htmlpad\dll\pad64.tlb 1>nul: 2>nul:
@del /q src\core\include\funcsig.hxx 1>nul: 2>nul:
@del /q imgfilt\imgutil\include\dlldata.c 1>nul: 2>nul:
@del /q imgfilt\imgutil\include\imgfilt.h 1>nul: 2>nul:
@del /q imgfilt\imgutil\include\imgfilt_p.c 1>nul: 2>nul:
@del /q imgfilt\imgutil\include\imgutil.h 1>nul: 2>nul:
@del /q imgfilt\imgutil\include\imgutil.tlb 1>nul: 2>nul:
@del /q imgfilt\imgutil\include\imgutil_p.c 1>nul: 2>nul:
@del /q imgfilt\imgutil\uuid\imgfilt_i.c 1>nul: 2>nul:
@del /q imgfilt\imgutil\uuid\imgutil_i.c 1>nul: 2>nul:
@del /q imgfilt\pngfilt\include\pngfilt.h 1>nul: 2>nul:
@del /q imgfilt\pngfilt\include\pngfilt.ic 1>nul: 2>nul:
@del /q imgfilt\pngfilt\include\pngfilt64.tlb 1>nul: 2>nul:
@del /q imgfilt\jpegfilt\include\jpegfilt.h 1>nul: 2>nul:
@del /q imgfilt\jpegfilt\include\jpegfilt.ic 1>nul: 2>nul:
@del /q imgfilt\wmffilt\include\wmffilt.h 1>nul: 2>nul:
@del /q imgfilt\wmffilt\include\wmffilt.ic 1>nul: 2>nul:
@del /q imgfilt\giffilt\include\giffilt.h 1>nul: 2>nul:
@del /q imgfilt\giffilt\include\giffilt.ic 1>nul: 2>nul:
@del /q iextag\dlldata.c 1>nul: 2>nul:
@del /q iextag\iextag.h 1>nul: 2>nul:
@del /q iextag\iextag.tlb 1>nul: 2>nul:
@del /q iextag\iextag_i.c 1>nul: 2>nul:
@del /q iextag\iextag_p.c 1>nul: 2>nul:
@del /q src\f3\drt\activex\debug\msforms.twd 1>nul: 2>nul:
@del /q src\f3\drt\activex\retail\msforms.twd 1>nul: 2>nul:
@del /q src\f3\drt\activex\ship\msforms.twd 1>nul: 2>nul:
@del /q src\f3\drt\samples\xtagxmlns_.htm 1>nul: 2>nul:
@del /q src\f3\rsrc\selfreg.inf 1>nul: 2>nul:
@del /q wb\dlldata.c 1>nul: 2>nul:
@del /q wb\wb.h 1>nul: 2>nul:
@del /q wb\wb.tlb 1>nul: 2>nul:
@del /q wb\wb_i.c 1>nul: 2>nul:
@del /q wb\wb_p.c 1>nul: 2>nul:
@del /q src\edit\dlldata.c 1>nul: 2>nul:
@del /q src\edit\optshold.h 1>nul: 2>nul:
@del /q src\edit\optshold.tlb 1>nul: 2>nul:
@del /q src\edit\optshold64.tlb 1>nul: 2>nul:
@del /q src\edit\optshold_i.c 1>nul: 2>nul:
@del /q src\edit\optshold_p.c 1>nul: 2>nul:
@del /q tried\triedit\dlldata.c 1>nul 2>nul:
@del /q tried\triedit\triedit.h 1>nul 2>nul:
@del /q tried\triedit\triedit.tlb 1>nul 2>nul:
@del /q tried\triedit\triedit_i.c 1>nul 2>nul:
@del /q tried\triedit\triedit_p.c 1>nul 2>nul:
@del /q tried\triedctl\dhtmled.h 1>nul 2>nul:
@del /q tried\triedctl\dhtmled.tlb 1>nul 2>nul:
@del /q tried\triedctl\dhtmled_i.c 1>nul 2>nul:
@del /q/s c.rsp 1>nul: 2>nul:
@del /q/s build*.log 1>nul: 2>nul:
@del /q/s build*.err 1>nul: 2>nul:
@del /q/s build*.wrn 1>nul: 2>nul:
@del /q/s build\win\*.* 1>nul: 2>nul:

if "%_NUKEALL%"=="" goto Quit
if "%_QUIET%"=="1" goto Quit
echo.
@echo Files remaining after nuke:
@dir *.* /a:-r-d/s/b

:Quit
set _NUKEALL=
set _NUKEMISC=
set _NUKERETAIL=
set _NUKEDEBUG=
set _NUKEPROFILE=
set _NUKEMETER=
echo.
echo [DONE] Nuking
echo.
