/***************************************************************************

NAME

    rdlocal.h -- for recordll only

DESCRIPTION


CAUTIONS

    ---

AUTHOR

    Steve Squires

HISTORY

    By		Date		Description
    --------	----------	-----------------------------------------
    sds		21 Nov 88	Initial Coding
		14 Dec		added bStartUp
		15 Dec		RecFlags now a word
		30 Dec		added screen size support
		3 Jan 89	LastDown now global
		6 Jan		renamed to bHaveMouseMsg
		12 Jan		changed some BOOL's to BBOOL
		3 Apr 1989	Heuristics now has return code
		16 May 1989	bProtMode
		9 Jun 1989	name change to recorder
    sds		12 Jun 1989	bProtMode now bNotBanked
    sds		17 Aug 1989	added id to hotkey struct
    sds 	8 Jan 1990	updated copyright
    sds 	27 Nov 1990	sysmsg
    sds 	7 Jan 1991	prototype cleanups for C6

*/

/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge Ltd.				*/
/* -------------------------------------------------------------------- */

struct hotkey {
    BYTE key;
    BYTE flags;
    BYTE id;
};

struct btimes {
    DWORD lLeft;
    DWORD lMiddle;
    DWORD lRight;
};

FARPROC APIENTRY JRHook(INT,WPARAM,LPEVENTMSGMSG);
FARPROC APIENTRY KHook(INT,WPARAM,LONG);
VOID FreeStringList(VOID);
INT NEAR Heuristics(VOID);

extern HWND hWndRecorder;
extern struct hotkey *phBase;
extern INT cHotKeys;
extern BBOOL bSuspend;
extern BBOOL bNoHotKeys;
extern BBOOL bBreakCheck;
extern CHAR cButtonsDown;	/* number of buttons down */
extern LONG StartTime;
extern FARPROC lpOldKHook;
extern FARPROC lpOldJRHook;
extern DWORD DblClickTime;
extern struct btimes LastDown;	/* time of last button down msg */

/* for record mode */
extern EVENTMSG sysmsg; 	/* most recent EVENTMSG recorded */
extern struct stringlist *psFirst;	/* window names during record */
extern INT MouseMode;
extern BBOOL bRecording;
extern BBOOL bStartUp;		/* TRUE until have first worthwhile msg */
extern BBOOL bHaveMouseMsg;	/* TRUE if macro has mouse msgs */
extern BBOOL bNotBanked;	/* TRUE if no EMS banking */
extern WORD cxScreen;
extern WORD cyScreen;
extern WORD cMsgs;
extern HANDLE hMemTrace;
extern WORD RecFlags;
/* ------------------------------ EOF --------------------------------- */
