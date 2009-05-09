/*
 * ReactOS New devices installation
 * Copyright (C) 2005, 2008 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * PROJECT:         ReactOS Add hardware control panel
 * FILE:            dll/cpl/hdwwiz/hdwwiz.c
 * PURPOSE:         ReactOS Add hardware control panel
 * PROGRAMMER:      Hervé Poussineau (hpoussin@reactos.org)
 *                  Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "resource.h"
#include "hdwwiz.h"

/* GLOBALS ******************************************************************/

HINSTANCE hApplet = NULL;
HFONT hTitleFont;
SP_CLASSIMAGELIST_DATA ImageListData;
PWSTR pDeviceStatusText;
HANDLE hProcessHeap;
HDEVINFO hDevInfoTypes;

typedef BOOL (WINAPI *PINSTALL_NEW_DEVICE)(HWND, LPGUID, PDWORD);


/* STATIC FUNCTIONS *********************************************************/

static HFONT
CreateTitleFont(VOID)
{
    NONCLIENTMETRICS ncm;
    LOGFONT LogFont;
    HDC hdc;
    INT FontSize;
    HFONT hFont;

    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    LogFont = ncm.lfMessageFont;
    LogFont.lfWeight = FW_BOLD;
    wcscpy(LogFont.lfFaceName, L"MS Shell Dlg");

    hdc = GetDC(NULL);
    FontSize = 12;
    LogFont.lfHeight = 0 - GetDeviceCaps (hdc, LOGPIXELSY) * FontSize / 72;
    hFont = CreateFontIndirect(&LogFont);
    ReleaseDC(NULL, hdc);

    return hFont;
}

static INT_PTR CALLBACK
StartPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Set title font */
            SendDlgItemMessage(hwndDlg, IDC_FINISHTITLE, WM_SETFONT, (WPARAM)hTitleFont, (LPARAM)TRUE);
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
SearchPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* TODO: PnP devices search */
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
IsConnctedPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
        {
            if(HIWORD(wParam) == BN_CLICKED)
            {
                if ((SendDlgItemMessage(hwndDlg, IDC_CONNECTED, BM_GETCHECK, 0, 0) == BST_CHECKED) ||
                    (SendDlgItemMessage(hwndDlg, IDC_NOTCONNECTED, BM_GETCHECK, 0, 0) == BST_CHECKED))
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                }
                else
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                }
            }
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    if ((SendDlgItemMessage(hwndDlg, IDC_CONNECTED, BM_GETCHECK, 0, 0) == BST_CHECKED) ||
                        (SendDlgItemMessage(hwndDlg, IDC_NOTCONNECTED, BM_GETCHECK, 0, 0) == BST_CHECKED))
                    {
                        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    }
                    else
                    {
                        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                    }
                }
                break;

                case PSN_WIZNEXT:
                {
                    if (SendDlgItemMessage(hwndDlg, IDC_NOTCONNECTED, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_NOTCONNECTEDPAGE);
                    else
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_PROBELISTPAGE);

                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
FinishPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Set title font */
            SendDlgItemMessage(hwndDlg, IDC_FINISHTITLE, WM_SETFONT, (WPARAM)hTitleFont, (LPARAM)TRUE);
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    /* Only "Finish" button */
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
NotConnectedPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Set title font */
            SendDlgItemMessage(hwndDlg, IDC_FINISHTITLE, WM_SETFONT, (WPARAM)hTitleFont, (LPARAM)TRUE);
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH | PSWIZB_BACK);
                }
                break;

                case PSN_WIZBACK:
                {
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_ISCONNECTEDPAGE);
                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

static VOID
TrimGuidString(LPWSTR szString, LPWSTR szNewString)
{
    WCHAR szBuffer[39];
    INT Index;

    if (wcslen(szString) == 38)
    {
        if ((szString[0] == L'{') && (szString[37] == L'}'))
        {
            for (Index = 0; Index < wcslen(szString); Index++)
                szBuffer[Index] = szString[Index + 1];

            szBuffer[36] = L'\0';
            wcscpy(szNewString, szBuffer);
            return;
        }
    }
    wcscpy(szNewString, L"\0");
}

static VOID
InitProbeListPage(HWND hwndDlg)
{
    LV_COLUMN Column;
    LV_ITEM Item;
    WCHAR szBuffer[MAX_STR_SIZE], szGuid[MAX_STR_SIZE],
          szTrimGuid[MAX_STR_SIZE], szStatusText[MAX_STR_SIZE];
    HWND hList = GetDlgItem(hwndDlg, IDC_PROBELIST);
    PWSTR pstrStatusText;
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DevInfoData;
    ULONG ulStatus, ulProblemNumber;
    GUID ClassGuid;
    RECT Rect;
    DWORD Index;

    if (!hList) return;

    ZeroMemory(&Column, sizeof(LV_COLUMN));

    GetClientRect(hList, &Rect);

    Column.mask         = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    Column.fmt          = LVCFMT_LEFT;
    Column.iSubItem     = 0;
    Column.pszText      = NULL;
    Column.cx           = Rect.right - GetSystemMetrics(SM_CXVSCROLL);
    (VOID) ListView_InsertColumn(hList, 0, &Column);

    ZeroMemory(&Item, sizeof(LV_ITEM));

    LoadString(hApplet, IDS_ADDNEWDEVICE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR));

    Item.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    Item.pszText    = (LPWSTR) szBuffer;
    Item.iItem      = (INT) ListView_GetItemCount(hList);
    Item.iImage     = -1;
    (VOID) ListView_InsertItem(hList, &Item);

    hDevInfo = SetupDiGetClassDevsEx(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT, NULL, NULL, 0);

    if (hDevInfo == INVALID_HANDLE_VALUE) return;

    /* get the device image List */
    ImageListData.cbSize = sizeof(ImageListData);
    SetupDiGetClassImageList(&ImageListData);

    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (Index = 0; TRUE; Index++)
    {
        szBuffer[0] = L'\0';

        if (!SetupDiEnumDeviceInfo(hDevInfo, Index, &DevInfoData)) break;

        if (CM_Get_DevNode_Status_Ex(&ulStatus, &ulProblemNumber, DevInfoData.DevInst, 0, NULL) == CR_SUCCESS)
        {
            if (ulStatus & DN_NO_SHOW_IN_DM) continue;
        }

        /* get the device's friendly name */
        if (!SetupDiGetDeviceRegistryProperty(hDevInfo,
                                              &DevInfoData,
                                              SPDRP_FRIENDLYNAME,
                                              0,
                                              (BYTE*)szBuffer,
                                              MAX_STR_SIZE,
                                              NULL))
        {
            /* if the friendly name fails, try the description instead */
            SetupDiGetDeviceRegistryProperty(hDevInfo,
                                             &DevInfoData,
                                             SPDRP_DEVICEDESC,
                                             0,
                                             (BYTE*)szBuffer,
                                             MAX_STR_SIZE,
                                             NULL);
        }

        SetupDiGetDeviceRegistryProperty(hDevInfo,
                                         &DevInfoData,
                                         SPDRP_CLASSGUID,
                                         0,
                                         (BYTE*)szGuid,
                                         MAX_STR_SIZE,
                                         NULL);

        TrimGuidString(szGuid, szTrimGuid);
        UuidFromStringW(szTrimGuid, &ClassGuid);

        SetupDiGetClassImageIndex(&ImageListData,
                                  &ClassGuid,
                                  &Item.iImage);

        DeviceProblemTextW(NULL,
                           DevInfoData.DevInst,
                           ulProblemNumber,
                           szStatusText,
                           sizeof(szStatusText) / sizeof(WCHAR));

        pstrStatusText = (PWSTR)HeapAlloc(hProcessHeap, 0, sizeof(szStatusText));
        lstrcpy(pstrStatusText, szStatusText);

        if (szBuffer[0] != L'\0')
        {
            /* Set device name */
            Item.pszText = (LPWSTR) szBuffer;
            Item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
            Item.lParam = (LPARAM) pstrStatusText;
            Item.iItem = (INT) ListView_GetItemCount(hList);
            (VOID) ListView_InsertItem(hList, &Item);
        }

        DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    }

    (VOID) ListView_SetImageList(hList, ImageListData.ImageList, LVSIL_SMALL);
    (VOID) ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);
    SetupDiDestroyDeviceInfoList(hDevInfo);
}

static INT_PTR CALLBACK
ProbeListPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT Index;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pDeviceStatusText = (PWSTR)HeapAlloc(hProcessHeap, 0, MAX_STR_SIZE);
            InitProbeListPage(hwndDlg);
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                }
                break;

                case PSN_WIZNEXT:
                {
                    Index = (INT) SendMessage(GetDlgItem(hwndDlg, IDC_PROBELIST), LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                    if (Index == -1) Index = 0;

                    if (Index == 0)
                    {
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_SELECTWAYPAGE);
                    }
                    else
                    {
                        LVITEM Item;
                        PWSTR pts;

                        ZeroMemory(&Item, sizeof(LV_ITEM));
                        Item.mask = LVIF_PARAM;
                        Item.iItem = Index;
                        (VOID) ListView_GetItem(GetDlgItem(hwndDlg, IDC_PROBELIST), &Item);
                        pts = (PWSTR) Item.lParam;
                        wcscpy(pDeviceStatusText, pts);

                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_HWSTATUSPAGE);
                    }
                    return TRUE;
                }
            }
        }
        break;

        case WM_DESTROY:
        {
            INT Index;
            LVITEM Item;

            for (Index = ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_PROBELIST)); --Index > 0;)
            {
                ZeroMemory(&Item, sizeof(LV_ITEM));
                Item.mask = LVIF_PARAM;
                Item.iItem = Index;
                (VOID) ListView_GetItem(GetDlgItem(hwndDlg, IDC_PROBELIST), &Item);
                HeapFree(hProcessHeap, 0, (LPVOID) Item.lParam);
            }
            HeapFree(hProcessHeap, 0, (LPVOID) pDeviceStatusText);
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
SelectWayPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    if (SendDlgItemMessage(hwndDlg, IDC_AUTOINSTALL, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        SendDlgItemMessage(hwndDlg, IDC_MANUALLYINST, BM_SETCHECK, 0, 0);
                    else
                    {
                        SendDlgItemMessage(hwndDlg, IDC_AUTOINSTALL, BM_SETCHECK, 1, 1);
                        SendDlgItemMessage(hwndDlg, IDC_MANUALLYINST, BM_SETCHECK, 0, 0);
                    }
                }
                break;

                case PSN_WIZNEXT:
                {
                    if (SendDlgItemMessage(hwndDlg, IDC_AUTOINSTALL, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_PROGRESSPAGE);
                    else
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_HWTYPESPAGE);

                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
DevStatusPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Set title font */
            SendDlgItemMessage(hwndDlg, IDC_FINISHTITLE, WM_SETFONT, (WPARAM)hTitleFont, (LPARAM)TRUE);
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    /* Set status text */
                    SetWindowText(GetDlgItem(hwndDlg, IDC_HWSTATUSEDIT), pDeviceStatusText);

                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH | PSWIZB_BACK);
                }
                break;

                case PSN_WIZBACK:
                {
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_PROBELISTPAGE);
                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

static INT
EnumDeviceClasses(INT ClassIndex,
                  LPWSTR DevClassName,
                  LPWSTR DevClassDesc,
                  BOOL *DevPresent,
                  INT *ClassImage)
{
    GUID ClassGuid;
    HKEY KeyClass;
    WCHAR ClassName[MAX_STR_SIZE];
    DWORD RequiredSize = MAX_STR_SIZE;
    UINT Ret;

    *DevPresent = FALSE;
    *DevClassName = L'\0';

    Ret = CM_Enumerate_Classes(ClassIndex,
                               &ClassGuid,
                               0);
    if (Ret != CR_SUCCESS)
    {
        /* all classes enumerated */
        if(Ret == CR_NO_SUCH_VALUE)
        {
            hDevInfoTypes = NULL;
            return -1;
        }

        if (Ret == CR_INVALID_DATA)
        {
            ; /*FIXME: what should we do here? */
        }

        /* handle other errors... */
    }

    if (SetupDiClassNameFromGuid(&ClassGuid,
                                 ClassName,
                                 RequiredSize,
                                 &RequiredSize))
    {
        lstrcpy(DevClassName, ClassName);
    }

    if (!SetupDiGetClassImageIndex(&ImageListData,
                                   &ClassGuid,
                                   ClassImage))
    {
        /* FIXME: can we do this?
         * Set the blank icon: IDI_SETUPAPI_BLANK = 41
         * it'll be image 24 in the imagelist */
        *ClassImage = 24;
    }

    /* Get device info for all devices of a particular class */
    hDevInfoTypes = SetupDiGetClassDevs(&ClassGuid,
                                   NULL,
                                   NULL,
                                   DIGCF_PRESENT);
    if (hDevInfoTypes == INVALID_HANDLE_VALUE)
    {
        hDevInfoTypes = NULL;
        return 0;
    }

    KeyClass = SetupDiOpenClassRegKeyEx(&ClassGuid,
                                        MAXIMUM_ALLOWED,
                                        DIOCR_INSTALLER,
                                        NULL,
                                        0);
    if (KeyClass != INVALID_HANDLE_VALUE)
    {

        LONG dwSize = MAX_STR_SIZE;

        if (RegQueryValue(KeyClass,
                          NULL,
                          DevClassDesc,
                          &dwSize) != ERROR_SUCCESS)
        {
            *DevClassDesc = L'\0';
        }
    }
    else
    {
        return -3;
    }

    *DevPresent = TRUE;

    RegCloseKey(KeyClass);

    return 0;
}

static VOID
InitHardWareTypesPage(HWND hwndDlg)
{
    HWND hList = GetDlgItem(hwndDlg, IDC_HWTYPESLIST);
    WCHAR DevName[MAX_STR_SIZE];
    WCHAR DevDesc[MAX_STR_SIZE];
    BOOL DevExist = FALSE;
    INT ClassRet, DevImage, Index = 0;
    LV_COLUMN Column;
    LV_ITEM Item;
    RECT Rect;

    if (!hList) return;

    ZeroMemory(&Column, sizeof(LV_COLUMN));

    GetClientRect(hList, &Rect);

    Column.mask         = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    Column.fmt          = LVCFMT_LEFT;
    Column.iSubItem     = 0;
    Column.pszText      = NULL;
    Column.cx           = Rect.right - GetSystemMetrics(SM_CXVSCROLL);
    (VOID) ListView_InsertColumn(hList, 0, &Column);

    ZeroMemory(&Item, sizeof(LV_ITEM));

    do
    {
        ClassRet = EnumDeviceClasses(Index,
                                     DevName,
                                     DevDesc,
                                     &DevExist,
                                     &DevImage);

        if ((ClassRet != -1) && (DevExist))
        {
            Item.mask   = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
            Item.iItem  = Index;
            Item.iImage = DevImage;

            if (DevDesc[0] != L'\0')
                Item.pszText = (LPWSTR) DevDesc;
            else
                Item.pszText = (LPWSTR) DevName;

            (VOID) ListView_InsertItem(hList, &Item);

            /* kill InfoList initialized in EnumDeviceClasses */
            if (hDevInfoTypes)
            {
                SetupDiDestroyDeviceInfoList(hDevInfoTypes);
                hDevInfoTypes = NULL;
            }
        }
        Index++;
    }
    while (ClassRet != -1);

    (VOID) ListView_SetImageList(hList, ImageListData.ImageList, LVSIL_SMALL);
}

static INT_PTR CALLBACK
HdTypesPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            InitHardWareTypesPage(hwndDlg);
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                }
                break;

                case PSN_WIZBACK:
                {
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_SELECTWAYPAGE);
                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
ProgressPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                }
                break;

                case PSN_WIZBACK:
                {
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_SELECTWAYPAGE);
                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

static VOID
HardwareWizardInit(HWND hwnd)
{
    HPROPSHEETPAGE ahpsp[10];
    PROPSHEETPAGE psp = {0};
    PROPSHEETHEADER psh;
    UINT nPages = 0;

    /* Create the Start page, until setup is working */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = StartPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_STARTPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create search page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_SEARCHTITLE);
    psp.pszHeaderSubTitle = NULL;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = SearchPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SEARCHPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create is connected page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ISCONNECTED);
    psp.pszHeaderSubTitle = NULL;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = IsConnctedPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_ISCONNECTEDPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create probe list page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_PROBELISTTITLE);
    psp.pszHeaderSubTitle = NULL;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = ProbeListPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROBELISTPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create select search way page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_SELECTWAYTITLE);
    psp.pszHeaderSubTitle = NULL;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = SelectWayPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SELECTWAYPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create device status page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = DevStatusPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_HWSTATUSPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create hardware types page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_HDTYPESTITLE);
    psp.pszHeaderSubTitle = NULL;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = HdTypesPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_HWTYPESPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create progress page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_SEARCHTITLE);
    psp.pszHeaderSubTitle = NULL;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = ProgressPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROGRESSPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create finish page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = FinishPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_FINISHPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create not connected page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = NotConnectedPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_NOTCONNECTEDPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the property sheet */
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.hInstance = hApplet;
    psh.hwndParent = hwnd;
    psh.nPages = nPages;
    psh.nStartPage = 0;
    psh.phpage = ahpsp;
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

    /* Create title font */
    hTitleFont = CreateTitleFont();

    /* Display the wizard */
    PropertySheet(&psh);

    DeleteObject(hTitleFont);
}

/* FUNCTIONS ****************************************************************/

BOOL WINAPI
InstallNewDevice(HWND hwndParent, LPGUID ClassGuid, PDWORD pReboot)
{
    return FALSE;
}

VOID WINAPI
AddHardwareWizard(HWND hwnd, LPWSTR lpName)
{
    if (lpName != NULL)
    {
        DPRINT1("No support of remote installation yet!\n");
        return;
    }

    HardwareWizardInit(hwnd);
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCpl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = 0;
                CPlInfo->idIcon = IDI_CPLICON;
                CPlInfo->idName = IDS_CPLNAME;
                CPlInfo->idInfo = IDS_CPLDESCRIPTION;
            }
            break;

        case CPL_DBLCLK:
            AddHardwareWizard(hwndCpl, NULL);
            break;
    }

    return FALSE;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hApplet = hinstDLL;
            hProcessHeap = GetProcessHeap();
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}
