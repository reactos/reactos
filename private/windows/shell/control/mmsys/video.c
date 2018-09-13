/*
 ***************************************************************
 *  video.c
 *
 *  Copyright (C) Microsoft, 1990, All Rights Reserved.
 *
 *  Displays the Simple media properties
 *
 *  History:
 *
 *  July 1994 -by- VijR (Created)
 *        
 ***************************************************************
 */

#include "mmcpl.h"
#include <windowsx.h>
#ifdef DEBUG
#undef DEBUG
#include <mmsystem.h>
#define DEBUG
#else
#include <mmsystem.h>
#endif
#include <commctrl.h>
#include <prsht.h>
#include "utils.h"
#include "medhelp.h"

#include "..\..\..\media\avi\inc\profile.key" // For ROOTKEY and KEYNAME etc

#define Assert(f)

/*
 ***************************************************************
 * Defines 
 ***************************************************************
 */                                                
//
// !!! These actually live in mciavi\graphic.h
// !!! If MCIAVI changes these, we're hosed!
//
#define MCIAVIO_ZOOMBY2             0x00000100
#define MCIAVIO_USEVGABYDEFAULT     0x00000200
#define MCIAVIO_1QSCREENSIZE        0x00010000
#define MCIAVIO_2QSCREENSIZE        0x00020000
#define MCIAVIO_3QSCREENSIZE        0x00040000
#define MCIAVIO_MAXWINDOWSIZE       0x00080000
#define MCIAVIO_DEFWINDOWSIZE       0x00000000
#define MCIAVIO_WINDOWSIZEMASK      0x000f0000

// This bit is set in dwOptions if f16BitCompat, but doesn't get written back
// directly into the registry's version of that DWORD.
//
#define MCIAVIO_F16BITCOMPAT        0x00000001

/*
 ***************************************************************
 * File Globals
 ***************************************************************
 */
static SZCODE aszMCIAVIOpt[] =    TEXT("Software\\Microsoft\\Multimedia\\Video For Windows\\MCIAVI");
static SZCODE aszDefVideoOpt[] = TEXT("DefaultOptions");
static SZCODE aszReject[] = TEXT("RejectWOWOpenCalls");
static SZCODE aszRejectSection[] = TEXT("MCIAVI");

HBITMAP g_hbmMonitor = NULL;    // monitor bitmap (original)
HBITMAP g_hbmScrSample = NULL;  // bitmap used for IDC_SCREENSAMPLE
HBITMAP g_hbmDefault = NULL;
HDC g_hdcMem = NULL;

/*
 ***************************************************************
 * Prototypes
 ***************************************************************
 */
BOOL PASCAL DoVideoCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);

/*
 ***************************************************************
 ***************************************************************
 */

// mmGetProfileInt/mmWriteProfileInt snitched from MCIAVI32
UINT
mmGetProfileInt(LPCTSTR appname, LPCTSTR valuename, INT uDefault)
{
    TCHAR achName[MAX_PATH];
    HKEY hkey;
    DWORD dwType;
    INT value = uDefault;
    DWORD dwData;
    int cbData;

    lstrcpy(achName, KEYNAME);
    lstrcat(achName, appname);
    if (RegOpenKey(ROOTKEY, achName, &hkey) == ERROR_SUCCESS) {

        cbData = sizeof(dwData);
        if (RegQueryValueEx(
            hkey,
            (LPTSTR)valuename,
            NULL,
            &dwType,
            (PBYTE) &dwData,
            &cbData) == ERROR_SUCCESS) {
                if (dwType == REG_DWORD) {
                    value = (INT)dwData;
                }
        }

        RegCloseKey(hkey);
    }

    return((UINT)value);
}


/*
 * write a UINT to the profile, if it is not the
 * same as the default or the value already there
 */
VOID
mmWriteProfileInt(LPCTSTR appname, LPCTSTR valuename, INT Value)
{
    // If we would write the same as already there... return.
    if (mmGetProfileInt(appname, valuename, !Value) == (UINT)Value) {
        return;
    }

    {
        TCHAR achName[MAX_PATH];
        HKEY hkey;

        lstrcpy(achName, KEYNAME);
        lstrcat(achName, appname);
        if (RegCreateKey(ROOTKEY, achName, &hkey) == ERROR_SUCCESS) {
            RegSetValueEx(
                hkey,
                valuename,
                0,
                REG_DWORD,
                (PBYTE) &Value,
                sizeof(Value)
            );

            RegCloseKey(hkey);
        }
    }

}

/*
 ***************************************************************
 ***************************************************************
 */

STATIC void WriteVideoOptions(DWORD dwOpt)
{
    HKEY hkVideoOpt;
    BOOL f16BitCompat;

    f16BitCompat = (dwOpt & MCIAVIO_F16BITCOMPAT) ? TRUE : FALSE;
    dwOpt &= ~MCIAVIO_F16BITCOMPAT;

    if(RegCreateKeyEx( HKEY_CURRENT_USER, (LPTSTR)aszMCIAVIOpt, 0, NULL, 0,KEY_WRITE | KEY_READ, NULL, &hkVideoOpt, NULL ))
        return;
    RegSetValueEx( hkVideoOpt, (LPTSTR)aszDefVideoOpt, 0, REG_DWORD,(LPBYTE)&dwOpt, sizeof(DWORD) );
    mmWriteProfileInt (aszRejectSection, aszReject, (int)f16BitCompat);

	RegCloseKey(hkVideoOpt);
}

STATIC DWORD ReadVideoOptions(void)
{
    HKEY hkVideoOpt;
    DWORD dwType;
    DWORD dwOpt;
    DWORD cbSize;

    if(RegCreateKeyEx( HKEY_CURRENT_USER, (LPTSTR)aszMCIAVIOpt, 0, NULL, 0,KEY_WRITE | KEY_READ, NULL, &hkVideoOpt, NULL ))
        return 0 ;

    cbSize = sizeof(DWORD);
    if (RegQueryValueEx( hkVideoOpt,(LPTSTR)aszDefVideoOpt,NULL,&dwType,(LPBYTE)&dwOpt,&cbSize ))
    {
        dwOpt = 0;
        RegSetValueEx( hkVideoOpt, (LPTSTR)aszDefVideoOpt, 0, REG_DWORD,(LPBYTE)&dwOpt, sizeof(DWORD) );
    }

    if (mmGetProfileInt (aszRejectSection, aszReject, 0))
    {
        dwOpt |= MCIAVIO_F16BITCOMPAT;
    }

	RegCloseKey(hkVideoOpt);

    return dwOpt;
}

STATIC void FillWindowSizeCB(DWORD dwOptions, HWND hCBWinSize)
{
    TCHAR sz1QScreenSize[64];
    TCHAR sz2QScreenSize[64];
    TCHAR sz3QScreenSize[64];
    TCHAR szMaxSize[64];
    TCHAR szOriginalSize[64];
    TCHAR szZoomedSize[64];
    int iIndex;
    LPTSTR  lpszCurDefSize;

    LoadString(ghInstance, IDS_NORMALSIZE, szOriginalSize, sizeof(szOriginalSize)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_ZOOMEDSIZE, szZoomedSize, sizeof(szZoomedSize)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_1QSCREENSIZE, sz1QScreenSize, sizeof(sz1QScreenSize)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_2QSCREENSIZE, sz2QScreenSize, sizeof(sz2QScreenSize)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_3QSCREENSIZE, sz3QScreenSize, sizeof(sz3QScreenSize)/sizeof(TCHAR));
    LoadString(ghInstance, IDS_VIDEOMAXIMIZED, szMaxSize, sizeof(szMaxSize)/sizeof(TCHAR));

    iIndex = ComboBox_AddString(hCBWinSize, szOriginalSize);
    ComboBox_SetItemData(hCBWinSize, iIndex, (LPARAM)MCIAVIO_DEFWINDOWSIZE);
    iIndex = ComboBox_AddString(hCBWinSize, szZoomedSize);
    ComboBox_SetItemData(hCBWinSize, iIndex, (LPARAM)MCIAVIO_ZOOMBY2);
    iIndex = ComboBox_AddString(hCBWinSize, sz1QScreenSize);
    ComboBox_SetItemData(hCBWinSize, iIndex, (LPARAM)MCIAVIO_1QSCREENSIZE);
    iIndex = ComboBox_AddString(hCBWinSize, sz2QScreenSize);
    ComboBox_SetItemData(hCBWinSize, iIndex, (LPARAM)MCIAVIO_2QSCREENSIZE);
    iIndex = ComboBox_AddString(hCBWinSize, sz3QScreenSize);
    ComboBox_SetItemData(hCBWinSize, iIndex, (LPARAM)MCIAVIO_3QSCREENSIZE);
    iIndex = ComboBox_AddString(hCBWinSize, szMaxSize);
    ComboBox_SetItemData(hCBWinSize, iIndex, (LPARAM)MCIAVIO_MAXWINDOWSIZE);

    switch(dwOptions & MCIAVIO_WINDOWSIZEMASK)
    {
        case MCIAVIO_DEFWINDOWSIZE:
            if (dwOptions & MCIAVIO_ZOOMBY2)
                lpszCurDefSize = szZoomedSize;
            else
                lpszCurDefSize = szOriginalSize;
            break;
        case MCIAVIO_1QSCREENSIZE:
            lpszCurDefSize =  sz1QScreenSize;
            break;
        case MCIAVIO_2QSCREENSIZE:
            lpszCurDefSize =  sz2QScreenSize;
            break;
        case MCIAVIO_3QSCREENSIZE:
            lpszCurDefSize =  sz3QScreenSize;
            break;
        case MCIAVIO_MAXWINDOWSIZE:
            lpszCurDefSize =  szMaxSize;
            break;
    }

    //
    // We should select string that matches exactly.
    //
    iIndex = ComboBox_FindStringExact(hCBWinSize, 0, lpszCurDefSize);
    ComboBox_SetCurSel(hCBWinSize, iIndex);

}

/*---------------------------------------------------------
**
**---------------------------------------------------------*/
// information about the monitor bitmap 
// x, y, dx, dy define the size of the "screen" part of the bitmap
// the RGB is the color of the screen's desktop
// these numbers are VERY hard-coded to a monitor bitmap
#define MON_X    16
#define MON_Y    17
#define MON_DX    152
#define MON_DY    112

#define MON_IMAGE_X (MON_X + 46)
#define MON_IMAGE_Y (MON_Y + 33)
#define MON_IMAGE_DX 59
#define MON_IMAGE_DY 44

#define MON_TITLE 10
#define MON_BORDER 2

#define MON_TRAY 8


STATIC HBITMAP FAR LoadMonitorBitmap( BOOL bFillDesktop )
{
    HBITMAP hbm;

    hbm = (HBITMAP) LoadImage(ghInstance,MAKEINTATOM(IDB_MONITOR), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);

    Assert(hbm);

    return hbm;
}

/*------------------------pixel resolution---------------------------
**
** show the sample screen with proper sizing
*/
STATIC void NEAR PASCAL ShowSampleScreen(HWND hDlg, int iMCIWindowSize)
{
    HBITMAP hbmOld;
    HBRUSH hbr;
    HDC hdcMem2;
    SIZE    dSrc = {MON_IMAGE_DX, MON_IMAGE_DY};
    POINT   ptSrc = {MON_IMAGE_X, MON_IMAGE_Y};
    SIZE    dDst;
    POINT   ptDst;
    RECT    rcImage = {MON_IMAGE_X, MON_IMAGE_Y, MON_IMAGE_X + MON_IMAGE_DX, MON_IMAGE_Y + MON_IMAGE_DY};

    if (!g_hbmMonitor || !g_hbmScrSample)
        return;

    switch(iMCIWindowSize)
    {
        case MCIAVIO_DEFWINDOWSIZE:
            dDst = dSrc;
            ptDst = ptSrc;
            break;

        case MCIAVIO_ZOOMBY2:
            dDst.cx = 2 * dSrc.cx;
            dDst.cy = 2 * dSrc.cy;
            ptDst.x = ptSrc.x - (int)(dSrc.cx/2);
            ptDst.y = ptSrc.y - (int)(dSrc.cy/2);
            break;
        
        case MCIAVIO_MAXWINDOWSIZE:
            ptDst.x = MON_X;
            ptDst.y = MON_Y;
            dDst.cx = MON_DX;
            dDst.cy = MON_DY - MON_TRAY;
            break;

        case MCIAVIO_1QSCREENSIZE:
            dDst.cx = MulDiv(MON_DX, 1, 4);
            dDst.cy = MulDiv(MON_DY, 1, 4);
            ptDst.x = MON_X + MulDiv((MON_DX - dDst.cx), 1 , 2);
            ptDst.y = MON_Y + MulDiv((MON_DY - dDst.cy - MON_TRAY), 1 , 2);
            break;
       
        case MCIAVIO_2QSCREENSIZE:
            dDst.cx = MulDiv(MON_DX, 1, 2);
            dDst.cy = MulDiv(MON_DY, 1, 2);
            ptDst.x = MON_X + MulDiv((MON_DX - dDst.cx), 1 , 2);
            ptDst.y = MON_Y + MulDiv((MON_DY - dDst.cy - MON_TRAY), 1 , 2);
            break;
        
        case MCIAVIO_3QSCREENSIZE:
            dDst.cx = MulDiv(MON_DX, 3, 4);
            dDst.cy = MulDiv(MON_DY, 3, 4);
            ptDst.x = MON_X + MulDiv((MON_DX - dDst.cx), 1 , 2);
            ptDst.y = MON_Y + MulDiv((MON_DY - dDst.cy - MON_TRAY), 1 , 2);
            break;
        
        case MCIAVIO_USEVGABYDEFAULT:
            dDst.cx = MON_DX;
            dDst.cy = MON_DY;
            ptDst.x = MON_X;
            ptDst.y = MON_Y;

            
            dSrc.cx = MON_IMAGE_DX - (2 * MON_BORDER);
            dSrc.cy = MON_IMAGE_DY - MON_TITLE - MON_BORDER;
            ptSrc.x = MON_IMAGE_X + MON_BORDER;
            ptSrc.y = MON_IMAGE_Y + MON_TITLE + MON_BORDER;
            break;

    } 

    // set up a work area to play in
    hdcMem2 = CreateCompatibleDC(g_hdcMem);
    if (!hdcMem2)
        return;
    SelectObject(hdcMem2, g_hbmScrSample);
    hbmOld = SelectObject(g_hdcMem, g_hbmMonitor);

    //copy the whole bmp first and then start stretching 
    BitBlt(hdcMem2, MON_X, MON_Y, MON_DX, MON_DY, g_hdcMem, MON_X, MON_Y, SRCCOPY);
    
    //Wipe out the existing Image
    hbr =   CreateSolidBrush( GetPixel( g_hdcMem, MON_X + 1, MON_Y + 1 ) );

    FillRect(hdcMem2, &rcImage, hbr);
    DeleteObject( hbr );


    // stretch the image to reflect the new size
    SetStretchBltMode( hdcMem2, COLORONCOLOR );

    StretchBlt( hdcMem2, ptDst.x, ptDst.y, dDst.cx, dDst.cy,
        g_hdcMem, ptSrc.x, ptSrc.y, dSrc.cx, dSrc.cy, SRCCOPY );

    SelectObject( hdcMem2, g_hbmDefault );
    DeleteObject( hdcMem2 );
    SelectObject( g_hdcMem, hbmOld );
    InvalidateRect(GetDlgItem(hDlg, IDC_SCREENSAMPLE), NULL, FALSE);
}


STATIC void DoMonitorBmp(HWND hDlg)
{
    HDC hdc = GetDC(NULL);
    HBITMAP hbm;

    g_hdcMem = CreateCompatibleDC(hdc);
    ReleaseDC(NULL, hdc);

    if (!g_hdcMem)
        return;

    hbm = CreateBitmap(1,1,1,1,NULL);
    g_hbmDefault = SelectObject(g_hdcMem, hbm);
    SelectObject(g_hdcMem, g_hbmDefault);
    DeleteObject(hbm);

    // set up bitmaps for sample screen
    g_hbmScrSample = LoadMonitorBitmap( TRUE ); // let them do the desktop
    SendDlgItemMessage(hDlg, IDC_SCREENSAMPLE, STM_SETIMAGE, IMAGE_BITMAP, (DWORD_PTR)g_hbmScrSample);

    // get a base copy of the bitmap for when the "internals" change
    g_hbmMonitor = LoadMonitorBitmap( FALSE ); // we'll do the desktop
}



const static DWORD aAdvVideoDlgHelpIds[] = {  // Context Help IDs
    (DWORD)IDC_STATIC,         IDH_VIDEO_ADVANCED_COMPAT,
    ID_ADVVIDEO_COMPAT,        IDH_VIDEO_ADVANCED_COMPAT,
                  
    0, 0
};

INT_PTR AdvancedVideoDlgProc (HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL *pfCompat = NULL;

    switch (wMsg)
    {
        case WM_INITDIALOG:
        {
            if ((pfCompat = (BOOL *)lParam) == NULL)
                return -1;

            CheckDlgButton (hDlg, ID_ADVVIDEO_COMPAT, (*pfCompat));
            break;
        }

        case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                    *pfCompat = IsDlgButtonChecked (hDlg, ID_ADVVIDEO_COMPAT);
                    // fall through

                case IDCANCEL:
                    EndDialog (hDlg, GET_WM_COMMAND_ID(wParam, lParam));
                    break;
            }
            break;
        }

        case WM_CONTEXTMENU:
        {
            WinHelp ((HWND)wParam, NULL, HELP_CONTEXTMENU,
                     (UINT_PTR)aAdvVideoDlgHelpIds);
            return TRUE;
        }

        case WM_HELP:
        {
            WinHelp (((LPHELPINFO)lParam)->hItemHandle, NULL,
                     HELP_WM_HELP, (UINT_PTR)aAdvVideoDlgHelpIds);
            return TRUE;
        }
    }

    return FALSE;
}


const static DWORD aVideoDlgHelpIds[] = {  // Context Help IDs
    IDC_GROUPBOX,              IDH_MMSE_GROUPBOX,
    IDC_SCREENSAMPLE,          IDH_VIDEO_GRAPHIC,
    IDC_VIDEO_FULLSCREEN,      IDH_VIDEO_FULL_SCREEN,
    IDC_VIDEO_INWINDOW,        IDH_VIDEO_FIXED_WINDOW,
    IDC_VIDEO_CB_SIZE,         IDH_VIDEO_FIXED_WINDOW,
    ID_VIDEO_ADVANCED,         IDH_VIDEO_ADVANCED_BUTTON,
                  
    0, 0
};

BOOL CALLBACK VideoDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR   *lpnm;
    
    switch (uMsg)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_KILLACTIVE:
                    FORWARD_WM_COMMAND(hDlg, IDOK, 0, 0, SendMessage);    
                    break;              

                case PSN_APPLY:
                    FORWARD_WM_COMMAND(hDlg, ID_APPLY, 0, 0, SendMessage);    
                    break;                                  

                case PSN_SETACTIVE:
                    FORWARD_WM_COMMAND(hDlg, ID_INIT, 0, 0, SendMessage);
                    break;
                    
                case PSN_RESET:
                    FORWARD_WM_COMMAND(hDlg, IDCANCEL, 0, 0, SendMessage);
                    break;
            }
            break;
        
        case WM_INITDIALOG:
        {
            DWORD dwOptions;

            dwOptions = ReadVideoOptions();
            FillWindowSizeCB(dwOptions, GetDlgItem(hDlg, IDC_VIDEO_CB_SIZE));
            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)dwOptions);
            DoMonitorBmp(hDlg);
            if (IsFlagSet(dwOptions, MCIAVIO_USEVGABYDEFAULT))
            {
                CheckRadioButton(hDlg, IDC_VIDEO_INWINDOW, IDC_VIDEO_FULLSCREEN, IDC_VIDEO_FULLSCREEN);
                EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_CB_SIZE), FALSE);
                ShowSampleScreen(hDlg, MCIAVIO_USEVGABYDEFAULT);
                break;
            }
            else
            {
                CheckRadioButton(hDlg, IDC_VIDEO_INWINDOW, IDC_VIDEO_FULLSCREEN, IDC_VIDEO_INWINDOW);
                EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_CB_SIZE), TRUE);
            }
            ShowSampleScreen(hDlg, dwOptions & (MCIAVIO_WINDOWSIZEMASK|MCIAVIO_ZOOMBY2));
            break;
        }
        case WM_DESTROY:
            if (g_hbmScrSample)
            {
                DeleteObject(g_hbmScrSample);
                g_hbmScrSample = NULL;
            }
            if (g_hbmMonitor)
            {
                DeleteObject(g_hbmMonitor);
                g_hbmMonitor = NULL;
            }
            if (g_hbmDefault)
            {
                DeleteObject(g_hbmDefault);
                g_hbmDefault = NULL;
            }
            if (g_hdcMem)
            {
                DeleteDC(g_hdcMem);
                g_hdcMem = NULL;
            }

            break;

        case WM_DROPFILES:
            break;

        case WM_CONTEXTMENU:
            WinHelp ((HWND) wParam, NULL, HELP_CONTEXTMENU,
                    (UINT_PTR) (LPTSTR) aVideoDlgHelpIds);
            return TRUE;

        case WM_HELP:
        {
            LPHELPINFO lphi = (LPVOID) lParam;
            WinHelp (lphi->hItemHandle, NULL, HELP_WM_HELP,
                    (UINT_PTR) (LPTSTR) aVideoDlgHelpIds);
            return TRUE;
        }
            
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hDlg, wParam, lParam, DoVideoCommand);
            break;
        
    }
    return FALSE;
}

BOOL PASCAL DoVideoCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        
    case ID_APPLY:
    {
        int iIndex;
        HWND hCBWinSize = GetDlgItem(hDlg,IDC_VIDEO_CB_SIZE);
        DWORD dwOldOpt;
        DWORD dwNewOpt;
        
        dwOldOpt = (DWORD)GetWindowLongPtr(hDlg, DWLP_USER);
        if(Button_GetCheck(GetDlgItem(hDlg, IDC_VIDEO_FULLSCREEN)))      
        {
            dwNewOpt = MCIAVIO_USEVGABYDEFAULT;                   
        }
        else
        {
            iIndex = ComboBox_GetCurSel(hCBWinSize);
            dwNewOpt = (DWORD)ComboBox_GetItemData(hCBWinSize, iIndex);
        }

        ClearFlag(dwOldOpt,MCIAVIO_WINDOWSIZEMASK|MCIAVIO_USEVGABYDEFAULT|MCIAVIO_ZOOMBY2);
        SetFlag(dwOldOpt, dwNewOpt);
        WriteVideoOptions(dwOldOpt);
        SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)dwOldOpt);
        return TRUE;
    }
        
    case IDOK:
        break;

    case IDCANCEL:
        break;

    case IDC_VIDEO_FULLSCREEN:
        EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_CB_SIZE), FALSE);
        PropSheet_Changed(GetParent(hDlg),hDlg);
        ShowSampleScreen(hDlg, MCIAVIO_USEVGABYDEFAULT);
        break;

    case IDC_VIDEO_INWINDOW:
    {
        HWND hwndCB = GetDlgItem(hDlg,IDC_VIDEO_CB_SIZE); 
        int iIndex = ComboBox_GetCurSel(hwndCB); 
        int iOpt  = (int)ComboBox_GetItemData(hwndCB, iIndex);

        EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_CB_SIZE), TRUE);
        PropSheet_Changed(GetParent(hDlg),hDlg);
        ShowSampleScreen(hDlg, iOpt);
        break;
    }

    case IDC_VIDEO_CB_SIZE:
        switch (codeNotify)
        {
            case CBN_SELCHANGE:
            {
                
                int iIndex = ComboBox_GetCurSel(hwndCtl); 
                int iOpt  = (int)ComboBox_GetItemData(hwndCtl, iIndex);

                PropSheet_Changed(GetParent(hDlg),hDlg);
                ShowSampleScreen(hDlg, iOpt);
                break;
            }
            default:
                break;
        }
        break;

    case ID_VIDEO_ADVANCED:
    {
        INT_PTR  f16BitCompat;
        f16BitCompat = (GetWindowLongPtr(hDlg, DWLP_USER) & MCIAVIO_F16BITCOMPAT);

        if (DialogBoxParam (ghInstance,
                            MAKEINTRESOURCE(ADVVIDEODLG),
                            hDlg,
                            AdvancedVideoDlgProc,
                            (LPARAM)&f16BitCompat) == IDOK)
        {
            SetWindowLongPtr (hDlg, DWLP_USER,
                           GetWindowLongPtr (hDlg, DWLP_USER)
                               & (~MCIAVIO_F16BITCOMPAT)
                               | ((f16BitCompat) ? (MCIAVIO_F16BITCOMPAT) : 0));
            PropSheet_Changed(GetParent(hDlg),hDlg);
        }
        break;
    }

    case ID_INIT:        
        break;

    }
    return FALSE;
}

