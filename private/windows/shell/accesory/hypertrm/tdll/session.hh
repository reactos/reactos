/*	File: D:\WACKER\tdll\session.hh (Created: 01-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:34p $
 */

#if !defined(FEATURES_H_INCLUDED)
You really need to include the "features.h" file before "session.hh"
#endif

typedef struct stSessionData *HHSESSION;

/* --- Data Structures --- */

struct stSessionData
	{
	long	lPrefix;			// used to verify session handle

	HWND	hwndSess;			// handle of session window
	HWND	hwndStatusbar;		// handle of statusbar window
	HWND	hwndToolbar;		// handle of toolbar window
	HWND	hwndTerm;			// handle of terminal window
	HWND	hwndSidebar;		// handle of sidebar window

	CRITICAL_SECTION csSess;	// for snychronizing access

	HTIMERMUX	hTimerMux;		// handle to multiplexed timers
	HUPDATE 	hUpdate;		// update info from emulator.
	HEMU		hEmu;			// emulator handle.
	HCOM		hCom;			// handle to internal com driver
	HCLOOP		hCLoop; 		// handle to com loop
	HCNCT		hCnct;			// handle to connection driver
	SF_HANDLE	hSysFile;		// system file handle
	HXFER		hXferHdl;		// transfer parameters
	HFILES		hFilesHdl;		// files and directory stuff
	HBACKSCRL	hBackscrl;		// backscroll handle
	HCAPTUREFILE hCaptFile; 	// capture file handle
	HPRINT		hPrint; 		// print handle
	HTRANSLATE	hTranslate; 	// character translation handle, mrw,3/1/95

	int 	nTimeout;			// timeout value for error messages

	TCHAR achSessCmdLn[256];	// passed in last parameter of CreateWindow()

	BOOL	fToolbarVisible;	// is the toolbar visible ?
	BOOL	fStatusbarVisible;	// is the statusbar visible ?

	// Suspend variables used to "scroll lock" sessions
	//
	BOOL	fSuspendScrlLck,
			fSuspendTermMarking,
			fSuspendTermLBtnDn,
			fSuspendTermCopy;

	BOOL	fSound;				// Turn on/off annoying sounds
	BOOL	fExit;				// Turn on/off exit upon disconnecting

	int		nIconId;			// resource ID for selected ICON, if any.
	HICON	hIcon;				// session icon, defaults to program icon.
  //HICON	hLittleIcon;		// small copy of previous icon, if available

	// Yes, we do need to keep the session name shadow!
	//
	TCHAR	achSessName[256];	// name of session.
	TCHAR	achOldSessName[256];// shadow of the session name.

	BOOL	fIsNewSession;		// TRUE if new session being created.

	int 	iCmdLnDial; 		// Used to see how to dial
#if defined(INCL_WINSOCK)
	int     iTelnetPort;		// To pass value entered as cmd ln URL
#endif

	long	lPostfix;			// used to verify session handle
								// keep this the last item, please

	RECT	rcSess;				// Session window's rect.
	int		iShowCmd;			// Session window's show state.
	};

#define	PRE_MAGIC		0x12345678
#define	POST_MAGIC		0x09ABCDEF

/* --- Function Prototypes --- */

HHSESSION VerifySessionHandle(const HSESSION hSession);
void hLock(const HHSESSION hhSess);
void hUnLock(const HHSESSION hhSess);
int sessInitializePrinterName(const HSESSION hSession);
int sessCheckAndLoadCmdLn(const HSESSION hSession);
int fTestOpenOldTrmFile(const HHSESSION hhSess, TCHAR *achName);
