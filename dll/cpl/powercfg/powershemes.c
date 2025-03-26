/*
 * PROJECT:         ReactOS Power Configuration Applet
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/powercfg/powershemes.c
 * PURPOSE:         powerschemes tab of applet
 * PROGRAMMERS:     Alexander Wurzinger (Lohnegrim at gmx dot net)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  Martin Rottensteiner
 *                  Dmitry Chapyshev (lentind@yandex.ru)
 */

#include "powercfg.h"
#include <debug.h>

typedef struct _POWER_SCHEME
{
    LIST_ENTRY ListEntry;
    UINT uId;
    LPTSTR pszName;
    LPTSTR pszDescription;
    POWER_POLICY PowerPolicy;
} POWER_SCHEME, *PPOWER_SCHEME;


typedef struct _POWER_SCHEMES_PAGE_DATA
{
    LIST_ENTRY PowerSchemesList;
    PPOWER_SCHEME pActivePowerScheme;
    PPOWER_SCHEME pSelectedPowerScheme;
} POWER_SCHEMES_PAGE_DATA, *PPOWER_SCHEMES_PAGE_DATA;


typedef struct _SAVE_POWER_SCHEME_DATA
{
    PPOWER_SCHEMES_PAGE_DATA pPageData;
    PPOWER_SCHEME pNewScheme;
    HWND hwndPage;
} SAVE_POWER_SCHEME_DATA, *PSAVE_POWER_SCHEME_DATA;


UINT Sec[]=
{
    60,
    120,
    180,
    300,
    600,
    900,
    1200,
    1500,
    1800,
    2700,
    3600,
    7200,
    10800,
    14400,
    18000,
    0
};


static
PPOWER_SCHEME
AddPowerScheme(
    PPOWER_SCHEMES_PAGE_DATA pPageData,
    UINT uId,
    DWORD dwName,
    LPTSTR pszName,
    DWORD dwDescription,
    LPWSTR pszDescription,
    PPOWER_POLICY pp)
{
    PPOWER_SCHEME pScheme;
    BOOL bResult = FALSE;

    pScheme = HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        sizeof(POWER_SCHEME));
    if (pScheme == NULL)
        return NULL;

    pScheme->uId = uId;
    CopyMemory(&pScheme->PowerPolicy, pp, sizeof(POWER_POLICY));

    if (dwName != 0)
    {
        pScheme->pszName = HeapAlloc(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     dwName);
        if (pScheme->pszName == NULL)
            goto done;

        _tcscpy(pScheme->pszName, pszName);
    }

    if (dwDescription != 0)
    {
        pScheme->pszDescription = HeapAlloc(GetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            dwDescription);
        if (pScheme->pszDescription == NULL)
            goto done;

        _tcscpy(pScheme->pszDescription, pszDescription);
    }

    InsertTailList(&pPageData->PowerSchemesList, &pScheme->ListEntry);
    bResult = TRUE;

done:
    if (bResult == FALSE)
    {
        if (pScheme->pszName)
            HeapFree(GetProcessHeap(), 0, pScheme->pszName);

        if (pScheme->pszDescription)
            HeapFree(GetProcessHeap(), 0, pScheme->pszDescription);

        HeapFree(GetProcessHeap(), 0, pScheme);
        pScheme = NULL;
    }

    return pScheme;
}


static
VOID
DeletePowerScheme(
    PPOWER_SCHEME pScheme)
{
    RemoveEntryList(&pScheme->ListEntry);

    if (pScheme->pszName)
        HeapFree(GetProcessHeap(), 0, pScheme->pszName);

    if (pScheme->pszDescription)
        HeapFree(GetProcessHeap(), 0, pScheme->pszDescription);

    HeapFree(GetProcessHeap(), 0, pScheme);
}


static
BOOLEAN
CALLBACK
EnumPowerSchemeCallback(
    UINT uiIndex,
    DWORD dwName,
    LPTSTR pszName,
    DWORD dwDesc,
    LPWSTR pszDesc,
    PPOWER_POLICY pp,
    LPARAM lParam)
{
    if (ValidatePowerPolicies(0, pp))
    {
        AddPowerScheme((PPOWER_SCHEMES_PAGE_DATA)lParam,
                       uiIndex,
                       dwName,
                       pszName,
                       dwDesc,
                       pszDesc,
                       pp);
    }

    return TRUE;
}

static
VOID
BuildSchemesList(
    PPOWER_SCHEMES_PAGE_DATA pPageData)
{
    InitializeListHead(&pPageData->PowerSchemesList);

    EnumPwrSchemes(EnumPowerSchemeCallback, (LPARAM)pPageData);
}


static
VOID
DestroySchemesList(
    PPOWER_SCHEMES_PAGE_DATA pPageData)
{
    PLIST_ENTRY ListEntry;
    PPOWER_SCHEME pScheme;

    for (;;)
    {
        ListEntry = pPageData->PowerSchemesList.Flink;
        if (ListEntry == &pPageData->PowerSchemesList)
            break;

        pScheme = CONTAINING_RECORD(ListEntry, POWER_SCHEME, ListEntry);
        DeletePowerScheme(pScheme);
    }

    pPageData->pActivePowerScheme = NULL;
    pPageData->pSelectedPowerScheme = NULL;
}


BOOLEAN
Pos_InitData(
    HWND hwndDlg)
{
    SYSTEM_POWER_CAPABILITIES spc;

    if (!GetPwrCapabilities(&spc))
    {
        return FALSE;
    }

    ShowWindow(GetDlgItem(hwndDlg, IDC_STANDBY),
               (spc.SystemS1 || spc.SystemS2 || spc.SystemS3) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_STANDBYACLIST),
               (spc.SystemS1 || spc.SystemS2 || spc.SystemS3) ? SW_SHOW : SW_HIDE);
    if (spc.SystemBatteriesPresent)
        ShowWindow(GetDlgItem(hwndDlg, IDC_STANDBYDCLIST),
                   (spc.SystemS1 || spc.SystemS2 || spc.SystemS3) ? SW_SHOW : SW_HIDE);

    ShowWindow(GetDlgItem(hwndDlg, IDC_HIBERNATE),
               (spc.HiberFilePresent) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEACLIST),
               (spc.HiberFilePresent) ? SW_SHOW : SW_HIDE);
    if (spc.SystemBatteriesPresent)
        ShowWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEDCLIST),
                   (spc.HiberFilePresent) ? SW_SHOW : SW_HIDE);

    return TRUE;
}


static
VOID
LoadConfig(
    HWND hwndDlg,
    PPOWER_SCHEMES_PAGE_DATA pPageData,
    PPOWER_SCHEME pScheme)
{
    INT i = 0, iCurSel = 0;
    TCHAR szTemp[MAX_PATH];
    TCHAR szConfig[MAX_PATH];
    PPOWER_POLICY pp;
    HWND hwndCtrl;

    iCurSel = (INT)SendDlgItemMessage(hwndDlg,
                                          IDC_ENERGYLIST,
                                          CB_GETCURSEL,
                                          0,
                                          0);
    if (iCurSel == CB_ERR)
        return;

    EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE_BTN),
                (iCurSel > 0));

    if (pScheme == NULL)
    {
        pScheme = (PPOWER_SCHEME)SendDlgItemMessage(hwndDlg,
                                                    IDC_ENERGYLIST,
                                                    CB_GETITEMDATA,
                                                    (WPARAM)iCurSel,
                                                    0);
        if (pScheme == (PPOWER_SCHEME)CB_ERR)
            return;
    }

    pPageData->pSelectedPowerScheme = pScheme;

    if (LoadString(hApplet, IDS_CONFIG1, szTemp, MAX_PATH))
    {
        _stprintf(szConfig, szTemp, pScheme->pszName);
        SetWindowText(GetDlgItem(hwndDlg, IDC_GRPDETAIL), szConfig);
    }

    pp = &pScheme->PowerPolicy;

    for (i = 0; i < 16; i++)
    {
        hwndCtrl = GetDlgItem(hwndDlg, IDC_MONITORACLIST);
        if (hwndCtrl != NULL && Sec[i] == pp->user.VideoTimeoutAc)
        {
            SendMessage(hwndCtrl, CB_SETCURSEL, i, 0);
        }

        hwndCtrl = GetDlgItem(hwndDlg, IDC_MONITORDCLIST);
        if (hwndCtrl != NULL && Sec[i] == pp->user.VideoTimeoutDc)
        {
            SendMessage(hwndCtrl, CB_SETCURSEL, i, 0);
        }

        hwndCtrl = GetDlgItem(hwndDlg, IDC_DISKACLIST);
        if (hwndCtrl != NULL && Sec[i] == pp->user.SpindownTimeoutAc)
        {
            SendMessage(hwndCtrl, CB_SETCURSEL, i - 2, 0);
        }

        hwndCtrl = GetDlgItem(hwndDlg, IDC_DISKDCLIST);
        if (hwndCtrl != NULL && Sec[i] == pp->user.SpindownTimeoutDc)
        {
            SendMessage(hwndCtrl, CB_SETCURSEL, i - 2, 0);
        }

        hwndCtrl = GetDlgItem(hwndDlg, IDC_STANDBYACLIST);
        if (hwndCtrl != NULL && Sec[i] == pp->user.IdleTimeoutAc)
        {
            SendMessage(hwndCtrl, CB_SETCURSEL, i, 0);
        }

        hwndCtrl = GetDlgItem(hwndDlg, IDC_STANDBYDCLIST);
        if (hwndCtrl != NULL && Sec[i] == pp->user.IdleTimeoutDc)
        {
            SendMessage(hwndCtrl, CB_SETCURSEL, i, 0);
        }

        hwndCtrl = GetDlgItem(hwndDlg, IDC_HIBERNATEACLIST);
        if (hwndCtrl != NULL && Sec[i] == pp->mach.DozeS4TimeoutAc)
        {
            SendMessage(hwndCtrl, CB_SETCURSEL, i, 0);
        }

        hwndCtrl = GetDlgItem(hwndDlg, IDC_HIBERNATEDCLIST);
        if (hwndCtrl != NULL && Sec[i] == pp->mach.DozeS4TimeoutDc)
        {
            SendMessage(hwndCtrl, CB_SETCURSEL, i, 0);
        }
    }
}


static VOID
Pos_InitPage(HWND hwndDlg)
{
    int ifrom = 0, i = 0, imin = 0;
    HWND hwnd = NULL;
    TCHAR szName[MAX_PATH];
    LRESULT index;

    for (i = 1; i < 9; i++)
    {
        switch (i)
        {
            case 1:
                hwnd = GetDlgItem(hwndDlg, IDC_MONITORACLIST);
                imin = IDS_TIMEOUT1;
                break;

            case 2:
                hwnd = GetDlgItem(hwndDlg, IDC_STANDBYACLIST);
                imin = IDS_TIMEOUT1;
                break;

            case 3:
                hwnd = GetDlgItem(hwndDlg, IDC_DISKACLIST);
                imin = IDS_TIMEOUT3;
                break;

            case 4:
                hwnd = GetDlgItem(hwndDlg, IDC_HIBERNATEACLIST);
                imin = IDS_TIMEOUT3;
                break;

            case 5:
                hwnd = GetDlgItem(hwndDlg, IDC_MONITORDCLIST);
                imin = IDS_TIMEOUT1;
                break;

            case 6:
                hwnd = GetDlgItem(hwndDlg, IDC_STANDBYDCLIST);
                imin = IDS_TIMEOUT1;
                break;

            case 7:
                hwnd = GetDlgItem(hwndDlg, IDC_DISKDCLIST);
                imin = IDS_TIMEOUT3;
                break;

            case 8:
                hwnd = GetDlgItem(hwndDlg, IDC_HIBERNATEDCLIST);
                imin = IDS_TIMEOUT3;
                break;

            default:
                hwnd = NULL;
                return;
        }

        if (hwnd == NULL)
            continue;

        for (ifrom = imin; ifrom < (IDS_TIMEOUT15 + 1); ifrom++)
        {
            if (LoadString(hApplet, ifrom, szName, MAX_PATH))
            {
                index = SendMessage(hwnd,
                                     CB_ADDSTRING,
                                     0,
                                    (LPARAM)szName);
                if (index == CB_ERR)
                    return;

                SendMessage(hwnd,
                             CB_SETITEMDATA,
                             index,
                             (LPARAM)Sec[ifrom - IDS_TIMEOUT16]);
            }
        }

        if (LoadString(hApplet, IDS_TIMEOUT16, szName, MAX_PATH))
        {
            index = SendMessage(hwnd,
                                 CB_ADDSTRING,
                                 0,
                                 (LPARAM)szName);
            if (index == CB_ERR)
                return;

            SendMessage(hwnd,
                         CB_SETITEMDATA,
                         index,
                         (LPARAM)Sec[0]);
        }
    }
}


static VOID
Pos_SaveData(
    HWND hwndDlg,
    PPOWER_SCHEMES_PAGE_DATA pPageData)
{
    PPOWER_SCHEME pScheme;
    HWND hwndCtrl;
    INT tmp;

    pScheme = pPageData->pSelectedPowerScheme;

    hwndCtrl = GetDlgItem(hwndDlg, IDC_MONITORACLIST);
    if (hwndCtrl != NULL)
    {
        tmp = (INT)SendMessage(hwndCtrl, CB_GETCURSEL, 0, 0);
        if (tmp > 0 && tmp < 16)
        {
            pScheme->PowerPolicy.user.VideoTimeoutAc = Sec[tmp];
        }
    }

    hwndCtrl = GetDlgItem(hwndDlg, IDC_MONITORDCLIST);
    if (hwndCtrl != NULL)
    {
        tmp = (INT)SendMessage(hwndCtrl, CB_GETCURSEL, 0, 0);
        if (tmp > 0 && tmp < 16)
        {
            pScheme->PowerPolicy.user.VideoTimeoutDc = Sec[tmp];
        }
    }

    hwndCtrl = GetDlgItem(hwndDlg, IDC_DISKACLIST);
    if (hwndCtrl != NULL)
    {
        tmp = (INT)SendMessage(hwndCtrl, CB_GETCURSEL, 0, 0);
        if (tmp > 0 && tmp < 16)
        {
            pScheme->PowerPolicy.user.SpindownTimeoutAc = Sec[tmp + 2];
        }
    }

    hwndCtrl = GetDlgItem(hwndDlg, IDC_DISKDCLIST);
    if (hwndCtrl != NULL)
    {
        tmp = (INT)SendMessage(hwndCtrl, CB_GETCURSEL, 0, 0);
        if (tmp > 0 && tmp < 16)
        {
            pScheme->PowerPolicy.user.SpindownTimeoutDc = Sec[tmp + 2];
        }
    }

    hwndCtrl = GetDlgItem(hwndDlg, IDC_STANDBYACLIST);
    if (hwndCtrl != NULL)
    {
        tmp = (INT)SendMessage(hwndCtrl, CB_GETCURSEL, 0, 0);
        if (tmp > 0 && tmp < 16)
        {
            pScheme->PowerPolicy.user.IdleTimeoutAc = Sec[tmp];
        }
    }

    hwndCtrl = GetDlgItem(hwndDlg, IDC_STANDBYDCLIST);
    if (hwndCtrl != NULL)
    {
        tmp = (INT)SendMessage(hwndCtrl, CB_GETCURSEL, 0, 0);
        if (tmp > 0 && tmp < 16)
        {
            pScheme->PowerPolicy.user.IdleTimeoutDc = Sec[tmp];
        }
    }

    hwndCtrl = GetDlgItem(hwndDlg, IDC_HIBERNATEACLIST);
    if (hwndCtrl != NULL)
    {
        tmp = (INT)SendMessage(hwndCtrl, CB_GETCURSEL, 0, 0);
        if (tmp > 0 && tmp < 16)
        {
            pScheme->PowerPolicy.mach.DozeS4TimeoutAc = Sec[tmp];
        }
    }

    hwndCtrl = GetDlgItem(hwndDlg, IDC_HIBERNATEDCLIST);
    if (hwndCtrl != NULL)
    {
        tmp = (INT)SendMessage(hwndCtrl, CB_GETCURSEL, 0, 0);
        if (tmp > 0 && tmp < 16)
        {
            pScheme->PowerPolicy.mach.DozeS4TimeoutDc = Sec[tmp];
        }
    }

    if (SetActivePwrScheme(pScheme->uId, NULL, &pScheme->PowerPolicy))
    {
        pPageData->pActivePowerScheme = pScheme;
    }
}


static
BOOL
DelScheme(
    HWND hwnd,
    PPOWER_SCHEMES_PAGE_DATA pPageData)
{
    WCHAR szTitleBuffer[256];
    WCHAR szRawBuffer[256], szCookedBuffer[512];
    INT iCurSel;
    HWND hList;
    PPOWER_SCHEME pScheme;
    WCHAR szErrorText[512];

    hList = GetDlgItem(hwnd, IDC_ENERGYLIST);

    iCurSel = SendMessage(hList, CB_GETCURSEL, 0, 0);
    if (iCurSel == CB_ERR)
        return FALSE;

    SendMessage(hList, CB_SETCURSEL, iCurSel, 0);

    pScheme = (PPOWER_SCHEME)SendMessage(hList, CB_GETITEMDATA, (WPARAM)iCurSel, 0);
    if (pScheme == (PPOWER_SCHEME)CB_ERR)
        return FALSE;

    LoadStringW(hApplet, IDS_DEL_SCHEME_TITLE, szTitleBuffer, ARRAYSIZE(szTitleBuffer));
    LoadStringW(hApplet, IDS_DEL_SCHEME, szRawBuffer, ARRAYSIZE(szRawBuffer));
    StringCchPrintfW(szCookedBuffer, ARRAYSIZE(szCookedBuffer), szRawBuffer, pScheme->pszName);

    if (MessageBoxW(hwnd, szCookedBuffer, szTitleBuffer, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
    {
        if (!DeletePwrScheme(pScheme->uId))
        {
            LoadStringW(hApplet, IDS_DEL_SCHEME_ERROR, szErrorText, sizeof(szErrorText) / sizeof(WCHAR));
            MessageBoxW(NULL, szErrorText, NULL, MB_OK | MB_ICONERROR);
            return FALSE;
        }

        iCurSel = SendMessage(hList, CB_FINDSTRING, -1, (LPARAM)pScheme->pszName);
        if (iCurSel != CB_ERR)
            SendMessage(hList, CB_DELETESTRING, iCurSel, 0);

        DeletePowerScheme(pScheme);

        iCurSel = SendMessage(hList, CB_FINDSTRING, -1, (LPARAM)pPageData->pActivePowerScheme->pszName);
        if (iCurSel != CB_ERR)
        {
            SendMessage(hList, CB_SETCURSEL, iCurSel, 0);
        }
        else
        {
            SendMessage(hList, CB_SETCURSEL, 0, 0);
        }

        LoadConfig(hwnd, pPageData, NULL);
        return TRUE;
    }

    return FALSE;
}


static
BOOL
SavePowerScheme(
    HWND hwndDlg,
    PSAVE_POWER_SCHEME_DATA pSaveSchemeData)
{
    PPOWER_SCHEMES_PAGE_DATA pPageData;
    PPOWER_SCHEME pScheme;
    TCHAR szSchemeName[512];
    BOOL bRet = FALSE;

    pPageData = pSaveSchemeData->pPageData;

    GetDlgItemText(hwndDlg, IDC_SCHEMENAME, szSchemeName, ARRAYSIZE(szSchemeName));

    pScheme = AddPowerScheme(pPageData,
                             -1,
                             (_tcslen(szSchemeName) + 1) * sizeof(TCHAR),
                             szSchemeName,
                             sizeof(TCHAR),
                             TEXT(""),
                             &pPageData->pSelectedPowerScheme->PowerPolicy);
    if (pScheme != NULL)
    {
        if (WritePwrScheme(&pScheme->uId,
                           pScheme->pszName,
                           pScheme->pszDescription,
                           &pScheme->PowerPolicy))
        {
            pSaveSchemeData->pNewScheme = pScheme;
            bRet = TRUE;
        }
        else
        {
            DeletePowerScheme(pScheme);
        }
    }

    return bRet;
}


INT_PTR
CALLBACK
SaveSchemeDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PSAVE_POWER_SCHEME_DATA pSaveSchemeData;

    pSaveSchemeData = (PSAVE_POWER_SCHEME_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pSaveSchemeData = (PSAVE_POWER_SCHEME_DATA)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pSaveSchemeData);

            SetDlgItemText(hwndDlg,
                           IDC_SCHEMENAME,
                           pSaveSchemeData->pPageData->pSelectedPowerScheme->pszName);
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg, SavePowerScheme(hwndDlg, pSaveSchemeData));
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, FALSE);
                    break;
            }
            break;
    }

    return FALSE;
}


static
VOID
SaveScheme(
    HWND hwndDlg,
    PPOWER_SCHEMES_PAGE_DATA pPageData)
{
    SAVE_POWER_SCHEME_DATA SaveSchemeData;
    POWER_POLICY BackupPowerPolicy;
    HWND hwndList;
    INT index;

    SaveSchemeData.pPageData = pPageData;
    SaveSchemeData.pNewScheme = NULL;
    SaveSchemeData.hwndPage = hwndDlg;

    CopyMemory(&BackupPowerPolicy,
               &pPageData->pSelectedPowerScheme->PowerPolicy,
               sizeof(POWER_POLICY));

    Pos_SaveData(hwndDlg, pPageData);

    if (DialogBoxParam(hApplet,
                       MAKEINTRESOURCE(IDD_SAVEPOWERSCHEME),
                       hwndDlg,
                       SaveSchemeDlgProc,
                       (LPARAM)&SaveSchemeData))
    {
        if (SaveSchemeData.pNewScheme)
        {
            hwndList = GetDlgItem(hwndDlg, IDC_ENERGYLIST);

            index = (INT)SendDlgItemMessage(hwndDlg,
                                          IDC_ENERGYLIST,
                                          CB_FINDSTRING,
                                          -1,
                                          (LPARAM)SaveSchemeData.pNewScheme->pszName);

            if (index == CB_ERR)
            {
                index = (INT)SendMessage(hwndList,
                                         CB_ADDSTRING,
                                         0,
                                         (LPARAM)SaveSchemeData.pNewScheme->pszName);
                if (index != CB_ERR)
                {
                    SendMessage(hwndList,
                                CB_SETITEMDATA,
                                index,
                                (LPARAM)SaveSchemeData.pNewScheme);

                    SendMessage(hwndList, CB_SETCURSEL, (WPARAM)index, 0);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE_BTN), TRUE);
                }

            }
            LoadConfig(hwndDlg, pPageData, SaveSchemeData.pNewScheme);
        }
    }

    CopyMemory(&pPageData->pSelectedPowerScheme->PowerPolicy,
               &BackupPowerPolicy,
               sizeof(POWER_POLICY));
}


static BOOL
CreateEnergyList(
    HWND hwndDlg,
    PPOWER_SCHEMES_PAGE_DATA pPageData)
{
    PLIST_ENTRY ListEntry;
    PPOWER_SCHEME pScheme;
    INT index;
    POWER_POLICY pp;
    SYSTEM_POWER_CAPABILITIES spc;
    HWND hwndList;
    UINT aps = 0;

    hwndList = GetDlgItem(hwndDlg, IDC_ENERGYLIST);

    if (!GetActivePwrScheme(&aps))
        return FALSE;

    if (!ReadGlobalPwrPolicy(&gGPP))
        return FALSE;

    if (!ReadPwrScheme(aps, &pp))
        return FALSE;

    if (!ValidatePowerPolicies(&gGPP, 0))
        return FALSE;

/*
    if (!SetActivePwrScheme(aps, &gGPP, &pp))
        return FALSE;
*/

    if (!GetPwrCapabilities(&spc))
        return FALSE;

    if (CanUserWritePwrScheme())
    {
        // TODO:
        // Enable write / delete powerscheme button
    }

    Pos_InitPage(hwndDlg);

    if (!GetActivePwrScheme(&aps))
        return FALSE;

    ListEntry = pPageData->PowerSchemesList.Flink;
    while (ListEntry != &pPageData->PowerSchemesList)
    {
        pScheme = CONTAINING_RECORD(ListEntry, POWER_SCHEME, ListEntry);

        index = (int)SendMessage(hwndList,
                                 CB_ADDSTRING,
                                 0,
                                 (LPARAM)pScheme->pszName);
        if (index == CB_ERR)
            break;

        SendMessage(hwndList,
                    CB_SETITEMDATA,
                    index,
                    (LPARAM)pScheme);

        if (aps == pScheme->uId)
        {
            SendMessage(hwndList,
                        CB_SELECTSTRING,
                        TRUE,
                        (LPARAM)pScheme->pszName);

            pPageData->pActivePowerScheme = pScheme;
            LoadConfig(hwndDlg, pPageData, pScheme);
        }

        ListEntry = ListEntry->Flink;
    }

    if (SendMessage(hwndList, CB_GETCOUNT, 0, 0) > 0)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_SAVEAS_BTN), TRUE);
    }

    return TRUE;
}


/* Property page dialog callback */
INT_PTR CALLBACK
PowerSchemesDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PPOWER_SCHEMES_PAGE_DATA pPageData;

    pPageData = (PPOWER_SCHEMES_PAGE_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pPageData = (PPOWER_SCHEMES_PAGE_DATA)HeapAlloc(GetProcessHeap(),
                                                            HEAP_ZERO_MEMORY,
                                                            sizeof(POWER_SCHEMES_PAGE_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pPageData);

            BuildSchemesList(pPageData);

            if (!Pos_InitData(hwndDlg))
            {
                // TODO:
                // Initialization failed
                // Handle error
                MessageBox(hwndDlg,_T("Pos_InitData failed\n"), NULL, MB_OK);
            }

            if (!CreateEnergyList(hwndDlg, pPageData))
            {
                // TODO:
                // Initialization failed
                // Handle error
                MessageBox(hwndDlg,_T("CreateEnergyList failed\n"), NULL, MB_OK);
            }
            return TRUE;

        case WM_DESTROY:
            if (pPageData)
            {
                DestroySchemesList(pPageData);
                HeapFree(GetProcessHeap(), 0, pPageData);
                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)NULL);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_ENERGYLIST:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        LoadConfig(hwndDlg, pPageData, NULL);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_DELETE_BTN:
                    DelScheme(hwndDlg, pPageData);
                    break;

                case IDC_SAVEAS_BTN:
                    SaveScheme(hwndDlg, pPageData);
                    break;

                case IDC_MONITORACLIST:
                case IDC_MONITORDCLIST:
                case IDC_DISKACLIST:
                case IDC_DISKDCLIST:
                case IDC_STANDBYACLIST:
                case IDC_STANDBYDCLIST:
                case IDC_HIBERNATEACLIST:
                case IDC_HIBERNATEDCLIST:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                    Pos_SaveData(hwndDlg, pPageData);
                    return TRUE;

                case PSN_SETACTIVE:
                    Pos_InitData(hwndDlg);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}
