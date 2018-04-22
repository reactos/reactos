/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/syssetup/proppage.c
 * PURPOSE:     Property page providers
 * PROGRAMMERS: Copyright 2018 Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

DWORD MouseSampleRates[] = {20, 40, 60, 80, 100, 200};


/*
 * @implemented
 */
BOOL
WINAPI
CdromPropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT("CdromPropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DiskPropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT("DiskPropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
EisaUpHalPropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT("EisaUpHalPropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
LegacyDriverPropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT1("LegacyDriverPropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    UNIMPLEMENTED;
    return FALSE;
}


static
VOID
MouseOnDialogInit(
    HWND hwndDlg,
    LPARAM lParam)
{
    WCHAR szBuffer[64];
    UINT i;

    /* Add the sample rates */
    for (i = 0; i < ARRAYSIZE(MouseSampleRates); i++)
    {
        wsprintf(szBuffer, L"%lu", MouseSampleRates[i]);
        SendDlgItemMessageW(hwndDlg,
                            IDC_PS2MOUSESAMPLERATE,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)szBuffer);
    }

    /* Add the detection options */
    for (i = IDS_DETECTIONDISABLED; i <= IDS_ASSUMEPRESENT; i++)
    {
        LoadStringW(hDllInstance, i, szBuffer, ARRAYSIZE(szBuffer));
        SendDlgItemMessageW(hwndDlg,
                            IDC_PS2MOUSEWHEEL,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)szBuffer);
    }

    /* Set the input buffer length range: 100-300 */
    SendDlgItemMessageW(hwndDlg,
                        IDC_PS2MOUSEINPUTUPDN,
                        UDM_SETRANGE32,
                        100,
                        300);

    SendDlgItemMessageW(hwndDlg,
                        IDC_PS2MOUSEINPUTUPDN,
                        UDM_SETPOS32,
                        0,
                        100);
}


static
VOID
MouseOnCommand(
    HWND hwndDlg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
        case IDC_PS2MOUSESAMPLERATE:
        case IDC_PS2MOUSEWHEEL:
        case IDC_PS2MOUSEINPUTLEN:
        case IDC_PS2MOUSEFASTINIT:
            if (HIWORD(wParam) == CBN_SELCHANGE ||
                HIWORD(wParam) == CBN_EDITCHANGE)
            {
                /* Enable the Apply button */
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;

        case IDC_PS2MOUSEDEFAULTS:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                /* Sample rate: 100 */
                SendDlgItemMessageW(hwndDlg,
                                    IDC_PS2MOUSESAMPLERATE,
                                    CB_SETCURSEL,
                                    4,
                                    0);

                /* Wheel detection: Assume wheel present */
                SendDlgItemMessageW(hwndDlg,
                                    IDC_PS2MOUSEWHEEL,
                                    CB_SETCURSEL,
                                    2,
                                    0);

                /* Input buffer length: 100 packets */
                SendDlgItemMessageW(hwndDlg,
                                    IDC_PS2MOUSEINPUTUPDN,
                                    UDM_SETPOS32,
                                    0,
                                    100);

                /* Fast Initialization: Checked */
                SendDlgItemMessage(hwndDlg,
                                   IDC_PS2MOUSEFASTINIT,
                                   BM_SETCHECK,
                                   (WPARAM)BST_CHECKED,
                                   (LPARAM)0);

                /* Enable the Apply button */
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;

        default:
            break;
    }
}


static
INT_PTR
CALLBACK
MouseDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    DPRINT("MouseDlgProc\n");

    switch (uMsg)
    {
        case WM_INITDIALOG:
            MouseOnDialogInit(hwndDlg, lParam);
            return TRUE;

        case WM_COMMAND:
            MouseOnCommand(hwndDlg, wParam, lParam);
            break;

    }

    return FALSE;
}



/*
 * @implemented
 */
BOOL
WINAPI
PS2MousePropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    PROPSHEETPAGEW PropSheetPage;
    HPROPSHEETPAGE hPropSheetPage;

    DPRINT("PS2MousePropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);

    if (lpPropSheetPageRequest->PageRequested != SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
        return FALSE;

    PropSheetPage.dwSize = sizeof(PROPSHEETPAGEW);
    PropSheetPage.dwFlags = 0;
    PropSheetPage.hInstance = hDllInstance;
    PropSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PS2MOUSEPROPERTIES);
    PropSheetPage.pfnDlgProc = MouseDlgProc;
    PropSheetPage.lParam = 0;
    PropSheetPage.pfnCallback = NULL;

    hPropSheetPage = CreatePropertySheetPageW(&PropSheetPage);
    if (hPropSheetPage == NULL)
    {
        DPRINT1("CreatePropertySheetPageW() failed!\n");
        return FALSE;
    }

    if (!(*lpfnAddPropSheetPageProc)(hPropSheetPage, lParam))
    {
        DPRINT1("lpfnAddPropSheetPageProc() failed!\n");
        DestroyPropertySheetPage(hPropSheetPage);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
TapePropPageProvider(
    _In_ PSP_PROPSHEETPAGE_REQUEST lpPropSheetPageRequest,
    _In_ LPFNADDPROPSHEETPAGE lpfnAddPropSheetPageProc,
    _In_ LPARAM lParam)
{
    DPRINT("TapePropPageProvider(%p %p %lx)\n",
           lpPropSheetPageRequest, lpfnAddPropSheetPageProc, lParam);
    return FALSE;
}

/* EOF */
