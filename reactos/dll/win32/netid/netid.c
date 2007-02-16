/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/netid/netid.c
 * PURPOSE:     Network ID Page
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
 *
 */

#include <windows.h>
#include <lm.h>
#include <prsht.h>
#include "resource.h"

static HINSTANCE hDllInstance;

static INT_PTR CALLBACK
NetIDPageProc(IN HWND hwndDlg,
              IN UINT uMsg,
              IN WPARAM wParam,
              IN LPARAM lParam)
{
    INT_PTR Ret = 0;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Display computer name */
            LPWKSTA_INFO_101 wki = NULL;
            DWORD Size = MAX_COMPUTERNAME_LENGTH + 1;
            TCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
            if (GetComputerName(ComputerName,&Size))
            {
                SetDlgItemText(hwndDlg,
                               IDC_COMPUTERNAME,
                               ComputerName);
            }
            if (NetWkstaGetInfo(NULL,
                                101,
                                (LPBYTE*)&wki) == NERR_Success)
            {
                SetDlgItemText(hwndDlg,
                               IDC_WORKGROUPDOMAIN_NAME,
                               wki->wki101_langroup);
            }

            if (wki != NULL)
                NetApiBufferFree(wki);

            Ret = TRUE;
            break;
        }
    }

    return Ret;
}

HPROPSHEETPAGE WINAPI
CreateNetIDPropertyPage(VOID)
{
    PROPSHEETPAGE psp = {0};

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance= hDllInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGECOMPUTER);
    psp.pfnDlgProc = NetIDPageProc;

    return CreatePropertySheetPage(&psp);
}

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hDllInstance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}
