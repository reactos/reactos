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

#include "mmsys.h"
#include "resource.h"

typedef enum
{
    HWPD_STANDARDLIST = 0,
    HWPD_LARGELIST,
    HWPD_MAX = HWPD_LARGELIST
} HWPAGE_DISPLAYMODE, *PHWPAGE_DISPLAYMODE;

typedef struct _IMGINFO
{
    HBITMAP hBitmap;
    INT cxSource;
    INT cySource;
} IMGINFO, *PIMGINFO;

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

static VOID
InitImageInfo(PIMGINFO ImgInfo)
{
    BITMAP bitmap;

    ZeroMemory(ImgInfo, sizeof(*ImgInfo));

    ImgInfo->hBitmap = LoadImage(hApplet,
                                 MAKEINTRESOURCE(IDB_SPEAKIMG),
                                 IMAGE_BITMAP,
                                 0,
                                 0,
                                 LR_DEFAULTCOLOR);

    if (ImgInfo->hBitmap != NULL)
    {
        GetObject(ImgInfo->hBitmap, sizeof(BITMAP), &bitmap);

        ImgInfo->cxSource = bitmap.bmWidth;
        ImgInfo->cySource = bitmap.bmHeight;
    }
}

/* Volume property page dialog callback */
//static INT_PTR CALLBACK
INT_PTR CALLBACK
VolumeDlgProc(HWND hwndDlg,
	        UINT uMsg,
	        WPARAM wParam,
	        LPARAM lParam)
{
    static IMGINFO ImgInfo;
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            InitImageInfo(&ImgInfo);
            break;
        }
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
            if(lpDrawItem->CtlID == IDC_SPEAKIMG)
            {
                HDC hdcMem;
                LONG left;

                /* position image in centre of dialog */
                left = (lpDrawItem->rcItem.right - ImgInfo.cxSource) / 2;

                hdcMem = CreateCompatibleDC(lpDrawItem->hDC);
                if (hdcMem != NULL)
                {
                    SelectObject(hdcMem, ImgInfo.hBitmap);
                    BitBlt(lpDrawItem->hDC,
                           left,
                           lpDrawItem->rcItem.top,
                           lpDrawItem->rcItem.right - lpDrawItem->rcItem.left,
                           lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
                           hdcMem,
                           0,
                           0,
                           SRCCOPY);
                    DeleteDC(hdcMem);
                }
            }
            return TRUE;
        }
    }

    return FALSE;
}



/* Audio property page dialog callback */
static INT_PTR CALLBACK
AudioDlgProc(HWND hwndDlg,
	        UINT uMsg,
	        WPARAM wParam,
	        LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);
    switch(uMsg)
    {
        case WM_INITDIALOG:
        break;
    }

    return FALSE;
}

/* Voice property page dialog callback */
static INT_PTR CALLBACK
VoiceDlgProc(HWND hwndDlg,
	        UINT uMsg,
	        WPARAM wParam,
	        LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);
    switch(uMsg)
    {
        case WM_INITDIALOG:
        break;
    }

    return FALSE;
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
    UNREFERENCED_PARAMETER(hwnd);

    LoadString(hApplet,
               IDS_CPLNAME,
               Caption,
               sizeof(Caption) / sizeof(TCHAR));

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
    psh.hwndParent = NULL;
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
