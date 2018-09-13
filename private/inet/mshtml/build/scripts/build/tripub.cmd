@echo off

REM *NOTES*-----------------------------------------------------------------------------------------------------------------------------------------------
REM
REM  This script will not run under Win95. The use of NT's command interpreter, CMD.EXE is necessary because of the new functions available with respect
REM  to the use of environment variables contained in this script. WINDIFF.EXE must be in the system path. 
REM
REM  This script will run on any platform type, but is NT-OS specific. Windiff will create a log file located on \\Trident\Public\Dyxxxx\Log\Dyxxxx.log which 
REM  will give the results of the drop from \\Trigger to \\Trident. Windiff has also been set to NET SEND to %computername% when the windiff is complete.
REM
REM  This script will automatically set up net connections between servers needed and then disconnect those same net connections when the windiffs are 
REM  complete. This script will use two parameters, See syntax below.
REM
REM     Revision Historw:
REM
REM     9.12.96         Bill Ritchie            New Script, Drops files from \\Trigger\F3drop\[Bldnum]\Build to \\TRIDENT\Public\[Bldnum]\Build
REM
REM -----------------------------------------------------------------------------------------------------------------------------------------------------

if '%1'=='' goto syntax

REM "Network Connections"

net use w: /d
net use x: /d
net use w: \\trigger\f3drop 
net use x: \\trident\public 
net use \\trigger\f3drop
net use \\trident\public


REM "Xcopy Files to Trident, Windiff Daily and Rename if Weekly or Release Candidate"

@echo %1
set _Bldtype=%1
@echo %_bldtype%

if '%2'=='' goto dy
if '%2'=='rc' goto rc
if '%2'=='wk' goto wk

:dy
start /wait /high "Xcopying From \\TRIGGER\F3Drop\%1\Build [To] \\TRIDENT\Public\%1\Build" cmd /c xcopy /seckidvr w:\%1\build\*.* x:\%1\build\  
if not exist x:\%1 md x:\%1
if not exist x:\%1\log md x:\%1\log
if not exist x:\%1\build md x:\%1\build
if exist x:\%1\log\%1.log del x:\%1\log\%1.log 
windiff w:\%1\build x:\%1\build -N %computername% -Sdx x:\%1\log\%1.log
goto disconnect

:rc
if '%2'=='rc' set _bldtype=%_bldtype:dy=rc%
start /wait /high "Tree Copying From \\TRIGGER\F3Drop\%1\Build [To] \\TRIDENT\Public\%_bldtype%\Build" cmd /c xcopy /seckidvr w:\%1\build\*.* x:\%_bldtype%\build\ 
if not exist x:\%_bldtype% md x:\%_bldtype%
if not exist x:\%_bldtype%\log md x:\%_bldtype%\log
if not exist x:\%_bldtype%\build md x:\%_bldtype%\build
if exist x:\%_bldtype%\log\%_bldtype%.log del x:\%_bldtype%\log\%_bldtype%.log 
windiff w:\%1\build x:\%_bldtype%\build -N %computername% -Sdx x:\%_bldtype%\log\%_bldtype%.log
ren w:\%1\ w:\%_bldtype%\
goto disconnect

:wk
if '%2'=='wk' set _bldtype=%_bldtype:dy=wk%
start /wait /high "Tree Copying From \\TRIGGER\F3Drop\%1\Build [To] \\TRIDENT\Public\%_bldtype%\Build" cmd /c xcopy /seckidvr w:\%1\build\*.* x:\%_bldtype%\build\ 
if not exist x:\%_bldtype% md x:\%_bldtype%
if not exist x:\%_bldtype%\log md x:\%_bldtype%\log
if not exist x:\%_bldtype%\build md x:\%_bldtype%\build
if exist x:\%_bldtype%\log\%_bldtype%.log del x:\%_bldtype%\log\%_bldtype%.log 
windiff w:\%1\build x:\%_bldtype%\build -N %computername% -Sdx x:\%_bldtype%\log\%_bldtype%.log
ren w:\%1\ w:\%_bldtype%\
goto disconnect


REM "Disconnect Network Connections and Remove Environment Variables"

:disconnect
net use w: /d
net use x: /d
set _bldtype=
goto end

:syntax
@echo "TRIPUB [%%1] [%%2]" *See Comment*
@echo "TRIPUB [Current Daily Build Number] plus [The first to letters, (Build Type), if the Destionation is to be renamed, IE:DYxxxx to RCxxxx]
@echo "TRIPUB [DYxxxx] plus either [RC] or [WK] 
@echo "COMMENT [%%2] IS NOT NEEDED if Destionation Directory is going to be the same NAME as the SOURCE Directory. IE:DYxxxx to DYxxxx           
goto end

:end
