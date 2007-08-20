
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
static TCHAR m_szFreeldrIni[MAX_PATH + 15];
static int m_FreeLdrIni = 0;
static TCHAR m_szDumpFile[MAX_PATH];
static TCHAR m_szMinidumpDir[MAX_PATH];
static DWORD m_dwCrashDumpEnabled = 0;

void SetTimeout(HWND hwndDlg, int Timeout)
{
    if (Timeout == 0)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_STRRECLISTUPDWN), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_STRRECLISTUPDWN), TRUE);
    }
    SendDlgItemMessage(hwndDlg, IDC_STRRECLISTUPDWN, UDM_SETPOS, (WPARAM) 0, (LPARAM) MAKELONG((short) Timeout, 0));
}

DWORD GetSystemDrive(TCHAR ** szSystemDrive)
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

PBOOTRECORD ReadFreeldrSection(HINF hInf, TCHAR * szSectionName)
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

    }while(SetupFindNextLine(&InfContext, &InfContext));

    return pRecord;
}


int LoadFreeldrSettings(HINF hInf, HWND hwndDlg)
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

    }while(SetupFindNextLine(&InfContext, &InfContext));

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

int LoadBootSettings(HINF hInf, HWND hwndDlg)
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

    }while(SetupFindNextLine(&InfContext, &InfContext));

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

    }while(SetupFindNextLine(&InfContext, &InfContext));
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

void DeleteBootRecords(HWND hwndDlg)
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

LRESULT LoadOSList(HWND hwndDlg)
{
    DWORD dwBufSize;
    TCHAR *szSystemDrive;
    HINF hInf;
       
    dwBufSize = GetSystemDrive(&szSystemDrive);
    if (!dwBufSize)
        return FALSE;
    

    _tcscpy(m_szFreeldrIni, szSystemDrive);
    _tcscat(m_szFreeldrIni, _T("\\freeldr.ini"));
    if (PathFileExists(m_szFreeldrIni))
    {
        /* freeldr.ini exists */
        hInf = SetupOpenInfFile(m_szFreeldrIni, 
                                NULL,
                                INF_STYLE_OLDNT,
                                NULL);

        if (hInf != INVALID_HANDLE_VALUE)
        {
            LoadFreeldrSettings(hInf, hwndDlg);
            SetupCloseInfFile(hInf);
            m_FreeLdrIni = 1;
            return TRUE;
        }
        return FALSE;
    }
    /* try load boot.ini settings */
    _tcscpy(m_szFreeldrIni, szSystemDrive);
    _tcscat(m_szFreeldrIni, _T("\\boot.ini"));

    if (PathFileExists(m_szFreeldrIni))
    {
        /* load boot.ini settings */
        hInf = SetupOpenInfFile(m_szFreeldrIni, 
                                NULL,
                                INF_STYLE_OLDNT,
                                NULL);

        if (hInf != INVALID_HANDLE_VALUE)
        {
            LoadBootSettings(hInf, hwndDlg);
            SetupCloseInfFile(hInf);
            m_FreeLdrIni = 2;
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

void SetCrashDlgItems(HWND hwnd)
{
    if (m_dwCrashDumpEnabled == 0)
    {
        /* no crash information required */
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECOVERWRITE), FALSE);
    }
    else if (m_dwCrashDumpEnabled == 3)
    {
        /* minidump type */
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECOVERWRITE), FALSE);
        SendMessage(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), WM_SETTEXT, (WPARAM)0, (LPARAM)m_szMinidumpDir);
    }
    else if (m_dwCrashDumpEnabled == 1 || m_dwCrashDumpEnabled == 2)
    {
        /* kernel or complete dump */
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_STRRECOVERWRITE), TRUE);
        SendMessage(GetDlgItem(hwnd, IDC_STRRECDUMPFILE), WM_SETTEXT, (WPARAM)0, (LPARAM)m_szDumpFile);
    }
    SendDlgItemMessage(hwnd, IDC_STRRECDEBUGCOMBO, CB_SETCURSEL, (WPARAM)m_dwCrashDumpEnabled, (LPARAM)0);
}

void WriteStartupRecoveryOptions(HWND hwndDlg)
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


    if (m_dwCrashDumpEnabled == 1 || m_dwCrashDumpEnabled == 2)
    {
        SendDlgItemMessage(hwndDlg, IDC_STRRECDUMPFILE, WM_GETTEXT, (WPARAM)sizeof(m_szDumpFile) / sizeof(TCHAR), (LPARAM)m_szDumpFile);
        RegSetValueEx(hKey, _T("DumpFile"), 0, REG_EXPAND_SZ, (LPBYTE)&m_szDumpFile, (_tcslen(m_szDumpFile) + 1) * sizeof(TCHAR));
    }
    else if (m_dwCrashDumpEnabled == 3)
    {
        SendDlgItemMessage(hwndDlg, IDC_STRRECDUMPFILE, WM_GETTEXT, (WPARAM)sizeof(m_szDumpFile) / sizeof(TCHAR), (LPARAM)m_szDumpFile);
        RegSetValueEx(hKey, _T("MinidumpDir"), 0, REG_EXPAND_SZ, (LPBYTE)&m_szDumpFile, (_tcslen(m_szDumpFile) + 1) * sizeof(TCHAR));
    }

    RegSetValueEx(hKey, _T("CrashDumpEnabled"), 0, REG_DWORD, (LPBYTE)&m_dwCrashDumpEnabled, sizeof(m_dwCrashDumpEnabled));
    RegCloseKey(hKey);
}

void LoadRecoveryOptions(HWND hwndDlg)
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
            _tcscpy(m_szDumpFile, szValue);
        }
        else if (!_tcscmp(szName, _T("MinidumpDir")))
        {
            _tcscpy(m_szMinidumpDir, szValue);
        }
        else if (!_tcscmp(szName, _T("CrashDumpEnabled")))
        {
            m_dwCrashDumpEnabled = dwValue;
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

    SetCrashDlgItems(hwndDlg);
    RegCloseKey(hKey);
}



/* Property page dialog callback */
INT_PTR CALLBACK
StartRecDlgProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    PBOOTRECORD pRecord;
    int iTimeout;
    LRESULT lResult;
    TCHAR szTimeout[10];

    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            LoadRecoveryOptions(hwndDlg);
            return LoadOSList(hwndDlg);
        }
        break;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_STRRECEDIT:
                {
                    ShellExecute(0, _T("open"), _T("notepad"), m_szFreeldrIni, NULL, SW_SHOWNORMAL);
                  // FIXME use CreateProcess and wait untill finished
                  //  DeleteBootRecords(hwndDlg);
                  //  LoadOSList(hwndDlg);
                    break;
                }	
                case IDOK:
                {
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
                        if (m_FreeLdrIni == 1) // FreeLdrIni style
                        {
                            /* set default timeout */
                            WritePrivateProfileString(_T("FREELOADER"),
                                                      _T("TimeOut"),
                                                      szTimeout,
                                                      m_szFreeldrIni);
                            /* set default os */
                            WritePrivateProfileString(_T("FREELOADER"),
                                                      _T("DefaultOS"),
                                                      pRecord->szSectionName,
                                                      m_szFreeldrIni);

                        }
                        else if (m_FreeLdrIni == 2) // BootIni style
                        {
                            /* set default timeout */
                            WritePrivateProfileString(_T("boot loader"),
                                                      _T("timeout"),
                                                      szTimeout,
                                                      m_szFreeldrIni);
                            /* set default os */
                            WritePrivateProfileString(_T("boot loader"),
                                                      _T("default"),
                                                      pRecord->szBootPath,
                                                      m_szFreeldrIni);

                        }
                    }
                    WriteStartupRecoveryOptions(hwndDlg);
                    DeleteBootRecords(hwndDlg);
                    EndDialog(hwndDlg,
                              LOWORD(wParam));
                    return TRUE;
                }
                case IDCANCEL:
                {
                    DeleteBootRecords(hwndDlg);
                    EndDialog(hwndDlg,
                              LOWORD(wParam));
                    return TRUE;
                }
                case IDC_STRECLIST:
                {
                    if (SendDlgItemMessage(hwndDlg, IDC_STRECLIST, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED)
                        SetTimeout(hwndDlg, 30);
                    else
                        SetTimeout(hwndDlg, 0);
                }
                case IDC_STRRECDEBUGCOMBO:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        LRESULT lResult;

                        lResult = SendDlgItemMessage(hwndDlg, IDC_STRRECDEBUGCOMBO, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                        if (lResult != CB_ERR && lResult != m_dwCrashDumpEnabled)
                        {
                            if (m_dwCrashDumpEnabled == 1 || m_dwCrashDumpEnabled == 2)
                            {
                                SendDlgItemMessage(hwndDlg, IDC_STRRECDUMPFILE, WM_GETTEXT, (WPARAM)sizeof(m_szDumpFile) / sizeof(TCHAR), (LPARAM)m_szDumpFile);
                            }
                            else if (m_dwCrashDumpEnabled == 3)
                            {
                                SendDlgItemMessage(hwndDlg, IDC_STRRECDUMPFILE, WM_GETTEXT, (WPARAM)sizeof(m_szMinidumpDir) / sizeof(TCHAR), (LPARAM)m_szMinidumpDir);
                            }
                            m_dwCrashDumpEnabled = lResult;
                            SetCrashDlgItems(hwndDlg);
                        }
                    }
                    break;
                }

            }
        }
        break;
    }
    return FALSE;
}
