////////////////////////////////////////////////////////////////
//
// this file is for global macros and global variables
// macros in the first section, variables (and macros associated with those variabls in the second
// (look for BEGIN GLOBALS
//
////////////////////////////////////////////////////////////////





// Map KERNEL32 unicode string functions to SHLWAPI
#define lstrcmpW    StrCmpW
#define lstrcmpiW   StrCmpIW
#define lstrcpyW    StrCpyW
#define lstrcpynW   StrCpyNW
#define lstrcatW    StrCatW


#define c_szNULL        TEXT("")
#define c_szHelpFile     TEXT("iexplore.hlp")
#define MAX_TOOLTIP_STRING 80

// Status bar defines
#define STATUS_PANES            5
#define STATUS_PANE_NAVIGATION  0
#define STATUS_PANE_PROGRESS    1
#define STATUS_PANE_OFFLINE     2
//#define STATUS_PANE_PRINTER     3
#define STATUS_PANE_SSL         3
#define STATUS_PANE_ZONE        4

#define ZONES_PANE_WIDTH        220

// logical defines for grfKeyState bits
#define FORCE_COPY (MK_CONTROL | MK_LBUTTON)    // means copy
#define FORCE_LINK (MK_LBUTTON | MK_CONTROL | MK_SHIFT)     // means link

// the only place ITB_MAX is really used is to make sure we don't have
// one of the distinguished values (e.g. ITB_VIEW, for both correctness
// and perf).  technically that means we can have ITB_MAX = (INT_MAX - 1),
// but 32000 ought to be plenty big enough and it's probably a bit safer
// in terms of collisions w/ ITB_VIEW.
#define ITB_MAX         32000           // max #
#define ITB_CSTATIC     2               // statically allocated guys
#define ITB_CGROW       2               // dynamic guys chunk size
// CASSERT(ITB_CSTATIC % ITB_CGROW == 0);

#define ISVISIBLE(hwnd)  ((GetWindowStyle(hwnd) & WS_VISIBLE) == WS_VISIBLE)

// this is for the file menus recently visited list.  
//  it represents the count of entries both back and forward 
//  that should be on the menu.
#define CRECENTMENU_MAXEACH     5

// shorthand
#ifndef ATOMICRELEASE
#ifdef __cplusplus
#define ATOMICRELEASET(p, type) { if(p) { type* punkT=p; p=NULL; punkT->Release();} }
#else
#define ATOMICRELEASET(p, type) { if(p) { type* punkT=p; p=NULL; punkT->lpVtbl->Release(punkT);} }
#endif

// doing this as a function instead of inline seems to be a size win.
//
#ifdef NOATOMICRELESEFUNC
#define ATOMICRELEASE(p) ATOMICRELEASET(p, IUnknown)
#else
#define ATOMICRELEASE(p) IUnknown_AtomicRelease((LPVOID*)&p)
#endif
#endif //ATOMICRELEASE

#ifdef SAFERELEASE
#undef SAFERELEASE
#endif
#define SAFERELEASE(p) ATOMICRELEASE(p)


#ifdef  UNICODE

   typedef WCHAR TUCHAR, *PTUCHAR;

#else   /* UNICODE */

   typedef unsigned char TUCHAR, *PTUCHAR;

#endif /* UNICODE */

#define LoadMenuPopup(id) SHLoadMenuPopup(MLGetHinst(), id)   
#define PropagateMessage SHPropagateMessage
#define MenuIndexFromID  SHMenuIndexFromID
#define Menu_RemoveAllSubMenus SHRemoveAllSubMenus
#define _EnableMenuItem SHEnableMenuItem
#define _CheckMenuItem SHCheckMenuItem

#define REGSTR_PATH_EXPLORERA    "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"
#define REGSTR_KEY_STREAMMRUA    REGSTR_PATH_EXPLORERA "\\StreamMRU"

#ifdef UNICODE
#define REGSTR_KEY_STREAMMRU        TEXT(REGSTR_PATH_EXPLORERA) TEXT("\\StreamMRU")
#else // UNICODE
#define REGSTR_KEY_STREAMMRU        REGSTR_KEY_STREAMMRUA
#endif // UNICODE
   
///////////////////////////////////////////////////////////////////////////////
///// BEGIN GLOBALS 
   
#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */
   
extern HINSTANCE g_hinst;
#define HINST_THISDLL g_hinst

extern BOOL g_fRunningOnNT;
extern BOOL g_bRunOnNT5;
extern BOOL g_bRunOnMemphis;
extern BOOL g_fRunOnFE;
extern HMENU g_hmenuFullSB;

//
// Is Mirroring APIs enabled (BiDi Memphis and NT5 only)
//
extern BOOL g_bMirroredOS;


extern HINSTANCE g_hinst;
#define HINST_THISDLL g_hinst

#define SID_SDropBlocker CLSID_SearchBand

#define FillExecInfo(_info, _hwnd, _verb, _file, _params, _dir, _show) \
        (_info).hwnd            = _hwnd;        \
        (_info).lpVerb          = _verb;        \
        (_info).lpFile          = _file;        \
        (_info).lpParameters    = _params;      \
        (_info).lpDirectory     = _dir;         \
        (_info).nShow           = _show;        \
        (_info).fMask           = 0;            \
        (_info).cbSize          = sizeof(SHELLEXECUTEINFO);

extern LCID g_lcidLocale;

//
// Globals (per-process)
//
extern LONG g_cThreads;
extern LONG g_cModelessDlg;
extern UINT g_tidParking;           // parking thread
extern HWND g_hDlgActive;
extern UINT g_msgMSWheel;
extern BOOL g_fShowCompColor;
extern COLORREF g_crAltColor;
extern HPALETTE g_hpalHalftone;


extern const GUID CGID_PrivCITCommands;


#ifdef __cplusplus
};                                   /* End of extern "C" {. */
#endif   /* __cplusplus */
