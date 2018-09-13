@echo off
if not "%_echo%" == "" echo on
@rem
@rem This walks thru the list of directories and generates cabs and
@rem self-extracting exes.  Add your component's dir to this list
@rem to have the buildlab build the cabs for you.
@rem
@rem If you want to edit this file, you can find it in the shell
@rem project, in the ext\cabs folder.
@rem

REM @rem Zip folders
REM if not exist zip\nul goto next0
REM cd zip
REM call cabzip.bat
REM cd ..

:next0

@rem FTP Folders

:next1

REM @rem Active Themes
REM if not exist themes\nul goto next2
REM cd themes
REM call cabtheme.bat
REM cd ..

:next2
@rem Internet Mail and News
REM if not exist mailnews\nul goto next3
REM cd mailnews
REM call cabimn.bat
RE< cd ..

:next3
@rem Active Movie
if not exist amovie\nul goto next4
cd amovie
call qcabo.bat
cd ..

:next4
@rem IE Data binding
REM if not exist iedata\nul goto next5
REM cd iedata
REM call doiedata.bat
REM cd ..

:next5
@rem Add your component here

:: Copy for the intlkit
