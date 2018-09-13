#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif

#ifdef SIZEOF
#undef SIZEOF
#endif

#define CCH_MAX_STRING    256
#define CCH_NONE          20        /* ARRAYSIZE( "(None)" ), big enough for German */
#define CCH_CLOSE         20        /* ARRAYSIZE( "Close" ), big enough for German */

#define CMSEC_COVER_WINDOW_TIMEOUT  (15 * 1000)     // 15 second timeout
#define ID_CVRWND_TIMER             0x96F251CC      // somewhat uniq id


// Maximum number of pages we will put in the PropertySheets
#define MAX_PAGES 24

#define DEBUG_SWITCH 1

//#ifndef WINNT
#define DbgPrint(pstr, x) DebugMsg(DEBUG_SWITCH, TEXT(pstr), x)
//#endif

// information about the monitor bitmap
// x, y, dx, dy define the size of the "screen" part of the bitmap
// the RGB is the color of the screen's desktop
// these numbers are VERY hard-coded to a monitor bitmap
#define MON_X   16
#define MON_Y   17
#define MON_DX  152
#define MON_DY  112
#define MON_W   184
#define MON_H   170
#define MON_RGB RGB(0, 128, 128)
#define MON_TRAY 8

#define CDPI_NORMAL     96      // Arbitrarily, 96dpi is "Normal"


#define         MIN_MINUTES     1
#define         MAX_MINUTES     9999	// The UI allows 4 digits maximum.
#define         BUFFER_SIZE     400

#define         MAX_METHODS     100


BOOL DeskInitCpl(void);
void DeskShowPropSheet( HINSTANCE hInst, HWND hwndParent, LPCTSTR szCmdLine );
BOOL CALLBACK _AddDisplayPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam);

BOOL APIENTRY BackgroundDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY ScreenSaverDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY AppearanceDlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GeneralPageProc    (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MultiMonitorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL GetAdvAdapterPropPage(LPVOID lpv, LPFNADDPROPSHEETPAGE lpfnAdd, LPARAM lparam);
BOOL GetAdvAdapterPropPageParam(LPVOID lpv, LPFNADDPROPSHEETPAGE lpfnAdd, LPARAM lparam, LPARAM lparamPage);

BOOL GetAdvMonitorPropPage(LPVOID lpv, LPFNADDPROPSHEETPAGE lpfnAdd, LPARAM lparam);
BOOL GetAdvMonitorPropPageParam(LPVOID lpv, LPFNADDPROPSHEETPAGE lpfnAdd, LPARAM lparam, LPARAM lparamPage);

BOOL APIENTRY DeskDefPropPageProc( HWND hDlg, UINT message, UINT wParam, LONG lParam);
LONG WINAPI MyStrToLong(LPCTSTR sz);

// fake.c
void AddFakeSettingsPage(PROPSHEETHEADER * ppsh);

// fixreg.c
void FixupRegistryHandlers(void);
BOOL GetDisplayKey(int i, LPTSTR szKey, DWORD cb);
void NukeDisplaySettings(void);

// background previewer includes

#define DIBERR_SUCCESS  1       // successful open
#define DIBERR_NOOPEN   -1      // file could not be opened
#define DIBERR_INVALID  -2      // file is not a valid bitmap

#define BP_NEWPAT       0x01    // pattern changed
#define BP_NEWWALL      0x02    // wallpaper changed
#define BP_TILE         0x04    // tile the wallpaper (center otherwise)
#define BP_REINIT       0x08    // reload the image (system colors changed)

#define WM_SETBACKINFO (WM_USER + 1)

#define BACKPREV_CLASS TEXT("BackgroundPreview")
#define LOOKPREV_CLASS TEXT("LookPreview")

BOOL FAR PASCAL RegisterBackPreviewClass(HINSTANCE hInst);
BOOL FAR PASCAL RegisterLookPreviewClass(HINSTANCE hInst);

HBITMAP FAR LoadMonitorBitmap( BOOL bFillDesktop );

#ifdef UNICODE
    UINT WinExecN( LPCTSTR lpCmdLine, UINT uCmdShow );
#else
    // If we're on Win95, then just use the ANSI-only WinExec instead of
    // rolling our own
#   define WinExecN    WinExec
#endif

#define SETTINGSPAGE_DEFAULT    -1
#define SETTINGSPAGE_FALLBACK   0

//#define Assert(p)   /* nothing */

#define ARRAYSIZE( a )  (sizeof(a) / sizeof(a[0]))
#define SIZEOF( a )     sizeof(a)


//
// CreateCoverWindow
//
// creates a window which obscures the display
//  flags:
//      0 means erase to black
//      COVER_NOPAINT means "freeze" the display
//
// just post it a WM_CLOSE when you're done with it
//
#define COVER_NOPAINT (0x1)
//
HWND FAR PASCAL CreateCoverWindow( DWORD flags );
void DestroyCoverWindow(HWND hwndCover);
//

//
// Macro to replace MAKEPOINT() since points now have 32 bit x & y
//
#define LPARAM2POINT( lp, ppt ) \
    ((ppt)->x = (int)(short)LOWORD(lp), (ppt)->y = (int)(short)HIWORD(lp))

//
// Globals
//
extern HINSTANCE hInstance;
extern TCHAR gszDeskCaption[CCH_MAX_STRING];

extern TCHAR g_szNULL[];
extern TCHAR g_szNone[CCH_NONE];
extern TCHAR g_szClose[CCH_CLOSE];
extern TCHAR g_szControlIni[];
extern TCHAR g_szPatterns[];

extern TCHAR g_szCurPattern[];   // name of currently selected pattern
extern TCHAR g_szCurWallpaper[]; // name of currently selected wallpaper
extern BOOL g_bValidBitmap;     // whether or not wallpaper is valid

extern TCHAR g_szBoot[];
extern TCHAR g_szSystemIni[];
extern TCHAR g_szWindows[];

extern HDC g_hdcMem;
extern HBITMAP g_hbmDefault;

#define WarnMsg(psz1, psz2) TraceMsg(TF_WARNING, "**** DESK.CPL: %s %s", psz1, psz2);
