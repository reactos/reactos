/************************************************************************

NAME

    recorder.h

DESCRIPTION


CAUTIONS

    ---

AUTHOR

    Steve Squires

HISTORY

    By		Date		Description
    --------	----------	-----------------------------------------
    sds		6 Dec 88	Initial Coding
		14 Dec		new string globals
		19 Dec		now using PMACRO typedef
		20 Dec		added DLGTRACEROK
		22 Dec 		added ListKeyCodes
		30 Dec		added cxScreen, cyScreen
    sds		3 Jan 89	added szWinOldApp, other strings load
				  at load time for speed
				improved dlg control id's
				created init segment
		5 Jan		dlgProfile is now dlgPref
		10 Jan		added lpfnEnum
    sds		13 Jan		new id's for new record/suspend dlg box
		1 Feb		added sysfontface, size
		1 March		removed sysfontface, size support
		8 Mar		added DLGCMDS, IDCMDS
		14 Mar		added ForceTrActivate
				deleted TIMERID, added wTimerID, lpfnWndProc
		15 Mar		DefFlags now a WORD
		16 Mar 89	new error reporting stuff
		25 Mar 1989	szErrResource
		28 Mar 1989	szTempDir
		30 Mar 1989	szNoMem, szError, szWarn, LdStr change
		5 Apr 1989	bRecording
		26 Apr 1989	JPActivate prototype
		27 Apr 1989	fBreakType, szDeskInt
		5 May 1989	szDeskInt deleted
		9 May 1989	KeyWndProc, hWndKey
		13 May 1989	removed some unused #define's
		16 May 1989	added IsProtMode, bProtMode
		18 May 1989	OpenIt, SaveIt
		18 May 1989	massive user intfc changes
		22 May 1989	cmd line from new tracer instance
		24 May 1989	REMOTECTL stuff
		26 May 1989	WIN2 upgrade
		30 May 1989	OpenIt change
    sds		31 May 1989	changed help menu id's
    sds		7 Jun 1989	DeleteMacro
    sds		9 Jun 1989	name change to recorder
    sds		12 Jun 1989	bProtMode now bNotBanked, bNoFlash, WIN2
    sds		3 Jul 1989	FixSysMenu now WIN2 only
		27 Jul 1989	added szDotHlp, szPlayback, szRecord
    sds		28 Jul 1989	TM_BREAK for REMOTECTL
    sds		7 Aug 1989	PeekABoo
    sds		17 Aug 1989	fBreaktype -> WORD
				SaveIt now BOOL
    sds		29 Aug 1989	HELP menu changes
    sds		18 Sep 1989	removed IsBanked prototype, added bSuspended
    sds		20 Sep 1989	WM_MINPLAYBACK
    sds		31 Oct 1989	FreeMacro changed
    sds		26 Dec 1989	CheckComment
    sds		8 Jan 1990	updated copyright
    sds 	8 Feb 1990	Win2.1 compatibility
    t-carlh	15 Aug 1990	Added OS/2 long filename support
    sds 	15 Mar 1990	Win2.1 compatibility
    sds 	26 Oct 1990	bBreakCheck
    sds 	7 Jan 1991	upgraded prototypes to keep C6 happy
*/

/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge Ltd.				*/
/* -------------------------------------------------------------------- */

#define WM_POSTERROR WM_USER+50
#define WM_MINPLAYBACK WM_USER+51


/* -------------------------------------------------------------------- */
/* Added conditional compilation for long filename support under OS/2	*/
/* t-carlh - August, 1990						*/
/* -------------------------------------------------------------------- */
#ifdef OS2
#define CCHSCRATCH 260	/* Maximum OS/2 long filename length */
#else
#define CCHSCRATCH 255	/* Maximum DOS filename length??? */
#endif

#define FLASHTIMER 500	/* icon flash interval during recording - msec */

/* resource id's */
#define RMENU 2
#define RKEYS 4

/* menu items */

#define ID_MERGE    9
#define ID_NEW	    10
#define ID_OPEN    11
#define ID_SAVE    12
#define ID_SAVEAS  13
#define ID_EXIT    14
#define ID_ABOUT   15

#define ID_RECORD  16
#define ID_PLAYBACK 17
#define ID_DELETE   18
#define ID_EDIT	    20

#define ID_HOTKEY   21
#define ID_PROFILE  22
#define ID_BREAK    23
#define ID_MINIMIZE 24

#define ID_CMDLINE  40	/* for msg from new recorder.exe */

#define ID_INDEX    0xffff
#define ID_KEYBOARD 0x1e
#define ID_COMMANDS 0x20
#define ID_PROCEDURES 0x21
#define ID_USEHELP  0xfffc

#define ID_LISTBOX  60

/* dialog boxes */
#define DLGOPEN 100
#define DLGSAVE 101
#define DLGRECORD 102
#define DLGMOUSE 103
#define DLGPLAYOPT 104
#define DLGSTART 105
#define DLGPLAY 106
#define DLGSPEED 107
#define DLGABOUT 108
#define DLGEDIT 109
#define DLGPICK 110
#define DLGMERGE 111
#define DLGPREF 112
#define DLGDESC	113
#define DLGPBERROR 114
#define DLGRECORDEROK 115
#define DLGINT	    116
#define DLGCMDS	117

#define IDCHECK 11
#define IDEDIT 12
#define IDEDIT1 13
#define IDEDIT2 14
#define IDLIST 15
#define IDDIRLIST 16
#define IDTEXT 17

#define IDDESC	 18
#define IDHOTKEY 19

#define IDSHIFT 20
#define IDCONTROL 21
#define IDALT 22

#define IDRECSPEED 36
#define IDLOOP	37

#define IDPLAYTO    40
#define IDWINRELATIVE 41
#define IDMOUSEOPT  42

#define IDSAVE	45
#define IDRESUME 46
#define IDTRASH 47
#define IDCMDS 48

/* MUST be consecutive and in this order */
#define CMSGTEXT    11	/* # of strings */
#define MSG_LDOWN   500
#define MSG_MDOWN   501
#define MSG_RDOWN   502
#define MSG_LUP	    503
#define MSG_MUP	    504
#define MSG_RUP	    505
#define MSG_MOVE    506
#define MSG_KDOWN   507
#define MSG_KUP	    508
#define MSG_SKDOWN  509
#define MSG_SKUP    510

#ifdef REMOTECTL
#define TM_OpenFile 1
#define TM_PLAY 2
#define TM_RECORD 3
#define TM_STOPRECORD 4
#define TM_SAVEAS 5
#define TM_NEW 6
#define TM_BREAK 7
#endif


struct pberror {
    WORD ecode;		/* error code */
    WORD offset;	/* of bad instruction */
    BYTE id;		/* of playback macro */
};

/* character constants */
#define CHDIRSEPARATOR	'\\'
#define CHDRIVESEPARATOR ':'
#define CHFILEEXTSEPARATOR '.'
#define CHWILDFILE '*'
#define CHWILDCHAR '?'

#ifdef NTPORT
HWND GetForegroundWindow( VOID );
#endif

/* strings */
extern CHAR szRecorder[];	/* window class name, WIN.INI, little else */
extern CHAR szPlayback[];	/* for WIN.INI only */
extern CHAR szRecord[]; 	/* for WIN.INI only*/

extern CHAR szScratch[];    /* scratch buffer CCHSCRATCH bytes long */
extern CHAR szNull[];
extern CHAR szDirSeparator[];
extern CHAR *szStarExt;    /* *.tr */
extern CHAR *szUntitled;    /* for caption if no file open */
CHAR *rgszMsg[CMSGTEXT];    /* strings of msg event types */
extern CHAR *szMouseCmd;    /* sprintf template for mouse cmd */
extern CHAR *szKeyCmd;	    /* sprintf template for key cmd */
extern CHAR *szTempFile;    /* trXXXXXX for mktemp function */
extern CHAR *szTempDir;     /* system TEMP directory */
extern CHAR *szCaption;     /* app name "Recorder" from .RC file */
extern CHAR *szDlgInt;	    /* #int class name */
extern CHAR *szDialogBox;   /* class name for display */
#ifdef WIN2
extern CHAR *szMsgInt;	    /* #int class name */
extern CHAR *szMessageBox;  /* class name for display */
extern CHAR *szWin200;	    /* real module name for desktop and MSDOS */
extern CHAR *szMSDOS;	    /* pretty name for MSDOS window */
#endif
extern CHAR *szWindows;     /* for pretty Windows module name */
extern CHAR *szDesktop;     /* class name of background desktop */
extern CHAR *szWinOldApp;   /* fake module name for old apps */
extern CHAR *szAlt;
extern CHAR *szControl;
extern CHAR *szShift;
extern CHAR *szScreen;
extern CHAR *szWindow;
extern CHAR *szNothing;
extern CHAR *szEverything;
extern CHAR *szClickDrag;
extern CHAR *szAnyWindow;
extern CHAR *szSameWindow;
extern CHAR *szFast;
extern CHAR *szRecSpeed;
extern CHAR *szErrResource;
extern CHAR *szNoMem;
extern CHAR *szError;
extern CHAR *szWarning;
extern CHAR *szDotHlp;

extern CHAR *szErrArg;	    /* arg to ErrMsgArg */

extern CHAR *szFileName;    /* current file name (NULL if none) */
extern OFSTRUCT ofCurrent;  /* for current file */

#ifdef NTPORT
extern INT hFileIn;	    /* -1 if closed */
extern INT hFileOut;	    /* -1 if closed */
#else
extern HANDLE hFileIn;	    /* -1 if closed */
extern HANDLE hFileOut;     /* -1 if closed */
#endif

extern BOOL bRecording;
extern WORD fBreakType;
extern BOOL bFileChanged;
extern PMACRO pmFirst;
extern PMACRO pmNew;
extern PMACRO pmEdit;
extern FARPROC lpfnEnum;    /* for EnumProc */
#ifdef REMOTECTL
extern WORD wm_bridge;
extern BOOL bNoFlash;
#endif

extern HWND hWndRecorder;
extern HWND hWndKey;
extern HWND hWndPrevActive; /* active window before Recorder was activated */
extern HWND hWndList;	    /* in client area */
extern HANDLE hMenuRecorder;
extern HANDLE hInstRecorder;
extern HANDLE hCurWait;	    /* hourglass */
extern HANDLE hCurNormal;
extern WORD cxScreen;
extern WORD cyScreen;

extern FARPROC lpfnWndProc;    /* for SetTimer */
extern WORD wTimerID;

extern BYTE DefMouseMode;    /* user profile */
extern BOOL bMinimize;	    /* TRUE to minimize on playback */
extern WORD DefFlags;
extern struct pberror pbe;  /* playback error info */

extern BYTE MouseMode;	    /* record mode for mouse */
extern BOOL bHotKeys;	    /* FALSE if disabled */
extern BOOL bCenterDlg;	    /* TRUE to center dlgAbout */
extern BOOL bNotBanked;	    /* TRUE if no EMS banking to worry about */
extern BOOL bSuspended;     /* TRUE if recording suspended */
extern BOOL bBreakCheck;    /* true if checking for break */

/* function prototypes */
VOID AddMacro(PMACRO pmN);
BOOL CheckComment(PMACRO pmEdit,CHAR *szComment);
BOOL CheckHotKey(BYTE key,WORD flags,PMACRO pmEdit);

#ifdef WIN2
BOOL SBComboRegister(HANDLE hInst, LPSTR lpszClassName, LPSTR lpszListBox,
 LPSTR lpszEdit);
#ifdef NTPORT
BOOL APIENTRY SBListRegister(HANDLE hInst, LPSTR pszClassName);
#else
BOOL FAR PASCAL SBListRegister(HANDLE hInst, LPSTR pszClassName);
#endif
#endif

VOID FAR CreateTime(HWND);
VOID DeleteMacro(PMACRO pm);
VOID FAR DoCaption (VOID);
VOID DoCmdLine(LPSTR lpszCmdLine);
INT DoDlg(HWND,FARPROC,WORD);
BOOL APIENTRY EnumProc(HWND hWnd,LONG lParam);
VOID ErrMsg(HWND hWnd,WORD ecode);
VOID ErrMsgArg(HWND hWnd,WORD ecode,CHAR *sz);
VOID ForceRecActivate(BOOL);
VOID FreeFile(VOID);
VOID FreeMacro(PMACRO pm,BOOL bDelHotKey);
VOID FreeMacroList(VOID);
UCHAR GetKeyCode(CHAR *psz);
VOID GetKeyString(UCHAR key,CHAR *psz);
BOOL HasWildCard(register CHAR *sz);
VOID HotKeyString(PMACRO pm,BOOL bWithComment);

#ifdef NTPORT
VOID JPActivate(LONG,HANDLE);
#else
VOID JPActivate(LONG,INT);
#endif

LONG APIENTRY KeyWndProc(HWND,WORD,WPARAM,LONG);
VOID FAR LdStr(HWND,WORD);
VOID ListKeyCodes(HWND hWnd);
INT LoadDesc(PMACRO pm);
HICON FAR LoadFileIcon( CHAR *, BOOL * );
INT LoadMacro(PMACRO pm);
VOID LoadMacroList(VOID);
INT APIENTRY lstrncmpi( LPSTR, LPSTR, INT );
LPSTR APIENTRY lstrcpy( LPSTR, LPSTR );
INT  APIENTRY lstrlen( LPSTR );
INT FAR MainLoop(VOID);
VOID NotImp(VOID);
BOOL OpenIt(HWND hWnd,LPSTR lpszFile);
VOID PeekABoo(VOID);
BOOL PlayHotKey(BYTE key,BYTE flags);
VOID PlayStart(PMACRO pm,BOOL bOnHotKey);
VOID PlayStop(VOID);

#ifdef WIN32
// temporary quick fixes
#define POFSTRUCT LPOFSTRUCT
#endif

INT RecReadFile(CHAR *szFile,POFSTRUCT pof);
VOID RecordBreak(VOID);
VOID RecorderMsg(WORD ecode);
VOID RecordStart(VOID);
VOID RecordStop(BOOL bKeepIt);
VOID ReleaseCode(PMACRO pm);
VOID ReleaseDesc(PMACRO pm);
BOOL SaveIt(HWND hWnd,LPSTR lpszFile,BOOL bOverWrite);
VOID ShutDown(VOID);
VOID WarnMsg(HWND hWnd,WORD ecode);
LONG APIENTRY WndProc(HWND,WORD,WPARAM,LONG);
VOID WriteDone(VOID);
INT RecWriteFile(CHAR *szName);

#ifdef WIN2
VOID FixSysMenu(HWND hDlg);
BOOL WarnEm(VOID);
#endif

VOID *AllocLocal(WORD wSize);
VOID FreeLocal(VOID *p);

#if 0
VOID *ReallocLocal(VOID *p,WORD newsize);

/* LONG file access routines  */
INT  APIENTRY lopen( LPSTR, INT );
WORD APIENTRY lread( INT, LPSTR, INT );
WORD APIENTRY lwrite( INT, LPSTR, INT );
#endif

/* dialog boxes */
BOOL APIENTRY dlgAbout	  (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY dlgCmds	  (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY dlgEdit	  (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY dlgInterrupt(HWND, WORD, WPARAM, LONG);
BOOL APIENTRY dlgMerge	  (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY dlgOpen	  (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY dlgPBError  (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY dlgPref	  (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY dlgRecord   (HWND, WORD, WPARAM, LONG);
BOOL APIENTRY dlgSave	  (HWND, WORD, WPARAM, LONG);

#ifdef REMOTECTL
VOID RemoteRegister(VOID);
INT RemoteWndProc(WORD message,WPARAM wParam, LONG lParam, BOOL *pb);
#endif
/* ------------------------------ EOF --------------------------------- */
