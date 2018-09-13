/*----------------------------------------------------------------------------*\
 *
 *  MCIWnd
 *
 *    MCIWnd window class *internal* header file.
 *
 *----------------------------------------------------------------------------*/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <digitalv.h>
#include <commdlg.h>
#include <shellapi.h>
#include "preview.h"

#define NOUPDOWN
#define NOSTATUSBAR
#define NOMENUHELP
#define NOBTNLIST
#define NODRAGLIST
#define NOPROGRESS

#include "strings.rc"
#include "commctrl.h"
#include "mciwnd.h"

/**************************************************************************
***************************************************************************/

#define GetWS(hwnd)     GetWindowLong(hwnd, GWL_STYLE)
#define PutWS(hwnd, f)  SetWindowLong(hwnd, GWL_STYLE, f)
#define TestWS(hwnd,f)  (GetWS(hwnd) & f)
#define SetWS(hwnd, f)  ((PutWS(hwnd, GetWS(hwnd) | f) & (f)) != (f))
#define ClrWS(hwnd, f)  ((PutWS(hwnd, GetWS(hwnd) & ~(f)) & (f)) != 0)

/******************************************************************************
 *****************************************************************************/

#ifdef DEBUG
    #define MODNAME "MCIWnd"
    static void cdecl dprintf(PSTR sz, ...);
    #define DPF     dprintf
#else
    #define DPF     ; / ## /
#endif

#define ABS(x)  ((int)(x) > 0) ? (x) : (-(x))

// the place we load internationalizable strings into
static char szString[128];
#define LoadSz(ID) (LoadString(hInst, ID, szString, sizeof(szString)), szString)

/******************************************************************************
 *****************************************************************************/

// !!! Ack!  A global in a library!  But the stupid Common Control code needs
// !!! to know the instance that registered the class.  I think.
HINSTANCE hInst;

char aszMCIWndClassName[] = MCIWND_WINDOW_CLASS;

// A brush to draw our OwnerDraw menus with
extern BOOL FAR PASCAL CreateDitherBrush(BOOL bIgnoreCount);
extern BOOL FAR PASCAL FreeDitherBrush(void);
extern HBRUSH hbrDither;

/******************************************************************************
 *****************************************************************************/

// icky constants
#define TIMER1  	1
#define TIMER2  	2
#define ACTIVE_TIMER	500
#define INACTIVE_TIMER	2000

#define IDBMP_TOOLBAR   959
#define ID_TOOLBAR	747
#define TB_HEIGHT       21 //23         // toolbar windows are this high
#define STANDARD_WIDTH  300		// width of non-windowed toolbar
#define SMALLEST_WIDTH  60		// smallest width allowed

#define IDM_MCIZOOM	 11000
#define IDM_MCIVOLUME	 12000
#define VOLUME_MAX	 200
#define IDM_MCISPEED	 13000
#define SPEED_MAX    	 200

#define IDM_MCINEW	103
#define IDM_MCIOPEN	104
#define IDM_MCICLOSE	105
#define IDM_MCIREWIND   106
#define IDM_MENU        107	// menu button and menu id
#define IDM_MCIEJECT    108	// eject button id
#define TOOLBAR_END     109	// last item in toolbar
#define IDM_MCICONFIG   110     // bring up a configure box
#define IDM_MCICOMMAND	111
#define IDM_COPY	112

/******************************************************************************
 *****************************************************************************/

typedef struct {
    HWND    hwnd;
    HWND    hwndOwner;	// who gets notification
    UINT    alias;
    UINT    wDeviceID;
    UINT    wDeviceType;
    DWORD   dwError;
    DWORD   dwStyle;
    BOOL    fHasTracks;
    int	    iNumTracks;
    int	    iFirstTrack;
    LONG    *pTrackStart;
    BOOL    fRepeat;
    BOOL    fCanWindow;
    BOOL    fHasPalette;
    BOOL    fCanRecord;
    BOOL    fCanPlay;
    BOOL    fCanSave;
    BOOL    fCanEject;
    BOOL    fCanConfig;
    BOOL    fUsesFiles;
    BOOL    fVideo;
    BOOL    fAudio;
    BOOL    fMdiWindow;
    BOOL    fScrolling;
    BOOL    fTracking;
  //BOOL    fSeekExact;
    BOOL    fVolume;
    WORD    wMaxVol;
    BOOL    fSpeed;
    BOOL    fPlayAfterSeek;
    BOOL    fActive;            // Is this window active right now?
    BOOL    fMediaValid;        // have dwMediaStart and dwMediaLen been set?
    RECT    rcNormal;
    HMENU   hmenu;
    HMENU   hmenuVolume;
    HMENU   hmenuSpeed;
    HFONT   hfont;
    HWND    hwndToolbar;
    HWND    hwndTrackbar;
    DWORD   dwMediaStart;
    DWORD   dwMediaLen;
    WORD    wTimer;
    DWORD   dwMode;
    DWORD   dwPos;
    UINT    iZoom;
    UINT    iActiveTimerRate;
    UINT    iInactiveTimerRate;
    char    achFileName[128]; // to store open filename
    char    achReturn[128];   // to store result of the last SendString
    OPENFILENAME ofn;   // Remember current extension, etc. for opening files
    UINT    uiHack;	// For OwnerDraw hack
    HMENU   hmenuHack;	// For OwnerDraw hack
    HICON   hicon;
} MCIWND, NEAR * PMCIWND;
