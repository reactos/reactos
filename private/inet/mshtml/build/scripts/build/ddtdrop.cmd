@echo off

REM *Comments*-----------------------------------------------------------------------------------------------------------------------------------------------
REM
REM  This script will not run under Win95. The use of NT's command interpreter, CMD.EXE is necessary because of the new functions available with respect
REM  to the use of environment variables contained in this script. WINDIFF.EXE must be in the system path. 
REM
REM  This script will run on any platform type, but is NT-OS specific. Windiff will create a log file located on \\Brant\DDTDrop\Dyxxxx\Log\Dyxxxx.log which 
REM  will give the results of the drop from \\Trigger to \\Brant. Windiff has also been set to NET SEND to %computername% when the windiff is complete.
REM
REM  This script will automatically set up net connections between servers needed and then disconnect those same net connections when the windiffs are 
REM  complete. This script will use two parameters, See syntax below.
REM
REM     Revision Historm:
REM
REM     9.12.96         Bill Ritchie            New Script, Drops files from \\Trigger\F3drop\Forms\[Bldnum] to \\Brant\DDTDrop\[Bldnum]
REM     9.19.96         Bill Ritchie            Replaced Tree Copy with Xcopy.
REM
REM -----------------------------------------------------------------------------------------------------------------------------------------------------

if '%1'=='' goto syntax

REM "Network Connections"

net use m: /d
net use n: /d
net use m: \\trigger\f3drop 
net use n: \\brant\ddtdrop 
net use \\trigger\f3drop
net use \\brant\ddtdrop


REM "Xcopy Files to Brant, Windiff Daily and Rename if Weekly or Release Candidate"

set _Bldtype=%1
@echo %_bldtype%

if '%2'=='' goto dy
if '%2'=='RC' goto rc
if '%2'=='rc' goto rc
if '%2'=='WK' goto wk
if '%2'=='wk' goto wk

:dy
start /wait /high "Xcopying From \\TRIGGER\F3Drop\%1 [To] \\BRANT\DDTDrop\Forms\%1" cmd /c xcopy /seckidvr m:\%1\*.* n:\forms\%1\  
if not exist n:\forms\%1 md n:\forms\%1
if not exist n:\forms\%1\log md n:\forms\%1\log
if exist n:\forms\%1\log\%1.log del n:\forms\%1\log\%1.log 
windiff m:\%1 n:\forms\%1 -N %computername% -Sdx n:\forms\%1\log\%1.log
goto disconnect

:rc
if '%2'=='rc' set _bldtype=%_bldtype:dy=rc%
@echo %_bldtype%
start /wait /high "Xcopying From \\TRIGGER\F3Drop\%1 [To] \\BRANT\DDTDrop\Forms\%_bldtype%" cmd /c xcopy /seckidvr m:\%1\*.* n:\forms\%_bldtype%\ 
if not exist n:\forms\%_bldtype% md n:\forms\%_bldtype%
if not exist n:\forms\%_bldtype%\log md n:\forms\%_bldtype%\log
if exist n:\forms\%_bldtype%\log\%_bldtype%.log del n:\forms\%_bldtype%\log\%_bldtype%.log 
windiff m:\%1 n:\forms\%_bldtype% -N %computername% -Sdx n:\forms\%_bldtype%\log\%_bldtype%.log
goto disconnect

:wk
if '%2'=='wk' set _bldtype=%_bldtype:dy=wk%
@echo %_bldtype%
start /wait /high "Xcopying From \\TRIGGER\F3Drop\%1 [To] \\BRANT\DDTDrop\Forms\%_bldtype%" cmd /c xcopy /seckidvr m:\%1\*.* n:\forms\%_bldtype%\
if not exist n:\forms\%_bldtype% md n:\forms\%_bldtype%
if not exist n:\forms\%_bldtype%\log md n:\forms\_bldtype%\log
if exist n:\forms\%_bldtype%\log\%_bldtype%.log del n:\forms\%_bldtype%\log\%_bldtype%.log 
windiff m:\%1 n:\forms\%_bldtype% -N %computername% -Sdx n:\forms\%_bldtype%\log\%_bldtype%.log
goto disconnect

REM "Disconnect Network Connections and Remove Environment Variables"

:disconnect
net use m: /d
net use n: /d
set _bldtype=
goto end

:syntax
@echo "DDTDROP [%%1] [%%2]" *See Comments*
@echo "DDTDROP [Current Daily Build Number] plus [The first two letters, (Build Type), if the Destionation is to be renamed, IE:DYxxxx to RCxxxx]
@echo "DDTDROP [DYxxxx] plus either [RC] or [WK] 
@echo "COMMENT [%%2] IS NOT NEEDED if Destionation Directory is going to be the same NAME as the SOURCE Directory. IE:DYxxxx to DYxxxx           
goto end

:end
