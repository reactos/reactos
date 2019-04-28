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
} POWER_SCHEMES_PAGE_DATA, *PPOWER_SCHEMES_PAGE_DATA;


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

    pScheme = HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        sizeof(POWER_SCHEME));
    if (pScheme == NULL)
        return NULL;

    pScheme->pszName = HeapAlloc(GetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 dwName + sizeof(TCHAR));

    pScheme->pszDescription = HeapAlloc(GetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        dwDescription + sizeof(TCHAR));
    if (pScheme->pszName == NULL || pScheme->pszDescription == NULL)
    {
        if (pScheme->pszName)
            HeapFree(GetProcessHeap(), 0, pScheme->pszName);

        if (pScheme->pszDescription)
            HeapFree(GetProcessHeap(), 0, pScheme->pszDescription);

        HeapFree(GetProcessHeap(), 0, pScheme);
        return NULL;
    }

    pScheme->uId = uId;
    _tcscpy(pScheme->pszName, pszName);
    _tcscpy(pScheme->pszDescription, pszDescription);

    CopyMemory(&pScheme->PowerPolicy, pp, sizeof(POWER_POLICY));

    InsertTailList(&pPageData->PowerSchemesList, &pScheme->ListEntry);

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
}


BOOLEAN
Pos_InitData(
    HWND hwndDlg)
{
    SYSTEM_POWER_CAPABILITIES spc;
/*
    RECT rectCtl, rectDlg, rectCtl2;
    LONG movetop = 0;
    LONG moveright = 0;

    if (GetWindowRect(hwndDlg,&rectDlg))
        {
            if (GetWindowRect(GetDlgItem(hwndDlg, IDC_SAT),&rectCtl2))
            {
                if (GetWindowRect(GetDlgItem(hwndDlg, IDC_MONITOR),&rectCtl))
                {
                    movetop=rectCtl.top - rectCtl2.top;
                    MoveWindow(GetDlgItem(hwndDlg, IDC_MONITOR),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left,rectCtl.bottom-rectCtl.top,FALSE);
                    if (GetWindowRect(GetDlgItem(hwndDlg, IDC_DISK),&rectCtl))
                    {
                        MoveWindow(GetDlgItem(hwndDlg, IDC_DISK),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left,rectCtl.bottom-rectCtl.top,FALSE);
                    }
                    if (GetWindowRect(GetDlgItem(hwndDlg, IDC_STANDBY),&rectCtl))
                    {
                        MoveWindow(GetDlgItem(hwndDlg, IDC_STANDBY),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left,rectCtl.bottom-rectCtl.top,FALSE);
                    }
                    if (GetWindowRect(GetDlgItem(hwndDlg, IDC_HIBERNATE),&rectCtl))
                    {
                        MoveWindow(GetDlgItem(hwndDlg, IDC_HIBERNATE),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left,rectCtl.bottom-rectCtl.top,FALSE);
                    }
                    if (GetWindowRect(GetDlgItem(hwndDlg, IDC_MONITORDCLIST),&rectCtl2))
                    {
                        movetop=movetop-8;
                        if (GetWindowRect(GetDlgItem(hwndDlg, IDC_MONITORACLIST),&rectCtl))
                        {
                            moveright=rectCtl.right - rectCtl2.right;
                            MoveWindow(GetDlgItem(hwndDlg, IDC_MONITORACLIST),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left-moveright,rectCtl.bottom-rectCtl.top,FALSE);
                            if (GetWindowRect(GetDlgItem(hwndDlg, IDC_DISKACLIST),&rectCtl))
                            {
                                MoveWindow(GetDlgItem(hwndDlg, IDC_DISKACLIST),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left-moveright,rectCtl.bottom-rectCtl.top,FALSE);
                            }
                            if (GetWindowRect(GetDlgItem(hwndDlg, IDC_STANDBYACLIST),&rectCtl))
                            {
                                MoveWindow(GetDlgItem(hwndDlg, IDC_STANDBYACLIST),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left-moveright,rectCtl.bottom-rectCtl.top,FALSE);
                            }
                            if (GetWindowRect(GetDlgItem(hwndDlg, IDC_HIBERNATEACLIST),&rectCtl))
                            {
                                MoveWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEACLIST),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top-movetop,rectCtl.right-rectCtl.left-moveright,rectCtl.bottom-rectCtl.top,FALSE);
                            }
                        }
                        if (GetWindowRect(GetDlgItem(hwndDlg, IDC_GRPDETAIL),&rectCtl))
                        {
                            MoveWindow(GetDlgItem(hwndDlg, IDC_GRPDETAIL),rectCtl.left-rectDlg.left,rectCtl.top-rectDlg.top,rectCtl.right-rectCtl.left,rectCtl.bottom-rectCtl.top,FALSE);
                        }
                    }
                }
            }
        }
    }
*/

    if (!GetPwrCapabilities(&spc))
    {
        return FALSE;
    }

    if (!spc.SystemBatteriesPresent)
    {
        ShowWindow(GetDlgItem(hwndDlg, IDC_SAT), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_IAC), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SAC), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_IDC), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SDC), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_MONITORDCLIST), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_DISKDCLIST), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_STANDBYDCLIST), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEDCLIST), SW_HIDE);
    }

    if (!(spc.SystemS1 || spc.SystemS2 || spc.SystemS3))
    {
        ShowWindow(GetDlgItem(hwndDlg, IDC_STANDBY), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_STANDBYACLIST), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_STANDBYDCLIST), SW_HIDE);
    }

    if (!spc.HiberFilePresent)
    {
        ShowWindow(GetDlgItem(hwndDlg, IDC_HIBERNATE), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEACLIST), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEDCLIST), SW_HIDE);
    }

    return TRUE;
}


static
VOID
LoadConfig(
    HWND hwndDlg)
{
    PPOWER_SCHEME pScheme;
    INT i = 0, iCurSel = 0;
    TCHAR szTemp[MAX_PATH];
    TCHAR szConfig[MAX_PATH];
    PPOWER_POLICY pp;

    iCurSel = (INT)SendDlgItemMessage(hwndDlg,
                                      IDC_ENERGYLIST,
                                      CB_GETCURSEL,
                                      0,
                                      0);
    if (iCurSel == CB_ERR)
        return;

    pScheme = (PPOWER_SCHEME)SendDlgItemMessage(hwndDlg,
                                                IDC_ENERGYLIST,
                                                CB_GETITEMDATA,
                                                (WPARAM)iCurSel,
                                                0);
    if (pScheme == (PPOWER_SCHEME)CB_ERR)
        return;

    if (LoadString(hApplet, IDS_CONFIG1, szTemp, MAX_PATH))
    {
        _stprintf(szConfig, szTemp, pScheme->pszName);
        SetWindowText(GetDlgItem(hwndDlg, IDC_GRPDETAIL), szConfig);
    }

    pp = &pScheme->PowerPolicy;

    for (i = 0; i < 16; i++)
    {
        if (Sec[i] == pp->user.VideoTimeoutAc)
        {
            SendDlgItemMessage(hwndDlg, IDC_MONITORACLIST,
                        CB_SETCURSEL,
                        i,
                        (LPARAM)0);
        }

        if (Sec[i] == pp->user.VideoTimeoutDc)
        {
            SendDlgItemMessage(hwndDlg, IDC_MONITORDCLIST,
                        CB_SETCURSEL,
                         i,
                         (LPARAM)0);
        }

        if (Sec[i] == pp->user.SpindownTimeoutAc)
        {
            SendDlgItemMessage(hwndDlg, IDC_DISKACLIST,
                       CB_SETCURSEL,
                       i - 2,
                       (LPARAM)0);
        }

        if (Sec[i] == pp->user.SpindownTimeoutDc)
        {
            SendDlgItemMessage(hwndDlg, IDC_DISKDCLIST,
                       CB_SETCURSEL,
                       i - 2,
                       (LPARAM)0);
        }

        if (Sec[i] == pp->user.IdleTimeoutAc)
        {
            SendDlgItemMessage(hwndDlg, IDC_STANDBYACLIST,
                       CB_SETCURSEL,
                       i,
                       (LPARAM)0);
        }

        if (Sec[i] == pp->user.IdleTimeoutDc)
        {
            SendDlgItemMessage(hwndDlg, IDC_STANDBYDCLIST,
                       CB_SETCURSEL,
                       i,
                       (LPARAM)0);
        }

        if (Sec[i] == pp->mach.DozeS4TimeoutAc)
        {
            SendDlgItemMessage(hwndDlg, IDC_HIBERNATEACLIST,
                       CB_SETCURSEL,
                       i,
                    (LPARAM)0);
        }

        if (Sec[i] == pp->mach.DozeS4TimeoutDc)
        {
            SendDlgItemMessage(hwndDlg, IDC_HIBERNATEDCLIST,
                       CB_SETCURSEL,
                       i,
                       (LPARAM)0);
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
                return;
        }

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
Pos_SaveData(HWND hwndDlg)
{
    PPOWER_SCHEME pScheme;
    INT iCurSel, tmp;

    iCurSel = (INT)SendDlgItemMessage(hwndDlg,
                                      IDC_ENERGYLIST,
                                      CB_GETCURSEL,
                                      0,
                                      0);
    if (iCurSel == CB_ERR)
        return;

    pScheme = (PPOWER_SCHEME)SendDlgItemMessage(hwndDlg,
                                                IDC_ENERGYLIST,
                                                CB_GETITEMDATA,
                                                (WPARAM)iCurSel,
                                                0);
    if (pScheme == (PPOWER_SCHEME)CB_ERR)
        return;

    tmp = (INT)SendDlgItemMessage(hwndDlg, IDC_MONITORACLIST,
                   CB_GETCURSEL,
                   0,
                   (LPARAM)0);
    if (tmp > 0 && tmp < 16)
    {
        pScheme->PowerPolicy.user.VideoTimeoutAc = Sec[tmp];
    }

    tmp = (INT)SendDlgItemMessage(hwndDlg, IDC_MONITORDCLIST,
                   CB_GETCURSEL,
                   0,
                   (LPARAM)0);
    if (tmp > 0 && tmp < 16)
    {
        pScheme->PowerPolicy.user.VideoTimeoutDc = Sec[tmp];
    }

    tmp = (INT)SendDlgItemMessage(hwndDlg, IDC_DISKACLIST,
                   CB_GETCURSEL,
                   0,
                   (LPARAM)0);
    if (tmp > 0 && tmp < 16)
    {
        pScheme->PowerPolicy.user.SpindownTimeoutAc = Sec[tmp + 2];
    }

    tmp = (INT)SendDlgItemMessage(hwndDlg, IDC_DISKDCLIST,
                   CB_GETCURSEL,
                   0,
                   (LPARAM)0);
    if (tmp > 0 && tmp < 16)
    {
        pScheme->PowerPolicy.user.SpindownTimeoutDc = Sec[tmp + 2];
    }

    tmp = (INT)SendDlgItemMessage(hwndDlg, IDC_STANDBYACLIST,
                   CB_GETCURSEL,
                   0,
                   (LPARAM)0);
    if (tmp > 0 && tmp < 16)
    {
        pScheme->PowerPolicy.user.IdleTimeoutAc = Sec[tmp];
    }

    tmp = (INT)SendDlgItemMessage(hwndDlg, IDC_STANDBYDCLIST,
                   CB_GETCURSEL,
                   0,
                   (LPARAM)0);
    if (tmp > 0 && tmp < 16)
    {
        pScheme->PowerPolicy.user.IdleTimeoutDc = Sec[tmp];
    }

    tmp = (INT)SendDlgItemMessage(hwndDlg, IDC_HIBERNATEACLIST,
                   CB_GETCURSEL,
                   0,
                   (LPARAM)0);
    if (tmp > 0 && tmp < 16)
    {
        pScheme->PowerPolicy.mach.DozeS4TimeoutAc = Sec[tmp];
    }

    tmp = (INT)SendDlgItemMessage(hwndDlg, IDC_HIBERNATEDCLIST,
                   CB_GETCURSEL,
                   0,
                   (LPARAM)0);
    if (tmp > 0 && tmp < 16)
    {
        pScheme->PowerPolicy.mach.DozeS4TimeoutDc = Sec[tmp];
    }

    SetActivePwrScheme(pScheme->uId, NULL, &pScheme->PowerPolicy);
    LoadConfig(hwndDlg);
}


static BOOL
DelScheme(HWND hwnd)
{
    WCHAR szTitleBuffer[256];
    WCHAR szRawBuffer[256], szCookedBuffer[512];
    INT iCurSel;
    HWND hList;
    PPOWER_SCHEME pScheme;

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
        UINT Current;

        if (GetActivePwrScheme(&Current))
        {
            SendMessage(hList, CB_SETCURSEL, (WPARAM)0, 0);
            SendMessage(hList, CB_DELETESTRING, (WPARAM)iCurSel, 0);
            if (Current == pScheme->uId)
                Pos_SaveData(hwnd);
        }

        if (DeletePwrScheme(pScheme->uId) != 0)
            return TRUE;
    }

    return FALSE;
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
    unsigned aps = 0;

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

            LoadConfig(hwndDlg);
        }

        ListEntry = ListEntry->Flink;
    }

    if (SendMessage(hwndList, CB_GETCOUNT, 0, 0) > 0)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE_BTN), TRUE);
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
                                                            0,
                                                            sizeof(PPOWER_SCHEMES_PAGE_DATA));
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
                        LoadConfig(hwndDlg);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_DELETE_BTN:
                    DelScheme(hwndDlg);
                    break;

                case IDC_SAVEAS_BTN:
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
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;
                if (lpnm->code == (UINT)PSN_APPLY)
                {
                    Pos_SaveData(hwndDlg);
                }
                return TRUE;
            }
            break;
    }

    return FALSE;
}
