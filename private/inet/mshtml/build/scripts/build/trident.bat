@echo off
setlocal
REM ! preceeding a flag indicates it is used internally only
REM _ preceeding a flag indicates it is a reportable option

REM ********************************************
REM  BUILD96P - runs every day at 6:00 AM
REM ********************************************
set _TEMPLATE=BUILD96P.BAT
set !TRIDENT=1
rem=============================================================
rem _F3DIR             Directory of forms3 SLM project.
rem                    Override here if you are using frozen source.
set _F3DIR=D:\forms3

rem _F3QADIR           Directory of f3qa SLM project.
rem=============================================================
rem !ProjSize          !ProjSize is the amount of free space to require
rem                    in the destination dir.  If not enough space is
rem                    free, the drop directory will be abandoned in
rem                    favor of the root of the first local drive that
rem                    has at least !ProjSize bytes free.
set !ProjSize=200000000
rem=============================================================
rem _RELEASE_CANDIDATE 0=No  1=Yes
rem                    This removes the words UNPUBLISHED WORK and
rem                    sets the version to 3.0
rem=============================================================
rem _DESTROOT          Where files are dropped.  A UNC is ok here.
rem                    The default is the current machine's f3drop share.
set _DESTROOT=\\trigger\trident
rem=============================================================
rem _OVERRIDE          0=No  1=Yes  This forces immediate SLM override on ssync
set _OVERRIDE=1
rem=============================================================
rem _UPDATEVER         0=No  1=Yes  This calculates a version based on
rem                    the date and the current version.h.  If you do
rem                    not want to update the project version, set it to 0.
rem                    If the drop directory still exists, a new directory
rem                    will be created with the .NEW extension.  The version
rem                    format is:
rem                    8.bbbb.c   where bbbb is MM/DD, Jan '94 = 01
rem                    2=Increase minor version (XX.XXXX.+1)
set _UPDATEVER=1
rem=============================================================
rem _DROPNAME          Leave blank to calculate from version.h, or
rem                    fill in to specify otherwise.  Calculated
rem                    format will be:   DYbbbb[_c] where
rem                    'c' is only included if it is nonzero.
set _DROPNAME=
rem=============================================================
rem _DROPSRCTOOLS      0=No  1=Yes  Drop \src and \tools
set _DROPSRCTOOLS=1
rem=============================================================
rem _SSYNC             0=No  1=Yes  SLM command SSYNC -a is executed
set _SSYNC=1
rem=============================================================
rem _TEST              0=No  1=Yes  Echo on and leave shell windows open.
set _TEST=0
rem=============================================================
rem _ALLOWMULTI        0=No  1=Yes  Don't stop if another build is
rem                    detected to be in progress (set to Yes if build
rem                    is scheduled to assure it will always run, or
rem                    set to No to assure it won't stomp on what you
rem                    are doing interactively.
set _ALLOWMULTI=1
rem=============================================================
rem _MAXRUN            Run this many at a time, set to # of processors
set _MAXRUN=2
rem=============================================================
rem                    0=Don't Make 1-9 Priority  (9 highest)
rem [W|M|A][D|S|P|DA|C][6|6P|6V]  (Win, Mac, Alpha) (Debug, Ship, Profile, Debug All, Code coverage) (96, 96P, 96 Viaduct)
set WD6P=9
set WS6P=9
SET WP6P=0
rem=============================================================
REM On the following options, you can use the prefixes below
REM to override options on individual makes:
rem [W|M|A][D|S|P|DA|C][6|6P|6V]  (Win, Mac, Alpha) (Debug, Ship, Profile, Debug All, Code coverage) (96, 96P, 96 Viaduct)
REM
REM   example:   WD6_MAKEFLAGS= fall
rem=============================================================
rem _MAKEFLAGS         ffresh  or  fall
set _MAKEFLAGS=ffresh _RELEASE 2
rem=============================================================
rem _CLEANMAKE         0=No  1=Yes  Delete the build tree before beginning
set _CLEANMAKE=1
rem=============================================================
rem _MAIL              0=No  1=Yes  Send result mail
set _MAIL=1
rem=============================================================
rem _SENDPASS           Groups to send final results to
rem                     /T = 'To'     /C = 'Cc'    /B = 'Bcc'
set _SENDPASS=/T trilead /T nashbld /T chrisvau /T a-willr
rem=============================================================
rem _SENDFAIL           Groups to send make failures to
rem                     /T = 'To'     /C = 'Cc'    /B = 'Bcc'
set _SENDFAIL=/T ie4slm /T nashbld /T chrisvau  /T a-willr
rem=============================================================
rem _DROP              0=No  1=Yes Drop the product (and update BUILDDB)
set _DROP=1
rem=============================================================
rem _TP3               0=No  1=Yes (send TP/3 triggers)
set _TP3=0
REM _TP3 by itself selects whether to clean up \\f3qa

SET WD6P_TP3=1
SET WS6P_TP3=1
SET WP6P_TP3=0
rem=============================================================
rem _RELEASE           0=No  1=Yes
set _RELEASE=2
rem=============================================================

call %_F3QADIR%\BUILD\BUILD.BAT
