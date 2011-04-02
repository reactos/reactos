/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/* Copyright (C) 2007, Dmitry Chapyshev <lentind@yandex.ru> */
/* Copyright (C) 2011, Rafal Harabien <rafalh1992@o2.pl> */

#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include <locale.h>
#include <lm.h>
#include <shlwapi.h>

#include "resource.h"

#define BUFFER_SIZE 32767

/* Load from resource and convert to OEM */
static
BOOL
GetOemStrings(UINT rcID, LPWSTR OutMsg)
{
    if (LoadStringW(GetModuleHandle(NULL), rcID, OutMsg, BUFFER_SIZE))
        return TRUE;

    return FALSE;
}

/* Load data from registry */
static
BOOL
RegGetSZ(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPWSTR Buf)
{
    DWORD dwBytes = BUFFER_SIZE*sizeof(WCHAR), dwType;
    BOOL bRet = TRUE;

    /* If SubKy is specified open it */
    if (lpSubKey && RegOpenKeyExW(hKey,
                                  lpSubKey,
                                  0,
                                  KEY_QUERY_VALUE,
                                  &hKey) != ERROR_SUCCESS)
    {
        wprintf(L"Warning! Cannot open %s. Last error: %lu.\n", lpSubKey, GetLastError());
        return FALSE;
    }

    if (RegQueryValueExW(hKey,
                         lpValueName,
                         NULL,
                         &dwType,
                         (LPBYTE)Buf,
                         &dwBytes) != ERROR_SUCCESS || (dwType != REG_SZ && dwType != REG_MULTI_SZ))
    {
        wprintf(L"Warning! Cannot query %s. Last error: %lu, type: %lu.\n", lpValueName, GetLastError(), dwType);
        dwBytes = 0;
        bRet = FALSE;
    }

    /* Close key if we opened it */
    if (lpSubKey)
        RegCloseKey(hKey);

    /* NULL-terminate string */
    Buf[min(BUFFER_SIZE-1, dwBytes/sizeof(WCHAR))] = L'\0';

    return bRet;
}

static
BOOL
RegGetDWORD(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPDWORD lpData)
{
    DWORD dwBytes = sizeof(*lpData), dwType;
    BOOL bRet = TRUE;

    /* If SubKy is specified open it */
    if (lpSubKey && RegOpenKeyExW(hKey,
                                 lpSubKey,
                                 0,
                                 KEY_QUERY_VALUE,
                                 &hKey) != ERROR_SUCCESS)
    {
        wprintf(L"Warning! Cannot open %s. Last error: %lu.\n", lpSubKey, GetLastError());
        return FALSE;
    }

    if (RegQueryValueExW(hKey,
                         lpValueName,
                         NULL,
                         &dwType,
                         (LPBYTE)lpData,
                         &dwBytes) != ERROR_SUCCESS || dwType != REG_DWORD)
    {
        wprintf(L"Warning! Cannot query %s. Last err: %lu, type: %lu\n", lpValueName, GetLastError(), dwType);
        *lpData = 0;
        bRet = FALSE;
    }

    /* Close key if we opened it */
    if (lpSubKey)
        RegCloseKey(hKey);

    return bRet;
}

static
void
FormatBytes(LPWSTR Buf, unsigned cBytes)
{
    WCHAR szMB[32];
    NUMBERFMTW fmt;

    _itow(cBytes / (1024*1024), szMB, 10);

    fmt.NumDigits = 0;
    fmt.LeadingZero = 0;
    fmt.Grouping = 3;
    fmt.lpDecimalSep = L"";
    fmt.lpThousandSep = L" ";
    fmt.NegativeOrder = 0;

    if(!GetNumberFormatW(LOCALE_SYSTEM_DEFAULT, 0, szMB, &fmt, Buf, BUFFER_SIZE))
        wprintf(L"Error! GetNumberFormat failed.\n");
}

static
void
FormatDateTime(time_t Time, LPWSTR lpBuf)
{
    unsigned cchBuf = BUFFER_SIZE, i;
    SYSTEMTIME SysTime;
    const struct tm *lpTm;

    lpTm = localtime(&Time);
    SysTime.wYear = (WORD)(1900 + lpTm->tm_year);
    SysTime.wMonth = (WORD)(1 + lpTm->tm_mon);
    SysTime.wDayOfWeek = (WORD)lpTm->tm_wday;
    SysTime.wDay = (WORD)lpTm->tm_mday;
    SysTime.wHour = (WORD)lpTm->tm_hour;
    SysTime.wMinute = (WORD)lpTm->tm_min;
    SysTime.wSecond = (WORD)lpTm->tm_sec;
    SysTime.wMilliseconds = 0;

    /* Time first */
    i = GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, &SysTime, NULL, lpBuf, cchBuf);
    if (i)
        --i; /* don't count NULL character */

    /* Time now */
    i += swprintf(lpBuf + i, L", ");
    GetTimeFormatW(LOCALE_SYSTEM_DEFAULT, 0, &SysTime, NULL, lpBuf + i, cchBuf - i);
}

/* Show usage */
static
VOID
Usage(VOID)
{
    WCHAR Buf[BUFFER_SIZE];

    if(GetOemStrings(IDS_USAGE, Buf))
        wprintf(L"%s", Buf);
}

/* Print all system information */
VOID
AllSysInfo(VOID)
{
    DWORD dwCharCount = BUFFER_SIZE, dwTimestamp;
    OSVERSIONINFOW VersionInfo;
    SYSTEM_INFO SysInfo;
    WCHAR Buf[BUFFER_SIZE], Tmp[BUFFER_SIZE], Msg[BUFFER_SIZE], szSystemDir[MAX_PATH];
    const WCHAR *lpcszSysType;
    LPWSTR lpBuffer;
    NETSETUP_JOIN_STATUS NetJoinStatus;
    MEMORYSTATUS MemoryStatus;
    unsigned int cSeconds;
    TIME_ZONE_INFORMATION TimeZoneInfo;
    HKEY hKey;

    if (!GetSystemDirectoryW(szSystemDir, sizeof(szSystemDir)/sizeof(szSystemDir[0])))
    {
        wprintf(L"Error! GetSystemDirectory failed.\n");
        return;
    }

    GetSystemInfo(&SysInfo);

    // getting computer name
    dwCharCount = BUFFER_SIZE;
    if (!GetComputerNameW(Buf, &dwCharCount))
        wprintf(L"Error! GetComputerName failed.\n");
    else if (GetOemStrings(IDS_HOST_NAME, Msg))
        wprintf(Msg, Buf);

    // open CurrentVersion key
    if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                    0,
                    KEY_QUERY_VALUE,
                    &hKey) != ERROR_SUCCESS)
    {
        wprintf(L"Error! RegOpenKeyEx failed.\n");
        return;
    }

    //getting OS Name
    RegGetSZ(hKey, NULL, L"ProductName", Buf);
    if (GetOemStrings(IDS_OS_NAME, Msg))
        wprintf(Msg, Buf);

    //getting OS Version
    ZeroMemory(&VersionInfo, sizeof(VersionInfo));
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    GetVersionExW(&VersionInfo);

    if (GetOemStrings(IDS_OS_VERSION, Msg))
        wprintf(Msg,
               (unsigned)VersionInfo.dwMajorVersion,
               (unsigned)VersionInfo.dwMinorVersion,
               (unsigned)VersionInfo.dwBuildNumber,
               VersionInfo.szCSDVersion,
               (unsigned)VersionInfo.dwBuildNumber);

    //getting OS Manufacturer

    //getting OS Configuration

    //getting OS Build Type
    RegGetSZ(hKey, NULL, L"CurrentType", Buf);
    if (GetOemStrings(IDS_OS_BUILD_TYPE, Msg))
        wprintf(Msg, Buf);

    //getting Registered Owner
    RegGetSZ(hKey, NULL, L"RegisteredOwner", Buf);
    if (GetOemStrings(IDS_REG_OWNER, Msg))
        wprintf(Msg, Buf);

    //getting Registered Organization
    RegGetSZ(hKey, NULL, L"RegisteredOrganization", Buf);
    if (GetOemStrings(IDS_REG_ORG, Msg))
        wprintf(Msg, Buf);

    //getting Product ID
    RegGetSZ(hKey, NULL, L"ProductId", Buf);
    if (GetOemStrings(IDS_PRODUCT_ID, Msg))
        wprintf(Msg, Buf);

    //getting Install Date
    RegGetDWORD(hKey, NULL, L"InstallDate", &dwTimestamp);
    FormatDateTime((time_t)dwTimestamp, Buf);
    if (GetOemStrings(IDS_INST_DATE, Msg))
        wprintf(Msg, Buf);

    // close Current Version key now
    RegCloseKey(hKey);

    //getting System Up Time
    cSeconds = GetTickCount() / 1000;
    if (GetOemStrings(IDS_UP_TIME, Msg))
        wprintf(Msg, cSeconds / (60*60*24), (cSeconds / (60*60)) % 24, (cSeconds / 60) % 60, cSeconds % 60);

    //getting System Manufacturer; HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\OEMInformation\Manufacturer for Win >= 6.0
    swprintf(Tmp, L"%s\\oeminfo.ini", szSystemDir);
    GetPrivateProfileStringW(L"General",
                             L"Manufacturer",
                             L"To Be Filled By O.E.M.",
                             Buf,
                             sizeof(Buf)/sizeof(Buf[0]),
                             Tmp);
    if (GetOemStrings(IDS_SYS_MANUFACTURER, Msg))
        wprintf(Msg, Buf);

    //getting System Model; HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\OEMInformation\Model for Win >= 6.0
    GetPrivateProfileStringW(L"General",
                             L"Model",
                             L"To Be Filled By O.E.M.",
                             Buf,
                             sizeof(Buf)/sizeof(Buf[0]),
                             Tmp);
    if (GetOemStrings(IDS_SYS_MODEL, Msg))
        wprintf(Msg, Buf);

    //getting System type
    switch (SysInfo.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_INTEL:
            lpcszSysType = L"X86-based PC";
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            lpcszSysType = L"IA64-based PC";
            break;
        case PROCESSOR_ARCHITECTURE_AMD64:
            lpcszSysType = L"AMD64-based PC";
            break;
        default:
            lpcszSysType = L"Unknown";
            break;
    }
    if (GetOemStrings(IDS_SYS_TYPE, Msg))
        wprintf(Msg, lpcszSysType);

    //getting Processor(s)
    if (GetOemStrings(IDS_PROCESSORS, Msg))
    {
        unsigned int i;
        wprintf(Msg, (unsigned int)SysInfo.dwNumberOfProcessors);
        for(i = 0; i < (unsigned int)SysInfo.dwNumberOfProcessors; i++)
        {
            swprintf(Tmp, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%u", i);

            RegGetSZ(HKEY_LOCAL_MACHINE, Tmp, L"Identifier", Buf);
            wprintf(L"                        [%02u]: %s", i+1, Buf);

            RegGetSZ(HKEY_LOCAL_MACHINE, Tmp, L"VendorIdentifier", Buf);
            wprintf(L" %s\n", Buf);
        }
    }

    //getting BIOS Version
    RegGetSZ(HKEY_LOCAL_MACHINE,
             L"HARDWARE\\DESCRIPTION\\System",
             L"SystemBiosVersion",
             Buf);
    if (GetOemStrings(IDS_BIOS_VERSION, Msg))
        wprintf(Msg, Buf);

    //gettings BIOS date
    RegGetSZ(HKEY_LOCAL_MACHINE,
             L"HARDWARE\\DESCRIPTION\\System",
             L"SystemBiosDate",
             Buf);
    if (GetOemStrings(IDS_BIOS_DATE, Msg))
        wprintf(Msg, Buf);

    //getting ReactOS Directory
    if (!GetWindowsDirectoryW(Buf, BUFFER_SIZE))
        wprintf(L"Error! GetWindowsDirectory failed.");
    else if (GetOemStrings(IDS_ROS_DIR, Msg))
        wprintf(Msg, Buf);

    //getting System Directory
    if (GetOemStrings(IDS_SYS_DIR, Msg))
        wprintf(Msg, szSystemDir);

    //getting Boot Device
    RegGetSZ(HKEY_LOCAL_MACHINE,
             L"SYSTEM\\Setup",
             L"SystemPartition",
             Buf);
    if (GetOemStrings(IDS_BOOT_DEV, Msg))
        wprintf(Msg, Buf);

    //getting System Locale
    if (GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_ILANGUAGE, Tmp, BUFFER_SIZE))
        if (RegGetSZ(HKEY_CLASSES_ROOT,
                     L"MIME\\Database\\Rfc1766",
                     Tmp,
                     Buf))
        {
            /* get rid of @filename,resource */
            lpBuffer = wcschr(Buf, L';');
            if (lpBuffer)
                SHLoadIndirectString(lpBuffer+1, lpBuffer+1, BUFFER_SIZE - (lpBuffer-Buf) - 1, NULL);

            if (GetOemStrings(IDS_SYS_LOCALE, Msg))
                wprintf(Msg, Buf);
        }

    //getting Input Locale
    if (RegGetSZ(HKEY_CURRENT_USER,
                 L"Keyboard Layout\\Preload",
                 L"1",
                 Tmp) && wcslen(Tmp) > 4)
        if (RegGetSZ(HKEY_CLASSES_ROOT,
                     L"MIME\\Database\\Rfc1766",
                     Tmp + 4,
                     Buf))
        {
            /* get rid of @filename,resource */
            lpBuffer = wcschr(Buf, L';');
            if (lpBuffer)
                SHLoadIndirectString(lpBuffer+1, lpBuffer+1, BUFFER_SIZE - (lpBuffer-Buf) - 1, NULL);

            if (GetOemStrings(IDS_INPUT_LOCALE, Msg))
                wprintf(Msg, Buf);
        }

    //getting Time Zone
    GetTimeZoneInformation(&TimeZoneInfo);

    /* Open Time Zones key */
    if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                     L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
                     0,
                     KEY_ENUMERATE_SUB_KEYS|KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        unsigned i;

        /* Find current timezone */
        dwCharCount = 256; // Windows seems to have a bug - it doesnt accept BUFFER_SIZE here
        for(i = 0; RegEnumKeyExW(hKey, i, Tmp, &dwCharCount, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; ++i, dwCharCount = 255)
        {
            RegGetSZ(hKey, Tmp, L"Std", Buf);

            if(!wcscmp(Buf, TimeZoneInfo.StandardName))
            {
                RegGetSZ(hKey, Tmp, L"Display", Buf);

                if (GetOemStrings(IDS_TIME_ZONE, Msg))
                    wprintf(Msg, Buf);

                break;
            }
        }
        RegCloseKey(hKey);
    }

    //getting Total Physical Memory
    GlobalMemoryStatus(&MemoryStatus);
    FormatBytes(Buf, MemoryStatus.dwTotalPhys);
    if (GetOemStrings(IDS_TOTAL_PHYS_MEM, Msg))
        wprintf(Msg, Buf);

    //getting Available Physical Memory
    FormatBytes(Buf, MemoryStatus.dwAvailPhys);
    if (GetOemStrings(IDS_AVAIL_PHISICAL_MEM,Msg))
        wprintf(Msg, Buf);

    //getting Virtual Memory: Max Size
    FormatBytes(Buf, MemoryStatus.dwTotalVirtual);
    if (GetOemStrings(IDS_VIRT_MEM_MAX, Msg))
        wprintf(Msg, Buf);

    //getting Virtual Memory: Available
    FormatBytes(Buf, MemoryStatus.dwAvailVirtual);
    if (GetOemStrings(IDS_VIRT_MEM_AVAIL, Msg))
        wprintf(Msg, Buf);

    //getting Virtual Memory: In Use
    FormatBytes(Buf, MemoryStatus.dwTotalVirtual-MemoryStatus.dwAvailVirtual);
    if (GetOemStrings(IDS_VIRT_MEM_INUSE, Msg))
        wprintf(Msg, Buf);

    //getting Page File Location(s)
    if (RegGetSZ(HKEY_LOCAL_MACHINE,
                 L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management",
                 L"PagingFiles",
                 Buf))
    {
        int i;

        for(i = 0; i < strlen((char*)Buf); i++)
        {
            if (Buf[i] == TEXT(' '))
            {
                Buf[i] = TEXT('\0');
                break;
            }
        }

        if(GetOemStrings(IDS_PAGEFILE_LOC, Msg))
            wprintf(Msg, Buf);
    }

    //getting Domain
    if (NetGetJoinInformation (NULL, &lpBuffer, &NetJoinStatus) == NERR_Success)
    {
        if(GetOemStrings(IDS_DOMAIN, Msg))
            wprintf(Msg, lpBuffer);

        NetApiBufferFree(lpBuffer);
    }

    //getting Logon Server

    //getting NetWork Card(s)
    if(GetOemStrings(IDS_NETWORK_CARDS, Msg))
    {

    }
}

/* Main program */
int
main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    if (argc > 1 && (!strcmp(argv[1], "/?") || !strcmp(argv[1], "-?")))
    {
        Usage();
        return 0;
    }

    AllSysInfo();

    return 0;
}
