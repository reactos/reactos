
/*----------------------------------------------------------------------------*\
 *
 *  MCIWnd
 *
 *----------------------------------------------------------------------------*/

#include "mciwndi.h"
#ifdef DAYTONA
// Include if debug string is read from registry
#include <profile.h>
#endif

#ifndef _WIN32
// variable args won't work in a DLL with DS!=SS unless _WINDLL is defined
#define WINDLL
#define _WINDLL
#define __WINDLL
#endif

#include <stdarg.h>
#include <stdlib.h>

#if defined CHICAGO && defined _WIN32
#define CHICAGO32
#endif

#define SQUAWKNUMZ(num) #num
#define SQUAWKNUM(num) SQUAWKNUMZ(num)
#define SQUAWK __FILE__ "(" SQUAWKNUM(__LINE__) ") :squawk: "

#ifdef _WIN32
#define MCIWndOpenA(hwnd, sz, f)     (LONG)MCIWndSM(hwnd, MCIWNDM_OPENA, (WPARAM)(UINT)(f),(LPARAM)(LPVOID)(sz))
#endif

#if !defined NUMELMS
    #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

#ifdef UNICODE
#include <wchar.h>

//
//  Assist with unicode conversions
//

int Iwcstombs(LPSTR lpstr, LPCWSTR lpwstr, int len)
{
    return WideCharToMultiByte(GetACP(), 0, lpwstr, -1, lpstr, len, NULL, NULL);
}

int Imbstowcs(LPWSTR lpwstr, LPCSTR lpstr, int len)
{
    return MultiByteToWideChar(GetACP(),
                               MB_PRECOMPOSED,
                               lpstr,
                               -1,
                               lpwstr,
                               len);
}

#endif

#ifdef CHICAGO
extern BOOL gfIsRTL;
#else
#define gfIsRTL 0
#endif

LRESULT CALLBACK _LOADDS MCIWndProc(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK _LOADDS SubClassedTrackbarWndProc(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam);

STATICFN HBRUSH NEAR PASCAL CreateDitherBrush(void);

STATICFN LRESULT OwnerDraw(PMCIWND p, UINT msg, WPARAM wParam, LPARAM lParam);
STATICFN BOOL NEAR PASCAL mciDialog(HWND hwnd);
STATICFN void NEAR PASCAL MCIWndCopy(PMCIWND p);

BOOL FAR _cdecl _loadds MCIWndRegisterClass(void)
{
    WNDCLASS cls;

    // !!! We need to register a global class with the hinstance of the DLL
    // !!! because it's the DLL that has the code for the window class.
    // !!! Otherwise, the class goes away on us and things start to blow!
    // !!! HACK HACK HACK The hInstance is the current DS which is the high
    // !!! word of the address of all global variables --- sorry NT
#ifndef _WIN32
    HINSTANCE hInstance = (HINSTANCE)HIWORD((LPVOID)&hInst); // random global
#else
    extern HINSTANCE ghInst;	// in video\init.c
    HINSTANCE hInstance = ghInst;
#endif

    // If we're already registered, we're OK
    if (GetClassInfo(hInstance, aszMCIWndClassName, &cls))
	return TRUE;

    // !!! Save the instance that created the class in a global for cutils.c
    // !!! which may need to know this.  I know, it's ugly.
    hInst = hInstance;

    cls.lpszClassName   = aszMCIWndClassName;
    cls.lpfnWndProc     = (WNDPROC)MCIWndProc;
    cls.style           = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;
    cls.hCursor         = LoadCursor(NULL,IDC_ARROW);
    cls.hIcon           = NULL;
    cls.lpszMenuName    = NULL;
    cls.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1);
    cls.hInstance	= hInstance;
    cls.cbClsExtra      = 0;
    cls.cbWndExtra      = sizeof(LPVOID); // big enough for far pointer

    if (RegisterClass(&cls)) {

        InitCommonControls();

        // !!! Other one-time initialization

	return TRUE;
    }

    return FALSE;
}


//
// Create the window
//
//
#ifdef UNICODE
// HACK ALERT:
// (see comments in MCIWndiCreate
// we set a flag to say that the szFile parameter is unicode.  Otherwise if
// the parent window is ansi (which is likely) we will try and interpret the
// string as ascii.  This is not safe if MCIWndCreate is called on multiple
// threads, but I do not want (currently... let me change my mind) to add
// a critical section to prevent multiple thread access.  Unfortunately
// there is no simple way to pass another parameter to MCIWndiCreate as
// the interface goes through the wndproc WM_CREATE message.

BOOL fFileNameIsUnicode = FALSE;
#endif

HWND FAR _cdecl _loadds MCIWndCreate(HWND hwndParent, HINSTANCE hInstance,
                      DWORD dwStyle, LPCTSTR szFile)
{
    HWND hwnd;
    int x,y,dx,dy;
    DWORD dwStyleEx;

#ifdef _WIN32
    #define GetCurrentInstance()    GetModuleHandle(NULL);
#else
    #define GetCurrentInstance()    SELECTOROF(((LPVOID)&hwndParent))
#endif

    if (hInstance == NULL)
        hInstance = GetCurrentInstance();

    if (!MCIWndRegisterClass())
	return NULL;

    if (HIWORD(dwStyle) == 0)
    {
	if (hwndParent)
	    dwStyle |= WS_CHILD | WS_BORDER | WS_VISIBLE;
	else
	    dwStyle |= WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    }

    // !!! Do we really want to do this?
    dwStyle |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    x = y = dy = 0;
    dx = STANDARD_WIDTH;

    // If we're making a top level window, pick some reasonable position
    if (hwndParent == NULL && !(dwStyle & WS_POPUP)) {
        x = CW_USEDEFAULT;
	// Visible overlapped windows treat y as a ShowWindow flag
	if (dwStyle & WS_VISIBLE)
	    y = SW_SHOW;
    }

    // Our preview open dialog rips if we don't provide a non-zero ID for a
    // child window.
    dwStyleEx = gfIsRTL ? WS_EX_LEFTSCROLLBAR | WS_EX_RIGHT | WS_EX_RTLREADING : 0;
#ifdef UNICODE
    fFileNameIsUnicode = TRUE;
    // strictly... we should enter a critical section here to lock out
    // all other MCIWndCreate calls.
#endif
    hwnd = CreateWindowEx(dwStyleEx,
                          aszMCIWndClassName, szNULL, dwStyle,
                          x, y, dx, dy,
                          hwndParent,
                          (HMENU)((dwStyle & WS_CHILD) ? 0x42 : 0),
                          hInstance, (LPVOID)szFile);
#ifdef UNICODE
    fFileNameIsUnicode = FALSE;
#endif
    return hwnd;
}

#ifdef UNICODE
// ansi thunk for above function - stub
HWND FAR _cdecl _loadds MCIWndCreateA(HWND hwndParent, HINSTANCE hInstance,
                    DWORD dwStyle, LPCSTR szFile)
{
    WCHAR * lpW;
    int sz;
    HWND    hwnd;

    if (szFile == NULL) {
        return MCIWndCreateW(hwndParent, hInstance, dwStyle, NULL);
    }

    sz = lstrlenA(szFile) + 1;

    lpW = (WCHAR *) LocalAlloc(LPTR, sizeof(WCHAR) * sz);
    if (!lpW) {
        return NULL;
    }

    Imbstowcs(lpW, szFile, sz);

    hwnd = MCIWndCreateW(hwndParent, hInstance, dwStyle, lpW);

    LocalFree ((HLOCAL)lpW);

    return hwnd;
}
#else
#if defined _WIN32
HWND FAR _loadds MCIWndCreateW(HWND hwndParent, HINSTANCE hInstance,
                    DWORD dwStyle, LPCWSTR szFile)
{
    #pragma message (SQUAWK "maybe later add support here")
    return NULL;
}
#endif
#endif // UNICODE

//
// Give a notification of something interesting to the proper authorites.
//
STATICFN LRESULT NotifyOwner(PMCIWND p, unsigned msg, WPARAM wParam, LPARAM lParam
)
{
    if (p->hwndOwner)
        return SendMessage(p->hwndOwner, msg, wParam, lParam);
    else
        return 0;
}


//
// If an error occured, set our error code and maybe bring up a dialog
// Clears the error code if command was successful.
//
STATICFN void MCIWndiHandleError(PMCIWND p, MCIERROR dw)
{
    TCHAR       ach[128];

    // Set/Clear our error code
    p->dwError = dw;

    if (dw) {

	// We want to bring up a dialog on errors, so do so.
	// Don't bring up a dialog while we're moving the thumb around because
	// that'll REALLY confuse the mouse capture
	if (!(p->dwStyle & MCIWNDF_NOERRORDLG) && !p->fScrolling &&
							!p->fTracking) {
            mciGetErrorString(p->dwError, ach, NUMELMS(ach));
	    MessageBox(p->hwnd, ach, LoadSz(IDS_MCIERROR),
		       MB_ICONEXCLAMATION | MB_OK);
	}

	// The "owner" wants to know the error.  We tell him after we
	// bring up the dialog, because otherwise, our VBX never gets this
	// event.  (Weird...)
	if (p->dwStyle & MCIWNDF_NOTIFYERROR) {
	    NotifyOwner(p, MCIWNDM_NOTIFYERROR, (WPARAM)p->hwnd, p->dwError);
	}

    }
}

//
// Send an MCI GetDevCaps command and return whether or not it's supported
// This will not set our error code
//
STATICFN BOOL MCIWndDevCaps(PMCIWND p, DWORD item)
{
    MCI_GETDEVCAPS_PARMS   mciDevCaps;
    DWORD               dw;

    if (p->wDeviceID == 0)
        return FALSE;

    mciDevCaps.dwItem = (DWORD)item;

    dw = mciSendCommand(p->wDeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM,
			(DWORD_PTR)(LPVOID)&mciDevCaps);

    if (dw == 0)
	return (BOOL)mciDevCaps.dwReturn;
    else
	return FALSE;
}

//
// Send an MCI Status command.
// This will not set our error code
//
STATICFN DWORD MCIWndStatus(PMCIWND p, DWORD item, DWORD err)
{
    MCI_STATUS_PARMS    mciStatus;
    DWORD               dw;

    if (p->wDeviceID == 0)
	return err;

    mciStatus.dwItem = (DWORD)item;

    dw = mciSendCommand(p->wDeviceID, MCI_STATUS, MCI_STATUS_ITEM,
			(DWORD_PTR)(LPVOID)&mciStatus);

    if (dw == 0)
	return (DWORD) mciStatus.dwReturn;
    else
	return err;
}

//
// Send an MCI String command
// Optionally set our error code.  Never clears it.
//
STATICFN DWORD MCIWndString(PMCIWND p, BOOL fSetErr, LPTSTR sz, ...)
{
    TCHAR   ach[MAX_PATH];
    int     i;
    DWORD   dw;
    va_list va;

    if (p->wDeviceID == 0)
	return 0;

    for (i=0; *sz && *sz != TEXT(' '); )
	ach[i++] = *sz++;

    i += wsprintf(&ach[i], TEXT(" %d "), (UINT)p->alias);

    va_start(va,sz);
    i += wvsprintf(&ach[i], sz, va);
    va_end(va);

    dw = mciSendString(ach, NULL, 0, NULL);

    DPF("MCIWndString('%ls'): %ld",(LPTSTR)ach, dw);

    if (fSetErr)
	MCIWndiHandleError(p, dw);

    return dw;
}


STATICFN long mciwnd_atol(LPTSTR sz)
{
    long l;

    //!!! check for (-) sign?
    for (l=0; *sz >= TEXT('0') && *sz <= TEXT('9'); sz++)
        l = l*10 + (*sz - TEXT('0'));

    return l;
}

#define SLASH(c)     ((c) == TEXT('/') || (c) == TEXT('\\'))

/*--------------------------------------------------------------+
| FileName  - return a pointer to the filename part of szPath   |
|             with no preceding path.                           |
+--------------------------------------------------------------*/
LPTSTR FAR FileName(LPTSTR szPath)
{
    LPCTSTR   sz;

    sz = &szPath[lstrlen(szPath)];
    for (; sz>szPath && !SLASH(*sz) && *sz!=TEXT(':');)
        sz = CharPrev(szPath, sz);
    return (sz>szPath ? (LPTSTR)++sz : (LPTSTR)sz);
}

//
// Sends an MCI String command and converts the return string to an integer
// Optionally sets our error code.  Never clears it.
//
STATICFN DWORD MCIWndGetValue(PMCIWND p, BOOL fSetErr, LPTSTR sz, DWORD err, ...)
{
    TCHAR   achRet[20];
    TCHAR   ach[MAX_PATH];
    DWORD   dw;
    int     i;
    va_list va;

    for (i=0; *sz && *sz != TEXT(' '); )
	ach[i++] = *sz++;

    if (p->wDeviceID)
        i += wsprintf(&ach[i], TEXT(" %d "), (UINT)p->alias);
    va_start(va, err);
    i += wvsprintf(&ach[i], sz, va);  //!!!use varargs
    va_end(va);

    dw = mciSendString(ach, achRet, NUMELMS(achRet), NULL);

    DPF("MCIWndGetValue('%ls'): %ld",(LPTSTR)ach, dw);

    if (fSetErr)
        MCIWndiHandleError(p, dw);

    if (dw == 0) {
        DPF("GetValue('%ls'): %ld",(LPTSTR)ach, mciwnd_atol(achRet));
        return mciwnd_atol(achRet);
    } else {
        DPF("MCIGetValue('%ls'): error=%ld",(LPTSTR)ach, dw);
	return err;
    }
}

//
// Send an MCI command and get the return string back
// This never sets our error code.
//
// Note: szRet can be the same string as sz
//
STATICFN DWORD MCIWndGet(PMCIWND p, LPTSTR sz, LPTSTR szRet, int len, ...)
{
    TCHAR   ach[MAX_PATH];
    int     i;
    DWORD   dw;
    va_list va;

    if (!p->wDeviceID) {
	szRet[0] = 0;
	return 0L;
    }

    for (i=0; *sz && *sz != TEXT(' '); )
	ach[i++] = *sz++;

    i += wsprintf(&ach[i], TEXT(" %d "), (UINT)p->alias);
    va_start(va, len);
    i += wvsprintf(&ach[i], sz, va);  //!!!use varargs
    va_end(va);

    // initialize to NULL return string
    szRet[0] = 0;

    dw = mciSendString(ach, szRet, len, p->hwnd);

    DPF("MCIWndGet('%ls'): '%ls'",(LPTSTR)ach, (LPTSTR)szRet);

    return dw;
}

//
// Gets the source or destination rect from the MCI device
// Does NOT set our error code since this is an internal function
//
STATICFN void MCIWndRect(PMCIWND p, LPRECT prc, BOOL fSource)
{
    MCI_DGV_RECT_PARMS      mciRect;
    DWORD dw=0;

    SetRectEmpty(prc);

    if (p->wDeviceID)
        dw = mciSendCommand(p->wDeviceID, MCI_WHERE,
            (DWORD)fSource ? MCI_DGV_WHERE_SOURCE : MCI_DGV_WHERE_DESTINATION,
            (DWORD_PTR)(LPVOID)&mciRect);

    if (dw == 0)
        *prc = mciRect.rc;

    prc->right  += prc->left;
    prc->bottom += prc->top;
}


STATICFN VOID MCIWndiSizePlaybar(PMCIWND p)
{
    RECT rc;
    UINT w, h;

    // No playbar!!
    if (p->dwStyle & MCIWNDF_NOPLAYBAR)
	return;

    #define SLOP 0     // Left outdent of toolbar

    // How big a window are we putting a toolbar on?
    GetClientRect(p->hwnd, &rc);
    w = rc.right;
    h = rc.bottom;

    SetWindowPos(p->hwndToolbar, NULL,
		-SLOP, h - TB_HEIGHT, w + SLOP, TB_HEIGHT,
		SWP_NOZORDER);

    // Make sure it's visible now
    ShowWindow(p->hwndToolbar, SW_SHOW);

    // Figure out where the toolbar ends and the trackbar begins
    SendMessage(p->hwndToolbar, TB_GETITEMRECT,
	(int)SendMessage(p->hwndToolbar, TB_COMMANDTOINDEX,
		TOOLBAR_END, 0),
	(LPARAM)(LPVOID)&rc);

    // Place the trackbar next to the end of the toolbar
    SetWindowPos(p->hwndTrackbar, HWND_TOP, rc.right,
		h - TB_HEIGHT + 2,
		w - rc.right, TB_HEIGHT,	// !!!
		0);

    //!!! Maybe put menu button on right side of trackbar?  So
    //!!! make sep the right size (size of the track bar!)
}

// Resize the window by the given percentage
// 0 means use DESTINATION rect and size it automatically
STATICFN VOID MCIWndiSize(PMCIWND p, int iSize)
{
    RECT rc, rcT;
    int  dx, dy;

    // If we're given a percentage, we take it from the SOURCE size.
    // For default, (zero), we use the destination size
    if (iSize)
        rc = p->rcNormal; /* get the original "normal size" rect */
    else {
	if (p->wDeviceID)
            MCIWndRect(p, &rc, FALSE);/* get the current (destination) size */
	else
	    SetRect(&rc, 0, 0, 0, 0);
	iSize = 100;
    }

    rc.bottom = MulDiv(rc.bottom, iSize, 100);
    rc.right = MulDiv(rc.right, iSize, 100);

    // Now set the movie to play in the new rect
    if (!IsRectEmpty(&rc))
        MCIWndString(p, FALSE, szPutDest,
	    0, 0, rc.right - rc.left, rc.bottom - rc.top);
	
    // If we're not supposed to resize our window to this new rect, at least
    // we'll fix up the toolbar before we leave (the buttons may have changed)
    if (p->dwStyle & MCIWNDF_NOAUTOSIZEWINDOW) {
	MCIWndiSizePlaybar(p);
	return;
    }

    // We're not a windowed device, or we're closed - don't touch our width
    if (IsRectEmpty(&rc)) {
        GetClientRect(p->hwnd, &rcT);
        rc.right = rcT.right;
    }

    // If we will have a playbar, grow the window by its height
    if (!(p->dwStyle & MCIWNDF_NOPLAYBAR))
        rc.bottom += TB_HEIGHT;

    // Now get the size for our window by growing it by its non-client size
    AdjustWindowRect(&rc, GetWindowLong(p->hwnd, GWL_STYLE), FALSE);

    // Now we have the new size for our MCIWND.  If the SetWindowPos didn't
    // actually result in changing its size, it will not generate a WM_SIZE
    // and it won't fix the toolbar or set the dest rect correctly, so we'll
    // have to call a WM_SIZE ourselves.  It's not enough to check the size
    // we are trying to make it, because it may not give us the size we want
    // (WM_GETMINMAXINFO) so we have to compare the original size and the
    // ultimate size.
    // Sometimes if it only changes by one pixel, it STILL won't generate a
    // WM_SIZE.
    GetWindowRect(p->hwnd, &rcT);

    SetWindowPos(p->hwnd, NULL, 0, 0, rc.right - rc.left,
                    rc.bottom - rc.top,
                    SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

    GetWindowRect(p->hwnd, &rc);
    dx = ABS((rcT.right - rcT.left) - (rc.right - rc.left));
    dy = ABS((rcT.bottom - rcT.top) - (rc.bottom - rc.top));
    if (dx < 2 && dy < 2) {
	PostMessage(p->hwnd, WM_SIZE, 0, 0);
    }
}


//
// Figure out the position in ms of the beginning of the track we're on
//
STATICFN DWORD MCIWndiPrevTrack(PMCIWND p)
{
    DWORD	dw;
    int		iTrack;

    if (!p->fHasTracks)
	return 0;

    MCIWndString(p, FALSE, szSetFormatTMSF);
    dw = MCIWndStatus(p, MCI_STATUS_POSITION, 0); // return value is 0xFFSSMMTT
    iTrack = LOWORD(dw) & 0xFF;
    // If we're less than 1 second into the track, choose the previous track
    if ((iTrack > p->iFirstTrack) && (!(LOWORD(dw) & 0xFF00)) &&
			((HIWORD(dw) & 0xFF) == 0))
	iTrack--;
    dw = p->pTrackStart[iTrack - p->iFirstTrack];
    MCIWndString(p, FALSE, szSetFormatMS);
    return dw;
}

//
// Figure out the position in ms of the beginning of the next track
//
STATICFN DWORD MCIWndiNextTrack(PMCIWND p)
{
    DWORD	dw;
    int		iTrack;

    if (!p->fHasTracks)
	return 0;

    MCIWndString(p, FALSE, szSetFormatTMSF);
    dw = MCIWndStatus(p, MCI_STATUS_POSITION, 0); // return value is 0xTTMMSSFF
    iTrack = (LOWORD(dw) & 0xFF) + 1;
    if (iTrack >= p->iNumTracks + p->iFirstTrack)
	iTrack--;
    dw = p->pTrackStart[iTrack - p->iFirstTrack];
    MCIWndString(p, FALSE, szSetFormatMS);
    return dw;
}


//
// Figure out where the tracks begin for making tics
//
STATICFN void MCIWndiCalcTracks(PMCIWND p)
{
    int		i;

    if (!p->fHasTracks)
	return;

    p->iNumTracks = (int)MCIWndGetValue(p, FALSE, szStatusNumTracks, 0);
    p->iFirstTrack = MCIWndGetValue(p, FALSE, szStatusPosTrack, 0, 0) == 0
		? 1 : 0;

    if (p->pTrackStart)
	LocalFree((HANDLE)p->pTrackStart);

    if (p->iNumTracks) {
	p->pTrackStart = (LONG *)LocalAlloc(LPTR,
						p->iNumTracks * sizeof(LONG));
	if (p->pTrackStart == NULL) {
	    p->iNumTracks = 0;
	    p->fHasTracks = FALSE;
	}
	for (i = 0; i < p->iNumTracks; i++) {
	    p->pTrackStart[i] =
		MCIWndGetValue(p, FALSE, szStatusPosTrack, 0,
		    p->iFirstTrack + i);
	}
    }
}


//
// Mark tics on the trackbar for the beginning of tracks
//
STATICFN void MCIWndiMarkTics(PMCIWND p)
{
    int		i;

    if (!p->fHasTracks)
	return;

    SendMessage(p->hwndTrackbar, TBM_SETTIC, 0, p->dwMediaStart);
    for (i = 0; i < p->iNumTracks; i++) {
	SendMessage(p->hwndTrackbar, TBM_SETTIC, 0, p->pTrackStart[i]);
    }
    SendMessage(p->hwndTrackbar, TBM_SETTIC,0, p->dwMediaStart + p->dwMediaLen);
}

STATICFN VOID MCIWndiValidateMedia(PMCIWND p)
{
    DWORD dw;

    if (!p->wDeviceID) {
	p->fMediaValid = FALSE;
	return;
    }

    dw = p->dwMediaLen;
    p->fMediaValid = TRUE;
    p->dwMediaStart = MCIWndGetStart(p->hwnd);
    p->dwMediaLen = MCIWndGetLength(p->hwnd);
    // !!! do something special if len=0?

    // We have a playbar, so set the ranges of the trackbar if we've changed
    if (dw != p->dwMediaLen && !(p->dwStyle & MCIWNDF_NOPLAYBAR)) {
	// must set position first or zero length range won't move thumb
        SendMessage(p->hwndTrackbar, TBM_CLEARTICS, TRUE, 0);
        SendMessage(p->hwndTrackbar, TBM_SETPOS, TRUE, p->dwMediaStart);
	SendMessage(p->hwndTrackbar, TBM_SETRANGEMIN, 0, p->dwMediaStart);
	SendMessage(p->hwndTrackbar, TBM_SETRANGEMAX, 0,
		p->dwMediaStart + p->dwMediaLen);

        MCIWndiCalcTracks(p);
        MCIWndiMarkTics(p);
    }
}

//
// Create the filter for the open dialog.  Caution: Don't overflow pchD !!!
//
STATICFN void MCIWndiBuildMeAFilter(LPTSTR pchD)
{
    LPTSTR      pchS;
    TCHAR       ach[128];

    // Our filter will look like:  "MCI Files\0*.avi;*.wav\0All Files\0*.*\0"
    // The actual extensions for the MCI files will come from the list in
    // the "mci extensions" section of win.ini

    lstrcpy(pchD, LoadSz(IDS_MCIFILES));

    // Creates a list like: "avi\0wav\0mid\0"
    GetProfileString(szMCIExtensions, NULL, szNULL, ach, NUMELMS(ach));
	
    for (pchD += lstrlen(pchD)+1, pchS = ach; *pchS;
		pchD += lstrlen(pchS)+3, pchS += lstrlen(pchS)+1) {
        lstrcpy(pchD, TEXT("*."));
	lstrcpy(pchD + 2, pchS);
        lstrcpy(pchD + 2 + lstrlen(pchS), TEXT(";"));
    }
    if (pchS != ach)
	--pchD;		// erase the last ;
    *pchD = TEXT('\0');
    lstrcpy(++pchD, LoadSz(IDS_ALLFILES));
    pchD += lstrlen(pchD) + 1;
    lstrcpy(pchD, TEXT("*.*\0"));
}

//
// Create the playbar windows we'll need later
//
STATICFN void MCIWndiMakeMeAPlaybar(PMCIWND p)
{
    TBBUTTON            tb[8];
    extern HINSTANCE ghInst;	// in video\init.c
    DWORD               dwStyleEx;


    // They don't want a playbar
    if (p->dwStyle & MCIWNDF_NOPLAYBAR)
	return;


#define MENUSEP 2
    tb[0].iBitmap = MENUSEP;
    tb[0].idCommand = -1;
    tb[0].fsState = 0;
    tb[0].fsStyle = TBSTYLE_SEP;
    tb[0].iString = -1;

    tb[1].iBitmap = 0;
    tb[1].idCommand = MCI_PLAY;
    tb[1].fsState = TBSTATE_ENABLED | TBSTATE_HIDDEN;
    tb[1].fsStyle = TBSTYLE_BUTTON;
    tb[1].iString = -1;

    tb[2].iBitmap = 2;
    tb[2].idCommand = MCI_STOP;
    tb[2].fsState = TBSTATE_ENABLED | TBSTATE_HIDDEN;
    tb[2].fsStyle = TBSTYLE_BUTTON;
    tb[2].iString = -1;

    tb[3].iBitmap = 4;
    tb[3].idCommand = MCI_RECORD;
    tb[3].fsState = TBSTATE_ENABLED | TBSTATE_HIDDEN;
    tb[3].fsStyle = TBSTYLE_BUTTON;
    tb[3].iString = -1;

    tb[4].iBitmap = 5;
    tb[4].idCommand = IDM_MCIEJECT;
    tb[4].fsState = TBSTATE_ENABLED | TBSTATE_HIDDEN;
    tb[4].fsStyle = TBSTYLE_BUTTON;
    tb[4].iString = -1;

    tb[5].iBitmap = MENUSEP;
    tb[5].idCommand = -1;
    tb[5].fsState = 0;
    tb[5].fsStyle = TBSTYLE_SEP;
    tb[5].iString = -1;

    tb[6].iBitmap = 3;
    tb[6].idCommand = IDM_MENU;
    tb[6].fsState = TBSTATE_ENABLED;
    tb[6].fsStyle = TBSTYLE_BUTTON;
    tb[6].iString = -1;

    tb[7].iBitmap = 4;
    tb[7].idCommand = TOOLBAR_END;
    tb[7].fsState = 0;
    tb[7].fsStyle = TBSTYLE_SEP;
    tb[7].iString = -1;

//    if (p->hbmToolbar)
//	DeleteObject(p->hbmToolbar);
    // Must use DLL's ghInst to get Bitmap
//    p->hbmToolbar = LoadBitmap(ghInst, MAKEINTRESOURCE(IDBMP_TOOLBAR));

    // Create invisible for now so it doesn't flash
    p->hwndToolbar = CreateToolbarEx(p->hwnd, TBSTYLE_BUTTON | TBSTYLE_TOOLTIPS
        | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
            CCS_NOPARENTALIGN | CCS_NORESIZE,
        ID_TOOLBAR, 8,
//	NULL, (UINT)p->hbmToolbar,
	ghInst, IDBMP_TOOLBAR,
	(LPTBBUTTON)&tb[0], 8,
        13, 13, 13, 13, sizeof(TBBUTTON));	// buttons are 13x13

    dwStyleEx = gfIsRTL ? WS_EX_LEFTSCROLLBAR | WS_EX_RIGHT | WS_EX_RTLREADING : 0;
    p->hwndTrackbar = CreateWindowEx(dwStyleEx,
        TRACKBAR_CLASS, NULL,
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, 0, 0, p->hwnd, NULL, GetWindowInstance(p->hwnd), NULL);

    // The trackbar eats key presses so we need to sub class it to see when
    // CTRL-1, CTRL-2, etc. are pressed
    fnTrackbarWndProc = (WNDPROC)GetWindowLongPtr(p->hwndTrackbar, GWLP_WNDPROC);
    SetWindowLongPtr(p->hwndTrackbar, GWLP_WNDPROC,
		(LONG_PTR)SubClassedTrackbarWndProc);

    // Force ValidateMedia to actually update
    p->dwMediaStart = p->dwMediaLen = 0;

    // Set the proper range for the scrollbar
    MCIWndiValidateMedia(p);
}


//
// Gray/ungray toolbar buttons as necessary
//
STATICFN void MCIWndiPlaybarGraying(PMCIWND p)
{
    DWORD	dwMode;

    if (!(p->dwStyle & MCIWNDF_NOPLAYBAR)) {
	dwMode = MCIWndGetMode(p->hwnd, NULL, 0);

	if (dwMode == MCI_MODE_PLAY) {
	    // Hide PLAY Show STOP
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON,
		MCI_PLAY, TRUE);
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON,
		MCI_STOP, FALSE);
	    SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		MCI_STOP, TRUE);
	    if (p->fCanRecord)
		SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		    MCI_RECORD, FALSE);	// !!! can't record ???
	    if (p->fCanEject)
		SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		    IDM_MCIEJECT, TRUE);

	// Treat PAUSE mode like STOP mode
	} else if (dwMode == MCI_MODE_PAUSE ||
		   dwMode == MCI_MODE_STOP) {
	    // Hide STOP Show PLAY
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON,
		MCI_STOP, TRUE);
	    if (p->fCanPlay) {
		SendMessage(p->hwndToolbar, TB_HIDEBUTTON,
		    MCI_PLAY, FALSE);
		SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		    MCI_PLAY, TRUE);
	    }
	    if (p->fCanRecord)
		SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		    MCI_RECORD, TRUE);
	    if (p->fCanEject)
		SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		    IDM_MCIEJECT, TRUE);

	} else if (dwMode == MCI_MODE_RECORD) {
	    // Hide PLAY Show STOP
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON,
		MCI_PLAY, TRUE);
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON,
		MCI_STOP, FALSE);
	    SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		MCI_STOP, TRUE);
	    SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		MCI_RECORD, FALSE);
	    if (p->fCanEject)
		SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		    IDM_MCIEJECT, TRUE);	// !!! safe ???

	    // recording can change the length
	    p->fMediaValid = FALSE;

	} else if (dwMode == MCI_MODE_SEEK) {
	    // Hide PLAY Show STOP
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON,
		MCI_PLAY, TRUE);
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON,
		MCI_STOP, FALSE);
	    SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		MCI_STOP, TRUE);
	    if (p->fCanRecord)
		SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		    MCI_RECORD, FALSE);
	    if (p->fCanEject)
		SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		    IDM_MCIEJECT, FALSE);
	} else {
	    // OPEN, NOT_READY, etc. etc.
	    // Disable everything
	    if (p->fCanPlay) {
		SendMessage(p->hwndToolbar, TB_HIDEBUTTON,
		    MCI_PLAY, FALSE);
		SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		    MCI_PLAY, FALSE);
	    }
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON,
		MCI_STOP, TRUE);
	    if (p->fCanRecord)
		SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		    MCI_RECORD, FALSE);
	    if (p->fCanEject)
		SendMessage(p->hwndToolbar, TB_ENABLEBUTTON,
		    IDM_MCIEJECT, FALSE);

	    // Clear all tics
	    SendMessage(p->hwndTrackbar, TBM_CLEARTICS,1,0);

	    // Clean out the trackbar
	    // Make a note to re-query start, length later
	    SendMessage(p->hwndTrackbar, TBM_SETPOS,
				TRUE, 0); // set b4 range
	    SendMessage(p->hwndTrackbar, TBM_SETRANGE,
				0, 0);
	    p->fMediaValid = FALSE;
	}
    }
}


//
// Set up the toolbar to have the right buttons
//
STATICFN void MCIWndiFixMyPlaybar(PMCIWND p)
{
    if (p->dwStyle & MCIWNDF_NOPLAYBAR)
	return;

    if (!p->wDeviceID) {
	//
        // gray the toolbar, go to some default buttons, and set zero len track
        //
        if (!(p->dwStyle & MCIWNDF_NOPLAYBAR)) {
            SendMessage(p->hwndToolbar, TB_HIDEBUTTON,   MCI_PLAY,    FALSE);
            SendMessage(p->hwndToolbar, TB_ENABLEBUTTON, MCI_PLAY,    FALSE);
            SendMessage(p->hwndToolbar, TB_HIDEBUTTON,   MCI_STOP,    TRUE );
            SendMessage(p->hwndToolbar, TB_HIDEBUTTON,   MCI_RECORD,  TRUE );
            SendMessage(p->hwndToolbar, TB_HIDEBUTTON,   IDM_MCIEJECT,TRUE );
            SendMessage(p->hwndToolbar, TB_HIDEBUTTON,   IDM_MENU,
		p->dwStyle & MCIWNDF_NOMENU);

            SendMessage(p->hwndTrackbar, TBM_SETPOS, TRUE, 0); // set b4 range
            SendMessage(p->hwndTrackbar, TBM_SETRANGE, 0, 0);
	}
    }

    if (p->wDeviceID) {
	//
	// Use the appropriate buttons
	//
        if (p->fCanPlay)
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON, MCI_PLAY, FALSE);
        else
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON, MCI_PLAY, TRUE);
        if (p->fCanRecord && (p->dwStyle & MCIWNDF_RECORD))
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON, MCI_RECORD, FALSE);
        else
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON, MCI_RECORD, TRUE);
        if (p->fCanEject)
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON, IDM_MCIEJECT, FALSE);
        else
	    SendMessage(p->hwndToolbar, TB_HIDEBUTTON, IDM_MCIEJECT, TRUE);

        SendMessage(p->hwndToolbar, TB_HIDEBUTTON, IDM_MENU,
		p->dwStyle & MCIWNDF_NOMENU);

	// COMMCTRL toolbar bug ... re-arranging buttons screws up the state
	// of the existing buttons, so we better re-gray things
	MCIWndiPlaybarGraying(p);
    }
}

//
// Make an appropriate menu
//
STATICFN void MCIWndiMakeMeAMenu(PMCIWND p)
{
    HMENU hmenu, hmenuWindow = NULL, hmenuVolume = NULL, hmenuSpeed = NULL;
    int	  i;
    WORD  j;

    //
    // Create the floating popup menu BY HAND since we have no resource file
    //

    // Destroy the old menu
    if (p->hmenu) {
        DestroyMenu(p->hmenu);
	if (p->hbrDither) {
	    DeleteObject(p->hbrDither);
	    p->hbrDither = NULL;
	}
    }
    p->hmenu = NULL;
    p->hmenuVolume = NULL;
    p->hmenuSpeed = NULL;


    // We don't want a menu!
    if (p->dwStyle & MCIWNDF_NOMENU)
	return;

    //
    // If we don't want an open command, and nothing's open, don't make
    // a menu.
    //
    if (!p->wDeviceID && (p->dwStyle & MCIWNDF_NOOPEN))
	return;

    //
    // Create the WINDOW sub-popup
    // !!! Do we want to have this menu if an AUTOSIZE flag is off?
    //
    if (p->wDeviceID && p->fCanWindow) {
	hmenuWindow = CreatePopupMenu();
	if (hmenuWindow) {
            AppendMenu(hmenuWindow, MF_ENABLED, IDM_MCIZOOM+50,
			LoadSz(IDS_HALFSIZE));
	    AppendMenu(hmenuWindow, MF_ENABLED, IDM_MCIZOOM+100,
			LoadSz(IDS_NORMALSIZE));
	    AppendMenu(hmenuWindow, MF_ENABLED, IDM_MCIZOOM+200,
			LoadSz(IDS_DOUBLESIZE));
	}
    }

    //
    // Create the VOLUME sub-popup
    //
    if (p->wDeviceID && p->fVolume) {
	hmenuVolume = CreatePopupMenu();
        if (hmenuVolume) {

	    // !!! Hack from Hell
	    // Put a bogus menu item at the top.  When WINDOWS tries to select
	    // it after we bring up the menu, we won't let it.  We want the
	    // thumb to stay on the current value.
            AppendMenu(hmenuVolume, MF_ENABLED | MF_OWNERDRAW,
			IDM_MCIVOLUME + VOLUME_MAX + 1, NULL);

	    // Create all the Real menu items.  Make the menu VOLUME_MAX items
	    // tall even though the number of unique entries may be less
            for (i=IDM_MCIVOLUME + p->wMaxVol; i>=IDM_MCIVOLUME; i-=5)
		for (j=0; j < VOLUME_MAX / p->wMaxVol; j++)
                    AppendMenu(hmenuVolume, MF_ENABLED | MF_OWNERDRAW, i, NULL);

	    // Now put a filler item at the bottom so every REAL item falls
 	    // inside the channel and there's a unique thumb position for each
	    // item.
            AppendMenu(hmenuVolume, MF_ENABLED | MF_OWNERDRAW,
			IDM_MCIVOLUME + VOLUME_MAX + 2, NULL);

	    // Now CHECK the current volume so the thumb can draw there
	    // round to nearest 5 so it matches a menu item identifier
            i = ((int)MCIWndGetValue(p, FALSE, szStatusVolume, 1000) / 50) * 5;
            CheckMenuItem(hmenuVolume, IDM_MCIVOLUME + i, MF_CHECKED);
        }
    }

    //
    // Create the SPEED sub-popup
    //
    if (p->wDeviceID && p->fSpeed) {
	hmenuSpeed = CreatePopupMenu();
	if (hmenuSpeed) {

	    // !!! Hack from Hell
	    // Put a bogus menu item at the top.  When WINDOWS tries to select
	    // it after we bring up the menu, we won't let it.  We want the
	    // thumb to stay on the current value.
            AppendMenu(hmenuSpeed, MF_ENABLED | MF_OWNERDRAW,
			IDM_MCISPEED + SPEED_MAX + 1, NULL);

	    // Create all the Real menu items
            for (i=IDM_MCISPEED + SPEED_MAX; i>=IDM_MCISPEED; i-=5)
                AppendMenu(hmenuSpeed, MF_ENABLED | MF_OWNERDRAW, i, NULL);

	    // Now put a filler item at the bottom so every REAL item falls
 	    // inside the channel and there's a unique thumb position for each
	    // item.
            AppendMenu(hmenuSpeed, MF_ENABLED | MF_OWNERDRAW,
			IDM_MCISPEED + SPEED_MAX + 2, NULL);

	    // Now CHECK the current speed so the thumb can draw there
	    // round to nearest 5 so it matches a menu item identifier
            i = ((int)MCIWndGetValue(p, FALSE, szStatusSpeed, 1000) / 50) * 5;
            CheckMenuItem(hmenuSpeed, IDM_MCISPEED + i, MF_CHECKED);
        }
    }

    hmenu = CreatePopupMenu();

    if (hmenu) {

	if (p->wDeviceID && p->dwStyle & MCIWNDF_NOPLAYBAR) {
	    if (p->fCanPlay) {
	    	AppendMenu(hmenu, MF_ENABLED, MCI_PLAY, LoadSz(IDS_PLAY));
	    	AppendMenu(hmenu, MF_ENABLED, MCI_STOP, LoadSz(IDS_STOP));
	    }
	    if (p->fCanRecord && (p->dwStyle & MCIWNDF_RECORD))
	        AppendMenu(hmenu, MF_ENABLED, MCI_RECORD, LoadSz(IDS_RECORD));
	    if (p->fCanEject)
	    	AppendMenu(hmenu, MF_ENABLED, IDM_MCIEJECT, LoadSz(IDS_EJECT));
	    if (p->fCanPlay ||
			(p->fCanRecord && (p->dwStyle & MCIWNDF_RECORD)) ||
			p->fCanEject)
                AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
	
	}

	if (hmenuWindow)
            AppendMenu(hmenu, MF_ENABLED|MF_POPUP, (UINT_PTR)hmenuWindow,
		LoadSz(IDS_VIEW));
	if (hmenuVolume)
	    AppendMenu(hmenu, MF_ENABLED|MF_POPUP, (UINT_PTR)hmenuVolume,
		LoadSz(IDS_VOLUME));
	if (hmenuSpeed)
            AppendMenu(hmenu, MF_ENABLED|MF_POPUP, (UINT_PTR)hmenuSpeed,
		LoadSz(IDS_SPEED));

	if (hmenuWindow || hmenuVolume || hmenuSpeed)
            AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);

        if (p->wDeviceID && p->fCanRecord && (p->dwStyle & MCIWNDF_RECORD))
            AppendMenu(hmenu, MF_ENABLED, IDM_MCINEW, LoadSz(IDS_NEW));

	if (!(p->dwStyle & MCIWNDF_NOOPEN))
	    AppendMenu(hmenu, MF_ENABLED, IDM_MCIOPEN,  LoadSz(IDS_OPEN));

        if (p->wDeviceID && p->fCanSave && (p->dwStyle & MCIWNDF_RECORD))
            AppendMenu(hmenu, MF_ENABLED, MCI_SAVE, LoadSz(IDS_SAVE));

	if (p->wDeviceID) {
	    if (!(p->dwStyle & MCIWNDF_NOOPEN)) {
		AppendMenu(hmenu, MF_ENABLED, IDM_MCICLOSE, LoadSz(IDS_CLOSE));
	
                AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
	    }

	    AppendMenu(hmenu, p->fAllowCopy ? MF_ENABLED : MF_GRAYED,
				IDM_COPY, LoadSz(IDS_COPY));
	
	    if (p->fCanConfig)
                AppendMenu(hmenu, MF_ENABLED, IDM_MCICONFIG,
			LoadSz(IDS_CONFIGURE));

	    // !!! Should we only show this in debug, or if a flag is set?
            AppendMenu(hmenu, MF_ENABLED, IDM_MCICOMMAND, LoadSz(IDS_COMMAND));
	}

	p->hmenu = hmenu;
	p->hmenuVolume = hmenuVolume;
	p->hmenuSpeed = hmenuSpeed;

        p->hbrDither = CreateDitherBrush(); // we'll need this for OwnerDraw
    }
}

//
// Set up everything for an empty window
//
STATICFN LONG MCIWndiClose(PMCIWND p, BOOL fRedraw)
{
    MCI_GENERIC_PARMS   mciGeneric;

    // Oh no!  The MCI device (probably MMP) has hooked our window proc and if
    // we close the device, it will go away, and the hook will DIE!  We need to
    // do everything BUT the closing of the device.  We'll delay that.
    if (GetWindowLongPtr(p->hwnd, GWLP_WNDPROC) != (LONG_PTR)MCIWndProc &&
    		p->wDeviceID && p->fCanWindow) {
        MCIWndString(p, FALSE, szWindowHandle, NULL);	// GO AWAY, DEVICE!
	PostMessage(p->hwnd, MCI_CLOSE, 0, p->wDeviceID);
    } else if (p->wDeviceID)
	// buggy drivers crash if we pass a null parms address
        mciSendCommand(p->wDeviceID, MCI_CLOSE, 0, (DWORD_PTR)(LPVOID)&mciGeneric);

    //
    // if the device had a palette, we need to send palette changes to
    // every window because we just deleted the palette that was realized.
    //
    if (p->fHasPalette) {
	// If we're dying this won't go through unless we SEND
	SendMessage(p->hwnd, MCIWNDM_PALETTEKICK, 0, 0);
    }

    // execute this function even if there's no deviceID since we may want
    // to gray things

    // The next timer will kill itself since wDeviceID is NULL
    p->wDeviceID = 0;
    p->achFileName[0] = 0;	// kill the filename
    p->dwMediaLen = 0;		// so next open will invalidate media

    // We don't want to redraw cuz we're opening a new file right away
    if (!fRedraw)
	return 0;

    // One of the show bits is on... clear the caption
    if (p->dwStyle & MCIWNDF_SHOWALL)
        SetWindowText(p->hwnd, LoadSz(IDS_NODEVICE));

    // Gray all the stuff on the playbar
    MCIWndiFixMyPlaybar(p);

    // Make an appropriate menu for our null device
    MCIWndiMakeMeAMenu(p);

    // Possibly snap ourselves to a small size since there's no device loaded
    // Also reposition the toolbar after it's been fixed up
    MCIWndiSize(p, 0);

    // We need to notify our "owner" that we've closed
    // note that a unicode szNull is also a valid ansi szNull.
    // so we dont have to thunk this for MCIWNDF_NOTIFYANSI
    // !!! This flag can have more than one bit set so the test is different
    if (p->dwStyle & MCIWNDF_NOTIFYMEDIA & ~MCIWNDF_NOTIFYANSI)
        NotifyOwner(p, MCIWNDM_NOTIFYMEDIA, (WPARAM)p->hwnd,
		(LPARAM)(LPVOID)szNULL);

    InvalidateRect(p->hwnd, NULL, TRUE);
    return 0;
}

#ifdef UNICODE
//
// Check to see if our parent is unicode.  We need this test to determine
// if the passed filename is unicode or ascii
//
BOOL TestForUnicode(HWND hwnd)
{
    HWND    hwndSave;

    /*
    ** Find the top level window associated with hwnd
    */
    hwndSave = hwnd;

    while ( hwndSave != (HWND)NULL ) {

        hwnd = hwndSave;
        hwndSave = GetParent( hwndSave );
    }
    return(IsWindowUnicode(hwnd));
}
#endif

//
// This is the WM_CREATE msg of our WndProc
//
STATICFN BOOL MCIWndiCreate(HWND hwnd, LPARAM lParam)
{
    PMCIWND             p;
    DWORD_PTR		dw;
    TCHAR               ach[20];
    HWND                hwndP;

    p = (PMCIWND)LocalAlloc(LPTR, sizeof(MCIWND));

    if (!p)
        return FALSE;

    SetWindowLongPtr(hwnd, 0, (UINT_PTR)p);

    p->hwnd = hwnd;
    p->hwndOwner = GetParent(hwnd);	// we'll send notifications here
    // Otherwise see if there's an owner
    if (p->hwndOwner == NULL)
	p->hwndOwner = GetWindowOwner(hwnd);
    p->alias = (UINT)(UINT_PTR)hwnd;
    p->dwStyle = GetWindowLong(hwnd, GWL_STYLE);

    DragAcceptFiles(p->hwnd, (p->dwStyle & (MCIWNDF_NOMENU | MCIWNDF_NOOPEN)) == 0);

    if (!(p->dwStyle & WS_CAPTION))
          p->dwStyle &= ~MCIWNDF_SHOWALL;

    // !!! Don't remove NOTIFY bits if there's no owner, because someone might
    // set one later.

    dw = (DWORD_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams;

    //
    // see if we are in a MDIClient
    //
    if ((p->dwStyle & WS_CHILD) && (hwndP = GetParent(hwnd))) {
        GetClassName(hwndP, ach, NUMELMS(ach));
        p->fMdiWindow = lstrcmpi(ach, szMDIClient) == 0;

        if (p->fMdiWindow)
            dw = ((LPMDICREATESTRUCT)dw)->lParam;
    }

    MCIWndiMakeMeAPlaybar(p);

//  if (szOpenFilter[0] == 0)
//      MCIWndiBuildMeAFilter(szOpenFilter);

    // Set the default timer frequencies
    p->iActiveTimerRate = ACTIVE_TIMER;
    p->iInactiveTimerRate = INACTIVE_TIMER;

    // initialize the OFN structure we'll use to open files
    p->achFileName[0] = TEXT('\0');
    p->ofn.lStructSize = sizeof(OPENFILENAME);
    p->ofn.hwndOwner = hwnd;
    p->ofn.hInstance = NULL;
//  p->ofn.lpstrFilter = szOpenFilter;
    p->ofn.lpstrCustomFilter = NULL;
    p->ofn.nMaxCustFilter = 0;
    p->ofn.nFilterIndex = 0;
;   p->ofn.lpstrFile = p->achFileName;
;   p->ofn.nMaxFile = NUMELMS(p->achFileName);
    p->ofn.lpstrFileTitle = NULL;
    p->ofn.nMaxFileTitle = 0;
    p->ofn.lpstrInitialDir = NULL;
    p->ofn.lpstrTitle = NULL; // "Open Device";
    p->ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    p->ofn.nFileOffset = 0;
    p->ofn.nFileExtension = 0;
    p->ofn.lpstrDefExt = NULL;
    p->ofn.lCustData = 0;
    p->ofn.lpfnHook = NULL;
    p->ofn.lpTemplateName = NULL;

    p->hicon = LoadIcon(hInst, MAKEINTRESOURCE(MPLAYERICON));

    // Gray stuff; disable things that aren't applicable with no device loaded
    MCIWndClose(hwnd);

#ifndef UNICODE
    if (dw && *(LPSTR)dw)     // treat extra parm as a filename
        MCIWndOpen(hwnd, (LPSTR)dw, 0);

#else //UNICODE
    /*
    ** Check that owner window is also Unicode.
    ** First check that there is a parameter to pass
    */

    if (dw) {

        // We now have to work out whether the extra parameter is
	// a unicode or ascii string pointer.  If we have gone through
	// our internal ascii open routines then the parameter will have
	// been converted to unicode.  If an application has registered the
	// mciwnd class, and then creates a window directly the conversion
	// will not have happened.

	if (fFileNameIsUnicode
	    || TestForUnicode(p->hwndOwner)) {

            if (dw && *(LPWSTR)dw)     // treat extra parm as a filename
                MCIWndOpen(hwnd, (LPWSTR)dw, 0);
        }
        else {

            if (dw && *(LPSTR)dw) {   // treat extra parm as a filename
                MCIWndOpenA(hwnd, (LPSTR)dw, 0);
            }
        }
    }

#endif

    return TRUE;
}

//
// Brings up an OpenDialog or a SaveDialog for the application and returns the
// filename.  Returns TRUE if a file name was chosen, FALSE on error or CANCEL.
//
STATICFN BOOL MCIWndOpenDlg(PMCIWND p, BOOL fSave, LPTSTR szFile, int len)
{
    BOOL f;

    // !!! Maybe this is a device name and our GetOpenFileName will fail.
    // !!! Find someway of bringing up an initial filename anyway?
    szFile[0] = 0;

    p->ofn.lpstrFile = szFile;
    p->ofn.nMaxFile = len;
    if (fSave)
        p->ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    else
        p->ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    //
    // use achReturn to hold the MCI Filter.
    //
    MCIWndiBuildMeAFilter(p->achReturn);
    p->ofn.lpstrFilter = p->achReturn;

    /* prompt user for file to open or save */
    if (fSave)
        f = GetSaveFileNamePreview(&(p->ofn));
    else
        f = GetOpenFileNamePreview(&(p->ofn));

    return f;
}

// Set our timer, if it's needed
STATICFN void MCIWndiSetTimer(PMCIWND p)
{
    // We need a TIMER to notify the "owner" when things change
    if (!(p->dwStyle & MCIWNDF_NOPLAYBAR) ||
         (p->dwStyle & MCIWNDF_NOTIFYMODE) ||
         (p->dwStyle & MCIWNDF_SHOWMODE) ||
         (p->dwStyle & MCIWNDF_SHOWPOS) ||
	 (p->dwStyle & MCIWNDF_NOTIFYPOS)) {
	p->wTimer = SetTimer(p->hwnd, TIMER1,
		p->fActive? p->iActiveTimerRate : p->iInactiveTimerRate,
		NULL);
    }
}

//
// Set the caption based on what they want to see... Name? Pos? Mode?
//
STATICFN VOID MCIWndiSetCaption(PMCIWND p)
{
    TCHAR        ach[200], achMode[40], achT[40], achPos[40];

    // Don't touch their window text if they don't want us to
    if (!(p->dwStyle & MCIWNDF_SHOWALL))
	return;

    ach[0] = TEXT('\0');

    if (p->wDeviceID == 0)
	return;

    if (p->dwStyle & MCIWNDF_SHOWNAME)
        wsprintf(ach, TEXT("%s"), FileName(p->achFileName));

    if (p->dwStyle & (MCIWNDF_SHOWPOS | MCIWNDF_SHOWMODE))
        lstrcat(ach, TEXT(" ("));

    if (p->dwStyle & MCIWNDF_SHOWPOS) {

	// Get the pretty version of the position as a string
        MCIWndGetPositionString(p->hwnd, achPos, NUMELMS(achPos));

        if (p->dwStyle & MCIWNDF_SHOWMODE)
            wsprintf(achT, TEXT("%s - "), (LPTSTR)achPos);
	else
            wsprintf(achT, TEXT("%s"), (LPTSTR)achPos);
	lstrcat(ach, achT);
    }

    if (p->dwStyle & MCIWNDF_SHOWMODE) {
        MCIWndGet(p, szStatusMode, achMode, NUMELMS(achMode));
	lstrcat(ach, achMode);
    }

    if (p->dwStyle & (MCIWNDF_SHOWPOS | MCIWNDF_SHOWMODE))
        lstrcat(ach, TEXT(")"));

    SetWindowText(p->hwnd, ach);
}

//
// Save a file.  Returns 0 for success
//
STATICFN LRESULT MCIWndiSave(PMCIWND p, WPARAM wFlags, LPTSTR szFile)
{
    TCHAR    ach[128];
    LRESULT    dw;

    //
    // If we don't have a filename to save, then get one from a dialog
    //
    if (szFile == (LPVOID)-1L) {
	lstrcpy(ach, p->achFileName);
        if (!MCIWndOpenDlg(p, TRUE, ach, NUMELMS(ach)))
            return -1;
        szFile = ach;
    }

    // !!! All good little boys should be saving to background... don't wait
    dw = MCIWndString(p, TRUE, szSave, szFile);
    if (dw == 0) {
	// Fix the filename of the current file
	lstrcpy(p->achFileName, szFile);
	// Fix the window caption
	MCIWndiSetCaption(p);
	// It's now OK to copy again. (For MCIWAVE, something freshly
	// recorded isn't persisted to the file and wouldn't have been
	// copied with our copy command, so it was disabled).
	p->fAllowCopy = TRUE;
	MCIWndiMakeMeAMenu(p);
    }
    return dw;
}

//
// Actually open a file and set up the window.  Returns 0 for success
//
STATICFN LRESULT MCIWndiOpen(PMCIWND p, WPARAM wFlags, LPTSTR szFile)
{
    DWORD               dw = 0;
    HCURSOR             hcurPrev;
    TCHAR               ach[128];
    UINT                wDeviceID;
    BOOL 		fNew = (wFlags & MCIWNDOPENF_NEW) != 0;

    //
    // We're opening an existing file, szFile is that filename
    // If we don't have a filename to open, then get one from a dialog
    //
    if (!fNew && szFile == (LPVOID)-1L) {
	lstrcpy(ach, p->achFileName);
        if (!MCIWndOpenDlg(p, FALSE, ach, NUMELMS(ach)))
            return -1;
        szFile = ach;
    }

    //
    // We want to open a new file, szFile is the device to open
    // If it's NULL, we use the current device
    //
    if (fNew && (szFile == NULL || *szFile == 0)) {
	// There is no device, so we can't do anything
	if (!p->wDeviceID)
	    return 42;	// !!! failure
        MCIWndGetDevice(p->hwnd, ach, NUMELMS(ach));
	szFile = ach;
    }

    // save the current device ID so we can put it back in case open fails.
    wDeviceID = p->wDeviceID;
    KillTimer(p->hwnd, TIMER1);	// setting the deviceID to 0 will mess up timer
    p->wDeviceID = 0;		// and if open fails, we don't want that
    p->alias++;			// use a new alias

    /*
     * Show the hourglass cursor -- who knows how long this stuff
     * will take
     */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Open a NEW file
    if (fNew) {
	dw = MCIWndGetValue(p, TRUE, szNew, 0,
		szFile, (UINT)p->alias);

    // open an existing file
    } else {

 	// first, try to open it shareable
        // don't show or update errors since we try to open it twice
        //
        // dont try shareable for "file" devices
        // silly hack to check for extension.
        //
#ifdef LATER
   !!! HACK ALERT !!!
	For NT (and probably Chicago) this test is VERY error prone.  If
	we have a long file name then we are in big trouble....
#endif
#if 0 //  IF the file exists, then do not open shareable.
    // This section, or something like it, should replace the gross hack
    // below.  Checking for a '.' as the fourth last character... yuk!
	{
	DWORD attr = GetFileAttributes(szFile);
	if (attr != (DWORD)(~0))
	    // The error value is -1, i.e. bitwise inverse of 0
            dw = 0;   // Force non shareable
        else
            dw = MCIWndGetValue(p, FALSE, szOpenShareable, 0,
                (LPTSTR)szFile, (UINT)p->alias);
	}
#else
	{
	    UINT n = lstrlen(szFile);
            if (n > 4 && szFile[n-4] == TEXT('.'))
                dw = 0;
            else
                dw = MCIWndGetValue(p, FALSE, szOpenShareable, 0,
                    (LPTSTR)szFile, (UINT)p->alias);
	}
#endif

        // Error! Try again, not shareable.
        if (dw == 0) {
            dw = MCIWndGetValue(p, FALSE, szOpen, 0,
                (LPTSTR)szFile, (UINT)p->alias);
	    // Last ditch attempt! Try AVI. It'll open anything.  This time,
	    // show, set errors.
	    if (dw == 0) {
                dw = MCIWndGetValue(p, TRUE, szOpenAVI, 0,
                    (LPTSTR)szFile, (UINT)p->alias);
	    }
	}
    }

    if (hcurPrev)
	SetCursor(hcurPrev);

    //
    // Ack! No deviceID... we failed to open
    //
    if (dw == 0)
    {
        p->wDeviceID = wDeviceID;
        MCIWndiSetTimer(p);	// Put the timer back now that DeviceID is back
//	p->achFileName[0] = 0;	// don't hurt the old filename!
	p->alias--;		// back to old alias
	// in case error box or open box wiped us out and we didn't paint
	// because our p->wDeviceID was null because of our open hack
	InvalidateRect(p->hwnd, NULL, TRUE);
        return p->dwError;
    }

    //
    // it worked, now close the old device and open the new.
    //
    if (wDeviceID)
    {
	p->wDeviceID = wDeviceID;
	p->alias--;	// back to old alias so the close might actually work
	MCIWndiClose(p, FALSE);	// don't redraw
	p->alias++;	// new alias again (ACK!)
    }

    p->wDeviceID = (UINT)dw;
    p->dwMode = (DWORD)~0L;	// first mode set will be detected as a change
    p->dwPos = (DWORD)~0L;	// first pos set will be detected as a change

    // Copy the file or device name into our filename spot.  Set the window
    // caption (if desired).
    lstrcpy(p->achFileName, szFile);
    MCIWndiSetCaption(p);	// wDeviceID must be set before calling this

    // !!! p->wDeviceType = QueryDeviceTypeMCI(p->wDeviceID);

    p->fAllowCopy = TRUE;	// until recorded into

    // Now set the playback window to be our MCI window
    p->fCanWindow = MCIWndString(p, FALSE, szWindowHandle, (UINT_PTR)p->hwnd) == 0;

    if (p->fCanWindow)
        MCIWndGetDest(p->hwnd, &p->rcNormal);
    else
	SetRect(&p->rcNormal, 0, 0, 0, 0);

    // Find out if the device supports palettes.
    p->fHasPalette = MCIWndString(p, FALSE, szStatusPalette) == 0;

    //
    // Now find out the capabilities of this device
    //

// !!! What about these ???
// MCI_GETDEVCAPS_DEVICE_TYPE      0x00000004L
// MCI_GETDEVCAPS_COMPOUND_DEVICE  0x00000006L

    // Find out if the device can record
    p->fCanRecord = MCIWndDevCaps(p, MCI_GETDEVCAPS_CAN_RECORD);

    // Find out if the device can play
    p->fCanPlay = MCIWndDevCaps(p, MCI_GETDEVCAPS_CAN_PLAY);

    // Find out if the device can save
    p->fCanSave = MCIWndDevCaps(p, MCI_GETDEVCAPS_CAN_SAVE);

    // Find out if the device can eject
    p->fCanEject = MCIWndDevCaps(p, MCI_GETDEVCAPS_CAN_EJECT);

    // Find out if the device is file based
    p->fUsesFiles = MCIWndDevCaps(p, MCI_GETDEVCAPS_USES_FILES);

    // Find out if the device has video
    p->fVideo = MCIWndDevCaps(p, MCI_GETDEVCAPS_HAS_VIDEO);

    // Find out if the device has video
    p->fAudio = MCIWndDevCaps(p, MCI_GETDEVCAPS_HAS_AUDIO);

    // Find out if the device can configure
    p->fCanConfig = (MCIWndString(p, FALSE, szConfigureTest) == 0);

    //
    //
    //

    // Now see if we support speed - try normal, half, and max
    p->fSpeed = MCIWndString(p, FALSE, szSetSpeed1000Test) == 0 &&
                MCIWndString(p, FALSE, szSetSpeed500Test) == 0 &&
                MCIWndString(p, FALSE, szSetSpeedTest, SPEED_MAX * 10) == 0;

    // Now see if we support volume - try normal, mute, and max
    p->fVolume = MCIWndString(p, FALSE, szSetVolumeTest, VOLUME_MAX * 5) ==0 &&
                 MCIWndString(p, FALSE, szSetVolume0Test) == 0;
    p->wMaxVol = 100;
    // If someone happens to support double volume, let's give it to them.
    if (MCIWndString(p, FALSE, szSetVolumeTest, VOLUME_MAX * 10) == 0)
	p->wMaxVol = 200;

    // See if the device would support tmsf mode.  If so, use milliseconds mode
    // and later on we'll fake knowing where tracks begin and end
    p->fHasTracks = (MCIWndString(p, FALSE, szSetFormatTMSF) == 0);
    if (p->fHasTracks) {
        dw = MCIWndString(p, FALSE, szSetFormatMS);
 	if (dw != 0)
	    p->fHasTracks = FALSE;
    }

    if (!p->fHasTracks) {
        // Force us into a reasonable time format
        dw = MCIWndString(p, FALSE, szSetFormatFrames);
        if (dw != 0)
	    dw = MCIWndString(p, FALSE, szSetFormatMS);
        if (dw != 0)
	    ;		// !!! What to do? Don't turn playbar off without
    }	 		// !!! destroying it...

    // Set the media length and trackbar range
    MCIWndiValidateMedia(p);

    // Fix the toolbar buttons for the new device
    MCIWndiFixMyPlaybar(p);

    // Make an appropriate menu for this device
    MCIWndiMakeMeAMenu(p);

    // We need a TIMER to notify the "owner" when things change
    MCIWndiSetTimer(p);

    // Set the size of the movie (and maybe the window) and re-draw new toolbar
    MCIWndiSize(p, p->iZoom);

#if 0 // We need the focus on our main window to get key accelerators
    // Bring focus to the thumb so caret will flash
    // I know the WM_SETFOCUS msg does this, but it seems to need to happen here
    // too.
    if (p->hwndTrackbar && GetFocus() == p->hwnd)
	SetFocus(p->hwndTrackbar);
#endif

    // We need to notify our "owner" that we've opened a new file
    // !!! This flag can have more than one bit set so the test is different
    if (p->dwStyle & MCIWNDF_NOTIFYMEDIA & ~MCIWNDF_NOTIFYANSI) {
#ifdef UNICODE
        if (p->dwStyle & MCIWNDF_NOTIFYANSI) {

            char * lpA;
            int sz;
            sz = lstrlen(szFile) + 1;
            lpA = (char *)LocalAlloc(LPTR, sz * sizeof(char));
            if (lpA) {
	       Iwcstombs(lpA, szFile, sz);
               NotifyOwner(p, MCIWNDM_NOTIFYMEDIA, (WPARAM)p->hwnd,(LPARAM)lpA);
               LocalFree ((HLOCAL)lpA);
            }
        }
        else
#endif
        {
            NotifyOwner(p, MCIWNDM_NOTIFYMEDIA, (WPARAM)p->hwnd,
			(LPARAM)szFile);
        }
    }

    // Make sure the newly opened movie paints in the window now
    InvalidateRect(p->hwnd, NULL, TRUE);

    return 0;	// success
}

STATICFN LONG MCIWndiChangeStyles(PMCIWND p, UINT mask, UINT value)
{
    DWORD	dwOldStyle = p->dwStyle;
    DWORD	dwMaskOff, dwValue, dwChanged;

    //
    // Using the mask, change the appropriate bits in the style
    //
    dwMaskOff = dwOldStyle & (~(DWORD)mask);
    dwValue   = (DWORD)mask & (DWORD)value;
    p->dwStyle = dwMaskOff | dwValue;

    //
    // Which bits changed?
    //
    dwChanged = (dwOldStyle & (DWORD)mask) ^ (dwValue & (DWORD)mask);

    //
    // We changed whether or not we want a menu button or a record button
    // on the playbar
    //
    if (dwChanged & (MCIWNDF_NOMENU | MCIWNDF_NOOPEN | MCIWNDF_RECORD)) {
	MCIWndiMakeMeAMenu(p);	// add/remove record from the menu
	// We have a playbar, so fix it
	if (!(p->dwStyle & MCIWNDF_NOPLAYBAR)) {
	    MCIWndiFixMyPlaybar(p);
	    MCIWndiSize(p, 0);
	}
    }

    //
    // We changed the show/don't show playbar flag!
    //
    if (dwChanged & MCIWNDF_NOPLAYBAR) {

 	// Remove the playbar
	if (p->dwStyle & MCIWNDF_NOPLAYBAR) {
	    DestroyWindow(p->hwndToolbar);
	    DestroyWindow(p->hwndTrackbar);
	    p->hwndToolbar = NULL;
	    p->hwndTrackbar = NULL;	
	    MCIWndiMakeMeAMenu(p);	// since toolbar's gone, menus change

	    if (!(p->dwStyle & MCIWNDF_NOAUTOSIZEWINDOW)) {
	        // Now resize the window smaller to account for the missing
		// playbar.  Don't touch the movie size.
		MCIWndiSize(p, 0);

	    // If the window isn't being resized, we may still need to grow
	    // the movie size a bit to take up the extra space where the toolbar
	    // vanished. (happens automatically in the previous case)
	    } else if (!(p->dwStyle & MCIWNDF_NOAUTOSIZEMOVIE)) {
		PostMessage(p->hwnd, WM_SIZE, 0, 0L);
	    }	

	// Add a playbar
	} else {
	    MCIWndiMakeMeAPlaybar(p);
	    MCIWndiFixMyPlaybar(p);
	    MCIWndiMakeMeAMenu(p);	// since toolbar's used, menus change

	    if (!(p->dwStyle & MCIWNDF_NOAUTOSIZEWINDOW)) {
	        // Now resize the window a little bigger to account for the new
		// playbar.  Don't touch the movie size.
		MCIWndiSize(p, 0);

	    // If the window isn't being resized, we may still need to shrink
	    // the movie size because the toolbar covers up some extra space.
	    // (happens automatically in the previous case)
	    } else if (!(p->dwStyle & MCIWNDF_NOAUTOSIZEMOVIE)) {
		PostMessage(p->hwnd, WM_SIZE, 0, 0L);

	    // Irregardless, we need to fix the toolbar
	    } else
		// Put the toolbar in a reasonable spot
		MCIWndiSizePlaybar(p);
	}
    }

    //
    // We changed a SHOW flag and need to reset the caption
    //
    if (dwChanged & MCIWNDF_SHOWALL)
	MCIWndiSetCaption(p);

    //
    // We turned the AUTOSIZEMOVIE flag on and need to resize the device.
    // This happens before AUTOSIZEWINDOW so if both flags are turned on
    // the movie will snap to the window not vice versa.
    // !!! Should we even snap it right now?
    //
    if (dwChanged & MCIWNDF_NOAUTOSIZEMOVIE &&
				!(p->dwStyle & MCIWNDF_NOAUTOSIZEMOVIE))
	PostMessage(p->hwnd, WM_SIZE, 0, 0);

    //
    // We turned the AUTOSIZEWINDOW flag on
    // Snap our window to the current movie size.
    //
    if (dwChanged & MCIWNDF_NOAUTOSIZEWINDOW &&
				!(p->dwStyle & MCIWNDF_NOAUTOSIZEWINDOW))
	MCIWndiSize(p, 0);

    DragAcceptFiles(p->hwnd, (p->dwStyle & MCIWNDF_NOMENU | MCIWNDF_NOOPEN) == 0);

    return 0;	// !!! success ?
}


//
// We're about to play.  We might want to seek to the beginning first if we're
// at the end, or seek to the end first if we're at the beginning and playing
// backwards.
//
STATICFN void MCIWndiPlaySeek(PMCIWND p, BOOL fBackwards)
{

    // Playing backwards? If we're at the beginning, seek to the end

    if (fBackwards) {
	if (MCIWndGetPosition(p->hwnd) <= MCIWndGetStart(p->hwnd))
	    MCIWndSeek(p->hwnd, MCIWND_END);
	return;
    }

    // Playing forwards.
    // If we're near the end, rewind before playing
    // Some devices are broken so we can't just test being at the end

    // Frames mode ... last or second to last frame
    if (MCIWndGetTimeFormat(p->hwnd, NULL, 0) == MCI_FORMAT_FRAMES) {
	if (MCIWndGetPosition(p->hwnd) >= MCIWndGetEnd(p->hwnd) - 1)
	    MCIWndSeek(p->hwnd, MCIWND_START);

    // Millisecond mode ... within last 1/4 second
    } else if (MCIWndGetTimeFormat(p->hwnd, NULL, 0) ==
					MCI_FORMAT_MILLISECONDS) {
	if (MCIWndGetEnd(p->hwnd) - MCIWndGetPosition(p->hwnd) < 250)
	    MCIWndSeek(p->hwnd, MCIWND_START);

    // something else ... no hack
    } else {
	if (MCIWndGetPosition(p->hwnd) == MCIWndGetEnd(p->hwnd))
	    MCIWndSeek(p->hwnd, MCIWND_START);
    }
}


//
// Handle our WM_TIMER
//
STATICFN void MCIWndiTimerStuff(PMCIWND p)
{
    DWORD	dwMode;
    DWORD	dwPos;

    //
    // Someone's interested in knowing the mode of the device
    //
    if ((p->dwStyle & MCIWNDF_NOTIFYMODE) ||
		!(p->dwStyle & MCIWNDF_NOPLAYBAR) ||
		(p->dwStyle & MCIWNDF_SHOWMODE)) {

	dwMode = MCIWndGetMode(p->hwnd, NULL, 0);

	//
	// If we haven't set the trackbar range or media length yet
	// because we weren't ready, maybe we can do it now!
	// Also, don't update media until you're done recording.
	//
	if (dwMode != MCI_MODE_NOT_READY && dwMode != MCI_MODE_OPEN &&
		dwMode != MCI_MODE_RECORD && p->fMediaValid == FALSE)
	    MCIWndiValidateMedia(p);

	//
	// No device loaded?  Time to kill our timer
	//
        if (p->wDeviceID == 0)
	    KillTimer(p->hwnd, TIMER1);

	//
	// The mode has changed!
	//
	if (dwMode != p->dwMode) {

	    p->dwMode = dwMode;

	    //
	    // Notify the "owner" of the mode change
	    //
	    if ((p->dwStyle & MCIWNDF_NOTIFYMODE))
		NotifyOwner(p, MCIWNDM_NOTIFYMODE, (WPARAM)p->hwnd, dwMode);

	    //
	    // Set the Window Caption to include the new mode
	    //
	    if ((p->dwStyle & MCIWNDF_SHOWMODE))
		MCIWndiSetCaption(p);

	    //
	    // Fix up the toolbar bitmaps if the mode has changed
	    //
	    MCIWndiPlaybarGraying(p);
	}
    }

    //
    // Someone's interested in knowing the new position
    //
    if (!(p->dwStyle & MCIWNDF_NOPLAYBAR) ||
	 (p->dwStyle & MCIWNDF_NOTIFYPOS) ||
	 (p->dwStyle & MCIWNDF_SHOWPOS)) {

	dwPos = MCIWndGetPosition(p->hwnd);

	//
	// The position has changed!
	//
	if (dwPos != p->dwPos) {

	    //
	    // Make sure start and length haven't changed too (format change) ?
	    //
	    MCIWndiValidateMedia(p);

	    p->dwPos = dwPos;

	    //
	    // Notify the "owner" of the position change
	    //
	    if ((p->dwStyle & MCIWNDF_NOTIFYPOS))
		NotifyOwner(p, MCIWNDM_NOTIFYPOS, (WPARAM)p->hwnd, dwPos);

	    //
	    // Set the Window Caption to include the new position
	    //
	    if ((p->dwStyle & MCIWNDF_SHOWPOS))
		MCIWndiSetCaption(p);

	    //
	    // Update the trackbar to the new position but not while
	    // we're dragging the thumb
	    //
	    if (!(p->dwStyle & MCIWNDF_NOPLAYBAR) && !p->fScrolling && p->dwMode != MCI_MODE_SEEK)
		SendMessage(p->hwndTrackbar, TBM_SETPOS, TRUE, dwPos);
	}
    }
}


STATICFN void MCIWndiDrop(HWND hwnd, WPARAM wParam)
{
    TCHAR       szPath[MAX_PATH];
    UINT	nDropped;

    // Get number of files dropped
    nDropped = DragQueryFile((HANDLE)wParam, (UINT)-1, NULL, 0);

    if (nDropped) {
	SetActiveWindow(hwnd);

	// Get the file that was dropped....
        DragQueryFile((HANDLE)wParam, 0, szPath, NUMELMS(szPath));

	MCIWndOpen(hwnd, szPath, 0);
    }
    DragFinish((HANDLE)wParam);     /* Delete structure alocated */
}

//--------ansi thunk functions ---------------------------------

#ifdef UNICODE

STATICFN LRESULT MCIWndiOpenA(PMCIWND p, WPARAM wFlags, LPCSTR szFile)
{
    WCHAR * lpW;
    int sz;
    LRESULT l;

    if ((szFile == NULL) || (szFile == (LPSTR)-1)) {
	return MCIWndiOpen(p, wFlags, (LPTSTR)szFile);
    }

    sz = lstrlenA(szFile) + 1;
    lpW = (WCHAR *)LocalAlloc(LPTR, sz * sizeof(WCHAR));
    if (!lpW) {
	return -1;
    }
    Imbstowcs(lpW, szFile, sz);

    l = MCIWndiOpen(p, wFlags, lpW);

    LocalFree((HLOCAL)lpW);
    return l;
}


// ansi version of MCIWndGet
STATICFN DWORD MCIWndGetA(PMCIWND p, LPSTR sz, LPSTR szRet, int len, ...)
{
    char    ach[MAX_PATH];
    int     i;
    DWORD   dw;
    va_list va;

    if (!p->wDeviceID) {
	szRet[0] = 0;
	return 0L;
    }

    for (i=0; *sz && *sz != ' '; )
	ach[i++] = *sz++;

    i += wsprintfA(&ach[i], " %d ", (UINT)p->alias);
    va_start(va, len);
    i += wvsprintfA(&ach[i], sz, va);  //!!!use varargs
    va_end(va);

    // initialize to NULL return string
    szRet[0] = 0;

    dw = mciSendStringA(ach, szRet, len, p->hwnd);

    DPF("MCIWndGetA('%s'): '%s'", (LPSTR)ach, (LPSTR)szRet);

    return dw;
}
#endif

/*--------------------------------------------------------------+
| MCIWndCommand - WM_COMMAND processing for MCIWnd class        |                         |
|                                                               |
+--------------------------------------------------------------*/

static LRESULT MCIWndCommands (
   PMCIWND p,         // IN:
   HWND    hwnd,      // IN:
   WPARAM  wParam,    // IN:
   LPARAM  lParam)    // IN:
{
   WORD wID = GET_WM_COMMAND_ID(wParam, lParam);
   HWND hwndCtl = GET_WM_COMMAND_HWND(wParam, lParam);
   WORD wNotify = GET_WM_COMMAND_CMD(wParam, lParam);

   // Check for ZOOM commands
   if (wID >= IDM_MCIZOOM && wID < IDM_MCIZOOM + 1000)
       MCIWndSetZoom(hwnd, wID - IDM_MCIZOOM);

   // !!! Hack from Hell
   // If our bogus top menu item is selected, turn it into the REAL
   // menu item closest to it.
   //
   if (wID == IDM_MCIVOLUME + VOLUME_MAX + 1)
       wID = IDM_MCIVOLUME + p->wMaxVol;
   if (wID == IDM_MCIVOLUME + VOLUME_MAX + 2)
       wID = IDM_MCIVOLUME;

   // VOLUME command? Uncheck old one, reset volume, and check new one
   // Round to the nearest 5 to match a menu identifier
   if (wID >=IDM_MCIVOLUME && wID <=IDM_MCIVOLUME + p->wMaxVol) {
	// 42 means we're auditioning changes as we drag the menu bar, but
	// we don't actually want to remember this value permanently (see
	// OwnerDraw).
        if (MCIWndSetVolume(hwnd, (wID - IDM_MCIVOLUME) * 10) == 0
                               && lParam != 42) {
           CheckMenuItem(p->hmenuVolume, p->uiHack, MF_UNCHECKED);
           CheckMenuItem(p->hmenuVolume, wID, MF_CHECKED);
       }
   }

   // !!! Hack from Hell
   // If our bogus top menu item is selected, turn it into the REAL
   // menu item closest to it.
   if (wID == IDM_MCISPEED + SPEED_MAX + 1)
       wID = IDM_MCISPEED + SPEED_MAX;
   if (wID == IDM_MCISPEED + SPEED_MAX + 2)
       wID = IDM_MCISPEED;

   // SPEED command? Uncheck old one, reset speed, and check new one
   // Round to the nearest 5 to match a menu identifier
   if (wID >=IDM_MCISPEED && wID <= IDM_MCISPEED + SPEED_MAX) {
	// 42 means we're auditioning changes as we drag the menu bar, but
	// we don't actually want to remember this value permanently (see
	// OwnerDraw).
        if (MCIWndSetSpeed(hwnd, (wID - IDM_MCISPEED) * 10) == 0
                               && lParam != 42) {
           CheckMenuItem(p->hmenuSpeed, p->uiHack, MF_UNCHECKED);
           CheckMenuItem(p->hmenuSpeed, wID, MF_CHECKED);
       }
   }

   switch(wID)
   {
       case MCI_RECORD:
           if (GetKeyState(VK_SHIFT) < 0)
           {
               //!!! toggle?
               //MCIWndRecordPreview(hwnd);
           }
           else
           {
                MCIWndRecord(hwnd);
		// would not copy newly recorded stuff anyway
		p->fAllowCopy = FALSE;
		MCIWndiMakeMeAMenu(p);
	
           }
           break;

       //            PLAY = normal play
       //      SHIFT+PLAY = play backward
       //       CTRL+PLAY = play fullscreen
       // SHIFT+CTRL+PLAY = play fullscreen backward
       //
       case MCI_PLAY:

#define MaybeRepeat (p->fRepeat ? (LPTSTR)szRepeat : (LPTSTR)szNULL)

           // NOTE: We never set errors for the repeat play, because
           // lots of device don't support it and would fail.

           if (GetKeyState(VK_SHIFT) < 0)
               // If we're at the beginning, seek to the end.
               MCIWndiPlaySeek(p, TRUE);
           else
               // If we're at the end, seek to the beginning.
               MCIWndiPlaySeek(p, FALSE);

           if (GetKeyState(VK_CONTROL) < 0)
           {
               if (GetKeyState(VK_SHIFT) < 0) {
                   if (MCIWndString(p, FALSE, szPlayFullscreenReverse,
                                                       MaybeRepeat))
                       MCIWndString(p, TRUE, szPlayFullscreenReverse,
                                                       (LPTSTR)szNULL);
               } else {
                   if (MCIWndString(p, FALSE, szPlayFullscreen,
                                                       MaybeRepeat))
                       MCIWndString(p, TRUE, szPlayFullscreen,
                                                       (LPTSTR)szNULL);
               }

           } else if (GetKeyState(VK_SHIFT) < 0) {
               if (MCIWndString(p, FALSE, szPlayReverse, MaybeRepeat))
                   MCIWndString(p, TRUE, szPlayReverse, (LPTSTR)szNULL);
           } else {
               if (MCIWndString(p, FALSE, szPlay, MaybeRepeat))
                   MCIWndString(p, TRUE, szPlay, (LPTSTR)szNULL);
           }

           // Kick ourselves to fix up toolbar since mode changed
           MCIWndiTimerStuff(p);

           break;

       case MCI_STOP:
           return MCIWndStop(hwnd);

       case MCI_PAUSE:
           return MCIWndPause(hwnd);

       case IDM_MCINEW:
           return MCIWndNew(hwnd, NULL);

       case IDM_MCIOPEN:
           return MCIWndOpenDialog(hwnd);

       case MCI_SAVE:
           return MCIWndSaveDialog(hwnd);

       case IDM_MCICLOSE:
           return MCIWndClose(hwnd);

       case IDM_MCICONFIG:
           MCIWndString(p, TRUE, szConfigure);

           // AVI's configure box might change the size (zoom by 2)
           // so we better call our size routine.
           MCIWndiSize(p, 0);

           // Taking ZOOM X 2 off might leave the outside not painted
           InvalidateRect(hwnd, NULL, TRUE);
           break;

       case IDM_MCICOMMAND:
           mciDialog(hwnd);

           // call mciwndisize?
           break;

       case IDM_COPY:
           MCIWndCopy(p);
           break;

       case IDM_MCIREWIND:
           return MCIWndSeek(hwnd, MCIWND_START);

       case IDM_MCIEJECT:
           return MCIWndEject(hwnd);

// When somebody presses a toolbar button in 16 bit Chicago, we are told
// about it via a WM_COMMAND.  32 bit Chicago and NT uses WM_NOTIFY, handled
// elsewhere.
#ifndef _WIN32
       case ID_TOOLBAR: {
           RECT rc;
           RECT rcT;
           MSG  msgT;

	   // We're only interested in a pressing of the Menu button
           if (wNotify != TBN_BEGINDRAG ||
               (UINT)hwndCtl != IDM_MENU ||
               !SendMessage(p->hwndToolbar, TB_ISBUTTONENABLED,
                       IDM_MENU, 0) ||
               !p->hmenu)
           break;

           SendMessage(p->hwndToolbar, TB_GETITEMRECT,
               (int)SendMessage(p->hwndToolbar, TB_COMMANDTOINDEX,
                       IDM_MENU, 0),
               (LPARAM)(LPVOID)&rc);
           rcT = rc;
           ClientToScreen(p->hwndToolbar, (LPPOINT)&rc);
           ClientToScreen(p->hwndToolbar, (LPPOINT)&rc + 1);

           // Push the button down (accelerator won't have done this)
           SendMessage(p->hwndToolbar, TB_PRESSBUTTON, IDM_MENU,
               TRUE);

           // Don't allow error dlgs to come up while we're tracking.
           // That would cause windows to shatter and send shrapnel
           // flying.
           p->fTracking = TRUE;
           TrackPopupMenu(p->hmenu, 0, rc.left, rc.bottom - 1, 0,
                       hwnd, &rc);  // don't dismiss menu inside button
           p->fTracking = FALSE;

           // Bring the button back up.
           SendMessage(p->hwndToolbar, TB_PRESSBUTTON, IDM_MENU,
               FALSE);

           // What if we press the menu button to make the menu go
           // away?  It's just going to bring the menu back up again!
           // So we need to pull the click out of the queue.
           // There are bugs in the toolbar code to prevent me from
           // doing this any other way (like disabling the button)
           if (PeekMessage(&msgT, p->hwndToolbar, WM_LBUTTONDOWN,
                               WM_LBUTTONDOWN, PM_NOREMOVE)) {
#ifdef _WIN32
               POINT pt = {MAKEPOINTS(msgT.lParam).x,
                           MAKEPOINTS(msgT.lParam).y };
#else
               POINT pt = MAKEPOINT(msgT.lParam);
#endif
               if (PtInRect(&rcT, pt))
                   PeekMessage(&msgT, p->hwndToolbar, WM_LBUTTONDOWN,
                               WM_LBUTTONDOWN, PM_REMOVE);
           }
           break;
       }
#endif

       default:
           break;
   }

   return 0;
}


/*--------------------------------------------------------------+
| SubClassedTrackbarWndProc - eat key presses that do something |
| 	special so trackbar never sees them.                    |
+--------------------------------------------------------------*/
LRESULT CALLBACK _LOADDS SubClassedTrackbarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSCHAR:
	    return SendMessage(GetParent(hwnd), msg, wParam, lParam);
    }

    return CallWindowProc(fnTrackbarWndProc, hwnd, msg, wParam, lParam);
}


/*--------------------------------------------------------------+
| MCIWndProc - MCI window's window proc                         |
|                                                               |
+--------------------------------------------------------------*/
LRESULT CALLBACK _LOADDS MCIWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PMCIWND             p;
    DWORD	    	dw;
    HDC		    	hdc;
    PAINTSTRUCT	    	ps;
    DWORD               dwPos;
    POINT		pt;
    MINMAXINFO FAR 	*lpmmi;
    RECT		rc;
    BOOL                f;
    TCHAR               ach[80];
    MCI_GENERIC_PARMS   mciGeneric;
    LPRECT              prc;
    int			i;
    HWND		hwndD;

    p = (PMCIWND)(UINT)GetWindowLong(hwnd, 0);

    switch(msg){
        case WM_CREATE:
            if (!MCIWndiCreate(hwnd, lParam))
                return -1;

            break;

        // Make the trackbar background BTNFACE to match the colour scheme
#ifdef _WIN32
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORDLG:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORMSGBOX:
	case WM_CTLCOLORSCROLLBAR:
	case WM_CTLCOLORSTATIC:
    	    if ((HWND)lParam == p->hwndTrackbar) {
#else
        case WM_CTLCOLOR:
            if ((HWND)LOWORD(lParam) == p->hwndTrackbar) {
#endif
            //  return (LRESULT)(UINT)GetStockObject(LTGRAY_BRUSH);
		SetBkColor((HDC)wParam, GetSysColor(COLOR_BTNFACE));
		SetTextColor((HDC)wParam, GetSysColor(COLOR_BTNTEXT));
                return (LRESULT)(UINT_PTR)GetSysColorBrush(COLOR_BTNFACE);
            }
            break;

	case MCI_SAVE:
	    // wParam presently unused and not given by the macro
            return MCIWndiSave(p, wParam, (LPTSTR)lParam);

#ifdef _WIN32
        case MCIWNDM_OPEN:
#else
        case MCI_OPEN:
#endif
            return MCIWndiOpen(p, wParam, (LPTSTR)lParam);

#ifdef UNICODE
        // ansi thunk for above
        case MCIWNDM_OPENA:
            return MCIWndiOpenA(p, wParam, (LPSTR)lParam);
#endif

	case MCIWNDM_NEW:
	    return MCIWndiOpen(p, MCIWNDOPENF_NEW, (LPTSTR)lParam);

#ifdef UNICODE
	//ansi thunk for above
	case MCIWNDM_NEWA:
	    return MCIWndiOpenA(p, MCIWNDOPENF_NEW, (LPSTR) lParam);
#endif
	
        case MCI_PLAY:

	    if (!p->wDeviceID)
		return 0;
	    // Seek to the beginning if we're near the end
	    MCIWndiPlaySeek(p, FALSE);

	case MCI_STOP:
	case MCI_PAUSE:
	case MCI_RESUME:
	case MCI_RECORD:

	    dw = 0;
	    // Report/Show errors for this
	    if (p->wDeviceID) {
		// buggy drivers crash if we pass a null parms address
	        dw = mciSendCommand(p->wDeviceID, msg, 0,
			(DWORD_PTR)(LPVOID)&mciGeneric);
		if (dw == 0 && msg == MCI_RECORD) {
		    // RECORDING dirties MCIWAVE and what we would copy would
		    // still be the old file, so just disable it to save grief.
		    p->fAllowCopy = FALSE;
		    MCIWndiMakeMeAMenu(p);
		}
		MCIWndiHandleError(p, dw);
		// kick ourselves to show new Mode
		MCIWndiTimerStuff(p);
	    }
	    return dw;

	case MCIWNDM_PLAYREVERSE:

	    if (!p->wDeviceID)
		return 0;
	    // Seek to the end if we're near the beginning
	    MCIWndiPlaySeek(p, TRUE);
            dw = MCIWndString(p, TRUE, szPlayReverse, (LPTSTR)szNULL);
	    // kick ourselves to show new Mode
	    MCIWndiTimerStuff(p);
	    return dw;

        case MCI_CLOSE:
	    if (lParam)
		// We delayed the closing of the MCI device because the MCI
		// device may have hooked our window proc and be on our stack
		// and killing it before would have destroyed the universe.
		// buggy drivers crash if we pass a null parms address
        	return mciSendCommand((UINT)lParam, MCI_CLOSE, 0,
			(DWORD_PTR)(LPVOID)&mciGeneric);
		
	    else
	        // Do all the stuff for closing
	        return MCIWndiClose(p, TRUE);

	case MCIWNDM_EJECT:
	    return MCIWndString(p, TRUE, szSetDoorOpen);

	case MCIWNDM_PLAYFROM:
	    if (lParam == MCIWND_START)
	        dw = MCIWndString(p, TRUE, szPlayFrom, MCIWndGetStart(hwnd));
	    else
	        dw = MCIWndString(p, TRUE, szPlayFrom, (LONG)lParam);
	    MCIWndiTimerStuff(p);	// kick ourselves to see mode change
	    return dw;

	case MCIWNDM_PLAYTO:
	    if (lParam == MCIWND_END)
	        dw = MCIWndString(p, TRUE, szPlayTo, MCIWndGetEnd(hwnd));
	    else if (lParam == MCIWND_START)
	        dw = MCIWndString(p, TRUE, szPlayTo, MCIWndGetStart(hwnd));
	    else
	        dw = MCIWndString(p, TRUE, szPlayTo, (LONG)lParam);
	    MCIWndiTimerStuff(p);	// kick ourselves to see mode change
	    return dw;

	case MCI_STEP:
            return MCIWndString(p, TRUE, szStep, (LONG)lParam);

	case MCI_SEEK:
	    if (lParam == MCIWND_START)
                return MCIWndString(p, TRUE, szSeek, MCIWndGetStart(hwnd));
	    else if (lParam == MCIWND_END)
                return MCIWndString(p, TRUE, szSeek, MCIWndGetEnd(hwnd));
	    else
                return MCIWndString(p, TRUE, szSeek, (LONG)lParam);

	case MCIWNDM_SETREPEAT:
	    p->fRepeat = (BOOL)lParam;
	    return 0;

	case MCIWNDM_GETREPEAT:
	    return p->fRepeat;

	case MCIWNDM_GETDEVICEID:
	    return p->wDeviceID;

	case MCIWNDM_GETALIAS:
	    return p->alias;

	case MCIWNDM_GETMODE:
            if (lParam)
                MCIWndGet(p, szStatusMode, (LPTSTR)lParam, (UINT)wParam);
	    return MCIWndStatus(p, MCI_STATUS_MODE, MCI_MODE_NOT_READY);

#ifdef UNICODE
	// ansi thunk for above
	case MCIWNDM_GETMODEA:
    	    if (lParam) {
		MCIWndGetA(p, szStatusModeA, (LPSTR)lParam, (UINT)wParam);
	    }
	    return MCIWndStatus(p, MCI_STATUS_MODE, MCI_MODE_NOT_READY);
#endif

	// Return the position as a string if they give us a buffer
	case MCIWNDM_GETPOSITION:
#ifdef UNICODE
	case MCIWNDM_GETPOSITIONA:
#endif
            if (lParam) {
		// If we can do tracks, let's give them a pretty string
		if (p->fHasTracks)
        	    MCIWndString(p, FALSE, szSetFormatTMSF);
#ifdef UNICODE
		if (msg == MCIWNDM_GETPOSITIONA) {
		    MCIWndGetA(p, szStatusPositionA,
			(LPSTR)lParam,(UINT)wParam);
		}
		else
#endif
		{
		    MCIWndGet(p, szStatusPosition,
			(LPTSTR)lParam,(UINT)wParam);
		}

		if (p->fHasTracks)
        	    MCIWndString(p, FALSE, szSetFormatMS);
	    }
	    return MCIWndStatus(p, MCI_STATUS_POSITION, 0);


	case MCIWNDM_GETSTART:
	    // Start is a stupid command that works differently
            return MCIWndGetValue(p, FALSE, szStatusStart, 0);

	case MCIWNDM_GETLENGTH:
	    return MCIWndStatus(p, MCI_STATUS_LENGTH, 0);

	case MCIWNDM_GETEND:
	    return MCIWndGetStart(hwnd) + MCIWndGetLength(hwnd);

        case MCIWNDM_SETZOOM:
            p->iZoom = (int)lParam;
	    MCIWndiSize(p, (int)lParam);
            return 0;

        case MCIWNDM_GETZOOM:
            return p->iZoom ? p->iZoom : 100;

        case MCIWNDM_GETPALETTE:
            return MCIWndGetValue(p, FALSE, szStatusPalette, 0);

        case MCIWNDM_SETPALETTE:
            return MCIWndString(p, TRUE, szSetPalette, (HPALETTE)wParam);

	//
	// Returns our error code
	//
	case MCIWNDM_GETERROR:
	    if (lParam) {
                mciGetErrorString(p->dwError, (LPTSTR)lParam, (UINT)wParam);
	    }
	    dw = p->dwError;
	//    p->dwError = 0L;	// we never clear the error
	    return dw;

#ifdef UNICODE
	// ansi thunk for above
	case MCIWNDM_GETERRORA:
	    if (lParam) {
		mciGetErrorStringA(p->dwError, (LPSTR)lParam, (UINT)wParam);
	    }
	    return p->dwError;
#endif

	case MCIWNDM_GETFILENAME:
	    if (lParam)
                lstrcpyn((LPTSTR)lParam, p->achFileName, (UINT)wParam);
            return (lParam == 0);    // !!!

#ifdef UNICODE
	// ansi thunk for above
	case MCIWNDM_GETFILENAMEA:
            if (lParam) {
		Iwcstombs((LPSTR)lParam, p->achFileName, (UINT)wParam);
	    }
	    return (lParam == 0);
#endif

	case MCIWNDM_GETDEVICE:
	    if (lParam)
                return MCIWndGet(p, szSysInfo, (LPTSTR)lParam,
		    (UINT)wParam);
	    return 42;	// !!!

#ifdef UNICODE
	// ansi thunk for above
	case MCIWNDM_GETDEVICEA:
	    if (lParam) {
		return MCIWndGetA(p, szSysInfoA, (LPSTR)lParam, (UINT)wParam);
	    } else {
		return 42; // why do they do this??
	    }
#endif


	case MCIWNDM_SETVOLUME:
	    // Uncheck the current volume, and check the new one.
	    // Round to nearest 5 so it matches a menu item identifier
            i = ((int)MCIWndGetValue(p, FALSE, szStatusVolume, 1000) / 50) * 5;
	    if (p->hmenuVolume)
                CheckMenuItem(p->hmenuVolume, IDM_MCIVOLUME + i, MF_UNCHECKED);
            dw = MCIWndString(p, TRUE, szSetVolume, (int)lParam);
            i = ((int)lParam / 50) * 5;
	    if (p->hmenuVolume)
                CheckMenuItem(p->hmenuVolume, IDM_MCIVOLUME + i, MF_CHECKED);
	    return dw;

	case MCIWNDM_GETVOLUME:
	    return MCIWndGetValue(p, FALSE, szStatusVolume, 1000);

	case MCIWNDM_SETSPEED:
	    // Uncheck the current speed, and check the new one.
	    // Round to nearest 5 so it matches a menu item identifier
            i = ((int)MCIWndGetValue(p, FALSE, szStatusSpeed, 1000) / 50) * 5;
	    if (p->hmenuSpeed)
                CheckMenuItem(p->hmenuSpeed, IDM_MCISPEED + i, MF_UNCHECKED);
            dw = MCIWndString(p, TRUE, szSetSpeed, (int)lParam);
            i = ((int)lParam / 50) * 5;
	    if (p->hmenuSpeed)
                CheckMenuItem(p->hmenuSpeed, IDM_MCISPEED + i, MF_CHECKED);
	    return dw;

	case MCIWNDM_GETSPEED:
            return MCIWndGetValue(p, FALSE, szStatusSpeed, 1000);

	case MCIWNDM_SETTIMEFORMAT:
            dw = MCIWndString(p, TRUE, szSetFormat, (LPTSTR)lParam);
	    MCIWndiValidateMedia(p);
	    return dw;

#ifdef UNICODE
	// ansi thunk for above
	case MCIWNDM_SETTIMEFORMATA:
	{
	    WCHAR * lpW;
	    int sz = lstrlenA( (LPSTR) lParam) + 1;
	    lpW = (WCHAR *) LocalAlloc(LPTR, sz * sizeof(WCHAR));
            if (lpW) {
                Imbstowcs(lpW, (LPSTR) lParam, sz);
		dw = MCIWndString(p, TRUE, szSetFormat, (LPTSTR)lpW);
		MCIWndiValidateMedia(p);
		LocalFree((HLOCAL)lpW);
	    } else {
		dw = MCIERR_OUT_OF_MEMORY;
	    }
	    return dw;
	}
#endif


	case MCIWNDM_GETTIMEFORMAT:
	    if (lParam)
                MCIWndGet(p, szStatusFormat, (LPTSTR)lParam, (UINT)wParam);
	    return MCIWndStatus(p, MCI_STATUS_TIME_FORMAT, 0);	// !!!

#ifdef UNICODE
	// ansi thunk for above
	case MCIWNDM_GETTIMEFORMATA:
	    if (lParam) {
		MCIWndGetA(p, szStatusFormatA, (LPSTR) lParam, (UINT)wParam);
	    }
	    return MCIWndStatus(p, MCI_STATUS_TIME_FORMAT, 0);
#endif


	case MCIWNDM_VALIDATEMEDIA:
	    MCIWndiValidateMedia(p);
	    break;

 	case MCIWNDM_GETSTYLES:
	    return (UINT)(p->dwStyle & 0xFFFF);

 	case MCIWNDM_CHANGESTYLES:
	    return MCIWndiChangeStyles(p, (UINT)wParam, (UINT)lParam);

        case MCIWNDM_SETACTIVETIMER:
	    if (wParam)
	        p->iActiveTimerRate = (unsigned)wParam;
	
	    if (p->fActive) {
                KillTimer(hwnd, TIMER1);
                MCIWndiSetTimer(p);
	    }
	    break;

        case MCIWNDM_SETINACTIVETIMER:
	    if (wParam)
	        p->iInactiveTimerRate = (unsigned)wParam;
	
	    if (!p->fActive) {
                KillTimer(hwnd, TIMER1);
                MCIWndiSetTimer(p);
	    }
	    break;

        case MCIWNDM_SETTIMERS:
	    if (wParam)
	        p->iActiveTimerRate = (unsigned)wParam;
	    if (lParam)
                p->iInactiveTimerRate = (unsigned)lParam;

            KillTimer(hwnd, TIMER1);
            MCIWndiSetTimer(p);

	    break;

	case MCIWNDM_GETACTIVETIMER:
	    return p->iActiveTimerRate;

	case MCIWNDM_GETINACTIVETIMER:
	    return p->iInactiveTimerRate;

        case MCIWNDM_SENDSTRING:
	    //
	    // App wants to send a string command.

	    // special case the CLOSE command to do our clean up
	    lstrcpyn(ach, (LPTSTR)lParam, lstrlen(szClose) + 1);
            if (lstrcmpi((LPTSTR)ach, szClose) == 0)
		return MCIWndClose(hwnd);

	    // Always sets/clears our error code
            dw = MCIWndGet(p, (LPTSTR)lParam, p->achReturn,NUMELMS(p->achReturn));
	    MCIWndiHandleError(p, dw);
	    // kick ourselves in case mode changed from this command
	    MCIWndiTimerStuff(p);
            return dw;

#ifdef UNICODE
	// ansi thunk for above
	// convert string and then re-send message rather than using
	// MCIWndGetA since we need to store the error string in UNICODE.
	case MCIWNDM_SENDSTRINGA:
	{
            WCHAR * lpW;
            int sz = lstrlenA( (LPSTR) lParam) + 1;
            lpW = (WCHAR *)LocalAlloc(LPTR, sz * sizeof(WCHAR));
            if (lpW) {
                Imbstowcs(lpW, (LPSTR) lParam, sz);

		dw = (DWORD) MCIWndProc(hwnd, MCIWNDM_SENDSTRING, 0,
                        (LPARAM) (LPTSTR)lpW);

		LocalFree((HLOCAL)lpW);
	    } else {
		dw = MCIERR_OUT_OF_MEMORY;
	    }
	    return dw;
	}
#endif

	// Gets the return string from the most recent MCIWndSendString()
        case MCIWNDM_RETURNSTRING:
	    if (lParam)
                lstrcpyn((LPTSTR)lParam, p->achReturn, (DWORD) wParam);
            return (lParam == 0);    // !!!

#ifdef UNICODE
	// ansi thunk for above
	case MCIWNDM_RETURNSTRINGA:
            if (lParam) {
		Iwcstombs((LPSTR) lParam, p->achReturn, (DWORD) wParam);
	    }
	    return (lParam == 0);
#endif

        case MCIWNDM_REALIZE:
	    // buggy drivers crash if we pass a null parms address
            dw = mciSendCommand(p->wDeviceID, MCI_REALIZE,
                (BOOL)wParam ? MCI_ANIM_REALIZE_BKGD : MCI_ANIM_REALIZE_NORM,
		(DWORD_PTR)(LPVOID)&mciGeneric);
            break;

        case MCIWNDM_GET_SOURCE:
	    MCIWndRect(p, (LPRECT)lParam, TRUE);
	    return 0L;

	case MCIWNDM_GET_DEST:
	    MCIWndRect(p, (LPRECT)lParam, FALSE);
	    return 0L;

        case MCIWNDM_PUT_SOURCE:
            prc = (LPRECT)lParam;

            return MCIWndString(p, FALSE, szPutSource,
		  prc->left, prc->top,
		  prc->right - prc->left,
                  prc->bottom - prc->top);

	case MCIWNDM_PUT_DEST:
	    prc = (LPRECT)lParam;

	    return MCIWndString(p, FALSE, szPutDest,
		  prc->left, prc->top,
		  prc->right - prc->left,
                  prc->bottom - prc->top);

        case MCIWNDM_CAN_PLAY:   return p->fCanPlay;
        case MCIWNDM_CAN_WINDOW: return p->fCanWindow;
        case MCIWNDM_CAN_RECORD: return p->fCanRecord;
        case MCIWNDM_CAN_SAVE:   return p->fCanSave;
        case MCIWNDM_CAN_EJECT:  return p->fCanEject;
        case MCIWNDM_CAN_CONFIG: return p->fCanConfig;

	case WM_TIMER:

	    // This timer means we've moved the mouse off of the menu and need
	    // to snap the thumb back to the original value
	    if (wParam == TIMER2) {
		KillTimer(hwnd, TIMER2);

		// If only this would cause OwnerDraw to execute, we could see
		// the thumb bounce back to it's default place.  Alas, no can do
		//CheckMenuItem(p->hmenuHack, p->uiHack, MF_UNCHECKED);
		//CheckMenuItem(p->hmenuHack, p->uiHack, MF_CHECKED);

		// This code will at least set the parameter back even though
		// the thumb won't physically move.
		if (p->hmenuHack == p->hmenuVolume)
		    MCIWndSetVolume(hwnd, (p->uiHack - IDM_MCIVOLUME) * 10);
		else
		    MCIWndSetSpeed(hwnd, (p->uiHack - IDM_MCISPEED) * 10);
	    }

	    //
	    // This is not our timer. Bail.
	    //
	    if (wParam != TIMER1)
		break;

	    MCIWndiTimerStuff(p);

	    break;

	case WM_GETMINMAXINFO:

            // bug fix?
            //
            if (!p)
                break;

	    // We don't want anybody messing with the window size
	    if (p->dwStyle & MCIWNDF_NOAUTOSIZEWINDOW)
		break;

	    // do we have a playbar?
	    f = !(p->dwStyle & MCIWNDF_NOPLAYBAR);

            lpmmi = (MINMAXINFO FAR *)(lParam);
            SetRect(&rc, 0, 0, SMALLEST_WIDTH, f ? TB_HEIGHT : 0);
            AdjustWindowRect(&rc, GetWindowLong(hwnd, GWL_STYLE), FALSE);
            lpmmi->ptMinTrackSize.y = rc.bottom - rc.top;
            lpmmi->ptMinTrackSize.x = rc.right - rc.left;

            if (!(p->wDeviceID) || !(p->fCanWindow))
                    lpmmi->ptMaxTrackSize.y = lpmmi->ptMinTrackSize.y;
            break;

        case WM_SIZE:

	    GetClientRect(hwnd, &rc);

	    if (!IsIconic(hwnd)) {
		// if we have a playbar, fix it up to the new size
                f = !(p->dwStyle & MCIWNDF_NOPLAYBAR);

                if (f) {
                    MCIWndiSizePlaybar(p);
                    rc.bottom -= TB_HEIGHT;
                }

                if (!(p->dwStyle & MCIWNDF_NOAUTOSIZEMOVIE))
                    MCIWndString(p, FALSE, szPutDest, 0,0, rc.right, rc.bottom);

	    } else {
                if (!(p->dwStyle & MCIWNDF_NOAUTOSIZEMOVIE))
                    MCIWndString(p, FALSE, szPutDest, 0,0, rc.right, rc.bottom);
	    }

	    // We need to notify the "owner" that our size changed.  Watch for
	    // excessive recursion, which happens with the VfW 1.1d MCIPUZZL
	    // sample app.  That sample's NOTIFYSIZE handler resizes the owner
	    // window as a function of the MCI window size using
	    // AdjustWindowRect().  AdjustWindowRect() used to have a bug which
	    // MCIPUZZL adjusted for by adding 1 to the window height.
	    // Since the AdjustWindowRect() bug has been fixed, that sample
	    // would recurse infinitely because it will size the owner window
	    // one larger which will then resize the MCIWnd one larger which
	    // will then notify the owner which will resize the owner wnd one
	    // larger, etc...
	    if (p->cOnSizeReentered < 3) {
		p->cOnSizeReentered++;
		if (p->dwStyle & MCIWNDF_NOTIFYSIZE) {
		    NotifyOwner(p, MCIWNDM_NOTIFYSIZE, (WPARAM)p->hwnd, (LPARAM)NULL);
		}
		p->cOnSizeReentered--;
	    }
	    break;

        case WM_RBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
        case WM_PARENTNOTIFY:

	    // If we haven't got a menu, or we don't want it, bail
            if (!p->hmenu || p->dwStyle & MCIWNDF_NOMENU)
                break;

	    // If this is not a right button down, bail
            if (msg == WM_PARENTNOTIFY && wParam != WM_RBUTTONDOWN)
                break;

	    GetCursorPos(&pt);

	    // Don't allow error dlgs to come up while we're tracking.  That
	    // would cause windows to enter the twilight zone.
	    p->fTracking = TRUE;
	    TrackPopupMenu(p->hmenu,
		TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
	    p->fTracking = FALSE;

            return 0;

        case WM_PALETTECHANGED:
	    if ((HWND)wParam != hwnd && p->fHasPalette)
		InvalidateRect(hwnd, NULL, FALSE);
	    break;

	case WM_QUERYNEWPALETTE:
	    if (p->fHasPalette)
		MCIWndRealize(hwnd, FALSE);
            break;

	// Send a WM_PALETTECHANGED to everyone in the system.  We need to do
	// this manually sometimes because GDI sucks the big wazoo.
	case MCIWNDM_PALETTEKICK:

	    hwndD = GetDesktopWindow();	// tell everyone DESKTOP changed it
            PostMessage((HWND)-1, WM_PALETTECHANGED, (WPARAM)hwndD, 0);

	    // DESKTOP won't repaint if we give it it's own HWND, so pick a
	    // random window and PRAY it'll stay valid.
	    hwndD = GetActiveWindow();
	    hwndD = GetWindow(hwndD, GW_HWNDLAST);
            PostMessage(GetDesktopWindow(), WM_PALETTECHANGED, (WPARAM)hwndD,0);
	    return 0;

	case MCIWNDM_OPENINTERFACE:
	    wsprintf(ach, szInterface, lParam);
            return MCIWndiOpen(p, 0, (LPTSTR)ach);

	case MCIWNDM_SETOWNER:
	    p->hwndOwner = (HWND)wParam;
	    return 0;

        case WM_ERASEBKGND:
            if (p->fCanWindow) {
                MCIWndRect(p, &rc, FALSE);
                SaveDC((HDC)wParam);
                ExcludeClipRect((HDC)wParam, rc.left, rc.top, rc.right,
                    rc.bottom);
                DefWindowProc(hwnd, msg, wParam, lParam);
                RestoreDC((HDC)wParam, -1);
                return 0;
            }
            break;

        case WM_PAINT:
	    hdc = BeginPaint(hwnd, &ps);
            if (p->wDeviceID && p->fCanWindow)
            {
                MCI_ANIM_UPDATE_PARMS mciUpdate;
		
                mciUpdate.hDC = hdc;

                dw = mciSendCommand(p->wDeviceID, MCI_UPDATE,
                            MCI_ANIM_UPDATE_HDC | MCI_WAIT |
                            MCI_DGV_UPDATE_PAINT,
                            (DWORD_PTR)(LPVOID)&mciUpdate);

                if (dw != 0) /* if the update fails then erase */
                    DefWindowProc(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);
	    } else if (IsIconic(hwnd)) {
		DefWindowProc(hwnd, WM_ICONERASEBKGND, (WPARAM)hdc, 0);
		DrawIcon(ps.hdc, 0, 0, p->hicon);
	    }
	    EndPaint(hwnd, &ps);
	    break;
	
	case WM_KEYDOWN:
		switch(wParam) {
		    case VK_LEFT:
			SendMessage(hwnd, WM_HSCROLL, TB_LINEUP, 0); return 0;
		    case VK_RIGHT:
			SendMessage(hwnd, WM_HSCROLL, TB_LINEDOWN, 0); return 0;
		    case VK_PRIOR:
			SendMessage(hwnd, WM_HSCROLL, TB_PAGEUP, 0); return 0;
		    case VK_NEXT:
			SendMessage(hwnd, WM_HSCROLL, TB_PAGEDOWN, 0); return 0;
		    case VK_HOME:
			SendMessage(hwnd, WM_HSCROLL, TB_TOP, 0); return 0;
		    case VK_END:
			SendMessage(hwnd, WM_HSCROLL, TB_BOTTOM, 0); return 0;

		    case VK_UP:
		    case VK_DOWN:
			dw = MCIWndGetValue(p, FALSE, szStatusVolume, 1000);
			if (wParam == VK_UP)
			    i = min((int)p->wMaxVol * 10, (int) dw + 100);
			else
			    i = max(0, (int) dw - 100);
			
			MCIWndSetVolume(p->hwnd, i);
			return 0;	// break will ding
		    default:
			break;
		}
	    break;


	case WM_KEYUP:
	    switch(wParam) {
		case VK_LEFT:
		case VK_RIGHT:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_HOME:
		case VK_END:
		    if (p->fScrolling)
		        SendMessage(hwnd, WM_HSCROLL, TB_ENDTRACK, 0);
		    return 0;	// break will ding
                case VK_ESCAPE:
                    MCIWndStop(hwnd);
                    return 0;
		default:
		    break;
	    }
			
    	    if (GetKeyState(VK_CONTROL) & 0x8000) {
		switch(wParam) {
		    case '1':
		    case '2':
		    case '3':
		    case '4':
			// Don't let somebody resize us if we're not normally
			// resizable.
			if (!(p->dwStyle & MCIWNDF_NOAUTOSIZEWINDOW) &&
				(p->dwStyle & WS_THICKFRAME))
			    MCIWndSetZoom(hwnd, 100 * (wParam - '0'));
			return 0;	// break will ding
		    case 'P':
			MCIWndPlay(hwnd); return 0;
		    case 'S':
			MCIWndStop(hwnd); return 0;
		    case 'D':
			// The key accelerator should only work if we gave
			// them a menu for this command
			if (!(p->dwStyle & MCIWNDF_NOMENU))
			    PostMessage(hwnd, WM_COMMAND, IDM_MCICONFIG, 0);
			return 0;
		    case 'C':
			PostMessage(hwnd, WM_COMMAND, IDM_COPY, 0); return 0;
		    case VK_F5:
			// The key accelerator should only work if we gave
			// them a menu for this command
			if (!(p->dwStyle & MCIWNDF_NOMENU))
			    PostMessage(hwnd, WM_COMMAND, IDM_MCICOMMAND, 0);
			return 0;
		    case 'F':
		    case 'O':
			if (!(p->dwStyle & MCIWNDF_NOOPEN))
			    MCIWndOpenDialog(hwnd);
			return 0;
		    case 'M':
			PostMessage(hwnd, WM_COMMAND,
				GET_WM_COMMAND_MPS(ID_TOOLBAR, IDM_MENU,
					TBN_BEGINDRAG));
			return 0;
		    default:
			break;
		}
	    }
	    break;

	case WM_SYSCHAR:
	    switch(wParam) {
	        case '1':
	        case '2':
	        case '3':
	        case '4':
		    // Don't let somebody resize us if we're not normally
		    // resizable.
		    if (!(p->dwStyle & MCIWNDF_NOAUTOSIZEWINDOW) &&
				(p->dwStyle & WS_THICKFRAME))
			MCIWndSetZoom(hwnd, 100 / ((UINT) wParam - '0'));
		    return 0;	// break will ding
	        default:
		    break;
	    }
	    break;

	case WM_HSCROLL:

#define FORWARD	 1
#define BACKWARD 2

            dwPos = (DWORD) SendMessage(p->hwndTrackbar, TBM_GETPOS, 0, 0);

	    // nothing to do - spurious END without BEGIN
	    if (!p->fScrolling && wParam == TB_ENDTRACK)
		break;

	    // Turn seek exactly off while scrolling and remember what it was
	    // Also, remember if we were playing just before we seeked so we
	    // can continue playing after the seek (so moving the thumb doesn't
	    // stop the play).
            if (!p->fScrolling) {
                p->fScrolling = TRUE;
		// Wierd artifacts happen if you turn seek exactly off while
		// seeking. You see the key frame and then the actual frame.
		// Nobody can remember why this was ever a good idea.
		//p->fSeekExact = MCIWndSeekExact(p, FALSE);
		// if we're still seeking from last time, don't change this
		if (p->dwMode != MCI_MODE_SEEK)
                    p->fPlayAfterSeek = (p->dwMode == MCI_MODE_PLAY);
		// Now which direction was it playing in?
		if (p->fPlayAfterSeek) {
                    MCIWndGet(p, szStatusForward, ach, NUMELMS(ach));
                    if (ach[0] == TEXT('F') || ach[0] == TEXT('f'))
			p->fPlayAfterSeek = BACKWARD;
		    else	// by default, choose forward. Some devices
				// don't understand this command and fail.
			p->fPlayAfterSeek = FORWARD;
		}
            }

	    switch(wParam)
	    {
		case TB_LINEUP:
		    dwPos--; break;
		case TB_LINEDOWN:
		    dwPos++; break;
		case TB_PAGEUP:
		    if (p->fHasTracks) {
			dwPos = MCIWndiPrevTrack(p); break;
		    } else {
                        dwPos -= p->dwMediaLen / 16; break;
		    }
		case TB_PAGEDOWN:
		    if (p->fHasTracks) {
			dwPos = MCIWndiNextTrack(p); break;
		    } else {
                        dwPos += p->dwMediaLen / 16; break;
		    }
		case TB_TOP:
		    dwPos = p->dwMediaStart; break;
		case TB_BOTTOM:
		    dwPos = p->dwMediaStart + p->dwMediaLen; break;
		case TB_THUMBTRACK:
		case TB_THUMBPOSITION:
		    break;
		case TB_ENDTRACK:
		    // All done.  Put seek exact back to what it used to be
		    p->fScrolling = FALSE;
		    // Don't do this anymore (see above)
		    //MCIWndSeekExact(p, p->fSeekExact);
		    break;

		default:
		    break;

	    }

	    // If we're windowed, update the position as we scroll.  That would
	    // be annoying for CD or wave, though.  Also, update as soon as we
	    // let go of the thumb.  Also, never seek around while we're open
	    // or not ready.
	    if ((p->fCanWindow || !p->fScrolling) && p->dwMode != MCI_MODE_OPEN
					&& p->dwMode != MCI_MODE_NOT_READY) {
	        MCIWndSeek(hwnd, dwPos);
		MCIWndiTimerStuff(p);	// kick ourselves to update mode
	    }

	    // After we're done, if we were playing before, go back to playing
	    if (!p->fScrolling && p->fPlayAfterSeek) {
		if (p->fPlayAfterSeek == FORWARD)
                    MCIWndPlay(hwnd);
		else
                    MCIWndPlayReverse(hwnd);
		MCIWndiTimerStuff(p);	// kick ourselves to update mode
	    }

	    // Set the trackbar to the (possibly) new position
	    SendMessage(p->hwndTrackbar, TBM_SETPOS, TRUE, dwPos);
            break;

        case WM_MENUSELECT:
            break;

	// Sent from a toolbar button being pressed
        case WM_COMMAND:
            return MCIWndCommands (p, hwnd, wParam, lParam);
            break;

// When somebody presses a toolbar button in 32 bit Chicago-land, we are told
// about it via a WM_NOTIFY.  Else, we are sent a WM_COMMAND, handled elsewhere.
#ifdef _WIN32
       case WM_NOTIFY: {

#define lpHdr ((TBNOTIFY *)lParam)

           if (lpHdr->hdr.code == TBN_BEGINDRAG) {

               RECT rc;
               RECT rcT;
               MSG  msgT;


	   // We're only interested in a pressing of the Menu button
               if (lpHdr->hdr.idFrom != ID_TOOLBAR ||
	       lpHdr->iItem != IDM_MENU ||
               !SendMessage(p->hwndToolbar, TB_ISBUTTONENABLED,
                       IDM_MENU, 0) ||
               !p->hmenu)
           break;

           SendMessage(p->hwndToolbar, TB_GETITEMRECT,
               (int)SendMessage(p->hwndToolbar, TB_COMMANDTOINDEX,
                       IDM_MENU, 0),
               (LPARAM)(LPVOID)&rc);
           rcT = rc;
           ClientToScreen(p->hwndToolbar, (LPPOINT)&rc);
           ClientToScreen(p->hwndToolbar, (LPPOINT)&rc + 1);

           // Push the button down (accelerator won't have done this)
           SendMessage(p->hwndToolbar, TB_PRESSBUTTON, IDM_MENU,
               TRUE);

           // Don't allow error dlgs to come up while we're tracking.
           // That would cause windows to shatter and send shrapnel
           // flying.
           p->fTracking = TRUE;
           TrackPopupMenu(p->hmenu, 0, rc.left, rc.bottom - 1, 0,
                       hwnd, &rc);  // don't dismiss menu inside button
           p->fTracking = FALSE;

           // Bring the button back up.
           SendMessage(p->hwndToolbar, TB_PRESSBUTTON, IDM_MENU,
               FALSE);

           // What if we press the menu button to make the menu go
           // away?  It's just going to bring the menu back up again!
           // So we need to pull the click out of the queue.
           // There are bugs in the toolbar code to prevent me from
           // doing this any other way (like disabling the button)
           if (PeekMessage(&msgT, p->hwndToolbar, WM_LBUTTONDOWN,
                               WM_LBUTTONDOWN, PM_NOREMOVE)) {
               POINT pt = {MAKEPOINTS(msgT.lParam).x,
                           MAKEPOINTS(msgT.lParam).y };
               if (PtInRect(&rcT, pt))
                   PeekMessage(&msgT, p->hwndToolbar, WM_LBUTTONDOWN,
                               WM_LBUTTONDOWN, PM_REMOVE);
           }
           }
           else if (lpHdr->hdr.code == TTN_NEEDTEXT) {

                LPTOOLTIPTEXT   lpTt;
                extern HINSTANCE ghInst;	// in video\init.c

                lpTt = (LPTOOLTIPTEXT)lParam;
                LoadString( ghInst, (UINT) lpTt->hdr.idFrom,
                            lpTt->szText, sizeof(lpTt->szText) );
                return 0;
           }
           break;
       }
#endif

        case WM_DESTROY:
	    // !!! MMP CLOSE will be deferred till AFTER the DESTROY

	    // Don't palette kick when we're going down.  Not necessary and VB
	    // is stupid and rips.
	    p->fHasPalette = FALSE;
            MCIWndiClose(p, FALSE);  //don't leave us playing into a random DC
	    break;

	// We can't free our pointer until now, because WM_NCDESTROY is the
	// guaranteed last message we'll get.  If we do it sooner, we could
	// fault trying to use it.
	case WM_NCDESTROY:

	    if (p->hmenu) {
                DestroyMenu(p->hmenu);
		if (p->hbrDither) {
		    DeleteObject(p->hbrDither);
		    p->hbrDither = NULL;
		}
	    }

 	    if (p->pTrackStart)
		LocalFree((HANDLE)p->pTrackStart);

	    if (p->hfont) {
		// !!! Someone else may have to go and create it again, but oh
		// !!! well.
		DeleteObject(p->hfont);
		p->hfont = NULL;
	    }

	    if (p->hicon) {
		DestroyIcon(p->hicon);
		p->hicon = NULL;
	    }
	
//	    if (p->hbmToolbar) {
//		DeleteObject(p->hbmToolbar);
//		p->hbmToolbar = NULL;
//	    }

	    // We can't destroy our pointer and then fall through and use it
	    f = p->fMdiWindow;
	    LocalFree((HLOCAL) p);
            SetWindowLong(hwnd, 0, 0);       // our p
	    if (f)
		return DefMDIChildProc(hwnd, msg, wParam, lParam);
	    else
		return DefWindowProc(hwnd, msg, wParam, lParam);

	// Use a different rate for the timer depending on if we're active
	// or not.
        case WM_NCACTIVATE:
	    // MDI windows need to realize their palette here
	    if (p->wDeviceID && p->fMdiWindow && p->fHasPalette)
		MCIWndRealize(hwnd, wParam == FALSE);
#if 0
	case WM_ACTIVATE:
	    p->fActive = wParam;
	    KillTimer(hwnd, TIMER1);
	    MCIWndiSetTimer(p);
#endif
	    break;
	
	case WM_SETFOCUS:
	    p->fActive = TRUE;
	    KillTimer(hwnd, TIMER1);
	    MCIWndiSetTimer(p);
	    break;

	case WM_KILLFOCUS:
	    p->fActive = FALSE;
	    KillTimer(hwnd, TIMER1);
	    MCIWndiSetTimer(p);
	    break;

	// If the user uses MCINOTIFY we pass the notify on to the "owner"
	case MM_MCINOTIFY:
	    // Kick ourselves to update toolbar/titles since getting a notify
	    // means that stuff might have changed.
	    MCIWndiTimerStuff(p);
	    return NotifyOwner(p, msg, wParam, lParam);
	
        case WM_DRAWITEM:
        case WM_MEASUREITEM:
        case WM_DELETEITEM:
            OwnerDraw(p, msg, wParam, lParam);
            return TRUE;        // !!!

        case WM_SYSCOMMAND:
            switch (wParam & ~0xF) {
                case SC_MINIMIZE:
		    // Minimizing from MAXIMIZED state better do the same thing
		    // as restore or windows will always think it's maximized
		    // and start wierding out on us (Chico bug 19541).
		    if (IsZoomed(hwnd)) {
			wParam = SC_RESTORE | (wParam & 0xF);
			break;	// MUST let DefWndProc run
		    }
                    if (p->wDeviceID && p->fCanWindow) {
                        RECT rc;
                        MCIWndGetDest(hwnd, &rc);
                        if (rc.right  > p->rcNormal.right &&
                            rc.bottom > p->rcNormal.bottom) {

			    // We pressed buttons on the title bar... we really
			    // better autosize window.
			    DWORD dw = p->dwStyle;
			    p->dwStyle &= ~MCIWNDF_NOAUTOSIZEWINDOW;
                            MCIWndSetZoom(hwnd, 100);
			    p->dwStyle = dw;
                            return 0;
                        }
                    }
                    break;

                case SC_MAXIMIZE:
                    if (p->fCanWindow && !IsIconic(hwnd)) {
                        RECT rc;
                        MCIWndGetDest(hwnd, &rc);
                        if (rc.right  < p->rcNormal.right &&
                            rc.bottom < p->rcNormal.bottom) {

			    // We pressed buttons on the title bar... we really
			    // better autosize window.
			    DWORD dw = p->dwStyle;
			    p->dwStyle &= ~MCIWNDF_NOAUTOSIZEWINDOW;
                            MCIWndSetZoom(hwnd, 100);
			    p->dwStyle = dw;
                            return 0;
                        }
                        if (rc.right  >= p->rcNormal.right &&
                            rc.right  <  p->rcNormal.right*2 &&
                            rc.bottom >= p->rcNormal.bottom &&
                            rc.bottom <  p->rcNormal.bottom*2) {

			    // We pressed buttons on the title bar... we really
			    // better autosize window.
			    DWORD dw = p->dwStyle;
			    p->dwStyle &= ~MCIWNDF_NOAUTOSIZEWINDOW;
                            MCIWndSetZoom(hwnd, 200);
			    p->dwStyle = dw;
                            return 0;
                        }
                    }
                    break;
            }
            break;

	case WM_DROPFILES:
	    MCIWndiDrop(hwnd, wParam);
	    break;

	case WM_QUERYDRAGICON:
	    return (LONG_PTR)(UINT_PTR)p->hicon;
    }

    if (p && p->fMdiWindow)
        return DefMDIChildProc(hwnd, msg, wParam, lParam);
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);
}

STATICFN void NEAR PASCAL PatRect(HDC hdc,int x,int y,int dx,int dy)
{
    RECT    rc;

    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
}

#define FillRC(hdc, prc)    PatRect(hdc, (prc)->left, (prc)->top, (prc)->right - (prc)->left, (prc)->bottom-(prc)->top)

STATICFN HBITMAP NEAR PASCAL CreateDitherBitmap(void)
{
    PBITMAPINFO pbmi;
    HBITMAP hbm;
    HDC hdc;
    int i;
    long patGray[8];
    DWORD rgb;

    pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) +
		(sizeof(RGBQUAD) * 16));
    if (!pbmi)
        return NULL;

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = 8;
    pbmi->bmiHeader.biHeight = 8;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = 1;
    pbmi->bmiHeader.biCompression = BI_RGB;

    rgb = GetSysColor(COLOR_BTNFACE);
    pbmi->bmiColors[0].rgbBlue  = GetBValue(rgb);
    pbmi->bmiColors[0].rgbGreen = GetGValue(rgb);
    pbmi->bmiColors[0].rgbRed   = GetRValue(rgb);
    pbmi->bmiColors[0].rgbReserved = 0;

    rgb = GetSysColor(COLOR_BTNHIGHLIGHT);
    pbmi->bmiColors[1].rgbBlue  = GetBValue(rgb);
    pbmi->bmiColors[1].rgbGreen = GetGValue(rgb);
    pbmi->bmiColors[1].rgbRed   = GetRValue(rgb);
    pbmi->bmiColors[1].rgbReserved = 0;


    /* initialize the brushes */

    for (i = 0; i < 8; i++)
       if (i & 1)
           patGray[i] = 0xAAAA5555L;   //  0x11114444L; // lighter gray
       else
           patGray[i] = 0x5555AAAAL;   //  0x11114444L; // lighter gray

    hdc = GetDC(NULL);

    hbm = CreateDIBitmap(hdc, &pbmi->bmiHeader, CBM_INIT, patGray, pbmi,
		DIB_RGB_COLORS);

    ReleaseDC(NULL, hdc);

    LocalFree((HANDLE)pbmi);

    return hbm;
}

STATICFN HBRUSH NEAR PASCAL CreateDitherBrush(void)
{
    HBITMAP hbmGray;
    HBRUSH  hbrDither;

    hbmGray = CreateDitherBitmap();
    if (hbmGray) {
	hbrDither = CreatePatternBrush(hbmGray);
	DeleteObject(hbmGray);
	return hbrDither;
    }
    return NULL;
}

//
// Draw the channel for the volume and speed menu controls
//
STATICFN void NEAR PASCAL DrawChannel(HDC hdc, LPRECT prc, HBRUSH hbrDither)
{
    HBRUSH hbrTemp;

    int iWidth = prc->right - prc->left;

    // draw the frame around the window
    SetBkColor(hdc, GetSysColor(COLOR_WINDOWFRAME));

    PatRect(hdc, prc->left, prc->top,      iWidth, 1);
    PatRect(hdc, prc->left, prc->bottom-2, iWidth, 1);
    PatRect(hdc, prc->left, prc->top,      1, prc->bottom-prc->top-1);
    PatRect(hdc, prc->right-1, prc->top, 1, prc->bottom-prc->top-1);

    SetBkColor(hdc, GetSysColor(COLOR_BTNHIGHLIGHT));
    PatRect(hdc, prc->left, prc->bottom-1, iWidth, 1);

    SetBkColor(hdc, GetSysColor(COLOR_BTNSHADOW));
    PatRect(hdc, prc->left+1, prc->top + 1, iWidth-2,1);

    // draw the background in dither gray
    hbrTemp = SelectObject(hdc, hbrDither);

    if (hbrTemp) {
        PatBlt(hdc, prc->left+1, prc->top + 2,
            iWidth-2, prc->bottom-prc->top-4, PATCOPY);
        SelectObject(hdc, hbrTemp);
    }
}

STATICFN LRESULT OwnerDraw(PMCIWND p, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RECT        rc, rcMenu, rcChannel, rcThumb;
    HDC         hdc;
    int         i,dx,dy,len;
    SIZE        size;
    TCHAR       ach[10];
    HWND        hwnd = p->hwnd;

    #define lpMIS  ((LPMEASUREITEMSTRUCT)lParam)
    #define lpDIS  ((LPDRAWITEMSTRUCT)lParam)

    #define WIDTH_FROM_THIN_AIR 14
    #define CHANNEL_INDENT	6	// for VOLUME and SPEED menu trackbar
    #define MENU_WIDTH          10
    #define THUMB               5
    #define MENU_ITEM_HEIGHT	2

    switch (msg)
    {
        case WM_MEASUREITEM:

            if (p->hfont == NULL)
                p->hfont = CreateFont (8, 0, 0, 0,
		                FW_NORMAL,FALSE,FALSE,FALSE,
		                ANSI_CHARSET,OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS,PROOF_QUALITY,
		                VARIABLE_PITCH | FF_DONTCARE,
                                szSmallFonts);

	    //
	    // The first and last menu items are the spaces above and below
	    // the channel, so they need to be taller.
	    //
	    if (lpMIS->itemID == IDM_MCIVOLUME + VOLUME_MAX + 1
		|| lpMIS->itemID == IDM_MCISPEED + SPEED_MAX + 1
	    	|| lpMIS->itemID == IDM_MCIVOLUME + VOLUME_MAX + 2
		|| lpMIS->itemID == IDM_MCISPEED + SPEED_MAX + 2) {

                lpMIS->itemHeight = CHANNEL_INDENT;
                lpMIS->itemWidth  = MENU_WIDTH;
	    } else {
                lpMIS->itemHeight = MENU_ITEM_HEIGHT;
                lpMIS->itemWidth  = MENU_WIDTH;
	    }

	    return TRUE;

        case WM_DRAWITEM:
            rc  = lpDIS->rcItem;
            hdc = lpDIS->hDC;

	    //
	    // Something has been deselected.  If we don't see a new selection
	    // soon, it means we've dragged the cursor off the menu, and we
	    // should pop the thumb back to its original spot.
	    //
	    if ((lpDIS->itemAction & ODA_SELECT) &&
				!(lpDIS->itemState & ODS_SELECTED))
		SetTimer(p->hwnd, TIMER2, 500, NULL);
		
            //
	    // When asked to draw the selected or checked menu item, we will
	    // draw the entire menu.  Otherwise, we don't do a thing
            //
	    if (lpDIS->itemState & (ODS_SELECTED | ODS_CHECKED)) {

		// This is the item that is checked, or the original spot for
		// the thumb.  Remember it so when we drag off the menu, we
		// can bounce the thumb back here.
		if (lpDIS->itemState & ODS_CHECKED) {
		    p->uiHack = lpDIS->itemID;
	            if (p->uiHack >= IDM_MCISPEED &&
	            		p->uiHack <= IDM_MCISPEED + SPEED_MAX)
			p->hmenuHack = p->hmenuSpeed;
		    else
			p->hmenuHack = p->hmenuVolume;
		}

		// Something is being selected.  Obviously the mouse is still
		// on the menu.  Scrap our timer that was waiting to see if
		// we've dragged off the menu.
		if (lpDIS->itemState & ODS_SELECTED)
		    KillTimer(p->hwnd, TIMER2);

		// !!! Hack from Hell !!!
	        // If we try to highlight the bogus menu items, bail!
	        if (lpDIS->itemID == IDM_MCIVOLUME + VOLUME_MAX + 1)
		    break;
	        if (lpDIS->itemID == IDM_MCIVOLUME + VOLUME_MAX + 2)
		    break;
	        if (lpDIS->itemID == IDM_MCISPEED + SPEED_MAX + 1)
		    break;
	        if (lpDIS->itemID == IDM_MCISPEED + SPEED_MAX + 2)
		    break;

		// Actually set the parameter to the value we're dragging so
		// we can hear it change as we move the slider.
		// 42 means DON'T CHECK it (remember which item was originally
		// checked).
		SendMessage(hwnd, WM_COMMAND, lpDIS->itemID, 42);

		//
		// Get the rect of our menu window.  GetClipBox is
		// not quite right, so we'll adjust for the border.  Our lpDIS
		// contains the proper width of the client area, so we'll use
		// that.
		//

                GetClipBox(hdc, &rc);
                rc.top++;	//!!! top border width
                rc.bottom -= 2;	//!!! bottom border width
                rc.left = lpDIS->rcItem.left;
                rc.right = lpDIS->rcItem.right;
	 	rcMenu = rc;	// This is the rect of the whole menu

		// !!!
		// Deflate the rect to the area we want the channel to be
		// drawn in.  Use HACKY constants.
		// !!!
                i = (rc.right - rc.left - WIDTH_FROM_THIN_AIR) / 2;
                rc.top    += CHANNEL_INDENT;
                rc.bottom -= CHANNEL_INDENT;
                rc.left   += i;
                rc.right  -= i;
		rcChannel = rc;	// This is the rect of the channel

		//
		// See where the thumb belongs
		//
                rc = lpDIS->rcItem;
		rc.bottom = rc.top + 2;		// Ouch! Make sure size is 2
		
		//
		// Don't draw the thumb higher than the top of the channel
		//
		if (rc.top < rcChannel.top) {
		    rc.top = rcChannel.top;
		    rc.bottom = rc.top + 2;	// itemHeight
		}

		//
		// Don't draw the thumb below the bottom of the channel
		//
		if (rc.top > rcChannel.bottom - 2) {	// where border is
		    rc.top = rcChannel.bottom - 2;
		    rc.bottom = rc.top + 2;
		}

		//
		// Munge the rect in a bit and draw the thumb there
		//
                rc.left  += 2;
                rc.right -= 2;
                rc.bottom+= THUMB;
                rc.top   -= THUMB;

#if 0
		// Make the thumb a little bigger on the checked value
	        if (lpDIS->itemState & ODS_CHECKED) {
		    rc.top -= 1;
		    rc.bottom += 1;
		}
#endif

		rcThumb = rc;	// This is the rect of the thumb

                dx = rc.right  - rc.left;
                dy = rc.bottom - rc.top;

                SetBkColor(hdc, GetSysColor(COLOR_WINDOWFRAME));
                PatRect(hdc, rc.left+1, rc.top, dx-2,1        );
                PatRect(hdc, rc.left+1, rc.bottom-1,dx-2,1    );
                PatRect(hdc, rc.left, rc.top+1, 1,dy-2        );
                PatRect(hdc, rc.right-1,  rc.top+1, 1,dy-2    );

                InflateRect(&rc,-1,-1);
                dx = rc.right  - rc.left;
                dy = rc.bottom - rc.top;

                SetBkColor(hdc, GetSysColor(COLOR_BTNHIGHLIGHT));
                PatRect(hdc, rc.left,   rc.top,   1,dy);
                PatRect(hdc, rc.left,   rc.top,   dx,1);

                SetBkColor(hdc, GetSysColor(COLOR_BTNSHADOW));
                PatRect(hdc, rc.right-1,rc.top+1, 1,dy-1);
                PatRect(hdc, rc.left+1, rc.bottom-1, dx-1,1);

                InflateRect(&rc,-1,-1);

                SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
                SelectObject(hdc, p->hfont);
                len = wsprintf(ach, TEXT("%d"), lpMIS->itemID % 1000);

                GetTextExtentPoint(hdc, ach, len, &size);
                ExtTextOut(hdc,
                    (rc.right  + rc.left - size.cx)/2,
                    (rc.bottom + rc.top - size.cy)/2,
                    ETO_OPAQUE,&rc,ach,len,NULL);

		//
		// Exclude the ClipRect that all that garbage drew into
		//
                ExcludeClipRect(hdc, rcThumb.left, rcThumb.top,
                        rcThumb.right, rcThumb.bottom);

		//
		// Next, draw the channel
                //
                DrawChannel(hdc, &rcChannel, p->hbrDither);
		ExcludeClipRect(hdc, rcChannel.left, rcChannel.top,
                        rcChannel.right, rcChannel.bottom);

		//
		// Lastly, fill the entire menu rect with the menu colour
		//
                SetBkColor(hdc, GetSysColor(COLOR_MENU));
                FillRC(hdc, &rcMenu);
            }

            return TRUE;

	case WM_DELETEITEM:
	    return TRUE;
    }
    return TRUE;
}


//
// Code to implement the MCI command dialog box
//

void PositionWindowNearParent(HWND hwnd)
{
    RECT    rc;
    RECT    rcParent;

    GetWindowRect(hwnd, &rc);
    rc.bottom -= rc.top;
    rc.right -= rc.left;
    GetWindowRect(GetParent(hwnd), &rcParent);

    if (rcParent.bottom + rc.bottom <
				GetSystemMetrics(SM_CYSCREEN)) {
	SetWindowPos(hwnd, NULL,
		     min(rc.left, GetSystemMetrics(SM_CXSCREEN) - rc.right),
		     rcParent.bottom,
		     0, 0,
		     SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    } else if (rc.bottom < rcParent.top) {
	SetWindowPos(hwnd, NULL,
		     min(rc.left, GetSystemMetrics(SM_CXSCREEN) - rc.right),
		     rcParent.top - rc.bottom,
		     0, 0,
		     SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    }
}

/*--------------------------------------------------------------+
| mciDialog - bring up the dialog for MCI Send Command          |
|                                                               |
+--------------------------------------------------------------*/
BOOL FAR PASCAL _loadds mciDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TCHAR   ach[255];
    TCHAR   achT[40];
    UINT    w;
    DWORD   dw;
    PMCIWND p;
    HWND    hwndP;

    switch (msg)
    {
        case WM_INITDIALOG:
	    // Remember our actually true parent
	    SetWindowLongPtr(hwnd, DWLP_USER, lParam);
	    PositionWindowNearParent(hwnd);
            return TRUE;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam,lParam))
            {
                case IDOK:
                    Edit_SetSel (GetDlgItem (hwnd, IDC_MCICOMMAND), 0, -1);
                    w = GetDlgItemText(hwnd, IDC_MCICOMMAND, ach, NUMELMS(ach));

		    hwndP = (HWND)GetWindowLongPtr(hwnd, DWLP_USER);
		    p = (PMCIWND)(UINT)GetWindowLong(hwndP, 0);

		    // special case the CLOSE command to do our clean up
	    	    lstrcpyn(achT, (LPTSTR)ach, lstrlen(szClose) + 1);
		    if (lstrcmpi((LPTSTR)achT, szClose) == 0) {
			MCIWndClose(hwndP);
			break;
		    }

                    dw = MCIWndGet(p, ach, ach, NUMELMS(ach));

                    if (dw != 0)
                        mciGetErrorString(dw, ach, NUMELMS(ach));

                    SetDlgItemText(hwnd, IDC_RESULT, ach);

	    	    // kick ourselves in case mode changed from this command
	    	    MCIWndiTimerStuff(p);
                    break;

                case IDCANCEL:
                    EndDialog(hwnd, FALSE);
                    break;
            }
            break;
    }

    return FALSE;
}

STATICFN BOOL NEAR PASCAL mciDialog(HWND hwnd)
{
    DialogBoxParam(hInst, MAKEINTATOM(DLG_MCICOMMAND), hwnd,
                (DLGPROC)mciDlgProc, (LPARAM)hwnd);
    return TRUE;
}


//
// Code to implement the Copy command:
//
//
// MCIWnd tries to copy the same things to the clipboard that VfW MPlayer
// would have.
//

#define SLASH(c)     ((c) == TEXT('/') || (c) == TEXT('\\'))
/**************************************************************************

    convert a file name to a fully qualifed path name, if the file
    exists on a net drive the UNC name is returned.

***************************************************************************/

STATICFN BOOL NetParseFile(LPTSTR szFile, LPTSTR szPath)
{
    TCHAR       achDrive[4];
    TCHAR       achRemote[128];
    int         cbRemote = NUMELMS(achRemote);

    // Dynamically link to WNetGetConnection.  That way MPR.DLL will only
    // get pulled in if it is needed.
#ifdef UNICODE
    typedef DWORD (APIENTRY *LPWNETGETCONNECTION)(
        LPCWSTR lpLocalName,
        LPWSTR lpRemoteName,
        LPDWORD lpnLength
        );
    char  szWNetGetConnection[] = "WNetGetConnectionW";
#else
    typedef DWORD (APIENTRY *LPWNETGETCONNECTION)(
        LPCSTR lpLocalName,
        LPSTR lpRemoteName,
        LPDWORD lpnLength
        );
    char  szWNetGetConnection[] = "WNetGetConnectionA";
#endif

    TCHAR szMpr[] = TEXT("MPR.DLL");
    LPWNETGETCONNECTION lpFn;
    HINSTANCE hInst;


    if (szPath == NULL)
        szPath = szFile;
    else
        szPath[0] = 0;

    //
    // Fully qualify the file name
    //
#ifdef _WIN32
    {
        LPTSTR pfile;
        TCHAR  achPath[MAX_PATH];

        achPath[0] = 0;
        if (GetFullPathName(szFile, MAX_PATH, achPath, &pfile) == 0) {
             return FALSE;
	}
        lstrcpy( szPath, achPath );
    }
#else
    {
        OFSTRUCT of;

        if (OpenFile(szFile, &of, OF_PARSE) == -1)
            return FALSE;

        lstrcpy(szPath, of.szPathName);
    }
#endif

    //
    // if the file is not drive based (probably UNC)
    //
    if (szPath[1] != TEXT(':'))
        return TRUE;

    achDrive[0] = szPath[0];
    achDrive[1] = TEXT(':');
    achDrive[2] = TEXT('\0');

    hInst = LoadLibrary(szMpr);
    if (hInst == NULL) {
        return FALSE;
    }
    *(FARPROC *)&lpFn = GetProcAddress( hInst, szWNetGetConnection);

    if ( (*lpFn)(achDrive, achRemote, &cbRemote) != WN_SUCCESS) {
        FreeLibrary(hInst);
        return FALSE;
    }

    FreeLibrary(hInst);

    if (!SLASH(achRemote[0]) || !SLASH(achRemote[1]))
	return TRUE;

    lstrcat(achRemote, szPath+2);
    lstrcpy(szPath, achRemote);

    return TRUE;
}



SZCODE aszMPlayerName[]           = TEXT("MPlayer");
HANDLE GetMPlayerData(PMCIWND p)
{
    TCHAR       szFileName[MAX_PATH];
    TCHAR	ach[40];
    TCHAR       szDevice[40];
    HANDLE      h;
    LPSTR       psz;
    int         len;
    LPTSTR      lpszCaption = szFileName;
    UINT	wOptions;
    RECT	rc;
    BOOL	fCompound, fFile;
    DWORD	dw;
    MCI_GETDEVCAPS_PARMS    mciDevCaps; /* for the MCI_GETDEVCAPS command */

    //
    // Get the Device Name
    //
    MCIWndGet(p, TEXT("sysinfo installname"), szDevice, NUMELMS(szDevice));

    //
    // determine if the device is simple or compound
    //
    mciDevCaps.dwItem = MCI_GETDEVCAPS_COMPOUND_DEVICE;
    dw = mciSendCommand(p->wDeviceID, MCI_GETDEVCAPS,
        MCI_GETDEVCAPS_ITEM, (DWORD_PTR)(LPSTR)&mciDevCaps);
    fCompound = (dw == 0 && mciDevCaps.dwReturn != 0);

    //
    // determine if the device handles files
    //
    if (fCompound) {
        mciDevCaps.dwItem = MCI_GETDEVCAPS_USES_FILES;
        dw = mciSendCommand(p->wDeviceID, MCI_GETDEVCAPS,
            MCI_GETDEVCAPS_ITEM, (DWORD_PTR)(LPSTR)&mciDevCaps);
        fFile = (dw == 0 && mciDevCaps.dwReturn != 0);
    }

    //
    // Compound devices that support files have an associated filename
    //
    if (fCompound && fFile) {
        lstrcpy(szFileName, p->achFileName);

        //
        // Sometimes the filename is really "device!filename" so we have to peel
        // the real filename out of it
        //
        lstrcpyn(ach, szFileName, lstrlen(szDevice) + 1);
        if ((lstrcmpi(szDevice, ach) == 0) &&
                        (szFileName[lstrlen(szDevice)] == TEXT('!'))) {
            lstrcpy(szFileName, &(p->achFileName[lstrlen(szDevice) + 1]));
        }

        NetParseFile(szFileName, (LPTSTR)NULL);
#ifndef _WIN32
        OemToAnsi(szFileName,szFileName);       // Map extended chars.
#endif
    } else {
        szFileName[0] = TEXT('\0');
    }

#ifdef DEBUG
    DPF("  GetLink: %ls|%ls!%ls\n",
        (LPTSTR)aszMPlayerName,
        (LPTSTR)szFileName,
        (LPTSTR)szDevice);
#endif

    /* How much data will we be writing? */
    len = 9 +                    // all the delimeters
          lstrlen(aszMPlayerName) +
          lstrlen(szFileName) +
          lstrlen(szDevice) +
          5 + 10 + 10 + 10 +     // max length of int and long strings
          lstrlen(lpszCaption);

    h = GlobalAlloc(GMEM_DDESHARE|GMEM_ZEROINIT, len);
    if (!h)
        return NULL;
    psz = GlobalLock(h);

    wOptions = 0x0030; // !!!! OPT_PLAY|OPT_BAR

    switch (MCIWndStatus(p, MCI_STATUS_TIME_FORMAT, 0)) {
	case MCI_FORMAT_FRAMES:
	    wOptions |= 1;	// frame mode
	    break;
	
	case MCI_FORMAT_MILLISECONDS:
	    wOptions |= 2;	// time mode
	    break;
    }
	
    MCIWndRect(p, &rc, FALSE);

    wsprintfA(psz,
#ifdef UNICODE
              "%ls%c%ls%c%ls%c%d%c%d%c%d%c%d%c%d%c%ls%c",
#else
              "%s%c%s%c%s%c%d%c%ld%c%ld%c%ld%c%d%c%s%c",
#endif
              (LPTSTR)aszMPlayerName, '\0',
              (LPTSTR)szFileName, '\0',
              (LPTSTR)szDevice, ',',
              wOptions, ',',
              0L, ',', // !!! sel start
              0L, ',', // !!! sel length
              p->dwPos, ';',
              rc.bottom - rc.top, ',',
              lpszCaption, '\0');

    return h;
}

HBITMAP FAR PASCAL BitmapMCI(PMCIWND p)
{
    HDC         hdc, hdcMem;
    HBITMAP     hbm, hbmT;
    HBRUSH      hbrOld;
    DWORD       dw;
    RECT        rc;
    HBRUSH hbrWindowColour;

    /* Minimum size of bitmap is icon size */
    int ICON_MINX = GetSystemMetrics(SM_CXICON);
    int ICON_MINY = GetSystemMetrics(SM_CYICON);

    /* Get size of a frame or an icon that we'll be drawing */
    MCIWndRect(p, &rc, FALSE);

    SetRect(&rc, 0, 0,
	    max(ICON_MINX, rc.right - rc.left),
	    max(ICON_MINX, rc.bottom - rc.top));

    hdc = GetDC(NULL);
    if (hdc == NULL)
        return NULL;
    hdcMem = CreateCompatibleDC(NULL);
    if (hdcMem == NULL) {
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    /* Big enough to hold text caption too, if necessary */
    hbm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    ReleaseDC(NULL, hdc);
    if (hbm == NULL) {
        DeleteDC(hdcMem);
        return NULL;
    }

    hbmT = SelectObject(hdcMem, hbm);

    hbrWindowColour     = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    hbrOld = SelectObject(hdcMem, hbrWindowColour);
    PatBlt(hdcMem, 0,0, rc.right, rc.bottom, PATCOPY);
    SelectObject(hdcMem, hbrOld);
    DeleteObject(hbrWindowColour);

    if (p->wDeviceID && p->fCanWindow)
    {
	MCI_ANIM_UPDATE_PARMS mciUpdate;

	mciUpdate.hDC = hdcMem;

	dw = mciSendCommand(p->wDeviceID, MCI_UPDATE,
		    MCI_ANIM_UPDATE_HDC | MCI_WAIT,
		    (DWORD_PTR)(LPVOID)&mciUpdate);
    } else {
	DrawIcon(hdcMem, rc.left, rc.top, p->hicon);
    }

    if (hbmT)
        SelectObject(hdcMem, hbmT);
    DeleteDC(hdcMem);

    return hbm;
}

HPALETTE CopyPalette(HPALETTE hpal)
{
    PLOGPALETTE ppal;
    int         nNumEntries = 0;
    int         i;

    if (!hpal)
        return NULL;

    GetObject(hpal,sizeof(int),(LPVOID)&nNumEntries);

    if (nNumEntries == 0)
        return NULL;

    ppal = (PLOGPALETTE)LocalAlloc(LPTR,sizeof(LOGPALETTE) +
                nNumEntries * sizeof(PALETTEENTRY));

    if (!ppal)
        return NULL;

    ppal->palVersion    = 0x300;
    ppal->palNumEntries = (WORD) nNumEntries;

    GetPaletteEntries(hpal,0,nNumEntries,ppal->palPalEntry);

    for (i=0; i<nNumEntries; i++)
        ppal->palPalEntry[i].peFlags = 0;

    hpal = CreatePalette(ppal);

    LocalFree((HANDLE)ppal);
    return hpal;
}

HANDLE FAR PASCAL PictureFromDib(HANDLE hdib, HPALETTE hpal)
{
    LPMETAFILEPICT      pmfp;
    HANDLE              hmfp;
    HANDLE              hmf;
    HANDLE              hdc;
    LPBITMAPINFOHEADER  lpbi;

    if (!hdib)
        return NULL;

    lpbi = (LPVOID)GlobalLock(hdib);
    if (lpbi->biClrUsed == 0 && lpbi->biBitCount <= 8)
        lpbi->biClrUsed = 1 << lpbi->biBitCount;

    hdc = CreateMetaFile(NULL);
    if (!hdc)
        return NULL;

    SetWindowOrgEx(hdc, 0, 0, NULL);
    SetWindowExtEx(hdc, (int)lpbi->biWidth, (int)lpbi->biHeight, NULL);

    if (hpal)
    {
        SelectPalette(hdc,hpal,FALSE);
        RealizePalette(hdc);
    }

    SetStretchBltMode(hdc, COLORONCOLOR);

    StretchDIBits(hdc,
        0,0,(int)lpbi->biWidth, (int)lpbi->biHeight,
        0,0,(int)lpbi->biWidth, (int)lpbi->biHeight,
        (LPBYTE)lpbi + (int)lpbi->biSize + (int)lpbi->biClrUsed * sizeof(RGBQUAD),
        (LPBITMAPINFO)lpbi,
        DIB_RGB_COLORS,
        SRCCOPY);

    if (hpal)
        SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), FALSE);

    hmf = CloseMetaFile(hdc);

    if (hmfp = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, sizeof(METAFILEPICT)))
    {
        pmfp = (LPMETAFILEPICT)GlobalLock(hmfp);

        hdc = GetDC(NULL);
#if 1
        pmfp->mm   = MM_ANISOTROPIC;
        pmfp->hMF  = hmf;
        pmfp->xExt = MulDiv((int)lpbi->biWidth ,2540,GetDeviceCaps(hdc, LOGPIXELSX));
        pmfp->yExt = MulDiv((int)lpbi->biHeight,2540,GetDeviceCaps(hdc, LOGPIXELSX));
#else
        pmfp->mm   = MM_TEXT;
        pmfp->hMF  = hmf;
        pmfp->xExt = (int)lpbi->biWidth;
        pmfp->yExt = (int)lpbi->biHeight;
#endif
        ReleaseDC(NULL, hdc);
    }
    else
    {
        DeleteMetaFile(hmf);
    }

    return hmfp;
}

#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */

/*
 *  DibFromBitmap()
 *
 *  Will create a global memory block in DIB format that represents the DDB
 *  passed in
 *
 */
HANDLE FAR PASCAL DibFromBitmap(HBITMAP hbm, HPALETTE hpal)
{
    BITMAP               bm;
    BITMAPINFOHEADER     bi;
    BITMAPINFOHEADER FAR *lpbi;
    DWORD                dw;
    HANDLE               hdib;
    HDC                  hdc;
    HPALETTE             hpalT;

    if (!hbm)
        return NULL;

    GetObject(hbm,sizeof(bm),(LPVOID)&bm);

    bi.biSize               = sizeof(BITMAPINFOHEADER);
    bi.biWidth              = bm.bmWidth;
    bi.biHeight             = bm.bmHeight;
    bi.biPlanes             = 1;
    bi.biBitCount           = (bm.bmPlanes * bm.bmBitsPixel) > 8 ? 24 : 8;
    bi.biCompression        = BI_RGB;
    bi.biSizeImage          = (DWORD)WIDTHBYTES(bi.biWidth * bi.biBitCount) * bi.biHeight;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = bi.biBitCount == 8 ? 256 : 0;
    bi.biClrImportant       = 0;

    dw  = bi.biSize + bi.biClrUsed * sizeof(RGBQUAD) + bi.biSizeImage;

    hdib = GlobalAlloc(GHND | GMEM_DDESHARE, dw);

    if (!hdib)
        return NULL;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib);
    *lpbi = bi;

    hdc = CreateCompatibleDC(NULL);

    if (hpal)
    {
        hpalT = SelectPalette(hdc,hpal,FALSE);
        RealizePalette(hdc);
    }

    GetDIBits(hdc, hbm, 0, (UINT)bi.biHeight,
        (LPBYTE)lpbi + (int)lpbi->biSize + (int)lpbi->biClrUsed * sizeof(RGBQUAD),
        (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

    if (hpal)
        SelectPalette(hdc,hpalT,FALSE);

    DeleteDC(hdc);

    return hdib;
}

SZCODE aszNative[]            = TEXT("Native");
SZCODE aszOwnerLink[]         = TEXT("OwnerLink");

// Pretend to be MPlayer copying to the clipboard
STATICFN void NEAR PASCAL MCIWndCopy(PMCIWND p)
{
    UINT	cfNative;
    UINT	cfOwnerLink;
    HBITMAP	hbm;
    HPALETTE	hpal;
    HANDLE	hdib;
    HANDLE	hmfp;

    cfNative    = RegisterClipboardFormat(aszNative);
    cfOwnerLink = RegisterClipboardFormat(aszOwnerLink);

    if (p->wDeviceID) {
	OpenClipboard(p->hwnd);
	
        EmptyClipboard();

        SetClipboardData(cfNative, GetMPlayerData(p));
        SetClipboardData(cfOwnerLink, GetMPlayerData(p));

	hbm  = BitmapMCI(p);
	hpal = MCIWndGetPalette(p->hwnd);
	hpal = CopyPalette(hpal);

	if (hbm) {
	    hdib = DibFromBitmap(hbm, hpal);

	    hmfp = PictureFromDib(hdib, hpal);

	    if (hmfp)
		SetClipboardData(CF_METAFILEPICT, hmfp);

	    if (hdib)
		SetClipboardData(CF_DIB, hdib);

	    DeleteObject(hbm);
	}

	if (hpal)
	    SetClipboardData(CF_PALETTE, hpal);

        CloseClipboard();
    }
}

/*****************************************************************************
 ****************************************************************************/

#ifdef DEBUG

STATICFN void cdecl dprintf(PSTR szFormat, ...)
{
    char ach[128];
    va_list va;

    static BOOL fDebug = -1;

    if (fDebug == -1) {
        fDebug = mmGetProfileIntA(szDebug, MODNAME, FALSE);
    }

    if (!fDebug)
        return;

    lstrcpyA(ach, MODNAME ": ");
    va_start(va,szFormat);
    wvsprintfA(ach+lstrlenA(ach),szFormat, va);
    va_end(va);
    lstrcatA(ach, "\r\n");

    OutputDebugStringA(ach);
}

#endif
