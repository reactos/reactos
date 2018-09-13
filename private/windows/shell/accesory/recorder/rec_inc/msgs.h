/*************************************************************************

NAME

    msgs.h

DESCRIPTION


CAUTIONS

    ---

AUTHOR

    Steve Squires

HISTORY

    By		Date		Description
    --------	----------	-----------------------------------------
    sds		6 Dec 88	Initial Coding
    sds		22 Dec 88	adapted for win30
    sds		30 Dec		added screen size msgs
    sds		3 Jan 89	Added winoldapp
		4 Jan 89	added E_WINONTOP
				removed unused M_'s
		12 Jan		added badwinversion
		1 Feb		added M_FONT, M_WRONGFONT
		1 March		removed M_FONT, M_WRONGFONT
		16 Mar		added M_HOTSWITCH, E_BADCMDLINE, etc
		25 Mar 1989	ERRRESOURCE
		27 Mar 1989	JRSYSMODAL, JPSYSMODAL
		28 Mar 1989	M_TEMPDIR
		29 Mar 1989	M_REPLACEFILE, M_SWITCH, E_NOFILE
		31 Mar 1989	renumbered E_NOFILE, added E_BADFILENAME
		27 Apr 1989	M_SYSMODAL, M_DESKINT
		5 May 1989	removed M_DESKINT
		19 May 1989	changes for new user interface
		9 Jun 1989	name change to recorder
		12 Jun 1989	WIN2'ized some stuff
		27 Jul 1989	added M_DOTHLP, removed M_RECORD/PLAYBACK
		17 Aug 1989	M_TOOLONG
    sds		19 Sep 1989	removed unused msgs
    sds		21 Sep 1989	M_PBERECORD, M_BREAKRECORD
    sds		16 Oct 1989	M_DELETEEMPTY
    sds		28 Nov 1989	E_NOTRECORDERFILE
    sds		26 Dec 1989	E_DUPCOMMENT, M_DUPCOMMENT
    sds		8 Jan 1990	updated copyright
*/

/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge Ltd.				*/
/* -------------------------------------------------------------------- */

/* message strings */
#define M_ERROR		1000
#define M_JRBREAK	1001
#define M_BADWINVERSION	1002
#define M_CAPTION	1003
#define M_WARNING	1004
#define M_DELETEQ	1005
#define M_NOEVENTS	1006
#define M_BREAKRECORD	1007
#define M_DELETEEMPTY	1008
#define M_DUPCOMMENT	1009
#define M_SAVECHANGES	1011
#define M_SAMEWINDOW	1012
#define M_ANYWINDOW	1013
#define M_FAST		1014
#define M_RECSPEED	1015
#define M_EVERYTHING	1016
#define M_CLICKDRAG	1017
#define M_WINDOW	1019
#define M_SCREEN	1020
#define M_UNTITLED	1021
#define M_CONTROL	1022
#define M_SHIFT		1023
#define M_ALT		1024
#define M_JPABORT	1025
#define M_NOTHING	1026
#define M_DOTHLP	1027
#define M_CLEAREDDUPHOT 1033
#define M_MERGECLEARED	1034
#define M_MOUSECMD	1035
#define M_KEYCMD	1036
#define M_TEMPFILE	1037
#define M_STAREXT	1038
#define M_DELMACRO	1039
#define M_DELETE	1040
#define M_PBERECORD	1041
#define M_DLGINT	1042
#define M_DLGBOX	1043
#define M_WINDOWS	1047
#define M_DESKTOP	1048
#define M_WRONGSCREEN	1050
#define M_RIGHTSCREEN	1051
#define M_NOMOUSE	1052
#define M_WINOLDAPP	1053
#define M_HOTSWITCH	1054
#define M_TEMPDIR	1055
#define M_REPLACEFILE	1056
#define M_CMDSYNTAX	1057
#define M_SWITCH	1058
#define M_SYSMODAL	1059
#define M_TOOLONG	1060
#define M_EXTRAINFO	1061
#ifdef WIN2
#define M_MSGINT	1044
#define M_MSGBOX	1045
#define M_MSDOS		1046
#define M_WIN200	1049
#endif

#define E_NOMEM		2000
#define E_BADVERSION	2006
#define E_TOOMANYHOT	2007
#define E_WRONGTARGET	2008
#define E_NOPBWIN	2009
#define E_NESTED	2010	/* don't need resource string */
#define E_TOONESTED	2011
#define E_BADMACRO	2012
#define E_TOOLONG	2013
#define E_BADHOTKEY	2014
#define E_DUPHOTKEY	2015
#define E_OUTOFBOUNDS 	2016
#define E_KEYORCOMMENT	2017
#define E_WINONTOP	2018
#define E_BADTDYN	2019
#define E_PBWININVISIBLE 2020
#define E_ERRRESOURCE	2021
#define E_JRSYSMODAL	2022
#define E_JPSYSMODAL	2023
#define E_BADFILENAME	2024

/* errors over 3000 take an argument - ErrMsgArg() depends on it */
#define E_BOGUSHOTKEY	3000
#define E_CANTCREATEFILE 3001
#define E_WRITE		3002
#define E_CANTOPEN	3003
#define E_READ		3004
#define E_HKNOFILE	3005
#define E_BADCMDLINE	3006
#define E_BADFILE	3007
#define E_NOFILENOHOT	3008
#define E_NOFILE	3009
#define E_NOTRECORDERFILE 3010
#define E_DUPCOMMENT	3011
/* ------------------------------ EOF --------------------------------- */
