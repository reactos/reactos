/*
 *  ReactOS Control Panel Applet
 *
 *  main.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <cpl.h>
#include <commctrl.h>
#include <tchar.h>

#include <stdio.h>
//#include <shellapi.h>

#include "main.h"
#include "system.h"

#ifndef __GNUC__
//#define CPL_STARTWPARAMS CPL_STARTWPARMS
//#define CPL_DBLCLICK CPL_DBLCLK
#endif

////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

extern HMODULE hModule;

typedef struct tagCPlAppletInstanceData {
    HWND hWnd;
    LONG lData;
} CPlAppletInstanceData;

CPlAppletInstanceData PowerAppletData = { 0, 0L };


////////////////////////////////////////////////////////////////////////////////
// Local Support Functions:
//

static LONG LaunchPowerApplet(UINT uAppNum, LONG lData, BOOL start)
{ 
/*
    if (start && !hControlPanelAppletDlg) {
#if 1
        DialogBox(hModule, MAKEINTRESOURCE(IDD_CPL_POWER_DIALOG), NULL, (DLGPROC)CPlAppletDlgProc);
#else
//        hControlPanelAppletDlg = CreateDialog(hModule, MAKEINTRESOURCE(IDD_CPL_POWER_DIALOG), NULL, (DLGPROC)CPlAppletDlgProc);
        hControlPanelAppletDlg = CreateDialogParam(hModule, MAKEINTRESOURCE(IDD_CPL_POWER_DIALOG),
                           NULL, (DLGPROC)CPlAppletDlgProc, (LPARAM)lData);
        if (hControlPanelAppletDlg) {
            ShowWindow(hControlPanelAppletDlg, SW_SHOW);
        }
#endif
    } else if (hControlPanelAppletDlg) {
        DestroyWindow(hControlPanelAppletDlg);
        hControlPanelAppletDlg = 0;
    }
 */
    return 0L; 
}

////////////////////////////////////////////////////////////////////////////////

typedef LONG (*LaunchAppletProc)(UINT, LONG, BOOL);

typedef struct tagCPLAppletINFO {
    UINT _uAppNum; 
    CPLINFO info;
    LaunchAppletProc proc;
    CPlAppletInstanceData* pData;
} CPLAppletINFO; 

static LONG LaunchCommonAppletDlg(UINT uAppNum, LONG lData, BOOL start);

static CPLAppletINFO cplAppletInfoArray[] = {
    { 0, { IDI_CPL_POWER_ICON, IDS_CPL_POWER_NAME, IDS_CPL_POWER_INFO, 0L }, LaunchCommonAppletDlg, &PowerAppletData },
};

static LONG LaunchCommonAppletDlg(UINT uAppNum, LONG lData, BOOL start)
{ 
    if (start) {

        if (!cplAppletInfoArray[uAppNum].pData->hWnd) {
            cplAppletInfoArray[uAppNum].pData->hWnd = CreateDialogParam(hModule, MAKEINTRESOURCE(IDD_CPL_POWER_DIALOG), NULL, (DLGPROC)CPlAppletDlgProc, (LPARAM)(cplAppletInfoArray[uAppNum].pData));
            ShowWindow(cplAppletInfoArray[uAppNum].pData->hWnd, SW_SHOW);
        } else {
            // find the applet window, make forground and set focus
            SetForegroundWindow(cplAppletInfoArray[uAppNum].pData->hWnd);
        }
    } else {
        if (cplAppletInfoArray[uAppNum].pData->hWnd) {
            DestroyWindow(cplAppletInfoArray[uAppNum].pData->hWnd);
            cplAppletInfoArray[uAppNum].pData->hWnd = NULL;
        }
    }
    return 0L; 
}

////////////////////////////////////////////////////////////////////////////////

static LONG InitialiseProc(VOID) // return zero on failure to initialise, host will then drop us
{
    // if (failed_to_initialise) return 0;
    return 1L;
}

LONG GetCountProc(VOID)
{
    return (sizeof(cplAppletInfoArray)/sizeof(CPLAppletINFO));
}

static LONG InquireProc(UINT uAppNum, CPLINFO* pCPLInfo)
{
    if (uAppNum < (sizeof(cplAppletInfoArray)/sizeof(CPLAppletINFO))) {
        if (pCPLInfo != NULL) {
            pCPLInfo->idName = CPL_DYNAMIC_RES;
            pCPLInfo->idInfo = CPL_DYNAMIC_RES;
            pCPLInfo->idIcon = CPL_DYNAMIC_RES;
            return 0L;
        }
    }
    return 1L;
}

static LONG NewInquireProc(UINT uAppNum, NEWCPLINFO* pNewCPLInfo)
{
    if (uAppNum < (sizeof(cplAppletInfoArray)/sizeof(CPLAppletINFO))) {
        if (pNewCPLInfo != NULL && pNewCPLInfo->dwSize == sizeof(NEWCPLINFO)) {
            pNewCPLInfo->dwSize = (DWORD)sizeof(NEWCPLINFO);
            pNewCPLInfo->dwFlags = 0;
            pNewCPLInfo->dwHelpContext = 0;
            pNewCPLInfo->lData = cplAppletInfoArray[uAppNum].pData->lData;
            //pNewCPLInfo->hIcon = LoadImage(hModule, (LPCTSTR)cplAppletInfoArray[uAppNum].info.idIcon, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
            pNewCPLInfo->hIcon = LoadImage(hModule, MAKEINTRESOURCE(cplAppletInfoArray[uAppNum].info.idIcon), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
            pNewCPLInfo->szHelpFile[0] = _T('\0');
            LoadString(hModule, cplAppletInfoArray[uAppNum].info.idName, pNewCPLInfo->szName, 32);
            LoadString(hModule, cplAppletInfoArray[uAppNum].info.idInfo, pNewCPLInfo->szInfo, 64);
            return 0L;
        }
    }
    return 1L;
}

static LONG StartWithParamsProc(UINT uAppNum, LPCTSTR lpszExtra) // return TRUE if handled, FALSE otherwise
{
    return FALSE;
}

static LONG DblClickProc(UINT uAppNum, LONG lData) // return zero on success, nonzero otherwise
{
    if (uAppNum < (sizeof(cplAppletInfoArray)/sizeof(CPLAppletINFO))) {
        return cplAppletInfoArray[uAppNum].proc(uAppNum, lData, TRUE);
    }
    return 1L;
}

static LONG StopProc(UINT uAppNum, LONG lData) // return zero if processed successfully
{
    if (uAppNum < (sizeof(cplAppletInfoArray)/sizeof(CPLAppletINFO))) {
        return cplAppletInfoArray[uAppNum].proc(uAppNum, lData, FALSE);
    }
    return 1L;
}

static LONG ExitProc(VOID) // return zero if processed successfully
{
    return 0L;
}

////////////////////////////////////////////////////////////////////////////////

LONG WINAPI CPlApplet(HWND hWnd, UINT message, LONG lParam1, LONG lParam2)
{
    switch (message) {
    case CPL_INIT:
        return InitialiseProc();
    case CPL_GETCOUNT:
        return GetCountProc();
    case CPL_INQUIRE:
        return InquireProc((UINT)lParam1, (CPLINFO*)lParam2);
    case CPL_NEWINQUIRE:
        return NewInquireProc((UINT)lParam1, (NEWCPLINFO*)lParam2);
    case CPL_STARTWPARMS:
        return StartWithParamsProc((UINT)lParam1, (LPCTSTR)lParam2);
    case CPL_DBLCLK:
        return DblClickProc((UINT)lParam1, lParam2);
    case CPL_STOP:
        return StopProc((UINT)lParam1, lParam2);
    case CPL_EXIT:
        return ExitProc();
    default:
        return 0L;
    }
    return 1L;
}
