/*	File: D:\WACKER\tdll\session.h (Created: 01-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */

/* --- Child window identifiers --- */

#define IDC_STATUS_WIN	 1	// ID of status window
#define IDC_TOOLBAR_WIN  2	// ID of toolbar window
#define IDC_TERMINAL_WIN 3	// ID of terminal window
#define IDC_SIDEBAR_WIN  4	// ID of sidebar window

/* --- Suspend identifiers --- */

#define SUSPEND_SCRLCK				1
#define SUSPEND_TERMINAL_MARKING	2
#define SUSPEND_TERMINAL_LBTNDN 	3
#define SUSPEND_TERMINAL_COPY		4

/* --- Command line connection flags --- */

#define CMDLN_DIAL_NEW		  0
#define CMDLN_DIAL_DIAL		  1
#define CMDLN_DIAL_OPEN 	  2 	// don't attempt connection
#define CMDLN_DIAL_WINSOCK    3		// try the command line as an IP address

/* --- Notification event IDs used with NotifyClient --- */

#define WM_SESS_NOTIFY		WM_USER+0x100
#define WM_SESS_ENDDLG		WM_USER+0x101
#define WM_FAKE_TIMER		WM_USER+0x102	// Used in timers.c
#define WM_CMDLN_DIAL		WM_USER+0x103
#define WM_SESS_SIZE_SHOW	WM_USER+0x104	// wPar=nCmdShow from WinMain()
#define WM_CNCT_DIALNOW 	WM_USER+0x105	// wPar=connection flags.
#define WM_DISCONNECT		WM_USER+0x106	// wPar=disconnect flags.
#define WM_HT_QUERYOPENFILE WM_USER+0x107	// lPar=global atom
#define WM_SESS_SHOW_SIDEBAR WM_USER+0x108	// mrw,4/13/95
#define WM_ERROR_MSG        WM_USER+0x109   // jmh,3/25/96

enum _emuNotify
	{
	EVENT_TERM_UPDATE,				// Server has updated terminal buffer.
	EVENT_TERM_TRACK,				// pause in data flow, can track cursor.
	EVENT_EMU_CLRATTR,				// Clear attribute has changed.
	EVENT_EMU_SETTINGS, 			// Emulator settings changed.
	EVENT_FATALMEM_ERROR,			// Unrecoverable memory error.
	EVENT_LOGFILE_ENTRY,			// Server has a waiting log file item
	EVENT_BYTESRCH_END, 			// Script Byte search operation ended
	EVENT_USER_XFER_END,			// User transfer ended
	EVENT_SCRIPT_XFER_END,			// Script transfer ended
	EVENT_PORTONLY_OPEN,			// similar to connection made
	EVENT_CONNECTION_OPENED,		// connection driver made connection.
	EVENT_CONNECTION_CLOSED,		// disconnect completed.
	EVENT_CONNECTION_INPROGRESS,	// connection in progress
	EVENT_GETSTRING_END,			// Get String operation ended
	EVENT_HOST_XFER_REQ,			// host transfer request made.
	EVENT_HOST_XFER_ENQ,			// host transfer enquiry make.
	EVENT_CLOOP_SEND,				// CLoop send called.
	EVENT_SCR_FUNC_END, 			// A script function ended
	EVENT_CLOSE_SESSION,			// Instructs to close the session.
	EVENT_ERROR_MSG, 				// argument has string to load.
	EVENT_LEARN_SOMETHING,			// learning has something to do
	EVENT_DDE_GOT_DATA, 			// dde has something to return
	EVENT_WAIT_FOR_CALLBACK,		// system should wait for callback.
	EVENT_KILL_CALLBACK_DLG,		// dismisses callback dialog
	EVENT_COM_DEACTIVATED,			// com driver deactivated port
	EVENT_CNCT_DLG, 				// cnct driver connection dialog message
	EVENT_PRINT_ERROR,				// printecho error.
	EVENT_LED_AA_ON,				// obvious...
	EVENT_LED_CD_ON,
	EVENT_LED_OH_ON,
	EVENT_LED_RD_ON,
	EVENT_LED_SD_ON,
	EVENT_LED_TR_ON,
	EVENT_LED_MR_ON,
	EVENT_LED_AA_OFF,
	EVENT_LED_CD_OFF,
	EVENT_LED_OH_OFF,
	EVENT_LED_RD_OFF,
	EVENT_LED_SD_OFF,
	EVENT_LED_TR_OFF,
	EVENT_LED_MR_OFF,
    EVENT_LOST_CONNECTION
	};

typedef enum _emuNotify NOTIFICATION;

void NotifyClient(const HSESSION hSession, const NOTIFICATION nEvent,
				  const long lExtra);

void DecodeNotification(const HWND hwndSession, WPARAM wPar, LPARAM lPar);

void DecodeSessionNotification(const HWND hwndSession,
							const NOTIFICATION nEvent,
							const LPARAM lExtra);

/* --- Create and Destroy Functions --- */

HSESSION CreateSessionHandle(const HWND hwndSession);

BOOL InitializeSessionHandle(const HSESSION hSession, const HWND hwnd,
							 const CREATESTRUCT *pcs);

BOOL ReinitializeSessionHandle(const HSESSION hSession, const int fUpdateTitle);

void DestroySessionHandle(const HSESSION hSession);

HWND CreateSessionToolbar(const HSESSION hSession, const HWND hwndSession);
HWND CreateTerminalWindow(const HWND hwndSession);

int  CreateEngineThread(const HSESSION hSession);
void DestroyEngineThread(const HSESSION hSession);

/* ---	Set and Query functions --- */

HWND sessQueryHwnd(const HSESSION hSession);
HWND sessQueryHwndStatusbar(const HSESSION hSession);
HWND sessQueryHwndToolbar(const HSESSION hSession);
HUPDATE sessQueryUpdateHdl(const HSESSION hSession);

void sessSetEngineThreadHdl(const HSESSION hSession, const HANDLE hThread);
HANDLE sessQueryEngineThreadHdl(const HSESSION hSession);

HWND sessQueryHwndTerminal(const HSESSION hSession);
HTIMERMUX sessQueryTimerMux(const HSESSION hSession);
HEMU sessQueryEmuHdl(const HSESSION hSession);
HCLOOP sessQueryCLoopHdl(const HSESSION hSession);
HCOM sessQueryComHdl(const HSESSION hSession);
HTRANSLATE sessQueryTranslateHdl(const HSESSION hSession);

void sessSetSysFileHdl(const HSESSION hSession, const SF_HANDLE hSF);
SF_HANDLE sessQuerySysFileHdl(const HSESSION hSession);

HBACKSCRL sessQueryBackscrlHdl(const HSESSION hSession);
HXFER sessQueryXferHdl(const HSESSION hSession);
HFILES sessQueryFilesDirsHdl(const HSESSION hSession);
HCAPTUREFILE sessQueryCaptureFileHdl(const HSESSION hSession);
HPRINT sessQueryPrintHdl(const HSESSION hSession);
void sessQueryCmdLn(const HSESSION hSession, LPTSTR pach, const int len);
HCNCT sessQueryCnctHdl(const HSESSION hSession);
#if defined(INCL_WINSOCK)
int sessQueryTelnetPort(const HSESSION hSession);
#endif

void sessSetTimeout(const HSESSION hSession, int nTimeout);
int sessQueryTimeout(const HSESSION hSession);
void  sessInitializeIcons(HSESSION hSession);
void  sessLoadIcons(HSESSION hSession);
void  sessSaveIcons(HSESSION hSession);

void  sessSetIconID(const HSESSION hSession, const int nID);
int   sessQueryIconID(const HSESSION hSession);

HICON sessQueryIcon(const HSESSION hSession);
HICON sessQueryLittleIcon(const HSESSION hSession);

void sessSetName(const HSESSION hSession, const LPTSTR pach);
void sessQueryName(const HSESSION hSession, const LPTSTR pach, unsigned uSize);
int sessQuerySound(const HSESSION hSession);
void sessSetSound(const HSESSION hSession, int fSound);
//mpt:10-28-97 added 'exit on disconnect' feature
int sessQueryExit(const HSESSION hSession);
void sessSetExit(const HSESSION hSession, int fExit);

int sessQueryIsNewSession(const HSESSION hSession);
void sessSetIsNewSession(const HSESSION hSession, int fIsNewSession);
void sessQueryOldName(const HSESSION hSession, const LPTSTR pach, unsigned uSize);
BOOL sessIsSessNameDefault(LPTSTR pacName);

void sessQueryWindowRect(const HSESSION hSession, RECT *rec);
int	 sessQueryWindowShowCmd(const HSESSION hSession);

HWND sessQuerySidebarHwnd(const HSESSION hSession);
HWND CreateSidebar(const HWND hwndSession, const HSESSION hSession);

/* --- sessmenu.c --- */

void sessInitMenuPopupCall(const HSESSION hSession, const HMENU hMenu);
void sessInitMenuPopupEdit(const HSESSION hSession, const HMENU hMenu);
void sessInitMenuPopupView(const HSESSION hSession, const HMENU hMenu);
void sessInitMenuPopupTransfer(const HSESSION hSession, const HMENU hMenu);
void sessInitMenuPopupHelp(const HSESSION hSession, const HMENU hMenu);
void HandleContextMenu(HWND hwnd, POINT point);

/* --- sessutil.c --- */

void sessSnapToTermWindow(const HWND hwnd);
BOOL sessComputeSnapSize(const HSESSION hSession, const LPRECT prc);
void sessSetMinMaxInfo(const HSESSION hSession, const PMINMAXINFO pmmi);

int  OpenSession(const HSESSION hSession, HWND hwnd);
BOOL SaveSession(const HSESSION hSession, HWND hwnd);
void SilentSaveSession(const HSESSION hSession, HWND hwnd, BOOL fExplicit);
void SaveAsSession(const HSESSION hSession, HWND hwnd);

void sessSaveSessionStuff(const HSESSION hSession);
BOOL sessLoadSessionStuff(const HSESSION hSession);

void sessSetSuspend(const HSESSION hSession, const int iReason);
void sessClearSuspend(const HSESSION hSession, const int iReason);
BOOL IsSessionSuspended(const HSESSION hSession);

BOOL sessSaveBackScroll(const HSESSION hSession);
BOOL sessRestoreBackScroll(const HSESSION hSession);

BOOL sessQueryToolbarVisible(const HSESSION hSession);
BOOL sessSetToolbarVisible(const HSESSION hSession, const BOOL fVisible);

BOOL sessQueryStatusbarVisible(const HSESSION hSession);
BOOL sessSetStatusbarVisible(const HSESSION hSession, const BOOL fVisible);

void sessCmdLnDial(const HSESSION hSession);

void sessUpdateAppTitle(const HSESSION hSession);

BOOL sessDisconnectToContinue(const HSESSION hSession, HWND hwnd);

void sessSizeAndShow(const HWND hwnd, const int nCmdShow);

void sessBeeper(const HSESSION hSession);

/* --- fontdlg.c --- */

void DisplayFontDialog(const HSESSION hSession, BOOL fPrinterFont);

/* --- termcpy.c --- */

BOOL CopyMarkedTextFromTerminal(const HSESSION hSession, void **ppv,
								DWORD *pdwCnt, const BOOL fIncludeLF);

BOOL CopyTextFromTerminal(const HSESSION hSession,
						  const PPOINT pptBeg,
						  const PPOINT pptEnd,
						  void **ppv,
						  DWORD *dwCnt,
						  const BOOL fIncludeLF);
/* --- clipbrd.c --- */

BOOL PasteFromClipboardToHost(const HWND hwnd, const HSESSION hSession);

/* --- toolbar.c --- */

VOID ToolbarNeedsText(HSESSION hSession, long lPar);

LRESULT ToolbarNotification(const HWND hwnd,
						const int nId,
						const int nNotify,
						const HWND hwndCtrl);

void ToolbarEnableMinitelButtons(const HWND hwndToolbar, const int fEnable);
