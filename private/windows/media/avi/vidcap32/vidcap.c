/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   vidcap.c: WinMain and command processing
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/
 
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <dos.h>
#include <shellapi.h>
#include <vfw.h>

#include "vidcap.h"
#include "vidframe.h"
#include "profile.h"

// generic window control classes
#include "toolbar.h"
#include "status.h"
#include "arrow.h"
#include "rlmeter.h"
#include "help.h"

// the standard toolbar class 'exports' this but doesn't put it in the
// header file
extern TCHAR     szToolBarClass[];//HACK!


// height of the buttons on a toolbar - depends on the
// size of the bitmaps within IDBMP_TOOLBAR
#define BUTTONWIDTH     24
#define BUTTONHEIGHT    22
#define TOOLBAR_HEIGHT          BUTTONHEIGHT + 6


// description and layout of toolbar buttons within IDBMP_TOOLBAR
#define APP_NUMTOOLS 8

#define BTN_SETFILE		0
#define BTN_EDITCAP		1
#define BTN_LIVE		2
#define BTN_CAPFRAME		3
#define BTN_CAPSEL		4
#define BTN_CAPAVI		5
#define BTN_CAPPAL		6
#define BTN_OVERLAY		7

static int           aiButton[] = {BTN_SETFILE, BTN_EDITCAP,
                            BTN_LIVE, BTN_OVERLAY, BTN_CAPFRAME,
                            BTN_CAPSEL, BTN_CAPAVI, BTN_CAPPAL };
static int           aiState[] = {BTNST_FOCUSUP, BTNST_UP,
                            BTNST_UP, BTNST_UP, BTNST_UP,
                            BTNST_UP, BTNST_UP, BTNST_UP};
static int           aiType[] ={BTNTYPE_PUSH, BTNTYPE_PUSH,
                            BTNTYPE_CHECKBOX, BTNTYPE_CHECKBOX,
                            BTNTYPE_PUSH,
                            BTNTYPE_PUSH, BTNTYPE_PUSH, BTNTYPE_PUSH};
static int           aiString[] = { IDC_toolbarSETFILE,
                            IDC_toolbarEDITCAP, IDC_toolbarLIVE,
                            IDC_toolbarOVERLAY,
                            IDC_toolbarCAPFRAME, IDC_toolbarCAPSEL,
                            IDC_toolbarCAPAVI, IDC_toolbarCAPPAL };
static int           aPos[] = { 10, 35, 75, 100, 150, 175, 200, 225 };




//
// Global Variables
//

// preferences
BOOL gbCentre;
BOOL gbToolBar;
BOOL gbStatusBar;
BOOL gbAutoSizeFrame;
int gBackColour;

BOOL gbLive, gbOverlay;
BOOL gfIsRTL;

// saved window sizes
int gWinX, gWinY;
int gWinCX, gWinCY;
int gWinShow;

// command line options
int gCmdLineDeviceID = -1;


TCHAR          gachAppName[]  = "vidcapApp" ;
TCHAR          gachIconName[] = "vidcapIcon" ;
TCHAR          gachMenuName[] = "vidcapMenu" ;
TCHAR          gachAppTitle[20];    //VidCap
TCHAR          gachCaptureFile[_MAX_PATH];
TCHAR          gachMCIDeviceName[21];
TCHAR          gachString[128] ;
TCHAR          gachBuffer[200] ;
TCHAR          gachLastError[256];


HINSTANCE      ghInstApp ;
HWND           ghWndMain = NULL ;
HWND           ghWndFrame;      // child of ghWndMain  - frames and scrolls
HWND           ghWndCap  ;      // child of ghWndCap
HWND           ghWndToolBar;
HWND           ghWndStatus;

HANDLE         ghAccel ;
WORD           gwDeviceIndex ;
WORD           gwPalFrames = DEF_PALNUMFRAMES ;
WORD           gwPalColors = DEF_PALNUMCOLORS ;
WORD           gwCapFileSize ;

CAPSTATUS      gCapStatus ;
CAPDRIVERCAPS  gCapDriverCaps ;
CAPTUREPARMS   gCapParms ;
BOOL           gbHaveHardware;
UINT           gDriverCount;
BOOL           gbIsScrncap;  // For Scrncap.drv, we must yield
BOOL           gbInLayout;
UINT           gAVStreamMaster;

HANDLE         ghwfex ;
LPWAVEFORMATEX glpwfex ;

FARPROC        fpErrorCallback ;
FARPROC        fpStatusCallback ;
FARPROC        fpYieldCallback ;


// set to false when we capture a palette (or if we have warned him and
// he says its ok
BOOL bDefaultPalette = TRUE;

#ifdef DEBUG
int	nTestCount;
#endif

// c-runtime cmd line
extern char ** __argv;
extern int __argc;

#define LimitRange(Val,Low,Hi) (max(Low,(min(Val,Hi))))


//
// Function prototypes
//
LRESULT FAR PASCAL MainWndProc(HWND, UINT, WPARAM, LPARAM) ;
LRESULT FAR PASCAL ErrorCallbackProc(HWND, int, LPSTR) ;
LRESULT FAR PASCAL StatusCallbackProc(HWND, int, LPSTR) ;
LRESULT FAR PASCAL YieldCallbackProc(HWND) ;
void vidcapSetLive(BOOL bLive);
void vidcapSetOverlay(BOOL bOverlay);
void vidcapSetCaptureFile(LPTSTR pFileName);

BOOL vidcapRegisterClasses(HINSTANCE hInstance, HINSTANCE hPrevInstance);
BOOL vidcapCreateWindows(HINSTANCE hInstance, HINSTANCE hPrevInstance);
void vidcapLayout(HWND hwnd);
BOOL vidcapEnumerateDrivers(HWND hwnd);
BOOL vidcapInitHardware(HWND hwnd, HWND hwndCap, UINT uIndex);
void vidcapReadProfile(void);
void vidcapWriteProfile(void);
void vidcapReadSettingsProfile(void);
void vidcapWriteSettingsProfile(void);


/* --- initialisation -------------------------------------------------- */


//
// WinMain: Application Entry Point Function
//
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
///////////////////////////////////////////////////////////////////////
//  hInstance:      handle for this instance
//  hPrevInstance:  handle for possible previous instances
//  lpszCmdLine:    long pointer to exec command line
//  nCmdShow:       Show code for main window display
///////////////////////////////////////////////////////////////////////

    MSG          msg ;
    BOOL bValidCmdline;
    BOOL fOK;
    int i;
    char ach[2];

    ghInstApp = hInstance ;
    LoadString(hInstance, IDS_CAP_RTL, ach, sizeof(ach));
    gfIsRTL = ach[0] == '1';
    gCmdLineDeviceID = -1;

    // read the app title string - used in several message boxes
    LoadString(hInstance, IDS_APP_TITLE, gachAppTitle, sizeof(gachAppTitle));

    // read defaults out of the registry
    vidcapReadProfile();

    // look for cmd line options
    bValidCmdline = TRUE;

    for ( i = 1; (i < __argc) && bValidCmdline; i++) {
        if ((__argv[i][0] == '/') || (__argv[i][0] == '-')) {

            switch(__argv[i][1]) {
            case 'D':
            case 'd':
                if (gCmdLineDeviceID < 0) {
                    // allow "-d0" and "-d 0"
                    PSTR p = &__argv[i][2];

                    if ((*p == 0) && ((i+1) < __argc)) {
                        p = __argv[++i];
                    }


                    gCmdLineDeviceID = atoi(p);
                } else {
                    bValidCmdline = FALSE;
                }
                break;

            default:
                bValidCmdline = FALSE;
            }
        } else {
            bValidCmdline = FALSE;
        }
    }
    
    if (gCmdLineDeviceID == -1)
	gCmdLineDeviceID = 0;


    if (!bValidCmdline) {
        MessageBoxID(IDS_ERR_CMDLINE, MB_OK|MB_ICONEXCLAMATION);
        return(0);
    }

    if (!vidcapRegisterClasses(hInstance, hPrevInstance)) {

        MessageBoxID(IDS_ERR_REGISTER_CLASS,
#ifdef BIDI
            MB_RTL_READING |
#endif

        MB_ICONEXCLAMATION) ;
        return 0 ;
    }


    if (!vidcapCreateWindows(hInstance, hPrevInstance)) {

        MessageBoxID(IDS_ERR_CREATE_WINDOW,
#ifdef BIDI
                MB_RTL_READING |
#endif

        MB_ICONEXCLAMATION | MB_OK) ;
        return IDS_ERR_CREATE_WINDOW ;
    }

    // Get the default setup for video capture from the AVICap window
    capCaptureGetSetup(ghWndCap, &gCapParms, sizeof(CAPTUREPARMS)) ;

    // Overwrite the defaults with settings we have saved in the profile
    vidcapReadSettingsProfile();

    // Show the main window before connecting the hardware as this can be
    // time consuming and the user should see something happening first...
    ShowWindow(ghWndMain, nCmdShow) ;
    UpdateWindow(ghWndMain) ;
    ghAccel = LoadAccelerators(hInstance, "VIDCAP") ;

    // Create a list of all capture drivers and append them to the Options menu
    if (!(fOK = vidcapEnumerateDrivers(ghWndMain))) {
	LoadString(ghInstApp, IDS_ERR_FIND_HARDWARE, gachLastError, sizeof(gachLastError));
    }
    // Try to connect to a capture driver
    else if (fOK = vidcapInitHardware(ghWndMain, ghWndCap, 
			       bValidCmdline ? gCmdLineDeviceID : 0)) {
	// Hooray, we now have a capture driver connected!
        vidcapSetCaptureFile(gachCaptureFile);
    }
    
    if (!fOK) {
        if (!DoDialog(ghWndMain, IDD_NoCapHardware, NoHardwareDlgProc,
                        (LONG_PTR) (LPSTR) gachLastError)) {
            // The user has asked to abort, since no driver was available
            PostMessage(ghWndMain, WM_COMMAND,
                        GET_WM_COMMAND_MPS(IDM_F_EXIT, 0, 0));
        }
    }
    
    // All set; get and process messages
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (! TranslateAccelerator(ghWndMain, ghAccel, &msg)) {
            TranslateMessage(&msg) ;
            DispatchMessage(&msg) ;
        }
    }

    return (int) msg.wParam;
}  // End of WinMain


BOOL
vidcapRegisterClasses(HINSTANCE hInstance, HINSTANCE hPrevInstance)
{
    WNDCLASS wc;

    if (! hPrevInstance) {
        // If it's the first instance, register the window class
        wc.lpszClassName = gachAppName ;
        wc.hInstance     = hInstance ;
        wc.lpfnWndProc   = MainWndProc ;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW) ;
        wc.hIcon         = LoadIcon(hInstance, gachIconName) ;
        wc.lpszMenuName  = gachMenuName ;
        wc.hbrBackground = GetStockObject(WHITE_BRUSH) ;
        wc.style         = CS_HREDRAW | CS_VREDRAW ;
        wc.cbClsExtra    = 0 ;
        wc.cbWndExtra    = 0 ;

        if (!RegisterClass(&wc)) {
            return(FALSE);
        }

        if (!ArrowInit(hInstance)) {
            return(FALSE);
        }

        if (!RLMeter_Register(hInstance)) {
            return(FALSE);
        }
    }

    if (!toolbarInit(hInstance, hPrevInstance)) {
        return(FALSE);
    }

    if (!statusInit(hInstance, hPrevInstance)) {
        return(FALSE);
    }
    return(TRUE);

}

BOOL
vidcapCreateWindows(HINSTANCE hInstance, HINSTANCE hPrevInstance)
{

    POINT pt;
    RECT rc;
    TOOLBUTTON tb;
    int i;

    // Create Application's Main window
    ghWndMain = CreateWindowEx(
            gfIsRTL ? WS_EX_LEFTSCROLLBAR | WS_EX_RIGHT | WS_EX_RTLREADING : 0,
            gachAppName,
            gachAppTitle,
            WS_CAPTION      |
            WS_SYSMENU      |
            WS_MINIMIZEBOX  |
            WS_MAXIMIZEBOX  |
            WS_THICKFRAME   |
            WS_CLIPCHILDREN |
            WS_OVERLAPPED,
            gWinX, gWinY,
            gWinCX, gWinCY,
            NULL,
            NULL,
            hInstance,
            0) ;

    if (ghWndMain == NULL) {
        return(FALSE);
    }


    /*
     * create a vidframe child window - this will create a child
     * AVICAP window within itself.
     *
     * Don't worry about size and position - vidcapLayout will do this
     * later (once we know the video format size).
     */
    ghWndFrame = vidframeCreate(
                    ghWndMain,
                    hInstance,
                    hPrevInstance,
                    0, 0, 0, 0,
                    &ghWndCap);

    if ((ghWndFrame == NULL) || (ghWndCap == NULL)) {
        return(FALSE);
    }

    // Register the status and error callbacks before driver connects
    // so we can get feedback about the connection process
    fpErrorCallback = MakeProcInstance((FARPROC)ErrorCallbackProc, ghInstApp) ;
    capSetCallbackOnError(ghWndCap, fpErrorCallback) ;

    fpStatusCallback = MakeProcInstance((FARPROC)StatusCallbackProc, ghInstApp) ;
    capSetCallbackOnStatus(ghWndCap, fpStatusCallback) ;

    // We'll only install a yield callback later if using Scrncap.drv
    fpYieldCallback = MakeProcInstance((FARPROC)YieldCallbackProc, ghInstApp) ;
    

    /*
     * CREATE THE TOOL BAR WINDOW
     */
    /* NOTE: let vidcapLayout() position it */
    ghWndToolBar = CreateWindowEx(
            gfIsRTL ? WS_EX_LEFTSCROLLBAR | WS_EX_RIGHT | WS_EX_RTLREADING : 0,
            szToolBarClass,
            NULL,
            WS_CHILD|WS_BORDER|WS_VISIBLE|WS_TABSTOP|
            WS_CLIPSIBLINGS,
            0, 0,
            0, 0,
            ghWndMain,
            NULL,
            hInstance,
            NULL);


    if (ghWndToolBar == NULL) {
        return(FALSE);
    }

    /* set the bitmap and button size to be used for this toolbar */
    pt.x = BUTTONWIDTH;
    pt.y = BUTTONHEIGHT;
    toolbarSetBitmap(ghWndToolBar, hInstance, IDBMP_TOOLBAR, pt);

    for (i = 0; i < APP_NUMTOOLS; i++) {
	rc.left = aPos[i];
	rc.top = 2;
	rc.right = rc.left + pt.x;
	rc.bottom = rc.top + pt.y;
	tb.rc = rc;
	tb.iButton = aiButton[i];
	tb.iState = aiState[i];
	tb.iType = aiType[i];
	tb.iString = aiString[i];
	toolbarAddTool(ghWndToolBar, tb);
    }

    // create the status bar - let vidcapLayout do the positioning
    ghWndStatus = CreateWindowEx(
                    gfIsRTL ? WS_EX_LEFTSCROLLBAR | WS_EX_RIGHT | WS_EX_RTLREADING : 0,
                    szStatusClass,
                    NULL,
                    WS_CHILD|WS_BORDER|WS_VISIBLE|WS_CLIPSIBLINGS,
                    0, 0,
                    0, 0,
                    ghWndMain,
                    NULL,
                    hInstance,
                    NULL);
    if (ghWndStatus == NULL) {
        return(FALSE);
    }

    return(TRUE);

}

/*
 * Enumerate the potential capture drivers and add the list to the Options
 * menu.  This function is only called once at startup.
 * Returns FALSE if no drivers are available.
 */
BOOL
vidcapEnumerateDrivers(HWND hwnd)
{
    TCHAR   achDeviceVersion[80] ;
    TCHAR   achDeviceAndVersion[160] ;
    UINT    uIndex ;
    HMENU   hMenuSub;

    gDriverCount = 0 ;

    hMenuSub = GetSubMenu (GetMenu (hwnd), 2);  // Options menu

    for (uIndex = 0 ; uIndex < MAXVIDDRIVERS ; uIndex++) {
        if (capGetDriverDescription(uIndex,
                       (LPSTR)achDeviceAndVersion, sizeof(achDeviceAndVersion),
                       (LPSTR)achDeviceVersion, sizeof(achDeviceVersion))) {
            // Concatenate the device name and version strings
            lstrcat (achDeviceAndVersion, ",   ");
            lstrcat (achDeviceAndVersion, achDeviceVersion);

            AppendMenu (hMenuSub, 
                        MF_STRING,
                        IDM_O_DRIVER0 + uIndex, 
                        achDeviceAndVersion);
            gDriverCount++;
        }
        else
            break;
    }		 

    // Now refresh menu, position capture window, start driver etc
    DrawMenuBar(ghWndMain) ;

    return (gDriverCount);
}

/*
 * Connect the capture window to a capture driver.
 * uIndex specifies the index of the driver to use.
 * Returns TRUE on success, or FALSE if the driver connection failed.
 */
BOOL
vidcapInitHardware(HWND hwnd, HWND hwndCap, UINT uIndex)
{
    UINT    uError ;
    UINT    uI;
    HMENU   hMenu;
    TCHAR   szName[MAX_PATH];
    TCHAR   szVersion[MAX_PATH];

    // Since the driver may not provide a reliable error string
    // provide a default
    LoadString(ghInstApp, IDS_ERR_FIND_HARDWARE, gachLastError, sizeof(gachLastError));

    // Try connecting to the capture driver
    if (uError = capDriverConnect(hwndCap, uIndex)) {
        gbHaveHardware = TRUE;
        gwDeviceIndex = (WORD) uIndex;
    }
    else {
        gbHaveHardware = FALSE;
        gbLive = FALSE;
        gbOverlay = FALSE;
    }

    // Get the capabilities of the capture driver
    capDriverGetCaps(hwndCap, &gCapDriverCaps, sizeof(CAPDRIVERCAPS)) ;

    // Get the settings for the capture window
    capGetStatus(hwndCap, &gCapStatus , sizeof(gCapStatus));

    // Modify the toolbar buttons
    toolbarModifyState(ghWndToolBar, BTN_CAPFRAME, 
        gbHaveHardware ? BTNST_UP : BTNST_GRAYED);
    toolbarModifyState(ghWndToolBar, BTN_CAPSEL, 
        gbHaveHardware ? BTNST_UP : BTNST_GRAYED);
    toolbarModifyState(ghWndToolBar, BTN_CAPAVI, 
        gbHaveHardware ? BTNST_UP : BTNST_GRAYED);
    toolbarModifyState(ghWndToolBar, BTN_LIVE, 
        gbHaveHardware ? BTNST_UP : BTNST_GRAYED);

    // Is overlay supported?
    toolbarModifyState(ghWndToolBar, BTN_OVERLAY, 
        (gbHaveHardware && gCapDriverCaps.fHasOverlay) ? 
        BTNST_UP : BTNST_GRAYED);

    // Can the device create palettes?
    toolbarModifyState(ghWndToolBar, BTN_CAPPAL, 
        (gbHaveHardware && gCapDriverCaps.fDriverSuppliesPalettes) ? 
        BTNST_UP : BTNST_GRAYED);

    // Check the appropriate driver in the Options menu
    hMenu = GetMenu (hwnd);
    for (uI = 0; uI < gDriverCount; uI++) {
        CheckMenuItem (hMenu, IDM_O_DRIVER0 + uI, 
                MF_BYCOMMAND | ((uIndex == uI) ? MF_CHECKED : MF_UNCHECKED));
    } 

    // Unlike all other capture drivers, Scrncap.drv needs to use
    // a Yield callback, and we don't want to abort on mouse clicks,
    // so determine if the current driver is Scrncap.drv
    capGetDriverDescription (uIndex, 
                szName, sizeof (szName),
                szVersion, sizeof (szVersion));

    // Set a flag if we're using Scrncap.drv
    gbIsScrncap = (_fstrstr (szName, "Screen Capture") != NULL);

    // Get video format and adjust capture window
    vidcapLayout(ghWndMain);
    InvalidateRect(ghWndMain, NULL, TRUE);

    // set the preview rate (units are millisecs)
    capPreviewRate(hwndCap, gbHaveHardware ? 33 : 0); 

    // set live/overlay to default
    vidcapSetLive(gbLive);
    vidcapSetOverlay(gbOverlay);

    strcat (szName, ",   ");
    strcat (szName, szVersion);

    statusUpdateStatus(ghWndStatus, 
        gbHaveHardware ? szName : gachLastError);

    return gbHaveHardware;
}


/*
 * layout the main window. Put the toolbar at the top and the status
 * line at the bottom, and then give all the rest to vidframe,
 *  - it will centre or scroll the AVICAP window appropriately.
 */
void
vidcapLayout(HWND hwnd)
{
    RECT rc;
    RECT rw;
    int cy;
    int cyBorder, cxBorder;
    int cyTotal;
    int cxToolbar;
    int cyMenuAndToolbarAndCaption;

    gbInLayout = TRUE;  // So that we process WM_GETMINMAXINFO normally

    /* for both the toolbar and status bar window,
     * we want just one of the four borders. We do this
     * by setting the WS_BORDER style, and sizing and positioning
     * the window so that the 3 unwanted borders are outside the parent.
     */
    cyBorder = GetSystemMetrics(SM_CYBORDER);
    cxBorder = GetSystemMetrics(SM_CXBORDER);

    // Figure out the height of the menu, toolbar, and caption
    GetWindowRect (hwnd, &rw);
    GetClientRect (hwnd, &rc);

    ClientToScreen (hwnd, (LPPOINT) &rc);
    cyMenuAndToolbarAndCaption = (rc.top - rw.top) + TOOLBAR_HEIGHT;

    cxToolbar = aPos[APP_NUMTOOLS - 1] + BUTTONWIDTH * 3;

    if (gbAutoSizeFrame && gbHaveHardware && gCapStatus.uiImageWidth) {
        cyTotal = gCapStatus.uiImageHeight +
                cyMenuAndToolbarAndCaption +
                (gbStatusBar ? statusGetHeight() : 0) +
                cyBorder * 2 + 
                12;     // vidFrame height
        // Never make the frame smaller than the toolbar
        if (gCapStatus.uiImageWidth >= (UINT) cxToolbar) {
            SetWindowPos(
                hwnd,
                0,	// placement-order handle
                0,	// horizontal position
                0,	// vertical position
                gCapStatus.uiImageWidth + cxBorder * 24,	// width
                cyTotal,	// height
                SWP_NOZORDER | SWP_NOMOVE 	// window-positioning flags
                );
        } else {
            SetWindowPos(
                hwnd,
                0,	// placement-order handle
                0,	// horizontal position
                0,	// vertical position
                cxToolbar,	// width
                cyTotal,	// height
                SWP_NOZORDER | SWP_NOMOVE 	// window-positioning flags
                );
        }
    }

    GetClientRect(hwnd, &rc);

    if (gbToolBar) {
        // put the toolbar at the top - in fact, just off the top so as to
        // hide it's border
        MoveWindow(
            ghWndToolBar,
            -cxBorder, -cyBorder,
            RECTWIDTH(rc)+ (cxBorder * 2),
            TOOLBAR_HEIGHT,
            TRUE);
        rc.top += (TOOLBAR_HEIGHT - cyBorder);
    } else {
        MoveWindow(ghWndToolBar, 0, 0, 0, 0, TRUE);
    }

    // status bar at the bottom
    if (gbStatusBar) {
        cy = statusGetHeight() + cyBorder;
        MoveWindow(
            ghWndStatus,
            -cxBorder, rc.bottom - cy,
            RECTWIDTH(rc) + (2 * cxBorder), cy + cyBorder,
            TRUE);
        rc.bottom -= cy;
    } else {
        MoveWindow(ghWndStatus, 0, 0, 0, 0, TRUE);
    }

    // rest of window goes to vidframe window
    MoveWindow(
        ghWndFrame,
        rc.left, rc.top,
        RECTWIDTH(rc), RECTHEIGHT(rc),
        TRUE);

    // Always layout the frame window, since it is aligned on a
    // DWORD boundary for maximum codec drawing efficiency
    vidframeLayout(ghWndFrame, ghWndCap);

    gbInLayout = FALSE; 
}

/*
 * initialise settings from the profile used before window creation time
 */
void
vidcapReadProfile(void)
{
    // read defaults out of the registry
    gbCentre = mmGetProfileFlag(gachAppTitle, "CenterImage", TRUE);
    gbToolBar = mmGetProfileFlag(gachAppTitle, "ToolBar", TRUE);
    gbStatusBar = mmGetProfileFlag(gachAppTitle, "StatusBar", TRUE);
    gbAutoSizeFrame = mmGetProfileFlag(gachAppTitle, "AutoSizeFrame", TRUE);
    gBackColour = mmGetProfileInt(gachAppTitle, "BackgroundColor", IDD_PrefsLtGrey);

    gWinX = mmGetProfileInt(gachAppTitle, "WindowXPos", (UINT) CW_USEDEFAULT);
	if (gWinX != (UINT) CW_USEDEFAULT)
    	gWinX = LimitRange(gWinX, 0, GetSystemMetrics (SM_CXSCREEN) - 40);
    gWinY = mmGetProfileInt(gachAppTitle, "WindowYPos", 0);
    gWinY = LimitRange(gWinY, 0, GetSystemMetrics (SM_CYSCREEN) - 40);
    gWinCX = mmGetProfileInt(gachAppTitle, "WindowWidth", 320);
    gWinCX = LimitRange(gWinCX, 20, GetSystemMetrics (SM_CXSCREEN));
    gWinCY = mmGetProfileInt(gachAppTitle, "WindowHeight", 240);
    gWinCY = LimitRange(gWinCY, 20, GetSystemMetrics (SM_CYSCREEN));
    gWinShow = mmGetProfileInt(gachAppTitle, "WindowShow", SW_SHOWDEFAULT);
    gWinShow = LimitRange(gWinShow, SW_SHOWNORMAL, SW_SHOWDEFAULT);

    gbOverlay = mmGetProfileInt(gachAppTitle, "OverlayWindow", FALSE);
    gbLive = mmGetProfileInt(gachAppTitle, "LiveWindow", TRUE);
}


void
vidcapWriteProfile(void)
{
    mmWriteProfileFlag(gachAppTitle, "CenterImage", gbCentre, TRUE);
    mmWriteProfileFlag(gachAppTitle, "ToolBar", gbToolBar, TRUE);
    mmWriteProfileFlag(gachAppTitle, "StatusBar", gbStatusBar, TRUE);
    mmWriteProfileFlag(gachAppTitle, "AutoSizeFrame", gbAutoSizeFrame, TRUE);
    mmWriteProfileInt(gachAppTitle,  "BackgroundColor", gBackColour, IDD_PrefsLtGrey);

    mmWriteProfileInt(gachAppTitle, "WindowXPos", gWinX, (UINT) CW_USEDEFAULT);
    mmWriteProfileInt(gachAppTitle, "WindowYPos", gWinY, 0);
    mmWriteProfileInt(gachAppTitle, "WindowWidth", gWinCX, 320);
    mmWriteProfileInt(gachAppTitle, "WindowHeight", gWinCY, 240);
    mmWriteProfileInt(gachAppTitle, "WindowShow", gWinShow, SW_SHOWDEFAULT);

    mmWriteProfileInt(gachAppTitle, "OverlayWindow", gbOverlay, FALSE);
    mmWriteProfileInt(gachAppTitle, "LiveWindow", gbLive, TRUE);
}

/*
 * initialise settings from the profile used AFTER window creation time
 */
void
vidcapReadSettingsProfile(void)
{
    DWORD dwSize;
    
    mmGetProfileString(gachAppTitle, "CaptureFile", "",
        gachCaptureFile, sizeof(gachCaptureFile));

    mmGetProfileString(gachAppTitle, "MCIDevice", "VideoDisc",
                gachMCIDeviceName, sizeof(gachMCIDeviceName));

    gCapParms.dwRequestMicroSecPerFrame = 
                mmGetProfileInt(gachAppTitle, "MicroSecPerFrame", 
                DEF_CAPTURE_RATE);

    gCapParms.dwRequestMicroSecPerFrame = 
                mmGetProfileInt(gachAppTitle, "MicroSecPerFrame", 
                DEF_CAPTURE_RATE);

    gCapParms.fCaptureAudio = mmGetProfileFlag(gachAppTitle, "CaptureAudio", 
                gCapStatus.fAudioHardware);

    gCapParms.fLimitEnabled = mmGetProfileFlag(gachAppTitle, "LimitEnabled", 
                FALSE);

    gCapParms.wTimeLimit = 
                mmGetProfileInt(gachAppTitle, "TimeLimit", 30);

    gCapParms.fMCIControl= mmGetProfileFlag(gachAppTitle, "MCIControl", FALSE);

    gCapParms.fStepMCIDevice= mmGetProfileFlag(gachAppTitle, "StepMCIDevice", FALSE);

    gCapParms.dwMCIStartTime = 
                mmGetProfileInt(gachAppTitle, "MCIStartTime", 10000);

    gCapParms.dwMCIStopTime = 
                mmGetProfileInt(gachAppTitle, "MCIStopTime", 20000);

    gCapParms.fStepCaptureAt2x = mmGetProfileFlag(gachAppTitle, "StepCapture2x", 
                FALSE);

    gCapParms.wStepCaptureAverageFrames = 
                mmGetProfileInt(gachAppTitle, "StepCaptureAverageFrames", 3);

    gCapParms.AVStreamMaster = mmGetProfileInt (gachAppTitle, "AVStreamMaster",
                AVSTREAMMASTER_AUDIO);

    gCapParms.fUsingDOSMemory = mmGetProfileFlag (gachAppTitle, "CaptureToDisk",
                TRUE);

    gCapParms.dwIndexSize = 
                mmGetProfileInt(gachAppTitle, "IndexSize", 
                CAP_SMALL_INDEX);
    
    // Retrieve the saved audio format
    // Ask the ACM what the largest known wave format is
    acmMetrics(NULL,
               ACM_METRIC_MAX_SIZE_FORMAT,
               &dwSize);

    // If a wave format was saved in the registry, use that size
    dwSize = max (dwSize, mmGetProfileBinary(gachAppTitle, "WaveFormatBinary",
			   NULL,
			   NULL,
			   0));
		  
    if (glpwfex = (LPWAVEFORMATEX) GlobalAllocPtr(GHND, dwSize)) {
	capGetAudioFormat(ghWndCap, glpwfex, (WORD)dwSize) ;
	mmGetProfileBinary(gachAppTitle, "WaveFormatBinary",
			   glpwfex,
			   glpwfex,
			   dwSize);

	// Do some sanity checking
	if (MMSYSERR_NOERROR == waveInOpen (NULL, WAVE_MAPPER,
					    glpwfex, 0, 0, WAVE_FORMAT_QUERY)) {
	    capSetAudioFormat(ghWndCap, glpwfex, (WORD)dwSize) ;
	} 
	GlobalFreePtr(glpwfex) ;
    }
}


void
vidcapWriteSettingsProfile(void)
{
    mmWriteProfileString(gachAppTitle, "CaptureFile", gachCaptureFile);

    mmWriteProfileString(gachAppTitle, "MCIDevice", gachMCIDeviceName);

    mmWriteProfileInt(gachAppTitle, "MicroSecPerFrame", 
                gCapParms.dwRequestMicroSecPerFrame, DEF_CAPTURE_RATE);

    mmWriteProfileFlag(gachAppTitle, "CaptureAudio", 
                gCapParms.fCaptureAudio, gCapStatus.fAudioHardware);

    mmWriteProfileFlag(gachAppTitle, "LimitEnabled", 
                gCapParms.fLimitEnabled, FALSE);

    mmWriteProfileInt(gachAppTitle, "TimeLimit", 
                gCapParms.wTimeLimit, 30);

    mmWriteProfileFlag(gachAppTitle, "MCIControl", 
                gCapParms.fMCIControl, FALSE);

    mmWriteProfileFlag(gachAppTitle, "StepMCIDevice", 
                gCapParms.fStepMCIDevice, FALSE);

    mmWriteProfileInt(gachAppTitle, "MCIStartTime", 
                gCapParms.dwMCIStartTime, 10000);

    mmWriteProfileInt(gachAppTitle, "MCIStopTime", 
                gCapParms.dwMCIStopTime, 20000);

    mmWriteProfileFlag(gachAppTitle, "StepCapture2x", 
                gCapParms.fStepCaptureAt2x, FALSE);

    mmWriteProfileInt(gachAppTitle, "StepCaptureAverageFrames", 
                gCapParms.wStepCaptureAverageFrames, 3);

    mmWriteProfileInt(gachAppTitle, "AVStreamMaster", 
                gCapParms.AVStreamMaster, AVSTREAMMASTER_AUDIO);

    mmWriteProfileFlag(gachAppTitle, "CaptureToDisk", 
                gCapParms.fUsingDOSMemory, TRUE);

    mmWriteProfileInt(gachAppTitle, "IndexSize", 
                gCapParms.dwIndexSize, CAP_SMALL_INDEX);

    // The audio format is written whenever it is changed via dlg
}




/* --- error/status functions -------------------------------------------*/

/*
 * put up a message box loading a string from the
 * resource file
 */
int
MessageBoxID(UINT idString, UINT fuStyle)
{
    TCHAR achMessage[256];   // max message length

    LoadString(ghInstApp, idString, achMessage, sizeof(achMessage));

    return MessageBox(ghWndMain, achMessage, gachAppTitle, fuStyle);
}



//
// ErrorCallbackProc: Error Callback Function
//
LRESULT FAR PASCAL ErrorCallbackProc(HWND hWnd, int nErrID, LPSTR lpErrorText)
{
////////////////////////////////////////////////////////////////////////
//  hWnd:          Application main window handle
//  nErrID:        Error code for the encountered error
//  lpErrorText:   Error text string for the encountered error
////////////////////////////////////////////////////////////////////////

    if (!ghWndMain)
        return FALSE;

    if (nErrID == 0)            // Starting a new major function
        return TRUE;            // Clear out old errors...

    // save the error message for use in NoHardwareDlgProc
    lstrcpy(gachLastError, lpErrorText);

    // Show the error ID and text

    MessageBox(hWnd, lpErrorText, gachAppTitle,
#ifdef BIDI
                MB_RTL_READING |
#endif
                MB_OK | MB_ICONEXCLAMATION) ;

    return (LRESULT) TRUE ;
}


//
// StatusCallbackProc: Status Callback Function
//
LRESULT FAR PASCAL StatusCallbackProc(HWND hWnd, int nID, LPSTR lpStatusText)
{
////////////////////////////////////////////////////////////////////////
//  hWnd:           Application main window handle
//  nID:            Status code for the current status
//  lpStatusText:   Status text string for the current status
////////////////////////////////////////////////////////////////////////

    static int CurrentID;

    if (!ghWndMain) {
        return FALSE;
    }

    // the CAP_END message sometimes overwrites a useful
    // statistics message.
    if (nID == IDS_CAP_END) {
        if ((CurrentID == IDS_CAP_STAT_VIDEOAUDIO) ||
            (CurrentID == IDS_CAP_STAT_VIDEOONLY)) {

            return(TRUE);
        }
    }
    CurrentID = nID;


    statusUpdateStatus(ghWndStatus, lpStatusText);

    return (LRESULT) TRUE ;
}


//
// YieldCallbackProc: Status Callback Function
// (Only used for Scrncap.drv driver)
//
LRESULT FAR PASCAL YieldCallbackProc(HWND hWnd)
{
////////////////////////////////////////////////////////////////////////
//  hWnd:           Application main window handle
////////////////////////////////////////////////////////////////////////

    MSG msg;

    if (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Return TRUE to continue capturing
    return (LRESULT) TRUE;
}



/*
 * load a string from the string table and return
 * a pointer to it for temporary use. Each call
 * overwrites the previous
 */
LPTSTR
tmpString(UINT idString)
{
    static TCHAR ach[350];

    LoadString(ghInstApp, idString, ach, sizeof(ach));

    // ensure null terminated
    ach[sizeof(ach) -1] = 0;

    return(ach);
}




/* --- connect to and init hardware ------------------------------------- */


void
vidcapSetLive(BOOL bLive)
{
    capPreview(ghWndCap, bLive);
    toolbarModifyState(ghWndToolBar, BTN_LIVE, bLive? BTNST_DOWN : BTNST_UP);
    CheckMenuItem(GetMenu(ghWndMain), IDM_O_PREVIEW,
        MF_BYCOMMAND | (bLive ? MF_CHECKED : MF_UNCHECKED));

    gbLive = bLive;

    if (bLive == TRUE) {
        vidcapSetOverlay(FALSE);
    }
}

void
vidcapSetOverlay(BOOL bOverlay)
{
    if (!gCapDriverCaps.fHasOverlay) {
        CheckMenuItem(GetMenu(ghWndMain), IDM_O_OVERLAY,
            MF_BYCOMMAND | MF_UNCHECKED);
        gbOverlay = FALSE;
        return;
    }

    capOverlay(ghWndCap, bOverlay);
    toolbarModifyState(ghWndToolBar, BTN_OVERLAY, bOverlay ? BTNST_DOWN : BTNST_UP);
    CheckMenuItem(GetMenu(ghWndMain), IDM_O_OVERLAY,
        MF_BYCOMMAND | (bOverlay ? MF_CHECKED : MF_UNCHECKED));

    gbOverlay = bOverlay;

    if (bOverlay == TRUE) {
        vidcapSetLive(FALSE);
    }
}

void
vidcapSetCaptureFile(LPTSTR pFileName)
{
    TCHAR achBuffer[_MAX_PATH];

    if ((pFileName != NULL) && (lstrlen(pFileName)  > 0)) {
        // record the capture filename
        if (lstrcmp(gachCaptureFile, pFileName)) {
            lstrcpy(gachCaptureFile, pFileName);
        }

        // and set window title
        wsprintf(achBuffer, "%s - %s", gachAppTitle, pFileName);
    } else {
        gachCaptureFile[0] = 0;
        lstrcpy(achBuffer, gachAppTitle);
    }

    capFileSetCaptureFile(ghWndCap, gachCaptureFile);
    SetWindowText(ghWndMain, achBuffer);
}

/* --- winproc and message handling --------------------------------------- */

/*
 * called from WM_COMMAND processing if the
 * message is from the toolbar. iButton contains the
 * button ID in the lower 8 bits, and the flags in the upper 8 bits/
 */
LONG FAR PASCAL
toolbarCommand (HWND hWnd, int iButton, HWND hwndToolbar)
{
    int iBtnPos, iState, iActivity, iString;


    // check repeat bit
    if (iButton & BTN_REPEAT) {
        return(0);
    }
    iButton &= 0xff;

    iBtnPos = toolbarIndexFromButton(hwndToolbar, iButton);
    iState = toolbarStateFromButton(hwndToolbar, iButton);
    iActivity = toolbarActivityFromButton(hwndToolbar, iButton);
    iString = toolbarStringFromIndex(hwndToolbar, iBtnPos);

    switch(iActivity) {

    case BTNACT_MOUSEDOWN:
    case BTNACT_KEYDOWN:
    case BTNACT_MOUSEMOVEON:
        statusUpdateStatus(ghWndStatus, MAKEINTRESOURCE(iString));
        break;

    case BTNACT_MOUSEMOVEOFF:
        statusUpdateStatus(ghWndStatus, NULL);
        break;

    case BTNACT_MOUSEUP:
    case BTNACT_KEYUP:

        statusUpdateStatus(ghWndStatus, NULL);
        switch(iButton) {
        case BTN_SETFILE:
            SendMessage(hWnd, WM_COMMAND,
                GET_WM_COMMAND_MPS(IDM_F_SETCAPTUREFILE, NULL, 0));
                break;

        case BTN_EDITCAP:
            // edit captured video
            SendMessage(hWnd, WM_COMMAND,
                GET_WM_COMMAND_MPS(IDM_F_EDITVIDEO, NULL, 0));
            break;

        case BTN_LIVE:
            SendMessage(hWnd,WM_COMMAND,
                GET_WM_COMMAND_MPS(IDM_O_PREVIEW, NULL, 0));
            break;

        case BTN_CAPFRAME:
            SendMessage(hWnd, WM_COMMAND,
                GET_WM_COMMAND_MPS(IDM_C_CAPTUREFRAME, NULL, 0));
            break;

        case BTN_CAPSEL:
            // capture selected frames
            SendMessage(hWnd, WM_COMMAND,
                GET_WM_COMMAND_MPS(IDM_C_CAPSEL, NULL, 0));
            break;

        case BTN_CAPAVI:
            SendMessage(hWnd,WM_COMMAND,
                GET_WM_COMMAND_MPS(IDM_C_CAPTUREVIDEO, NULL, 0));
            break;

        case BTN_CAPPAL:
            SendMessage(hWnd, WM_COMMAND,
                GET_WM_COMMAND_MPS(IDM_C_PALETTE, NULL, 0));
            break;

        case BTN_OVERLAY:
            SendMessage(hWnd, WM_COMMAND,
                GET_WM_COMMAND_MPS(IDM_O_OVERLAY, NULL, 0));
            break;
        }
        break;
    }
    return(0);
}


/*
 * Put up a dialog to allow the user to select a capture file.
 */
LONG FAR PASCAL
cmdSetCaptureFile(HWND hWnd)
{
    OPENFILENAME ofn ;
    LPSTR p;
    TCHAR        achFileName[_MAX_PATH];
    TCHAR        achBuffer[_MAX_PATH] ;
    UINT         wError ;
    HANDLE hFilter;
    int oldhelpid;


    // Get current capture file name and
    // then try to get the new capture file name
    if (wError = capFileGetCaptureFile(ghWndCap, achFileName,
                                sizeof(achFileName))) {

        // Get just the path info
        // Terminate the full path at the last backslash
        lstrcpy (achBuffer, achFileName);
        for (p = achBuffer + lstrlen(achBuffer); p > achBuffer; p--) {
            if (*p == '\\') {
                *(p+1) = '\0';
                break;
            }
        }

        _fmemset(&ofn, 0, sizeof(OPENFILENAME)) ;
        ofn.lStructSize = sizeof(OPENFILENAME) ;
        ofn.hwndOwner = hWnd ;

        //load filters from resource stringtable
        hFilter = FindResource(ghInstApp, MAKEINTRESOURCE(ID_FILTER_AVI), RT_RCDATA);
        if ((hFilter = LoadResource(ghInstApp, hFilter)) == NULL) {
            ofn.lpstrFilter = NULL;
        } else {
            ofn.lpstrFilter = LockResource(hFilter);
        }

        ofn.nFilterIndex = 0 ;
        ofn.lpstrFile = achFileName ;
        ofn.nMaxFile = sizeof(achFileName) ;
        ofn.lpstrFileTitle = NULL;
        ofn.lpstrTitle = tmpString(IDS_TITLE_SETCAPTUREFILE);
        ofn.nMaxFileTitle = 0 ;
        ofn.lpstrInitialDir = achBuffer;
        ofn.Flags =
#ifdef BIDI
        OFN_BIDIDIALOG |
#endif
        OFN_HIDEREADONLY |
        OFN_NOREADONLYRETURN |
        OFN_PATHMUSTEXIST ;

        // set help context for dialog
        oldhelpid = SetCurrentHelpContext(IDA_SETCAPFILE);

        if (GetOpenFileName(&ofn)) {
            OFSTRUCT os;

            vidcapSetCaptureFile(achFileName);


            /*
             * if this is a new file, then invite the user to
             * allocate some space
             */
            if (OpenFile(achFileName, &os, OF_EXIST) == HFILE_ERROR) {

                /*
                 * show the allocate file space dialog to encourage
                 * the user to pre-allocate space
                 */
                if (DoDialog(hWnd, IDD_AllocCapFileSpace, AllocCapFileProc, 0)) {

		    // ensure repaint after dismissing dialog before
		    // possibly lengthy operation
		    UpdateWindow(ghWndMain);

                    // If user has hit OK then alloc requested capture file space
                    if (! capFileAlloc(ghWndCap, (long) gwCapFileSize * ONEMEG)) {
                        MessageBoxID(IDS_ERR_CANT_PREALLOC,
#ifdef BIDI
                                    MB_RTL_READING |
#endif
                                    MB_OK | MB_ICONEXCLAMATION) ;
                    }
                }
            }

        }

        // restore old help context
        SetCurrentHelpContext(oldhelpid);

        if (hFilter) {
            UnlockResource(hFilter);
        }

    }
    return(0);
}

/*
 * query the user for a filename, and then save the captured video
 * to that file
 */
LONG FAR PASCAL
cmdSaveVideoAs(HWND hWnd)
{
    OPENFILENAME ofn ;
    TCHAR        achFileName[_MAX_PATH];
    UINT         wError ;
    HANDLE       hFilter;
    int          oldhelpid;



    // Get the current capture file name and
    // then get the substitute file name to save video in
    if (wError = capFileGetCaptureFile(ghWndCap, achFileName, sizeof(achFileName))) {

        _fmemset(&ofn, 0, sizeof(OPENFILENAME)) ;
        ofn.lStructSize = sizeof(OPENFILENAME) ;
        ofn.hwndOwner = hWnd ;

        //load filters from resource stringtable
        hFilter = FindResource(ghInstApp, MAKEINTRESOURCE(ID_FILTER_AVI), RT_RCDATA);
        if ((hFilter = LoadResource(ghInstApp, hFilter)) == NULL) {
            ofn.lpstrFilter = NULL;
        } else {
            ofn.lpstrFilter = LockResource(hFilter);
        }

        ofn.nFilterIndex = 0 ;
        ofn.lpstrFile = achFileName ;
        ofn.nMaxFile = sizeof(achFileName) ;
        ofn.lpstrFileTitle = NULL ;
        ofn.lpstrTitle = tmpString(IDS_TITLE_SAVEAS);
        ofn.nMaxFileTitle = 0 ;
        ofn.lpstrInitialDir = NULL ;
        ofn.Flags =
#ifdef BIDI
        OFN_BIDIDIALOG |
#endif
        OFN_OVERWRITEPROMPT |  OFN_PATHMUSTEXIST ;


        // set help context
        oldhelpid = SetCurrentHelpContext(IDA_SAVECAPFILE);

        if (GetSaveFileName(&ofn)) {
            // If the user has hit OK then set save file name
            capFileSaveAs(ghWndCap, achFileName) ;
        }

        SetCurrentHelpContext(oldhelpid);

        if (hFilter) {
            UnlockResource(hFilter);
        }
    }
    return(0);
}


/*
 * Put up a dialog to allow the user to select a palette file and then
 * load that palette
 */
LONG FAR PASCAL
cmdLoadPalette(HWND hWnd)
{
    OPENFILENAME ofn ;
    TCHAR        achFileName[_MAX_PATH];
    HANDLE       hFilter;
    int          oldhelpid;



    achFileName[0] = 0;

    _fmemset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;

    //load filters from resource stringtable
    hFilter = FindResource(ghInstApp, MAKEINTRESOURCE(ID_FILTER_PALETTE), RT_RCDATA);
    if ((hFilter = LoadResource(ghInstApp, hFilter)) == NULL) {
        ofn.lpstrFilter = NULL;
    } else {
        ofn.lpstrFilter = LockResource(hFilter);
    }

    ofn.nFilterIndex = 1;
    ofn.lpstrFile = achFileName;
    ofn.nMaxFile = sizeof(achFileName);
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrTitle = tmpString(IDS_TITLE_LOADPALETTE);
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags =
#ifdef BIDI
    OFN_BIDIDIALOG |
#endif
    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;


    // set help context id
    oldhelpid = SetCurrentHelpContext(IDA_LOADPAL);

    if (GetOpenFileName(&ofn)) {
        // If the user has hit OK then load palette
        capPaletteOpen(ghWndCap, achFileName);
    }

    SetCurrentHelpContext(oldhelpid);

    if (hFilter) {
        UnlockResource(hFilter);
    }
    return(0);
}

/*
 * query the user for a filename, and then save the current palette
 * to that file
 */
LONG FAR PASCAL
cmdSavePalette(HWND hWnd)
{
    OPENFILENAME ofn ;
    TCHAR        achFileName[_MAX_PATH];
    HANDLE       hFilter;
    int          oldhelpid;


    achFileName[0] = 0;

    _fmemset(&ofn, 0, sizeof(OPENFILENAME)) ;
    ofn.lStructSize = sizeof(OPENFILENAME) ;
    ofn.hwndOwner = hWnd ;

    //load filters from resource stringtable
    hFilter = FindResource(ghInstApp, MAKEINTRESOURCE(ID_FILTER_PALETTE), RT_RCDATA);
    if ((hFilter = LoadResource(ghInstApp, hFilter)) == NULL) {
        ofn.lpstrFilter = NULL;
    } else {
        ofn.lpstrFilter = LockResource(hFilter);
    }

    ofn.nFilterIndex = 1;
    ofn.lpstrFile = achFileName;
    ofn.nMaxFile = sizeof(achFileName);
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrTitle = tmpString(IDS_TITLE_SAVEPALETTE);
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags =
#ifdef BIDI
    OFN_BIDIDIALOG |
#endif
    OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    // set help context for F1 key
    oldhelpid = SetCurrentHelpContext(IDA_SAVEPAL);

    if (GetSaveFileName(&ofn)) {
        // If the user has hit OK then set save file name
        capPaletteSave(ghWndCap, achFileName);
    }

    SetCurrentHelpContext(oldhelpid);

    if (hFilter) {
        UnlockResource(hFilter);
    }

    return(0);
}


/*
 * query the user for a filename, and then save the current frame
 * to that file
 */
LONG FAR PASCAL
cmdSaveDIB(HWND hWnd)
{
    OPENFILENAME ofn ;
    TCHAR        achFileName[_MAX_PATH];
    HANDLE       hFilter;
    int          oldhelpid;


    achFileName[0] = 0;

    _fmemset(&ofn, 0, sizeof(OPENFILENAME)) ;
    ofn.lStructSize = sizeof(OPENFILENAME) ;
    ofn.hwndOwner = hWnd ;

    //load filters from resource stringtable
    hFilter = FindResource(ghInstApp, MAKEINTRESOURCE(ID_FILTER_DIB), RT_RCDATA);
    if ((hFilter = LoadResource(ghInstApp, hFilter)) == NULL) {
        ofn.lpstrFilter = NULL;
    } else {
        ofn.lpstrFilter = LockResource(hFilter);
    }

    ofn.nFilterIndex = 1;
    ofn.lpstrFile = achFileName;
    ofn.nMaxFile = sizeof(achFileName);
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrTitle = tmpString(IDS_TITLE_SAVEDIB);
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags =
#ifdef BIDI
    OFN_BIDIDIALOG |
#endif
    OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

    // set help context for F1 handling
    oldhelpid = SetCurrentHelpContext(IDA_SAVEDIB);

    if (GetSaveFileName(&ofn)) {

        // If the user has hit OK then set save file name
        capFileSaveDIB(ghWndCap, achFileName);
    }

    SetCurrentHelpContext(oldhelpid);

    if (hFilter) {
        UnlockResource(hFilter);
    }

    return(0);
}

//
// MenuProc: Processes All Menu-based Operations
//
LRESULT FAR PASCAL MenuProc(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
////////////////////////////////////////////////////////////////////////
//  hWnd:      Application main window handle
//  hMenu:     Application menu handle
//  wParam:    Menu option
//  lParam:    Additional info for any menu option
////////////////////////////////////////////////////////////////////////

    BOOL         fResult ;
    DWORD        dwSize ;
    int          oldhelpid;

    HMENU hMenu = GetMenu(hWnd) ;

    switch (GET_WM_COMMAND_ID(wParam, lParam)) {

	case IDC_TOOLBAR:
            return toolbarCommand(hWnd, GET_WM_COMMAND_CMD(wParam, lParam), ghWndToolBar);

/* --- file --- */
        case IDM_F_SETCAPTUREFILE:
            return cmdSetCaptureFile(hWnd);

        case IDM_F_SAVEVIDEOAS:
            return cmdSaveVideoAs(hWnd);
            break;

        case IDM_F_ALLOCATESPACE:
            if (DoDialog(hWnd, IDD_AllocCapFileSpace, AllocCapFileProc, 0)) {

		// ensure repaint after dismissing dialog before
		// possibly lengthy operation
		UpdateWindow(ghWndMain);


                // If user has hit OK then alloc requested capture file space
                if (! capFileAlloc(ghWndCap, (long) gwCapFileSize * ONEMEG)) {
                    MessageBoxID(IDS_ERR_CANT_PREALLOC,
#ifdef BIDI
                                MB_RTL_READING |
#endif
                                MB_OK | MB_ICONEXCLAMATION) ;
                }
            }
            break ;

        case IDM_F_EXIT:
            DestroyWindow(hWnd) ;
            break;

        case IDM_F_LOADPALETTE:
            return cmdLoadPalette(hWnd);

        case IDM_F_SAVEPALETTE:
            return cmdSavePalette(hWnd);

        case IDM_F_SAVEFRAME:
            return cmdSaveDIB(hWnd);

        case IDM_F_EDITVIDEO:
        {
            HINSTANCE  u;
            BOOL	f = TRUE;	/* assume the best */
            HCURSOR     hOldCursor;

            /* build up the command line "AviEdit -n filename" */
            if (lstrlen(gachCaptureFile) > 0) {

                hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                u = ShellExecute (hWnd, TEXT("open"), gachCaptureFile, NULL, NULL, SW_SHOWNORMAL);
                if ((UINT_PTR) u < 32){
            	/* report error on forking VidEdit */
                    MessageBoxID(IDS_ERR_VIDEDIT, MB_OK|MB_ICONEXCLAMATION);
            	f = FALSE;
                }

                SetCursor(hOldCursor);
            }
            return f;

        }


/* --- edit --- */

        case IDM_E_COPY:
            capEditCopy(ghWndCap) ;
            break;

        case IDM_E_PASTEPALETTE:
            capPalettePaste(ghWndCap) ;
            break;

        case IDM_E_PREFS:
            {
                if (DoDialog(hWnd, IDD_Prefs, PrefsDlgProc, 0)) {

                        // write prefs to profile

                        // force new brush
                        vidframeSetBrush(ghWndFrame, gBackColour);

                        // re-do layout
                        vidcapLayout(hWnd);

                }
            }
            break;

/* --- options --- */

        case IDM_O_PREVIEW:
            // Toggle Preview
    	    capGetStatus(ghWndCap, &gCapStatus, sizeof(CAPSTATUS)) ;
            vidcapSetLive(!gCapStatus.fLiveWindow) ;
            break;

        case IDM_O_OVERLAY:
            // Toggle Overlay
    	    capGetStatus(ghWndCap, &gCapStatus, sizeof(CAPSTATUS)) ;
            vidcapSetOverlay(!gCapStatus.fOverlayWindow);
            break ;

        case IDM_O_AUDIOFORMAT:
#ifdef  USE_ACM
            {
                ACMFORMATCHOOSE cfmt;
                static BOOL fDialogUp = FALSE;

                if (fDialogUp)
                    return FALSE;

                fDialogUp = TRUE;
                // Ask the ACM what the largest wave format is.....
                acmMetrics(NULL,
                            ACM_METRIC_MAX_SIZE_FORMAT,
                            &dwSize);

                // Get the current audio format
                dwSize = max (dwSize, capGetAudioFormatSize (ghWndCap));
                if (glpwfex = (LPWAVEFORMATEX) GlobalAllocPtr(GHND, dwSize)) {
                    capGetAudioFormat(ghWndCap, glpwfex, (WORD)dwSize) ;

		    _fmemset (&cfmt, 0, sizeof (ACMFORMATCHOOSE));
		    cfmt.cbStruct = sizeof (ACMFORMATCHOOSE);
		    cfmt.fdwStyle =  ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT;
		    cfmt.fdwEnum =   ACM_FORMATENUMF_HARDWARE |
				     ACM_FORMATENUMF_INPUT;
		    cfmt.hwndOwner = hWnd;
		    cfmt.pwfx =     glpwfex;
		    cfmt.cbwfx =    dwSize;

		    //oldhelpid = SetCurrentHelpContext(IDA_AUDIOSETUP);
		    if (!acmFormatChoose(&cfmt)) {
			capSetAudioFormat(ghWndCap, glpwfex, (WORD)glpwfex->cbSize +
					  sizeof (WAVEFORMATEX)) ;
			mmWriteProfileBinary(gachAppTitle, "WaveFormatBinary",
					     (LPVOID) glpwfex, glpwfex->cbSize +
					     sizeof (WAVEFORMATEX));
		    }
		    //SetCurrentHelpContext(oldhelpid);

		    GlobalFreePtr(glpwfex) ;
		}
                fDialogUp = FALSE;
            }
#else
            {
                // Get current audio format and then find required format
                dwSize = capGetAudioFormatSize (ghWndCap);  
                glpwfex = (LPWAVEFORMATEX) GlobalAllocPtr(GHND, dwSize) ;
                capGetAudioFormat(ghWndCap, glpwfex, (WORD)dwSize) ;

                if (DoDialog(hWnd, IDD_AudioFormat, AudioFormatProc, 0)) {
                        // If the user has hit OK, set the new audio format
                        capSetAudioFormat(ghWndCap, glpwfex, (WORD)dwSize) ;
			mmWriteProfileBinary(gachAppTitle, "WaveFormatBinary",
					 (LPVOID) glpwfex, dwSize);
                }
                GlobalFreePtr(glpwfex) ;
            }
#endif
            break ;

        case IDM_O_VIDEOFORMAT:
            if (gCapDriverCaps.fHasDlgVideoFormat) {
                // Only if the driver has a "Video Format" dialog box
                oldhelpid = SetCurrentHelpContext(IDA_VIDFORMAT);
                if (capDlgVideoFormat(ghWndCap)) {  // If successful,
                    // Get the new image dimension and center capture window
                    capGetStatus(ghWndCap, &gCapStatus, sizeof(CAPSTATUS)) ;
                    vidcapLayout(hWnd);
                }
                SetCurrentHelpContext(oldhelpid);
            }
            break;

        case IDM_O_VIDEOSOURCE:
            if (gCapDriverCaps.fHasDlgVideoSource) {
                // Only if the driver has a "Video Source" dialog box
                oldhelpid = SetCurrentHelpContext(IDA_VIDSOURCE);
                capDlgVideoSource(ghWndCap) ;
                capGetStatus(ghWndCap, &gCapStatus, sizeof(CAPSTATUS)) ;
                vidcapLayout(hWnd);
                SetCurrentHelpContext(oldhelpid);
            }
            break ;

        case IDM_O_VIDEODISPLAY:
            if (gCapDriverCaps.fHasDlgVideoDisplay) {
                // Only if the driver has a "Video Display" dialog box
                oldhelpid = SetCurrentHelpContext(IDA_VIDDISPLAY);
                capDlgVideoDisplay(ghWndCap) ;
                capGetStatus(ghWndCap, &gCapStatus, sizeof(CAPSTATUS)) ;
                SetCurrentHelpContext(oldhelpid);
            }
            break ;

        case IDM_O_CHOOSECOMPRESSOR:
            oldhelpid = SetCurrentHelpContext(IDA_COMPRESSION);
            capDlgVideoCompression(ghWndCap);
            SetCurrentHelpContext(oldhelpid);
            break;

        // Select a driver to activate
        case IDM_O_DRIVER0:
        case IDM_O_DRIVER1:
        case IDM_O_DRIVER2:
        case IDM_O_DRIVER3:
        case IDM_O_DRIVER4:
        case IDM_O_DRIVER5:
        case IDM_O_DRIVER6:
        case IDM_O_DRIVER7:
        case IDM_O_DRIVER8:
        case IDM_O_DRIVER9:
            vidcapInitHardware(ghWndMain, ghWndCap, (UINT) (wParam - IDM_O_DRIVER0));
            break;

/* --- capture --- */


        case IDM_C_PALETTE:
            if (DoDialog(hWnd, IDD_MakePalette, MakePaletteProc, 0)) {
                // Palette is created within the dialog
                bDefaultPalette = FALSE;
            }
            break;

        case IDM_C_CAPTUREVIDEO:

            // warn user if he is still using the default palette
            if (bDefaultPalette) {

		LPBITMAPINFOHEADER lpbi;
		int sz;

		// fUsingDefaultPalette will be TRUE even if the
		// current capture format is non-palettised. This is a
		// bizarre decision of Jay's.

		sz = (int)capGetVideoFormatSize(ghWndCap);
		lpbi = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, sz);

		if (lpbi) {    // We can warn s/he
		    if (capGetVideoFormat(ghWndCap, lpbi, sz) &&
			(lpbi->biCompression == BI_RGB) &&
			(lpbi->biBitCount <= 8)) {

			CAPSTATUS cs;

			// if we've warned him once, we can forget it
			bDefaultPalette = FALSE;

			capGetStatus(ghWndCap, &cs, sizeof(cs));

			if (cs.fUsingDefaultPalette) {

			    if (MessageBoxID(IDS_WARN_DEFAULT_PALETTE,
       				     MB_OKCANCEL| MB_ICONEXCLAMATION)== IDCANCEL) {
				break;
			    }
			}
		    }
		    LocalFree(lpbi);
		}
            }

            // Invoke a Dlg box to setup all the params
            if (DoDialog(hWnd, IDD_CapSetUp, CapSetUpProc, 0)) {

                // set the defaults we won't bother the user with
                gCapParms.fMakeUserHitOKToCapture = !gCapParms.fMCIControl;
                gCapParms.wPercentDropForError = 10;

                // fUsingDOSMemory is obsolete, but we use it here as
                // a flag which is TRUE if "CapturingToDisk"
                // The number of video buffers should be enough to get through
                // disk seeks and thermal recalibrations if "CapturingToDisk"
                // If "CapturingToMemory", get as many buffers as we can.

                gCapParms.wNumVideoRequested = 
                        gCapParms.fUsingDOSMemory ? 32 : 1000;

                // Don't abort on the left mouse anymore!
                gCapParms.fAbortLeftMouse = FALSE;
                gCapParms.fAbortRightMouse = TRUE;

                // If the Driver is Scrncap.drv, the following values are special

                // If wChunkGranularity is zero, the granularity will be set to the
                // disk sector size.
                gCapParms.wChunkGranularity = (gbIsScrncap ? 32 : 0);

                // Scrncap requires a callback for the message pump
                capSetCallbackOnYield(ghWndCap, 
                        (gbIsScrncap ? fpYieldCallback : NULL));

                // If the user has hit OK, set the new setup info
                capCaptureSetSetup(ghWndCap, &gCapParms, sizeof(CAPTUREPARMS)) ;
            } else {
                break;
            }

            // if no capture file, get that
            if (lstrlen(gachCaptureFile) <= 0) {
                cmdSetCaptureFile(hWnd);
                if (lstrlen(gachCaptureFile) <= 0) {
                    break;
                }
            }

            // Capture video sequence
            fResult = capCaptureSequence(ghWndCap) ;
            break;

        case IDM_C_CAPTUREFRAME:
            // Turn off overlay / preview (gets turned off by frame capture)
            vidcapSetLive(FALSE);
            vidcapSetOverlay(FALSE);

            // Grab a frame
            fResult = capGrabFrameNoStop(ghWndCap) ;
            break;


        case IDM_C_CAPSEL:
            {
                FARPROC fproc;

                // if no capture file, get that
                if (lstrlen(gachCaptureFile) <= 0) {
                    cmdSetCaptureFile(hWnd);
                    if (lstrlen(gachCaptureFile) <= 0) {
                        break;
                    }
                }

                fproc = MakeProcInstance(CapFramesProc, ghInstApp);
                DialogBox(ghInstApp, MAKEINTRESOURCE(IDD_CAPFRAMES), hWnd, (DLGPROC) fproc);
                FreeProcInstance(fproc);
            }
            break;

#ifdef DEBUG
        case IDM_C_TEST:
	    nTestCount = 0;
	    // Intentional fall through
	    
        case IDM_C_TESTAGAIN:
            // set the defaults we won't bother the user with
            gCapParms.fMakeUserHitOKToCapture = FALSE;
            gCapParms.wPercentDropForError = 100;

            gCapParms.wNumVideoRequested = 
                    gCapParms.fUsingDOSMemory ? 32 : 1000;

            // Don't abort on the left mouse anymore!
            gCapParms.fAbortLeftMouse = FALSE;
            gCapParms.fAbortRightMouse = TRUE;

            // If wChunkGranularity is zero, the granularity will be set to the
            // disk sector size.
            gCapParms.wChunkGranularity = (gbIsScrncap ? 32 : 0);

            // If the user has hit OK, set the new setup info
            capCaptureSetSetup(ghWndCap, &gCapParms, sizeof(CAPTUREPARMS)) ;

            // if no capture file, get that
            if (lstrlen(gachCaptureFile) <= 0) {
                cmdSetCaptureFile(hWnd);
                if (lstrlen(gachCaptureFile) <= 0) {
                    break;
                }
            }
	    
	    {
		TCHAR buf[80];

      gCapParms.wNumVideoRequested = 10;
      gCapParms.wNumAudioRequested = 5;
		gCapParms.fLimitEnabled = TRUE;
		if (gCapParms.wTimeLimit == 0)
		    gCapParms.wTimeLimit = 5;
		capCaptureSetSetup(ghWndCap, &gCapParms, sizeof(CAPTUREPARMS)) ;
		
		// Capture video sequence
      fResult = capCaptureSequence(ghWndCap) ;
		
		wsprintf (buf, "TestCount = %d", nTestCount++);
		statusUpdateStatus(ghWndStatus, buf);
		
		// Hold down the right mouse button to abort
		if (!GetAsyncKeyState(VK_RBUTTON) & 0x0001)
		    PostMessage (hWnd, WM_COMMAND, IDM_C_TESTAGAIN, 0L);
            }
            break;
#endif
	    
/* --- help --- */
        case IDM_H_CONTENTS:
            HelpContents();
            break;

        case IDM_H_ABOUT:
            ShellAbout(
                hWnd,
                "VidCap",
                "Video Capture Tool",
                LoadIcon(ghInstApp,  gachIconName)
            );
            //DoDialog(hWnd, IDD_HelpAboutBox, AboutProc, 0);
            break ;


    }

    return 0L ;
}

/* --- menu help and enable/disable handling ------------------------ */

// write or clear status line help text when the user brings up or cancels a
// menu. This depends on there being strings in the string table with
// the same ID as the corresponding menu item.
// Help text for the items along the menu bar (File, Edit etc) depends
// on IDM_FILE, IDM_EDIT being defined with values 100 apart in the same
// order as their index in the menu
void
MenuSelect(HWND hwnd, UINT cmd, UINT flags, HMENU hmenu)
{
    if ((LOWORD(flags) == 0xffff) && (hmenu == NULL)) {
        //menu closing - remove message
        statusUpdateStatus(ghWndStatus, NULL);
    } else if ( (flags & (MF_SYSMENU|MF_POPUP)) == (MF_SYSMENU|MF_POPUP)) {
        // the system menu itself
        statusUpdateStatus(ghWndStatus, MAKEINTRESOURCE(IDM_SYSMENU));
    } else if ((flags & MF_POPUP) == 0) {
        // a menu command item
        statusUpdateStatus(ghWndStatus, MAKEINTRESOURCE(cmd));
    } else {
        //a popup menu - we need to search to find which one.
        // note that the cmd item in Win16 will now have a
        // menu handle, whereas in Win32 it has an index.
        // NOTE: this code assumes that the menu items
        // are #defined 100 apart in the same order, starting
        // with IDM_FILE
#ifdef _WIN32
        statusUpdateStatus(ghWndStatus, MAKEINTRESOURCE(IDM_FILE + (cmd * 100)));
#else
        int i,c;
        HMENU hmenuMain; 

        hmenuMain = GetMenu(hWnd);
        c = GetMenuItemCount(hmenuMain);

        for(i = 0; i < c; i++) {
            if (hmenu == GetSubMenu(hmenuMain, i)) {
                statusUpdateStatus(MAKEINTRESOURCE(IDM_FILE + (cmd*100)));
                return(0);
            }
        }
        statusUpdateStatus(NULL);
#endif
    }
}

// a popup menu is being selected - enable or disable menu items
int
InitMenuPopup(
    HWND hwnd,
    HMENU hmenu,
    int index
)
{
    int i = MF_ENABLED;
    CAPSTATUS cs;
    BOOL bUsesPalettes;

    capGetStatus(ghWndCap, &cs, sizeof(cs));

    // try to see if the driver uses palettes
    if ((cs.hPalCurrent != NULL) || (cs.fUsingDefaultPalette)) {
        bUsesPalettes = TRUE;
    } else {
        bUsesPalettes = FALSE;
    }


    switch(index) {
    case 0:         // IDM_FILE

        if (lstrlen(gachCaptureFile) <= 0) {
            i = MF_GRAYED;
        }
        // save as enabled only if we have a capture file
        EnableMenuItem(hmenu, IDM_F_SAVEVIDEOAS, i);
        // edit video possible only if we have a capture file AND we've
        // captured something
        EnableMenuItem(hmenu, IDM_F_EDITVIDEO,
            (cs.dwCurrentVideoFrame > 0) ? i : MF_GRAYED);

        // allow save palette if there is one
        EnableMenuItem(hmenu, IDM_F_SAVEPALETTE,
            (cs.hPalCurrent != NULL) ? MF_ENABLED:MF_GRAYED);

        // allow load palette if the driver uses palettes
        EnableMenuItem(hmenu, IDM_F_LOADPALETTE,
            bUsesPalettes ? MF_ENABLED : MF_GRAYED);

        break;

    case 1:         // IDM_EDIT

        // paste palettes if driver uses them and there is one pastable
        EnableMenuItem(hmenu, IDM_E_PASTEPALETTE,
            (bUsesPalettes && IsClipboardFormatAvailable(CF_PALETTE)) ? MF_ENABLED:MF_GRAYED);

        break;

    case 2:         // IDM_OPTIONS

        EnableMenuItem(hmenu, IDM_O_AUDIOFORMAT,
            cs.fAudioHardware ? MF_ENABLED : MF_GRAYED);

        EnableMenuItem(hmenu, IDM_O_OVERLAY,
            gCapDriverCaps.fHasOverlay ? MF_ENABLED:MF_GRAYED);

        EnableMenuItem(hmenu, IDM_O_VIDEOFORMAT,
            gCapDriverCaps.fHasDlgVideoFormat ? MF_ENABLED:MF_GRAYED);

        EnableMenuItem(hmenu, IDM_O_VIDEODISPLAY,
            gCapDriverCaps.fHasDlgVideoDisplay ? MF_ENABLED:MF_GRAYED);

        EnableMenuItem(hmenu, IDM_O_VIDEOSOURCE,
            gCapDriverCaps.fHasDlgVideoSource ? MF_ENABLED:MF_GRAYED);

        EnableMenuItem(hmenu, IDM_O_PREVIEW,
                gbHaveHardware ? MF_ENABLED:MF_GRAYED);


    case 3:     // IDM_CAPTURE
        if (!gbHaveHardware) {
            i = MF_GRAYED;
        }
        EnableMenuItem(hmenu, IDM_C_CAPSEL, i);
        EnableMenuItem(hmenu, IDM_C_CAPTUREFRAME, i);
        EnableMenuItem(hmenu, IDM_C_CAPTUREVIDEO, i);
        EnableMenuItem(hmenu, IDM_C_PALETTE, (gbHaveHardware &&
            gCapDriverCaps.fDriverSuppliesPalettes) ? MF_ENABLED : MF_GRAYED);

        break;
    }
    return(0);
}




//
// MainWndProc: Application Main Window Procedure
//
LRESULT FAR PASCAL MainWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
////////////////////////////////////////////////////////////////////////
//  hWnd:      Application main window handle
//  Message:   Next message to be processed
//  wParam:    WORD param for the message
//  lParam:    LONG param for the message
////////////////////////////////////////////////////////////////////////

    switch (Message) {

        static BOOL fMinimized;

        case WM_SYSCOMMAND:
	    if ((wParam & 0xfff0) == SC_MAXIMIZE)
	    	fMinimized = FALSE;
	    else if ((wParam & 0xfff0) == SC_RESTORE)
	    	fMinimized = FALSE;
	    else if ((wParam & 0xfff0) == SC_MINIMIZE)
	    	fMinimized = TRUE;	
	    return DefWindowProc(hWnd, Message, wParam, lParam);			
	    break;

        case WM_COMMAND:
            MenuProc(hWnd, wParam, lParam) ;
            break ;

        case WM_CREATE:
            HelpInit(ghInstApp, "vidcap.hlp", hWnd);
            break;

        case WM_NCHITTEST:
        {
            LRESULT dw;

            dw = DefWindowProc(hWnd, Message, wParam, lParam);
            // Don't allow border resize if autosizing
            if (gbAutoSizeFrame) {
                if (dw >= HTSIZEFIRST && dw <= HTSIZELAST)
                    dw = HTCAPTION;
            }
            return dw;
                
        }
            break;

        case WM_GETMINMAXINFO:
            // Don't allow manual sizing if window locked to the capture size
            if (gbHaveHardware && gbAutoSizeFrame && !gbInLayout) {
                RECT rW;

                LPMINMAXINFO lpMMI = (LPMINMAXINFO) lParam;

                GetWindowRect (hWnd, &rW);
                lpMMI->ptMinTrackSize.x = rW.right - rW.left;
                lpMMI->ptMinTrackSize.y = rW.bottom - rW.top;
                lpMMI->ptMaxTrackSize = lpMMI->ptMinTrackSize;
            }
            break;

        case WM_MOVE:
	    if (!fMinimized) {
	    	vidcapLayout (hWnd);
	    }
	    break;

        case WM_SIZE:
	    if (!fMinimized) {
	    	vidcapLayout (hWnd);
	    }
	    break;

        case WM_MENUSELECT:
            {
                UINT cmd = GET_WM_MENUSELECT_CMD(wParam, lParam);
                UINT flags = GET_WM_MENUSELECT_FLAGS(wParam, lParam);
                HMENU hmenu = GET_WM_MENUSELECT_HMENU(wParam, lParam);

                MenuSelect(hWnd, cmd, flags, hmenu);
            }
            break;

        case WM_INITMENUPOPUP:
            {
                BOOL bSystem = (BOOL) HIWORD(lParam);

                if (!bSystem) {
                    return InitMenuPopup(hWnd,
                            (HMENU) wParam, (int) LOWORD(lParam));
                } else {
                    return(DefWindowProc(hWnd, Message, wParam, lParam));
                }
            }


        case WM_SYSCOLORCHANGE:
            // we don't use this ourselves, but we should pass
            // it on to all three children
            SendMessage(ghWndFrame, Message, wParam, lParam);
            SendMessage(ghWndToolBar, Message, wParam, lParam);
            SendMessage(ghWndStatus, Message, wParam, lParam);
            return (TRUE);


        case WM_PALETTECHANGED:
        case WM_QUERYNEWPALETTE:
            // Pass the buck to Capture window proc
            return SendMessage(ghWndCap, Message, wParam, lParam) ;
            break ;

        case WM_SETFOCUS:
            // the toolbar is the only part that needs the focus
            SetFocus(ghWndToolBar);
            break;


        case WM_ACTIVATEAPP:
            if (wParam && ghWndCap) 
                capPreviewRate(ghWndCap, 15); // Fast preview when active
            else
                capPreviewRate(ghWndCap, 1000); // Slow preview when inactive
            break;

        case WM_NEXTDLGCTL:
            // if anyone is tabbing about, move the focus to the
            // toolbar
            SetFocus(ghWndToolBar);

            // select the correct button to handle moving off one
            // end and back on the other end
            if (lParam == FALSE) {
                // are we moving forwards or backwards ?
                if (wParam == 0) {
                    // move to next - so select first button
                    toolbarSetFocus(ghWndToolBar, TB_FIRST);
                } else {
                    // move to previous - so select last
                    toolbarSetFocus(ghWndToolBar, TB_LAST);
                }
            }
            break;

        case WM_PAINT:
        {
            HDC           hDC ;
            PAINTSTRUCT   ps ;

            hDC = BeginPaint(hWnd, &ps) ;

            // Included in case the background is not a pure color
            SetBkMode(hDC, TRANSPARENT) ;

            EndPaint(hWnd, &ps) ;
            break ;
        }

        case WM_CLOSE:
            // Disable and free all the callbacks 
            capSetCallbackOnError(ghWndCap, NULL) ;
			if (fpErrorCallback) {
            	FreeProcInstance(fpErrorCallback) ;
				fpErrorCallback = NULL;
			}

            capSetCallbackOnStatus(ghWndCap, NULL) ;
			if (fpStatusCallback) {
            	FreeProcInstance(fpStatusCallback) ;
				fpStatusCallback = NULL;
			}

            capSetCallbackOnYield(ghWndCap, NULL) ;
			if (fpYieldCallback) {
            	FreeProcInstance(fpYieldCallback) ;
				fpYieldCallback = NULL;
			}

            // Disconnect the current capture driver
            capDriverDisconnect (ghWndCap);

            // Destroy child windows, modeless dialogs, then this window...
            // DestroyWindow(ghWndCap) ;
            DestroyWindow(hWnd) ;
            break ;

        case WM_DESTROY:
            {
                // remember window size and position
                // - this will be written to the profile
                WINDOWPLACEMENT wp;

                wp.length = sizeof (WINDOWPLACEMENT);
                GetWindowPlacement(hWnd, &wp);

                gWinShow = wp.showCmd;
                gWinX = wp.rcNormalPosition.left;
                gWinY = wp.rcNormalPosition.top;
                gWinCX = RECTWIDTH(wp.rcNormalPosition);
                gWinCY = RECTHEIGHT(wp.rcNormalPosition);

                // write defaults out to the registry
                vidcapWriteProfile();
                vidcapWriteSettingsProfile();

                HelpShutdown();

            }

            PostQuitMessage(0) ;
            break ;

        default:
            return DefWindowProc(hWnd, Message, wParam, lParam) ;
    }

    return 0L;
}   // End of MainWndProc
