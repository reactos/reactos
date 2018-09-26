/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Network property page provider
 * COPYRIGHT:   Copyright 2018 Eric Kohl (eric.kohl@reactos.org)
 */

#include "precomp.h"

static
INT_PTR
CALLBACK
NetPropertyPageDlgProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            ERR("NetPropertyPageDlgProc: WM_INITDIALOG\n");
            return TRUE;
//            return OnInitDialog(hwnd, wParam, lParam);

//        case WM_DESTROY:
//            OnDestroy(hwnd);
//            break;

        default:
            break;
    }

    return FALSE;
}


BOOL
WINAPI
NetPropPageProvider(
    PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    LPARAM lParam)
{
    PROPSHEETPAGEW PropSheetPage;
    HPROPSHEETPAGE hPropSheetPage;

    ERR("NetPropPageProvider(%p %p %lx)\n",
          lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);

    if (lpPropSheetPageRequest->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
    {
        ERR("SPPSR_ENUM_ADV_DEVICE_PROPERTIES\n");

        PropSheetPage.dwSize = sizeof(PROPSHEETPAGEW);
        PropSheetPage.dwFlags = 0;
        PropSheetPage.hInstance = netcfgx_hInstance;
        PropSheetPage.u.pszTemplate = MAKEINTRESOURCE(IDD_NET_PROPERTY_DLG);
        PropSheetPage.pfnDlgProc = NetPropertyPageDlgProc;
        PropSheetPage.lParam = 0;
        PropSheetPage.pfnCallback = NULL;

        hPropSheetPage = CreatePropertySheetPageW(&PropSheetPage);
        if (hPropSheetPage == NULL)
        {
            ERR("CreatePropertySheetPageW() failed!\n");
            return FALSE;
        }

        if (!(*lpfnAddPropSheetPageProc)(hPropSheetPage, lParam))
        {
            ERR("lpfnAddPropSheetPageProc() failed!\n");
            DestroyPropertySheetPage(hPropSheetPage);
            return FALSE;
        }
    }

    ERR("Done!\n");

    return TRUE;
}

/* EOF */
