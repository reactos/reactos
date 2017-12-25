/*
 * PROJECT:     Ports installer library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\msports\serial.c
 * PURPOSE:     Serial Port property functions
 * COPYRIGHT:   Copyright 2011 Eric Kohl
 */

#include "precomp.h"

static
BOOL
OnInitSerialSettingsDialog(HWND hwnd,
             WPARAM wParam,
             LPARAM lParam)
{
    FIXME("Port_OnInit()\n");
    return TRUE;
}


static
INT_PTR
CALLBACK
SerialSettingsDlgProc(HWND hwnd,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam)
{
    FIXME("SerialSettingsDlgProc()\n");

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return OnInitSerialSettingsDialog(hwnd, wParam, lParam);
    }

    return FALSE;
}


BOOL
WINAPI
SerialPortPropPageProvider(PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
                           LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
                           LPARAM lParam)
{
    PROPSHEETPAGEW PropSheetPage;
    HPROPSHEETPAGE hPropSheetPage;

    FIXME("SerialPortPropPageProvider(%p %p %lx)\n",
          lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);

    if (lpPropSheetPageRequest->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
    {
        FIXME("SPPSR_ENUM_ADV_DEVICE_PROPERTIES\n");

        PropSheetPage.dwSize = sizeof(PROPSHEETPAGEW);
        PropSheetPage.dwFlags = 0;
        PropSheetPage.hInstance = hInstance;
        PropSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SERIALSETTINGS);
        PropSheetPage.pfnDlgProc = SerialSettingsDlgProc;
        PropSheetPage.lParam = 0;
        PropSheetPage.pfnCallback = NULL;

        hPropSheetPage = CreatePropertySheetPageW(&PropSheetPage);
        if (hPropSheetPage == NULL)
        {
            FIXME("CreatePropertySheetPageW() failed!\n");
            return FALSE;
        }

        if (!(*lpfnAddPropSheetPageProc)(hPropSheetPage, lParam))
        {
            FIXME("lpfnAddPropSheetPageProc() failed!\n");
            DestroyPropertySheetPage(hPropSheetPage);
            return FALSE;
        }
    }

    FIXME("Done!\n");

    return TRUE;
}

/* EOF */