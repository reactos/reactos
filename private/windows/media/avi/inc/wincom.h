/****************************************************************************
 *
 *	WINCOM.H
 *
 *	Exported definitions for MM/Windows common code library.
 *
 ****************************************************************************/

#ifndef _WINCOM_H_
#define _WINCOM_H_


/*************************************
 *
 *	OPEN FILE DIALOG BOX
 *
 *************************************/

int FAR PASCAL OpenFileDialog(HWND hwndParent, LPSTR lpszTitle,
				LPSTR lpszExtension, WORD wFlags,
				LPSTR lpszFileBuf, WORD wFileLen );

/*  Flags for OpenFileDialog  */
#define DLGOPEN_MUSTEXIST	0x0001
#define DLGOPEN_NOSHOWSPEC	0x0002
#define DLGOPEN_SAVE		0x0004
#define DLGOPEN_OPEN		0x0008
#define DLGOPEN_SHOWDEFAULT	0x0020
#define DLGOPEN_NOBEEPS		0x0040
#define DLGOPEN_SEARCHPATH	0x0080

/*  Return codes  */
#define DLG_CANCEL	0
#define DLG_MISSINGFILE	-1
#define DLG_OKFILE	1


/**************************************************
 *
 *  DEFAULT FILE OPEN DIALOG PROCEDURE STUFF
 *
 **************************************************/

BOOL FAR PASCAL DefDlgOpenProc(HWND hwnd, unsigned msg,
				WORD wParam, LONG lParam);

typedef struct _DlgOpenCreate {
	LPSTR	lpszTitle;	// NULL if use default dialog caption
	LPSTR	lpszExt;	// NULL defaults to *.*
	LPSTR	lpszBuf;	// final filename buffer
	WORD	wBufLen;	// length of this buffer
	WORD	wFlags;		// DLGOPEN_xxx flags
	DWORD	dwExtra;	// for use by the owner of the dialog
} DlgOpenCreate;
typedef DlgOpenCreate FAR *FPDlgOpenCreate;

#define SetDialogReturn(hwnd, val) SetWindowLong(hwnd, 0, val)

/*  These messages are sent to/from DefDlgOpenProc to make the
 *  Dialog box work.  They may be answered by the "owner" of the dialog
 *  box.
 */
#define DLGOPEN_OKTOCLOSE	(WM_USER + 1)	// ask whether ok to close box
#define DLGOPEN_CLOSEBOX	(WM_USER + 2)	// tell dlgOpen to close box
#define DLGOPEN_SETEXTEN	(WM_USER + 3)	// sets default extensions
// #define DLGOPEN_SETEDITTEXT	(WM_USER + 4)	// sets editbox contents
// #define DLGOPEN_REFRESH		(WM_USER + 5)	// refresh the box?
#define DLGOPEN_CHANGED		(WM_USER + 6)	// edit box has been changed
#define DLGOPEN_RESETDIR	(WM_USER + 7)	// directory change

BOOL FAR PASCAL IconDirBox(HWND hwnd, WORD wId, unsigned msg,
				WORD wParam, LONG lParam);
BOOL FAR PASCAL IconDirBoxFixup(HWND hwndDirbox);


/*
 *  HUGE READ/WRITE functions
 *
 */
typedef char huge * HPSTR;

// LONG FAR PASCAL _hread( int hFile, HPSTR hpBuffer, DWORD dwBytes ); 
// LONG FAR PASCAL _hwrite( int hFile, HPSTR hpBuffer, DWORD dwBytes ); 


/*
 *  FAR STRING FUNCTIONS 
 */
LPSTR FAR PASCAL lstrncpy(LPSTR dest, LPSTR source, WORD count);
LPSTR FAR PASCAL lstrncat(LPSTR dest, LPSTR source, WORD count);
int   FAR PASCAL lstrncmp(LPSTR d, LPSTR s, WORD n);

/*
 *  FAR MEMORY FUNCTIONS 
 */
void FAR * FAR PASCAL lmemcpy(LPSTR dest, LPSTR source, WORD count);
void FAR * FAR PASCAL hmemmove(HPSTR dest, HPSTR source, LONG count);
HANDLE FAR PASCAL CopyHandle(WORD wFlags, HANDLE h);

void FAR PASCAL fmemfill (LPSTR lpMem, DWORD count, BYTE bFill);

/*
 *  BYTE SWAPPING FUNCTIONS 
 */
WORD FAR PASCAL ByteSwapWORD( WORD w );
DWORD FAR PASCAL ByteSwapDWORD( DWORD dw );


/*
 * atol for far strings
 */
LONG FAR PASCAL StringToLong( LPSTR lpstr );


/*
 *  PATH PARSING FUNCTIONS
 */
BOOL FAR PASCAL AddExtension(LPSTR lpszPath, LPSTR lpszExt, WORD wBufLen);
WORD FAR PASCAL SplitPath(LPSTR path, LPSTR drive, LPSTR dir,
			LPSTR fname, LPSTR ext);
void FAR PASCAL MakePath(LPSTR lpPath, LPSTR lpDrive, LPSTR lpDir,
			LPSTR lpFname, LPSTR lpExt);
LPSTR FAR PASCAL QualifyPathname(LPSTR lpszFile);


/* return codes from SplitPath */
#define PATH_OK		0	/* path is fine */
#define PATH_TOOLONG	1	/* filename too long   */
#define PATH_ILLEGAL	2	/* filename is illegal */

		
/* Definitions stolen from <stdlib.h> */
#ifndef _MAX_PATH

#define _MAX_PATH      144      /* max. length of full pathname */
#define _MAX_DRIVE   3      /* max. length of drive component */
#define _MAX_DIR       130      /* max. length of path component */
#define _MAX_FNAME   9      /* max. length of file name component */
#define _MAX_EXT     5      /* max. length of extension component */

#endif

/*
 *  DOS FAR UTILITY FUNCTIONS
 */
typedef struct _FindFileStruct {
	char	chReserved[21];
	BYTE	bAttribute;
	WORD	wTime;
	WORD	wDate;
	DWORD	dwSize;
	char	chFilename[13];
} FindFileStruct;
typedef FindFileStruct	FAR *FPFindFileStruct;
typedef FindFileStruct	NEAR *NPFindFileStruct;

#define	DOS_READONLY	0x0001
#define	DOS_HIDDEN	0x0002
#define DOS_SYSTEM	0x0004
#define DOS_VOLUME	0x0008
#define DOS_DIRECTORY	0x0010
#define DOS_ARCHIVE	0x0020
#define DOS_FILES	(DOS_READONLY | DOS_SYSTEM)
#define DOS_ALL		(DOS_FILES | DOS_DIRECTORY | DOS_HIDDEN)

/*  Return codes from DosFindFirst and DosFindNext  */
#define	DOSFF_OK		0
#define DOSFF_FILENOTFOUND	2
#define DOSFF_PATHINVALID	3
#define DOSFF_NOMATCH		0x12

WORD FAR PASCAL DosFindFirst(FPFindFileStruct lpFindStruct,
				LPSTR lpszFileSpec, WORD wAttrib);
WORD FAR PASCAL DosFindNext(FPFindFileStruct lpFindStruct);

int FAR PASCAL DosChangeDir(LPSTR lpszPath);
WORD FAR PASCAL DosGetCurrentDrive();
BOOL FAR PASCAL DosSetCurrentDrive(WORD wDrive);
WORD FAR PASCAL DosGetCurrentDir(WORD wCurdrive, LPSTR lpszBuf);
BOOL FAR PASCAL DosGetCurrentPath(LPSTR lpszBuf, WORD wLen);
WORD FAR PASCAL DosDeleteFile(LPSTR lpszFile);
BOOL FAR PASCAL DosGetVolume(BYTE chDrive, LPSTR lpszBuf);

WORD FAR PASCAL DosQueryNet(WORD wIndex, LPSTR lpszLocal, LPSTR lspzRemote);
WORD FAR PASCAL mscdGetDrives(LPSTR lpszDriveBuf);

/*  Return codes from DosQueryNet()  */
#define NET_ERROR	(-1)
#define	NET_INVALID	(0x0100)
#define	NET_TYPEMASK	(0x00ff)
#define NET_PRINTER	(0x0003)
#define NET_DRIVE	(0x0004)



/*
 *  ERROR MESSAGE REPORTING BOX
 */
short FAR cdecl ErrorResBox(	HWND	hwnd,
				HANDLE	hInst,
				WORD	flags,
				WORD	idAppName,
				WORD	idErrorStr, ...);

/*
 *  PROGRESS BAR GRAPH CONTROL - class "ProgBar"
 */
#define BAR_SETRANGE	(WM_USER + 0)
#define BAR_SETPOS	(WM_USER + 2)
#define BAR_DELTAPOS	(WM_USER + 4)
#define CTLCOLOR_PROGBAR	(CTLCOLOR_MAX + 2)


/*
 *  STATUS TEXT CONTROL - class "MPStatusText".
 *
 *  See wincom project file status.c for more information on control.
 */
#define ST_GETSTATUSHEIGHT	(WM_USER + 0)
#define ST_SETRIGHTSIDE		(WM_USER + 1)
#define ST_GETRIGHTSIDE		(WM_USER + 2)
#define CTLCOLOR_STATUSTEXT	(CTLCOLOR_MAX + 1)


/*
 *  ARROW CONTROL - class "ComArrow".
 */
LONG FAR PASCAL ArrowEditChange( HWND hwndEdit, WORD wParam, 
			LONG lMin, LONG lMax );

/*
 *  CHOOSER CONTROL - class "CHOOSER".
 */
/* Chooser Window control messages */
#define CM_SETITEMRECTSIZE	(WM_USER + 1)
#define CM_CALCSIZE		(WM_USER + 2)
#define CM_ADDITEM		(WM_USER + 3)
#define CM_INSERTITEM		(WM_USER + 4)
#define CM_DELETEITEM		(WM_USER + 5)
#define CM_GETCOUNT		(WM_USER + 6)
#define CM_GETITEMDATA		(WM_USER + 7)
#define CM_GETCURSEL		(WM_USER + 8)
#define CM_SETCURSEL		(WM_USER + 9)
#define CM_FINDITEM		(WM_USER + 10)
#define CM_ERR			LB_ERR

/* Chooser Window notification messages */
#define CN_SELECTED		100
#define CN_DESELECTED		101


/*
 *  Mac-like small Non-client window message handler
 */
LONG FAR PASCAL ncMsgFilter(HWND hwnd,unsigned msg, WORD wParam, LONG lParam);

/*  Window styles used by ncMsgFilter  */
#define WF_SIZEFRAME	WS_THICKFRAME
#define WF_SYSMENU	WS_SYSMENU
#define WF_MINIMIZED	WS_MINIMIZE
#define WF_SIZEBOX	0x0002


/*  Obscure stuff to deal with DLL loading/unloading  */
typedef HANDLE	HLIBLIST;
typedef WORD	DYNALIBID;

typedef struct _DynaLib {
	HANDLE	hModule;
	WORD	wRefcount;
	char	achLibname[_MAX_PATH];
} DynaLib;
typedef DynaLib FAR *FPDynaLib;

HLIBLIST FAR PASCAL dllMakeList(WORD wSize, LPSTR lpszLoadPoint,
			LPSTR lpszFreePoint);
BOOL FAR PASCAL	dllUnloadLib(HLIBLIST hlist, DYNALIBID id, BOOL fDestroy);
BOOL FAR PASCAL	dllDestroyList(HLIBLIST hlist);
BOOL FAR PASCAL dllForceUnload(HLIBLIST hlist);
HANDLE FAR PASCAL dllLoadLib(HLIBLIST hlist, DYNALIBID libid);
BOOL FAR PASCAL	dllGetInfo(HLIBLIST hlist, DYNALIBID libid, FPDynaLib fpLib);
BOOL FAR PASCAL dllIsLoaded(HLIBLIST hlist, DYNALIBID libid);
DYNALIBID FAR PASCAL dllAddLib(HLIBLIST hlist, LPSTR lpszName);
DYNALIBID FAR PASCAL dllIterAll(HLIBLIST hlist, DYNALIBID idLast);
DYNALIBID FAR PASCAL dllFindHandle(HLIBLIST hlist, HANDLE hModHandle);
DYNALIBID FAR PASCAL dllFindName(HLIBLIST hlist, LPSTR lpszName);
DYNALIBID FAR PASCAL dllAddLoadedLib(HLIBLIST hlist,HANDLE hModule,BOOL fLoad);



/**********************************
 *
 *	FOR DOS FILE FUNCTIONS (SWITCH PSP)
 *
 **********************************/


/* flags for DosSeek */
#define  SEEK_CUR 1
#define  SEEK_END 2
#define  SEEK_SET 0

/* DOS attributes */
#define ATTR_READONLY   0x0001
#define ATTR_HIDDEN     0x0002
#define ATTR_SYSTEM     0x0004
#define ATTR_VOLUME     0x0008
#define ATTR_DIR        0x0010
#define ATTR_ARCHIVE    0x0020
#define ATTR_FILES      (ATTR_READONLY+ATTR_SYSTEM)
#define ATTR_ALL_FILES  (ATTR_READONLY+ATTR_SYSTEM+ATTR_HIDDEN)
#define ATTR_ALL        (ATTR_READONLY+ATTR_DIR+ATTR_HIDDEN+ATTR_SYSTEM)

typedef struct {
    char        Reserved[21];
    BYTE        Attr;
    WORD        Time;
    WORD        Date;
    DWORD       Length;
    char        szName[13];
}   FCB;

typedef FCB     * PFCB;
typedef FCB FAR * LPFCB;

/* functions from dos.asm */

extern int   FAR PASCAL DosError(void);

extern int   FAR PASCAL DosOpen(LPSTR szFile,WORD acc);
extern int   FAR PASCAL DosCreate(LPSTR szFile,WORD acc);
extern int   FAR PASCAL DosDup(int fh);
extern void  FAR PASCAL DosClose(int fh);

extern DWORD FAR PASCAL DosSeek(int fh,DWORD ulPos,WORD org);
extern DWORD FAR PASCAL DosRead(int fh,LPSTR pBuf,DWORD ulSize);
extern DWORD FAR PASCAL DosWrite(int fh,LPSTR pBuf,DWORD ulSize);

/* DOS ERROR CODES */

#define ERROR_OK            0x00
#define ERROR_FILENOTFOUND  0x02    /* File not found */
#define ERROR_PATHNOTFOUND  0x03    /* Path not found */
#define ERROR_NOFILEHANDLES 0x04    /* Too many open files */
#define ERROR_ACCESSDENIED  0x05    /* Access denied */
#define ERROR_INVALIDHANDLE 0x06    /* Handle invalid */
#define ERROR_FCBNUKED      0x07    /* Memory control blocks destroyed */
#define ERROR_NOMEMORY      0x08    /* Insufficient memory */
#define ERROR_FCBINVALID    0x09    /* Memory block address invalid */
#define ERROR_ENVINVALID    0x0A    /* Environment invalid */
#define ERROR_FORMATBAD     0x0B    /* Format invalid */
#define ERROR_ACCESSCODEBAD 0x0C    /* Access code invalid */
#define ERROR_DATAINVALID   0x0D    /* Data invalid */
#define ERROR_UNKNOWNUNIT   0x0E    /* Unknown unit */
#define ERROR_DISKINVALID   0x0F    /* Disk drive invalid */
#define ERROR_RMCHDIR       0x10    /* Attempted to remove current directory */
#define ERROR_NOSAMEDEV     0x11    /* Not same device */
#define ERROR_NOFILES       0x12    /* No more files */
#define ERROR_13            0x13    /* Write-protected disk */
#define ERROR_14            0x14    /* Unknown unit */
#define ERROR_15            0x15    /* Drive not ready */
#define ERROR_16            0x16    /* Unknown command */
#define ERROR_17            0x17    /* Data error (CRC) */
#define ERROR_18            0x18    /* Bad request-structure length */
#define ERROR_19            0x19    /* Seek error */
#define ERROR_1A            0x1A    /* Unknown media type */
#define ERROR_1B            0x1B    /* Sector not found */
#define ERROR_WRITE         0x1D    /* Write fault */
#define ERROR_1C            0x1C    /* Printer out of paper */
#define ERROR_READ          0x1E    /* Read fault */
#define ERROR_1F            0x1F    /* General failure */
#define ERROR_SHARE         0x20    /* Sharing violation */
#define ERROR_21            0x21    /* File-lock violation */
#define ERROR_22            0x22    /* Disk change invalid */
#define ERROR_23            0x23    /* FCB unavailable */
#define ERROR_24            0x24    /* Sharing buffer exceeded */
#define ERROR_32            0x32    /* Unsupported network request */
#define ERROR_33            0x33    /* Remote machine not listening */
#define ERROR_34            0x34    /* Duplicate name on network */
#define ERROR_35            0x35    /* Network name not found */
#define ERROR_36            0x36    /* Network busy */
#define ERROR_37            0x37    /* Device no longer exists on network */
#define ERROR_38            0x38    /* NetBIOS command limit exceeded */
#define ERROR_39            0x39    /* Error in network adapter hardware */
#define ERROR_3A            0x3A    /* Incorrect response from network */
#define ERROR_3B            0x3B    /* Unexpected network error */
#define ERROR_3C            0x3C    /* Remote adapter incompatible */
#define ERROR_3D            0x3D    /* Print queue full */
#define ERROR_3E            0x3E    /* Not enough room for print file */
#define ERROR_3F            0x3F    /* Print file was deleted */
#define ERROR_40            0x40    /* Network name deleted */
#define ERROR_41            0x41    /* Network access denied */
#define ERROR_42            0x42    /* Incorrect network device type */
#define ERROR_43            0x43    /* Network name not found */
#define ERROR_44            0x44    /* Network name limit exceeded */
#define ERROR_45            0x45    /* NetBIOS session limit exceeded */
#define ERROR_46            0x46    /* Temporary pause */
#define ERROR_47            0x47    /* Network request not accepted */
#define ERROR_48            0x48    /* Print or disk redirection paused */
#define ERROR_50            0x50    /* File already exists */
#define ERROR_51            0x51    /* Reserved */
#define ERROR_52            0x52    /* Cannot make directory */
#define ERROR_53            0x53    /* Fail on Int 24H (critical error) */
#define ERROR_54            0x54    /* Too many redirections */
#define ERROR_55            0x55    /* Duplicate redirection */
#define ERROR_56            0x56    /* Invalid password */
#define ERROR_57            0x57    /* Invalid parameter */
#define ERROR_58            0x58    /* Net write fault */

/*
 *  DIB and BITMAP UTILITIES
 */
HANDLE FAR PASCAL dibCreate(DWORD dwWidth, DWORD dwHeight, WORD wBitCount,
			    WORD wPalSize, WORD wGmemFlags, WORD wDibFlags);
#define DBC_PALINDEX	0x0001

#define dibWIDTHBYTES(i)	(((i) + 31) / 32 * 4)



/*
 *  WPF OUTPUT WINDOW
 */
#define WPF_CHARINPUT	0x00000001L

int	FAR cdecl wpfVprintf(HWND hwnd, LPSTR lpszFormat, LPSTR pargs);
int	FAR cdecl wpfPrintf(HWND hwnd, LPSTR lpszFormat, ...);
void	FAR PASCAL wpfOut(HWND hwnd, LPSTR lpsz);

HWND FAR PASCAL wpfCreateWindow(HWND hwndParent, HANDLE hInst,LPSTR lpszTitle,
				DWORD dwStyle, WORD x, WORD y,
				WORD dx, WORD dy, int iMaxLines, WORD wID);

/*  Control messages sent to WPF window  */
//#define WPF_SETNLINES	(WM_USER + 1)
#define WPF_GETNLINES	(WM_USER + 2)
#define WPF_SETTABSTOPS	(WM_USER + 4)
#define WPF_GETTABSTOPS	(WM_USER + 5)
#define WPF_GETNUMTABS	(WM_USER + 6)
#define WPF_SETOUTPUT	(WM_USER + 7)
#define WPF_GETOUTPUT	(WM_USER + 8)
#define WPF_CLEARWINDOW (WM_USER + 9)

/*  Flags for WPF_SET/GETOUTPUT  */
#define	WPFOUT_WINDOW		1
#define WPFOUT_COM1		2
#define WPFOUT_NEWFILE		3
#define WPFOUT_APPENDFILE	4
#define WPFOUT_DISABLED		5

/*  Messages sent to owner of window  */
#define WPF_NTEXT	(0xbff0)
#define WPF_NCHAR	(0xbff1)


/**********************************
 *
 *	DEBUGGING SUPPORT
 *
 **********************************/

BOOL	FAR PASCAL	wpfDbgSetLocation(WORD wLoc, LPSTR lpszFile);
int	FAR cdecl	wpfDbgOut(LPSTR lpszFormat, ...);
BOOL	FAR PASCAL	wpfSetDbgWindow(HWND hwnd, BOOL fDestroyOld);

#define	WinPrintf	wpfDbgOut

#ifdef DEBUG
	BOOL	__fEval;
	BOOL	__iDebugLevel;

	int FAR PASCAL __WinAssert(LPSTR lpszFile, int iLine);

	#define WinAssert(exp)		\
		((exp) ? 0 : __WinAssert((LPSTR) __FILE__, __LINE__))
	#define WinEval(exp) (__fEval=(exp), WinAssert(__fEval), __fEval)

	#define wpfGetDebugLevel(lpszModule)	\
		(__iDebugLevel = GetProfileInt("MMDebug", (lpszModule), 0))

        #define wpfSetDebugLevel(i)    \
                (__iDebugLevel = (i))

        #define wpfDebugLevel()    (__iDebugLevel)

	#define dprintf if (__iDebugLevel) wpfDbgOut
	#define dprintf1 if (__iDebugLevel >= 1) wpfDbgOut
	#define dprintf2 if (__iDebugLevel >= 2) wpfDbgOut
	#define dprintf3 if (__iDebugLevel >= 3) wpfDbgOut
	#define dprintf4 if (__iDebugLevel >= 4) wpfDbgOut
#else
	#define WinAssert(exp) 0
	#define WinEval(exp) (exp)

        #define wpfGetDebugLevel(lpszModule) 0
        #define wpfSetDebugLevel(i)          0
        #define wpfDebugLevel()              0

	#define dprintf if (0) ((int (*)(char *, ...)) 0)
	#define dprintf1 if (0) ((int (*)(char *, ...)) 0)
	#define dprintf2 if (0) ((int (*)(char *, ...)) 0)
	#define dprintf3 if (0) ((int (*)(char *, ...)) 0)
	#define dprintf4 if (0) ((int (*)(char *, ...)) 0)
#endif


/**  THIS MUST BE LAST LINE OF FILE  **/
#endif
