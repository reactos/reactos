/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ReactOS Networking Configuration
 * COPYRIGHT:   Copyright 2016 Eric Kohl
 */

#include "precomp.h"

#include <syssetup/syssetup.h>


#define NETWORK_SETUP_MAGIC 0x50515253

typedef struct _NETWORKSETUPDATA
{
    DWORD dwMagic;
    BOOL bTypicalNetworkSetup;


    PSETUPDATA pSetupData;
} NETWORKSETUPDATA, *PNETWORKSETUPDATA;


extern "C"
{

static
VOID
SetBoldText(
    HWND hwndDlg,
    INT control,
    PSETUPDATA pSetupData)
{
    SendDlgItemMessageW(hwndDlg, control, WM_SETFONT, (WPARAM)pSetupData->hBoldFont, MAKELPARAM(TRUE, 0));
}

static
INT_PTR
CALLBACK
NetworkSettingsPageDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PNETWORKSETUPDATA pNetworkSetupData;
    PSETUPDATA pSetupData;
    LPNMHDR lpnm;

    /* Retrieve pointer to the global setup data */
    pNetworkSetupData = (PNETWORKSETUPDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    if ((pNetworkSetupData != NULL) &&
        (pNetworkSetupData->dwMagic == NETWORK_SETUP_MAGIC))
        pSetupData = pNetworkSetupData->pSetupData;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pNetworkSetupData = (PNETWORKSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pNetworkSetupData);
            pSetupData = pNetworkSetupData->pSetupData;

            /* Set the fonts of both the options to bold */
            SetBoldText(hwndDlg, IDC_NETWORK_TYPICAL, pSetupData);
            SetBoldText(hwndDlg, IDC_NETWORK_CUSTOM, pSetupData);

            /* Set the typical settings option as the default */
            SendDlgItemMessage(hwndDlg, IDC_NETWORK_TYPICAL, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

            if (pSetupData->UnattendSetup)
            {
                //...
            }
            break;

        case WM_DESTROY:
            /* ATTENTION: Free pNetworkSetupData only in one of the page functions!!! */
            //...
            break;

        case WM_NOTIFY:
            lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    /* Enable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (pSetupData->UnattendSetup)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_NETWORKCOMPONENTPAGE);
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    pNetworkSetupData->bTypicalNetworkSetup = FALSE;

                    /* If the Typical Settings button is chosen, then skip to the Domain Page */
                    if (IsDlgButtonChecked(hwndDlg, IDC_NETWORK_TYPICAL) == BST_CHECKED)
                    {
                        pNetworkSetupData->bTypicalNetworkSetup = TRUE;
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_NETWORKDOMAINPAGE);
                        return TRUE;
                    }
                    break;

                case PSN_WIZBACK:
                    pSetupData->UnattendSetup = FALSE;
                    break;
            }
            break;

        default:
            break;
    }

    return FALSE;
}

static
INT_PTR
CALLBACK
NetworkComponentPageDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PNETWORKSETUPDATA pNetworkSetupData;
    PSETUPDATA pSetupData;
    LPNMHDR lpnm;

    /* Retrieve pointer to the global setup data */
    pNetworkSetupData = (PNETWORKSETUPDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    if ((pNetworkSetupData != NULL) &&
        (pNetworkSetupData->dwMagic == NETWORK_SETUP_MAGIC))
        pSetupData = pNetworkSetupData->pSetupData;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pNetworkSetupData = (PNETWORKSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pNetworkSetupData);
            pSetupData = pNetworkSetupData->pSetupData;

            SetBoldText(hwndDlg, IDC_NETWORK_DEVICE, pSetupData);

            if (pSetupData->UnattendSetup)
            {
                //...
            }
            break;

        case WM_DESTROY:
            /* ATTENTION: Free pNetworkSetupData only in one of the page functions!!! */
            //...
            break;

        case WM_NOTIFY:
            lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    /* Enable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (pSetupData->UnattendSetup)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_NETWORKDOMAINPAGE);
                        return TRUE;
                    }
                    break;

                case PSN_WIZBACK:
                    pSetupData->UnattendSetup = FALSE;
                    break;
            }
            break;

        default:
            break;
    }

    return FALSE;
}

static
INT_PTR
CALLBACK
NetworkDomainPageDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PNETWORKSETUPDATA pNetworkSetupData;
    PSETUPDATA pSetupData;
    WCHAR DomainName[51];
    WCHAR Title[64];
    WCHAR ErrorName[256];
    LPNMHDR lpnm;

    /* Retrieve pointer to the global setup data */
    pNetworkSetupData = (PNETWORKSETUPDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    if ((pNetworkSetupData != NULL) &&
        (pNetworkSetupData->dwMagic == NETWORK_SETUP_MAGIC))
        pSetupData = pNetworkSetupData->pSetupData;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* Save pointer to the global setup data */
            pNetworkSetupData = (PNETWORKSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)pNetworkSetupData);
            pSetupData = pNetworkSetupData->pSetupData;

            /* Set the workgroup option as the default */
            SendDlgItemMessage(hwndDlg, IDC_SELECT_WORKGROUP, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

            wcscpy(DomainName, L"WORKGROUP");

            /* Display current computer name */
            SetDlgItemTextW(hwndDlg, IDC_DOMAIN_NAME, DomainName);

            /* Set focus to owner name */
            SetFocus(GetDlgItem(hwndDlg, IDC_DOMAIN_NAME));

            if (pSetupData->UnattendSetup)
            {
                //...
            }
            break;

        case WM_DESTROY:
            //...
            /* ATTENTION: Free pNetworkSetupData only in one of the page functions!!! */
            if (pNetworkSetupData != NULL)
            {
                if ((!IsBadReadPtr(pNetworkSetupData, sizeof(NETWORKSETUPDATA))) &&
                    (pNetworkSetupData->dwMagic == NETWORK_SETUP_MAGIC))
                    HeapFree(GetProcessHeap(), 0, pNetworkSetupData);

                SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)NULL);
            }
            break;

        case WM_NOTIFY:
            lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    /* Enable the Back and Next buttons */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    if (pSetupData->UnattendSetup)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, pSetupData->uPostNetworkWizardPage);
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    DomainName[0] = 0;
                    if (GetDlgItemTextW(hwndDlg, IDC_DOMAIN_NAME, DomainName, 50) == 0)
                    {
                        if (0 == LoadStringW(netshell_hInstance, IDS_REACTOS_SETUP, Title, sizeof(Title) / sizeof(Title[0])))
                        {
                            wcscpy(Title, L"ReactOS Setup");
                        }
                        if (0 == LoadStringW(netshell_hInstance, IDS_WZD_DOMAIN_NAME, ErrorName, sizeof(ErrorName) / sizeof(ErrorName[0])))
                        {
                            wcscpy(ErrorName, L"Setup cannot continue until you\nenter the name of your domain/workgroup.");
                        }
                        MessageBoxW(hwndDlg, ErrorName, Title, MB_ICONERROR | MB_OK);

                        SetFocus(GetDlgItem(hwndDlg, IDC_DOMAIN_NAME));
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

                        //TODO: Implement setting the Domain/Workgroup

                        return TRUE;
                    }
                    break;

                case PSN_WIZBACK:
                    pSetupData->UnattendSetup = FALSE;

                    /* If the Typical setup chosen, then skip back to the Settings Page */
                    if (pNetworkSetupData->bTypicalNetworkSetup == TRUE)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_NETWORKSETTINGSPAGE);
                        return TRUE;
                    }
                    break;
            }
            break;

        default:
            break;
    }

    return FALSE;
}


DWORD
WINAPI
NetSetupRequestWizardPages(
    PDWORD pPageCount,
    HPROPSHEETPAGE *pPages,
    PSETUPDATA pSetupData)
{
    PNETWORKSETUPDATA pNetworkSetupData;
    PROPSHEETPAGE psp = {0};
    DWORD dwPageCount = 3;
    INT nPage = 0;

    if (pPageCount == NULL)
        return ERROR_INVALID_PARAMETER;

    if (pPages == NULL)
    {
        *pPageCount = dwPageCount;
        return ERROR_SUCCESS;
    }

    if (*pPageCount < dwPageCount)
        return ERROR_BUFFER_OVERFLOW;

    pSetupData->uFirstNetworkWizardPage = IDD_NETWORKSETTINGSPAGE;

    pNetworkSetupData = (PNETWORKSETUPDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NETWORKSETUPDATA));
    if (pNetworkSetupData == NULL)
        return ERROR_OUTOFMEMORY;

    pNetworkSetupData->dwMagic = NETWORK_SETUP_MAGIC;
    pNetworkSetupData->pSetupData = pSetupData;


    /* Create the Network Settings page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.hInstance = netshell_hInstance;
    psp.lParam = (LPARAM)pNetworkSetupData;

    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_NETWORKSETTINGSTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_NETWORKSETTINGSSUBTITLE);
    psp.pfnDlgProc = NetworkSettingsPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_NETWORKSETTINGSPAGE);
    pPages[nPage++] = CreatePropertySheetPage(&psp);

    /* Create the Network Components page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_NETWORKCOMPONENTTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_NETWORKCOMPONENTSUBTITLE);
    psp.pfnDlgProc = NetworkComponentPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_NETWORKCOMPONENTPAGE);
    pPages[nPage++] = CreatePropertySheetPage(&psp);

    /* Create the Domain/Workgroup page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_NETWORKDOMAINTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_NETWORKDOMAINSUBTITLE);
    psp.pfnDlgProc = NetworkDomainPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_NETWORKDOMAINPAGE);
    pPages[nPage++] = CreatePropertySheetPage(&psp);

    *pPageCount = dwPageCount;

    return ERROR_SUCCESS;
}

} // extern "C"
