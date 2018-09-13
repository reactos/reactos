
/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991,1992                    **/
/***************************************************************************/
// ts=4

// CLIPBOOK.H - ClipBook viewer main include file
// 11-91 clausgi created

#include   <commdlg.h>
#include   <nddeapi.h>
#include   "ddeml.h"
#include   "vclpbrd.h"

// switched debug function
#if   DEBUG
#define   PERROR   if(DebugLevel>0)DebOut
#define   PINFO    if(DebugLevel>1)DebOut
extern void DumpDdeInfo(PNDDESHAREINFO, LPTSTR);
extern void PrintSid(PSID);
extern void PrintSD(PSECURITY_DESCRIPTOR);
#else
#define   PERROR(x)
#define   PINFO(x)
#define   DumpDdeInfo(x,y)
#define PrintSid(x)
#define PrintSD(x)
#endif

#define SetStatusBarText(x) if(hwndStatus)SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)(LPTSTR)(x));

// Custom resource types
#define      ENVSTR 256

// resource ID's
#define      IDACCELERATORS         1
#define      IDFRAMEICON            2
#define      IDI_CLIPBRD            3
#define      IDI_CLIPBOOK           4
#define      IDI_REMOTE             5
#define      IDBITMAP               6
#define      IDSTATUS               7
#define      IDCVMENU               8
#define      IBM_UPARROW            9
#define      IBM_DNARROW           10
#define      IBM_UPARROWD          11
#define      IBM_DNARROWD          12
#define      IDLOCKICON            13
#define      IDSHAREICON           14
#define      IDC_TOOLBAR          401

#define      IDC_CLIPBOOK          16
#define      IDC_CLIPBRD           17
#define      IDC_REMOTE            18

// user defined messages
#define      WM_CLOSE_REALLY    WM_USER
#define      WM_F1DOWN         (WM_USER + 1)

// menuhelp constants
#define MH_BASE        0x1000
#define MH_POPUPBASE   0x1100

/////////////////////////////////////////////
//
//   Data Structures and typedefs
//
//////////////////////////////////////////////

/////////////////////
// PER MDI CLIENT DATA

struct MdiInfo {
   HCONV   hExeConv;
   HCONV   hClpConv;
   HSZ     hszClpTopic;
   HSZ     hszConvPartner;
   HSZ     hszConvPartnerNP;
   HWND    hWndListbox;
   WORD    DisplayMode;
   WORD    OldDisplayMode;
   DWORD   flags;
   TCHAR   szBaseName[MAX_COMPUTERNAME_LENGTH + 1];

   UINT   CurSelFormat;
   LONG   cyScrollLast;
   LONG   cyScrollNow;
   int    cxScrollLast;
   int    cxScrollNow;
   RECT   rcWindow;
   WORD   cyLine, cxChar, cxMaxCharWidth; // Size of a standard text char
   WORD   cxMargin, cyMargin;      // White border size around clip data area
   BOOL   fDisplayFormatChanged;

   PVCLPBRD pVClpbrd;
   HCONV    hVClpConv;
   HSZ      hszVClpTopic;

   // scrollbars, etc. for the damn paging control

   HWND   hwndVscroll;
   HWND   hwndHscroll;
   HWND   hwndSizeBox;
   HWND   hwndPgUp;
   HWND   hwndPgDown;
};

typedef struct MdiInfo   MDIINFO;
typedef struct MdiInfo * PMDIINFO;
typedef struct MdiInfo FAR * LPMDIINFO;

////////////////////////////////
// data request record

#define      RQ_PREVBITMAP   10
#define      RQ_COPY         11
#define      RQ_SETPAGE      12
#define      RQ_EXECONV      13

struct DataRequest_tag
   {
   WORD   rqType;      // one of above defines
   HWND   hwndMDI;
   HWND   hwndList;
   UINT   iListbox;
   BOOL   fDisconnect;
   WORD   wFmt;
   };

typedef struct DataRequest_tag DATAREQ;
typedef struct DataRequest_tag * PDATAREQ;

////////////////////////////////
// Owner draw listbox data struct
#define MAX_PAGENAME_LENGTH 64

struct ListEntry_tag
   {
   TCHAR name[MAX_PAGENAME_LENGTH + 1];
   HBITMAP hbmp;
   BOOL fDelete;
   BOOL fTriedGettingPreview;
   };

typedef struct ListEntry_tag LISTENTRY;
typedef struct ListEntry_tag * PLISTENTRY;
typedef struct ListEntry_tag FAR * LPLISTENTRY;


// extra window data for MDI child registerclass
// contains a pointer to above MDIINFO struct
#define   GWL_MDIINFO      0
#define   CBWNDEXTRA       sizeof(long)

// per MDI window flags - used for MDIINFO.flags

#define      F_LOCAL      0x00000001
#define      F_CLPBRD     0x00000002

// per MDI display mode - MDIINFO.DisplayMode

#define   DSP_LIST   10
#define   DSP_PREV   11
#define   DSP_PAGE   12

///////////////////////////////////////////////////
// Data struct used to pass share info to SedCallback
typedef struct
   {
   SECURITY_INFORMATION si;
   WCHAR awchCName[MAX_COMPUTERNAME_LENGTH + 3];
   WCHAR awchSName[MAX_NDDESHARENAME + 1];
   }
   SEDCALLBACKCONTEXT;

/////////////////////////////////////////////
// useful(ess) macros

#define PRIVATE_FORMAT(fmt)   ((fmt) >= 0xC000)
#define GETMDIINFO(x) ((PMDIINFO)(GetWindowLong((x),GWL_MDIINFO)))

// parameter codes for MyGetFormat()

#define   GETFORMAT_LIE      200
#define   GETFORMAT_DONTLIE   201

// default DDEML synchronous transaction timeouts
// note these should be generous
#define   SHORT_SYNC_TIMEOUT   (24L*1000L)
#define   LONG_SYNC_TIMEOUT   (60L*1000L)

// owner draw listbox and bitmap metrics constants
#define   LSTBTDX   16   // width of folder ( with or without hand )
#define   LSTBTDY   16   // height of folder ( with or without hand )

#define   SHR_PICT_X   0   // offsets of shared folder bitmap
#define   SHR_PICT_Y   0
#define   SAV_PICT_X   16   // offsets of non-shared folder bitmap
#define   SAV_PICT_Y   0

#define   PREVBRD      4   // border around preview bitmaps

#define      BTNBARBORDER   2
#define      DEF_WIDTH      400  // initial app size
#define      DEF_HEIGHT      300

#define      SZBUFSIZ      256

// combined styles for owner draw listbox variants
#define   LBS_LISTVIEW   (LBS_OWNERDRAWFIXED|LBS_DISABLENOSCROLL)
#define   LBS_PREVIEW    (LBS_MULTICOLUMN|LBS_OWNERDRAWFIXED)

// Maximum allowed length of a permission name
#define MAC_PERMNAMELEN 64

//////////// compile options //////////////
#define   MAX_ALLOWED_PAGES   127

///////////////////////////////////////////
// main menu items
#define   IDM_FIRST        20

#define   IDM_ABOUT        20
#define   IDM_EXIT         21
#define   IDM_COPY         22
#define   IDM_DELETE       23
#define   IDM_SHARE        24
#define   IDM_LOCAL        25
#define   IDM_PROPERTIES   26
#define   IDM_OPEN         27
#define   IDM_SAVEAS       28
#define   IDM_NOP          29
#define   IDM_CONNECT      30
#define   IDM_DISCONNECT   31
#define   IDM_CONTENTS     32
#define   IDM_SEARCHHELP   33
#define   IDM_HELPHELP     34
#define   IDM_KEEP         35
#define   IDM_UNSHARE      36
#define   IDM_TOOLBAR      37
#define   IDM_STATUSBAR    38
#define   IDM_TILEVERT     39
#define   IDM_CASCADE      40
#define   IDM_ARRANGEICONS 41
#define   IDM_CLPWND       43
#define   IDM_REFRESH      44
#define   IDM_LISTVIEW     45
#define   IDM_PREVIEWS     46
#define   IDM_PAGEVIEW     47
#define   IDM_TILEHORZ     48
#define   IDM_PERMISSIONS  49
#define   IDM_AUDITING     50
#define   IDM_OWNER        51

#define IDM_LAST           51

#define WINDOW_MENU_INDEX   4
#define DISPLAY_MENU_INDEX  3   // submenu to put format entries i.e. "&Text"
#define SECURITY_MENU_INDEX 2

// strings
#define   IDS_HELV             21
#define   IDS_APPNAME          22
#define   IDS_SHROBJNAME       23
#define   IDS_INTERNALERR      26
#define   IDS_LOCALCLIP        27
#define   IDS_CLIPBOARD        28
#define   IDS_DATAUNAVAIL      29
#define   IDS_READINGITEM      30
#define   IDS_VIEWHELPFMT      31
#define   IDS_ACTIVATEFMT      32
#define   IDS_RENDERING        33
#define   IDS_DEFFORMAT        34
#define   IDS_GETTINGDATA      35
#define   IDS_NAMEEXISTS       36
#define   IDS_NOCONNECTION     38
#define   IDS_ESTABLISHING     39
#define   IDS_CLIPBOOKONFMT    40
#define   IDS_PAGEFMT          41
#define   IDS_PAGEFMTPL        42
#define   IDS_PAGEOFPAGEFMT    43
#define   IDS_DELETE           44
#define   IDS_DELETECONFIRMFMT 45
#define   IDS_FILEFILTER       46
#define   IDS_PASTEDLGTITLE    47
#define   IDS_SHAREDLGTITLE    48
#define   IDS_PAGENAMESYNTAX   49
#define   IDS_PASSWORDSYNTAX   50
#define   IDS_SHARINGERROR     51
#define   IDS_MAXPAGESERROR    52
#define   IDS_PRIVILEGEERROR   53
#define   IDS_CB_PAGE          54
#define   IDS_NOCLPBOOK        55
#define   IDS_GETPERMS         56
#define   IDS_TRUSTSHRKEY      256
#define   IDS_CLPBKKEY         257
// First permission name -- starts an array of permnames.
#define   IDS_PERMNAMEFIRST    0x0400
#define   IDS_AUDITNAMEFIRST   0x0410

// Filter string for Open/Save dialogs -- actually a custom resource.
#define   IDS_FILTERTEXT      55

// control ID's
#define   ID_LISTBOX      200
#define   ID_VSCROLL      201
#define   ID_HSCROLL      202
#define   ID_SIZEBOX      203
#define   ID_PAGEUP      204
#define   ID_PAGEDOWN      205

// function externs

///////// CLIPBOOK.C //////////////////////

extern int WinMain( HINSTANCE, HINSTANCE, LPSTR, int);
UINT (WINAPI *WNetServerBrowseDialog)(HWND, LPTSTR, LPTSTR, WORD, DWORD);
BOOL SyncOpenClipboard(HWND);
BOOL SyncCloseClipboard(void);

///////// CVUTIL.C ////////////////////////

extern HCONV InitSysConv ( HWND, HSZ, HSZ, BOOL );
extern BOOL AssertConnection ( HWND hwnd );
extern VOID HandleOwnerDraw ( HWND hwnd, WPARAM wParam, LPARAM lParam );
extern HWND FAR PASCAL NewWindow( VOID );
extern BOOL UpdateListBox ( HWND, HCONV );
extern VOID AdjustMDIClientSize(VOID);
extern HDDEDATA GetConvDataItem ( HWND, LPTSTR, LPTSTR, UINT );
extern LPARAM FAR PASCAL MyMsgFilterProc( int nCode, WPARAM wParam, LPARAM lParam);
extern VOID ResetScrollInfo ( HWND );
extern BOOL IsShared ( LPLISTENTRY lpLE );
extern BOOL SetShared ( LPLISTENTRY lpLE, BOOL fShared );
extern LRESULT CALLBACK FrameWndProc(HWND, UINT, WPARAM, LPARAM);
extern LRESULT CALLBACK ChildWndProc(HWND, UINT, WPARAM, LPARAM);

// From DDE.C
extern HDDEDATA EXPENTRY DdeCallback( WORD wType, WORD wFmt, HCONV hConv,
   HSZ hsz1, HSZ hsz2, HDDEDATA hData, DWORD lData1, DWORD lData2);
extern DWORD GetClipsrvVersion(HWND hwndMDIChild);

extern BOOL FAR PASCAL ConnectDlgProc ( HWND, UINT, WPARAM, LPARAM);
extern BOOL FAR PASCAL KeepAsDlgProc ( HWND, UINT, WPARAM, LPARAM);
extern BOOL FAR PASCAL ShareDlgProc ( HWND, UINT, WPARAM, LPARAM);
extern VOID PASCAL InitializeMenu (HANDLE);
extern BOOL InitListBox ( HWND hwnd, HDDEDATA hData );
extern VOID GetPreviewBitmap ( HWND hwnd, LPTSTR szName, UINT index );
extern VOID SetBitmapToListboxEntry ( HDDEDATA hbmp, HWND hwndList, UINT index);
extern HWND CreateNewListBox ( HWND, DWORD );
extern BOOL ForceRenderAll ( HWND hwnd, PVCLPBRD pVclp );
extern BOOL UpdateNofMStatus(HWND hwnd);
extern BOOL CreateNewRemoteWindow ( LPTSTR szMachineName, BOOL fReconnect );
extern BOOL RestoreAllSavedConnections ( VOID );
extern VOID AdjustControlSizes ( HWND );
extern VOID   ShowHideControls ( HWND );
extern VOID LoadIntlStrings ( VOID );
extern VOID SaveWindowPlacement ( PWINDOWPLACEMENT );
extern BOOL ReadWindowPlacement ( LPTSTR, PWINDOWPLACEMENT );
extern BOOL CreateTools ( HWND );
extern VOID DeleteTools ( HWND );
#if DEBUG
extern VOID DebOut ( LPTSTR format, ... );
#endif
extern UINT MyGetFormat ( LPTSTR szFmt, int mode );
extern BOOL SetClipboardFormatFromDDE ( HWND hwnd, UINT Fmt, HDDEDATA hDDE );
extern HDDEDATA MySyncXact ( LPBYTE lpbData, DWORD cbDataLen, HCONV hConv,
   HSZ hszItem, UINT wFmt, UINT wType, DWORD dwTimeout, LPDWORD lpdwResult );
BOOL LockApp ( BOOL fLock, LPTSTR lpszComment );
extern int MessageBoxID( HANDLE, HWND, UINT, UINT, UINT);
extern PDATAREQ CreateNewDataReq ( VOID );
extern BOOL DeleteDataReq ( PDATAREQ lpD );
extern BOOL ProcessDataReq ( HDDEDATA hData, PDATAREQ lpD );

// from CVCOMMAN.C
extern LRESULT ClipBookCommand ( HWND, UINT, WPARAM, LPARAM );

// externals from shellapi.h
// #include <shellapi.h>

// from stlib
extern int atoi( TCHAR * );

// From STRTOK.C
LPSTR  strtokA(LPSTR, LPCSTR);
LPWSTR strtokW(LPWSTR, LPCWSTR);

// from CVINIT.C
extern   HWND   hwndToolbar;
extern   HWND   hwndStatus;
extern   DWORD  nIDs[];

// from SHARES.C
extern BOOL WINAPI PermissionsEdit(HWND, LPTSTR, BOOL);
extern DWORD SedCallback(HWND, HANDLE, ULONG, PSECURITY_DESCRIPTOR,
      PSECURITY_DESCRIPTOR, BOOLEAN, BOOLEAN, LPDWORD);
extern LRESULT EditAuditing(void);
extern LRESULT EditOwner(void);
extern BOOL GetTokenHandle(HANDLE *);

// From DEBUG.C
extern LRESULT OnIDMKeep(HWND, UINT, WPARAM, LPARAM);

// From AUDITCHK.C
extern BOOL AuditPrivilege(int);
// Flags to call AuditPrivilege with
#define AUDIT_PRIVILEGE_CHECK 0
#define AUDIT_PRIVILEGE_ON    1
#define AUDIT_PRIVILEGE_OFF   2

////////////////// external data ///////////////////////

// clipBook.c

extern WINDOWPLACEMENT   Wpl;

extern HINSTANCE      hInst;

extern HFONT  hOldFont, hFontPreview;
extern HFONT  hFontUni;

extern BOOL   fStatus;
extern BOOL   fToolBar;
extern BOOL   fAppLockedState;
extern BOOL   fAppShuttingDown;
extern BOOL   fClipboardNeedsPainting;
extern BOOL   fSharePreference;
extern BOOL   fNeedToTileWindows;
extern BOOL   fShareEnabled;
extern BOOL   fNetDDEActive;
extern BOOL   fFillingClpFromDde;

extern HBITMAP   hbmStatus;

#ifdef   DEBUG
extern int DebugLevel;
#endif

extern HWND   hwndNextViewer;   // for clpbrd viewer chain
extern HWND   hwndDummy;   // used as dummy SetCapture target

// child window controls

extern HWND      hwndMDIClient;
extern HWND      hwndActiveChild;
extern HWND      hwndClpbrd;
extern HWND      hwndClpOwner;
extern HWND      hwndLocal;
extern HWND      hwndApp;

extern PMDIINFO   pActiveMDI;

extern HDC         hBtnDC;

extern HBITMAP      hBmpBtn;
extern HBITMAP      hOldBitmap;
extern HBITMAP      hPreviewBmp;
extern HBITMAP      hPgUpBmp;
extern HBITMAP      hPgDnBmp;
extern HBITMAP      hPgUpDBmp;
extern HBITMAP      hPgDnDBmp;
extern HACCEL      hAccel;

extern HDC         hMemDC;

extern int         dyStatus;
extern int         dyButtonBar;
extern int         dyBorder;
extern int         dyPrevFont;
extern UINT       cf_preview;

extern UINT       cf_linkcopy;
extern UINT       cf_objectlinkcopy;
extern UINT       cf_link;

extern TCHAR      szClipBookClass[];
extern TCHAR      szClipBookMenu[];
extern TCHAR      szChild[];

// localized strings
extern TCHAR     szHelv[];   // status line font
extern TCHAR     szAppName[];
extern TCHAR     szNDDEcode[];
extern TCHAR     szNDDEcode1[];
extern TCHAR     szClpBookShare[];
extern TCHAR     szInternalError[];
extern TCHAR     szLocalClpBk[];
extern TCHAR     szSysClpBrd[];
extern TCHAR     szDataUnavail[];
extern TCHAR     szReadingItem[];
extern TCHAR     szViewHelpFmt[];
extern TCHAR     szActivateFmt[];
extern TCHAR     szRendering[];
extern TCHAR     szDefaultFormat[];
extern TCHAR     szGettingData[];
extern TCHAR     szPageFmt[];
extern TCHAR     szPageFmtPl[];
extern TCHAR     szPageOfPageFmt[];
extern TCHAR     szEstablishingConn[];
extern TCHAR     szNoConnection[];
extern TCHAR     szClipBookOnFmt[];
extern TCHAR     szDelete[];
extern TCHAR     szDeleteConfirmFmt[];
extern TCHAR     szFileFilter[];
extern TCHAR    *szFilter;

// Registration strings
extern HKEY      hkeyRoot;
extern TCHAR      szIni[];
extern TCHAR      szPref[];
extern TCHAR      szConn[];
extern TCHAR      szWindows[];
extern TCHAR      szStatusbar[];
extern TCHAR      szToolbar[];
extern TCHAR      szDefView[];
extern TCHAR      szDebug[];

extern TCHAR      szHelpFile[];
extern TCHAR      szNull[];

// buffers
extern TCHAR      szBuf[];
extern TCHAR      szBuf2[];
extern TCHAR      szConvPartner[];
extern TCHAR      szKeepAs[];


// DDEML stuff

extern DWORD      idInst;         // DDEML handle

// HDDEDATA   hDdeData;

extern HSZ      hszFormatList;
extern HSZ      hszSystem;
extern HSZ      hszTopics;
extern HSZ      hszAllTopics;
extern HSZ      hszClpBookShare;

// from clipdsp.c
extern HBRUSH hbrBackground;
