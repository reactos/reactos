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
#include <stdlib.h>
#include <tchar.h>
//#include <shellapi.h>
#include "main.h"
#include "system.h"
#include "trace.h"

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

CPlAppletInstanceData SystemAppletData = { 0, 0L };
CPlAppletInstanceData IrDaAppletData = { 0, 1L };
CPlAppletInstanceData MouseAppletData = { 0, 2L };
CPlAppletInstanceData KeyboardAppletData = { 0, 3L };
CPlAppletInstanceData RegionsAppletData = { 0, 4L };
CPlAppletInstanceData UsersAppletData = { 0, 5L };
CPlAppletInstanceData DateTimeAppletData = { 0, 6L };
CPlAppletInstanceData FoldersAppletData = { 0, 7L };
CPlAppletInstanceData DisplayAppletData = { 0, 8L };
CPlAppletInstanceData AccessibleAppletData = { 0, 9L };
CPlAppletInstanceData PhoneModemAppletData = { 0, 10L };


////////////////////////////////////////////////////////////////////////////////
// Local Support Functions:
//


typedef LONG (*LaunchAppletProc)(UINT, LONG, BOOL);

typedef struct tagCPLAppletINFO {
    UINT _uAppNum; 
    CPLINFO info;
    LaunchAppletProc proc;
    CPlAppletInstanceData* pData;
} CPLAppletINFO; 

static LONG LaunchCommonAppletDlg(UINT uAppNum, LONG lData, BOOL start);

static CPLAppletINFO cplAppletInfoArray[] = {
    { 0, IDI_CPL_SYSTEM_ICON, IDS_CPL_SYSTEM_NAME, IDS_CPL_SYSTEM_INFO, 0L, LaunchCommonAppletDlg, &SystemAppletData },
    { 1, IDI_CPL_IRDA_ICON, IDS_CPL_IRDA_NAME, IDS_CPL_IRDA_INFO, 0L, LaunchCommonAppletDlg, &IrDaAppletData },
    { 2, IDI_CPL_MOUSE_ICON, IDS_CPL_MOUSE_NAME, IDS_CPL_MOUSE_INFO, 0L, LaunchCommonAppletDlg, &MouseAppletData },
    { 3, IDI_CPL_KEYBOARD_ICON, IDS_CPL_KEYBOARD_NAME, IDS_CPL_KEYBOARD_INFO, 0L, LaunchCommonAppletDlg, &KeyboardAppletData },
    { 4, IDI_CPL_REGIONS_ICON, IDS_CPL_REGIONS_NAME, IDS_CPL_REGIONS_INFO, 0L, LaunchCommonAppletDlg, &RegionsAppletData },
    { 5, IDI_CPL_USERS_ICON, IDS_CPL_USERS_NAME, IDS_CPL_USERS_INFO, 0L, LaunchCommonAppletDlg, &UsersAppletData },
    { 6, IDI_CPL_DATETIME_ICON, IDS_CPL_DATETIME_NAME, IDS_CPL_DATETIME_INFO, 0L, LaunchCommonAppletDlg, &DateTimeAppletData },
    { 7, IDI_CPL_FOLDERS_ICON, IDS_CPL_FOLDERS_NAME, IDS_CPL_FOLDERS_INFO, 0L, LaunchCommonAppletDlg, &FoldersAppletData },
    { 8, IDI_CPL_DISPLAY_ICON, IDS_CPL_DISPLAY_NAME, IDS_CPL_DISPLAY_INFO, 0L, LaunchCommonAppletDlg, &DisplayAppletData },
    { 9, IDI_CPL_ACCESSIBLE_ICON, IDS_CPL_ACCESSIBLE_NAME, IDS_CPL_ACCESSIBLE_INFO, 0L, LaunchCommonAppletDlg, &AccessibleAppletData },
    {10, IDI_CPL_PHONE_MODEM_ICON, IDS_CPL_PHONE_MODEM_NAME, IDS_CPL_PHONE_MODEM_INFO, 0L, LaunchCommonAppletDlg, &PhoneModemAppletData },
};


static LONG LaunchCommonAppletDlg(UINT uAppNum, LONG lData, BOOL start)
{ 
    if (start) {

        if (!cplAppletInfoArray[uAppNum].pData->hWnd) {
            cplAppletInfoArray[uAppNum].pData->hWnd = CreateDialogParam(hModule, MAKEINTRESOURCE(IDD_CPL_TABBED_DIALOG), NULL, (DLGPROC)CPlAppletDlgProc, (LPARAM)(cplAppletInfoArray[uAppNum].pData));
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
            pCPLInfo->idName = cplAppletInfoArray[uAppNum].info.idName;
            pCPLInfo->idInfo = cplAppletInfoArray[uAppNum].info.idInfo;
            pCPLInfo->idIcon = cplAppletInfoArray[uAppNum].info.idIcon;
            pCPLInfo->lData =  cplAppletInfoArray[uAppNum].pData->lData;
            return 0L;
        }
    }
    return 1L;
}

static LONG NewInquireProc(UINT uAppNum, NEWCPLINFO* pNewCPLInfo)
{
    if (uAppNum < (sizeof(cplAppletInfoArray)/sizeof(CPLAppletINFO))) {
        if (pNewCPLInfo != NULL) {
//            pNewCPLInfo->idName = cplAppletInfoArray[uAppNum].info.idName;
//            pNewCPLInfo->idInfo = cplAppletInfoArray[uAppNum].info.idInfo;
//            pNewCPLInfo->idIcon = cplAppletInfoArray[uAppNum].info.idIcon;
//            return 0L;
        }
    }
    return 1L;
}

static LONG StartWithParamsProc(UINT uAppNum, LPCTSTR lpszExtra) // return TRUE if handled, FALSE otherwise
{
    TRACE(_T("StartWithParamsProc(%u, %s)\n"), uAppNum, lpszExtra);

    return FALSE;
}

static LONG DblClickProc(UINT uAppNum, LONG lData) // return zero on success, nonzero otherwise
{
    TRACE(_T("DblClickProc(%u, %d)\n"), uAppNum, lData);

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

static BOOL initialised = FALSE;

LONG WINAPI CPlApplet(HWND hWnd, UINT message, LONG lParam1, LONG lParam2)
{
    if (!initialised) { // this probably isn't required, or at least move to DllMain ?
        initialised = TRUE;
        // Initialize the Windows Common Controls DLL
        InitCommonControls();
    }
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
