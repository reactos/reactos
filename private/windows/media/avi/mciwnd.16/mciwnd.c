
/*----------------------------------------------------------------------------*\
 *
 *  MCIWnd
 *
 *----------------------------------------------------------------------------*/

#include "mciwndi.h"

LONG FAR PASCAL _loadds MCIWndProc(HWND hwnd, unsigned msg, WORD wParam, LONG lParam);

static LONG OwnerDraw(PMCIWND p, UINT msg, WORD wParam, LONG lParam);
static BOOL NEAR PASCAL mciDialog(HWND hwnd);
static void NEAR PASCAL MCIWndCopy(PMCIWND p);

BOOL FAR _loadds MCIWndRegisterClass(void)
{
    WNDCLASS cls;

    // !!! We need to register a global class with the hinstance of the DLL
    // !!! because it's the DLL that has the code for the window class.
    // !!! Otherwise, the class goes away on us and things start to blow!
    // !!! HACK HACK HACK The hInstance is the current DS which is the high
    // !!! word of the address of all global variables --- sorry NT
#ifndef WIN32
    HINSTANCE hInstance = (HINSTANCE)HIWORD((LPVOID)&hInst); // random global
#else
    HINSTANCE hInstance = GetModuleHandle(NULL);
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

	extern BOOL FAR PASCAL InitToolbarClass(HINSTANCE hInstance);
	extern BOOL FAR PASCAL InitTrackBar(HINSTANCE hInstance);

        if (!InitToolbarClass(hInstance))
	    return FALSE;
        if (!InitTrackBar(hInstance))
	    return FALSE;

        // !!! Other one-time initialization

	return TRUE;
    }

    return FALSE;
}

HWND FAR _loadds MCIWndCreate(HWND hwndParent, HINSTANCE hInstance,
		      DWORD dwStyle, LPSTR szFile)
{
    HWND hwnd;
    int x,y,dx,dy;

#ifdef WIN32
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

    hwnd =
#ifdef BIDI
	CreateWindowEx(WS_EX_BIDI_SCROLL |  WS_EX_BIDI_MENU |WS_EX_BIDI_NOICON,
#else
	CreateWindow (
#endif
			aszMCIWndClassName, szNULL, dwStyle,
                        x, y, dx, dy,
			hwndParent,
			(HMENU)((dwStyle & WS_CHILD) ? 0x42 : NULL),
			hInstance, (LPVOID)szFile);

    return hwnd;
}

//
// Give a notification of something interesting to the proper authorites.
//
static LRESULT NotifyOwner(PMCIWND p, unsigned msg, WPARAM wParam, LPARAM lParam)
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
static void MCIWndiHandleError(PMCIWND p, DWORD dw)
{
    char	ach[128];

    // Set/Clear our error code
    p->dwError = dw;

    if (dw) {

	// We want to bring up a dialog on errors, so do so.
	// Don't bring up a dialog while we're moving the thumb around because
	// that'll REALLY confuse the mouse capture
	if (!(p->dwStyle & MCIWNDF_NOERRORDLG) && !p->fScrolling &&
							!p->fTracking) {
	    mciGetErrorString(p->dwError, ach, sizeof(ach));
	    MessageBox(p->hwnd, ach, LoadSz(IDS_MCIERROR),
#ifdef BIDI
		       MB_RTL_READING |
#endif
		       MB_ICONEXCLAMATION | MB_OK);
	}

	// The "owner" wants to know the error.  We tell him after we
	// bring up the dialog, because otherwise, our VBX never gets this
	// event.  (Wierd...)
	if (p->dwStyle & MCIWNDF_NOTIFYERROR) {
	    NotifyOwner(p, MCIWNDM_NOTIFYERROR, p->hwnd, p->dwError);
	}

    }
}

//
// Send an MCI GetDevCaps command and return whether or not it's supported
// This will not set our error code
//
static BOOL MCIWndDevCaps(PMCIWND p, DWORD item)
{
    MCI_GETDEVCAPS_PARMS   mciDevCaps;
    DWORD               dw;

    if (p->wDeviceID == NULL)
        return FALSE;

    mciDevCaps.dwItem = (DWORD)item;

    dw = mciSendCommand(p->wDeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM,
			(DWORD)(LPVOID)&mciDevCaps);

    if (dw == 0)
	return (BOOL)mciDevCaps.dwReturn;
    else
	return FALSE;
}

//
// Send an MCI Status command.
// This will not set our error code
//
static DWORD MCIWndStatus(PMCIWND p, DWORD item, DWORD err)
{
    MCI_STATUS_PARMS    mciStatus;
    DWORD               dw;

    if (p->wDeviceID == NULL)
	return err;

    mciStatus.dwItem = (DWORD)item;

    dw = mciSendCommand(p->wDeviceID, MCI_STATUS, MCI_STATUS_ITEM,
			(DWORD)(LPVOID)&mciStatus);

    if (dw == 0)
	return mciStatus.dwReturn;
    else
	return err;
}

//
// Send an MCI String command
// Optionally set our error code.  Never clears it.
//
static DWORD MCIWndString(PMCIWND p, BOOL fSetErr, LPSTR sz, ...)
{
    char    ach[256];
    int     i;
    DWORD   dw;

    if (p->wDeviceID == NULL)
	return 0;

    for (i=0; *sz && *sz != ' '; )
	ach[i++] = *sz++;

    i += wsprintf(&ach[i], " %d ", (UINT)p->alias);
    i += wvsprintf(&ach[i], sz, &sz + 1);  //!!! use varargs

    dw = mciSendString(ach, NULL, 0, NULL);

    DPF("MCIWndString('%s'): %ld",(LPSTR)ach, dw);

    if (fSetErr)
	MCIWndiHandleError(p, dw);

    return dw;
}


static long atol(LPSTR sz)
{
    long l;

    //!!! check for (-) sign?
    for (l=0; *sz >= '0' && *sz <= '9'; sz++)
        l = l*10 + (*sz - '0');

    return l;
}

#define SLASH(c)     ((c) == '/' || (c) == '\\')

/*--------------------------------------------------------------+
| FileName  - return a pointer to the filename part of szPath   |
|             with no preceding path.                           |
+--------------------------------------------------------------*/
LPSTR FAR FileName(LPSTR szPath)
{
    LPCSTR   sz;

    sz = &szPath[lstrlen(szPath)];
    for (; sz>szPath && !SLASH(*sz) && *sz!=':';)
        sz = AnsiPrev(szPath, sz);
    return (sz>szPath ? (LPSTR)++sz : (LPSTR)sz);
}

//
// Sends an MCI String command and converts the return string to an integer
// Optionally sets our error code.  Never clears it.
//
static DWORD MCIWndGetValue(PMCIWND p, BOOL fSetErr, LPSTR sz, DWORD err, ...)
{
    char    achRet[20];
    char    ach[256];
    DWORD   dw;
    int     i;

    for (i=0; *sz && *sz != ' '; )
	ach[i++] = *sz++;

    if (p->wDeviceID)
	i += wsprintf(&ach[i], " %d ", (UINT)p->alias);
    i += wvsprintf(&ach[i], sz, &err + 1);  //!!!use varargs

    dw = mciSendString(ach, achRet, sizeof(achRet), NULL);

    DPF("MCIWndGetValue('%s'): %ld",(LPSTR)ach, dw);

    if (fSetErr)
        MCIWndiHandleError(p, dw);

    if (dw == 0) {
        DPF("GetValue('%s'): %ld",(LPSTR)ach, atol(achRet));
        return atol(achRet);
    } else {
        DPF("MCIGetValue('%s'): error=%ld",(LPSTR)ach, dw);
	return err;
    }
}

//
// Send an MCI command and get the return string back
// This never sets our error code.
//
// Note: szRet can be the same string as sz
//
static DWORD MCIWndGet(PMCIWND p, LPSTR sz, LPSTR szRet, int len, ...)
{
    char    ach[256];
    int     i;
    DWORD   dw;

    if (!p->wDeviceID) {
	szRet[0] = 0;
	return 0L;
    }
    
    for (i=0; *sz && *sz != ' '; )
	ach[i++] = *sz++;

    i += wsprintf(&ach[i], " %d ", (UINT)p->alias);
    i += wvsprintf(&ach[i], sz, &len + 1);  //!!!use varargs

    // initialize to NULL return string
    szRet[0] = 0;

    dw = mciSendString(ach, szRet, len, p->hwnd);

    DPF("MCIWndGet('%s'): '%s'",(LPSTR)ach, (LPSTR)szRet);

    return dw;
}

//
// Gets the source or destination rect from the MCI device
// Does NOT set our error code since this is an internal function
//
static void MCIWndRect(PMCIWND p, LPRECT prc, BOOL fSource)
{
    MCI_DGV_RECT_PARMS      mciRect;
    DWORD dw=0;

    SetRectEmpty(prc);

    if (p->wDeviceID)
        dw = mciSendCommand(p->wDeviceID, MCI_WHERE,
            (DWORD)fSource ? MCI_DGV_WHERE_SOURCE : MCI_DGV_WHERE_DESTINATION,
            (DWORD)(LPVOID)&mciRect);

    if (dw == 0)
        *prc = mciRect.rc;

    prc->right  += prc->left;
    prc->bottom += prc->top;
}


static VOID MCIWndiSizePlaybar(PMCIWND p)
{
    RECT rc;
    WORD w, h;

    // No playbar!!
    if (p->dwStyle & MCIWNDF_NOPLAYBAR)
	return;

    #define SLOP 7      // Left outdent of toolbar

    // How big a window are we putting a toolbar on?
    GetClientRect(p->hwnd, &rc);
    w = rc.right;
    h = rc.bottom;

    // Trackbar is a child of Toolbar
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
    SetWindowPos(p->hwndTrackbar, NULL,
		rc.right, 3, w - rc.right + 5, TB_HEIGHT,	// !!!
		SWP_NOZORDER);

    //!!! Maybe put menu button on right side of trackbar?  So
    //!!! make sep the right size (size of the track bar!)
}

// Resize the window by the given percentage
// 0 means use DESTINATION rect and size it automatically
static VOID MCIWndiSize(PMCIWND p, int iSize)
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

    // Now we have the new size for our MCIWND.  If it's not changing size,
    // the SetWindowPos will not generate a WM_SIZE and it won't call our
    // SizePlaybar to fix the toolbar.  So we better call it ourselves.
    // Sometimes we're off by one pixel and it STILL won't generate a WM_SIZE.
    GetWindowRect(p->hwnd, &rcT);
    dx = ABS((rcT.right - rcT.left) - (rc.right - rc.left));
    dy = ABS((rcT.bottom - rcT.top) - (rc.bottom - rc.top));
    if (dx < 2 && dy < 2)
	MCIWndiSizePlaybar(p);

    SetWindowPos(p->hwnd, NULL, 0, 0, rc.right - rc.left,
                    rc.bottom - rc.top,
                    SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

    // We need to notify the "owner" that our size changed
    if (p->dwStyle & MCIWNDF_NOTIFYSIZE)
	NotifyOwner(p, MCIWNDM_NOTIFYSIZE, p->hwnd, NULL);
}


//
// Figure out the position in ms of the beginning of the track we're on
//
static DWORD MCIWndiPrevTrack(PMCIWND p)
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
static DWORD MCIWndiNextTrack(PMCIWND p)
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
static void MCIWndiCalcTracks(PMCIWND p)
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
static void MCIWndiMarkTics(PMCIWND p)
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

static VOID MCIWndiValidateMedia(PMCIWND p)
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
static void MCIWndiBuildMeAFilter(LPSTR pchD)
{
    LPSTR	pchS;
    char	ach[128];

    // Our filter will look like:  "MCI Files\0*.avi;*.wav\0All Files\0*.*\0"
    // The actual extensions for the MCI files will come from the list in
    // the "mci extensions" section of win.ini

    lstrcpy(pchD, LoadSz(IDS_MCIFILES));

    // Creates a list like: "avi\0wav\0mid\0"
    GetProfileString(szMCIExtensions, NULL, szNULL, ach, sizeof(ach));
	
    for (pchD += lstrlen(pchD)+1, pchS = ach; *pchS;
		pchD += lstrlen(pchS)+3, pchS += lstrlen(pchS)+1) {
	lstrcpy(pchD, "*.");
	lstrcpy(pchD + 2, pchS);
	lstrcpy(pchD + 2 + lstrlen(pchS), ";");
    }
    if (pchS != ach)
	--pchD;		// erase the last ;
    *pchD = '\0';
    lstrcpy(++pchD, LoadSz(IDS_ALLFILES));
    pchD += lstrlen(pchD) + 1;
    lstrcpy(pchD, "*.*\0");
}

//
// Create the playbar windows we'll need later
//
static void MCIWndiMakeMeAPlaybar(PMCIWND p)
{
    TBBUTTON            tb[7];

    extern char aszTrackbarClassName[];

    // They don't want a playbar
    if (p->dwStyle & MCIWNDF_NOPLAYBAR)
	return;

    tb[0].iBitmap = 0;
    tb[0].idCommand = MCI_PLAY;
    tb[0].fsState = TBSTATE_ENABLED | TBSTATE_HIDDEN;
    tb[0].fsStyle = TBSTYLE_BUTTON;
    tb[0].iString = -1;

    tb[1].iBitmap = 2;
    tb[1].idCommand = MCI_STOP;
    tb[1].fsState = TBSTATE_ENABLED | TBSTATE_HIDDEN;
    tb[1].fsStyle = TBSTYLE_BUTTON;
    tb[1].iString = -1;

    tb[2].iBitmap = 4;
    tb[2].idCommand = MCI_RECORD;
    tb[2].fsState = TBSTATE_ENABLED | TBSTATE_HIDDEN;
    tb[2].fsStyle = TBSTYLE_BUTTON;
    tb[2].iString = -1;

    tb[3].iBitmap = 5;
    tb[3].idCommand = IDM_MCIEJECT;
    tb[3].fsState = TBSTATE_ENABLED | TBSTATE_HIDDEN;
    tb[3].fsStyle = TBSTYLE_BUTTON;
    tb[3].iString = -1;

#define MENUSEP 2
    tb[4].iBitmap = MENUSEP;
    tb[4].idCommand = -1;
    tb[4].fsState = 0;
    tb[4].fsStyle = TBSTYLE_SEP;
    tb[4].iString = -1;

    tb[5].iBitmap = 3;
    tb[5].idCommand = IDM_MENU;
    tb[5].fsState = TBSTATE_ENABLED;
    tb[5].fsStyle = TBSTYLE_BUTTON;
    tb[5].iString = -1;

    tb[6].iBitmap = 4;
    tb[6].idCommand = TOOLBAR_END;
    tb[6].fsState = 0;
    tb[6].fsStyle = TBSTYLE_SEP;
    tb[6].iString = -1;

    // Create invisible for now so it doesn't flash
    p->hwndToolbar = CreateToolbarEx(p->hwnd,
        WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
            CCS_NOPARENTALIGN | CCS_NORESIZE,
        ID_TOOLBAR, 7, GetWindowInstance(p->hwnd),
	IDBMP_TOOLBAR, (LPTBBUTTON)&tb[0], 7,
        13, 13, 13, 13, sizeof(TBBUTTON));	// buttons are 13x13

    p->hwndTrackbar =
#ifdef BIDI
	CreateWindowEx(WS_EX_BIDI_SCROLL |  WS_EX_BIDI_MENU |WS_EX_BIDI_NOICON,
#else
	CreateWindow (
#endif
	aszTrackbarClassName, NULL,
        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, 0, 0, p->hwndToolbar, NULL, GetWindowInstance(p->hwnd), NULL);

    // Force ValidateMedia to actually update
    p->dwMediaStart = p->dwMediaLen = 0;

    // Set the proper range for the scrollbar
    MCIWndiValidateMedia(p);
}


//
// Gray/ungray toolbar buttons as necessary
//
static void MCIWndiPlaybarGraying(PMCIWND p)
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
static void MCIWndiFixMyPlaybar(PMCIWND p)
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
static void MCIWndiMakeMeAMenu(PMCIWND p)
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
        FreeDitherBrush();
    }
    p->hmenu = NULL;

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
	        AppendMenu(hmenu, MF_SEPARATOR, NULL, NULL);
	
	}

	if (hmenuWindow)
            AppendMenu(hmenu, MF_ENABLED|MF_POPUP, (UINT)hmenuWindow,
		LoadSz(IDS_VIEW));
	if (hmenuVolume)
	    AppendMenu(hmenu, MF_ENABLED|MF_POPUP, (UINT)hmenuVolume,
		LoadSz(IDS_VOLUME));
	if (hmenuSpeed)
            AppendMenu(hmenu, MF_ENABLED|MF_POPUP, (UINT)hmenuSpeed,
		LoadSz(IDS_SPEED));

	if (hmenuWindow || hmenuVolume || hmenuSpeed)
            AppendMenu(hmenu, MF_SEPARATOR, NULL, NULL);

        if (p->wDeviceID && p->fCanRecord && (p->dwStyle & MCIWNDF_RECORD))
            AppendMenu(hmenu, MF_ENABLED, IDM_MCINEW, LoadSz(IDS_NEW));

	if (!(p->dwStyle & MCIWNDF_NOOPEN))
	    AppendMenu(hmenu, MF_ENABLED, IDM_MCIOPEN,  LoadSz(IDS_OPEN));

        if (p->wDeviceID && p->fCanSave && (p->dwStyle & MCIWNDF_RECORD))
            AppendMenu(hmenu, MF_ENABLED, MCI_SAVE, LoadSz(IDS_SAVE));

	if (p->wDeviceID) {
	    if (!(p->dwStyle & MCIWNDF_NOOPEN)) {
		AppendMenu(hmenu, MF_ENABLED, IDM_MCICLOSE, LoadSz(IDS_CLOSE));
	    
		AppendMenu(hmenu, MF_SEPARATOR, NULL, NULL);
	    }

	    AppendMenu(hmenu, MF_ENABLED, IDM_COPY, LoadSz(IDS_COPY));
	    
	    if (p->fCanConfig)
                AppendMenu(hmenu, MF_ENABLED, IDM_MCICONFIG,
			LoadSz(IDS_CONFIGURE));

	    // !!! Should we only show this in debug, or if a flag is set?
            AppendMenu(hmenu, MF_ENABLED, IDM_MCICOMMAND, LoadSz(IDS_COMMAND));
	}

	p->hmenu = hmenu;
	p->hmenuVolume = hmenuVolume;
	p->hmenuSpeed = hmenuSpeed;

 	CreateDitherBrush(FALSE);	// we'll need this to paint OwnerDraw
    }
}

//
// Set up everything for an empty window
//
static LONG MCIWndiClose(PMCIWND p, BOOL fRedraw)
{
    MCI_GENERIC_PARMS   mciGeneric;

    // Oh no!  The MCI device (probably MMP) has hooked our window proc and if
    // we close the device, it will go away, and the hook will DIE!  We need to
    // do everything BUT the closing of the device.  We'll delay that.
    if (GetWindowLong(p->hwnd, GWL_WNDPROC) != (LONG)MCIWndProc &&
    		p->wDeviceID && p->fCanWindow) {
        MCIWndString(p, FALSE, szWindowHandle, NULL);	// GO AWAY, DEVICE!
	PostMessage(p->hwnd, MCI_CLOSE, 0, p->wDeviceID);
    } else if (p->wDeviceID)
	// buggy drivers crash if we pass a null parms address
        mciSendCommand(p->wDeviceID, MCI_CLOSE, 0, (DWORD)(LPVOID)&mciGeneric);

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
    if (p->dwStyle & MCIWNDF_NOTIFYMEDIA)
        NotifyOwner(p, MCIWNDM_NOTIFYMEDIA, p->hwnd, (LPARAM)(LPVOID)szNULL);

    InvalidateRect(p->hwnd, NULL, TRUE);
    return 0;
}

//
// This is the WM_CREATE msg of our WndProc
//
static BOOL MCIWndiCreate(HWND hwnd, LONG lParam)
{
    PMCIWND             p;
    DWORD		dw;
    char                ach[20];
    HWND                hwndP;

    p = (PMCIWND)LocalAlloc(LPTR, sizeof(MCIWND));

    if (!p)
        return FALSE;

    SetWindowLong(hwnd, 0, (LONG)(UINT)p);

    p->hwnd = hwnd;
    p->hwndOwner = GetParent(hwnd);	// we'll send notifications here
    p->alias = (UINT)hwnd;
    p->dwStyle = GetWindowLong(hwnd, GWL_STYLE);

    DragAcceptFiles(p->hwnd, (p->dwStyle & (MCIWNDF_NOMENU | MCIWNDF_NOOPEN)) == 0);
    
    if (!(p->dwStyle & WS_CAPTION))
          p->dwStyle &= ~MCIWNDF_SHOWALL;

    dw = (DWORD)((LPCREATESTRUCT)lParam)->lpCreateParams;

    //
    // see if we are in a MDIClient
    //
    if ((p->dwStyle & WS_CHILD) && (hwndP = GetParent(hwnd))) {
        GetClassName(hwndP, ach, sizeof(ach));
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
    p->achFileName[0] = '\0';
    p->ofn.lStructSize = sizeof(OPENFILENAME);
    p->ofn.hwndOwner = hwnd;
    p->ofn.hInstance = NULL;
//  p->ofn.lpstrFilter = szOpenFilter;
    p->ofn.lpstrCustomFilter = NULL;
    p->ofn.nMaxCustFilter = 0;
    p->ofn.nFilterIndex = 0;
;   p->ofn.lpstrFile = p->achFileName;
;   p->ofn.nMaxFile = sizeof(p->achFileName);
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

    if (dw && *(LPSTR)dw)     // treat extra parm as a filename
        MCIWndOpen(hwnd, (LPSTR)dw, 0);

    return TRUE;
}

//
// Brings up an OpenDialog or a SaveDialog for the application and returns the
// filename.  Returns TRUE if a file name was chosen, FALSE on error or CANCEL.
//
static BOOL MCIWndOpenDlg(PMCIWND p, BOOL fSave, LPSTR szFile, int len)
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
static void MCIWndiSetTimer(PMCIWND p)
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
// Save a file.  Returns 0 for success
//
static LONG MCIWndiSave(PMCIWND p, WORD wFlags, LPSTR szFile)
{
    char                ach[128];

    //
    // If we don't have a filename to save, then get one from a dialog
    //
    if (szFile == (LPVOID)-1L) {
	lstrcpy(ach, p->achFileName);
        if (!MCIWndOpenDlg(p, TRUE, ach, sizeof(ach)))
            return -1;
        szFile = ach;
    }

    // !!! All good little boys should be saving to background... don't wait
    return MCIWndString(p, TRUE, szSave, szFile);
}

//
// Actually open a file and set up the window.  Returns 0 for success
//
static LONG MCIWndiOpen(PMCIWND p, WORD wFlags, LPSTR szFile)
{
    DWORD               dw = 0;
    HCURSOR             hcurPrev;
    char                ach[128];
    UINT                wDeviceID;
    BOOL 		fNew = wFlags & MCIWNDOPENF_NEW;

    //
    // We're opening an existing file, szFile is that filename
    // If we don't have a filename to open, then get one from a dialog
    //
    if (!fNew && szFile == (LPVOID)-1L) {
	lstrcpy(ach, p->achFileName);
        if (!MCIWndOpenDlg(p, FALSE, ach, sizeof(ach)))
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
	MCIWndGetDevice(p->hwnd, ach, sizeof(ach));
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
        // silly hack to check for entension.
        //
        if (lstrlen(szFile) > 4 && szFile[lstrlen(szFile)-4] == '.')
            dw = 0;
        else
            dw = MCIWndGetValue(p, FALSE, szOpenShareable, 0,
                (LPSTR)szFile, (UINT)p->alias);

        // Error! Try again, not shareable.
        if (dw == 0) {
            dw = MCIWndGetValue(p, FALSE, szOpen, 0,
		(LPSTR)szFile, (UINT)p->alias);
	    // Last ditch attempt! Try AVI. It'll open anything.  This time,
	    // show, set errors.
	    if (dw == 0) {
                dw = MCIWndGetValue(p, TRUE, szOpenAVI, 0,
		    (LPSTR)szFile, (UINT)p->alias);
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
    
    // Copy the file or device name into our filename spot
    lstrcpy(p->achFileName, szFile);

    // !!! p->wDeviceType = QueryDeviceTypeMCI(p->wDeviceID);

    // Now set the playback window to be our MCI window
    p->fCanWindow = MCIWndString(p, FALSE, szWindowHandle, (UINT)p->hwnd) == 0;

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
#ifdef DEBUG
    // !!! MCIAVI says no the stupid driver...
    p->fCanConfig = p->fCanWindow;
#endif

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

    // set window text
    if (p->dwStyle & MCIWNDF_SHOWNAME)
        SetWindowText(p->hwnd, FileName(szFile));

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
    if (p->dwStyle & MCIWNDF_NOTIFYMEDIA)
        NotifyOwner(p, MCIWNDM_NOTIFYMEDIA, p->hwnd, (LPARAM)szFile);

    // Make sure the newly opened movie paints in the window now
    InvalidateRect(p->hwnd, NULL, TRUE);

    return 0;	// success
}

//
// Set the caption based on what they want to see... Name? Pos? Mode?
//
static VOID MCIWndiSetCaption(PMCIWND p)
{
    char	ach[200], achMode[40], achT[40], achPos[40];

    // Don't touch their window text if they don't want us to
    if (!(p->dwStyle & MCIWNDF_SHOWALL))
	return;

    ach[0] = 0;

    if (p->wDeviceID == NULL)
	return;

    if (p->dwStyle & MCIWNDF_SHOWNAME)
	wsprintf(ach, "%s", FileName(p->achFileName));

    if (p->dwStyle & (MCIWNDF_SHOWPOS | MCIWNDF_SHOWMODE))
	lstrcat(ach, " (");

    if (p->dwStyle & MCIWNDF_SHOWPOS) {

	// Get the pretty version of the position as a string
	MCIWndGetPositionString(p->hwnd, achPos, sizeof(achPos));

        if (p->dwStyle & MCIWNDF_SHOWMODE)
	    wsprintf(achT, "%s - ", (LPSTR)achPos);
	else
	    wsprintf(achT, "%s", (LPSTR)achPos);
	lstrcat(ach, achT);
    }

    if (p->dwStyle & MCIWNDF_SHOWMODE) {
	MCIWndGet(p, szStatusMode, achMode, sizeof(achMode));
	lstrcat(ach, achMode);
    }

    if (p->dwStyle & (MCIWNDF_SHOWPOS | MCIWNDF_SHOWMODE))
	lstrcat(ach, ")");

    SetWindowText(p->hwnd, ach);
}

// We never use this any more
#if 0
static BOOL MCIWndSeekExact(PMCIWND p, BOOL fExact)
{
    DWORD dw;
    BOOL  fWasExact;

    if (p->wDeviceID == NULL)
        return FALSE;

    // see if the device even has this feature
    dw = MCIWndString(p, FALSE, szStatusSeekExactly);
    if (dw != 0)
        return FALSE;

    // get current value.
    dw = MCIWndStatus(p, MCI_DGV_STATUS_SEEK_EXACTLY, MCI_OFF);
    fWasExact = (dw != MCI_OFF) ? TRUE : FALSE;

    if (fExact)
	dw = MCIWndString(p, FALSE, szSetSeekExactOn);
    else
	dw = MCIWndString(p, FALSE, szSetSeekExactOff);

    return fWasExact;
}
#endif

static LONG MCIWndiChangeStyles(PMCIWND p, UINT mask, UINT value)
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
	    p->hwndToolbar = NULL;
	    p->hwndTrackbar = NULL;	// child destroyed automatically
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
static void MCIWndiPlaySeek(PMCIWND p, BOOL fBackwards)
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
static void MCIWndiTimerStuff(PMCIWND p)
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
	if (p->wDeviceID == NULL)
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
		NotifyOwner(p, MCIWNDM_NOTIFYMODE, p->hwnd, dwMode);

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
		NotifyOwner(p, MCIWNDM_NOTIFYPOS, p->hwnd, dwPos);

	    //
	    // Set the Window Caption to include the new position
	    //
	    if ((p->dwStyle & MCIWNDF_SHOWPOS))
		MCIWndiSetCaption(p);

	    //
	    // Update the trackbar to the new position but not while
	    // we're dragging the thumb
	    //
	    if (!(p->dwStyle & MCIWNDF_NOPLAYBAR) && !p->fScrolling)
		SendMessage(p->hwndTrackbar, TBM_SETPOS, TRUE, dwPos);
	}
    }
}


static void MCIWndiDrop(HWND hwnd, WPARAM wParam)
{
    char	szPath[256];
    UINT	nDropped;

    // Get number of files dropped
    nDropped = DragQueryFile((HANDLE)wParam,0xFFFF,NULL,0);
    
    if (nDropped) { 
	SetActiveWindow(hwnd);

	// Get the file that was dropped....
	DragQueryFile((HANDLE)wParam, 0, szPath, sizeof(szPath));

	MCIWndOpen(hwnd, szPath, 0);
    }
    DragFinish((HANDLE)wParam);     /* Delete structure alocated */
}

/*--------------------------------------------------------------+
| MCIWndProc - MCI window's window proc                         |
|                                                               |
+--------------------------------------------------------------*/
LONG FAR PASCAL _loadds MCIWndProc(HWND hwnd, unsigned msg, WORD wParam, LONG lParam)
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
    char		ach[80];
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

        // Make the trackbar background LTGRAY like the toolbar
        case WM_CTLCOLOR:
            if ((HWND)LOWORD(lParam) == p->hwndTrackbar)
		return (LRESULT)(UINT)GetStockObject(LTGRAY_BRUSH);
            break;

	case MCI_SAVE:
	    // wParam presently unused and not given by the macro
	    return MCIWndiSave(p, wParam, (LPSTR)lParam);

	case MCI_OPEN:
	    return MCIWndiOpen(p, wParam, (LPSTR)lParam);
	    
	case MCIWNDM_NEW:
	    return MCIWndiOpen(p, MCIWNDOPENF_NEW, (LPSTR)lParam);
	
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
			(DWORD)(LPVOID)&mciGeneric);
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
	    dw = MCIWndString(p, TRUE, szPlayReverse, (LPSTR)szNULL);
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
			(DWORD)(LPVOID)&mciGeneric);
		
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
		MCIWndGet(p, szStatusMode, (LPSTR)lParam, (UINT)wParam);
	    return MCIWndStatus(p, MCI_STATUS_MODE, MCI_MODE_NOT_READY);

	// Return the position as a string if they give us a buffer
	case MCIWNDM_GETPOSITION:
            if (lParam) {
		// If we can do tracks, let's give them a pretty string
		if (p->fHasTracks)
        	    MCIWndString(p, FALSE, szSetFormatTMSF);
		MCIWndGet(p, szStatusPosition, (LPSTR)lParam,(UINT)wParam);
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
            return MCIWndGetValue(p, FALSE, szStatusPalette, NULL);

        case MCIWNDM_SETPALETTE:
            return MCIWndString(p, TRUE, szSetPalette, (HPALETTE)wParam);

	//
	// Returns our error code
	//
	case MCIWNDM_GETERROR:
	    if (lParam) {
		mciGetErrorString(p->dwError, (LPSTR)lParam, (UINT)wParam);
	    }
	    dw = p->dwError;
	//    p->dwError = 0L;	// we never clear the error
	    return dw;

	case MCIWNDM_GETFILENAME:
	    if (lParam)
	        lstrcpyn((LPSTR)lParam, p->achFileName, (UINT)wParam);
	    return (lParam == NULL);	// !!!

	case MCIWNDM_GETDEVICE:
	    if (lParam)
	        return MCIWndGet(p, szSysInfo, (LPSTR)lParam,
		    (UINT)wParam);
	    return 42;	// !!!

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
	    dw = MCIWndString(p, TRUE, szSetFormat, (LPSTR)lParam);
	    MCIWndiValidateMedia(p);
	    return dw;

	case MCIWNDM_GETTIMEFORMAT:
	    if (lParam)
		MCIWndGet(p, szStatusFormat, (LPSTR)lParam, (UINT)wParam);
	    return MCIWndStatus(p, MCI_STATUS_TIME_FORMAT, 0);	// !!!

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
	    if (lstrcmpi((LPSTR)lParam, szClose) == 0)
		return MCIWndClose(hwnd);

	    // Always sets/clears our error code
            dw = MCIWndGet(p, (LPSTR)lParam, p->achReturn,sizeof(p->achReturn));
	    MCIWndiHandleError(p, dw);
	    // kick ourselves in case mode changed from this command
	    MCIWndiTimerStuff(p);
            return dw;

	// Gets the return string from the most recent MCIWndSendString()
        case MCIWNDM_RETURNSTRING:
	    if (lParam)
	        lstrcpyn((LPSTR)lParam, p->achReturn, wParam);
	    return (lParam == NULL);	// !!!

        case MCIWNDM_REALIZE:
	    // buggy drivers crash if we pass a null parms address
            dw = mciSendCommand(p->wDeviceID, MCI_REALIZE,
                (BOOL)wParam ? MCI_ANIM_REALIZE_BKGD : MCI_ANIM_REALIZE_NORM,
		(DWORD)(LPVOID)&mciGeneric);
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
	    return MCIWndiOpen(p, 0, (LPSTR)ach);

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
                            (DWORD)(LPVOID)&mciUpdate);

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
			SendMessage(hwnd, WM_HSCROLL, TB_LINEUP, 0); break;
		    case VK_RIGHT:
			SendMessage(hwnd, WM_HSCROLL, TB_LINEDOWN, 0); break;
		    case VK_PRIOR:
			SendMessage(hwnd, WM_HSCROLL, TB_PAGEUP, 0); break;
		    case VK_NEXT:
			SendMessage(hwnd, WM_HSCROLL, TB_PAGEDOWN, 0); break;
		    case VK_HOME:
			SendMessage(hwnd, WM_HSCROLL, TB_TOP, 0); break;
		    case VK_END:
			SendMessage(hwnd, WM_HSCROLL, TB_BOTTOM, 0); break;

		    case VK_UP:
		    case VK_DOWN:
			dw = MCIWndGetValue(p, FALSE, szStatusVolume, 1000);
			if (wParam == VK_UP)
			    i = min((int)p->wMaxVol * 10, (int) dw + 100);
			else
			    i = max(0, (int) dw - 100);
			
			MCIWndSetVolume(p->hwnd, i);
			break;
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
		    break;
                case VK_ESCAPE:
                    MCIWndStop(hwnd);
                    break;
		default:
		    break;
	    }
			
    	    if (GetKeyState(VK_CONTROL) & 0x8000) {
		switch(wParam) {
		    case '1':
		    case '2':
		    case '3':
		    case '4':
			if (!(p->dwStyle & MCIWNDF_NOAUTOSIZEWINDOW))
			    MCIWndSetZoom(hwnd, 100 * (wParam - '0'));
			break;
		    case 'P':
			MCIWndPlay(hwnd); break;
		    case 'S':
			MCIWndStop(hwnd); break;
		    case 'D':
			PostMessage(hwnd, WM_COMMAND, IDM_MCICONFIG, 0); break;
		    case 'C':
			PostMessage(hwnd, WM_COMMAND, IDM_COPY, 0); break;
		    case VK_F5:
			PostMessage(hwnd, WM_COMMAND, IDM_MCICOMMAND, 0); break;
		    case 'F':
		    case 'O':
			if (!(p->dwStyle & MCIWNDF_NOOPEN))
			    MCIWndOpenDialog(hwnd);
			break;
		    case 'M':
			PostMessage(hwnd, WM_COMMAND, ID_TOOLBAR,
				MAKELONG(IDM_MENU, TBN_BEGINDRAG)); break;
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
		    if (!(p->dwStyle & MCIWNDF_NOAUTOSIZEWINDOW))
			MCIWndSetZoom(hwnd, 100 / ((UINT) wParam - '0'));
		    return 0;	// break will ding
	        default:
		    break;
	    }
	    break;

	case WM_HSCROLL:

#define FORWARD	 1
#define BACKWARD 2

            dwPos = SendMessage(p->hwndTrackbar, TBM_GETPOS, 0, 0);

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
		    MCIWndGet(p, szStatusForward, ach, sizeof(ach));
		    if (ach[0] == 'F' || ach[0] == 'f')
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
	    // Check for ZOOM commands
	    if (wParam >= IDM_MCIZOOM && wParam < IDM_MCIZOOM + 1000)
		MCIWndSetZoom(hwnd, wParam - IDM_MCIZOOM);

	    // !!! Hack from Hell
	    // If our bogus top menu item is selected, turn it into the REAL
	    // menu item closest to it.
	    if (wParam == IDM_MCIVOLUME + VOLUME_MAX + 1)
		wParam = IDM_MCIVOLUME + p->wMaxVol;
	    if (wParam == IDM_MCIVOLUME + VOLUME_MAX + 2)
		wParam = IDM_MCIVOLUME;

	    // VOLUME command? Uncheck old one, reset volume, and check new one
	    // Round to the nearest 5 to match a menu identifier
	    if (wParam >=IDM_MCIVOLUME && wParam <=IDM_MCIVOLUME + p->wMaxVol) {
		if (MCIWndSetVolume(hwnd, (wParam - IDM_MCIVOLUME) * 10) == 0
					&& lParam != 42) {
	            CheckMenuItem(p->hmenuVolume, p->uiHack, MF_UNCHECKED);
		    // change state only for a real command, not while dragging
		    CheckMenuItem(p->hmenuVolume, wParam, MF_CHECKED);
		}
	    }

	    // !!! Hack from Hell
	    // If our bogus top menu item is selected, turn it into the REAL
	    // menu item closest to it.
	    if (wParam == IDM_MCISPEED + SPEED_MAX + 1)
		wParam = IDM_MCISPEED + SPEED_MAX;
	    if (wParam == IDM_MCISPEED + SPEED_MAX + 2)
		wParam = IDM_MCISPEED;

	    // SPEED command? Uncheck old one, reset speed, and check new one
	    // Round to the nearest 5 to match a menu identifier
	    if (wParam >=IDM_MCISPEED && wParam <= IDM_MCISPEED + SPEED_MAX) {
		if (MCIWndSetSpeed(hwnd, (wParam - IDM_MCISPEED) * 10) == 0
					&& lParam != 42) {
		    // change state only for a real command, not while dragging
	            CheckMenuItem(p->hmenuSpeed, p->uiHack, MF_UNCHECKED);
		    CheckMenuItem(p->hmenuSpeed, wParam, MF_CHECKED);
		}
	    }

	    switch(wParam)
	    {
		MSG msgT;
		RECT rcT;

                case MCI_RECORD:
                    if (GetKeyState(VK_SHIFT) < 0)
                    {
                        //!!! toggle?
                        //MCIWndRecordPreview(hwnd);
                    }
                    else
                    {
                        MCIWndRecord(hwnd);
                    }
                    break;

                //            PLAY = normal play
                //      SHIFT+PLAY = play backward
                //       CTRL+PLAY = play fullscreen
                // SHIFT+CTRL+PLAY = play fullscreen backward
                //
                case MCI_PLAY:

	#define MaybeRepeat (p->fRepeat ? (LPSTR)szRepeat : (LPSTR)szNULL)

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
								(LPSTR)szNULL);
                        } else {
                            if (MCIWndString(p, FALSE, szPlayFullscreen,
								MaybeRepeat))
                            	MCIWndString(p, TRUE, szPlayFullscreen,
								(LPSTR)szNULL);
			}
                    } else if (GetKeyState(VK_SHIFT) < 0) {
                        if (MCIWndString(p, FALSE, szPlayReverse, MaybeRepeat))
                            MCIWndString(p, TRUE, szPlayReverse, (LPSTR)szNULL);
                    } else {
                        if (MCIWndString(p, FALSE, szPlay, MaybeRepeat))
                    	    MCIWndString(p, TRUE, szPlay, (LPSTR)szNULL);
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

                case ID_TOOLBAR:
                    if (HIWORD(lParam) != TBN_BEGINDRAG ||
                        LOWORD(lParam) != IDM_MENU ||
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
		        if (PtInRect(&rcT, MAKEPOINT(msgT.lParam)))
			    PeekMessage(&msgT, p->hwndToolbar, WM_LBUTTONDOWN,
				        WM_LBUTTONDOWN, PM_REMOVE);
		    }

                    break;

                default:
		    break;
	    }
            break;

        case WM_DESTROY:
	    // !!! MMP CLOSE will be deferred till AFTER the DESTROY

	    // Don't palette kick when we're going down.  Not necessary and VB
	    // is stupid and rips.
	    p->fHasPalette = FALSE;
            MCIWndiClose(p, FALSE);  //don't leave us playing into a random DC

	    if (p->hmenu) {
                DestroyMenu(p->hmenu);
		FreeDitherBrush();
	    }

 	    if (p->pTrackStart)
		LocalFree((HANDLE)p->pTrackStart);

	    if (p->hfont) {
		// !!! Someone else may have to go and create it again, but oh
		// !!! well.
		DeleteObject(p->hfont);
		p->hfont = NULL;
	    }

	    if (p->hicon)
		DestroyIcon(p->hicon);
	    
	    // We can't destroy our pointer and then fall through and use it
	    f = p->fMdiWindow;
	    LocalFree((HLOCAL) p);
	    SetWindowLong(hwnd, 0, NULL);	// our p
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
			    dw = p->dwStyle;
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
			    dw = p->dwStyle;
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
			    dw = p->dwStyle;
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
	    return (LONG)(UINT)p->hicon;
    }

    if (p && p->fMdiWindow)
        return DefMDIChildProc(hwnd, msg, wParam, lParam);
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void NEAR PASCAL PatRect(HDC hdc,int x,int y,int dx,int dy)
{
    RECT    rc;

    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
}

#define FillRC(hdc, prc)    PatRect(hdc, (prc)->left, (prc)->top, (prc)->right - (prc)->left, (prc)->bottom-(prc)->top)

//
// Draw the channel for the volume and speed menu controls
//
static void NEAR PASCAL DrawChannel(HDC hdc, LPRECT prc)
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

static LONG OwnerDraw(PMCIWND p, UINT msg, WORD wParam, LONG lParam)
{
    RECT        rc, rcMenu, rcChannel, rcThumb;
    HDC         hdc;
    int         i,dx,dy,len;
    char        ach[10];
    DWORD       dw;
    HWND	hwnd = p->hwnd;

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

//              SetBkColor(hdc, GetSysColor(COLOR_BTNHILIGHT));
                SetBkColor(hdc, RGB(255,255,255));
                PatRect(hdc, rc.left,   rc.top,   1,dy);
                PatRect(hdc, rc.left,   rc.top,   dx,1);

                SetBkColor(hdc, GetSysColor(COLOR_BTNSHADOW));
                PatRect(hdc, rc.right-1,rc.top+1, 1,dy-1);
                PatRect(hdc, rc.left+1, rc.bottom-1, dx-1,1);

                InflateRect(&rc,-1,-1);

                SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
                SelectObject(hdc, p->hfont);
                len = wsprintf(ach, "%d", lpMIS->itemID % 1000);
                dw = GetTextExtent(hdc, ach, len);
                ExtTextOut(hdc,
                    (rc.right  + rc.left - LOWORD(dw))/2,
                    (rc.bottom + rc.top - HIWORD(dw))/2,
                    ETO_OPAQUE,&rc,ach,len,NULL);
//              FillRC(hdc, &rc);

		//
		// Exclude the ClipRect that all that garbage drew into
		//
                ExcludeClipRect(hdc, rcThumb.left, rcThumb.top,
                        rcThumb.right, rcThumb.bottom);
#if 0   // why?
		ExcludeClipRect(hdc, rcThumb.left+1, rcThumb.top,
			rcThumb.right-1, rcThumb.bottom);
		ExcludeClipRect(hdc, rcThumb.left, rcThumb.top+1,
			rcThumb.left+1, rcThumb.bottom-1);
		ExcludeClipRect(hdc, rcThumb.right-1, rcThumb.top+1,
			rcThumb.right, rcThumb.bottom-1);
#endif
		//
		// Next, draw the channel
		//
                DrawChannel(hdc, &rcChannel);
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
BOOL FAR PASCAL _loadds mciDlgProc(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
    char    ach[255];
    UINT    w;
    DWORD   dw;
    PMCIWND p;
    HWND    hwndP;

    switch (msg)
    {
        case WM_INITDIALOG:
	    // Remember our actually true parent
	    SetWindowLong(hwnd, DWL_USER, lParam);
	    PositionWindowNearParent(hwnd);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
#ifdef WIN32
                    SendDlgItemMessage(hwnd, IDC_MCICOMMAND, EM_SETSEL, 0, (LPARAM)-1);
#else
                    SendDlgItemMessage(hwnd, IDC_MCICOMMAND, EM_SETSEL, 0, MAKELONG(0, -1));
#endif
                    w = GetDlgItemText(hwnd, IDC_MCICOMMAND, ach, sizeof(ach));

		    hwndP = (HWND)GetWindowLong(hwnd, DWL_USER);
		    p = (PMCIWND)(UINT)GetWindowLong(hwndP, 0);

		    // special case the CLOSE command to do our clean up
		    if (lstrcmpi((LPSTR)ach, szClose) == 0) {
			MCIWndClose(hwndP);
			break;
		    }

                    dw = MCIWndGet(p, ach, ach, sizeof(ach));

                    if (dw != 0)
                        mciGetErrorString(dw, ach, sizeof(ach));

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

static BOOL NEAR PASCAL mciDialog(HWND hwnd)
{
    DialogBoxParam(hInst, MAKEINTATOM(DLG_MCICOMMAND), hwnd,
		(DLGPROC)mciDlgProc, hwnd);
    return TRUE; 
}


//
// Code to implement the Copy command:
//
//
// MCIWnd tries to copy the same things to the clipboard that VfW MPlayer
// would have.
// 

#define SLASH(c)     ((c) == '/' || (c) == '\\')
/**************************************************************************

    convert a file name to a fully qualifed path name, if the file
    exists on a net drive the UNC name is returned.

***************************************************************************/

static BOOL NetParseFile(LPSTR szFile, LPSTR szPath)
{
    char        achDrive[4];
    char        achRemote[128];
    int         cbRemote = sizeof(achRemote);
    OFSTRUCT    of;

    if (szPath == NULL)
        szPath = szFile;
    else
        szPath[0] = 0;

    //
    // Fully qualify the file name
    //
    if (OpenFile(szFile, &of, OF_PARSE) == -1)
        return FALSE;

    lstrcpy(szPath, of.szPathName);

    //
    // if the file is not drive based (probably UNC)
    //
    if (szPath[1] != ':')
        return TRUE;

    achDrive[0] = szPath[0];
    achDrive[1] = ':';
    achDrive[2] = '\0';

    if (WNetGetConnection(achDrive, achRemote, &cbRemote) != WN_SUCCESS)
        return FALSE;

    if (!SLASH(achRemote[0]) || !SLASH(achRemote[1]))
	return TRUE;

    lstrcat(achRemote, szPath+2);
    lstrcpy(szPath, achRemote);

    return TRUE;
}



SZCODE aszMPlayerName[]           = "MPlayer";
HANDLE GetMPlayerData(PMCIWND p)
{
    char        szFileName[128];
    char	ach[40];
    char        szDevice[40];
    HANDLE      h;
    LPSTR       psz;
    int         len;
    LPSTR	lpszCaption = szFileName;
    UINT	wOptions;
    RECT	rc;
    BOOL	fCompound, fFile;
    DWORD	dw;
    MCI_GETDEVCAPS_PARMS    mciDevCaps; /* for the MCI_GETDEVCAPS command */

    //
    // Get the Device Name
    //
    MCIWndGet(p, "sysinfo installname", szDevice, sizeof(szDevice));
    
    //
    // determine if the device is simple or compound
    //
    mciDevCaps.dwItem = MCI_GETDEVCAPS_COMPOUND_DEVICE;
    dw = mciSendCommand(p->wDeviceID, MCI_GETDEVCAPS,
        MCI_GETDEVCAPS_ITEM, (DWORD)(LPSTR)&mciDevCaps);
    fCompound = (dw == 0 && mciDevCaps.dwReturn != 0);

    //
    // determine if the device handles files
    //
    if (fCompound) {
        mciDevCaps.dwItem = MCI_GETDEVCAPS_USES_FILES;
        dw = mciSendCommand(p->wDeviceID, MCI_GETDEVCAPS,
            MCI_GETDEVCAPS_ITEM, (DWORD)(LPSTR)&mciDevCaps);
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
			(szFileName[lstrlen(szDevice)] == '!')) {
	    lstrcpy(szFileName, &(p->achFileName[lstrlen(szDevice) + 1]));
	}

        NetParseFile(szFileName, (LPSTR)NULL);
        OemToAnsi(szFileName,szFileName);	// Map extended chars.
    } else {
	szFileName[0] = 0;
    }

#ifdef DEBUG
    DPF("  GetLink: %s|%s!%s\n",
        (LPSTR)aszMPlayerName,
        (LPSTR)szFileName,
        (LPSTR)szDevice);
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
    
    wsprintf(psz, "%s%c%s%c%s%c%d%c%ld%c%ld%c%ld%c%d%c%s%c",
        (LPSTR)aszMPlayerName, 0,
        (LPSTR)szFileName, 0,
        (LPSTR)szDevice, ',',
	wOptions, ',',
	0L, ',', // !!! sel start
	0L, ',', // !!! sel length
	p->dwPos, ',',
	rc.bottom - rc.top, ',',
        lpszCaption, 0);

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
		    (DWORD)(LPVOID)&mciUpdate);
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
    int         nNumEntries;
    int         i;

    if (!hpal)
        return NULL;

    GetObject(hpal,sizeof(int),(LPSTR)&nNumEntries);

    if (nNumEntries == 0)
        return NULL;

    ppal = (PLOGPALETTE)LocalAlloc(LPTR,sizeof(LOGPALETTE) +
                nNumEntries * sizeof(PALETTEENTRY));

    if (!ppal)
        return NULL;

    ppal->palVersion    = 0x300;
    ppal->palNumEntries = nNumEntries;

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

    GetObject(hbm,sizeof(bm),(LPSTR)&bm);

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
        (LPSTR)lpbi + (int)lpbi->biSize + (int)lpbi->biClrUsed * sizeof(RGBQUAD),
        (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

    if (hpal)
        SelectPalette(hdc,hpalT,FALSE);

    DeleteDC(hdc);

    return hdib;
}

SZCODE aszNative[]            = "Native";
SZCODE aszOwnerLink[]         = "OwnerLink";

// Pretend to be MPlayer copying to the clipboard
static void NEAR PASCAL MCIWndCopy(PMCIWND p)
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

static void cdecl dprintf(PSTR szFormat, ...)
{
    char ach[128];

    static BOOL fDebug = -1;

    if (fDebug == -1)
        fDebug = GetProfileInt(szDebug, MODNAME, FALSE);

    if (!fDebug)
        return;

    lstrcpy(ach, MODNAME ": ");
    wvsprintf(ach+lstrlen(ach),szFormat,(LPSTR)(&szFormat+1));
    lstrcat(ach, "\r\n");

    OutputDebugString(ach);
}

#endif
