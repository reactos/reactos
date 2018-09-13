// **************************************************************************
//
// RunOnce.C
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1992-1993
//  All rights reserved
//
//  RunOnce wrapper. This encapsulates all applications that would like
//  to run the first time we re-boot. It lists these apps for the user
//      and allows the user to launce the apps (like apple at ease).
//
//      5 June 1994     FelixA  Started
//  8 June  Felix   Defined registry strings and functionality.
//                  Got small buttons displayed, but not working.
//  9 June  Felix   Both big and small buttons. Nice UI.
//                  Got single click app launching.
//
// 23 June  Felix   Moving it to a Chicago make thingy not Dolphin
//
// *************************************************************************/
//
#include "runonce.h"
#include <shlobj.h>
#include <stdlib.h>
#include <regstr.h>
#include <shellapi.h>
#include <shlobjp.h>
// #include <shsemip.h>

extern int g_iState;    // Command line args.

extern HINSTANCE g_hInst;          // current instance

#define WM_FINISHED (WM_USER+0x123)

#include "resource.h"

int g_fCleanBoot;
TCHAR c_szRunOnce[]=REGSTR_PATH_RUNONCE;
TCHAR c_szSetup[]=REGSTR_PATH_SETUP;
TCHAR g_szWallpaper[] = TEXT("wallpaper");
TCHAR szTileWall[] = TEXT("TileWallpaper");
TCHAR szFallback[] = TEXT("*DisplayFallback");
const TCHAR c_szTimeChangedRunOnce[] = TEXT("WarnTimeChanged"); //kernel32 and explorer use this

// Run time can be set for big or small buttons.
int g_Small=0;
HDC g_hdcMem=NULL;
DWORD g_dwThread;

//***************************************************************************
//
// <Function>()
// <Explanation>
//
// ENTRY:
//      <Params>
//
// EXIT:
//      <Params>
//
//***************************************************************************

//***************************************************************************
//
// DoAnyRandomOneTimeStuff()
//   Just a place to toss random stuff for RunOnce app to do.
//
// ENTRY:
//      void
//
// EXIT:
//      void
//
//***************************************************************************
void DoAnyRandomOneTimeStuff(void)
{
    HKEY runonce;

    // remove any time-changed warning added by kernel32 during boot
    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szRunOnce, &runonce) == ERROR_SUCCESS)
    {
        RegDeleteValue(runonce, (LPCTSTR)c_szTimeChangedRunOnce);
        RegCloseKey(runonce);
    }
}

//***************************************************************************
//
// RunOnceFill()
//   Fills the List box in the run-once dlg.
//
// ENTRY:
//      HWND of the thing to fill.
//
// EXIT:
//      <Params>
// BOOL NEAR PASCAL RunRegApps(HKEY hkeyParent, LPCSTR szSubkey, BOOL fDelete, BOOL fWait)
//
//***************************************************************************
BOOL   RunOnceFill(HWND hWnd)
{
    HKEY hkey;
    // HKEY hDescKey;
    BOOL fShellInit = FALSE;
    HKEY hkeyParent = HKEY_LOCAL_MACHINE;
    TCHAR szSubkey[MAX_PATH];
    BOOL fDelete=FALSE;
    BOOL fWait=FALSE;

    // Enumerate HKLM\Runonce\Setup - *.*
    lstrcpy(szSubkey,c_szRunOnce);
    lstrcat(szSubkey,TEXT("\\Setup"));
    if (RegOpenKey(hkeyParent, szSubkey, &hkey) == ERROR_SUCCESS)
    {
        DWORD cbData, cbValue, dwType, i;
        TCHAR szValueName[MAX_PATH], szCmdLine[MAX_PATH];
        LRESULT lRes;
        DWORD dwNumSubkeys=1, dwNumValues=5;

        for (i = 0; ; i++)
        {
            cbValue = sizeof(szValueName) ;
            cbData = sizeof(szCmdLine) / sizeof(TCHAR);

            if (RegEnumValue(hkey, i, szValueName, &cbValue, NULL, &dwType, (LPBYTE) szCmdLine, &cbData) != ERROR_SUCCESS)
                break;

            if (dwType == REG_SZ)
            {
                PTASK pTask;
                pTask = (PTASK)LocalAlloc( LPTR ,sizeof(TASK));
                lstrcpyn(pTask->Text,szValueName, MAX_TEXT );
                lstrcpyn( pTask->Cmd, szCmdLine, MAX_PATH );
                lRes = SendMessage( hWnd, LB_ADDSTRING,  0, (LPARAM)pTask );
                if( lRes == LB_ERR || lRes == LB_ERRSPACE )
                {
                    LocalFree(pTask);
                    pTask=NULL;
                }
            }
        }
        RegCloseKey(hkey);
    }

    return(fShellInit);
}

//***************************************************************************
//
// LaunchApp()
//  Given an index into the list box, will spawn the task, wait for it to
// finish.
//
// ENTRY:
//      Index into list.
//
// EXIT:
//      <Params>
//
//***************************************************************************
int LaunchApp(HWND hWnd, WORD wItem )
{
    LPTSTR lpszCmdLine;
    STARTUPINFO startup;
#ifndef DEBUG
    PROCESS_INFORMATION pi;
#endif
    PTASK pTask;
    RECT rWnd;

    GetWindowRect(hWnd, &rWnd);
    SendMessage(hWnd,LB_SETCURSEL,wItem,0);
    pTask = (PTASK)SendMessage( hWnd, LB_GETITEMDATA, wItem, 0L);
    if(pTask != (PTASK)LB_ERR )
    {
        lpszCmdLine = &pTask->Cmd[0];

        // Now exec it.
        startup.cb = sizeof(startup);
        startup.lpReserved = NULL;
        startup.lpDesktop = NULL;
        startup.lpTitle = NULL;
        startup.dwFlags = STARTF_USEPOSITION; // Set start position
        startup.dwX=rWnd.right+5;
        startup.dwY=rWnd.top+5;
        startup.cbReserved2 = 0;
        startup.lpReserved2 = NULL;

#ifdef DEBUG
    MessageBox(hWnd, lpszCmdLine,TEXT("DebugRun"),MB_OK);
#else
        if (CreateProcess(NULL, lpszCmdLine, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP,
                          NULL, NULL, &startup, &pi))
        {
            WaitForSingleObjectEx(pi.hProcess, INFINITE, TRUE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else
        {
            MessageBeep( MB_ICONEXCLAMATION );
        }
#endif
    }
    else
    {
        MessageBeep( MB_ICONEXCLAMATION );
    }



    // Remove any selection after the app terminates.
    SendMessage( hWnd, LB_SETCURSEL, (WPARAM)-1, 0);
    return FALSE;
}

//***************************************************************************
//
// RunAppsInList()
// Enumerates all the items in the list box, spawning each in turn.
//
// ENTRY:
//      HWND of Parent.
//
// EXIT:
//      <Params>
//
//***************************************************************************
DWORD WINAPI RunAppsInList(LPVOID lp)
{
    HWND hWnd=(HWND)lp;
    WORD i,iNumItems;
    HKEY hkey;
    HKEY hkeyDest;
    TCHAR szSubkey[MAX_PATH];
    TCHAR szWallpaper[MAX_PATH];
    DWORD cbSize;
    DWORD dwType;

    // Run all the applications in the list
    iNumItems = (WORD)SendMessage(hWnd,LB_GETCOUNT,0,0L);
    for(i=0;i<iNumItems;i++)
    {
        LaunchApp(hWnd,i);
    }

    // Delete the runonce subkey for setup.
#ifdef DEBUG
    MessageBox( hWnd, szSubkey, TEXT("Delete Key - not done"), MB_OK);
#else
    lstrcpy(szSubkey,c_szRunOnce);
    lstrcat(szSubkey,TEXT("\\Setup"));
    RegDeleteKey( HKEY_LOCAL_MACHINE, szSubkey );
#endif


    // Now see if we should reboot/restart.
    if (g_iState & (CMD_DO_REBOOT|CMD_DO_RESTART))
    {
        HKEY hkey;
        TCHAR achTitle[80];
        DWORD dwSetupFlags=0;

        //
        // because we are going to reboot, remove the VGA fallback.
        // line from OneRunce.
        //
        if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szRunOnce, &hkey) == ERROR_SUCCESS)
        {
            RegDeleteValue(hkey, szFallback);
            RegCloseKey(hkey);
        }

        szWallpaper[0]=0;
        LoadString(g_hInst, IDS_PAMPER, szWallpaper, sizeof(szWallpaper));
        GetWindowText(GetParent(hWnd), achTitle, sizeof(achTitle));

        // Get the setup flags.
        if(RegOpenKey(HKEY_LOCAL_MACHINE, c_szSetup, &hkey) == ERROR_SUCCESS)
        {
            cbSize=sizeof(dwSetupFlags);
            if(RegQueryValueEx(hkey, REGSTR_VAL_SETUPFLAGS, NULL , &dwType, (LPBYTE)&dwSetupFlags, &cbSize) != ERROR_SUCCESS )
                dwSetupFlags=0;
            RegCloseKey(hkey);
        }
        //
        //  always reboot the system, dont give the user a choice.
        //
        //  alow OEMs not to have to click OK.
#ifdef DEBUG
        MessageBox(hWnd,dwSetupFlags & SUF_BATCHINF?TEXT("Batchfile used"):TEXT("No batch"),TEXT("Batch"),MB_OK);
#endif
        if( !(dwSetupFlags & SUF_BATCHINF) || !GetPrivateProfileInt(TEXT("Setup"),TEXT("NoPrompt2Boot"),0,TEXT("MSBATCH.INF")))
            MessageBox(GetParent(hWnd),szWallpaper,achTitle,MB_OK|MB_ICONEXCLAMATION);
#ifndef DEBUG
        ExitWindowsEx(EWX_REBOOT, 0);
#endif
    }

    PostMessage(GetParent(hWnd),WM_FINISHED,0,0L);
    return 0;
}


//***************************************************************************
//
// <Function>()
// <Explanation>
//
// ENTRY:
//      <Params>
//
// EXIT:
//      <Params>
//
//***************************************************************************
#define CXBORDER 3

LRESULT   _HandleLBMeasureItem(HWND hwndLB, MEASUREITEMSTRUCT  *lpmi)
{
    RECT    rWnd;
    int     wWnd;
    HDC     hDC;
    HFONT   hfontOld;
    PTASK   pTask;

    // Get the Height and Width of the child window
    GetWindowRect (hwndLB, &rWnd);
    wWnd = rWnd.right - rWnd.left;

    lpmi->itemWidth = wWnd;

    pTask = (PTASK)lpmi->itemData;

    hDC= GetDC(NULL);
    if( (hfontOld  = SelectObject(hDC,g_hBoldFont)) != 0 )
    {
        rWnd.top    = 0;
        rWnd.left   = CXBORDER*2 + g_cxSmIcon;
        rWnd.right  = lpmi->itemWidth - rWnd.left - CXBORDER*2 - g_cxSmIcon;
        rWnd.bottom = 0;
        DrawText(hDC,pTask->Text, lstrlen(pTask->Text),&rWnd, DT_CALCRECT | DT_WORDBREAK );
        SelectObject(hDC, hfontOld);
    }
    ReleaseDC(NULL,hDC);

    lpmi->itemHeight = rWnd.bottom + 2*CXBORDER;

    return TRUE;
}

//---------------------------------------------------------------------------
//***************************************************************************
//
// <Function>()
// <Explanation>
//
// ENTRY:
//      <Params>
//
// EXIT:
//      <Params>
//
//***************************************************************************
LRESULT   _HandleMeasureItem(HWND hwnd, MEASUREITEMSTRUCT  *lpmi)
{
    if (lpmi->CtlType == ODT_LISTBOX)
        return _HandleLBMeasureItem(hwnd, lpmi);
    return TRUE;
}

//---------------------------------------------------------------------------
//***************************************************************************
//
// _HandleLBDrawItem()
//  Draws the Title, Text, and icon for an entry.
//
// ENTRY:
//      HWND and the Item to draw.
//
// EXIT:
//      <Params>
//
//***************************************************************************
LRESULT   _HandleLBDrawItem(HWND hwndLB, DRAWITEMSTRUCT  *lpdi)
{
    RECT rc;
    HFONT hfontOld;
    int xArrow,y;
    PTASK pTask;
    BITMAP bm;
    HGDIOBJ hbmArrow,hbmOld;

    // Don't draw anything for an empty list.
    if ((int)lpdi->itemID < 0)
        return TRUE;

    pTask = (PTASK)lpdi->itemData;
    if(pTask == (PTASK)LB_ERR || !pTask )
        return FALSE;

    if ((lpdi->itemAction & ODA_SELECT) || (lpdi->itemAction & ODA_DRAWENTIRE))
    {
        // Put in the Title text
        hfontOld  = SelectObject(lpdi->hDC,(lpdi->itemState & ODS_SELECTED)?g_hBoldFont:g_hfont);
        ExtTextOut(lpdi->hDC,
                   lpdi->rcItem.left+ CXBORDER*2 + g_cxSmIcon,
                   lpdi->rcItem.top+CXBORDER,
                   ETO_OPAQUE,
                   &lpdi->rcItem,
                   NULL, 0,
                   NULL);
        rc.top    = lpdi->rcItem.top    + CXBORDER;
        rc.left   = lpdi->rcItem.left   + CXBORDER*2 + g_cxSmIcon;
        rc.right  = lpdi->rcItem.right;
        rc.bottom = lpdi->rcItem.bottom;
        DrawText( lpdi->hDC,
                   pTask->Text, lstrlen(pTask->Text),
                   &rc,
                   DT_WORDBREAK);
        SelectObject(lpdi->hDC, hfontOld);

    // Draw the little triangle thingies.
    if(lpdi->itemState & ODS_SELECTED)
    {
        if (!g_hdcMem)
        {
            g_hdcMem = CreateCompatibleDC(lpdi->hDC);
        }
        // selected SRCSTENCIL=0x00d8074a
        // not selected SRCAND.
        if (g_hdcMem)
        {
            hbmArrow = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_MNARROW));
            GetObject(hbmArrow, sizeof(bm), &bm);
            hbmOld = SelectObject(g_hdcMem, hbmArrow);
            xArrow = lpdi->rcItem.left + CXBORDER; // - bm.bmWidth;
            y = ((g_SizeTextExt.cy - bm.bmHeight)/2) + CXBORDER + lpdi->rcItem.top;
            BitBlt(lpdi->hDC, xArrow, y, bm.bmWidth, bm.bmHeight, g_hdcMem, 0, 0, SRCAND); // dwRop);
            SelectObject(g_hdcMem, hbmOld);
            DeleteObject(hbmArrow);
        }
    }
    }
    return TRUE;
}

//---------------------------------------------------------------------------
LRESULT   _HandleCtlColorListbox(HWND hwnd, HDC hdc)
{
    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
    return (LRESULT) g_hbrBkGnd;
}

//---------------------------------------------------------------------------
LRESULT   _HandleDrawItem(HWND hwnd, DRAWITEMSTRUCT  *lpdi)
{
    if (lpdi->CtlType == ODT_LISTBOX)
        return _HandleLBDrawItem(hwnd, lpdi);
    return TRUE;
}

//---------------------------------------------------------------------------
LRESULT   _HandleDeleteItem(HWND hwnd, DELETEITEMSTRUCT  *lpdi)
{
    if(lpdi)
        if(lpdi->itemData)
        {
            LocalFree( (HLOCAL)lpdi->itemData );
            return TRUE;
        }
    return FALSE;
}

//***************************************************************************
//
// ShrinkToFit()
//     Makes the List box no bigger then it has to be
//     makes the parent window rsize to the LB size.
//
// ENTRY:
//     hwnd Parent
//     hwnd List box
//
// EXIT:
//
//***************************************************************************
void ShrinkToFit( HWND hWnd, HWND hLb )
{
    LONG lCount;
    LONG lNumItems;
    LONG lTotalHeight;
    LONG lHeight;
    RECT rWnd;
    LONG lChange;

    lNumItems = (LONG)SendMessage( hLb, LB_GETCOUNT, 0, 0L );
    lTotalHeight =0;
    for( lCount=0;lCount<lNumItems; lCount++ )
    {
         lHeight = (LONG)SendMessage( hLb, LB_GETITEMHEIGHT, lCount, 0L );
         lTotalHeight+=lHeight;
    }

    // Set the height of the ListBox to the number of items in it.
    GetWindowRect (hLb, &rWnd);
    SetWindowPos( hLb, hWnd, 0,0,
        rWnd.right - rWnd.left - (CXBORDER*2 + g_cxSmIcon) ,
        lTotalHeight,
        SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOZORDER );

    // Work out how much it changed in height
    lChange = lTotalHeight - (rWnd.bottom-rWnd.top);

    // Size the parent to fit around the child.
    GetWindowRect(hWnd, &rWnd);
    SetWindowPos( hWnd,0, 0,0,
        rWnd.right - rWnd.left,
        rWnd.bottom-rWnd.top + lChange,
        SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOZORDER );
}


//***************************************************************************
//
// <Function>()
// <Explanation>
//
// ENTRY:
//      <Params>
//
// EXIT:
//      <Params>
//
//***************************************************************************
LRESULT CALLBACK dlgProcRunOnce(
                HWND hWnd,         // window handle
                UINT message,      // type of message
                WPARAM uParam,     // additional information
                LPARAM lParam)     // additional information
{
    int wmId, wmEvent;
    HANDLE hThread;

    switch (message)
    {
        case WM_DELETEITEM:
            return _HandleDeleteItem( hWnd, (LPDELETEITEMSTRUCT)lParam );

        case WM_MEASUREITEM:
            return _HandleMeasureItem(hWnd, (MEASUREITEMSTRUCT  *) lParam);

        case WM_DRAWITEM:
            return _HandleDrawItem(hWnd, (DRAWITEMSTRUCT  *) lParam);

        case WM_INITDIALOG:
            CreateGlobals( hWnd );
            DoAnyRandomOneTimeStuff();
            g_fCleanBoot = GetSystemMetrics(SM_CLEANBOOT);
            TopLeftWindow( hWnd, GetParent(hWnd) );
            RunOnceFill( GetDlgItem(hWnd,IDC_LIST2) );
            // Now calculate the size needed for the LB and resize LB and parent.
            ShrinkToFit( hWnd, GetDlgItem(hWnd,IDC_LIST2));
            hThread = CreateThread(NULL, 0, RunAppsInList, (LPVOID)GetDlgItem(hWnd,IDC_LIST2),0, &g_dwThread );
            CloseHandle(hThread);
        break;

        case WM_FINISHED:
            EndDialog(hWnd,0);
            // DestroyWindow(hWnd);
        break;

        case WM_CTLCOLORLISTBOX:
            return _HandleCtlColorListbox((HWND)lParam, (HDC)uParam);

        case WM_COMMAND:  // message: command from application menu
            wmId    = LOWORD(uParam);
            wmEvent = HIWORD(uParam);
            if( wmEvent==LBN_SELCHANGE )
            {
                // LaunchApp( (HWND) lParam, LOWORD(uParam) );
                // De-select the item now.
                break;
            }
            else
            switch (wmId)
            {
                case IDOK:
                    EndDialog( hWnd, wmId);
                break;

                default:
                    // return (DefWindowProc(hWnd, message, uParam, lParam));
                break;
            }
        break;


        default:          // Passes it on if unproccessed
           // return (DefWindowProc(hWnd, message, uParam, lParam));
        return FALSE;
    }
    return TRUE;
}
