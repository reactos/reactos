/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991,1992                    **/
/***************************************************************************/
// ts=4

// CLIPBOOK.C - ClipBook viewer main and window procs
// 11-91 clausgi created

#define   WIN31
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "clipbook.h"
#include "dialogs.h"
#include "clipdsp.h"
#include "..\common\common.h"
#include <commctrl.h>
#include <nddeapi.h>
#include "uniconv.h"

#ifndef UNICODE_CHARSET
#define UNICODE_CHARSET 1
#endif

#include ".\helpids.h"

// used for winnet server browse call
#define WNETBROWSEENTRYPOINT 146
typedef UINT (WINAPI *WNETCALL)(HWND, LPTSTR, LPTSTR, WORD, DWORD );
WNETCALL WNetServerBrowseDialog;
HANDLE hmutexClp;

HANDLE hmodNetDriver;

// forward declarations for module internal stuff

static BOOL  InitApplication( HINSTANCE hInstance );
static BOOL  InitInstance( HINSTANCE hInstance, int nCmdShow );
static VOID  StripCharRange (TCHAR *s, TCHAR lower, TCHAR upper );
static VOID  SendMessageToKids (WORD msg, WPARAM wParam, LPARAM lParam);

// Static data

static HHOOK hMsgFilterHook;

// Stuff for dealing with minimized MDI children
static HCURSOR hcurClipbook;
static HCURSOR hcurClipbrd;
static HCURSOR hcurRemote;
HICON   hicClipbrd;
HICON   hicClipbook;
HICON   hicRemote;

// Icon for Lock on thumbnail bitmaps
HICON   hicLock;

// Handle for Unicode font, if it exists
HFONT   hFontUni;



// Application-wide flags

#if DEBUG
int   DebugLevel;
#endif

BOOL   fStatus;                        // status bar shown?
BOOL   fToolBar;                       // tool bar shown?
BOOL   fShareEnabled;                  // sharing allowed in system.ini?
BOOL   fNetDDEActive = TRUE;           // NetDDE detected?
BOOL   fAppLockedState = FALSE;        // app UI locked (see LockApp())
BOOL   fClipboardNeedsPainting = FALSE;// indicates deferred clp paint
BOOL   fSharePreference;               // shared checked on paste?
BOOL   fNeedToTileWindows = FALSE;     // need to tile windows on size
BOOL   fAppShuttingDown = FALSE;       // in process of closing
BOOL   fFillingClpFromDde = FALSE;     // in process of adding clp formats
BOOL   fAuditEnabled;

HWND   hwndNextViewer = NULL;   // for clpbrd viewer chain
HWND   hwndDummy;   // used as dummy SetCapture target

// special case clipboard formats

UINT cf_bitmap;         // we send/receive these in private 'packed' format
UINT cf_metafilepict;
UINT cf_palette;

UINT cf_preview;      // PREVBMPSIZxPREVBMPSIZ preview bitmap private format

// these are formats that contain untranslated copies of link and objlink data
UINT cf_objectlinkcopy;
UINT cf_objectlink;
UINT cf_linkcopy;
UINT cf_link;

// DDEML

// These are effective constants created once and destroyed when we die
HSZ      hszSystem;
HSZ      hszTopics;
HSZ      hszDataSrv;
HSZ      hszFormatList;
HSZ      hszClpBookShare;


DWORD   dwCurrentHelpId = 0L;

// instance proc from MSGF_DDEMGR filter
HOOKPROC lpMsgFilterProc;

WINDOWPLACEMENT   Wpl;

HINSTANCE  hInst;

HACCEL     hAccel;

HFONT      hOldFont, hFontStatus, hFontPreview;

// this handle identifies the currently active MDI window
HWND      hwndActiveChild = 0;
// this pointer points to the MDI info struct of the active MDI window
// IT SHOULD ALWAYS == GETMDIINFO(hwndActiveChild)
PMDIINFO   pActiveMDI = NULL;

// this handle identifies the clipboard window
HWND      hwndClpbrd = 0;

// this handle identifies the local clipbook window
HWND      hwndLocal = 0;

// this handle identifies the clipboard owning MDI child (if any)
HWND      hwndClpOwner = 0;

HWND      hwndMDIClient;   // handle to MDI Client window

HWND      hwndApp;      // global app window

HDC         hBtnDC;         // memory DC used for owner draw stuff

HBITMAP      hOldBitmap;
HBITMAP      hPreviewBmp;
HBITMAP      hPgUpBmp;
HBITMAP      hPgDnBmp;
HBITMAP      hPgUpDBmp;
HBITMAP      hPgDnDBmp;

int         dyStatus;      // height of status bar
int         dyButtonBar;    // height of button bar
int         dyPrevFont;      // height of listbox font - height+external

TCHAR      szHelpFile[]      = TEXT("clipbrd.hlp");

TCHAR      szClipBookClass[]   = TEXT("ClipBookWClass");   // frame window class
TCHAR      szChild[] = TEXT("CVchild");   // Class name for MDI window
TCHAR      szDummy[] = TEXT("CVdummy");      // class name of hidden dummy window

TCHAR     szNDDEcode[] = TEXT("NDDE$");
TCHAR     szNDDEcode1[] = TEXT("NDDE$0001");
TCHAR     szClpBookShare[] = TEXT("CLPBK$");

#define      SMLRCBUF   32
#define      BIGRCBUF   64

// localized strings
TCHAR      szHelv[SMLRCBUF];   // status line font
TCHAR      szAppName[SMLRCBUF];
TCHAR      szLocalClpBk[SMLRCBUF];
TCHAR      szSysClpBrd[SMLRCBUF];
TCHAR      szDataUnavail[BIGRCBUF];
TCHAR      szReadingItem[BIGRCBUF];

#ifdef JAPAN
TCHAR      szViewHelpFmt[BIGRCBUF];
TCHAR      szActivateFmt[BIGRCBUF];
#else
TCHAR      szViewHelpFmt[SMLRCBUF];
TCHAR      szActivateFmt[SMLRCBUF];
#endif

TCHAR      szRendering[BIGRCBUF];
TCHAR      szDefaultFormat[BIGRCBUF];
TCHAR      szGettingData[BIGRCBUF];
TCHAR      szEstablishingConn[BIGRCBUF];
TCHAR      szClipBookOnFmt[BIGRCBUF];
TCHAR      szPageFmt[SMLRCBUF];
TCHAR      szPageFmtPl[SMLRCBUF];
TCHAR      szPageOfPageFmt[SMLRCBUF];
TCHAR      szDelete[SMLRCBUF];
TCHAR      szDeleteConfirmFmt[SMLRCBUF];
TCHAR      szFileFilter[BIGRCBUF];
TCHAR     *szFilter;

// Registry key strings
static TCHAR szRoot[128];
TCHAR szPref[]      = TEXT("Preferences");
TCHAR szConn[]      = TEXT("Connections");
TCHAR szStatusbar[] = TEXT("StatusBar");
TCHAR szToolbar[]   = TEXT("ToolBar");
TCHAR szShPref[]    = TEXT("AutoShare");
TCHAR szEnableShr[] = TEXT("EnableShare");
TCHAR szDefView[]   = TEXT("DefView");
#if DEBUG
TCHAR szDebug[]     = TEXT("Debug");
#endif
TCHAR szNull[]      = TEXT("");

HKEY hkeyRoot;

// buffers
TCHAR      szBuf[SZBUFSIZ];
TCHAR      szBuf2[SZBUFSIZ];

TCHAR      szConvPartner[128];   // bigger than max server name
TCHAR      szKeepAs[ MAX_NDDESHARENAME + 2 ];


// DDEML stuff

DWORD      idInst = 0;         // DDEML handle

////////////////////// functions //////////////////////////////////

int WinMain(
HINSTANCE hInstance,
HINSTANCE hPrevInstance,
LPSTR lpAnsiCmdLine,
int nCmdShow)
{
MSG msg;
LPTSTR lpCmdLine = GetCommandLine ();

#if DEBUG
DebugLevel = 2;
#endif

LoadString(hInstance, IDS_APPNAME, szAppName, SMLRCBUF);

// Only one instance is supported
// hPrevInstance always == null under NT, so we have to rely on FWin.
if (hwndApp = FindWindow(szClipBookClass, NULL))
   {
   PINFO(TEXT("Found previous instance\r\n"));

   if (IsIconic(hwndApp))
      {
      ShowWindow(hwndApp, SW_RESTORE);
      SetForegroundWindow(hwndApp);
      }
   else
      {
      SetForegroundWindow(hwndApp);
      }

   return FALSE;
   }

if (!InitApplication(hInstance))
   {
   PERROR(TEXT("InitApp fail\r\n"));
   return (FALSE);
   }

if (!InitInstance(hInstance, nCmdShow ))
   {
   PERROR(TEXT("InitInstance fail\r\n"));
   return (FALSE);
   }

UpdateWindow ( hwndApp );

// if we were started with the name of a file on the command line,
// attempt to load the .clp file via an open dde execute

if ( MyOpenFile ( lpCmdLine, NULL, OF_EXIST ) != INVALID_HANDLE_VALUE )
   {
   if ( ClearClipboard(hwndApp) )
      {
      lstrcat(lstrcpy(szBuf, SZCMD_OPEN), SkipProgramName(lpCmdLine));

      MySyncXact ( (LPBYTE)szBuf, ByteCountOf(lstrlen(szBuf) + 1),
         GETMDIINFO(hwndLocal)->hExeConv, 0L, CF_TEXT,
         XTYP_EXECUTE, SHORT_SYNC_TIMEOUT, NULL );
      InitializeMenu ( GetMenu(hwndApp) );
      }
   }

while (GetMessage (&msg, NULL, 0, 0))
   {
   (*lpMsgFilterProc)(MSGF_DDEMGR, 0, (LONG)(LPMSG)&msg);
   }

// Clear the mutex
CloseHandle(hmutexClp);

// free up our HSZ 'constants'
DdeFreeStringHandle(idInst, hszTopics);
DdeFreeStringHandle(idInst, hszFormatList );
DdeFreeStringHandle(idInst, hszSystem);
DdeFreeStringHandle(idInst, hszDataSrv);
DdeFreeStringHandle(idInst, hszClpBookShare );

// Free icons & cursors
DestroyIcon(hicClipbrd);
DestroyIcon(hicClipbook);
DestroyIcon(hicRemote);
DestroyIcon(hicLock);
DestroyCursor(hcurClipbrd);
DestroyCursor(hcurClipbook);
DestroyCursor(hcurRemote);

DdeUninitialize(idInst);

UnhookWindowsHookEx(hMsgFilterHook);

return (msg.wParam);
}


static BOOL  InitApplication ( HINSTANCE hInstance )
{
WNDCLASS  wc;

// Register the frame window
wc.style = 0;
wc.lpfnWndProc = FrameWndProc;
wc.cbClsExtra = 0;
wc.cbWndExtra = 0;
wc.hInstance = hInstance;
wc.hIcon = LoadIcon ( hInstance, (LPTSTR) MAKEINTRESOURCE(IDFRAMEICON) );
wc.hCursor = LoadCursor(NULL, IDC_ARROW);
wc.hbrBackground = NULL;      // will paint whole client area
wc.lpszMenuName =  (LPTSTR) MAKEINTRESOURCE(IDCVMENU);
wc.lpszClassName = szClipBookClass;

if (0 == RegisterClass(&wc))
   {
   return FALSE;
   }

// Register the MDI child class
wc.lpfnWndProc   = ChildWndProc;
wc.hIcon       = NULL;
wc.lpszMenuName  = NULL;
wc.cbWndExtra   = CBWNDEXTRA;
wc.lpszClassName = szChild;
wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
wc.style = CS_DBLCLKS;

if (0 == RegisterClass(&wc))
   return FALSE;

// register dummy window for SetCapture target
wc.lpfnWndProc = DefWindowProc;
wc.hIcon = NULL;
wc.lpszMenuName = NULL;
wc.cbWndExtra = 0;
wc.lpszClassName = szDummy;
wc.style = 0;

if (0 == RegisterClass(&wc))
   return FALSE;
}

// Purpose: Initialize a given instance of the application.
//
//
//////////////////////////////////////////////////////////////////////
static BOOL InitInstance(
HINSTANCE hInstance,
int nCmdShow)
{
HANDLE  hRsc;
LOGFONT UniFont;
HRSRC   hFilterRsc;
HGLOBAL hFilterText;
DWORD   dwKeyStatus;
HMENU   hFileMenu;
SC_HANDLE hsc;
SC_HANDLE hsrvWksta;
SERVICE_STATUS ss;

// Stuff used to make the local server's name, "\\<computername>\NDDE$"
TCHAR     atchSrvName[MAX_COMPUTERNAME_LENGTH + 9];
DWORD     dwSize;

hInst = hInstance;

hmutexClp = CreateMutex(NULL, FALSE, SZMUTEXCLP);

hAccel = LoadAccelerators(hInstance,
      (LPTSTR) MAKEINTRESOURCE(IDACCELERATORS));
if (NULL == hAccel)
   {
   PERROR(TEXT("error loading accelerator table\n\r"));
#ifndef DEBUG
   return FALSE;
#endif
   }

// Load cursors for dragging MDI children
hcurClipbook = LoadCursor(hInst, (LPTSTR) MAKEINTRESOURCE(IDC_CLIPBOOK));
hcurClipbrd  = LoadCursor(hInst, (LPTSTR) MAKEINTRESOURCE(IDC_CLIPBRD));
hcurRemote   = LoadCursor(hInst, (LPTSTR) MAKEINTRESOURCE(IDC_REMOTE));

// Load icons for MDI children
hicClipbook = LoadIcon(hInst, (LPTSTR) MAKEINTRESOURCE(IDI_CLIPBOOK));
hicClipbrd  = LoadIcon(hInst, (LPTSTR) MAKEINTRESOURCE(IDI_CLIPBRD));
hicRemote   = LoadIcon(hInst, (LPTSTR) MAKEINTRESOURCE(IDI_REMOTE));

// Load Lock icon
hicLock = LoadIcon ( hInst, (LPTSTR) MAKEINTRESOURCE(IDLOCKICON));

// Load the Unicode font, for displaying Unicode text.
GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), (LPVOID)&UniFont);
UniFont.lfCharSet = UNICODE_CHARSET;
lstrcpy(UniFont.lfFaceName, UNICODE_FONT_NAME);
hFontUni = CreateFontIndirect(&UniFont);
if (hFontUni == NULL)
   {
   hFontUni = (HFONT)GetStockObject(SYSTEM_FONT);
   }


LoadIntlStrings();

/* Load custom resources for CommDlg filters - these can't be strings, */
/* because they contain embedded NULL characters.                      */
/* a-mgates 9/24/92                                                    */
hFilterRsc = FindResourceEx(hInst,
      (LPTSTR) MAKEINTRESOURCE(ENVSTR), (LPTSTR) MAKEINTRESOURCE(IDS_FILTERTEXT),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) );
hFilterText = LoadResource(hInst, hFilterRsc);
szFilter = LockResource(hFilterText);


// initialize variables in clipdsp.c
fOwnerDisplay = FALSE;

// initialize DDEML
if ( DdeInitialize( &idInst,(PFNCALLBACK)DdeCallback,APPCLASS_STANDARD, 0L) )
   {
   PERROR(TEXT("The DDEML did not initialize\n\r"));
   return FALSE;
   }

// create our hsz constants
atchSrvName[0] = atchSrvName[1] = TEXT('\\');
dwSize = MAX_COMPUTERNAME_LENGTH;
GetComputerName(atchSrvName + 2, &dwSize);
lstrcat(atchSrvName, TEXT("\\NDDE$"));

hszDataSrv =   DdeCreateStringHandle(idInst, atchSrvName,    CP_WINANSI);
hszSystem =    DdeCreateStringHandle(idInst, TEXT("CLPBK$"), CP_WINANSI);
hszTopics =    DdeCreateStringHandle(idInst, SZDDESYS_ITEM_TOPICS, CP_WINANSI);
hszFormatList =   DdeCreateStringHandle(idInst, SZ_FORMAT_LIST,    CP_WINANSI);
hszClpBookShare = DdeCreateStringHandle(idInst, szClpBookShare,    CP_WINANSI);

if ( DdeGetLastError(idInst) != DMLERR_NO_ERROR )
   {
   PERROR(TEXT("DDEML error during init\n\r"));
   return FALSE;
   }

// We set this hook up so that we can catch the MSGF_DDEMGR message
// which is called when DDEML is in a modal loop during synchronous
// transaction processing.
lpMsgFilterProc = (HOOKPROC)MyMsgFilterProc;
hMsgFilterHook = SetWindowsHookEx(WH_MSGFILTER, lpMsgFilterProc, hInst,
      GetCurrentThreadId());
if (NULL == hMsgFilterHook)
   {
   PERROR(TEXT("SetWindowsHook failed\n\r"));
   return FALSE;
   }

// get preference flags
LoadString(hInst, IDS_CLPBKKEY, szRoot, CharSizeOf(szRoot));
if ( ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, szRoot, 0L, NULL,
         REG_OPTION_NON_VOLATILE,
         KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_ENUMERATE_SUB_KEYS,
         NULL, &hkeyRoot, &dwKeyStatus))
   {
   PERROR(TEXT("Could not set up root key\r\n"));
   fStatus = TRUE;
   fToolBar = TRUE;
   fSharePreference = TRUE;
   fShareEnabled = FALSE;
   }
else
   {
   DWORD iSize = sizeof(fStatus);

   PINFO (TEXT("Root key created. Key Status %ld."),dwKeyStatus);

   if (ERROR_SUCCESS != RegQueryValueEx(hkeyRoot, szStatusbar,
         NULL, NULL, (LPBYTE)&fStatus, &iSize))
      {
      fStatus = TRUE;
      }
   if (ERROR_SUCCESS != RegQueryValueEx(hkeyRoot, szToolbar,
         NULL, NULL, (LPBYTE)&fToolBar, &iSize))
      {
      fToolBar = TRUE;
      }
   if (ERROR_SUCCESS != RegQueryValueEx(hkeyRoot, szShPref,
         NULL, NULL, (LPBYTE)&fSharePreference, &iSize))
      {
      fSharePreference = FALSE;
      }
#if DEBUG
   if (ERROR_SUCCESS != RegQueryValueEx(hkeyRoot, szDebug, NULL, NULL,
         (LPBYTE)&DebugLevel, &iSize))
      {
      DebugLevel = 2;
      }
#endif

   if (ERROR_SUCCESS != RegQueryValueEx(hkeyRoot, szEnableShr,
         NULL, NULL, (LPBYTE)&fShareEnabled, &iSize))
      {
      fShareEnabled = TRUE;
      }
   }

// Figure out if NetBIOS is active or not, thus if we can net connect
fNetDDEActive = FALSE;
hsc = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
if (NULL != hsc)
   {
   hsrvWksta = OpenService(hsc, TEXT("LanmanWorkstation"), SERVICE_QUERY_STATUS);
   if (NULL != hsrvWksta)
      {
      if (QueryServiceStatus(hsrvWksta, &ss))
         {
         if (ss.dwCurrentState == SERVICE_RUNNING)
            {
            fNetDDEActive = TRUE;
            }
         }
      CloseServiceHandle(hsrvWksta);
      }
   CloseServiceHandle(hsc);
   }
else
   {
   PERROR(TEXT("Couldn't open SC mgr\r\n"));
   }

fAuditEnabled = AuditPrivilege(AUDIT_PRIVILEGE_CHECK);

// Create main window
if ( !( hwndApp = CreateWindow (
   szClipBookClass,
   szAppName,
   WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
   CW_USEDEFAULT,
   CW_USEDEFAULT,
   CW_USEDEFAULT,
   CW_USEDEFAULT,
   NULL,
   NULL,
   hInstance,
   NULL)))
   {
   PERROR(TEXT("CreateWindow failed!\r\n"));
   return FALSE;
   }

// Get the handle to the Display popup menu for adding format entries
hDispMenu = GetSubMenu( GetMenu(hwndApp), DISPLAY_MENU_INDEX);

hFileMenu = GetSubMenu(GetMenu(hwndApp), 0);

// get rid of share menu entries?
if ( !fShareEnabled )
   {
   EnableMenuItem ( hFileMenu, IDM_SHARE, MF_BYCOMMAND | MF_GRAYED);
   EnableMenuItem ( hFileMenu, IDM_UNSHARE, MF_BYCOMMAND | MF_GRAYED);
   EnableMenuItem ( hFileMenu, IDM_PROPERTIES, MF_BYCOMMAND | MF_GRAYED);
   }

// get rid of connect/disonnect entries?
if ( !fNetDDEActive )
   {
   EnableMenuItem ( hFileMenu, IDM_CONNECT, MF_BYCOMMAND | MF_GRAYED);
   EnableMenuItem ( hFileMenu, IDM_DISCONNECT, MF_BYCOMMAND | MF_GRAYED);
   }

DrawMenuBar(hwndApp);

// window placement...
if ( ReadWindowPlacement ( szAppName, &Wpl ))
   {
   Wpl.showCmd = nCmdShow;
   Wpl.ptMaxPosition.x = -1;
   Wpl.ptMaxPosition.y = -1;
   SetWindowPlacement ( hwndApp, &Wpl );
   UpdateWindow(hwndApp);
   }
else
   {
   ShowWindow ( hwndApp, nCmdShow );
   }

// make our SetCapture target window
if ( !( hwndDummy = CreateWindow(
               szDummy,
               szNull,
                WS_CHILD | WS_DISABLED ^ WS_VISIBLE,
               0, 0, 0, 0, hwndApp, NULL, hInstance,
               NULL )))
   return FALSE;

// Make clipboard window -- needs to happen BEFORE we SetClipboardViewer,
// 'cause hwndApp will get a WM_DRAWCLIPBOARD and there won't be any friggin'
// windows.
SendMessage ( hwndApp, WM_COMMAND, IDM_CLPWND, 0L );

// Attach us to the clipboard viewer chain
hwndNextViewer = SetClipboardViewer(hwndApp);

// hconv=

// create initial local window.
SendMessage ( hwndApp, WM_COMMAND, IDM_LOCAL, 0L );

// force paint before restoring other connections so we don't
// have to wait too long. Should we do this earlier?
UpdateWindow(hwndApp);

// restore previous connections
if ( fNetDDEActive )
   RestoreAllSavedConnections();

return TRUE;
}

static VOID StripCharRange (
TCHAR *s,
TCHAR lower,
TCHAR upper)
{
TCHAR *p = s, *q = s;

do {
   if ( *p < lower || *p > upper )
      *q++ = *p;
   } while ( *p++ );
}


//
// Purpose: Message handler for WM_DRAWCLIPBOARD
//
// Params:
//    hwnd - Window handle
/////////////////////////////////////////////////////////////////////////////
void OnDrawClipboard(
HWND hwnd)
{
UINT wNewFormat;
UINT wOldFormat;

// If we are in a transaction, defer processing this message
// until the next unlock - if we did this now we could cause
// other apps to break...

// clipboard may have been empty and now isn't
InitializeMenu ( GetMenu(hwnd) );

if ( fAppLockedState )
   {
   fClipboardNeedsPainting = TRUE;
   }
else
   {
   fClipboardNeedsPainting = FALSE;

   // Update the popup menu entries.
   // UpdateCBMenu( hwnd, hwndClpbrd ); ... NOT

   if (IsWindow ( hwndClpbrd ))
      {
      wOldFormat = GetBestFormat( hwndClpbrd,
         GETMDIINFO(hwndClpbrd)->CurSelFormat );

      GETMDIINFO(hwndClpbrd)->CurSelFormat = CBM_AUTO;
      GETMDIINFO(hwndClpbrd)->fDisplayFormatChanged = TRUE;

      wNewFormat = GetBestFormat( hwndClpbrd, CBM_AUTO );

      // NOTE OwnerDisplay stuff applies only to the "real" clipboard!

      ShowHideControls(hwndClpbrd);

      if (wOldFormat == CF_OWNERDISPLAY)
         {
         /* Save the owner Display Scroll info */
         SaveOwnerScrollInfo(hwndClpbrd);
         ShowScrollBar ( hwndClpbrd, SB_BOTH, FALSE );
         ResetScrollInfo( hwndClpbrd );
         InvalidateRect ( hwndClpbrd, NULL, TRUE );
         }
      else
         {
         if (wNewFormat == CF_OWNERDISPLAY)
            {
            /* Restore the owner display scroll info */
            ShowHideControls(hwndClpbrd);
            ShowWindow ( pActiveMDI->hwndSizeBox, SW_HIDE );
            RestoreOwnerScrollInfo(hwndClpbrd);
            InvalidateRect ( hwndClpbrd, NULL, TRUE );
            }
         else
            {
            // Change the character dimensions based on the format.
            ChangeCharDimensions(hwndClpbrd, wOldFormat, wNewFormat);

            // Initialize the owner display scroll info, because the
            // contents have changed.
            InitOwnerScrollInfo();

            // Force a total repaint. fOwnerDisplay gets updated during
            // a total repaint.
            InvalidateRect(hwndClpbrd, NULL, TRUE);
            ResetScrollInfo(hwndClpbrd);

            // force update here BEFORE sending on WM_DRAWCLIPBOARD
            UpdateWindow (hwndClpbrd);
            }
         }
      }
   }

// Pass the message on to the next clipboard viewer in the chain.
if (hwndNextViewer != NULL)
   {
   SendMessage(hwndNextViewer, WM_DRAWCLIPBOARD, 0, 0);
   }
}

LRESULT OnEraseBkgnd(
HWND hwnd,
HDC hdc)
{
// if (IsIconic(hwnd))
//    return(TRUE);
// else
   return DefMDIChildProc(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0L);
}

LRESULT OnPaint(
HWND hwnd)
{
PMDIINFO pMDI;
PAINTSTRUCT ps;

pMDI = GETMDIINFO(hwnd);
BeginPaint(hwnd, &ps);

if ( IsIconic ( hwnd ))
   {
//   DefMDIChildProc(hwnd, WM_ERASEBKGND, (WPARAM)ps.hdc, 0L);

   if ( pMDI->flags & F_CLPBRD )
      {
      DrawIcon ( ps.hdc, 0, 0, hicClipbrd);
      }
   else if ( pMDI->flags & F_LOCAL )
      {
      DrawIcon ( ps.hdc, 0, 0, hicClipbook);
      }
   else
      {
      DrawIcon ( ps.hdc, 0, 0, hicRemote);
      }
   }
else if ( pMDI->DisplayMode == DSP_PAGE )
   {
   HPALETTE hpal, hpalT;
   HBRUSH   hbr;

   // Fill background with proper color - DefMDIChildProc fills with
   // app_workspace.
   hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
   FillRect(ps.hdc, &ps.rcPaint, hbr);
   DeleteObject(hbr);

   if ( !fAppShuttingDown )
      {
      if ( VOpenClipboard( pMDI->pVClpbrd, hwnd))
         {
         SetBkColor(ps.hdc, GetSysColor(COLOR_WINDOW));
         SetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWTEXT));

         if (hpal = VGetClipboardData( pMDI->pVClpbrd, CF_PALETTE))
            {
            PINFO(TEXT("Palette found, selecting & realizing\r\n"));
            hpalT = SelectPalette(ps.hdc, hpal, pMDI != pActiveMDI );
            RealizePalette(ps.hdc);
            }

         DrawStuff( hwnd, &ps, hwnd );

         if (hpal)
            {
            // We don't want to put the DEFAULT palette in the foreground.
            SelectPalette(ps.hdc, hpalT, FALSE);
            }

         VCloseClipboard( pMDI->pVClpbrd );
         }
#if DEBUG
      else
         {
         SetStatusBarText("Clipboard changed but could not open");
         }
#endif
      }
   }
EndPaint(hwnd, &ps);
return 0L;
}

// window procedures
LRESULT CALLBACK FrameWndProc(
HWND hwnd,
UINT msg,
WPARAM wParam,
LPARAM lParam)
{
   UINT wNewFormat;
   UINT wOldFormat;
   int tmp;

   // PINFO(TEXT("FrameWnd Msg: %u %ld %ld\r\n"), msg, wParam, lParam);

   switch (msg)
      {
   case WM_CREATE:
      {

      CLIENTCREATESTRUCT ccs;
      RECT rc;

      /* Find window menu where children will be listed */
      ccs.hWindowMenu = GetSubMenu (GetMenu(hwnd), WINDOW_MENU_INDEX );
      ccs.idFirstChild = 4100; // IDM_WINDOWCHILD;

      // initialize special case clipboard formats...
      // note that CF_BITMAP, CF_METAFILEPICT, CF_PALETTE are
      // re-registered in a private format because data for these
      // formats is exchanged between this app and clipsrv.exe in
      // a flat DDEML data handle - not the regular DDE interpretation
      // of a handle that carries those format IDs

      if ( LoadString ( hInst, CF_BITMAP, szBuf, SZBUFSIZ ))
         cf_bitmap = RegisterClipboardFormat ( szBuf );
      if ( LoadString ( hInst, CF_METAFILEPICT, szBuf, SZBUFSIZ ))
         cf_metafilepict = RegisterClipboardFormat ( szBuf );
      if ( LoadString ( hInst, CF_PALETTE, szBuf, SZBUFSIZ ))
         cf_palette = RegisterClipboardFormat ( szBuf );

      cf_preview        = RegisterClipboardFormat (SZPREVNAME);
      cf_link           = RegisterClipboardFormat(SZLINK);
      cf_linkcopy       = RegisterClipboardFormat (SZLINKCOPY);
      cf_objectlink     = RegisterClipboardFormat(SZOBJECTLINK);
      cf_objectlinkcopy = RegisterClipboardFormat (SZOBJECTLINKCOPY);

      CreateTools( hwnd );   // creates toolbar window, brushes, etc.

      // determine height of toolbar window and save...
      GetClientRect ( hwndToolbar, &rc );
      dyButtonBar = rc.bottom - rc.top;

      // determine height of statusbar window and save...
      GetClientRect ( hwndStatus, &rc );
      dyStatus = rc.bottom - rc.top;

      // Create the MDI client - will be sized later
      hwndMDIClient = CreateWindow (TEXT("mdiclient"),
              NULL,
               WS_BORDER | WS_CHILD | WS_CLIPCHILDREN | MDIS_ALLCHILDSTYLES|
              WS_HSCROLL | WS_VSCROLL,
              0,
               0,
               0,
               0,
               hwnd,
               (HMENU)0xCAC,
               hInst,
              (LPVOID)&ccs);
      ShowWindow ( hwndMDIClient, SW_SHOW );

      }
      break;

   case WM_QUERYNEWPALETTE:
       // Tell the active document to realize in foreground.
      if ( hwndActiveChild )
         tmp = (WORD)SendMessage(hwndActiveChild, WM_QUERYNEWPALETTE,0, 0L);
      else
         break;

       // If mapping is unchanged, other documents could still change,
       // so give them a change to realize.
       if (!tmp)
         SendMessageToKids(WM_PALETTECHANGED, (WPARAM)hwndActiveChild, 0L);
       return(tmp);
       break;

   // System palette has changed, so pass it on to the children.
   case WM_PALETTECHANGED:
       SendMessageToKids(WM_PALETTECHANGED, wParam, lParam);
       break;

   case WM_MENUSELECT:
      PINFO(TEXT("MenuSelect %lx\r\n"), wParam);

      // no context menu help for popup entries
      if ( HIWORD(wParam) & MF_POPUP )
         {
         dwCurrentHelpId = 0;
         }
      else if ( HIWORD(wParam) & MF_SYSMENU )
         {
         dwCurrentHelpId = IDH_SYSMENU;
         }
      else
         {
         // We don't care if the menuitem's disabled, checked, whatever...
         wParam = LOWORD(wParam);

         // was this a dynamically added clipboard entry?
         if (( wParam >= 0xc000 && wParam <= 0xffff ||   // registerd format?
            wParam >= CF_TEXT && wParam <= CF_ENHMETAFILE || // intrinsic format?
            wParam >= CF_OWNERDISPLAY && wParam <= CF_DSPMETAFILEPICT )

            // gotta exclude sc_ stuff - overlaps with formats
            && ! ( wParam >= SC_SIZE && wParam <= SC_HOTKEY ) )
            {
            GetMenuString ( GetMenu(hwnd), wParam,
               szBuf2, SZBUFSIZ, MF_BYCOMMAND );
            StripCharRange ( szBuf2, TEXT('&'), TEXT('&') );
            wsprintf( szBuf, szViewHelpFmt, szBuf2 );
            SendMessage( hwndStatus, SB_SETTEXT, SBT_NOBORDERS|255,
               (LPARAM)szBuf );
            dwCurrentHelpId = IDH_FORMATS;
            break;
            }

         if ( wParam >= 4100 && wParam <= 4200 )
            {
            GetMenuString (GetMenu(hwnd), wParam,szBuf2,SZBUFSIZ, MF_BYCOMMAND);
            StripCharRange ( szBuf2, TEXT('&'), TEXT('&') );
            StripCharRange ( szBuf2, TEXT('0'), TEXT('9') );
            wsprintf( szBuf, szActivateFmt, *szBuf2 == TEXT(' ') ? szBuf2 + 1 :
               szBuf2 );
            SendMessage( hwndStatus, SB_SETTEXT, SBT_NOBORDERS|255,
               (LPARAM)szBuf );
            dwCurrentHelpId = IDH_NAMESWIND;
            break;
            }

         dwCurrentHelpId = IDH_BASE + wParam;

         MenuHelp( (WORD)msg, wParam, lParam, GetMenu(hwnd), hInst,
            hwndStatus, nIDs );
         }
      break;

   case WM_F1DOWN:
      PINFO(TEXT("Help on context %ld\r\n"), dwCurrentHelpId);
      if ( dwCurrentHelpId )
         {
         WinHelp(hwndApp, szHelpFile, HELP_CONTEXT, dwCurrentHelpId );
         }
      else
         {
         WinHelp(hwndApp, szHelpFile, HELP_INDEX,  0L );
         }
      DrawMenuBar(hwndApp);
      break;

   case WM_DRAWITEM:
      HandleOwnerDraw( hwnd, wParam, lParam );
      break;

   case WM_INITMENU:
      InitializeMenu ((HMENU)wParam);
      UpdateCBMenu ( hwnd, hwndActiveChild );
      break;

   case WM_SYSCOLORCHANGE:
      DeleteTools( hwnd );
      CreateTools( hwnd );
      break;

   case WM_COMMAND:
      return ClipBookCommand ( hwnd, msg, wParam, lParam );

   case WM_CLOSE:

#if DEBUG
      if ( fAppLockedState )
         PERROR(TEXT("Very bad: WM_CLOSE while locked\n\r"));
#endif

      // force all clipboard formats rendered before exiting
      // so we don't end up yielding in WM_RENDERALLFORMATS
      // and get into trouble.

      fAppShuttingDown = TRUE;

      ForceRenderAll(hwnd, (PVCLPBRD)NULL );
      PostMessage(hwnd, WM_CLOSE_REALLY, 0, 0L );
      WinHelp(hwnd, szHelpFile, HELP_QUIT, 0L);
      break;

   case WM_CLOSE_REALLY:
      // this is necessary to avoid processing messages in our
      // queue when we yield getting the clipboard data in
      // ForceRenderAll and destroying the app prematurely
      return DefFrameProc (hwnd,hwndMDIClient,WM_CLOSE,0,0L);

   case WM_DESTROY:

#if DEBUG
      if ( fAppLockedState )
         {
         PERROR(TEXT("Very bad: WM_DESTROY while locked\n\r"));
         }
#endif

      // Take us out of the viewer chain
      ChangeClipboardChain(hwnd, hwndNextViewer);

      DeleteTools ( hwnd );

      Wpl.length = sizeof ( Wpl );
      Wpl.flags = 0;
      GetWindowPlacement ( hwnd, &Wpl );
      SaveWindowPlacement ( &Wpl );

      if (hkeyRoot != NULL)
         {
         RegSetValueEx(hkeyRoot, szStatusbar, 0L, REG_DWORD,
               (LPBYTE)&fStatus, sizeof(fStatus));
         RegSetValueEx(hkeyRoot, szToolbar, 0L, REG_DWORD,
               (LPBYTE)&fToolBar, sizeof(fToolBar));
         RegSetValueEx(hkeyRoot, szShPref, 0L, REG_DWORD,
               (LPBYTE)&fSharePreference, sizeof(fSharePreference));
         }

      PostQuitMessage (0);
      break;

   case WM_DRAWCLIPBOARD:
      OnDrawClipboard(hwnd);
      break;

   case WM_CHANGECBCHAIN:

      if (hwndNextViewer == NULL)
         return(FALSE);

      if ( (HWND)wParam == hwndNextViewer)
         {
         hwndNextViewer = (HWND)lParam;
         return(TRUE);
         }
      return(SendMessage(hwndNextViewer, WM_CHANGECBCHAIN, wParam, lParam));

   case WM_RENDERALLFORMATS:

      // WM_DESTROY follows close on the heels of this message, and
      // we will process it and die while another copy of FrameWndProc
      // is in sync DDEML transaction...
      //
      // Note that we now attempt to render all formats in WM_DESTROY
      // before doing the PostQuitMessage so we should not have to
      // respond to this message.

      break;

   case WM_RENDERFORMAT:
      {
      HDDEDATA hListData;
      HDDEDATA hFmtData;
      HSZ hszFmt;
      LPTSTR lpszList = TEXT("");
      LPTSTR p, q;
      DWORD cbDataLen;
      UINT RealFmt;
      TCHAR c;

      PINFO(TEXT("Frame WM_RENDERFORMAT: %d\r\n"),wParam);

      if ( !IsWindow(hwndClpOwner))
         {
         PERROR(TEXT("Strange?: null clipboard owner window!\n\r"));
         break;
         }

      DdeKeepStringHandle ( idInst, hszFormatList );

      hListData = MySyncXact ( NULL, 0L,
         GETMDIINFO(hwndClpOwner)->hClpConv, hszFormatList,
         CF_TEXT, XTYP_REQUEST, SHORT_SYNC_TIMEOUT, NULL );


      if ( !hListData )
         {
         PERROR(TEXT("WM_RENDERFORM: REQUEST for formatlist failed: %x\n\r"),
            DdeGetLastError(idInst) );
         MessageBoxID ( hInst, hwnd, IDS_DATAUNAVAIL, IDS_APPNAME,
            MB_OK | MB_ICONEXCLAMATION );
         break;
         }

      lpszList = (LPTSTR)DdeAccessData ( hListData, &cbDataLen );

      if ( !lpszList )
         {
         PERROR(TEXT("WM_RENDERFORM: DdeAccessData failed!\n\r"));
         MessageBoxID ( hInst, hwnd, IDS_DATAUNAVAIL, IDS_APPNAME,
            MB_OK | MB_ICONEXCLAMATION );
         break;
         }

      for (q = _tcstok(lpszList, TEXT("\t"));q;q = _tcstok(NULL, TEXT("\t")))
         {
         RealFmt = MyGetFormat ( q, GETFORMAT_DONTLIE );

         if ( wParam == RealFmt || msg == WM_RENDERALLFORMATS )
            {
            PINFO(TEXT("Getting format %d\r\n"), RealFmt);

            hszFmt = DdeCreateStringHandle ( idInst, q, 0 );

            hFmtData = MySyncXact ( NULL, 0L,
               GETMDIINFO(hwndClpOwner)->hClpConv,
               hszFmt, MyGetFormat ( q, GETFORMAT_LIE ),
               XTYP_REQUEST, LONG_SYNC_TIMEOUT, NULL );

            if ( hFmtData )
               {
               SetClipboardFormatFromDDE ( hwndClpbrd, RealFmt , hFmtData );
               // VSetClipboardData(NULL, RealFmt, hFmtData);
               }
            else
               {
               PERROR(TEXT("REQUEST for %s failed %x\n\r"),
                  q, DdeGetLastError(idInst) );
               }
            }
         }

      DdeFreeDataHandle ( hListData );
      break;
      }

   case WM_SIZE:
      SendMessage ( hwndToolbar, WM_SIZE, 0, 0L );
      SendMessage ( hwndStatus, WM_SIZE, 0, 0L );
      AdjustMDIClientSize();
      break;

   case WM_PARENTNOTIFY:
      // PINFO(TEXT("Recieved WM_PARENTNOTIFY %d %ld\r\n"), wParam, lParam);
      break;

   default:
      return DefFrameProc (hwnd,hwndMDIClient,msg,wParam,lParam);
   }
return 0;
}

extern LRESULT OnPaint(HWND);
extern LRESULT OnEraseBkgnd(HWND, HDC);

//////////////////////////////////////////////////////////////////////
LRESULT CALLBACK ChildWndProc(
HWND hwnd,
UINT msg,
WPARAM wParam,
LPARAM lParam)
{
   LPMEASUREITEMSTRUCT   lpmisCtl;
   PAINTSTRUCT ps;
   HDC hdc;
   int tmp;
   PMDIINFO pMDI;
   int i;
   HPALETTE hCurrentPal, hOldPal;

// PERROR(TEXT("ChildWndProc msg: %u %ld %ld\r\n"),msg, wParam, lParam);

   switch (msg) {

   case WM_MDIACTIVATE:
      SendMessage(hwndMDIClient, WM_MDIREFRESHMENU, 0, 0);
      DrawMenuBar(hwndApp);
      if ( (HWND)lParam != hwnd )
         break;
      // fall through

   case WM_SETFOCUS:
      hwndActiveChild = hwnd;
      pActiveMDI = GETMDIINFO(hwndActiveChild);

      if ( pActiveMDI->DisplayMode != DSP_PAGE &&
            IsWindow(  pActiveMDI->hWndListbox ))
         {
         SetFocus (  pActiveMDI->hWndListbox );
         }
      else
         {
         SetFocus (  hwndActiveChild );
         }

      InitializeMenu( GetMenu(hwndApp) );
      UpdateNofMStatus(hwndActiveChild);
      return (DefMDIChildProc(hwnd, msg, wParam, lParam));

   case WM_LBUTTONDBLCLK:
      if ( GETMDIINFO(hwnd)->DisplayMode == DSP_PAGE &&
                     !( GETMDIINFO(hwnd)->flags & F_CLPBRD ))
         {
         if ( GETMDIINFO(hwnd)->OldDisplayMode == DSP_LIST )
            SendMessage ( hwndApp, WM_COMMAND, IDM_LISTVIEW, 0L );
         else if ( GETMDIINFO(hwnd)->OldDisplayMode == DSP_PREV )
            SendMessage ( hwndApp, WM_COMMAND, IDM_PREVIEWS, 0L );
         }
      break;

   case WM_PALETTECHANGED:
       if (hwnd == (HWND)wParam)
         break;

   case WM_QUERYNEWPALETTE:

      if ( GETMDIINFO(hwnd)->DisplayMode != DSP_PAGE )
         return 0;

      i = 0;
      if ( VOpenClipboard( GETMDIINFO(hwnd)->pVClpbrd, hwnd )) {
         if ( hCurrentPal = VGetClipboardData( GETMDIINFO(hwnd)->pVClpbrd,
               CF_PALETTE)) {
            hdc = GetDC(hwnd);
            hOldPal = SelectPalette(hdc, hCurrentPal,
                        (msg == WM_QUERYNEWPALETTE) ? FALSE : TRUE);
            i = RealizePalette(hdc);

            SelectPalette(hdc, hOldPal, TRUE);
            RealizePalette(hdc);
            ReleaseDC(hwnd, hdc);
            if (i)
               InvalidateRect(hwnd, NULL, TRUE);
         }
         VCloseClipboard( GETMDIINFO(hwnd)->pVClpbrd );
      }
       return(i);
       break;

   case WM_MENUSELECT:

      MenuHelp(
         (WORD)msg,
         wParam,
         lParam,
         GetMenu(hwndApp),
         hInst,
         hwndStatus,
         nIDs );
      break;

   case WM_CREATE:

      if ( ( pMDI = (LPMDIINFO)GlobalAllocPtr(GPTR, sizeof(MDIINFO))) == NULL )
         {
         PERROR(TEXT("MdiInfo alloc failed\n\r"));
         break;
         }

      SetWindowLong ( hwnd, GWL_MDIINFO, (long)pMDI );
      pMDI->DisplayMode = DSP_LIST;
      pMDI->hExeConv = 0L;
      pMDI->hClpConv = 0L;
      pMDI->hVClpConv = 0L;
      pMDI->flags = 0L;
      pMDI->CurSelFormat = CBM_AUTO;
      pMDI->cyScrollLast = -1L;
      pMDI->cxScrollLast = -1;
      pMDI->cyScrollNow = 0L;
      pMDI->cxScrollNow = 0;
      pMDI->pVClpbrd = NULL;
      pMDI->hszConvPartner = 0L;
      pMDI->hszConvPartnerNP = 0L;
      pMDI->hszClpTopic = 0L;
      pMDI->fDisplayFormatChanged = TRUE;
      pMDI->hWndListbox = CreateWindow(TEXT("listbox"),
                     szNull,
                     WS_CHILD | LBS_STANDARD | LBS_NOINTEGRALHEIGHT |
                     LBS_LISTVIEW,
                     0, 0, 100, 100,   // non zero size to work around
                                 // possible USER bug which results
                                 // in divide by zero
                     hwnd,
                     (HMENU)ID_LISTBOX,
                     hInst,
                     0L );

      // create the scroll bars
      pMDI->hwndVscroll = CreateWindowW(L"scrollbar", L"",
         WS_CHILD|SBS_VERT, 0, 0, 0, 0, hwnd, (HMENU)ID_VSCROLL,
         hInst, 0L );
      pMDI->hwndHscroll = CreateWindowW(L"scrollbar", L"",
         WS_CHILD|SBS_HORZ, 0, 0, 0, 0, hwnd, (HMENU)ID_VSCROLL,
         hInst, 0L );

      // create the stupid corner size box
      pMDI->hwndSizeBox = CreateWindowW(L"scrollbar",L"",
         WS_CHILD|SBS_SIZEBOX,
         0, 0, 0, 0, hwnd, (HMENU)ID_SIZEBOX, hInst, 0L );

      // create the page fwd/bkwd buttons
      pMDI->hwndPgUp = CreateWindowW(L"button", L"",
         WS_CHILD | BS_OWNERDRAW,
         0, 0, 0, 0, hwnd, (HMENU)ID_PAGEUP, hInst, 0L );
      pMDI->hwndPgDown = CreateWindowW(L"button", L"",
         WS_CHILD | BS_OWNERDRAW,
         0, 0, 0, 0, hwnd, (HMENU)ID_PAGEDOWN, hInst, 0L );

      SetCharDimensions( hwnd, GetStockObject ( SYSTEM_FONT ));
      break;

   case WM_VSCROLL:
      if (wParam != SB_THUMBTRACK)
         {
         if (fOwnerDisplay)
            SendOwnerMessage(WM_VSCROLLCLIPBOARD,
               (WPARAM)hwnd, (LPARAM)wParam);
         else
            ClipbrdVScroll(hwnd, LOWORD(wParam), LOWORD(lParam));
         }
      break;

   case WM_HSCROLL:
      if (wParam != SB_THUMBTRACK)
         {
         if (fOwnerDisplay)
            SendOwnerMessage(WM_HSCROLLCLIPBOARD,
               (WPARAM)hwnd, (LPARAM)wParam);
         else
            ClipbrdHScroll(hwnd, LOWORD(wParam), LOWORD(lParam));
         }
      break;

   // case WM_ERASEBKGND:
   //    break;
   //    return OnEraseBkgnd(hwnd,(HDC)wParam);

   case WM_QUERYDRAGICON:
      if ( GETMDIINFO(hwnd)->flags & F_CLPBRD )
         return (LRESULT)hcurClipbrd;
      else if ( GETMDIINFO(hwnd)->flags & F_LOCAL )
         return (LRESULT)hcurClipbook;
      else
         return (LRESULT)hcurRemote;

   case WM_CLOSE:
      if ( ! ( GETMDIINFO(hwnd)->flags & ( F_CLPBRD | F_LOCAL ) ) )
         {
         PINFO(TEXT("removing reconn for '%s'\n\r"),
            GETMDIINFO(hwnd)->szBaseName );
         // CharToOem ( GETMDIINFO(hwnd)->szBaseName, szBuf );

         if (NULL != hkeyRoot)
            {
            lstrcpy(szBuf, GETMDIINFO(hwnd)->szBaseName);
            lstrcat(szBuf, szConn);
            RegDeleteValue(hkeyRoot, szBuf);

            lstrcpy(szBuf, GETMDIINFO(hwnd)->szBaseName);
            lstrcat(szBuf, szWindows);
            RegDeleteValue(hkeyRoot, szBuf);
            }
         }
      WinHelp(hwnd, szHelpFile, HELP_QUIT, 0L);
      return(DefMDIChildProc(hwnd, msg, wParam, lParam));

   case WM_PAINT:
      return OnPaint(hwnd);

   case WM_KEYDOWN:
   {
      WORD      sb;

      pMDI = GETMDIINFO(hwnd);

      if ( pMDI->DisplayMode != DSP_PAGE )
         return (DefMDIChildProc(hwnd, msg, wParam, lParam));

      switch (wParam) {
      case VK_UP:
         sb = SB_LINEUP;
         goto VertScroll;
      case VK_DOWN:
         sb = SB_LINEDOWN;
         goto VertScroll;
      case VK_PRIOR:
         sb = SB_PAGEUP;
         goto VertScroll;
      case VK_NEXT:
         sb = SB_PAGEDOWN;

      VertScroll:
         SendMessage(hwnd, WM_VSCROLL, sb, 0L);
         break;

      case VK_LEFT:
         sb = SB_LINEUP;
         goto HorzScroll;
      case VK_RIGHT:
         sb = SB_LINEDOWN;
         goto HorzScroll;
      case VK_TAB:
         sb = (GetKeyState( VK_SHIFT ) < 0) ? SB_PAGEUP : SB_PAGEDOWN;
      HorzScroll:
         SendMessage( hwnd, WM_HSCROLL, sb, 0L);
         break;

      default:
         return (DefMDIChildProc(hwnd, msg, wParam, lParam));
      }
      break;
   }

   case WM_SIZE:

      pMDI = GETMDIINFO(hwnd);

      AdjustControlSizes( hwnd );
      pMDI->fDisplayFormatChanged = TRUE;
      InvalidateRect ( hwnd, NULL, FALSE );

      if ( pMDI->DisplayMode == DSP_PAGE )
         ResetScrollInfo ( hwnd );

      return (DefMDIChildProc(hwnd, msg, wParam, lParam));

   case WM_DRAWITEM:

      HandleOwnerDraw( hwnd, wParam, lParam );
      break;

   case WM_COMPAREITEM:

      if ( wParam != ID_LISTBOX )
         break;

      tmp = lstrcmpi (
         &((LPLISTENTRY)((LPCOMPAREITEMSTRUCT)lParam)->itemData1)->name[1],
         &((LPLISTENTRY)((LPCOMPAREITEMSTRUCT)lParam)->itemData2)->name[1]);

      if ( tmp < 0 )
         {
         return -1;
         }
      else if (tmp > 0)
         {
         return 1;
         }
      else
         {
         return 0;
         }
      break;

   case WM_DELETEITEM:

      if ( wParam != ID_LISTBOX )
         break;

      // if item is marked for saving (for a new listbox), dont delete
      if ( ((LPLISTENTRY)((LPDELETEITEMSTRUCT)lParam)->itemData)->fDelete
                        == FALSE )
         break;

      // delete preview bmp if there is one
      if (((LPLISTENTRY)((LPDELETEITEMSTRUCT)lParam)->itemData)->hbmp)
         DeleteObject (
         ((LPLISTENTRY)((LPDELETEITEMSTRUCT)lParam)->itemData)->hbmp);

      GlobalFreePtr( (LPVOID)((LPDELETEITEMSTRUCT)lParam)->itemData );
      break;

   case WM_MEASUREITEM:

      lpmisCtl = (MEASUREITEMSTRUCT *) lParam;

      switch ( wParam ) {
      case ID_LISTBOX:

         if ( GETMDIINFO(hwnd)->DisplayMode == DSP_LIST )
            lpmisCtl->itemHeight = max( LSTBTDY, dyPrevFont + 1);
         else
            lpmisCtl->itemHeight = 3*dyPrevFont + PREVBMPSIZ + 2*PREVBRD;
         break;
      case ID_PAGEUP:
      case ID_PAGEDOWN:
         lpmisCtl->itemWidth = GetSystemMetrics ( SM_CXHSCROLL );
         lpmisCtl->itemHeight = GetSystemMetrics ( SM_CYVSCROLL );
         break;
      }
      break;

   case WM_COMMAND:
      switch (LOWORD(wParam))
         {
      case ID_LISTBOX:

         if ( !( IsWindow( GETMDIINFO(hwnd)->hWndListbox)))
            break;

         // InitializeMenu(GetMenu(hwndApp));

         switch(HIWORD(wParam))
            {
         case LBN_SETFOCUS:
         case LBN_SELCHANGE:
            UpdateNofMStatus(hwnd);
            InitializeMenu( GetMenu(hwndApp));
            break;
         case LBN_SELCANCEL:
         case LBN_KILLFOCUS:
            //UpdateNofMStatus(NULL);
            break;
         case LBN_DBLCLK:
            // Double-clicks cause me to go to page view
            //SendMessage ( hwndApp, WM_COMMAND, IDM_COPY, 0L );
            SendMessage ( hwndApp, WM_COMMAND, IDM_PAGEVIEW, 0L );
            break;
            }
         break;

      case ID_PAGEUP:
      case ID_PAGEDOWN:
         SendMessage ( hwndApp, WM_COMMAND, wParam, 0L );
         break;

      default:
         return(DefMDIChildProc(hwnd, WM_COMMAND, wParam, lParam));
         }
      break;

   case WM_SYSCOMMAND:
      // The Close menuitem on the system menus of the clipboard and
      // local clipbook windows should be greyed, so we shouldn't get
      // that message.
      switch ( wParam )
         {
      case SC_CLOSE:
         // don't allow close of local or clipboard
         if ( GETMDIINFO(hwnd)->flags & ( F_LOCAL | F_CLPBRD ) )
            wParam = SC_MINIMIZE;
         break;
      default:
         break;
         }
      return DefMDIChildProc(hwnd, msg, wParam, lParam );

   case WM_DESTROY:

      pMDI = GETMDIINFO(hwnd);

      DdeDisconnect( pMDI->hExeConv );

      if ( pMDI->hClpConv )
         DdeDisconnect ( pMDI->hClpConv );
      if ( pMDI->hVClpConv )
         DdeDisconnect ( pMDI->hVClpConv );
      if ( pMDI->hszConvPartner )
         DdeFreeStringHandle ( idInst, pMDI->hszConvPartner );
      if ( pMDI->hszConvPartnerNP )
         DdeFreeStringHandle ( idInst, pMDI->hszConvPartnerNP );
      if ( pMDI->hszClpTopic )
         DdeFreeStringHandle ( idInst, pMDI->hszClpTopic );
      if ( pMDI->hszVClpTopic )
         DdeFreeStringHandle ( idInst, pMDI->hszVClpTopic );

      if ( pMDI->pVClpbrd )
         DestroyVClipboard ( pMDI->pVClpbrd );

      if ( hwnd == hwndLocal )
         hwndLocal = NULL;
      if ( hwnd == hwndClpbrd )
         hwndClpbrd = NULL;

      // free up the MDI info struct
      GlobalFree ( (HGLOBAL)pMDI );

      break;

   default:
      return (DefMDIChildProc(hwnd, msg, wParam, lParam));
   }
   return 0L;
}

/****************************************************************************
 *
 *  FUNCTION   : SendMessageToKids
 *
 *  PURPOSE    : Send the given message with the given parameters to all
 *               of the MDI child windows.
 *
 *  RETURNS    : None.
 *
 ****************************************************************************/

VOID SendMessageToKids (WORD msg, WPARAM wParam, LPARAM lParam)
{
    register HWND hwndT;

    hwndT = GetWindow (hwndMDIClient, GW_CHILD);
    while (hwndT) {
      SendMessage (hwndT, msg, wParam, lParam);
      hwndT = GetWindow(hwndT, GW_HWNDNEXT);
    }
}

static BOOL fClpOpen;

BOOL SyncOpenClipboard(
HWND hwnd)
{
BOOL fOK;

if (!fClpOpen)
   {
   // PINFO(TEXT("\r\nClipbook: Opening Clipboard\r\n"));

   WaitForSingleObject(hmutexClp, INFINITE);
   fOK = OpenClipboard(hwnd);
   if (!fOK)
      {
      PERROR(TEXT("OpenClipboard failed\r\n"));
      ReleaseMutex(hmutexClp);
      }
   else
      {
      fClpOpen = TRUE;
      }

   return fOK;
   }
else
   {
   PERROR(TEXT("Attempt at opening clipboard twice!\r\n"));
   return(FALSE);
   }
}

BOOL SyncCloseClipboard(
void)
{
BOOL fOK;

// PINFO(TEXT("\r\nClipbook: Closing Clipboard\r\n"));

fOK = CloseClipboard();
ReleaseMutex(hmutexClp);

if (!fOK)
   {
   PERROR(TEXT("CloseClipboard failed\r\n"));
   }

fClpOpen = FALSE;

return fOK;
}

// REVIEWPT
