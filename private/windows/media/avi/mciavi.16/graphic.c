/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1991. All rights reserved.

   Title:   graphic.c - Multimedia Systems Media Control Interface
            driver for AVI.

*****************************************************************************/

#include "graphic.h"
//#include "dispdib.h"
#include "cnfgdlg.h"
#include <string.h>
#ifdef EXPIRE
#include <dos.h>
#endif
#include "avitask.h"

#ifdef DEBUG
#define static
#endif

//
//  This is the version number of MSVIDEO.DLL we need in order to run
//  build 81 is when we added the VideoForWindowsVersion() function to
//  MSVIDEO.DLL
//
//  in build 85
//    we removed the ICDecompressOpen() function and it became a macro.
//    we added a parameter to ICGetDisplayFormat()
//    we make DrawDibProfileDisplay() take a parameter
//
//  in build 108
//    Added ICOpenFunction() to open a hic using a function directly,
//	without calling ICInstall
//    Added some more ICDRAW_ messages
//
//  in build 109
//    Addded ICMessage() to compman
//    removed ICDrawSuggest() made it a macro.
//    Added ICMODE_FASTDECOMPRESS to ICLocate()
//
//  Under NT the first build is sufficient !!! Is this true now?
//
#ifdef WIN32
#define MSVIDEO_VERSION     (0x01000000)          // 1.00.00.00
#else
#define MSVIDEO_VERSION     (0x010a0000l+109)     // 1.10.00.109
#endif

/* statics */
static INT              swCommandTable = -1;

#ifdef WIN32
static SZCODE		szDisplayDibLib[] = TEXT("DISPDB32.DLL");
#else
static SZCODE		szDisplayDibLib[] = TEXT("DISPDIB.DLL");
#endif

/*
 * files should be UNICODE. function names should not
 */
static SZCODEA          szDisplayDib[]    = "DisplayDib";
static SZCODEA          szDisplayDibEx[]  = "DisplayDibEx";
#ifdef WIN32
STATICDT SZCODE         szMSVideo[]       = TEXT("MSVFW32");  // With GetModuleHandle
#else
static SZCODE           szMSVideo[]       = TEXT("MSVIDEO");
#endif

BOOL   gfEvil;          // TRUE if we cant close cuz dialog box is up
BOOL   gfEvilSysMenu;   // TRUE if we cant close cuz system menu is up

NPMCIGRAPHIC npMCIList; // list of all open instances.

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void | GraphicInit | This function is called when the DriverProc
 *      gets a DRV_LOAD message.
 *
 ***************************************************************************/
BOOL FAR PASCAL GraphicInit(void)
{
    InitializeDebugOutput("MCIAVI");

    if (!GraphicWindowInit())
	return FALSE;

    swCommandTable = mciLoadCommandResource(ghModule, TEXT("mciavi"), 0);

    return TRUE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicDrvOpen | This function is called when the DriverProc
 *      gets a DRV_OPEN message. This happens each time that a new movie
 *      is opened thru MCI.
 *
 * @parm LPMCI_OPEN_DRIVER_PARMS | lpOpen | Far pointer to the standard
 *      MCI open parameters
 *
 * @rdesc Returns the mci device id. The installable driver interface will
 *      pass this ID to the DriverProc in the dwDriverID parameter on all
 *      subsequent messages. To fail the open, return 0L.
 *
 ***************************************************************************/

DWORD PASCAL GraphicDrvOpen(LPMCI_OPEN_DRIVER_PARMS lpOpen)
{
    /* Specify the custom command table and the device type  */

    lpOpen->wCustomCommandTable = swCommandTable;
    lpOpen->wType = MCI_DEVTYPE_DIGITAL_VIDEO;

    /* Set the device ID to the MCI Device ID */

    return (DWORD) (UINT)lpOpen->wDeviceID;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void | GraphicFree | This function is called when the DriverProc
 *      gets a DRV_FREE message. This happens when the drivers open count
 *      reaches 0.
 *
 ***************************************************************************/

void PASCAL GraphicFree(void)
{
    if (swCommandTable != -1) {
	    mciFreeCommandResource(swCommandTable);
	    swCommandTable = -1;
    }

#ifdef WIN32
    /*
     * unregister class so we can re-register it next time we are loaded
     */
    GraphicWindowFree();
#endif
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicDelayedNotify | This is a utility function that
 *      sends a notification saved with GraphicSaveCallback to mmsystem
 *      which posts a message to the application.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data.
 *
 * @parm UINT | wStatus | The type of notification to use can be one of
 *      MCI_NOTIFY_SUCCESSFUL, MCI_NOTIFY_SUPERSEDED, MCI_NOTIFY_ABORTED
 *      or MCI_NOTIFY_FAILURE (see MCI ispec.)
 *
 ***************************************************************************/

void FAR PASCAL GraphicDelayedNotify(NPMCIGRAPHIC npMCI, UINT wStatus)
{
    /* Send any saved notification */

    if (npMCI->hCallback) {

	// If the system menu is the only thing keeping us from closing, bring
	// it down and then close.
	if (gfEvilSysMenu)
	    SendMessage(npMCI->hwnd, WM_CANCELMODE, 0, 0);

	// If a dialog box is up, and keeping us from closing, we can't send the
	// notify or it will close us.
	if (!gfEvil)
    	    mciDriverNotify(npMCI->hCallback, npMCI->wDevID, wStatus);

    	npMCI->hCallback = NULL;
    }
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicImmediateNotify | This is a utility function that
 *      sends a successful notification message to mmsystem if the
 *      notification flag is set and the error field is 0.
 *
 * @parm UINT | wDevID | device ID.
 *
 * @parm LPMCI_GENERIC_PARMS | lpParms | Far pointer to an MCI parameter
 *      block. The first field of every MCI parameter block is the
 *      callback handle.
 *
 * @parm DWORD | dwFlags | Parm. block flags - used to check whether the
 *      callback handle is valid.
 *
 * @parm DWORD | dwErr | Notification only occurs if the command is not
 *      returning an error.
 *
 ***************************************************************************/

void FAR PASCAL GraphicImmediateNotify(UINT wDevID,
    LPMCI_GENERIC_PARMS lpParms,
    DWORD dwFlags, DWORD dwErr)
{
    if (!LOWORD(dwErr) && (dwFlags & MCI_NOTIFY)) {
	//Don't have an npMCI - see GraphicDelayedNotify
	//if (gfEvil)
	    //SendMessage(npMCI->hwnd, WM_CANCELMODE, 0, 0);

	// If a dialog box is up, and keeping us from closing, we can't send the
	// notify or it will close us.
	if (!gfEvil) // !!! EVIL !!!
            mciDriverNotify((HANDLE) (UINT)lpParms->dwCallback,
					wDevID, MCI_NOTIFY_SUCCESSFUL);
    }
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicSaveCallback | This is a utility function that saves
 *      a new callback in the instance data block.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data.
 *
 * @parm HANDLE | hCallback | callback handle
 *
 ***************************************************************************/

void NEAR PASCAL GraphicSaveCallback (NPMCIGRAPHIC npMCI, HANDLE hCallback)
{
    /* If there's an old callback, kill it. */
    GraphicDelayedNotify(npMCI, MCI_NOTIFY_SUPERSEDED);

    /* Save new notification callback window handle */
    npMCI->hCallback = hCallback;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicClose | This function closes the movie and
 *  releases the instance data.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD PASCAL GraphicClose (NPMCIGRAPHIC npMCI)
{
    DWORD dwRet = 0L;
    NPMCIGRAPHIC p;

    if (npMCI) {
	
        dwRet = DeviceClose (npMCI);
        Assert(dwRet == 0);

	// If the system menu is the only thing keeping us from closing, bring
	// it down and then close.
	if (gfEvilSysMenu)
	    SendMessage(npMCI->hwnd, WM_CANCELMODE, 0, 0);

      	if (gfEvil) {
	    DPF(("************************************************\n"));
	    DPF(("** EVIL: Failing the close because we'd die   **\n"));
            DPF(("************************************************\n"));
            LeaveCrit(npMCI);
	    return MCIERR_DRIVER_INTERNAL;
        }
	
        /* If the default window still exists, close and destroy it. */
        if (IsWindow(npMCI->hwndDefault)) {
	    if (!DestroyWindow(npMCI->hwndDefault))
		dwRet = MCIERR_DRIVER_INTERNAL;
	}

	if (npMCI->szFilename) {
	    LocalFree((HANDLE) (npMCI->szFilename));
	}

        //
        // find this instance on the list
        //
        if (npMCI == npMCIList) {
            npMCIList = npMCI->npMCINext;
        }
        else {
            for (p=npMCIList; p && p->npMCINext != npMCI; p=p->npMCINext)
                ;

            Assert(p && p->npMCINext == npMCI);

            p->npMCINext = npMCI->npMCINext;
        }

#ifdef WIN32
	// Delete the critical section object
	LeaveCriticalSection(&npMCI->CritSec);
	DeleteCriticalSection(&npMCI->CritSec);
#endif


	/* Free the instance data block allocated in GraphicOpen */

	LocalFree((HANDLE)npMCI);
    }
	
    return dwRet;
}

DWORD NEAR PASCAL FixFileName(NPMCIGRAPHIC npMCI, LPCTSTR lpName)
{
    TCHAR	ach[256];

    ach[(sizeof(ach)/sizeof(TCHAR)) - 1] = TEXT('\0');

#ifndef WIN32
    _fstrncpy(ach, (LPTSTR) lpName, (sizeof(ach)/sizeof(TCHAR)) - (1*sizeof(TCHAR)));
#else
    wcsncpy(ach, (LPTSTR) lpName, (sizeof(ach)/sizeof(TCHAR)) - (1*sizeof(TCHAR)));
#endif

    //
    // treat any string that starts with a '@' as valid and pass it to the
    // device any way.
    //
    if (ach[0] != '@')
    {
    if (!mmioOpen(ach, NULL, MMIO_PARSE))
	return MCIERR_FILENAME_REQUIRED;	
    }

    npMCI->szFilename = (NPTSTR) LocalAlloc(LPTR,
				    sizeof(TCHAR) * (lstrlen(ach) + 1));

    if (!npMCI->szFilename) {
	return MCIERR_OUT_OF_MEMORY;
    }

    lstrcpy(npMCI->szFilename, ach);

    return 0L;
}

/**************************************************************************

***************************************************************************/

#define SLASH(c)     ((c) == TEXT('/') || (c) == TEXT('\\'))
//#define SLASH(c)     ((c) == '/' || (c) == '\\')  // Filename Ascii??

STATICFN LPCTSTR FAR FileName(LPCTSTR szPath)
{
    LPCTSTR   sz;

    sz = &szPath[lstrlen(szPath)];
    for (; sz>szPath && !SLASH(*sz) && *sz!=TEXT(':');)
        sz = CharPrev(szPath, sz);
    return (sz>szPath ? sz + 1 : sz);
}

/****************************************************************************
****************************************************************************/

STATICFN DWORD NEAR PASCAL GetMSVideoVersion()
{
    HANDLE h;

    extern DWORD FAR PASCAL VideoForWindowsVersion(void);

    //
    // don't call VideoForWindowsVersion() if it does not exist or KERNEL
    // will kill us with a undef dynalink error.
    //
    if ((h = GetModuleHandle(szMSVideo)) && GetProcAddress(h, (LPSTR) MAKEINTATOM(2)))
        return VideoForWindowsVersion();
    else
        return 0;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicOpen | This function opens a movie file,
 *      initializes an instance data block, and creates the default
 *      stage window.
 *
 * @parm NPMCIGRAPHIC FAR * | lpnpMCI | Far pointer to a near pointer
 *      to instance data block to be filled in by this function.
 *
 * @parm DWORD | dwFlags | Flags for the open message.
 *
 * @parm LPMCI_DGV_OPEN_PARMS | Parameters for the open message.
 *
 * @parm UINT | wDeviceID | The MCI Device ID for this instance.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD PASCAL GraphicOpen (NPMCIGRAPHIC FAR * lpnpMCI, DWORD dwFlags,
    LPMCI_DGV_OPEN_PARMS lpOpen, UINT wDeviceID)
{
    NPMCIGRAPHIC npMCI;
    DWORD	dwStyle;
    HWND	hWnd;
    HWND	hWndParent;
    DWORD	dwRet;

    if (dwFlags & MCI_OPEN_SHAREABLE) {

        if (lpOpen->lpstrElementName == NULL ||
            lpOpen->lpstrElementName[0] != '@') {
            return MCIERR_UNSUPPORTED_FUNCTION;
        }
    }

    //
    //  check the verion of MSVIDEO.DLL before going any further
    //  if we run a "new" version of MCIAVI on a old MSVIDEO.DLL
    //  then bad things will happen.  We assume all MSVIDEO.DLLs
    //  will be backward compatible so we check for any version
    //  greater than the expected version.
    //

    DPF(("Video For Windows Version %d.%02d.%02d.%02d\n", HIBYTE(HIWORD(GetMSVideoVersion())), LOBYTE(HIWORD(GetMSVideoVersion())), HIBYTE(LOWORD(GetMSVideoVersion())), LOBYTE(LOWORD(GetMSVideoVersion())) ));

    if (GetMSVideoVersion() < MSVIDEO_VERSION)
    {
        TCHAR achError[128];
        TCHAR ach[40];

        LoadString(ghModule, MCIAVI_BADMSVIDEOVERSION, achError, sizeof(achError)/sizeof(TCHAR));
        LoadString(ghModule, MCIAVI_PRODUCTNAME, ach, sizeof(ach)/sizeof(TCHAR));
	MessageBox(NULL,achError,ach,
#ifdef BIDI
		MB_RTL_READING |
#endif
	MB_OK|MB_SYSTEMMODAL|MB_ICONEXCLAMATION);

        return MCIERR_DRIVER_INTERNAL;
    }

#ifndef WIN32
#pragma message("Support passing in MMIOHANDLEs with OPEN_ELEMENT_ID?")
#endif

    if (lpOpen->lpstrElementName == NULL) {
	// they're doing an "open new".

	// !!! handle this, probably by not actually reading a file.
	// ack.
    }

    /* Be sure we have a real, non-empty filename, not an id. */
    if ((!(dwFlags & MCI_OPEN_ELEMENT))
	    || (lpOpen->lpstrElementName == NULL)
	    || (*(lpOpen->lpstrElementName) == '\0'))
        return MCIERR_UNSUPPORTED_FUNCTION;

    // Allocate an instance data block. Code ASSUMES Zero Init.

    if (!(npMCI = (NPMCIGRAPHIC) LocalAlloc(LPTR, sizeof (MCIGRAPHIC))))
        return MCIERR_OUT_OF_MEMORY;

#ifdef WIN32
    // init the per-device critsec
    InitializeCriticalSection(&npMCI->CritSec);
    npMCI->lCritRefCount = 0;

#endif

    // now hold the critical section for the rest of the open
    EnterCrit(npMCI);
#ifdef DEBUG
    npMCI->mciid = MCIID;
#endif

    //
    // add this device to our list
    //
    npMCI->npMCINext = npMCIList;
    npMCIList = npMCI;

    npMCI->wMessageCurrent = MCI_OPEN;

    // Allocate some space for the filename
    // Copy the filename into the data block
    dwRet = FixFileName(npMCI, lpOpen->lpstrElementName);
    if (dwRet != 0L) {
	// note we hold the critical section into GraphicClose, and
	// release it just before deleting it within GraphicClose. This is
	// because we need to hold it during the call to DeviceClose.
	GraphicClose(npMCI);
	return dwRet;
    }
	
    // Create the default window - the caller may
    // supply style and parent window.

    if (dwFlags & MCI_DGV_OPEN_PARENT)
        hWndParent = lpOpen->hWndParent;
    else
        hWndParent = NULL;

    if (dwFlags & MCI_DGV_OPEN_WS) {
        // CW_USEDEFAULT can't be used with popups or children so
        // if the user provides the style, default to full-screen.

        dwStyle = lpOpen->dwStyle;
	hWnd =

        // Note:  The CreateWindow/Ex call is written this way as on Win32
        // CreateWindow is a MACRO, and hence the call must be contained
        // within the preprocessor block.
#ifdef BIDI
	CreateWindowEx(WS_EX_BIDI_SCROLL |  WS_EX_BIDI_MENU |WS_EX_BIDI_NOICON,
            szClassName,
            FileName(npMCI->szFilename),
            dwStyle,
            0, 0,
            GetSystemMetrics (SM_CXSCREEN),
            GetSystemMetrics (SM_CYSCREEN),
            hWndParent,
            NULL, ghModule, (LPTSTR)npMCI);
#else
	CreateWindow(
            szClassName,
            FileName(npMCI->szFilename),
            dwStyle,
            0, 0,
            GetSystemMetrics (SM_CXSCREEN),
            GetSystemMetrics (SM_CYSCREEN),
            hWndParent,
            NULL, ghModule, (LPTSTR)npMCI);
#endif
    } else {
        dwStyle = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX |
                  WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        if (GetProfileInt("mciavi", "CreateVisible", 0)) {
            DPF0(("Creating the window visible\n"));
            dwStyle |= WS_VISIBLE;
        } else {
            DPF0(("Creating the window INvisible\n"));
        }
	hWnd =
#ifdef BIDI
	CreateWindowEx(WS_EX_BIDI_SCROLL |  WS_EX_BIDI_MENU |WS_EX_BIDI_NOICON,
	    szClassName,
	    FileName(npMCI->szFilename),
            dwStyle,
	    CW_USEDEFAULT, 0,
	    CW_USEDEFAULT, 0,
            hWndParent,
            NULL, ghModule, (LPTSTR)npMCI);
#else
	CreateWindow(
	    szClassName,
	    FileName(npMCI->szFilename),
            dwStyle,
	    CW_USEDEFAULT, 0,
	    CW_USEDEFAULT, 0,
            hWndParent,
            NULL, ghModule, (LPTSTR)npMCI);
#endif
    }

    if (!hWnd) {
	// see above - we release and delete the critsec within GraphicClose
        DPF0(("Failed to create the window\n"));
        GraphicClose(npMCI);
        return MCIERR_CREATEWINDOW;
    }

    /* Fill in some more of the instance data.
    ** The rest of the fields are filled in in DeviceOpen.
    */

    npMCI->hCallingTask = GetCurrentTask();
    npMCI->hCallback = NULL;
    npMCI->wDevID = wDeviceID;
    npMCI->hwndDefault = hWnd;
    npMCI->hwnd = hWnd;
    npMCI->dwTimeFormat = MCI_FORMAT_FRAMES;
    npMCI->dwSpeedFactor = 1000;
    npMCI->dwVolume = MAKELONG(500, 500);
    npMCI->lTo = 0L;
    npMCI->dwFlags = MCIAVI_PLAYAUDIO | MCIAVI_SHOWVIDEO;
    npMCI->dwOptionFlags = ReadConfigInfo() | MCIAVIO_STRETCHTOWINDOW;

    // perform device-specific initialization

    dwRet = DeviceOpen(npMCI, dwFlags);

    if (dwRet != 0) {
	// see above - we release and delete the critsec within GraphicClose
        GraphicClose(npMCI);
	return dwRet;
    }

    *lpnpMCI = npMCI;

    npMCI->wMessageCurrent = 0;

    LeaveCrit(npMCI);

    return 0L;
}


/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicLoad | This function supports the MCI_LOAD command.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the Load message.
 *
 * @parm LPMCI_DGV_LOAD_PARMS | lpLoad | Parameters for the LOAD message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/
DWORD NEAR PASCAL GraphicLoad(NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_LOAD_PARMS lpLoad)
{
#ifndef LOADACTUALLYWORKS
    return MCIERR_UNSUPPORTED_FUNCTION;
#else
    DWORD dw;

    if (!(dwFlags & MCI_LOAD_FILE))
	return MCIERR_MISSING_PARAMETER;

    dw = FixFileName(npMCI, lpLoad->lpfilename);

    if (dw)
	return dw;

    dw = DeviceLoad(npMCI);

    return dw;
#endif
}


/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicSeek | This function sets the current frame. The
 *      device state after a seek is MCI_MODE_PAUSE
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the seek message.
 *
 * @parm LPMCI_DGV_SEEK_PARMS | lpSeek | Parameters for the seek message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicSeek (NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_SEEK_PARMS lpSeek)
{
    LONG lTo;
    BOOL fTest = FALSE;
    /* Do some range checking then pass onto the device-specific routine. */

    if (dwFlags & MCI_TEST) {
	dwFlags &= ~(MCI_TEST);
	fTest = TRUE;
    }

    switch (dwFlags & (~(MCI_WAIT | MCI_NOTIFY))) {
        case MCI_TO:
	    lTo = ConvertToFrames(npMCI, lpSeek->dwTo);
            break;

        case MCI_SEEK_TO_START:
            lTo = 0;
            break;

        case MCI_SEEK_TO_END:
            lTo = npMCI->lFrames;
            break;

	case 0:
	    return MCIERR_MISSING_PARAMETER;

	default:
            if (dwFlags & ~(MCI_TO |
			    MCI_SEEK_TO_START |
			    MCI_SEEK_TO_END |
			    MCI_WAIT |
			    MCI_NOTIFY))
                return MCIERR_UNRECOGNIZED_KEYWORD;
	    else
                return MCIERR_FLAGS_NOT_COMPATIBLE;
	    break;
    }

    if (!IsWindow(npMCI->hwnd))
        return MCIERR_NO_WINDOW;

    if (lTo < 0 || lTo > npMCI->lFrames)
    	return MCIERR_OUTOFRANGE;

    if (fTest)
	return 0L;

    GraphicDelayedNotify (npMCI, MCI_NOTIFY_ABORTED);

    if (dwFlags & MCI_NOTIFY) {
        GraphicSaveCallback(npMCI, (HANDLE) (UINT)lpSeek->dwCallback);
    }

    /* Clear the 'repeat' flags */
    npMCI->dwFlags &= ~(MCIAVI_REPEATING);

    return DeviceSeek(npMCI, lTo, dwFlags);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicCue | This function gets the movie ready to play,
 *	but leaves it paused.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the cue message.
 *
 * @parm LPMCI_DGV_CUE_PARMS | lpCue | Parameters for the cue message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicCue(NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_CUE_PARMS lpCue)
{
    LONG		lTo;
    DWORD		dwRet = 0L;

    if (dwFlags & MCI_DGV_CUE_INPUT)
        return MCIERR_UNSUPPORTED_FUNCTION;

    if (dwFlags & MCI_DGV_CUE_NOSHOW)
        return MCIERR_UNSUPPORTED_FUNCTION;

    if (dwFlags & MCI_TO) {
	lTo = ConvertToFrames(npMCI, lpCue->dwTo);

	if (lTo < 0L || lTo > npMCI->lFrames)
            return MCIERR_OUTOFRANGE;
    }

    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST)
	return 0L;

    GraphicDelayedNotify(npMCI, MCI_NOTIFY_ABORTED);

    if (dwFlags & MCI_NOTIFY) {
        GraphicSaveCallback(npMCI, (HANDLE) (UINT)lpCue->dwCallback);
    }

    /* Clear the 'repeat' flags */
    npMCI->dwFlags &= ~(MCIAVI_REPEATING);

    /* Set up to play to end of file */
    npMCI->lTo = npMCI->lFrames;

    dwRet = DeviceCue(npMCI, lTo, dwFlags);

    return dwRet;
}

#ifndef WIN32
#ifdef EXPIRE
//
// return the current date....
//
//       dx = year
//       ah = month
//       al = day
//
#pragma optimize("", off)
DWORD DosGetDate(void)
{
    if (0)
	return 0;

    _asm {
        mov     ah,2ah
        int     21h
        mov     ax,dx
        mov     dx,cx
    }
}
#pragma optimize("", on)
#endif
#endif

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicPlay | This function starts playback of the movie. If
 *      the reverse flag is specified, the movie plays backwards. If the fast
 *      or slow flags are specified the movie plays faster or slower.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the play message.
 *
 * @parm LPMCI_DGV_PLAY_PARMS | lpPlay | Parameters for the play message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicPlay (NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_PLAY_PARMS lpPlay )
{
    LONG		lTo, lFrom;
    DWORD		dwRet;
#ifdef EXPIRE
#pragma message("Remove the expiration code after Beta ships")
    if (DosGetDate() >= EXPIRE)
    {
        return MCIERR_AVI_EXPIRED;
    }
#endif

    if (!(dwFlags & (MCI_MCIAVI_PLAY_FULLSCREEN | MCI_MCIAVI_PLAY_WINDOW)) &&
		    (npMCI->dwOptionFlags & MCIAVIO_USEVGABYDEFAULT)) {
	if (npMCI->dwOptionFlags & MCIAVIO_ZOOMBY2)
	    dwFlags |= MCI_MCIAVI_PLAY_FULLBY2;
	else
	    dwFlags |= MCI_MCIAVI_PLAY_FULLSCREEN;
    }


    if (dwFlags & (MCI_MCIAVI_PLAY_FULLSCREEN | MCI_MCIAVI_PLAY_FULLBY2)) {
#if 0
        if (ghDISPDIB == NULL) {
            UINT w;

            w = SetErrorMode(SEM_NOOPENFILEERRORBOX);

#ifndef WIN32
            if ((ghDISPDIB = LoadLibrary(szDisplayDibLib)) > HINSTANCE_ERROR)
#else
            if ((ghDISPDIB = LoadLibrary(szDisplayDibLib)) != NULL)
#endif
            {
                (FARPROC)DisplayDibProc = GetProcAddress(ghDISPDIB, szDisplayDib);
                (FARPROC)DisplayDibExProc = GetProcAddress(ghDISPDIB, szDisplayDibEx);
            }
	    else
                ghDISPDIB = (HINSTANCE)-1;

            SetErrorMode(w);
	
            DPF(("ghDISPDIB=0x%04x, DisplayDibProc=0x%08lx\n", ghDISPDIB, DisplayDibProc));
	}
	if (DisplayDibProc == NULL)
	    return MCIERR_AVI_NODISPDIB;
#endif
    } else {
	if (!IsWindow(npMCI->hwnd))
	    return MCIERR_NO_WINDOW;
	
	npMCI->dwFlags |= MCIAVI_NEEDTOSHOW;
    }

    /* Range checks : 0 < 'from' <= 'to' <= last frame */

    if (dwFlags & MCI_TO) {
	lTo = ConvertToFrames(npMCI, lpPlay->dwTo);

        if (lTo < 0L || lTo > npMCI->lFrames)
            return MCIERR_OUTOFRANGE;
    } else if (dwFlags & MCI_DGV_PLAY_REVERSE)
	lTo = 0;
    else
        lTo = npMCI->lFrames;

    dwFlags |= MCI_TO;

    if (dwFlags & MCI_FROM) {
	lFrom = ConvertToFrames(npMCI, lpPlay->dwFrom);

        if (lFrom < 0L || lFrom > npMCI->lFrames)
	    return MCIERR_OUTOFRANGE;
    } else if (dwRet = DevicePosition(npMCI, &lFrom))
        return dwRet;

    /* check 'to' and 'from' relationship.  */
    if (lTo < lFrom)
	dwFlags |= MCI_DGV_PLAY_REVERSE;

    if ((lFrom < lTo) && (dwFlags & MCI_DGV_PLAY_REVERSE))
	return MCIERR_OUTOFRANGE;

    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST)
	return 0L;

    /* We want any previous playing to be aborted if and only if a 'from'
    ** parameter is specified.  If only a new 'to' parameter is specified,
    ** we can just change the 'to' value, and play will stop at the
    ** proper time.
    */

    /* set the 'to' position after we've stopped, if we're stopping. */

    if (dwFlags & MCI_FROM) {
	/* If MCI_FROM flag is specified then reset the starting location */
	DeviceStop(npMCI, MCI_WAIT);
	npMCI->lFrom = lFrom;
    } else {
	/* We still set the "From" variable, so that we'll correctly
	** play from the current position.
	** !!! Should this be done instead by updating lFrom to be the
	** current position when playback ends?
	*/
	npMCI->lFrom = lFrom;
    }

    /* If we're changing the "to" position, abort any pending notify. */
    if (lTo != npMCI->lTo) {
	GraphicDelayedNotify (npMCI, MCI_NOTIFY_ABORTED);
    }

	
    /* Don't set up notify until here, so that the seek won't make it happen*/
    if (dwFlags & MCI_NOTIFY) {
        GraphicSaveCallback(npMCI, (HANDLE) (UINT)lpPlay->dwCallback);
    }

    /* Set up the 'repeat' flags */
    npMCI->dwFlags &= ~(MCIAVI_REPEATING);

    if (dwFlags & MCI_DGV_PLAY_REPEAT) {
	/* If from position isn't given, repeat from either the beginning or
	** end of file as appropriate.
	*/
	npMCI->lRepeatFrom =
	    (dwFlags & MCI_FROM) ? lFrom :
		((dwFlags & MCI_DGV_PLAY_REVERSE) ? npMCI->lFrames : 0);
    }

    /* Go ahead and actually play... */
    return DevicePlay(npMCI, lTo, dwFlags);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIWAVE
 *
 * @api DWORD | GraphicStep | This function steps through several frames
 *      of a movie. If the reverse flag is set, then the step is backwards.
 *      If the step count is not specified then it defaults to 1. If the
 *      step count plus the current position exceeds the movie length, the
 *      step is out of range.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the step message.
 *
 * @parm LPMCI_DGV_STEP_PARMS | lpStep | Parameters for the step message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicStep (NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_STEP_PARMS lpStep)
{
    LONG	lFrameCur;
    LONG	lFrames;
    DWORD	dwRet;
    BOOL	fReverse;
    BOOL	fSeekExactOff;

    fReverse = (dwFlags & MCI_DGV_STEP_REVERSE) == MCI_DGV_STEP_REVERSE;

    // Default to 1 frame step if frame count is not specified

    if (dwFlags & MCI_DGV_STEP_FRAMES) {
	lFrames = (LONG) lpStep->dwFrames;
	
	if (fReverse) {
	    if (lFrames < 0)
		return MCIERR_FLAGS_NOT_COMPATIBLE;
	}
    } else
        lFrames = 1;


    lFrames = fReverse ? -lFrames : lFrames;

    /* stop before figuring out whether frame count is within range, */
    /* unless the TEST flag is set. */

    if (!(dwFlags & MCI_TEST)) {
	if (dwRet = DeviceStop(npMCI, MCI_WAIT))
	    return dwRet;
    }

    if (dwRet = DevicePosition(npMCI, &lFrameCur))
        return dwRet;

    if ((lFrames + lFrameCur > npMCI->lFrames) ||
		(lFrames + lFrameCur < 0))
        return MCIERR_OUTOFRANGE;

    if (!IsWindow(npMCI->hwnd))
        return MCIERR_NO_WINDOW;

    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST)
	return 0L;

    GraphicDelayedNotify (npMCI, MCI_NOTIFY_ABORTED);

    /* Clear the 'repeat' flags */
    npMCI->dwFlags &= ~(MCIAVI_REPEATING);


    if (dwFlags & MCI_NOTIFY) {
        GraphicSaveCallback(npMCI, (HANDLE) (UINT)lpStep->dwCallback);
    }

    fSeekExactOff = (npMCI->dwOptionFlags & MCIAVIO_SEEKEXACT) == 0;

    npMCI->dwOptionFlags |= MCIAVIO_SEEKEXACT;

    npMCI->dwFlags |= MCIAVI_NEEDTOSHOW;

    if (fSeekExactOff) {
	/* If we were not in seek exact mode, make seek finish
	** before we turn seek exact back off.
	*/
	dwRet = DeviceSeek(npMCI, lFrames + lFrameCur, dwFlags | MCI_WAIT);
	npMCI->dwOptionFlags &= ~(MCIAVIO_SEEKEXACT);
    } else
	dwRet = DeviceSeek(npMCI, lFrames + lFrameCur, dwFlags);

    return dwRet;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicStop | This function stops playback of the movie.
 *      After a stop the state will be MCI_MODE_STOP. The frame counter
 *	is not reset.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the stop message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicStop (NPMCIGRAPHIC npMCI, DWORD dwFlags,
					LPMCI_GENERIC_PARMS lpParms)
{
    if (!IsWindow(npMCI->hwnd))
        return MCIERR_NO_WINDOW;

    if (dwFlags & MCI_DGV_STOP_HOLD)
	return MCIERR_UNSUPPORTED_FUNCTION;

    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST)
	return 0L;

    GraphicDelayedNotify (npMCI, MCI_NOTIFY_ABORTED);

    /* Do we need to handle notify here? */
    return DeviceStop(npMCI, dwFlags);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicPause | Pauses movie playback.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicPause(NPMCIGRAPHIC npMCI, DWORD dwFlags,
					LPMCI_GENERIC_PARMS lpParms)
{
    if (!IsWindow(npMCI->hwnd))
        return MCIERR_NO_WINDOW;

    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST)
	return 0L;

    if (dwFlags & MCI_NOTIFY) {
        GraphicSaveCallback(npMCI, (HANDLE) (UINT)lpParms->dwCallback);
    }

    return DevicePause(npMCI, dwFlags);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicResume | This function resumes playback of a paused
 *      movie.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicResume (NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD	dwRet;
    UINT	wMode;

    //  Resume is only allowed if MCIAVI is paused or playing

    wMode = DeviceMode(npMCI);

    if (wMode != MCI_MODE_PAUSE && wMode != MCI_MODE_PLAY)
        return MCIERR_NONAPPLICABLE_FUNCTION;

    if (!IsWindow(npMCI->hwnd))
        return MCIERR_NO_WINDOW;

    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST)
	return 0L;

    if (dwFlags & MCI_NOTIFY) {
	GraphicSaveCallback(npMCI, (HANDLE) (UINT)lpParms->dwCallback);
    }

    dwRet = DeviceResume(npMCI, dwFlags & MCI_WAIT);

    return dwRet;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicStatus | This function returns numeric status info.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the status message.
 *
 * @parm LPMCI_STATUS_PARMS | lpPlay | Parameters for the status message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicStatus (NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_STATUS_PARMS lpStatus)
{
    DWORD dwRet = 0L;

    if (dwFlags & (MCI_DGV_STATUS_DISKSPACE))
	return MCIERR_UNSUPPORTED_FUNCTION;

    if (dwFlags & MCI_STATUS_ITEM) {

        lpStatus->dwReturn = 0L;

        if ((dwFlags & MCI_TRACK) &&
		!((lpStatus->dwItem == MCI_STATUS_POSITION) ||
			(lpStatus->dwItem == MCI_STATUS_LENGTH)))
            return MCIERR_FLAGS_NOT_COMPATIBLE;

        if ((dwFlags & MCI_STATUS_START) &&
			(lpStatus->dwItem != MCI_STATUS_POSITION))
            return MCIERR_FLAGS_NOT_COMPATIBLE;

        if (dwFlags & MCI_DGV_STATUS_REFERENCE)
            return MCIERR_FLAGS_NOT_COMPATIBLE;
	
        switch (lpStatus->dwItem) {
            case MCI_STATUS_POSITION:
	
                if (dwFlags & MCI_TRACK) {
                    /* POSITION with TRACK means return the start of the  */
                    /* track. */

                    if (lpStatus->dwTrack != 1)
	                dwRet = MCIERR_OUTOFRANGE;
	            else
                        /* return start frame of track (always 0) */
                        lpStatus->dwReturn = 0L;
		} else if (dwFlags & MCI_STATUS_START)
                    // POSITION with START means return the starting playable
                    // position of the media.
                    lpStatus->dwReturn = 0L;
                else {
		    /* Otherwise return current frame */
		    dwRet = DevicePosition(npMCI, (LPLONG) &lpStatus->dwReturn);
		    lpStatus->dwReturn = ConvertFromFrames(npMCI,
						(LONG) lpStatus->dwReturn);
		}
                break;

            case MCI_STATUS_LENGTH:


                if (dwFlags & MCI_TRACK && lpStatus->dwTrack != 1) {
                    /* LENGTH with TRACK means return the length of track */

                    lpStatus->dwReturn = 0L;
                    dwRet = MCIERR_OUTOFRANGE;
		}
		
		lpStatus->dwReturn = ConvertFromFrames(npMCI, npMCI->lFrames);
                break;

            case MCI_STATUS_NUMBER_OF_TRACKS:
            case MCI_STATUS_CURRENT_TRACK:

                lpStatus->dwReturn = 1L;
        	break;

            case MCI_STATUS_READY:

                /* Return TRUE if device can receive commands */
		if (DeviceMode(npMCI) != MCI_MODE_NOT_READY)
		    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
		else
		    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
        	dwRet = MCI_RESOURCE_RETURNED;
	        break;

            case MCI_STATUS_MODE:
	    {
		WORD	wMode;
                wMode = DeviceMode(npMCI);
		lpStatus->dwReturn = MAKEMCIRESOURCE(wMode, wMode);
                dwRet = MCI_RESOURCE_RETURNED;
	    }
                break;

	    case MCI_DGV_STATUS_PAUSE_MODE:
		if (DeviceMode(npMCI) != MCI_MODE_PAUSE)
		    dwRet = MCIERR_NONAPPLICABLE_FUNCTION;
		else {
		    lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_MODE_PLAY, MCI_MODE_PLAY);
		    dwRet = MCI_RESOURCE_RETURNED;
		}
		break;
		
            case MCI_STATUS_MEDIA_PRESENT:

                lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
        	dwRet = MCI_RESOURCE_RETURNED;
	        break;

            case MCI_DGV_STATUS_FORWARD:
		if (npMCI->dwFlags & MCIAVI_REVERSE)
		    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
		else
		    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
        	dwRet = MCI_RESOURCE_RETURNED;
	        break;

            case MCI_DGV_STATUS_HWND:
	
                lpStatus->dwReturn = (DWORD)(UINT)npMCI->hwnd;
                if (!IsWindow(npMCI->hwnd))
                    dwRet = MCIERR_NO_WINDOW;
	        break;

            case MCI_DGV_STATUS_HPAL:

//		lpStatus->dwReturn = (DWORD) (UINT) DrawDibGetPalette(npMCI->hdd);

		lpStatus->dwReturn = 0;
		
                if (npMCI->nVideoStreams == 0) {
		    dwRet = MCIERR_UNSUPPORTED_FUNCTION;
                } else {
		    dwRet = ICSendMessage(npMCI->hicDraw, ICM_DRAW_GET_PALETTE, 0, 0);
		
		    if (dwRet == ICERR_UNSUPPORTED) {
			dwRet = MCIERR_UNSUPPORTED_FUNCTION;
		    } else {
			lpStatus->dwReturn = dwRet;
			dwRet = 0;
		    }
		}
		DPF2(("Status HPAL returns: %lu\n", lpStatus->dwReturn));
                break;

            case MCI_STATUS_TIME_FORMAT:

                lpStatus->dwReturn = MAKEMCIRESOURCE(npMCI->dwTimeFormat,
				npMCI->dwTimeFormat + MCI_FORMAT_RETURN_BASE);
        	dwRet = MCI_RESOURCE_RETURNED;
	        break;
		
	    case MCI_DGV_STATUS_AUDIO:
                lpStatus->dwReturn = (npMCI->dwFlags & MCIAVI_PLAYAUDIO) ?
					(MAKEMCIRESOURCE(MCI_ON, MCI_ON_S)) :
					(MAKEMCIRESOURCE(MCI_OFF, MCI_OFF_S));
		dwRet = MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER;
		break;

	    case MCI_DGV_STATUS_WINDOW_VISIBLE:
		if (npMCI->hwnd && IsWindowVisible(npMCI->hwnd))
		    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
		else
		    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
        	dwRet = MCI_RESOURCE_RETURNED;
	        break;

	    case MCI_DGV_STATUS_WINDOW_MINIMIZED:
		if (npMCI->hwnd && IsIconic(npMCI->hwnd))
		    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
		else
		    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
        	dwRet = MCI_RESOURCE_RETURNED;
	        break;

	    case MCI_DGV_STATUS_WINDOW_MAXIMIZED:
		if (npMCI->hwnd && IsZoomed(npMCI->hwnd))
		    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
		else
		    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
        	dwRet = MCI_RESOURCE_RETURNED;
	        break;

	    case MCI_DGV_STATUS_SAMPLESPERSEC:
	    case MCI_DGV_STATUS_AVGBYTESPERSEC:
	    case MCI_DGV_STATUS_BLOCKALIGN:
	    case MCI_DGV_STATUS_BITSPERSAMPLE:
		dwRet = MCIERR_UNSUPPORTED_FUNCTION;
		break;
		
            case MCI_DGV_STATUS_BITSPERPEL:
                if (npMCI->psiVideo)
                    lpStatus->dwReturn = ((LPBITMAPINFOHEADER)npMCI->psiVideo->lpFormat)->biBitCount;
                else
                    dwRet = MCIERR_UNSUPPORTED_FUNCTION;
		break;
		
#ifndef WIN32
#pragma message("Are we going to support brightness/color/contrast/tint?")
#endif
	    case MCI_DGV_STATUS_BRIGHTNESS:
	    case MCI_DGV_STATUS_COLOR:
	    case MCI_DGV_STATUS_CONTRAST:
	    case MCI_DGV_STATUS_TINT:
	    case MCI_DGV_STATUS_GAMMA:
	    case MCI_DGV_STATUS_SHARPNESS:
	    case MCI_DGV_STATUS_FILE_MODE:
	    case MCI_DGV_STATUS_FILE_COMPLETION:
	    case MCI_DGV_STATUS_KEY_INDEX:
	    case MCI_DGV_STATUS_KEY_COLOR:
		dwRet = MCIERR_UNSUPPORTED_FUNCTION;
		break;
		
	    case MCI_DGV_STATUS_FILEFORMAT:
// Fall through to Unsupported case...
//                lpStatus->dwReturn = MAKEMCIRESOURCE(MCI_DGV_FF_AVI,
//						MCI_DGV_FF_AVI);
//		dwRet = MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER;
//		break;
//		
	    case MCI_DGV_STATUS_BASS:
	    case MCI_DGV_STATUS_TREBLE:
		dwRet = MCIERR_UNSUPPORTED_FUNCTION;
		break;
		
	    case MCI_DGV_STATUS_VOLUME:
	    {
		WORD	wLeftVolume, wRightVolume;
		// Be sure volume is up to date....
		DeviceGetVolume(npMCI);

		wLeftVolume = LOWORD(npMCI->dwVolume);
		wRightVolume = LOWORD(npMCI->dwVolume);

		switch (dwFlags & (MCI_DGV_STATUS_LEFT | MCI_DGV_STATUS_RIGHT)) {
		    case MCI_DGV_STATUS_LEFT:
			lpStatus->dwReturn = (DWORD) wLeftVolume;
		    break;
		
		    case 0:
			lpStatus->dwReturn = (DWORD) wRightVolume;
		    break;
		
		    default:
			lpStatus->dwReturn = ((DWORD) wLeftVolume + (DWORD) wRightVolume) / 2;
		    break;
		}
	    }
		break;

	    case MCI_DGV_STATUS_MONITOR:
                lpStatus->dwReturn = (DWORD)
				     MAKEMCIRESOURCE(MCI_DGV_MONITOR_FILE,
						MCI_DGV_FILE_S);
		dwRet = MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER;
		break;

	    case MCI_DGV_STATUS_SEEK_EXACTLY:
                lpStatus->dwReturn =
				(npMCI->dwOptionFlags & MCIAVIO_SEEKEXACT) ?
					(MAKEMCIRESOURCE(MCI_ON, MCI_ON_S)) :
					(MAKEMCIRESOURCE(MCI_OFF, MCI_OFF_S));
		dwRet = MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER;
		break;
		
	    case MCI_DGV_STATUS_SIZE:
		/* We haven't reserved any space, so return zero. */
		lpStatus->dwReturn = 0L;
		break;
		
	    case MCI_DGV_STATUS_SMPTE:
		dwRet = MCIERR_UNSUPPORTED_FUNCTION;
		break;

	    case MCI_DGV_STATUS_UNSAVED:
		lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
		dwRet = MCI_RESOURCE_RETURNED;
		break;
		
	    case MCI_DGV_STATUS_VIDEO:
                lpStatus->dwReturn = (npMCI->dwFlags & MCIAVI_SHOWVIDEO) ?
					(MAKEMCIRESOURCE(MCI_ON, MCI_ON_S)) :
					(MAKEMCIRESOURCE(MCI_OFF, MCI_OFF_S));
        	dwRet = MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER;
		break;
		
	    case MCI_DGV_STATUS_SPEED:
		lpStatus->dwReturn = npMCI->dwSpeedFactor;
		break;
		
	    case MCI_DGV_STATUS_FRAME_RATE:
	    {
		DWORD	dwTemp;

		dwTemp = npMCI->dwMicroSecPerFrame;
		
		/* If they haven't specifically asked for the "nominal"
		** rate of play, adjust by the current speed.
		*/
		if (!(dwFlags & MCI_DGV_STATUS_NOMINAL))
		    dwTemp = muldiv32(dwTemp, 1000L, npMCI->dwSpeedFactor);
		
		if (dwTemp == 0)
		    lpStatus->dwReturn = 1000;
		else
		    /* Our return value is in "thousandths of frames/sec",
		    ** and dwTemp is the number of microseconds per frame.
		    ** Thus, we divide a billion microseconds by dwTemp.
		    */
		    lpStatus->dwReturn = muldiv32(1000000L, 1000L, dwTemp);
		break;
	    }

	    case MCI_DGV_STATUS_AUDIO_STREAM:
		lpStatus->dwReturn = 0;
                if (npMCI->nAudioStreams) {
		    int	stream;

		    for (stream = 0; stream < npMCI->streams; stream++) {
			if (SH(stream).fccType == streamtypeAUDIO)
			    ++lpStatus->dwReturn;

			if (stream == npMCI->nAudioStream)
			    break;
		    }
		}
		break;
		
	    case MCI_DGV_STATUS_VIDEO_STREAM:
	    case MCI_DGV_STATUS_AUDIO_INPUT:
	    case MCI_DGV_STATUS_AUDIO_RECORD:
	    case MCI_DGV_STATUS_AUDIO_SOURCE:
	    case MCI_DGV_STATUS_VIDEO_RECORD:
	    case MCI_DGV_STATUS_VIDEO_SOURCE:
	    case MCI_DGV_STATUS_VIDEO_SRC_NUM:
	    case MCI_DGV_STATUS_MONITOR_METHOD:
	    case MCI_DGV_STATUS_STILL_FILEFORMAT:
		dwRet = MCIERR_UNSUPPORTED_FUNCTION;
		break;
		
	    case MCI_AVI_STATUS_FRAMES_SKIPPED:
                lpStatus->dwReturn = npMCI->lSkippedFrames;
		break;
		
	    case MCI_AVI_STATUS_AUDIO_BREAKS:
                lpStatus->dwReturn = npMCI->lAudioBreaks;
		break;
		
	    case MCI_AVI_STATUS_LAST_PLAY_SPEED:
		lpStatus->dwReturn = npMCI->dwSpeedPercentage;
		break;
		
            default:
                dwRet = MCIERR_UNSUPPORTED_FUNCTION;
        	break;
        } /* end switch (item) */
    } else if (dwFlags & MCI_DGV_STATUS_REFERENCE) {

	if (lpStatus->dwReference > (DWORD) npMCI->lFrames)
            dwRet = MCIERR_OUTOFRANGE;

        else if (npMCI->psiVideo) {
            lpStatus->dwReference = MovieToStream(npMCI->psiVideo,
                    lpStatus->dwReference);

            lpStatus->dwReturn = FindPrevKeyFrame(npMCI, npMCI->psiVideo,
                    lpStatus->dwReference);

            lpStatus->dwReturn = StreamToMovie(npMCI->psiVideo,
                    lpStatus->dwReturn);
        }
        else {
            lpStatus->dwReturn = 0;
        }
    } else /* item flag not set */
        dwRet = MCIERR_MISSING_PARAMETER;

    if ((dwFlags & MCI_TEST) && (LOWORD(dwRet) == 0)) {
	/* There is no error, but the test flag is on.  Return as little
	** as possible.
	*/
	dwRet = 0;
	lpStatus->dwReturn = 0;
    }

    return dwRet;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicInfo | This function returns alphanumeric information.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the info. message.
 *
 * @parm LPMCI_INFO_PARMS | lpPlay | Parameters for the info message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicInfo(NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_INFO_PARMS lpInfo)
{
    DWORD	dwRet = 0L;
    TCHAR	ch = TEXT('\0');
    BOOL	fTest = FALSE;

    if (!lpInfo->lpstrReturn)
    	return MCIERR_PARAM_OVERFLOW;

    if (dwFlags & MCI_TEST)
	fTest = TRUE;

    dwFlags &= ~(MCI_WAIT | MCI_NOTIFY | MCI_TEST);

    switch (dwFlags) {
    case 0L:
	return MCIERR_MISSING_PARAMETER;
	
    case MCI_INFO_FILE:
	if (!npMCI)
	    return MCIERR_UNSUPPORTED_FUNCTION;
	
	if (lpInfo->dwRetSize < (DWORD)(lstrlen(npMCI->szFilename) + 1)) {
            ch = npMCI->szFilename[lpInfo->dwRetSize];
            npMCI->szFilename[lpInfo->dwRetSize] = '\0';
            dwRet = MCIERR_PARAM_OVERFLOW;
	}
        lstrcpy (lpInfo->lpstrReturn, npMCI->szFilename);
        if (ch)
            npMCI->szFilename[lpInfo->dwRetSize] = ch;
	break;
	
    case MCI_INFO_PRODUCT:

#ifdef DEBUG
        #include "..\verinfo\usa\verinfo.h"

        wsprintf(lpInfo->lpstrReturn,
            TEXT("VfW %d.%02d.%02d"), MMVERSION, MMREVISION, MMRELEASE);
#else
        /* !!! Not returning PARAM_OVERFLOW here but I am above - lazy eh */
        LoadString(ghModule, MCIAVI_PRODUCTNAME, lpInfo->lpstrReturn,
                (UINT)lpInfo->dwRetSize);
#endif
	break;

    case MCI_DGV_INFO_TEXT:
	if (!npMCI)
	    return MCIERR_UNSUPPORTED_FUNCTION;
	
     	if (IsWindow(npMCI->hwnd))
            GetWindowText(npMCI->hwnd, lpInfo->lpstrReturn,
					LOWORD(lpInfo->dwRetSize));
        else
            dwRet = MCIERR_NO_WINDOW;
	break;

    case MCI_INFO_VERSION:
	/* !!! Not returning PARAM_OVERFLOW here but I am above - lazy eh */
	LoadString(ghModule, MCIAVI_VERSION, lpInfo->lpstrReturn,
		(UINT)lpInfo->dwRetSize);
	break;

	case MCI_DGV_INFO_USAGE:
	    dwRet = MCIERR_UNSUPPORTED_FUNCTION;
	    break;

    case MCI_DGV_INFO_ITEM:
	switch (lpInfo->dwItem) {	
	case MCI_DGV_INFO_AUDIO_QUALITY:
	case MCI_DGV_INFO_VIDEO_QUALITY:
	case MCI_DGV_INFO_STILL_QUALITY:
	case MCI_DGV_INFO_AUDIO_ALG:
	case MCI_DGV_INFO_VIDEO_ALG:
	case MCI_DGV_INFO_STILL_ALG:
	default:
	    dwRet = MCIERR_UNSUPPORTED_FUNCTION;
	    break;
	}
	break;

    default:
    	dwRet = MCIERR_FLAGS_NOT_COMPATIBLE;
	break;
    }

    if (fTest && (LOWORD(dwRet) == 0)) {
	/* There is no error, but the test flag is on.  Return as little
	** as possible.
	*/
	dwRet = 0;
	if (lpInfo->dwRetSize)
	    lpInfo->lpstrReturn[0] = '\0';
    }

    return dwRet;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicSet | This function sets various options.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the set message.
 *
 * @parm LPMCI_SET_PARMS | lpSet | Parameters for the set message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicSet (NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_SET_PARMS lpSet)
{
    DWORD	dwRet = 0L;
    DWORD	dwAction;

    if (dwFlags & MCI_DGV_SET_FILEFORMAT)
        return MCIERR_UNSUPPORTED_FUNCTION;

    if (dwFlags & MCI_DGV_SET_STILL)
        return MCIERR_UNSUPPORTED_FUNCTION;

    dwAction = dwFlags & (MCI_SET_TIME_FORMAT		|
                         MCI_SET_VIDEO			|
                         MCI_SET_AUDIO			|
			 MCI_DGV_SET_SEEK_EXACTLY	|
			 MCI_DGV_SET_SPEED
			     );
    dwFlags &= 	(MCI_SET_ON				|
                         MCI_SET_OFF			|
			 MCI_TEST
			     );

    /* First, check if the parameters are all okay */

    if (!dwAction)
        return MCIERR_UNSUPPORTED_FUNCTION;

    if (dwAction & MCI_SET_TIME_FORMAT) {
        if (lpSet->dwTimeFormat != MCI_FORMAT_FRAMES
		&& lpSet->dwTimeFormat != MCI_FORMAT_MILLISECONDS)
            return MCIERR_UNSUPPORTED_FUNCTION;
    }

    if ((dwAction & MCI_SET_AUDIO) &&
		(lpSet->dwAudio != MCI_SET_AUDIO_ALL)) {
        return MCIERR_UNSUPPORTED_FUNCTION;
    }

    if (dwAction & MCI_DGV_SET_SPEED) {
	if (lpSet->dwSpeed > 100000L)
	    return MCIERR_OUTOFRANGE;
    }

    switch (dwFlags & (MCI_SET_ON | MCI_SET_OFF)) {
	case 0:
	    if (dwAction & (MCI_SET_AUDIO |
				MCI_SET_VIDEO |
				MCI_DGV_SET_SEEK_EXACTLY))
		return MCIERR_MISSING_PARAMETER;
	    break;

	case MCI_SET_ON | MCI_SET_OFF:
	    return MCIERR_FLAGS_NOT_COMPATIBLE;

	default:
	    if (dwAction & (MCI_DGV_SET_SPEED | MCI_SET_TIME_FORMAT))
		return MCIERR_FLAGS_NOT_COMPATIBLE;
	    break;
    }

    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST)
	return 0L;

    /* Now, actually carry out the command */
    if (dwAction & MCI_SET_TIME_FORMAT)
	npMCI->dwTimeFormat = lpSet->dwTimeFormat;

    if (dwAction & MCI_SET_VIDEO) {
	npMCI->dwFlags &= ~(MCIAVI_SHOWVIDEO);
	if (dwFlags & MCI_SET_ON) {
	    npMCI->dwFlags |= MCIAVI_SHOWVIDEO;
	    InvalidateRect(npMCI->hwnd, NULL, FALSE);
	}
    }

    if (dwAction & MCI_DGV_SET_SEEK_EXACTLY) {
	npMCI->dwOptionFlags &= ~(MCIAVIO_SEEKEXACT);
	if (dwFlags & MCI_SET_ON)
	    npMCI->dwOptionFlags |= MCIAVIO_SEEKEXACT;
    }

    if (dwAction & MCI_DGV_SET_SPEED) {
	dwRet = DeviceSetSpeed(npMCI, lpSet->dwSpeed);
    }

    if (dwRet == 0L && (dwAction & MCI_SET_AUDIO)) {
	dwRet = DeviceMute(npMCI, dwFlags & MCI_SET_OFF ? TRUE : FALSE);
    }
	
    return dwRet;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicSetAudio | This function sets various audio options.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the set audio message.
 *
 * @parm LPMCI_SET_PARMS | lpSet | Parameters for the set audio message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/
DWORD NEAR PASCAL GraphicSetAudio (NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_SETAUDIO_PARMS lpSet)
{
    DWORD	dwRet = 0L;

    if (npMCI->nAudioStreams == 0) {
	return MCIERR_UNSUPPORTED_FUNCTION;
    }

    if ((dwFlags & MCI_DGV_SETAUDIO_ITEM) &&
	    (lpSet->dwItem == MCI_DGV_SETAUDIO_VOLUME) &&
	    (dwFlags & MCI_DGV_SETAUDIO_VALUE)) {
	WORD	wLeft, wRight;
	
	if (dwFlags & (MCI_DGV_SETAUDIO_ALG |
        	   MCI_DGV_SETAUDIO_QUALITY |
        	   MCI_DGV_SETAUDIO_RECORD |
        	   MCI_DGV_SETAUDIO_CLOCKTIME))
	    return MCIERR_UNSUPPORTED_FUNCTION;
        if (lpSet->dwValue > 1000L)
	    return MCIERR_OUTOFRANGE;
    	if (dwFlags & MCI_TEST)
	    return 0L;

	// Be sure volume is up to date....
	DeviceGetVolume(npMCI);
	
	wLeft = LOWORD(npMCI->dwVolume);
	wRight = HIWORD(npMCI->dwVolume);
	if (!(dwFlags & MCI_DGV_SETAUDIO_RIGHT))
	    wLeft = (WORD) lpSet->dwValue;
	
	if (!(dwFlags & MCI_DGV_SETAUDIO_LEFT))
	    wRight = (WORD) lpSet->dwValue;
	
	dwRet = DeviceSetVolume(npMCI, MAKELONG(wLeft, wRight));
    } else if ((dwFlags & MCI_DGV_SETAUDIO_ITEM) &&
	    (lpSet->dwItem == MCI_DGV_SETAUDIO_STREAM) &&
	    (dwFlags & MCI_DGV_SETAUDIO_VALUE)) {
	if (dwFlags & (MCI_DGV_SETAUDIO_ALG |
        	   MCI_DGV_SETAUDIO_QUALITY |
        	   MCI_DGV_SETAUDIO_RECORD |
        	   MCI_DGV_SETAUDIO_LEFT |
        	   MCI_DGV_SETAUDIO_CLOCKTIME |
        	   MCI_DGV_SETAUDIO_RIGHT))
	    return MCIERR_UNSUPPORTED_FUNCTION;
	if (lpSet->dwValue > (DWORD) npMCI->nAudioStreams || lpSet->dwValue == 0)
	    return MCIERR_OUTOFRANGE;
    	if (dwFlags & MCI_TEST)
	    return 0L;
	dwRet = DeviceSetAudioStream(npMCI, (WORD) lpSet->dwValue);
    } else if (dwFlags & (MCI_DGV_SETAUDIO_ITEM |
		   MCI_DGV_SETAUDIO_VALUE |
		   MCI_DGV_SETAUDIO_ALG |
        	   MCI_DGV_SETAUDIO_QUALITY |
        	   MCI_DGV_SETAUDIO_RECORD |
        	   MCI_DGV_SETAUDIO_LEFT |
        	   MCI_DGV_SETAUDIO_CLOCKTIME |
        	   MCI_DGV_SETAUDIO_RIGHT))
	return MCIERR_UNSUPPORTED_FUNCTION;

    switch (dwFlags & (MCI_SET_ON | MCI_SET_OFF)) {
    case MCI_SET_ON:
	if (!(dwFlags & MCI_TEST))
	    dwRet = DeviceMute(npMCI, FALSE);
	break;
    case MCI_SET_OFF:
	if (!(dwFlags & MCI_TEST))
	    dwRet = DeviceMute(npMCI, TRUE);
	break;
    case MCI_SET_ON | MCI_SET_OFF:
	dwRet = MCIERR_FLAGS_NOT_COMPATIBLE;
	break;

    default:
	if (!(dwFlags & MCI_DGV_SETAUDIO_ITEM))
	    dwRet = MCIERR_MISSING_PARAMETER;
	break;
    }

    return dwRet;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicSetVideo | This function sets various Video options.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the set video message.
 *
 * @parm LPMCI_SET_PARMS | lpSet | Parameters for the set video message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/
DWORD NEAR PASCAL GraphicSetVideo (NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_SETVIDEO_PARMS lpSet)
{
    DWORD	dwRet = 0L;

    if (dwFlags & (MCI_DGV_SETVIDEO_OVER |
		    MCI_DGV_SETVIDEO_RECORD |
		    MCI_DGV_SETVIDEO_SRC_NUMBER |
		    MCI_DGV_SETVIDEO_QUALITY |
		    MCI_DGV_SETVIDEO_ALG |
		    MCI_DGV_SETVIDEO_STILL |
		    MCI_DGV_SETVIDEO_CLOCKTIME
			))
	return MCIERR_UNSUPPORTED_FUNCTION;

    if (dwFlags & MCI_DGV_SETVIDEO_ITEM) {
	switch (lpSet->dwItem) {
	    case MCI_DGV_SETVIDEO_PALHANDLE:
		if (dwFlags & MCI_DGV_SETVIDEO_VALUE) {
                    if (lpSet->dwValue &&
                        lpSet->dwValue != MCI_AVI_SETVIDEO_PALETTE_HALFTONE &&
#if 1
                        !IsGDIObject((HPALETTE) lpSet->dwValue))
#else
			GetObjectType((HPALETTE) lpSet->dwValue) != OBJ_PAL)
#endif
			return MCIERR_AVI_BADPALETTE;
		}
		
		if (!(dwFlags & MCI_TEST))
		    dwRet = DeviceSetPalette(npMCI,
				((dwFlags & MCI_DGV_SETVIDEO_VALUE) ?
					(HPALETTE) lpSet->dwValue : NULL));
                break;

            case MCI_DGV_SETVIDEO_STREAM:

                if (!(dwFlags & MCI_DGV_SETVIDEO_VALUE))
                    return MCIERR_UNSUPPORTED_FUNCTION;

                if (lpSet->dwValue == 0 ||
                    lpSet->dwValue > (DWORD)npMCI->nVideoStreams + npMCI->nOtherStreams)
                    return MCIERR_OUTOFRANGE;

                if (dwFlags & MCI_SET_ON)
                    DPF(("SetVideoStream to #%d on\n", (int)lpSet->dwValue));
                else if (dwFlags & MCI_SET_OFF)
                    DPF(("SetVideoStream to #%d off\n", (int)lpSet->dwValue));
                else
                    DPF(("SetVideoStream to #%d\n", (int)lpSet->dwValue));
		
                if (!(dwFlags & MCI_TEST)) {
                    dwRet = DeviceSetVideoStream(npMCI, (UINT)lpSet->dwValue,
                          !(dwFlags & MCI_SET_OFF));
                }
                break;
		
            case MCI_AVI_SETVIDEO_DRAW_PROCEDURE:

		if (DeviceMode(npMCI) != MCI_MODE_STOP)
                    return MCIERR_UNSUPPORTED_FUNCTION;
		
		if (npMCI->hicDrawDefault) {
		    if (npMCI->hicDrawDefault != (HIC) -1)
			ICClose(npMCI->hicDrawDefault);
		    npMCI->hicDrawDefault = 0;
		    npMCI->dwFlags &= ~(MCIAVI_USERDRAWPROC);
		}

                if (lpSet->dwValue) {

                    if (IsBadCodePtr((FARPROC) lpSet->dwValue)) {
                        DPF(("Bad code pointer!!!!\n"));
                        return MCIERR_OUTOFRANGE; //!!!MCIERR_BAD_PARAM;
                    }

                    npMCI->hicDrawDefault = ICOpenFunction(streamtypeVIDEO,
                        FOURCC_AVIDraw,ICMODE_DRAW,(FARPROC) lpSet->dwValue);

		    if (!npMCI->hicDrawDefault) {
			return MCIERR_INTERNAL;
		    }
		    DPF(("Successfully set new draw procedure....\n"));

		    npMCI->dwFlags |= MCIAVI_USERDRAWPROC;
                }

		npMCI->dwFlags |= MCIAVI_NEEDDRAWBEGIN;
		InvalidateRect(npMCI->hwnd, NULL, FALSE);
		return 0;
		
	    default:
		dwRet = MCIERR_UNSUPPORTED_FUNCTION;
		break;
	}
    } else if (dwFlags & (MCI_SET_ON | MCI_SET_OFF)) {
	switch (dwFlags & (MCI_SET_ON | MCI_SET_OFF)) {
	case MCI_SET_ON:
	    if (!(dwFlags & MCI_TEST)) {
		InvalidateRect(npMCI->hwnd, NULL, FALSE);
		npMCI->dwFlags |= MCIAVI_SHOWVIDEO;
	    }
	    break;
	case MCI_SET_OFF:
	    if (!(dwFlags & MCI_TEST))
		npMCI->dwFlags &= ~(MCIAVI_SHOWVIDEO);
	    break;
	case MCI_SET_ON | MCI_SET_OFF:
	    dwRet = MCIERR_FLAGS_NOT_COMPATIBLE;
	    break;
	}
    } else
	dwRet = MCIERR_MISSING_PARAMETER;

    return dwRet;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicSignal | This function sets signals.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the set PositionAdvise message.
 *
 * @parm LPMCI_SIGNAL_PARMS | lpSignal | Parameters for the signal
 *	message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/
DWORD NEAR PASCAL GraphicSignal(NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_SIGNAL_PARMS lpSignal)
{
    DWORD	dwRet = 0L;
    DWORD	dwUser;
    DWORD	dwPosition;
    DWORD	dwPeriod;

    dwUser = (dwFlags & MCI_DGV_SIGNAL_USERVAL) ? lpSignal->dwUserParm : 0L;

    if (dwFlags & MCI_DGV_SIGNAL_CANCEL) {
	if (dwFlags & (MCI_DGV_SIGNAL_AT |
	    	       MCI_DGV_SIGNAL_EVERY |
	    	       MCI_DGV_SIGNAL_POSITION))
	    return MCIERR_FLAGS_NOT_COMPATIBLE;

	if (!npMCI->dwSignals)
	    return MCIERR_NONAPPLICABLE_FUNCTION;

	if (dwUser && (npMCI->signal.dwUserParm != dwUser))
	    return MCIERR_NONAPPLICABLE_FUNCTION;

	if (!(dwFlags & MCI_TEST))
	    --npMCI->dwSignals;
    } else {
	if ((npMCI->dwSignals != 0) && (dwUser != npMCI->signal.dwUserParm)) {
	    /* !!! Should we allow more than one signal? */
	    return MCIERR_DGV_DEVICE_LIMIT;
	}

	if (dwFlags & MCI_DGV_SIGNAL_AT) {
	    /* Use position passed in */
	    dwPosition = ConvertToFrames(npMCI, lpSignal->dwPosition);
	    if (dwPosition > (DWORD) npMCI->lFrames)
		return MCIERR_OUTOFRANGE;
	} else {
	    /* Get current position */
	    DevicePosition(npMCI, (LPLONG) &dwPosition);
	}

	if (dwFlags & MCI_DGV_SIGNAL_EVERY) {
	    dwPeriod = (DWORD) ConvertToFrames(npMCI, lpSignal->dwPeriod);
	
	    if (dwPeriod == 0 || (dwPeriod > (DWORD) npMCI->lFrames))
		return MCIERR_OUTOFRANGE;
	} else {
	    /* It's a one-time signal */
	    dwPeriod = 0L;
	}

	if (dwFlags & MCI_TEST)
	    return 0;

	npMCI->signal.dwPosition = dwPosition;
	npMCI->signal.dwPeriod = dwPeriod;	
	npMCI->signal.dwUserParm = dwUser;
	npMCI->signal.dwCallback = lpSignal->dwCallback;
	npMCI->dwSignalFlags = dwFlags;

	/* The signal isn't really activated until we do this. */
	if (!npMCI->dwSignals)
	    ++npMCI->dwSignals;
    }
	
    return 0L;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicList | This function supports the MCI_LIST command.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the List message.
 *
 * @parm LPMCI_DGV_LIST_PARMS | lpList | Parameters for the list message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/
DWORD NEAR PASCAL GraphicList(NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_LIST_PARMS lpList)
{
    return MCIERR_UNSUPPORTED_FUNCTION;
}


/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicGetDevCaps | This function returns  device
 *      capabilities
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the GetDevCaps message.
 *
 * @parm LPMCI_GETDEVCAPS_PARMS | lpCaps | Parameters for the GetDevCaps
 *      message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicGetDevCaps (NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_GETDEVCAPS_PARMS lpCaps )
{

    DWORD dwRet = 0L;


    if (dwFlags & MCI_GETDEVCAPS_ITEM)
        {

        switch (lpCaps->dwItem)
            {
            case MCI_GETDEVCAPS_CAN_RECORD:
            case MCI_GETDEVCAPS_CAN_EJECT:
            case MCI_GETDEVCAPS_CAN_SAVE:
            case MCI_DGV_GETDEVCAPS_CAN_LOCK:
            case MCI_DGV_GETDEVCAPS_CAN_STR_IN:
            case MCI_DGV_GETDEVCAPS_CAN_FREEZE:
            case MCI_DGV_GETDEVCAPS_HAS_STILL:
		
                lpCaps->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
                dwRet = MCI_RESOURCE_RETURNED;
                break;

            case MCI_DGV_GETDEVCAPS_CAN_REVERSE:
            case MCI_GETDEVCAPS_CAN_PLAY:
            case MCI_GETDEVCAPS_HAS_AUDIO:
            case MCI_GETDEVCAPS_HAS_VIDEO:
            case MCI_GETDEVCAPS_USES_FILES:
            case MCI_GETDEVCAPS_COMPOUND_DEVICE:
            case MCI_DGV_GETDEVCAPS_PALETTES:
            case MCI_DGV_GETDEVCAPS_CAN_STRETCH:
            case MCI_DGV_GETDEVCAPS_CAN_TEST:
                lpCaps->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
                dwRet = MCI_RESOURCE_RETURNED;
                break;

            case MCI_GETDEVCAPS_DEVICE_TYPE:

                lpCaps->dwReturn = MAKEMCIRESOURCE(MCI_DEVTYPE_DIGITAL_VIDEO,
					    MCI_DEVTYPE_DIGITAL_VIDEO);
                dwRet = MCI_RESOURCE_RETURNED;
                break;

	    case MCI_DGV_GETDEVCAPS_MAX_WINDOWS:
	    case MCI_DGV_GETDEVCAPS_MAXIMUM_RATE:
	    case MCI_DGV_GETDEVCAPS_MINIMUM_RATE:
            default:

                dwRet = MCIERR_UNSUPPORTED_FUNCTION;
                break;
            }
        }
    else
        dwRet = MCIERR_MISSING_PARAMETER;

    if ((dwFlags & MCI_TEST) && (LOWORD(dwRet) == 0)) {
	/* There is no error, but the test flag is on.  Return as little
	** as possible.
	*/
	dwRet = 0;
	lpCaps->dwReturn = 0;
    }

    return (dwRet);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicWindow | This function controls the stage window
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the window message.
 *
 * @parm LPMCI_DGV_WINDOW_PARMS | lpPlay | Parameters for the window message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicWindow (NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_WINDOW_PARMS lpWindow)
{
    DWORD   dwRet = 0L;
    int	    i = 0;
    HWND    hWndNew;

    if (dwFlags & MCI_DGV_WINDOW_HWND) {
        // Set a new stage window. If the parameter is NULL, then
        // use the default window. Otherwise, hide the default
        // window and use the given window handle.

        if (!lpWindow->hWnd)
            hWndNew = npMCI->hwndDefault;
        else
            hWndNew = lpWindow->hWnd;

        if (!IsWindow(hWndNew))
            return MCIERR_NO_WINDOW;

	/* If the test flag is set, return without doing anything. */
	/* Question: do we have to check for more possible errors? */
	if (dwFlags & MCI_TEST)
	    return 0L;

        // only change if the new window handle is different from the current
        // stage window handle

        if (hWndNew != npMCI->hwnd) {
            dwRet = DeviceSetWindow(npMCI, hWndNew);

            if (npMCI->hwnd != npMCI->hwndDefault &&
				    IsWindow(npMCI->hwndDefault))
                ShowWindow(npMCI->hwndDefault, SW_HIDE);
	}
    }

    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST)
	return dwRet;

    if (!dwRet) {
        if (IsWindow(npMCI->hwnd)) {
            if (dwFlags & MCI_DGV_WINDOW_STATE)
                ShowWindow (npMCI->hwnd, lpWindow->nCmdShow);

            if (dwFlags & MCI_DGV_WINDOW_TEXT)
                SetWindowText(npMCI->hwnd, lpWindow->lpstrText);
	} else
	    dwRet = MCIERR_NO_WINDOW;
    }

    return dwRet;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicPut | This function sets the offset and extent
 *      of the animation within the client area of the stage window.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the put message.
 *
 * @parm LPMCI_DGV_RECT_PARMS | lpDestination | Parameters for the
 *      destination message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicPut ( NPMCIGRAPHIC npMCI,
    DWORD dwFlags, LPMCI_DGV_RECT_PARMS lpParms)
{
    BOOL	frc;
    RECT	rc;

    if (dwFlags & (MCI_DGV_PUT_FRAME | MCI_DGV_PUT_VIDEO))
	return MCIERR_UNSUPPORTED_FUNCTION;

    frc = (dwFlags & MCI_DGV_RECT) == MCI_DGV_RECT;

    if (!IsWindow(npMCI->hwnd))
	return MCIERR_NO_WINDOW;

    switch (dwFlags & (MCI_DGV_PUT_SOURCE | MCI_DGV_PUT_DESTINATION |
			    MCI_DGV_PUT_WINDOW)) {
	case 0L:
	    return MCIERR_MISSING_PARAMETER;
	
	case MCI_DGV_PUT_SOURCE:
	    // If a rectangle is supplied, use it.
	    if (frc) {
		rc.left = lpParms->ptOffset.x;
		rc.top = lpParms->ptOffset.y;
		rc.right = lpParms->ptOffset.x + lpParms->ptExtent.x;
		rc.bottom = lpParms->ptOffset.y + lpParms->ptExtent.y;
		
		if (lpParms->ptExtent.x <= 0) {
		    rc.right = rc.left + (npMCI->rcDest.right - npMCI->rcDest.left);
		}
		if (lpParms->ptExtent.y <= 0) {
		    rc.bottom = rc.top + (npMCI->rcDest.bottom - npMCI->rcDest.top);
		}		
	    } else {
		/* Reset to default */
                rc = npMCI->rcMovie;
	    }
	    break;
	
	case MCI_DGV_PUT_DESTINATION:
	    // If a rectangle is supplied, use it.
	    if (frc) {
		rc.left = lpParms->ptOffset.x;
		rc.top = lpParms->ptOffset.y;
		rc.right = lpParms->ptOffset.x + lpParms->ptExtent.x;
		rc.bottom = lpParms->ptOffset.y + lpParms->ptExtent.y;
		
		if (lpParms->ptExtent.x <= 0) {
		    rc.right = rc.left + (npMCI->rcDest.right - npMCI->rcDest.left);
		}
		if (lpParms->ptExtent.y <= 0) {
		    rc.bottom = rc.top + (npMCI->rcDest.bottom - npMCI->rcDest.top);
		}
		
	    } else {
		/* Reset to size of stage window */
		GetClientRect(npMCI->hwnd, &rc);
	    }
	    break;
	
	case MCI_DGV_PUT_WINDOW:
	    if (dwFlags & MCI_TEST)
		return 0L;

	    // De-minimize their window, so we don't end up with
	    // a giant icon....
	    if (IsIconic(npMCI->hwnd))
		ShowWindow(npMCI->hwnd, SW_RESTORE);
	
	    // If a rectangle is supplied, use it.
	    if (frc) {
		RECT	rcOld;
		
		rc.left = lpParms->ptOffset.x;
		rc.right = lpParms->ptOffset.x + lpParms->ptExtent.x;
		rc.top = lpParms->ptOffset.y;
		rc.bottom = lpParms->ptOffset.y + lpParms->ptExtent.y;
		if (dwFlags & MCI_DGV_PUT_CLIENT) {
		    AdjustWindowRect(&rc,
				    GetWindowLong(npMCI->hwnd, GWL_STYLE),
				    FALSE);
		}

		// Default to just moving if width, height == 0....
		GetWindowRect(npMCI->hwnd, &rcOld);
		if (lpParms->ptExtent.x <= 0) {
		    rc.right = rc.left + (rcOld.right - rcOld.left);
		}
		if (lpParms->ptExtent.y <= 0) {
		    rc.bottom = rc.top + (rcOld.bottom - rcOld.top);
		}
		
		MoveWindow(npMCI->hwnd,
			    rc.left, rc.top,
			    rc.right - rc.left, rc.bottom - rc.top, TRUE);
	    } else {
		// !!! What should we do if there's no rectangle?
		
		/* Reset to "natural" size? */
                rc = npMCI->rcMovie;
		
		if (npMCI->dwOptionFlags & MCIAVIO_ZOOMBY2)
		    SetRect(&rc, 0, 0, rc.right*2, rc.bottom*2);
		
		AdjustWindowRect(&rc, GetWindowLong(npMCI->hwnd, GWL_STYLE),
					    FALSE);

		SetWindowPos(npMCI->hwnd, NULL, 0, 0,
				rc.right - rc.left, rc.bottom - rc.top,
				SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	    }

	    // Premiere 1.0 depends on the window always being visible
	    // after a PUT_WINDOW command.  Make it so.
	    ShowWindow(npMCI->hwnd, SW_RESTORE);
	    return 0L;
	
	default:
	    return MCIERR_FLAGS_NOT_COMPATIBLE;
    }

    if (dwFlags & MCI_DGV_PUT_CLIENT)
	return MCIERR_FLAGS_NOT_COMPATIBLE;
	
    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST)
	return 0L;

    return DevicePut(npMCI, &rc, dwFlags);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicWhere | This function returns the current
 *	source and destination rectangles, in offset/extent form.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the query source message.
 *
 * @parm LPMCI_DGV_RECT_PARMS | lpParms | Parameters for the message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicWhere(NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_RECT_PARMS lpParms)
{
    RECT	rc;

    if (dwFlags & (MCI_DGV_WHERE_FRAME | MCI_DGV_WHERE_VIDEO))
	return MCIERR_UNSUPPORTED_FUNCTION;

    // !!! WHERE_WINDOW?
	
    switch (dwFlags & (MCI_DGV_WHERE_SOURCE | MCI_DGV_WHERE_DESTINATION |
			    MCI_DGV_WHERE_WINDOW)) {
	case 0L:
	    return MCIERR_MISSING_PARAMETER;
	
	case MCI_DGV_WHERE_SOURCE:
	    if (dwFlags & MCI_DGV_WHERE_MAX) {
                lpParms->ptOffset.x = npMCI->rcMovie.left;
                lpParms->ptOffset.y = npMCI->rcMovie.top;
                lpParms->ptExtent.x = npMCI->rcMovie.right - npMCI->rcMovie.left;
                lpParms->ptExtent.y = npMCI->rcMovie.bottom - npMCI->rcMovie.top;
	    } else {
		lpParms->ptOffset.x = npMCI->rcSource.left;
		lpParms->ptOffset.y = npMCI->rcSource.top;
                lpParms->ptExtent.x = npMCI->rcSource.right  - npMCI->rcSource.left;
                lpParms->ptExtent.y = npMCI->rcSource.bottom - npMCI->rcSource.top;
	    }
	    break;
	
	case MCI_DGV_WHERE_DESTINATION:
	    if (dwFlags & MCI_DGV_WHERE_MAX) {
		/* Return size of window */
		GetClientRect(npMCI->hwnd, &rc);
		lpParms->ptOffset.x = 0;
		lpParms->ptOffset.y = 0;
		lpParms->ptExtent.x = rc.right;
		lpParms->ptExtent.y = rc.bottom;
	    } else {
		/* Return current destination size */
		lpParms->ptOffset.x = npMCI->rcDest.left;
		lpParms->ptOffset.y = npMCI->rcDest.top;
		lpParms->ptExtent.x = npMCI->rcDest.right - npMCI->rcDest.left;
		lpParms->ptExtent.y = npMCI->rcDest.bottom - npMCI->rcDest.top;
	    }
	    break;
	
	case MCI_DGV_WHERE_WINDOW:
	    if (dwFlags & MCI_DGV_WHERE_MAX) {
		/* Return maximum size of window */
		GetClientRect(npMCI->hwnd, &rc);
		lpParms->ptOffset.x = 0;
		lpParms->ptOffset.y = 0;
		lpParms->ptExtent.x = GetSystemMetrics(SM_CXSCREEN);
		lpParms->ptExtent.y = GetSystemMetrics(SM_CYSCREEN);
	    } else {
		/* Return size of window */
		GetWindowRect(npMCI->hwnd, &rc);
		lpParms->ptOffset.x = rc.left;
		lpParms->ptOffset.y = rc.top;
		lpParms->ptExtent.x = rc.right - rc.left;
		lpParms->ptExtent.y = rc.bottom - rc.top;
	    }
	    break;

	default:
	    return MCIERR_FLAGS_NOT_COMPATIBLE;
    }

    return 0L;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicRealize | This function realizes the current palette
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicRealize(NPMCIGRAPHIC npMCI, DWORD dwFlags)
{
    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST)
        return 0L;

    npMCI->fForceBackground = (dwFlags & MCI_DGV_REALIZE_BKGD) != 0;

    return DeviceRealize(npMCI);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | GraphicUpdate | This function refreshes the current frame.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwFlags | Flags for the message.
 *
 * @parm LPMCI_DGV_UPDATE_PARMS | lpParms | Parameters for the message.
 *
 * @rdesc Returns an MCI error code.
 *
 ***************************************************************************/

DWORD NEAR PASCAL GraphicUpdate(NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_DGV_UPDATE_PARMS lpParms)
{
    RECT    rc;

    rc.left   = lpParms->ptOffset.x;
    rc.top    = lpParms->ptOffset.y;
    rc.right  = lpParms->ptOffset.x + lpParms->ptExtent.x;
    rc.bottom = lpParms->ptOffset.y + lpParms->ptExtent.y;

    if (!(dwFlags & MCI_DGV_UPDATE_HDC)) {
	InvalidateRect(npMCI->hwnd, (dwFlags & MCI_DGV_RECT) ? &rc : NULL, TRUE);
	UpdateWindow(npMCI->hwnd);
        return 0;
    }

    /* If the test flag is set, return without doing anything. */
    /* Question: do we have to check for more possible errors? */
    if (dwFlags & MCI_TEST)
	return 0L;

    /* It's ok to pass a NULL rect to DeviceUpdate() */

#ifndef WIN32
#pragma message("!!! Fix update parms!")
#endif
    return DeviceUpdate (npMCI, dwFlags, lpParms->hDC, (dwFlags & MCI_DGV_RECT) ? &rc : NULL);
}

DWORD FAR PASCAL GraphicConfig(NPMCIGRAPHIC npMCI, DWORD dwFlags)
{
    DWORD dwOptions = npMCI->dwOptionFlags;

    if (!(dwFlags & MCI_TEST)) {
        gfEvil++;
        if (ConfigDialog(NULL, npMCI)) {

#ifdef DEBUG
            //
            // in DEBUG always reset the dest rect because the user may
            // have played with the DEBUG DrawDib options and we will
            // need to call DrawDibBegin() again.
            //
            if (TRUE) {
#else
            if ((npMCI->dwOptionFlags & (MCIAVIO_STUPIDMODE|MCIAVIO_ZOOMBY2))
                        != (dwOptions & (MCIAVIO_STUPIDMODE|MCIAVIO_ZOOMBY2)) ) {
#endif

		npMCI->lFrameDrawn = (- (LONG) npMCI->wEarlyRecords) - 1;
                SetWindowToDefaultSize(npMCI);
                SetRectEmpty(&npMCI->rcDest); //This will force a change!
                ResetDestRect(npMCI);
            }
        }
        else {
            npMCI->dwOptionFlags = dwOptions;
        }
        gfEvil--;
    }

    return 0L;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | mciSpecial | This function handles all the MCI
 *      commands that don't require instance data such as open.
 *
 * @parm UINT | wDeviceID | The MCI device ID
 *
 * @parm UINT | wMessage | The requested action to be performed.
 *
 * @parm DWORD | dwFlags | Flags for the message.
 *
 * @parm DWORD | lpParms | Parameters for this message.
 *
 * @rdesc Error Constant. 0L on success
 *
 ***************************************************************************/

DWORD NEAR PASCAL mciSpecial (UINT wDeviceID, UINT wMessage, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    NPMCIGRAPHIC npMCI = 0L;
    DWORD dwRet;

    /* since there in no instance block, there is no saved notification */
    /* to abort. */

    switch (wMessage) {
	case MCI_OPEN_DRIVER:
            if (dwFlags & (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID))
                dwRet = GraphicOpen (&npMCI, dwFlags,
			    (LPMCI_DGV_OPEN_PARMS) lpParms, wDeviceID);
            else
                dwRet = 0L;

            mciSetDriverData (wDeviceID, (UINT)npMCI);
            break;

        case MCI_GETDEVCAPS:
            dwRet = GraphicGetDevCaps(NULL, dwFlags,
			    (LPMCI_GETDEVCAPS_PARMS)lpParms);
            break;

        case MCI_CONFIGURE:

            if (!(dwFlags & MCI_TEST))
                ConfigDialog(NULL, NULL);

	    dwRet = 0L;
	    break;

        case MCI_INFO:
            dwRet = GraphicInfo(NULL, dwFlags, (LPMCI_DGV_INFO_PARMS)lpParms);
            break;

        case MCI_CLOSE_DRIVER:
            dwRet = 0L;
            break;

        default:
            dwRet = MCIERR_UNSUPPORTED_FUNCTION;
            break;
    }

    GraphicImmediateNotify (wDeviceID, lpParms, dwFlags, dwRet);
    return (dwRet);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | mciDriverEntry | This function is the MCI handler
 *
 * @parm UINT | wDeviceID | The MCI device ID
 *
 * @parm UINT | wMessage | The requested action to be performed.
 *
 * @parm DWORD | dwFlags | Flags for the message.
 *
 * @parm DWORD | lpParms | Parameters for this message.
 *
 * @rdesc Error Constant. 0L on success
 *
 ***************************************************************************/

DWORD PASCAL mciDriverEntry (UINT wDeviceID, UINT wMessage, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    NPMCIGRAPHIC npMCI = 0L;
    DWORD dwRet = MCIERR_UNRECOGNIZED_COMMAND;
    BOOL fDelayed = FALSE;
    BOOL fNested = FALSE;

    /* All current commands require a parameter block. */

    if (!lpParms && (dwFlags & MCI_NOTIFY))
        return (MCIERR_MISSING_PARAMETER);

    npMCI = (NPMCIGRAPHIC) (UINT)mciGetDriverData(wDeviceID);

    if (!npMCI)
        return mciSpecial(wDeviceID, wMessage, dwFlags, lpParms);

    /*
     * grab this device's critical section
     */
    EnterCrit(npMCI);


    if (npMCI->wMessageCurrent) {
	fNested = TRUE;
	
	if (wMessage != MCI_STATUS && wMessage != MCI_GETDEVCAPS &&
		    wMessage != MCI_INFO) {
	    DPF(("Warning!!!!!\n"));
	    DPF(("Warning!!!!!     MCIAVI reentered: received %x while processing %x\n", wMessage, npMCI->wMessageCurrent));
	    DPF(("Warning!!!!!\n"));
	    DPF(("Warning!!!!!\n"));
//	    Assert(0);
//	    LeaveCrit(npMCI);
//	    return MCIERR_DEVICE_NOT_READY;
	}
    } else	
	npMCI->wMessageCurrent = wMessage;

    switch (wMessage) {

	case MCI_CLOSE_DRIVER:

            /* Closing the driver causes any currently saved notifications */
            /* to abort. */

            GraphicDelayedNotify(npMCI, MCI_NOTIFY_ABORTED);

	    // note that GraphicClose will release and delete the critsec
 	    dwRet = GraphicClose(npMCI);

            mciSetDriverData(wDeviceID, 0L);
	
	    npMCI = NULL;
	    break;

    	case MCI_PLAY:
	
            dwRet = GraphicPlay(npMCI, dwFlags, (LPMCI_PLAY_PARMS)lpParms);
	    fDelayed = TRUE;
            break;

    	case MCI_CUE:
	
            dwRet = GraphicCue(npMCI, dwFlags, (LPMCI_DGV_CUE_PARMS)lpParms);
	    fDelayed = TRUE;
            break;

	case MCI_STEP:

            dwRet = GraphicStep(npMCI, dwFlags, (LPMCI_DGV_STEP_PARMS)lpParms);
	    fDelayed = TRUE;
	    break;
	
	case MCI_STOP:

            dwRet = GraphicStop(npMCI, dwFlags, lpParms);
            break;

	case MCI_SEEK:

            dwRet = GraphicSeek (npMCI, dwFlags, (LPMCI_SEEK_PARMS)lpParms);
	    fDelayed = TRUE;
            break;

	case MCI_PAUSE:

            dwRet = GraphicPause(npMCI, dwFlags, lpParms);
	    fDelayed = TRUE;
            break;

        case MCI_RESUME:

            dwRet = GraphicResume(npMCI, dwFlags, lpParms);
	    fDelayed = TRUE;
            break;

        case MCI_SET:

            dwRet = GraphicSet(npMCI, dwFlags,
				(LPMCI_DGV_SET_PARMS)lpParms);
	    break;

	case MCI_STATUS:

            dwRet = GraphicStatus(npMCI, dwFlags,
				(LPMCI_DGV_STATUS_PARMS)lpParms);
	    break;

	case MCI_INFO:

 	    dwRet = GraphicInfo (npMCI, dwFlags, (LPMCI_DGV_INFO_PARMS)lpParms);
	    break;

        case MCI_GETDEVCAPS:

            dwRet = GraphicGetDevCaps(npMCI, dwFlags, (LPMCI_GETDEVCAPS_PARMS)lpParms);
	    break;

        case MCI_REALIZE:

            dwRet = GraphicRealize(npMCI, dwFlags);
            break;

        case MCI_UPDATE:

            dwRet = GraphicUpdate(npMCI, dwFlags, (LPMCI_DGV_UPDATE_PARMS)lpParms);
            break;

	case MCI_WINDOW:
 	
            dwRet = GraphicWindow(npMCI, dwFlags, (LPMCI_DGV_WINDOW_PARMS)lpParms);
	    break;

        case MCI_PUT:

 	    dwRet = GraphicPut(npMCI, dwFlags, (LPMCI_DGV_RECT_PARMS)lpParms);
            break;
	
        case MCI_WHERE:

            dwRet = GraphicWhere(npMCI, dwFlags, (LPMCI_DGV_RECT_PARMS)lpParms);
            break;
	
	case MCI_CONFIGURE:
	    dwRet = GraphicConfig(npMCI, dwFlags);
	    break;

	case MCI_SETAUDIO:
	    dwRet = GraphicSetAudio(npMCI, dwFlags,
			(LPMCI_DGV_SETAUDIO_PARMS) lpParms);
	    break;

	case MCI_SETVIDEO:
	    dwRet = GraphicSetVideo(npMCI, dwFlags,
			(LPMCI_DGV_SETVIDEO_PARMS) lpParms);
	    break;

	case MCI_SIGNAL:
	    dwRet = GraphicSignal(npMCI, dwFlags,
			(LPMCI_DGV_SIGNAL_PARMS) lpParms);
	    break;
	
	case MCI_LIST:
	    dwRet = GraphicList(npMCI, dwFlags,
			(LPMCI_DGV_LIST_PARMS) lpParms);
	    break;

        case MCI_LOAD:
	    dwRet = GraphicLoad(npMCI, dwFlags,
				  (LPMCI_DGV_LOAD_PARMS) lpParms);
	    break;
	    	
        case MCI_RECORD:
        case MCI_SAVE:
	
        case MCI_CUT:
        case MCI_COPY:
        case MCI_PASTE:
        case MCI_UNDO:
	
	case MCI_DELETE:
	case MCI_CAPTURE:
	case MCI_QUALITY:
	case MCI_MONITOR:
	case MCI_RESERVE:
	case MCI_FREEZE:
	case MCI_UNFREEZE:
            dwRet = MCIERR_UNSUPPORTED_FUNCTION;
            break;
	
	    /* Do we need this case? */
	default:
            dwRet = MCIERR_UNRECOGNIZED_COMMAND;
            break;
    }

    if (!fDelayed || (dwFlags & MCI_TEST)) {
	/* We haven't processed the notify yet. */
        if (npMCI && (dwFlags & MCI_NOTIFY) && (!LOWORD(dwRet)))
	    /* Throw away the old notify */
            GraphicDelayedNotify(npMCI, MCI_NOTIFY_SUPERSEDED);

	/* And send the new one out immediately. */
        GraphicImmediateNotify(wDeviceID, lpParms, dwFlags, dwRet);
    }

    /* If there's an error, don't save the callback.... */
    if (fDelayed && dwRet != 0 && (dwFlags & MCI_NOTIFY))
	npMCI->hCallback = 0;

    //
    //  see if we need to tell the DRAW device about moving.
    //  MPlayer is sending the status and position command alot
    //  so this is a "timer"
    //
    //  !!!do we need to do it this often?
    //
    if (npMCI && (npMCI->dwFlags & MCIAVI_WANTMOVE))
	CheckWindowMove(npMCI, FALSE);

    if (npMCI && !fNested)
	npMCI->wMessageCurrent = 0;

    // free critical section for this device - if we haven't deleted it
    // GraphicClose may have released and deleted it if we were deleting
    // the device instance.
    if (npMCI) {
	LeaveCrit(npMCI);
    }

    return dwRet;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api LONG | ConvertToFrames | Convert from the current time format into
 *	frames.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm DWORD | dwTime | Input time.
 *
 ***************************************************************************/
STATICFN LONG NEAR PASCAL ConvertToFrames(NPMCIGRAPHIC npMCI, DWORD dwTime)
{
    if (npMCI->dwTimeFormat == MCI_FORMAT_FRAMES) {
	return (LONG) dwTime;
    } else {
	if (npMCI->dwMicroSecPerFrame > 1000) {
	/* This needs to round down--muldiv32 likes to round off. */
	return (LONG) muldivrd32(dwTime, 1000L, npMCI->dwMicroSecPerFrame);
	} else {
	    return (LONG) muldivru32(dwTime, 1000L, npMCI->dwMicroSecPerFrame);
        }
    }
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api DWORD | ConvertFromFrames | Convert from frames into the current
 *	time format.
 *
 * @parm NPMCIGRAPHIC | npMCI | Near pointer to instance data block
 *
 * @parm LONG | lFrame | Frame number to convert.
 *
 ***************************************************************************/
DWORD NEAR PASCAL ConvertFromFrames(NPMCIGRAPHIC npMCI, LONG lFrame)
{
    if (npMCI->dwTimeFormat == MCI_FORMAT_FRAMES) {
	return (DWORD)lFrame;
    } else {
	if (npMCI->dwMicroSecPerFrame > 1000)
	return muldivru32(lFrame, npMCI->dwMicroSecPerFrame, 1000L);
	else
	    return muldivrd32(lFrame, npMCI->dwMicroSecPerFrame, 1000L);
    }
}
