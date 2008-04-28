/*
 * PROJECT:     ReactX Diagnosis Application
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/dxdiag/display.c
 * PURPOSE:     ReactX diagnosis display page
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald
 *
 */

#include "precomp.h"
#include <initguid.h>
#include <devguid.h>

static
BOOL
InitializeDialog(HWND hwndDlg)
{
    HDEVINFO hInfo;
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    SP_DEVINFO_DATA InfoData;
    DWORD dwIndex = 0;
    WCHAR szText[100];

    hInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, NULL, hwndDlg, DIGCF_PRESENT|DIGCF_PROFILE);
    if (hInfo == INVALID_HANDLE_VALUE)
        return FALSE;

    do
    {
        ZeroMemory(&InterfaceData, sizeof(InterfaceData));
        InterfaceData.cbSize = sizeof(InterfaceData);
        InfoData.cbSize = sizeof(InfoData);

        if (SetupDiEnumDeviceInfo(hInfo, dwIndex, &InfoData))
        {
            /* set device name */
            if (SetupDiGetDeviceRegistryPropertyW(hInfo, &InfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)szText, sizeof(szText), NULL))
                SendDlgItemMessageW(hwndDlg, IDC_STATIC_ADAPTER_ID, WM_SETTEXT, 0, (LPARAM)szText);

        }
        /* FIXME
         * only enumerate the first display adapter 
         */
         break;

        if (GetLastError() == ERROR_NO_MORE_ITEMS)
            break;

        dwIndex++;
    }while(TRUE);


    SetupDiDestroyDeviceInfoList(hInfo);
    return TRUE;
}

INT_PTR CALLBACK
DisplayPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    PDXDIAG_CONTEXT pContext = (PDXDIAG_CONTEXT)GetWindowLongPtr(hDlg, DWLP_USER);
    switch (message) 
    {
        case WM_INITDIALOG:
        {
            pContext = (PDXDIAG_CONTEXT) lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pContext);
            SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            InitializeDialog(hDlg);
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_BUTTON_TESTDD:
                    /* FIXME log result errors */
                    ShowWindow(pContext->hMainDialog, SW_HIDE);
                    StartDDTest(pContext->hMainDialog, hInst, IDS_DDPRIMARY_DESCRIPTION, IDS_DDPRIMARY_RESULT, 1);
                    StartDDTest(pContext->hMainDialog, hInst, IDS_DDOFFSCREEN_DESCRIPTION, IDS_DDOFFSCREEN_RESULT, 2);
                    StartDDTest(pContext->hMainDialog, hInst, IDS_DDFULLSCREEN_DESCRIPTION, IDS_DDFULLSCREEN_RESULT, 3);
                    ShowWindow(pContext->hMainDialog, SW_SHOW);
                    /* FIXME resize window */
                    break;
            }
            break;
        }
    }

    return FALSE;
}
