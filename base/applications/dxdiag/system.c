/*
 * PROJECT:     ReactX Diagnosis Application
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/dxdiag/system.c
 * PURPOSE:     ReactX diagnosis system page
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald
 *
 */

#include "precomp.h"

typedef BOOL (WINAPI *ISWOW64PROC) (HANDLE, PBOOL);

BOOL
GetRegValue(HKEY hBaseKey, LPWSTR SubKey, LPWSTR ValueName, DWORD Type, LPWSTR Result, DWORD Size)
{
    HKEY hKey;
    LONG res;
    DWORD dwType;
    DWORD dwSize;

    if (RegOpenKeyExW(hBaseKey, SubKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return FALSE;

    dwSize = Size;
    res = RegQueryValueExW(hKey, ValueName, NULL, &dwType, (LPBYTE)Result, &dwSize);
    RegCloseKey(hKey);

    if (res != ERROR_SUCCESS)
        return FALSE;

    if (dwType != Type)
        return FALSE;

    if (Size == sizeof(DWORD))
        return TRUE;

    Result[(Size / sizeof(WCHAR))-1] = L'\0';
    return TRUE;
}


static
BOOL
GetDirectXVersion(WCHAR * szBuffer)
{
    WCHAR szVer[20];

    if (!GetRegValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\DirectX", L"Version", REG_SZ, szVer, sizeof(szVer)))
        return FALSE;

    if(!wcscmp(szVer, L"4.02.0095"))
        wcscpy(szBuffer, L"1.0");
    else if (!wcscmp(szVer, L"4.03.00.1096"))
        wcscpy(szBuffer, L"2.0");
    else if (!wcscmp(szVer, L"4.04.0068"))
        wcscpy(szBuffer, L"3.0");
    else if (!wcscmp(szVer, L"4.04.0069"))
        wcscpy(szBuffer, L"3.0");
    else if (!wcscmp(szVer, L"4.05.00.0155"))
        wcscpy(szBuffer, L"5.0");
    else if (!wcscmp(szVer, L"4.05.01.1721"))
        wcscpy(szBuffer, L"5.0");
    else if (!wcscmp(szVer, L"4.05.01.1998"))
        wcscpy(szBuffer, L"5.0");
    else if (!wcscmp(szVer, L"4.06.02.0436"))
        wcscpy(szBuffer, L"6.0");
    else if (!wcscmp(szVer, L"4.07.00.0700"))
        wcscpy(szBuffer, L"7.0");
    else if (!wcscmp(szVer, L"4.07.00.0716"))
        wcscpy(szBuffer, L"7.0a");
    else if (!wcscmp(szVer, L"4.08.00.0400"))
        wcscpy(szBuffer, L"8.0");
    else if (!wcscmp(szVer, L"4.08.01.0881"))
        wcscpy(szBuffer, L"8.1");
    else if (!wcscmp(szVer, L"4.08.01.0810"))
        wcscpy(szBuffer, L"8.1");
    else if (!wcscmp(szVer, L"4.09.0000.0900"))
        wcscpy(szBuffer, L"9.0");
    else if (!wcscmp(szVer, L"4.09.00.0900"))
        wcscpy(szBuffer, L"9.0");
    else if (!wcscmp(szVer, L"4.09.0000.0901"))
        wcscpy(szBuffer, L"9.0a");
    else if (!wcscmp(szVer, L"4.09.00.0901"))
        wcscpy(szBuffer, L"9.0a");
    else if (!wcscmp(szVer, L"4.09.0000.0902"))
        wcscpy(szBuffer, L"9.0b");
    else if (!wcscmp(szVer, L"4.09.00.0902"))
        wcscpy(szBuffer, L"9.0b");
    else if (!wcscmp(szVer, L"4.09.00.0904"))
        wcscpy(szBuffer, L"9.0c");
    else if (!wcscmp(szVer, L"4.09.0000.0904"))
        wcscpy(szBuffer, L"9.0c");
    else
        return FALSE;

    return TRUE;
}

VOID GetSystemCPU(WCHAR *szBuffer)
{
    SYSTEM_INFO archInfo;
    ISWOW64PROC fnIsWow64Process;
    BOOL isWow64 = FALSE;

    /* Find out if the program is running through WOW64 or not. Apparently,
    IsWow64Process() is not available on all versions of Windows, so the function
    has to be imported at runtime. If the function cannot be found, then assume
    the program is not running in WOW64. */
    fnIsWow64Process = (ISWOW64PROC)GetProcAddress(
        GetModuleHandleW(L"kernel32"), "IsWow64Process");

    if (fnIsWow64Process != NULL)
        fnIsWow64Process(GetCurrentProcess(), &isWow64);

    /* If the program is compiled as 32-bit, but is running in WOW64, it will
    automatically report as 32-bit regardless of the actual system architecture.
    It detects whether or not the program is using WOW64 or not, and then
    uses GetNativeSystemInfo(). If it is, it will properly report the actual
    system architecture to the user. */
    if (isWow64)
        GetNativeSystemInfo(&archInfo);
    else
        GetSystemInfo(&archInfo);

    /* Now check to see what the system architecture is */
    if(archInfo.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_UNKNOWN)
    {
        switch(archInfo.wProcessorArchitecture)
        {
        case PROCESSOR_ARCHITECTURE_INTEL:
        {
            wsprintfW(szBuffer, L"32-bit");
            break;
        }
        case PROCESSOR_ARCHITECTURE_AMD64:
        {
            wsprintfW(szBuffer, L"64-bit");
            break;
        }
        case PROCESSOR_ARCHITECTURE_IA64:
        {
            wsprintfW(szBuffer, L"Itanium");
            break;
        }
        case PROCESSOR_ARCHITECTURE_ARM:
        {
            wsprintfW(szBuffer, L"ARM");
            break;
        }
        default:break;
        }
    }
}

static
SIZE_T
GetBIOSValue(
    BOOL UseSMBios,
    PCHAR DmiString,
    LPWSTR RegValue,
    PVOID pBuf,
    DWORD cchBuf,
    BOOL bTrim)
{
    SIZE_T Length = 0;
    BOOL Result;

    if (UseSMBios)
    {
        Length = GetSMBiosStringW(DmiString, pBuf, cchBuf, bTrim);
    }
    if (Length == 0)
    {
        Result = GetRegValue(HKEY_LOCAL_MACHINE, L"Hardware\\Description\\System\\BIOS", RegValue, REG_SZ, pBuf, cchBuf * sizeof(WCHAR));
        if (Result)
        {
            Length = wcslen(pBuf);
        }
    }
    return Length;
}

static
VOID
InitializeSystemPage(HWND hwndDlg)
{
    WCHAR szTime[200];
    WCHAR szOSName[50];
    DWORD Length;
    DWORDLONG AvailableBytes, UsedBytes;
    MEMORYSTATUSEX mem;
    WCHAR szFormat[50];
    WCHAR szDesc[50];
    SYSTEM_INFO SysInfo;
    OSVERSIONINFO VersionInfo;
    PVOID SMBiosBuf;
    PCHAR DmiStrings[ID_STRINGS_MAX] = { 0 };
    BOOL Result;

    /* set date/time */
    szTime[0] = L'\0';
    Length = GetDateFormat(LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE, NULL, NULL, szTime, _countof(szTime));
    if (Length)
    {
        szTime[Length-1] = L',';
        szTime[Length++] = L' ';
    }
    Length = GetTimeFormatW(LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT|LOCALE_NOUSEROVERRIDE, NULL, NULL, szTime + Length, _countof(szTime));
    szTime[_countof(szTime)-1] = L'\0';
    SendDlgItemMessageW(hwndDlg, IDC_STATIC_TIME, WM_SETTEXT, 0, (LPARAM)szTime);

    /* set computer name */
    szTime[0] = L'\0';
    Length = _countof(szTime);
    if (GetComputerNameW(szTime, &Length))
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_COMPUTER, WM_SETTEXT, 0, (LPARAM)szTime);

    /* set product name */
    if (GetRegValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ProductName", REG_SZ, szOSName, sizeof(szOSName)))
    {
        if (LoadStringW(hInst, IDS_OS_VERSION, szFormat, _countof(szFormat)))
        {
            WCHAR szCpuName[50];

            ZeroMemory(&VersionInfo, sizeof(OSVERSIONINFO));
            VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

            GetSystemCPU(szCpuName);

            if (GetVersionEx(&VersionInfo))
            {
                szTime[_countof(szTime)-1] = L'\0';
                wsprintfW(szTime, szFormat, szOSName, szCpuName, VersionInfo.dwMajorVersion, VersionInfo.dwMinorVersion, VersionInfo.dwBuildNumber);
                SendDlgItemMessageW(hwndDlg, IDC_STATIC_OS, WM_SETTEXT, 0, (LPARAM)szTime);
            }
            else
            {
                /* If the version of the OS cannot be retrieved for some reason, then just give the OS Name and Architecture */
                szTime[_countof(szTime)-1] = L'\0';
                wsprintfW(szTime, L"%s %s", szOSName, szCpuName);
                SendDlgItemMessageW(hwndDlg, IDC_STATIC_OS, WM_SETTEXT, 0, (LPARAM)szTime);
            }
        }
    }
    else
    {
        if (LoadStringW(hInst, IDS_VERSION_UNKNOWN, szTime, _countof(szTime)))
        {
            szTime[_countof(szTime)-1] = L'\0';
            SendDlgItemMessage(hwndDlg, IDC_STATIC_VERSION, WM_SETTEXT, 0, (LPARAM)szTime);
        }
    }

    /* FIXME set product language/local language */
    if (GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SLANGUAGE, szTime, _countof(szTime)))
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_LANG, WM_SETTEXT, 0, (LPARAM)szTime);

    /* prepare SMBIOS data */
    SMBiosBuf = LoadSMBiosData(DmiStrings);

    /* set system manufacturer */
    szTime[0] = L'\0';
    Length = GetBIOSValue(SMBiosBuf != NULL,
                          DmiStrings[SYS_VENDOR],
                          L"SystemManufacturer",
                          szTime, _countof(szTime), FALSE);
    if (Length > 0)
    {
        szTime[_countof(szTime)-1] = L'\0';
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_MANU, WM_SETTEXT, 0, (LPARAM)szTime);
    }

    /* set motherboard model */
    szTime[0] = L'\0';
    Length = GetBIOSValue(SMBiosBuf != NULL,
                          DmiStrings[SYS_PRODUCT],
                          L"SystemProductName",
                          szTime, _countof(szTime), FALSE);
    if (Length > 0)
    {
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_MODEL, WM_SETTEXT, 0, (LPARAM)szTime);
    }

    /* set bios model */
    szTime[0] = L'\0';
    Length = GetBIOSValue(SMBiosBuf != NULL,
                          DmiStrings[BIOS_VENDOR],
                          L"BIOSVendor",
                          szTime, _countof(szTime), TRUE);
    if (Length > 0)
    {
        DWORD Index;
        DWORD StrLength = _countof(szTime);

        Index = wcslen(szTime);
        if (Index + 1 < _countof(szTime))
        {
            szTime[Index++] = L' ';
            szTime[Index] = L'\0';
        }
        StrLength -= Index;

        Length = GetBIOSValue(SMBiosBuf != NULL,
                              DmiStrings[BIOS_DATE],
                              L"BIOSReleaseDate",
                              szTime + Index, StrLength, TRUE);
        if (Length > 0)
        {
            if (Index + StrLength > _countof(szTime) - 15)
            {
                //FIXME  retrieve BiosMajorRelease, BiosMinorRelease
                //StrLength = wcslen(szTime + Index);
                //szTime[Index+StrLength] = L' ';
                //wcscpy(szTime + Index + StrLength, L"Ver: "); //FIXME NON-NLS
                //szTime[_countof(szTime)-1] = L'\0';
            }
            SendDlgItemMessageW(hwndDlg, IDC_STATIC_BIOS, WM_SETTEXT, 0, (LPARAM)szTime);
        }
    }

    /* clean SMBIOS data */
    FreeSMBiosData(SMBiosBuf);

    /* set processor string */
    Result = GetRegValue(HKEY_LOCAL_MACHINE, L"Hardware\\Description\\System\\CentralProcessor\\0", L"ProcessorNameString", REG_SZ, szDesc, sizeof(szDesc));
    if (!Result)
    {
        /* Processor Brand String not found */
        /* FIXME: Implement CPU name detection routine */

        /* Finally try to use Identifier string */
        Result = GetRegValue(HKEY_LOCAL_MACHINE, L"Hardware\\Description\\System\\CentralProcessor\\0", L"Identifier", REG_SZ, szDesc, sizeof(szDesc));
    }
    if (Result)
    {
        TrimDmiStringW(szDesc);
        /* FIXME retrieve current speed */
        szFormat[0] = L'\0';
        GetSystemInfo(&SysInfo);
        if (SysInfo.dwNumberOfProcessors > 1)
            LoadStringW(hInst, IDS_FORMAT_MPPROC, szFormat, _countof(szFormat));
        else
            LoadStringW(hInst, IDS_FORMAT_UNIPROC, szFormat, _countof(szFormat));

        szFormat[_countof(szFormat)-1] = L'\0';
        wsprintfW(szTime, szFormat, szDesc, SysInfo.dwNumberOfProcessors);
        SendDlgItemMessageW(hwndDlg, IDC_STATIC_PROC, WM_SETTEXT, 0, (LPARAM)szTime);
    }

    /* retrieve available memory */
    ZeroMemory(&mem, sizeof(mem));
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem))
    {
        if (LoadStringW(hInst, IDS_FORMAT_MB, szFormat, _countof(szFormat)))
        {
            /* set total mem string */
            szFormat[_countof(szFormat)-1] = L'\0';
            wsprintfW(szTime, szFormat, (mem.ullTotalPhys/1048576));
            SendDlgItemMessageW(hwndDlg, IDC_STATIC_MEM, WM_SETTEXT, 0, (LPARAM)szTime);
        }

        if (LoadStringW(hInst, IDS_FORMAT_SWAP, szFormat, _countof(szFormat)))
        {
            /* set swap string */
            AvailableBytes = (mem.ullTotalPageFile-mem.ullTotalPhys)/1048576;
            UsedBytes = (mem.ullTotalPageFile-mem.ullAvailPageFile)/1048576;

            szFormat[_countof(szFormat)-1] = L'\0';
            wsprintfW(szTime, szFormat, (UsedBytes), (AvailableBytes));
            SendDlgItemMessageW(hwndDlg, IDC_STATIC_SWAP, WM_SETTEXT, 0, (LPARAM)szTime);
        }
    }
    /* set directx version string */
    wcscpy(szTime, L"ReactX ");
    if (GetDirectXVersion(szTime + 7))
    {
        SendDlgItemMessage(hwndDlg, IDC_STATIC_VERSION, WM_SETTEXT, 0, (LPARAM)szTime);
    }
    else
    {
        if (LoadStringW(hInst, IDS_VERSION_UNKNOWN, szTime, _countof(szTime)))
        {
            szTime[_countof(szTime)-1] = L'\0';
            SendDlgItemMessage(hwndDlg, IDC_STATIC_VERSION, WM_SETTEXT, 0, (LPARAM)szTime);
        }
    }
}


INT_PTR CALLBACK
SystemPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            InitializeSystemPage(hDlg);
            return TRUE;
        }
    }

    return FALSE;
}
