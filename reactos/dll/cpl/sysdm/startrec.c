/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/startrec.c
 * PURPOSE:     Computer settings for startup and recovery
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *              Copyright 2007 Johannes Anderwald <johannes dot anderwald at student dot tugraz dot at>
 */

#include "precomp.h"

typedef struct _STARTINFO
{
    WCHAR szFreeldrIni[MAX_PATH + 15];
    WCHAR szDumpFile[MAX_PATH];
    WCHAR szMinidumpDir[MAX_PATH];
    DWORD dwCrashDumpEnabled;
    INT iFreeLdrIni;
} STARTINFO, *PSTARTINFO;


static VOID
SetTimeout(HWND hwndDlg, INT Timeout)
{
    if (Timeout == 0)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_STRRECLISTUPDWN), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_STRRECLISTEDIT), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_STRRECLISTUPDWN), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_STRRECLISTEDIT), TRUE);
    }
    SendDlgItemMessageW(hwndDlg, IDC_STRRECLISTUPDWN, UDM_SETRANGE, (WPARAM) 0, (LPARAM) MAKELONG((short) 999, 0));
    SendDlgItemMessageW(hwndDlg, IDC_STRRECLISTUPDWN, UDM_SETPOS, (WPARAM) 0, (LPARAM) MAKELONG((short) Timeout, 0));
}

static VOID
SetRecoveryTimeout(HWND hwndDlg, INT Timeout)
{
    if (Timeout == 0)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_STRRECRECUPDWN), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_STRRECRECEDIT), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_STRRECRECUPDWN), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_STRRECRECEDIT), TRUE);
    }
    SendDlgItemMessageW(hwndDlg, IDC_STRRECRECUPDWN, UDM_SETRANGE, (WPARAM) 0, (LPARAM) MAKELONG((short) 999, 0));
    SendDlgItemMessageW(hwndDlg, IDC_STRRECRECUPDWN, UDM_SETPOS, (WPARAM) 0, (LPARAM) MAKELONG((short) Timeout, 0));
}


static DWORD
GetSystemDrive(WCHAR **szSystemDrive)
{
    DWORD dwBufSize;

    /* get Path to freeldr.ini or boot.ini */
    *szSystemDrive = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    if (*szSystemDrive != NULL)
    {
        dwBufSize = GetEnvironmentVariableW(L"SystemDrive", *szSystemDrive, MAX_PATH);
        if (dwBufSize > MAX_PATH)
        {
            WCHAR *szTmp;
            DWORD dwBufSize2;

            szTmp = HeapReAlloc(GetProcessHeap(), 0, *szSystemDrive, dwBufSize * sizeof(WCHAR));
            if (szTmp == NULL)
                goto FailGetSysDrive;

            *szSystemDrive = szTmp;

            dwBufSize2 = GetEnvironmentVariableW(L"SystemDrive", *szSystemDrive, dwBufSize);
            if (dwBufSize2 > dwBufSize || dwBufSize2 == 0)
                goto FailGetSysDrive;
        }
        else if (dwBufSize == 0)
        {
FailGetSysDrive:
            HeapFree(GetProcessHeap(), 0, *szSystemDrive);
            *szSystemDrive = NULL;
            return 0;
        }

        return dwBufSize;
    }

    return 0;
}

static PBOOTRECORD
ReadFreeldrSection(HINF hInf, WCHAR *szSectionName)
{
    PBOOTRECORD pRecord;
    INFCONTEXT InfContext;
    WCHAR szName[MAX_PATH];
    WCHAR szValue[MAX_PATH];
    DWORD LineLength;

    if (!SetupFindFirstLineW(hInf,
                            szSectionName,
                            NULL,
                            &InfContext))
    {
        /* failed to find section */
        return NULL;
    }

    pRecord = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BOOTRECORD));
    if (pRecord == NULL)
    {
        return NULL;
    }

    wcscpy(pRecord->szSectionName, szSectionName);

    do
    {
        if (!SetupGetStringFieldW(&InfContext,
                                  0,
                                  szName,
                                  sizeof(szName) / sizeof(WCHAR),
                                  &LineLength))
        {
            break;
        }

        if (!SetupGetStringFieldW(&InfContext,
                                  1,
                                  szValue,
                                  sizeof(szValue) / sizeof(WCHAR),
                                  &LineLength))
        {
            break;
        }

        if (!_wcsnicmp(szName, L"BootType", 8))
        {
            if (!_wcsnicmp(szValue, L"ReactOS", 7))
            {
                //FIXME store as enum
                pRecord->BootType = 1;
            }
            else
            {
                pRecord->BootType = 0;
            }
        }
        else if (!_wcsnicmp(szName, L"SystemPath", 10))
        {
            wcscpy(pRecord->szBootPath, szValue);
        }
        else if (!_wcsnicmp(szName, L"Options", 7))
        {
            //FIXME store flags as values
            wcscpy(pRecord->szOptions, szValue);
        }

    }
    while (SetupFindNextLine(&InfContext, &InfContext));

    return pRecord;
}


static INT
LoadFreeldrSettings(HINF hInf, HWND hwndDlg)
{
    INFCONTEXT InfContext;
    PBOOTRECORD pRecord;
    WCHAR szDefaultOs[MAX_PATH];
    WCHAR szName[MAX_PATH];
    WCHAR szValue[MAX_PATH];
    DWORD LineLength;
    DWORD TimeOut;
    LRESULT lResult;

    if (!SetupFindFirstLineW(hInf,
                           L"FREELOADER",
                           L"DefaultOS",
                           &InfContext))
    {
        /* failed to find default os */
        return FALSE;
    }

    if (!SetupGetStringFieldW(&InfContext,
                             1,
                             szDefaultOs,
                             sizeof(szDefaultOs) / sizeof(WCHAR),
                             &LineLength))
    {
        /* no key */
        return FALSE;
    }

    if (!SetupFindFirstLineW(hInf,
                           L"FREELOADER",
                           L"TimeOut",
                           &InfContext))
    {
        /* expected to find timeout value */
        return FALSE;
    }


    if (!SetupGetIntField(&InfContext,
                          1,
                          (PINT)&TimeOut))
    {
        /* failed to retrieve timeout */
        return FALSE;
    }

    if (!SetupFindFirstLineW(hInf,
                           L"Operating Systems",
                           NULL,
                           &InfContext))
    {
       /* expected list of operating systems */
       return FALSE;
    }

    do
    {
        if (!SetupGetStringFieldW(&InfContext,
                                 0,
                                 szName,
                                 sizeof(szName) / sizeof(WCHAR),
                                 &LineLength))
        {
            /* the ini file is messed up */
            return FALSE;
        }

        if (!SetupGetStringFieldW(&InfContext,
                                 1,
                                 szValue,
                                 sizeof(szValue) / sizeof(WCHAR),
                                 &LineLength))
        {
            /* the ini file is messed up */
            return FALSE;
        }

        pRecord = ReadFreeldrSection(hInf, szName);
        if (pRecord)
        {
            lResult = SendDlgItemMessageW(hwndDlg, IDC_STRECOSCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM)szValue);
            if (lResult != CB_ERR)
            {
                SendDlgItemMessageW(hwndDlg, IDC_STRECOSCOMBO, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)pRecord);
                if (!wcscmp(szDefaultOs, szName))
                {
                    /* we store the friendly name as key */
                    wcscpy(szDefaultOs, szValue);
                }
            }
            else
            {
               HeapFree(GetProcessHeap(), 0, pRecord);
            }
        }
    }
    while (SetupFindNextLine(&InfContext, &InfContext));

    /* find default os in list */
    lResult = SendDlgItemMessageW(hwndDlg, IDC_STRECOSCOMBO, CB_FINDSTRING, (WPARAM)-1, (LPARAM)szDefaultOs);
    if (lResult != CB_ERR)
    {
       /* set cur sel */
       SendDlgItemMessageW(hwndDlg, IDC_STRECOSCOMBO, CB_SETCURSEL, (WPARAM)lResult, (LPARAM)0);
    }

    if(TimeOut)
    {
        SendDlgItemMessageW(hwndDlg, IDC_STRECLIST, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
    }

    SetTimeout(hwndDlg, TimeOut);

    return TRUE;
}

static INT
LoadBootSettings(HINF hInf, HWND hwndDlg)
{
    INFCONTEXT InfContext;
    WCHAR szName[MAX_PATH];
    WCHAR szValue[MAX_PATH];
    DWORD LineLength;
    DWORD TimeOut = 0;
    WCHAR szDefaultOS[MAX_PATH];
    WCHAR szOptions[MAX_PATH];
    PBOOTRECORD pRecord;
    LRESULT lResult;

    if(!SetupFindFirstLineW(hInf,
                           L"boot loader",
                           NULL,
                           &InfContext))
    {
        return FALSE;
    }

    do
    {
        if (!SetupGetStringFieldW(&InfContext,
                                 0,
                                 szName,
                                 sizeof(szName) / sizeof(WCHAR),
                                 &LineLength))
        {
            return FALSE;
        }

        if (!SetupGetStringFieldW(&InfContext,
                                 1,
                                 szValue,
                                 sizeof(szValue) / sizeof(WCHAR),
                                 &LineLength))
        {
            return FALSE;
        }

        if (!_wcsnicmp(szName, L"timeout", 7))
        {
            TimeOut = _wtoi(szValue);
        }

        if (!_wcsnicmp(szName, L"default", 7))
        {
            wcscpy(szDefaultOS, szValue);
        }

    }
    while (SetupFindNextLine(&InfContext, &InfContext));

    if (!SetupFindFirstLineW(hInf,
                            L"operating systems",
                            NULL,
                            &InfContext))
    {
        /* failed to find operating systems section */
        return FALSE;
    }

    do
    {
        if (!SetupGetStringFieldW(&InfContext,
                                 0,
                                 szName,
                                 sizeof(szName) / sizeof(WCHAR),
                                 &LineLength))
        {
            return FALSE;
        }

        if (!SetupGetStringFieldW(&InfContext,
                                 1,
                                 szValue,
                                 sizeof(szValue) / sizeof(WCHAR),
                                 &LineLength))
        {
            return FALSE;
        }

        SetupGetStringFieldW(&InfContext,
                            2,
                            szOptions,
                            sizeof(szOptions) / sizeof(WCHAR),
                            &LineLength);

        pRecord = (PBOOTRECORD) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BOOTRECORD));
        if (pRecord)
        {
            pRecord->BootType = 0;
            wcscpy(pRecord->szBootPath, szName);
            wcscpy(pRecord->szSectionName, szValue);
            wcscpy(pRecord->szOptions, szOptions);

            if (!wcscmp(szName, szDefaultOS))
            {
                /* ms boot ini stores the path not the friendly name */
                wcscpy(szDefaultOS, szValue);
            }

            lResult = SendDlgItemMessageW(hwndDlg, IDC_STRECOSCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM)szValue);
            if (lResult != CB_ERR)
            {
                SendDlgItemMessageW(hwndDlg, IDC_STRECOSCOMBO, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)pRecord);
            }
            else
            {
               HeapFree(GetProcessHeap(), 0, pRecord);
            }
        }

    }
    while (SetupFindNextLine(&InfContext, &InfContext));

    /* find default os in list */
    lResult = SendDlgItemMessageW(hwndDlg, IDC_STRECOSCOMBO, CB_FINDSTRING, (WPARAM)0, (LPARAM)szDefaultOS);
    if (lResult != CB_ERR)
    {
       /* set cur sel */
       SendDlgItemMessageW(hwndDlg, IDC_STRECOSCOMBO, CB_SETCURSEL, (WPARAM)lResult, (LPARAM)0);
    }

    if(TimeOut)
    {
        SendDlgItemMessageW(hwndDlg, IDC_STRECLIST, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
    }

    SetTimeout(hwndDlg, TimeOut);

    return TRUE;
}

static VOID
DeleteBootRecords(HWND hwndDlg)
{
    LRESULT lIndex;
    LONG index;
    PBOOTRECORD pRecord;

    lIndex = SendDlgItemMessageW(hwndDlg, IDC_STRECOSCOMBO, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
    if (lIndex == CB_ERR)
        return;

    for (index = 0; index <lIndex; index++)
    {
        pRecord = (PBOOTRECORD) SendDlgItemMessageW(hwndDlg, IDC_STRECOSCOMBO, CB_GETITEMDATA, (WPARAM)index, (LPARAM)0);
        if ((INT_PTR)pRecord != CB_ERR)
        {
            HeapFree(GetProcessHeap(), 0, pRecord);
        }
    }

    SendDlgItemMessageW(hwndDlg, IDC_STRECOSCOMBO, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
}

static LRESULT
LoadOSList(HWND hwndDlg, PSTARTINFO pStartInfo)
{
    DWORD dwBufSize;
    WCHAR *szSystemDrive;
    HINF hInf;

    dwBufSize = GetSystemDrive(&szSystemDrive);
    if (dwBufSize == 0)
        return FALSE;

    wcscpy(pStartInfo->szFreeldrIni, szSystemDrive);
    wcscat(pStartInfo->szFreeldrIni, L"\\freeldr.ini");

    if (PathFileExistsW(pStartInfo->szFreeldrIni))
    {
        /* free resource previously allocated by GetSystemDrive() */
        HeapFree(GetProcessHeap(), 0, szSystemDrive);
        /* freeldr.ini exists */
        hInf = SetupOpenInfFileW(pStartInfo->szFreeldrIni,
                                NULL,
                                INF_STYLE_OLDNT,
                                NULL);

        if (hInf != INVALID_HANDLE_VALUE)
        {
            LoadFreeldrSettings(hInf, hwndDlg);
            SetupCloseInfFile(hInf);
            pStartInfo->iFreeLdrIni = 1;
            return TRUE;
        }
        return FALSE;
    }

    /* try load boot.ini settings */
    wcscpy(pStartInfo->szFreeldrIni, szSystemDrive);
    wcscat(pStartInfo->szFreeldrIni, L"\\boot.ini");

    /* free resource previously allocated by GetSystemDrive() */
    HeapFree(GetProcessHeap(), 0, szSystemDrive);

    if (PathFileExistsW(pStartInfo->szFreeldrIni))
    {
        /* load boot.ini settings */
        hInf = SetupOpenInfFileW(pStartInfo->szFreeldrIni,
                                NULL,
                                INF_STYLE_OLDNT,
                                NULL);

        if (hInf != INVALID_HANDLE_VALUE)
        {
            LoadBootSettings(hInf, hwndDlg);
            SetupCloseInfFile(hInf);
            pStartInfo->iFreeLdrIni = 2;
            return TRUE;
        }

        return FALSE;
    }

    return FALSE;
}

static VOID
SetCrashDlgItems(HWND hwnd, PSTARTINFO pStartInfo)
{
    if (pStartInfo->dwCrashDumpEnabled == 0)
    {
        /* no crash information required */
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECOVERWRITE), FALSE);
    }
    else if (pStartInfo->dwCrashDumpEnabled == 3)
    {
        /* minidump type */
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECOVERWRITE), FALSE);
        SendMessageW(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), WM_SETTEXT, (WPARAM)0, (LPARAM)pStartInfo->szMinidumpDir);
    }
    else if (pStartInfo->dwCrashDumpEnabled == 1 || pStartInfo->dwCrashDumpEnabled == 2)
    {
        /* kernel or complete dump */
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECOVERWRITE), TRUE);
        SendMessageW(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), WM_SETTEXT, (WPARAM)0, (LPARAM)pStartInfo->szDumpFile);
    }
    SendDlgItemMessageW(hwnd, IDC_STRRECDEBUGCOMBO, CB_SETCURSEL, (WPARAM)pStartInfo->dwCrashDumpEnabled, (LPARAM)0);
}

static VOID
WriteStartupRecoveryOptions(HWND hwndDlg, PSTARTINFO pStartInfo)
{
    HKEY hKey;
    DWORD lResult;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                     L"System\\CurrentControlSet\\Control\\CrashControl",
                     0,
                     KEY_WRITE,
                     &hKey) != ERROR_SUCCESS)
    {
        /* failed to open key */
        return;
    }

    lResult = (DWORD) SendDlgItemMessage(hwndDlg, IDC_STRRECWRITEEVENT, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
    RegSetValueExW(hKey, L"LogEvent", 0, REG_DWORD, (LPBYTE)&lResult, sizeof(lResult));

    lResult = (DWORD) SendDlgItemMessage(hwndDlg, IDC_STRRECSENDALERT, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
    RegSetValueExW(hKey, L"SendAlert", 0, REG_DWORD, (LPBYTE)&lResult, sizeof(lResult));

    lResult = (DWORD) SendDlgItemMessage(hwndDlg, IDC_STRRECRESTART, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
    RegSetValueExW(hKey, L"AutoReboot", 0, REG_DWORD, (LPBYTE)&lResult, sizeof(lResult));

    lResult = (DWORD) SendDlgItemMessage(hwndDlg, IDC_STRRECOVERWRITE, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
    RegSetValueExW(hKey, L"Overwrite", 0, REG_DWORD, (LPBYTE)&lResult, sizeof(lResult));


    if (pStartInfo->dwCrashDumpEnabled == 1 || pStartInfo->dwCrashDumpEnabled == 2)
    {
        SendDlgItemMessage(hwndDlg, IDC_STRRECDUMPFILE, WM_GETTEXT, (WPARAM)sizeof(pStartInfo->szDumpFile) / sizeof(WCHAR), (LPARAM)pStartInfo->szDumpFile);
        RegSetValueExW(hKey, L"DumpFile", 0, REG_EXPAND_SZ, (LPBYTE)pStartInfo->szDumpFile, (wcslen(pStartInfo->szDumpFile) + 1) * sizeof(WCHAR));
    }
    else if (pStartInfo->dwCrashDumpEnabled == 3)
    {
        SendDlgItemMessage(hwndDlg, IDC_STRRECDUMPFILE, WM_GETTEXT, (WPARAM)sizeof(pStartInfo->szDumpFile) / sizeof(WCHAR), (LPARAM)pStartInfo->szDumpFile);
        RegSetValueExW(hKey, L"MinidumpDir", 0, REG_EXPAND_SZ, (LPBYTE)pStartInfo->szDumpFile, (wcslen(pStartInfo->szDumpFile) + 1) * sizeof(WCHAR));
    }

    RegSetValueExW(hKey, L"CrashDumpEnabled", 0, REG_DWORD, (LPBYTE)(DWORD_PTR)pStartInfo->dwCrashDumpEnabled, sizeof(pStartInfo->dwCrashDumpEnabled));
    RegCloseKey(hKey);
}

static VOID
LoadRecoveryOptions(HWND hwndDlg, PSTARTINFO pStartInfo)
{
    HKEY hKey;
    WCHAR szName[MAX_PATH];
    DWORD dwValue, dwValueLength, dwType;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     L"System\\CurrentControlSet\\Control\\CrashControl",
                     0,
                     KEY_READ,
                     &hKey) != ERROR_SUCCESS)
    {
        /* failed to open key */
        return;
    }

    dwValueLength = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"LogEvent", NULL, &dwType, (LPBYTE)&dwValue, &dwValueLength) == ERROR_SUCCESS && dwType == REG_DWORD && dwValue)
        SendDlgItemMessageW(hwndDlg, IDC_STRRECWRITEEVENT, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);

    dwValueLength = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"SendAlert", NULL, &dwType, (LPBYTE)&dwValue, &dwValueLength) == ERROR_SUCCESS && dwType == REG_DWORD && dwValue)
        SendDlgItemMessageW(hwndDlg, IDC_STRRECSENDALERT, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);

    dwValueLength = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"AutoReboot", NULL, &dwType, (LPBYTE)&dwValue, &dwValueLength) == ERROR_SUCCESS && dwType == REG_DWORD && dwValue)
        SendDlgItemMessageW(hwndDlg, IDC_STRRECRESTART, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);

    dwValueLength = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"Overwrite", NULL, &dwType, (LPBYTE)&dwValue, &dwValueLength) == ERROR_SUCCESS && dwType == REG_DWORD && dwValue)
        SendDlgItemMessageW(hwndDlg, IDC_STRRECOVERWRITE, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);

    dwValueLength = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"CrashDumpEnabled", NULL, &dwType, (LPBYTE)&dwValue, &dwValueLength) == ERROR_SUCCESS && dwType == REG_DWORD && dwValue)
        pStartInfo->dwCrashDumpEnabled = dwValue;

   dwValueLength = sizeof(pStartInfo->szDumpFile);
   if (RegQueryValueExW(hKey, L"DumpFile", NULL, &dwType, (LPBYTE)pStartInfo->szDumpFile, &dwValueLength) != ERROR_SUCCESS)
       pStartInfo->szDumpFile[0] = L'\0';

    dwValueLength = sizeof(pStartInfo->szMinidumpDir);
    if (RegQueryValueExW(hKey, L"MinidumpDir", NULL, &dwType, (LPBYTE)pStartInfo->szMinidumpDir, &dwValueLength) != ERROR_SUCCESS)
        pStartInfo->szMinidumpDir[0] = L'\0';

    if (LoadStringW(hApplet, IDS_NO_DUMP, szName, sizeof(szName) / sizeof(WCHAR)))
    {
        szName[(sizeof(szName)/sizeof(WCHAR))-1] = L'\0';
        SendDlgItemMessageW(hwndDlg, IDC_STRRECDEBUGCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM) szName);
    }

    if (LoadString(hApplet, IDS_FULL_DUMP, szName, sizeof(szName) / sizeof(WCHAR)))
    {
        szName[(sizeof(szName)/sizeof(WCHAR))-1] = L'\0';
        SendDlgItemMessageW(hwndDlg, IDC_STRRECDEBUGCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM) szName);
    }

    if (LoadStringW(hApplet, IDS_KERNEL_DUMP, szName, sizeof(szName) / sizeof(WCHAR)))
    {
        szName[(sizeof(szName)/sizeof(WCHAR))-1] = L'\0';
        SendDlgItemMessageW(hwndDlg, IDC_STRRECDEBUGCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM) szName);
    }

    if (LoadStringW(hApplet, IDS_MINI_DUMP, szName, sizeof(szName) / sizeof(WCHAR)))
    {
        szName[(sizeof(szName)/sizeof(WCHAR))-1] = L'\0';
        SendDlgItemMessageW(hwndDlg, IDC_STRRECDEBUGCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM) szName);
    }

    SetCrashDlgItems(hwndDlg, pStartInfo);
    RegCloseKey(hKey);
}


/* Property page dialog callback */
INT_PTR CALLBACK
StartRecDlgProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    PSTARTINFO pStartInfo;
    PBOOTRECORD pRecord;
    int iTimeout;
    LRESULT lResult;
    WCHAR szTimeout[10];

    UNREFERENCED_PARAMETER(lParam);

    pStartInfo = (PSTARTINFO)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pStartInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(STARTINFO));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pStartInfo);

            LoadRecoveryOptions(hwndDlg, pStartInfo);
            SetRecoveryTimeout(hwndDlg, 0);
            return LoadOSList(hwndDlg, pStartInfo);

        case WM_DESTROY:
            DeleteBootRecords(hwndDlg);
            HeapFree(GetProcessHeap(), 0, pStartInfo);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_STRRECEDIT:
                    ShellExecuteW(0, L"open", L"notepad", pStartInfo->szFreeldrIni, NULL, SW_SHOWNORMAL);
                    // FIXME use CreateProcess and wait untill finished
                    //  DeleteBootRecords(hwndDlg);
                    //  LoadOSList(hwndDlg);
                    break;

                case IDOK:
                    /* save timeout */
                    if (SendDlgItemMessage(hwndDlg, IDC_STRECLIST, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED)
                        iTimeout = SendDlgItemMessage(hwndDlg, IDC_STRRECLISTUPDWN, UDM_GETPOS, (WPARAM)0, (LPARAM)0);
                    else
                        iTimeout = 0;
                    swprintf(szTimeout, L"%i", iTimeout);

                    lResult = SendDlgItemMessageW(hwndDlg, IDC_STRECOSCOMBO, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                    if (lResult == CB_ERR)
                    {
                        /* ? */
                        DeleteBootRecords(hwndDlg);
                        return TRUE;
                    }

                    pRecord = (PBOOTRECORD) SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_GETITEMDATA, (WPARAM)lResult, (LPARAM)0);

                    if ((INT_PTR)pRecord != CB_ERR)
                    {
                        if (pStartInfo->iFreeLdrIni == 1) // FreeLdrIni style
                        {
                            /* set default timeout */
                            WritePrivateProfileStringW(L"FREELOADER",
                                                      L"TimeOut",
                                                      szTimeout,
                                                      pStartInfo->szFreeldrIni);
                            /* set default os */
                            WritePrivateProfileStringW(L"FREELOADER",
                                                      L"DefaultOS",
                                                      pRecord->szSectionName,
                                                      pStartInfo->szFreeldrIni);

                        }
                        else if (pStartInfo->iFreeLdrIni == 2) // BootIni style
                        {
                            /* set default timeout */
                            WritePrivateProfileStringW(L"boot loader",
                                                      L"timeout",
                                                      szTimeout,
                                                      pStartInfo->szFreeldrIni);
                            /* set default os */
                            WritePrivateProfileStringW(L"boot loader",
                                                      L"default",
                                                      pRecord->szBootPath,
                                                      pStartInfo->szFreeldrIni);

                        }
                    }

                    WriteStartupRecoveryOptions(hwndDlg, pStartInfo);
                    EndDialog(hwndDlg,
                              LOWORD(wParam));
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg,
                              LOWORD(wParam));
                    return TRUE;

                case IDC_STRECLIST:
                    if (SendDlgItemMessage(hwndDlg, IDC_STRECLIST, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED)
                        SetTimeout(hwndDlg, 30);
                    else
                        SetTimeout(hwndDlg, 0);
                    break;

                case IDC_STRRECREC:
                    if (SendDlgItemMessage(hwndDlg, IDC_STRRECREC, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED)
                        SetRecoveryTimeout(hwndDlg, 30);
                    else
                        SetRecoveryTimeout(hwndDlg, 0);
                    break;

                case IDC_STRRECDEBUGCOMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        LRESULT lResult;

                        lResult = SendDlgItemMessage(hwndDlg, IDC_STRRECDEBUGCOMBO, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                        if (lResult != CB_ERR && lResult != (LRESULT)pStartInfo->dwCrashDumpEnabled)
                        {
                            if (pStartInfo->dwCrashDumpEnabled == 1 || pStartInfo->dwCrashDumpEnabled == 2)
                            {
                                SendDlgItemMessageW(hwndDlg, IDC_STRRECDUMPFILE, WM_GETTEXT, (WPARAM)sizeof(pStartInfo->szDumpFile) / sizeof(WCHAR), (LPARAM)pStartInfo->szDumpFile);
                            }
                            else if (pStartInfo->dwCrashDumpEnabled == 3)
                            {
                                SendDlgItemMessageW(hwndDlg, IDC_STRRECDUMPFILE, WM_GETTEXT, (WPARAM)sizeof(pStartInfo->szMinidumpDir) / sizeof(WCHAR), (LPARAM)pStartInfo->szMinidumpDir);
                            }

                            pStartInfo->dwCrashDumpEnabled = (DWORD)lResult;
                            SetCrashDlgItems(hwndDlg, pStartInfo);
                        }
                    }
                    break;
            }
            break;
    }

    return FALSE;
}
