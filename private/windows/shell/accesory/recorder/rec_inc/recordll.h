/************************************************************************

NAME

    recordll.h -- shared between recorder and its dll

DESCRIPTION


CAUTIONS

    ---

AUTHOR

    Steve Squires

HISTORY

    By		Date		Description
    --------	----------	-----------------------------------------
    sds		6 Dec 88	Initial Coding
		15 Dec		flags byte now a word, added B_CAPS,
				SCROLL, NUMLOCK
		19 Dec		cMsgs is now in paramL of first msg
		30 Dec		added screen size to macro hdr
				added closetdyn
    sds		6 Jan 89	moved VERSION here
    sds		9 Jan 89	lmemcpy is now void far *
    sds		12 Jan 89	added BBOOL
    sds		1 Feb 89	added font name and size to filemacro struct
				bumped version to 0005
		2 Feb		changed font face storage
		1 March		font/face stuff removed
				version 0006
    sds		6 Apr 1989	RecordEvent now private to tdyn
				separate versions for tdyn and file
				ResumeRecord(void) now SuspendRecord(BOOL)
    sds		26 Apr 1989	B_FORCEACTIVE, B_RESTORE
    sds		5 May 1989	DESKTOPMODULE
    sds		16 May 1989	InitTDyn change, renamed from tdyn.h
    sds		23 May 1989	WM_PBDONE
    sds		9 Jun 1989	name change to recorder
    sds		28 Jul 1989	BreakPlayback for REMOTECTL
    sds		8 Aug 1989	MAXWAIT moved here
    sds		17 Aug 1989	TVERSION now 8, AddHotKey change
    sds 	8 Jan 1990	extended MAXWAIT from 1 second to 10
    sds 	26 Oct 1990	new arg to StopPlayback
*/

/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge Ltd.				*/
/* -------------------------------------------------------------------- */

#ifdef NTPORT

#ifndef CHAR
typedef char	       CHAR;
#endif

#ifndef UCHAR
typedef unsigned char  UCHAR;
#endif

#ifndef UINT
typedef unsigned int   UINT;
#endif

#endif


#define FVERSION 0x0006 	/* for file */
#define TVERSION 0x0008		/* for tdyn */
#define CCHCOMMENT 40		/* size of macro comment field */
#define VK_SCROLLLOCK 0x91	/* not in windows.h */

#define DESKTOPMODULE 0xd	/* funky module!desktop substitute */

typedef FARPROC FAR *LPFARPROC;
typedef CHAR BBOOL;

#define WM_PBLOST WM_USER+2
#define WM_REC_HOTKEY WM_USER+3
#define WM_JPACTIVATE WM_USER+4
#define WM_JPABORT  WM_USER+5
#define WM_JRBREAK WM_USER+6
#define WM_JRABORT WM_USER+7
#define WM_PBDONE WM_USER+8

#define CEVENTALLOC	25  /* allocate mem in chunks this size */
#define MAXWAIT (180*55)    /* max 10-sec timeout waiting for Recorder proper
			      - also implicitly is timeout waiting for
			      PB window to appear */

/* mouse record mode */
#define RM_DRAGS    0
#define RM_EVERYTHING 1
#define RM_NOTHING  2

/* for flags word */
#define B_SHIFT 	  1
#define B_CONTROL	  2
#define B_ALT		  4
#define B_SHFT_CONT_ALT   (B_SHIFT|B_CONTROL|B_ALT)

#define B_WINRELATIVE	8  /* else screen relative */
#define B_PLAYTOWINDOW	0x10 /* else play to anyone */
#define B_RECSPEED	0x20  /* else full blast */
#define B_NESTS 	0x40	/* playback can nest into others */
#define B_LOOP		0x80	/* autoloop */
#define B_CAPSLOCK	0x100	/* 1 if down when record began */
#define B_NUMLOCK	0x200	/* ditto */
#define B_SCRLOCK	0x400	/* ditto */

#ifdef NTPORT
typedef struct recordermsg {
    WORD message;
    WORD paramL;
    WORD paramH;
    WORD time;
    union {
	UINT szWindow;		/* offset into global playback segment */
	struct stringlist *ps;	/* contents during record mode */
    } u;
} RECORDERMSG;
#else
typedef struct recordermsg {
    WORD message;
    WORD paramL;
    WORD paramH;
    WORD time;
    union {
	CHAR *szWindow; 	/* offset into global playback segment */
	struct stringlist *ps;	/* contents during record mode */
    } u;
} RECORDERMSG;
#endif

/* first RECORDERMSG in playback block is for control.
    message is MAKEWORD(speed,flags)
*/
#define B_FORCEACTIVE 0x8000	/* OR'ed into message field on
				buttondown following trashed ALT/ESC/TAB */
#define B_RESTORE 0x4000	/* OR'ed in if should restore it too */

typedef RECORDERMSG FAR *LPRECORDERMSG;

/* macro structure in file */
struct filemacro {
    BYTE hotkey;
    WORD flags;		/* see bits above */
    WORD cchCode;
    LONG lPosCode;	/* 0 if not avail from file */
    WORD cchDesc;
    LONG lPosDesc;	/* 0 if not avail from file */
    WORD cxScreen;	/* screen size at record time, 0 if no mouse msgs */
    WORD cyScreen;
    CHAR szComment[CCHCOMMENT + 1];
};

/* macro structure in memory */
struct macro {
    struct macro *pmNext;
    HANDLE hMemCode;	/* zero if code not loaded */
    HANDLE hMemDesc;	/* NULL if none */
    struct filemacro f;
    LONG lPosCodeT;	/* offset of code in output file */
    LONG lPosDescT;	/* offset of description in output file */
    BYTE id;		/* of playback */
};
typedef struct macro *PMACRO;

struct stringlist {
    struct stringlist *psNext;
    CHAR sz[1];
};

/* functions in DLL */
BOOL APIENTRY AddHotKey(BYTE key,WORD flags,BYTE id);
#ifdef REMOTECTL
VOID APIENTRY BreakPlayback(VOID);
#endif
VOID APIENTRY CloseRDLL(VOID);
BOOL APIENTRY DelHotKey(BYTE key,WORD flags);
VOID APIENTRY EnableBreak(BOOL b);
VOID APIENTRY EnaHotKeys(BOOL b);
UINT APIENTRY InitRDLL(HWND hWnd,BOOL bProtected);

#ifdef NTPORT
LPVOID lmemcpy(LPVOID, LPVOID, INT);
#else
VOID APIENTRY lmemcpy(LPVOID,LPVOID,INT);
#endif

VOID APIENTRY SetPBWindow(HWND hWnd);
INT APIENTRY StartPlayback(HANDLE,BYTE);
VOID APIENTRY StopPlayback(BOOL,BOOL);
VOID APIENTRY StartRecord(HANDLE h,INT mouMode,WORD recflags);
INT APIENTRY StopRecord(struct macro FAR *, BOOL);
VOID APIENTRY SuspendRecord(BOOL b);
VOID APIENTRY Wait(BOOL);

/* ------------------------------ EOF --------------------------------- */
