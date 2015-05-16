/*
 * PROJECT:     Ports installer library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\msports\parallel.c
 * PURPOSE:     Parallel Port property functions
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#include "precomp.h"

static
BOOL
OnInitDialog(HWND hwnd,
             WPARAM wParam,
             LPARAM lParam)
{
    TRACE("Port_OnInit()\n");
    return TRUE;
}


static
INT_PTR
CALLBACK
ParallelSettingsDlgProc(HWND hwnd,
                        UINT uMsg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    TRACE("ParallelSettingsDlgProc()\n");

    switch (uMsg)
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd, IDC_NEVER_INTERRUPT, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            return OnInitDialog(hwnd, wParam, lParam);
    }

    return FALSE;
}


BOOL
WINAPI
ParallelPortPropPageProvider(PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
                             LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
                             LPARAM lParam)
{
    PROPSHEETPAGEW PropSheetPage;
    HPROPSHEETPAGE hPropSheetPage;

    TRACE("ParallelPortPropPageProvider(%p %p %lx)\n",
          lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);

    if (lpPropSheetPageRequest->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
    {
        TRACE("SPPSR_ENUM_ADV_DEVICE_PROPERTIES\n");

        PropSheetPage.dwSize = sizeof(PROPSHEETPAGEW);
        PropSheetPage.dwFlags = 0;
        PropSheetPage.hInstance = hInstance;
        PropSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PARALLELSETTINGS);
        PropSheetPage.pfnDlgProc = ParallelSettingsDlgProc;
        PropSheetPage.lParam = 0;
        PropSheetPage.pfnCallback = NULL;

        hPropSheetPage = CreatePropertySheetPageW(&PropSheetPage);
        if (hPropSheetPage == NULL)
        {
            TRACE("CreatePropertySheetPageW() failed!\n");
            return FALSE;
        }

        if (!(*lpfnAddPropSheetPageProc)(hPropSheetPage, lParam))
        {
            TRACE("lpfnAddPropSheetPageProc() failed!\n");
            DestroyPropertySheetPage(hPropSheetPage);
            return FALSE;
        }
    }

    TRACE("Done!\n");

    return TRUE;
}

/* EOF */
