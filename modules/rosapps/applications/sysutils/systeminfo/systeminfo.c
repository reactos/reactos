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
#include <stdarg.h>
#include <windows.h>
#include <time.h>
#include <locale.h>
#include <lm.h>
#include <shlwapi.h>
#include <iphlpapi.h>
#include <winsock2.h>
#include <udmihelp.h>

#include "resource.h"

#define BUFFER_SIZE 1024

/* Load string from registry */
static
unsigned
RegGetSZ(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPWSTR lpBuf, DWORD cchBuf)
{
    DWORD dwBytes = cchBuf*sizeof(WCHAR), dwType = 0;
    unsigned cChars;

    /* If SubKey is specified open it */
    if (lpSubKey && RegOpenKeyExW(hKey,
                                  lpSubKey,
                                  0,
                                  KEY_QUERY_VALUE,
                                  &hKey) != ERROR_SUCCESS)
    {
        wprintf(L"Warning! Cannot open %s. Last error: %lu.\n", lpSubKey, GetLastError());
        return 0;
    }

    /* Query registry value and check its type */
    if (RegQueryValueExW(hKey,
                         lpValueName,
                         NULL,
                         &dwType,
                         (LPBYTE)lpBuf,
                         &dwBytes) != ERROR_SUCCESS || (dwType != REG_SZ && dwType != REG_MULTI_SZ))
    {
        wprintf(L"Warning! Cannot query %s. Last error: %lu, type: %lu.\n", lpValueName, GetLastError(), dwType);
        dwBytes = 0;
    }
    else if (dwBytes == 0)
    {
        wcscpy(lpBuf, L"N/A");
        dwBytes = 6;
    }

    /* Close key if we opened it */
    if (lpSubKey)
        RegCloseKey(hKey);

    cChars = dwBytes/sizeof(WCHAR);

    /* NULL-terminate string */
    lpBuf[min(cchBuf-1, cChars)] = L'\0';

    /* Don't count NULL characters */
    while(cChars && !lpBuf[cChars-1])
        --cChars;

    return cChars;
}

/* Load DWORD from registry */
static
BOOL
RegGetDWORD(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPDWORD lpData)
{
    DWORD dwBytes = sizeof(*lpData), dwType;
    BOOL bRet = TRUE;

    /* If SubKey is specified open it */
    if (lpSubKey && RegOpenKeyExW(hKey,
                                 lpSubKey,
                                 0,
                                 KEY_QUERY_VALUE,
                                 &hKey) != ERROR_SUCCESS)
    {
        wprintf(L"Warning! Cannot open %s. Last error: %lu.\n", lpSubKey, GetLastError());
        return FALSE;
    }

    /* Query registry value and check its type */
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

/* Format bytes */
static
VOID
FormatBytes(LPWSTR lpBuf, unsigned cBytes)
{
    WCHAR szMB[32];
    NUMBERFMTW fmt;
    unsigned i;

    _itow(cBytes / (1024*1024), szMB, 10);

    fmt.NumDigits = 0;
    fmt.LeadingZero = 0;
    fmt.Grouping = 3;
    fmt.lpDecimalSep = L"";
    fmt.lpThousandSep = L" ";
    fmt.NegativeOrder = 0;

    i = GetNumberFormatW(LOCALE_SYSTEM_DEFAULT, 0, szMB, &fmt, lpBuf, BUFFER_SIZE - 3);
    if (i)
        --i; /* don't count NULL character */
    wcscpy(lpBuf + i, L" MB");
}

/* Format date and time */
static
VOID
FormatDateTime(time_t Time, LPWSTR lpBuf)
{
    unsigned i;
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

    /* Copy date first */
    i = GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, &SysTime, NULL, lpBuf, BUFFER_SIZE - 2);
    if (i)
        --i; /* don't count NULL character */

    /* Copy time now */
    i += swprintf(lpBuf + i, L", ");

    GetTimeFormatW(LOCALE_SYSTEM_DEFAULT, 0, &SysTime, NULL, lpBuf + i, BUFFER_SIZE - i);
}

ULONGLONG GetSecondsQPC(VOID)
{
    LARGE_INTEGER Counter, Frequency;

    QueryPerformanceCounter(&Counter);
    QueryPerformanceFrequency(&Frequency);

    return Counter.QuadPart / Frequency.QuadPart;
}

ULONGLONG GetSeconds(VOID)
{
    ULONGLONG (WINAPI * pGetTickCount64)(VOID);
    ULONGLONG Ticks64;
    HMODULE hModule = GetModuleHandleW(L"kernel32.dll");

    pGetTickCount64 = (PVOID)GetProcAddress(hModule, "GetTickCount64");
    if (pGetTickCount64)
    {
        return pGetTickCount64() / 1000;
    }

    hModule = LoadLibraryW(L"kernel32_vista.dll");

    if (!hModule)
    {
        return GetSecondsQPC();
    }

    pGetTickCount64 = (PVOID)GetProcAddress(hModule, "GetTickCount64");

    if (pGetTickCount64)
    {
        Ticks64 = pGetTickCount64() / 1000;
    }
    else
    {
        Ticks64 = GetSecondsQPC();
    }

    FreeLibrary(hModule);
    return Ticks64;
}

/* Show usage */
static
VOID
Usage(VOID)
{
    WCHAR Buf[4096];
    if (LoadStringW(GetModuleHandle(NULL), IDS_USAGE, Buf, 4096))
        wprintf(L"%s", Buf);
}

static
VOID
PrintRow(UINT nTitleID, BOOL bIndent, LPWSTR lpFormat, ...)
{
    WCHAR Buf[BUFFER_SIZE];
    va_list Args;
    unsigned c;

    if (nTitleID)
    {
        c = LoadStringW(GetModuleHandle(NULL), nTitleID, Buf, BUFFER_SIZE - 2);
        if (!c)
            return;

        wcscpy(Buf + c, L": ");
    } else
        Buf[0] = L'\0';

    if (!bIndent)
        wprintf(L"%-32s", Buf);
    else if (Buf[0])
        wprintf(L"%38s%-16s", L"", Buf);
    else
        wprintf(L"%38s", L"");

    va_start(Args, lpFormat);
    vwprintf(lpFormat, Args);
    va_end(Args);

    wprintf(L"\n");
}

/* Print all system information */
VOID
AllSysInfo(VOID)
{
    DWORD dwCharCount = BUFFER_SIZE, dwTimestamp, dwResult;
    OSVERSIONINFOW VersionInfo;
    SYSTEM_INFO SysInfo;
    WCHAR Buf[BUFFER_SIZE], Tmp[BUFFER_SIZE], szSystemDir[MAX_PATH];
    const WCHAR *lpcszSysType;
    LPWSTR lpBuffer;
    NETSETUP_JOIN_STATUS NetJoinStatus;
    MEMORYSTATUS MemoryStatus;
    unsigned int cSeconds, i, j;
    TIME_ZONE_INFORMATION TimeZoneInfo;
    HKEY hKey;
    PIP_ADAPTER_ADDRESSES pAdapters;
    ULONG cbAdapters;
    PVOID SMBiosBuf;
    PCHAR DmiStrings[ID_STRINGS_MAX] = { 0 };

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
    else
        PrintRow(IDS_HOST_NAME, FALSE, L"%s", Buf);

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
    RegGetSZ(hKey, NULL, L"ProductName", Buf, BUFFER_SIZE);
    PrintRow(IDS_OS_NAME, FALSE, L"%s", Buf);

    //getting OS Version
    ZeroMemory(&VersionInfo, sizeof(VersionInfo));
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    GetVersionExW(&VersionInfo);

    if (!LoadStringW(GetModuleHandle(NULL), IDS_BUILD, Tmp, BUFFER_SIZE))
        Tmp[0] = L'\0';
    PrintRow(IDS_OS_VERSION,
             FALSE,
             L"%lu.%lu.%lu %s %s %lu",
             VersionInfo.dwMajorVersion,
             VersionInfo.dwMinorVersion,
             VersionInfo.dwBuildNumber,
             VersionInfo.szCSDVersion,
             Tmp,
             VersionInfo.dwBuildNumber);

    //getting OS Manufacturer

    //getting OS Configuration

    //getting OS Build Type
    RegGetSZ(hKey, NULL, L"CurrentType", Buf, BUFFER_SIZE);
    PrintRow(IDS_OS_BUILD_TYPE, FALSE, L"%s", Buf);

    //getting Registered Owner
    RegGetSZ(hKey, NULL, L"RegisteredOwner", Buf, BUFFER_SIZE);
    PrintRow(IDS_REG_OWNER, FALSE, L"%s", Buf);

    //getting Registered Organization
    RegGetSZ(hKey, NULL, L"RegisteredOrganization", Buf, BUFFER_SIZE);
    PrintRow(IDS_REG_ORG, FALSE, L"%s", Buf);

    //getting Product ID
    RegGetSZ(hKey, NULL, L"ProductId", Buf, BUFFER_SIZE);
    PrintRow(IDS_PRODUCT_ID, FALSE, L"%s", Buf);

    //getting Install Date
    RegGetDWORD(hKey, NULL, L"InstallDate", &dwTimestamp);
    FormatDateTime((time_t)dwTimestamp, Buf);
    PrintRow(IDS_INST_DATE, FALSE, L"%s", Buf);

    // close Current Version key now
    RegCloseKey(hKey);

    //getting System Up Time
    cSeconds = GetSeconds();
    if (!LoadStringW(GetModuleHandle(NULL), IDS_UP_TIME_FORMAT, Tmp, BUFFER_SIZE))
        Tmp[0] = L'\0';
    swprintf(Buf, Tmp, cSeconds / (60*60*24), (cSeconds / (60*60)) % 24, (cSeconds / 60) % 60, cSeconds % 60);
    PrintRow(IDS_UP_TIME, FALSE, L"%s", Buf);

    // prepare SMBIOS data
    SMBiosBuf = LoadSMBiosData(DmiStrings);

    //getting System Manufacturer; HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\OEMInformation\Manufacturer for Win >= 6.0
    swprintf(Tmp, L"%s\\oeminfo.ini", szSystemDir);
    GetPrivateProfileStringW(L"General",
                             L"Manufacturer",
                             L"",
                             Buf,
                             sizeof(Buf)/sizeof(Buf[0]),
                             Tmp);
    if (wcslen(Buf) == 0 && SMBiosBuf)
    {
        GetSMBiosStringW(DmiStrings[SYS_VENDOR], Buf, _countof(Buf), FALSE);
    }
    PrintRow(IDS_SYS_MANUFACTURER, FALSE, L"%s", Buf);

    //getting System Model; HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\OEMInformation\Model for Win >= 6.0
    GetPrivateProfileStringW(L"General",
                             L"Model",
                             L"",
                             Buf,
                             sizeof(Buf)/sizeof(Buf[0]),
                             Tmp);
    if (wcslen(Buf) == 0 && SMBiosBuf)
    {
        GetSMBiosStringW(DmiStrings[SYS_PRODUCT], Buf, _countof(Buf), FALSE);
    }
    PrintRow(IDS_SYS_MODEL, FALSE, L"%s", Buf);

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
    PrintRow(IDS_SYS_TYPE, FALSE, L"%s", lpcszSysType);

    //getting Processor(s)
    if (!LoadStringW(GetModuleHandle(NULL), IDS_PROCESSORS_FORMAT, Tmp, BUFFER_SIZE))
        Tmp[0] = L'\0';
    swprintf(Buf, Tmp, (unsigned)SysInfo.dwNumberOfProcessors);
    PrintRow(IDS_PROCESSORS, FALSE, L"%s", Buf);
    for(i = 0; i < (unsigned int)SysInfo.dwNumberOfProcessors; i++)
    {
        swprintf(Tmp, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%u", i);
        j = swprintf(Buf, L"[%02u]: ", i + 1);

        j += RegGetSZ(HKEY_LOCAL_MACHINE, Tmp, L"Identifier", Buf + j, BUFFER_SIZE - j);
        if(j + 1 < BUFFER_SIZE)
            Buf[j++] = L' ';
        RegGetSZ(HKEY_LOCAL_MACHINE, Tmp, L"VendorIdentifier", Buf + j, BUFFER_SIZE - j);

        PrintRow(0, FALSE, L"%s", Buf);
    }

    //getting BIOS Version
    if (SMBiosBuf)
    {
        j = GetSMBiosStringW(DmiStrings[BIOS_VENDOR], Buf, BUFFER_SIZE, TRUE);
        if (j + 1 < BUFFER_SIZE)
        {
            Buf[j++] = L' ';
            Buf[j] = L'\0';
        }
        GetSMBiosStringW(DmiStrings[BIOS_VERSION], Buf + j, BUFFER_SIZE - j, TRUE);
    }
    else
    {
        RegGetSZ(HKEY_LOCAL_MACHINE,
                 L"HARDWARE\\DESCRIPTION\\System",
                 L"SystemBiosVersion",
                 Buf,
                 BUFFER_SIZE);
    }
    PrintRow(IDS_BIOS_VERSION, FALSE, L"%s", Buf);

    //gettings BIOS date
    if (SMBiosBuf)
    {
        GetSMBiosStringW(DmiStrings[BIOS_DATE], Buf, BUFFER_SIZE, TRUE);
    }
    else
    {
        RegGetSZ(HKEY_LOCAL_MACHINE,
                 L"HARDWARE\\DESCRIPTION\\System",
                 L"SystemBiosDate",
                 Buf,
                 BUFFER_SIZE);
    }
    PrintRow(IDS_BIOS_DATE, FALSE, L"%s", Buf);

    // clean SMBIOS data
    FreeSMBiosData(SMBiosBuf);

    //getting ReactOS Directory
    if (!GetWindowsDirectoryW(Buf, BUFFER_SIZE))
        wprintf(L"Error! GetWindowsDirectory failed.");
    else
        PrintRow(IDS_ROS_DIR, FALSE, L"%s", Buf);

    //getting System Directory
    PrintRow(IDS_SYS_DIR, 0, L"%s", szSystemDir);

    //getting Boot Device
    RegGetSZ(HKEY_LOCAL_MACHINE,
             L"SYSTEM\\Setup",
             L"SystemPartition",
             Buf,
             BUFFER_SIZE);
    PrintRow(IDS_BOOT_DEV, FALSE, L"%s", Buf);

    //getting System Locale
    if (GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_ILANGUAGE, Tmp, BUFFER_SIZE))
        if (RegGetSZ(HKEY_CLASSES_ROOT,
                     L"MIME\\Database\\Rfc1766",
                     Tmp,
                     Buf,
                     BUFFER_SIZE))
        {
            /* get rid of @filename,resource */
            lpBuffer = wcschr(Buf, L';');
            if (lpBuffer)
                SHLoadIndirectString(lpBuffer+1, lpBuffer+1, BUFFER_SIZE - (lpBuffer-Buf) - 1, NULL);

            PrintRow(IDS_SYS_LOCALE, FALSE, L"%s", Buf);
        }

    //getting Input Locale
    if (RegGetSZ(HKEY_CURRENT_USER,
                 L"Keyboard Layout\\Preload",
                 L"1",
                 Tmp,
                 BUFFER_SIZE) && wcslen(Tmp) > 4)
        if (RegGetSZ(HKEY_CLASSES_ROOT,
                     L"MIME\\Database\\Rfc1766",
                     Tmp + 4,
                     Buf,
                     BUFFER_SIZE))
        {
            /* get rid of @filename,resource */
            lpBuffer = wcschr(Buf, L';');
            if (lpBuffer)
                SHLoadIndirectString(lpBuffer+1, lpBuffer+1, BUFFER_SIZE - (lpBuffer-Buf) - 1, NULL);

            PrintRow(IDS_INPUT_LOCALE, FALSE, L"%s", Buf);
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
        dwCharCount = BUFFER_SIZE;
        for(i = 0; RegEnumKeyExW(hKey, i, Tmp, &dwCharCount, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; ++i, dwCharCount = 255)
        {
            RegGetSZ(hKey, Tmp, L"Std", Buf, BUFFER_SIZE);

            if (!wcscmp(Buf, TimeZoneInfo.StandardName))
            {
                RegGetSZ(hKey, Tmp, L"Display", Buf, BUFFER_SIZE);

                PrintRow(IDS_TIME_ZONE, FALSE, L"%s", Buf);

                break;
            }
        }
        RegCloseKey(hKey);
    }

    //getting Total Physical Memory
    GlobalMemoryStatus(&MemoryStatus);
    FormatBytes(Buf, MemoryStatus.dwTotalPhys);
    PrintRow(IDS_TOTAL_PHYS_MEM, FALSE, L"%s", Buf);

    //getting Available Physical Memory
    FormatBytes(Buf, MemoryStatus.dwAvailPhys);
    PrintRow(IDS_AVAIL_PHISICAL_MEM, FALSE, L"%s", Buf);

    //getting Virtual Memory: Max Size
    FormatBytes(Buf, MemoryStatus.dwTotalVirtual);
    PrintRow(IDS_VIRT_MEM_MAX, FALSE, L"%s", Buf);

    //getting Virtual Memory: Available
    FormatBytes(Buf, MemoryStatus.dwAvailVirtual);
    PrintRow(IDS_VIRT_MEM_AVAIL, FALSE, L"%s", Buf);

    //getting Virtual Memory: In Use
    FormatBytes(Buf, MemoryStatus.dwTotalVirtual-MemoryStatus.dwAvailVirtual);
    PrintRow(IDS_VIRT_MEM_INUSE, FALSE, L"%s", Buf);

    //getting Page File Location(s)
    if (RegGetSZ(HKEY_LOCAL_MACHINE,
                 L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management",
                 L"PagingFiles",
                 Buf,
                 BUFFER_SIZE))
    {
        int i;

        for(i = 0; Buf[i]; i++)
        {
            if (Buf[i] == L' ')
            {
                Buf[i] = L'\0';
                break;
            }
        }

        PrintRow(IDS_PAGEFILE_LOC, FALSE, L"%s", Buf);
    }

    //getting Domain
    if (NetGetJoinInformation (NULL, &lpBuffer, &NetJoinStatus) == NERR_Success)
    {
        if (NetJoinStatus == NetSetupWorkgroupName || NetJoinStatus == NetSetupDomainName)
            PrintRow(IDS_DOMAIN, FALSE, L"%s", lpBuffer);

        NetApiBufferFree(lpBuffer);
    }

    //getting Logon Server

    //getting NetWork Card(s)
    cbAdapters = 4096;
    pAdapters = malloc(cbAdapters);
    while((dwResult = GetAdaptersAddresses(AF_UNSPEC, 0x0002, NULL, pAdapters, &cbAdapters)) == ERROR_BUFFER_OVERFLOW)
    {
        cbAdapters += 4096;
        pAdapters = (PIP_ADAPTER_ADDRESSES)realloc(pAdapters, cbAdapters);
    }

    if (dwResult == ERROR_SUCCESS)
    {
        PIP_ADAPTER_ADDRESSES pCurrentAdapter = pAdapters;
        unsigned cAdapters = 0;

        /* Count adapters */
        for(i = 0; pCurrentAdapter; ++i)
        {
            if (pCurrentAdapter->IfType != 24 && pCurrentAdapter->IfType != 131)
                ++cAdapters;
            pCurrentAdapter = pCurrentAdapter->Next;
        }


        /* Print adapters count */
        if (!LoadStringW(GetModuleHandle(NULL), IDS_NETWORK_CARDS_FORMAT, Tmp, BUFFER_SIZE))
            Tmp[0] = L'\0';
        swprintf(Buf, Tmp, cAdapters);
        PrintRow(IDS_NETWORK_CARDS, FALSE, L"%s", Buf);

        /* Show information about each adapter */
        pCurrentAdapter = pAdapters;
        for(i = 0; pCurrentAdapter; ++i)
        {
            if (pCurrentAdapter->IfType != 24 && pCurrentAdapter->IfType != 131)//IF_TYPE_SOFTWARE_LOOPBACK)
            {
                PIP_ADAPTER_UNICAST_ADDRESS pAddress;

                PrintRow(0, FALSE, L"[%02u]: %s", i + 1, pCurrentAdapter->Description);
                PrintRow(IDS_CONNECTION_NAME, TRUE, L"%s", pCurrentAdapter->FriendlyName);
                if (!(pCurrentAdapter->Flags & 0x0004))
                {
                    if (!LoadStringW(GetModuleHandle(NULL), IDS_NO, Buf, BUFFER_SIZE))
                        Buf[0] = L'\0';
                    PrintRow(IDS_DHCP_ENABLED, TRUE, Buf);
                }
                if (pCurrentAdapter->OperStatus == IfOperStatusDown)
                {
                    if (!LoadStringW(GetModuleHandle(NULL), IDS_MEDIA_DISCONNECTED, Buf, BUFFER_SIZE))
                        Buf[0] = L'\0';
                    PrintRow(IDS_STATUS, TRUE, Buf);
                }
                else
                {
                    if (!LoadStringW(GetModuleHandle(NULL), IDS_IP_ADDRESSES, Buf, BUFFER_SIZE))
                        Buf[0] = L'\0';
                    PrintRow(0, TRUE, Buf);
                    pAddress = pCurrentAdapter->FirstUnicastAddress;
                    for (j = 0; pAddress; ++j)
                    {
                        dwCharCount = BUFFER_SIZE;
                        WSAAddressToStringW(pAddress->Address.lpSockaddr, pAddress->Address.iSockaddrLength, NULL, Buf, &dwCharCount);
                        PrintRow(0, TRUE, L"[%02u]: %s", j + 1, Buf);
                        pAddress = pAddress->Next;
                    }
                }
            }
            pCurrentAdapter = pCurrentAdapter->Next;
        }
    }
    free(pAdapters);
}

/* Main program */
int
main(int argc, char *argv[])
{
    WSADATA WsaData;
    int i;

    setlocale(LC_ALL, "");

    WSAStartup(MAKEWORD(2, 2), &WsaData);

    for (i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "/?") || !strcmp(argv[i], "-?"))
        {
            Usage();
            return 0;
        }
        else
        {
            printf("Unsupported argument: %s\n", argv[i]);
            return -1;
        }
    }

    AllSysInfo();

    return 0;
}
