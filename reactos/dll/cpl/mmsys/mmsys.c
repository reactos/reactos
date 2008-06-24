/*
 *  ReactOS
 *  Copyright (C) 2005 ReactOS Team
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
/* $Id: main.c 12852 2005-01-06 13:58:04Z mf $
 *
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            lib/cpl/mmsys/mmsys.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Thoams Weidenmueller <w3seek@reactos.com>
 *                  Dmitry Chapyshev <dmitry@reactos.org>
 * UPDATE HISTORY:
 *      2005/11/23  Created
 */
#include <windows.h>
#include <commctrl.h>
#include <initguid.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <devguid.h>
#include <cpl.h>
#include <debug.h>

#include "mmsys.h"
#include "resource.h"

typedef enum
{
    HWPD_STANDARDLIST = 0,
    HWPD_LARGELIST,
    HWPD_MAX = HWPD_LARGELIST
} HWPAGE_DISPLAYMODE, *PHWPAGE_DISPLAYMODE;

HWND WINAPI
DeviceCreateHardwarePageEx(HWND hWndParent,
                           LPGUID lpGuids,
                           UINT uNumberOfGuids,
                           HWPAGE_DISPLAYMODE DisplayMode);

#define NUM_APPLETS	(1)


HINSTANCE hApplet = 0;

/* Applets */
const APPLET Applets[NUM_APPLETS] =
{
  {IDI_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, MmSysApplet},
};


HRESULT WINAPI
DllCanUnloadNow(VOID)
{
    DPRINT1("DllCanUnloadNow() stubs\n");
    return S_OK;
}

HRESULT WINAPI
DllGetClassObject(REFCLSID rclsid,
                  REFIID riid,
                  LPVOID *ppv)
{
    DPRINT1("DllGetClassObject() stubs\n");
    return S_OK;
}


VOID WINAPI
ShowDriverSettingsAfterForkW(HWND hwnd,
                             HINSTANCE hInstance,
                             LPWSTR lpszCmd,
                             int nCmdShow)
{
    DPRINT1("ShowDriverSettingsAfterForkW() stubs\n");
}

VOID WINAPI
ShowDriverSettingsAfterForkA(HWND hwnd,
                             HINSTANCE hInstance,
                             LPTSTR lpszCmd,
                             int nCmdShow)
{
    DPRINT1("ShowDriverSettingsAfterForkA() stubs\n");
}

VOID WINAPI
ShowDriverSettingsAfterFork(HWND hwnd,
                            HINSTANCE hInstance,
                            LPTSTR lpszCmd,
                            int nCmdShow)
{
    DPRINT1("ShowDriverSettingsAfterFork() stubs\n");
}

BOOL WINAPI
ShowMMCPLPropertySheet(HWND hwnd,
                       LPCTSTR pszPropSheet,
                       LPTSTR pszName,
                       LPTSTR pszCaption)
{
    DPRINT1("ShowMMCPLPropertySheet() stubs\n");
    return TRUE;
}

VOID WINAPI
ShowAudioPropertySheet(HWND hwnd,
                       HINSTANCE hInstance,
                       LPTSTR lpszCmd,
                       int nCmdShow)
{
    DPRINT1("ShowAudioPropertySheet() stubs\n");
}

VOID WINAPI
mmseRunOnceW(HWND hwnd,
             HINSTANCE hInstance,
             LPWSTR lpszCmd,
             int nCmdShow)
{
    DPRINT1("mmseRunOnceW() stubs\n");
}

VOID WINAPI
mmseRunOnceA(HWND hwnd,
             HINSTANCE hInstance,
             LPTSTR lpszCmd,
             int nCmdShow)
{
    DPRINT1("mmseRunOnceA() stubs\n");
}

VOID WINAPI
mmseRunOnce(HWND hwnd,
            HINSTANCE hInstance,
            LPSTR lpszCmd,
            int nCmdShow)
{
    DPRINT1("mmseRunOnce() stubs\n");
}

DWORD WINAPI
MediaClassInstaller(DI_FUNCTION diFunction,
                    HDEVINFO hDevInfo,
                    PSP_DEVINFO_DATA pspDevInfoData)
{
    DPRINT1("MediaClassInstaller() stubs\n");
    return 0x0;
}

BOOL WINAPI
MediaPropPageProvider(LPVOID Info,
                      LPFNADDPROPSHEETPAGE PropSheetPage,
                      LPARAM lParam)
{
    DPRINT1("MediaPropPageProvider() stubs\n");
    return TRUE;
}

VOID WINAPI
ShowFullControlPanel(HWND hwnd,
                     HINSTANCE hInstance,
                     LPTSTR lpszCmd,
                     int nCmdShow)
{
    DPRINT1("ShowFullControlPanel() stubs\n");
}


/* Hardware property page dialog callback */
static INT_PTR CALLBACK
HardwareDlgProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            GUID Guids[2];
            Guids[0] = GUID_DEVCLASS_CDROM;
            Guids[1] = GUID_DEVCLASS_MEDIA;

            /* create the hardware page */
            DeviceCreateHardwarePageEx(hwndDlg,
                                       Guids,
                                       sizeof(Guids) / sizeof(Guids[0]),
                                       HWPD_LARGELIST);
            break;
        }
    }

    return FALSE;
}

LONG APIENTRY
MmSysApplet(HWND hwnd,
            UINT uMsg,
            LPARAM wParam,
            LPARAM lParam)
{
    PROPSHEETPAGE psp[5];
    PROPSHEETHEADER psh; //= {0};
    TCHAR Caption[256];

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(uMsg);

    LoadString(hApplet,
               IDS_CPLNAME,
               Caption,
               sizeof(Caption) / sizeof(TCHAR));

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
    psh.hwndParent = hwnd;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet,
                         MAKEINTRESOURCE(IDI_CPLICON));
    psh.pszCaption = Caption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[0], IDD_VOLUME,VolumeDlgProc);
    InitPropSheetPage(&psp[1], IDD_SOUNDS,SoundsDlgProc);
    InitPropSheetPage(&psp[2], IDD_AUDIO,AudioDlgProc);
    InitPropSheetPage(&psp[3], IDD_VOICE,VoiceDlgProc);
    InitPropSheetPage(&psp[4], IDD_HARDWARE,HardwareDlgProc);

    return (LONG)(PropertySheet(&psh) != -1);
}

VOID
InitPropSheetPage(PROPSHEETPAGE *psp,
                  WORD idDlg,
                  DLGPROC DlgProc)
{
    ZeroMemory(psp, sizeof(PROPSHEETPAGE));
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hApplet;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pfnDlgProc = DlgProc;
}


/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCpl,
          UINT uMsg,
          LPARAM lParam1,
          LPARAM lParam2)
{
    switch(uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
        {
            CPLINFO *CPlInfo = (CPLINFO*)lParam2;
            UINT uAppIndex = (UINT)lParam1;

            CPlInfo->lData = 0;
            CPlInfo->idIcon = Applets[uAppIndex].idIcon;
            CPlInfo->idName = Applets[uAppIndex].idName;
            CPlInfo->idInfo = Applets[uAppIndex].idDescription;
            break;
        }

        case CPL_DBLCLK:
        {
            UINT uAppIndex = (UINT)lParam1;
            Applets[uAppIndex].AppletProc(hwndCpl,
                                          uMsg,
                                          lParam1,
                                          lParam2);
            break;
        }
    }

    return FALSE;
}


BOOL STDCALL
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hApplet = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}
