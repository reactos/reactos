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
 *   vidcap.h: Main application include file
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/

#include "dialogs.h"

#define USE_ACM	1	        // Use ACM dialogs for audio format selection

//
// General purpose constants...
//
#define MAXVIDDRIVERS            10

#define DEF_CAPTURE_FPS          15
#define MIN_CAPTURE_FPS          (1.0 / 60)     // one frame per minute
#define MAX_CAPTURE_FPS          100

#define FPS_TO_MS(f)             ((DWORD) ((double)1.0e6 / f))

#define DEF_CAPTURE_RATE         FPS_TO_MS(DEF_CAPTURE_FPS)
#define MIN_CAPTURE_RATE         FPS_TO_MS(MIN_CAPTURE_FPS)
#define MAX_CAPTURE_RATE         FPS_TO_MS(MAX_CAPTURE_FPS)


#define DEF_PALNUMFRAMES         10
#define DEF_PALNUMCOLORS         236L
#define ONEMEG                   (1024L * 1024L)

//standard index size options
#define CAP_LARGE_INDEX          (30 * 60 * 60 * 3)     // 3 hrs @ 30fps
#define CAP_SMALL_INDEX          (30 * 60 * 15)         // 15 minutes @ 30fps


//
// Menu Ids...must not conflict with string table ids
// these are also the id of help strings in the string table
// (along with all the SC_ system menu items).
// menu popups must start 10 apart and be numbered in the same order
// as they appear if the help text is to work correctly for the
// popup heads as well as for the menu items.
//
#define IDM_SYSMENU               100

#define IDM_FILE                  200
#define IDM_F_SETCAPTUREFILE      201
#define IDM_F_SAVEVIDEOAS         202
#define IDM_F_ALLOCATESPACE       203
#define IDM_F_EXIT                204
#define IDM_F_LOADPALETTE         205
#define IDM_F_SAVEPALETTE         206
#define IDM_F_SAVEFRAME           207
#define IDM_F_EDITVIDEO           208


#define IDM_EDIT                  300
#define IDM_E_COPY                301
#define IDM_E_PASTEPALETTE        302
#define IDM_E_PREFS               303

#define IDM_CAPTURE               400
#define IDM_C_CAPTUREVIDEO        401
#define IDM_C_CAPTUREFRAME        402
#define IDM_C_PALETTE             403
#define IDM_C_CAPSEL              404
#define IDM_C_TEST                405
#define IDM_C_TESTAGAIN           406

#define IDM_OPTIONS               500
#define IDM_O_PREVIEW             501
#define IDM_O_OVERLAY             502
#define IDM_O_AUDIOFORMAT         503
#define IDM_O_VIDEOFORMAT         504
#define IDM_O_VIDEOSOURCE         505
#define IDM_O_VIDEODISPLAY        506
#define IDM_O_CHOOSECOMPRESSOR    507

#define IDM_O_DRIVER0             520
#define IDM_O_DRIVER1             521
#define IDM_O_DRIVER2             522
#define IDM_O_DRIVER3             523
#define IDM_O_DRIVER4             524
#define IDM_O_DRIVER5             525
#define IDM_O_DRIVER6             526
#define IDM_O_DRIVER7             527
#define IDM_O_DRIVER8             528
#define IDM_O_DRIVER9             529

#define IDM_HELP                  600
#define IDM_H_CONTENTS            601
#define IDM_H_ABOUT               602


// filter rcdata ids
#define ID_FILTER_AVI           900
#define ID_FILTER_PALETTE       901
#define ID_FILTER_DIB           902


/*
 * string table id
 *
 * NOTE: string table ID's must not conflict with IDM_ menu ids,
 * as there is a help string for each menu id.
 */


#define IDS_APP_TITLE            1001

#define IDS_ERR_REGISTER_CLASS   1002
#define IDS_ERR_CREATE_WINDOW    1003
#define IDS_ERR_FIND_HARDWARE    1004
#define IDS_ERR_CANT_PREALLOC    1005
#define IDS_ERR_MEASUREFREEDISK  1006
#define IDS_ERR_SIZECAPFILE      1007
#define IDS_ERR_RECONNECTDRIVER  1008
#define IDS_ERR_CMDLINE          1009
#define IDS_WARN_DEFAULT_PALETTE 1010

#define IDS_TITLE_SETCAPTUREFILE 1101
#define IDS_TITLE_SAVEAS         1102
#define IDS_TITLE_LOADPALETTE    1104
#define IDS_TITLE_SAVEPALETTE    1105
#define IDS_TITLE_SAVEDIB        1106
#define IDS_PROMPT_CAPFRAMES     1107
#define IDS_STATUS_NUMFRAMES     1108
#define IDS_CAP_CLOSE            1109
#define IDS_MCI_CONTROL_ERROR    1110
#define IDS_ERR_ACCESS_SOUNDDRIVER 1111
#define IDS_ERR_VIDEDIT          1112

#define IDC_toolbarSETFILE      1220
#define IDC_toolbarCAPFRAME     1221
#define IDC_toolbarCAPSEL       1222
#define IDC_toolbarCAPAVI       1223
#define IDC_toolbarCAPPAL       1224
#define IDC_toolbarLIVE         1225
#define IDC_toolbarEDITCAP      1226
#define IDC_toolbarOVERLAY      1227

#define IDS_CAPPAL_CLOSE        1230
#define IDS_CAPPAL_STATUS       1231
#define IDS_CAPPAL_STOP         1232
#define IDS_CAPPAL_START        1233

#define	IDS_CAP_RTL             1234

#define IDBMP_TOOLBAR		100	// main toolbar


//
// Macro Definitions...
//
#define IsDriverIndex(w) ( ((w) >= IDM_O_DRIVERS)  &&  \
                           ((w) - IDM_O_DRIVERS < MAXVIDDRIVERS) )

#define RECTWIDTH(rc)  ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)


//
// Global Variables...
//

// preferences
extern BOOL gbCentre;
extern BOOL gbToolBar;
extern BOOL gbStatusBar;
extern BOOL gbAutoSizeFrame;
extern int gBackColour;
extern BOOL gfIsRTL;

extern TCHAR           gachAppName[] ;
extern TCHAR           gachAppTitle[];
extern TCHAR           gachIconName[] ;
extern TCHAR           gachMenuName[] ;
extern TCHAR           gachString[] ;
extern TCHAR           gachMCIDeviceName[] ;

extern HINSTANCE      ghInstApp ;
extern HWND           ghWndMain ;
extern HWND           ghWndCap ;
extern HWND           ghWndFrame;
extern HANDLE         ghAccel ;
extern WORD           gwDeviceIndex ;
extern WORD           gwPalFrames ;
extern WORD           gwPalColors ;
extern WORD           gwCapFileSize ;
extern BOOL           gbLive ;

extern CAPSTATUS      gCapStatus ;
extern CAPDRIVERCAPS  gCapDriverCaps ;
extern CAPTUREPARMS   gCapParms ;

extern HANDLE         ghwfex ;
extern LPWAVEFORMATEX glpwfex ;

//
// Dialog Box Procedures...
//
LRESULT FAR PASCAL AboutProc(HWND, UINT, WPARAM, LPARAM) ;
LRESULT FAR PASCAL AudioFormatProc(HWND, UINT, WPARAM, LPARAM) ;
LRESULT FAR PASCAL CapSetUpProc(HWND, UINT, WPARAM, LPARAM) ;
LRESULT CALLBACK MakePaletteProc(HWND, UINT, WPARAM, LPARAM) ;
LRESULT FAR PASCAL AllocCapFileProc(HWND, UINT, WPARAM, LPARAM) ;
LRESULT FAR PASCAL PrefsDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT FAR PASCAL NoHardwareDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT FAR PASCAL CapFramesProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam);

// utility functions (in vidcap.c)
/*
 * put up a message box. the main window ghWndMain is used as the parent
 * window, and the app title gachAppTitle is used as the dialog title.
 * the text for the dialog -idString- is loaded from the resource string table
 */
int MessageBoxID(UINT idString, UINT fuStyle);
LPSTR tmpString(UINT idString);

