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
    TCHAR szFreeldrIni[MAX_PATH + 15];
    TCHAR szDumpFile[MAX_PATH];
    TCHAR szMinidumpDir[MAX_PATH];
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
    SendDlgItemMessage(hwndDlg, IDC_STRRECLISTUPDWN, UDM_SETRANGE, (WPARAM) 0, (LPARAM) MAKELONG((short) 999, 0));
    SendDlgItemMessage(hwndDlg, IDC_STRRECLISTUPDWN, UDM_SETPOS, (WPARAM) 0, (LPARAM) MAKELONG((short) Timeout, 0));
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
    SendDlgItemMessage(hwndDlg, IDC_STRRECRECUPDWN, UDM_SETRANGE, (WPARAM) 0, (LPARAM) MAKELONG((short) 999, 0));
    SendDlgItemMessage(hwndDlg, IDC_STRRECRECUPDWN, UDM_SETPOS, (WPARAM) 0, (LPARAM) MAKELONG((short) Timeout, 0));
}


static DWORD
GetSystemDrive(TCHAR **szSystemDrive)
{
    DWORD dwBufSize;

    /* get Path to freeldr.ini or boot.ini */
    *szSystemDrive = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(TCHAR));
    if (szSystemDrive != NULL)
    {
        dwBufSize = GetEnvironmentVariable(_T("SystemDrive"), *szSystemDrive, MAX_PATH);
        if (dwBufSize > MAX_PATH)
        {
            TCHAR *szTmp;
            DWORD dwBufSize2;

            szTmp = HeapReAlloc(GetProcessHeap(), 0, *szSystemDrive, dwBufSize * sizeof(TCHAR));
            if (szTmp == NULL)
                goto FailGetSysDrive;

            *szSystemDrive = szTmp;

            dwBufSize2 = GetEnvironmentVariable(_T("SystemDrive"), *szSystemDrive, dwBufSize);
            if (dwBufSize2 > dwBufSize || dwBufSize2 == 0)
                goto FailGetSysDrive;
        }
        else if (dwBufSize == 0)
        {
FailGetSysDrive:
            HeapFree(GetProcessHeap(), 0, szSystemDrive);
            *szSystemDrive = NULL;
            return FALSE;
        }

        return dwBufSize;
    }

    return FALSE;
}

static PBOOTRECORD
ReadFreeldrSection(HINF hInf, TCHAR *szSectionName)
{
    PBOOTRECORD pRecord;
    INFCONTEXT InfContext;
    TCHAR szName[MAX_PATH];
    TCHAR szValue[MAX_PATH];
    DWORD LineLength;

    if (!SetupFindFirstLine(hInf,
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

    _tcscpy(pRecord->szSectionName, szSectionName);

    do
    {
        if (!SetupGetStringField(&InfContext,
                                  0,
                                  szName,
                                  sizeof(szName) / sizeof(TCHAR),
                                  &LineLength))
        {
            break;
        }

        if (!SetupGetStringField(&InfContext,
                                  1,
                                  szValue,
                                  sizeof(szValue) / sizeof(TCHAR),
                                  &LineLength))
        {
            break;
        }

        if (!_tcsnicmp(szName, _T("BootType"), 8))
        {
            if (!_tcsnicmp(szValue, _T("ReactOS"), 7))
            {
                //FIXME store as enum
                pRecord->BootType = 1;
            }
            else
            {
                pRecord->BootType = 0;
            }
        }
        else if (!_tcsnicmp(szName, _T("SystemPath"), 10))
        {
            _tcscpy(pRecord->szBootPath, szValue);
        }
        else if (!_tcsnicmp(szName, _T("Options"), 7))
        {
            //FIXME store flags as values
            _tcscpy(pRecord->szOptions, szValue);
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
    TCHAR szDefaultOs[MAX_PATH];
    TCHAR szName[MAX_PATH];
    TCHAR szValue[MAX_PATH];
    DWORD LineLength;
    DWORD TimeOut;
    LRESULT lResult;

    if (!SetupFindFirstLine(hInf,
                           _T("FREELOADER"),
                           _T("DefaultOS"),
                           &InfContext))
    {
        /* failed to find default os */
        return FALSE;
    }

    if (!SetupGetStringField(&InfContext,
                             1,
                             szDefaultOs,
                             sizeof(szDefaultOs) / sizeof(TCHAR),
                             &LineLength))
    {
        /* no key */
        return FALSE;
    }

    if (!SetupFindFirstLine(hInf,
                           _T("FREELOADER"),
                           _T("TimeOut"),
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

    if (!SetupFindFirstLine(hInf,
                           _T("Operating Systems"),
                           NULL,
                           &InfContext))
    {
       /* expected list of operating systems */
       return FALSE;
    }

    do
    {
        if (!SetupGetStringField(&InfContext,
                                 0,
                                 szName,
                                 sizeof(szName) / sizeof(TCHAR),
                                 &LineLength))
        {
            /* the ini file is messed up */
            return FALSE;
        }

        if (!SetupGetStringField(&InfContext,
                                 1,
                                 szValue,
                                 sizeof(szValue) / sizeof(TCHAR),
                                 &LineLength))
        {
            /* the ini file is messed up */
            return FALSE;
        }

        pRecord = ReadFreeldrSection(hInf, szName);
        if (pRecord)
        {
            lResult = SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM)szValue);
            if (lResult != CB_ERR)
            {
                SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)pRecord);
                if (!_tcscmp(szDefaultOs, szName))
                {
                    /* we store the friendly name as key */
                    _tcscpy(szDefaultOs, szValue);
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
    lResult = SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_FINDSTRING, (WPARAM)-1, (LPARAM)szDefaultOs);
    if (lResult != CB_ERR)
    {
       /* set cur sel */
       SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_SETCURSEL, (WPARAM)lResult, (LPARAM)0);
    }

    if(TimeOut)
    {
        SendDlgItemMessage(hwndDlg, IDC_STRECLIST, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
    }

    SetTimeout(hwndDlg, TimeOut);

    return TRUE;
}

static INT
LoadBootSettings(HINF hInf, HWND hwndDlg)
{
    INFCONTEXT InfContext;
    TCHAR szName[MAX_PATH];
    TCHAR szValue[MAX_PATH];
    DWORD LineLength;
    DWORD TimeOut = 0;
    TCHAR szDefaultOS[MAX_PATH];
    TCHAR szOptions[MAX_PATH];
    PBOOTRECORD pRecord;
    LRESULT lResult;

    if(!SetupFindFirstLine(hInf,
                           _T("boot loader"),
                           NULL,
                           &InfContext))
    {
        return FALSE;
    }

    do
    {
        if (!SetupGetStringField(&InfContext,
                                 0,
                                 szName,
                                 sizeof(szName) / sizeof(TCHAR),
                                 &LineLength))
        {
            return FALSE;
        }

        if (!SetupGetStringField(&InfContext,
                                 1,
                                 szValue,
                                 sizeof(szValue) / sizeof(TCHAR),
                                 &LineLength))
        {
            return FALSE;
        }

        if (!_tcsnicmp(szName, _T("timeout"), 7))
        {
            TimeOut = _ttoi(szValue);
        }

        if (!_tcsnicmp(szName, _T("default"), 7))
        {
            _tcscpy(szDefaultOS, szValue);
        }

    }
    while (SetupFindNextLine(&InfContext, &InfContext));

    if (!SetupFindFirstLine(hInf,
                            _T("operating systems"),
                            NULL,
                            &InfContext))
    {
        /* failed to find operating systems section */
        return FALSE;
    }

    do
    {
        if (!SetupGetStringField(&InfContext,
                                 0,
                                 szName,
                                 sizeof(szName) / sizeof(TCHAR),
                                 &LineLength))
        {
            return FALSE;
        }

        if (!SetupGetStringField(&InfContext,
                                 1,
                                 szValue,
                                 sizeof(szValue) / sizeof(TCHAR),
                                 &LineLength))
        {
            return FALSE;
        }

        SetupGetStringField(&InfContext,
                            2,
                            szOptions,
                            sizeof(szOptions) / sizeof(TCHAR),
                            &LineLength);

        pRecord = (PBOOTRECORD) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BOOTRECORD));
        if (pRecord)
        {
            pRecord->BootType = 0;
            _tcscpy(pRecord->szBootPath, szName);
            _tcscpy(pRecord->szSectionName, szValue);
            _tcscpy(pRecord->szOptions, szOptions);

            if (!_tcscmp(szName, szDefaultOS))
            {
                /* ms boot ini stores the path not the friendly name */
                _tcscpy(szDefaultOS, szValue);
            }

            lResult = SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM)szValue);
            if (lResult != CB_ERR)
            {
                SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)pRecord);
            }
            else
            {
               HeapFree(GetProcessHeap(), 0, pRecord);
            }
        }

    }
    while (SetupFindNextLine(&InfContext, &InfContext));

    /* find default os in list */
    lResult = SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_FINDSTRING, (WPARAM)0, (LPARAM)szDefaultOS);
    if (lResult != CB_ERR)
    {
       /* set cur sel */
       SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_SETCURSEL, (WPARAM)lResult, (LPARAM)0);
    }

    if(TimeOut)
    {
        SendDlgItemMessage(hwndDlg, IDC_STRECLIST, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
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

    lIndex = SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
    if (lIndex == CB_ERR)
        return;

    for (index = 0; index <lIndex; index++)
    {
        pRecord = (PBOOTRECORD) SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_GETITEMDATA, (WPARAM)index, (LPARAM)0);
        if ((INT)pRecord != CB_ERR)
        {
            HeapFree(GetProcessHeap(), 0, pRecord);
        }
    }

    SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
}

static LRESULT
LoadOSList(HWND hwndDlg, PSTARTINFO pStartInfo)
{
    DWORD dwBufSize;
    TCHAR *szSystemDrive;
    HINF hInf;

    dwBufSize = GetSystemDrive(&szSystemDrive);
    if (!dwBufSize)
        return FALSE;

    _tcscpy(pStartInfo->szFreeldrIni, szSystemDrive);
    _tcscat(pStartInfo->szFreeldrIni, _T("\\freeldr.ini"));

    if (PathFileExists(pStartInfo->szFreeldrIni))
    {
        /* freeldr.ini exists */
        hInf = SetupOpenInfFile(pStartInfo->szFreeldrIni,
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
    _tcscpy(pStartInfo->szFreeldrIni, szSystemDrive);
    _tcscat(pStartInfo->szFreeldrIni, _T("\\boot.ini"));

    if (PathFileExists(pStartInfo->szFreeldrIni))
    {
        /* load boot.ini settings */
        hInf = SetupOpenInfFile(pStartInfo->szFreeldrIni,
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
        SendMessage(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), WM_SETTEXT, (WPARAM)0, (LPARAM)pStartInfo->szMinidumpDir);
    }
    else if (pStartInfo->dwCrashDumpEnabled == 1 || pStartInfo->dwCrashDumpEnabled == 2)
    {
        /* kernel or complete dump */
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECOVERWRITE), TRUE);
        SendMessage(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), WM_SETTEXT, (WPARAM)0, (LPARAM)pStartInfo->szDumpFile);
    }
    SendDlgItemMessage(hwnd, IDC_STRRECDEBUGCOMBO, CB_SETCURSEL, (WPARAM)pStartInfo->dwCrashDumpEnabled, (LPARAM)0);
}

static VOID
WriteStartupRecoveryOptions(HWND hwndDlg, PSTARTINFO pStartInfo)
{
    HKEY hKey;
    DWORD lResult;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     _T("System\\CurrentControlSet\\Control\\CrashControl"),
                     0,
                     KEY_WRITE,
                     &hKey) != ERROR_SUCCESS)
    {
        /* failed to open key */
        return;
    }

    lResult = (DWORD) SendDlgItemMessage(hwndDlg, IDC_STRRECWRITEEVENT, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
    RegSetValueEx(hKey, _T("LogEvent"), 0, REG_DWORD, (LPBYTE)&lResult, sizeof(lResult));

    lResult = (DWORD) SendDlgItemMessage(hwndDlg, IDC_STRRECSENDALERT, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
    RegSetValueEx(hKey, _T("SendAlert"), 0, REG_DWORD, (LPBYTE)&lResult, sizeof(lResult));

    lResult = (DWORD) SendDlgItemMessage(hwndDlg, IDC_STRRECRESTART, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
    RegSetValueEx(hKey, _T("AutoReboot"), 0, REG_DWORD, (LPBYTE)&lResult, sizeof(lResult));

    lResult = (DWORD) SendDlgItemMessage(hwndDlg, IDC_STRRECOVERWRITE, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
    RegSetValueEx(hKey, _T("Overwrite"), 0, REG_DWORD, (LPBYTE)&lResult, sizeof(lResult));


    if (pStartInfo->dwCrashDumpEnabled == 1 || pStartInfo->dwCrashDumpEnabled == 2)
    {
        SendDlgItemMessage(hwndDlg, IDC_STRRECDUMPFILE, WM_GETTEXT, (WPARAM)sizeof(pStartInfo->szDumpFile) / sizeof(TCHAR), (LPARAM)pStartInfo->szDumpFile);
        RegSetValueEx(hKey, _T("DumpFile"), 0, REG_EXPAND_SZ, (LPBYTE)pStartInfo->szDumpFile, (_tcslen(pStartInfo->szDumpFile) + 1) * sizeof(TCHAR));
    }
    else if (pStartInfo->dwCrashDumpEnabled == 3)
    {
        SendDlgItemMessage(hwndDlg, IDC_STRRECDUMPFILE, WM_GETTEXT, (WPARAM)sizeof(pStartInfo->szDumpFile) / sizeof(TCHAR), (LPARAM)pStartInfo->szDumpFile);
        RegSetValueEx(hKey, _T("MinidumpDir"), 0, REG_EXPAND_SZ, (LPBYTE)pStartInfo->szDumpFile, (_tcslen(pStartInfo->szDumpFile) + 1) * sizeof(TCHAR));
    }

    RegSetValueEx(hKey, _T("CrashDumpEnabled"), 0, REG_DWORD, (LPBYTE)pStartInfo->dwCrashDumpEnabled, sizeof(pStartInfo->dwCrashDumpEnabled));
    RegCloseKey(hKey);
}

static VOID
LoadRecoveryOptions(HWND hwndDlg, PSTARTINFO pStartInfo)
{
    HKEY hKey;
    DWORD dwValues;
    TCHAR szName[MAX_PATH];
    TCHAR szValue[MAX_PATH];
    DWORD i, dwName, dwValue, dwValueLength, dwType;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     _T("System\\CurrentControlSet\\Control\\CrashControl"),
                     0,
                     KEY_READ,
                     &hKey) != ERROR_SUCCESS)
    {
        /* failed to open key */
        return;
    }

    if (RegQueryInfoKey(hKey,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwValues,
                        NULL,
                        NULL,
                        NULL,
                        NULL) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return;
    }

    for (i = 0; i < dwValues; i++)
    {
        dwName = sizeof(szName) / sizeof(TCHAR);

        RegEnumValue(hKey, i, szName, &dwName, NULL, &dwType, NULL, NULL);
        if (dwType == REG_DWORD)
        {
            dwValueLength = sizeof(dwValue);
            dwName = sizeof(szName) / sizeof(TCHAR);
            if (RegEnumValue(hKey, i, szName, &dwName, NULL, &dwType, (LPBYTE)&dwValue, &dwValueLength) != ERROR_SUCCESS)
                continue;
        }
        else
        {
            dwValueLength = sizeof(szValue);
            dwName = sizeof(szName) / sizeof(TCHAR);
            if (RegEnumValue(hKey, i, szName, &dwName, NULL, &dwType, (LPBYTE)&szValue, &dwValueLength) != ERROR_SUCCESS)
                continue;
        }

        if (!_tcscmp(szName, _T("LogEvent")))
        {
            if (dwValue)
                SendDlgItemMessage(hwndDlg, IDC_STRRECWRITEEVENT, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
        }
        else if (!_tcscmp(szName, _T("SendAlert")))
        {
            if (dwValue)
                SendDlgItemMessage(hwndDlg, IDC_STRRECSENDALERT, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
        }
        else if (!_tcscmp(szName, _T("AutoReboot")))
        {
            if (dwValue)
                SendDlgItemMessage(hwndDlg, IDC_STRRECRESTART, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
        }
        else if (!_tcscmp(szName, _T("Overwrite")))
        {
            if (dwValue)
                SendDlgItemMessage(hwndDlg, IDC_STRRECOVERWRITE, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
        }
        else if (!_tcscmp(szName, _T("DumpFile")))
        {
            _tcscpy(pStartInfo->szDumpFile, szValue);
        }
        else if (!_tcscmp(szName, _T("MinidumpDir")))
        {
            _tcscpy(pStartInfo->szMinidumpDir, szValue);
        }
        else if (!_tcscmp(szName, _T("CrashDumpEnabled")))
        {
            pStartInfo->dwCrashDumpEnabled = dwValue;
        }
    }

    if (LoadString(hApplet, IDS_NO_DUMP, szValue, sizeof(szValue) / sizeof(TCHAR)) < sizeof(szValue) / sizeof(TCHAR))
        SendDlgItemMessage(hwndDlg, IDC_STRRECDEBUGCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM) szValue);

    if (LoadString(hApplet, IDS_FULL_DUMP, szValue, sizeof(szValue) / sizeof(TCHAR)) < sizeof(szValue) / sizeof(TCHAR))
        SendDlgItemMessage(hwndDlg, IDC_STRRECDEBUGCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM) szValue);

    if (LoadString(hApplet, IDS_KERNEL_DUMP, szValue, sizeof(szValue) / sizeof(TCHAR)) < sizeof(szValue) / sizeof(TCHAR))
        SendDlgItemMessage(hwndDlg, IDC_STRRECDEBUGCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM) szValue);

    if (LoadString(hApplet, IDS_MINI_DUMP, szValue, sizeof(szValue) / sizeof(TCHAR)) < sizeof(szValue) / sizeof(TCHAR))
        SendDlgItemMessage(hwndDlg, IDC_STRRECDEBUGCOMBO, CB_ADDSTRING, (WPARAM)0, (LPARAM) szValue);

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
    TCHAR szTimeout[10];

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
                    ShellExecute(0, _T("open"), _T("notepad"), pStartInfo->szFreeldrIni, NULL, SW_SHOWNORMAL);
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
                    _stprintf(szTimeout, _T("%i"), iTimeout);

                    lResult = SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                    if (lResult == CB_ERR)
                    {
                        /* ? */
                        DeleteBootRecords(hwndDlg);
                        return TRUE;
                    }

                    pRecord = (PBOOTRECORD) SendDlgItemMessage(hwndDlg, IDC_STRECOSCOMBO, CB_GETITEMDATA, (WPARAM)lResult, (LPARAM)0);

                    if ((INT)pRecord != CB_ERR)
                    {
                        if (pStartInfo->iFreeLdrIni == 1) // FreeLdrIni style
                        {
                            /* set default timeout */
                            WritePrivateProfileString(_T("FREELOADER"),
                                                      _T("TimeOut"),
                                                      szTimeout,
                                                      pStartInfo->szFreeldrIni);
                            /* set default os */
                            WritePrivateProfileString(_T("FREELOADER"),
                                                      _T("DefaultOS"),
                                                      pRecord->szSectionName,
                                                      pStartInfo->szFreeldrIni);

                        }
                        else if (pStartInfo->iFreeLdrIni == 2) // BootIni style
                        {
                            /* set default timeout */
                            WritePrivateProfileString(_T("boot loader"),
                                                      _T("timeout"),
                                                      szTimeout,
                                                      pStartInfo->szFreeldrIni);
                            /* set default os */
                            WritePrivateProfileString(_T("boot loader"),
                                                      _T("default"),
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
                        if (lResult != CB_ERR && lResult != pStartInfo->dwCrashDumpEnabled)
                        {
                            if (pStartInfo->dwCrashDumpEnabled == 1 || pStartInfo->dwCrashDumpEnabled == 2)
                            {
                                SendDlgItemMessage(hwndDlg, IDC_STRRECDUMPFILE, WM_GETTEXT, (WPARAM)sizeof(pStartInfo->szDumpFile) / sizeof(TCHAR), (LPARAM)pStartInfo->szDumpFile);
                            }
                            else if (pStartInfo->dwCrashDumpEnabled == 3)
                            {
                                SendDlgItemMessage(hwndDlg, IDC_STRRECDUMPFILE, WM_GETTEXT, (WPARAM)sizeof(pStartInfo->szMinidumpDir) / sizeof(TCHAR), (LPARAM)pStartInfo->szMinidumpDir);
                            }

                            pStartInfo->dwCrashDumpEnabled = lResult;
                            SetCrashDlgItems(hwndDlg, pStartInfo);
                        }
                    }
                    break;
            }
            break;
    }

    return FALSE;
}
