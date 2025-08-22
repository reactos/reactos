/*
 * PROJECT:     ReactOS SystemInfo Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Displays system information.
 * COPYRIGHT:   Copyright 2007 Dmitry Chapyshev <lentind@yandex.ru>
 *              Copyright 2011 Rafal Harabien <rafalh1992@o2.pl>
 *              Copyright 2018 Stanislav Motylkov <x86corez@gmail.com>
 */

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
#include <conutils.h>

#include "resource.h"

#define BUFFER_SIZE 1024

/* Load string from registry */
static
UINT
RegGetSZ(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPWSTR lpBuf, DWORD cchBuf)
{
    DWORD dwBytes = cchBuf * sizeof(WCHAR), dwType = 0;
    UINT cChars;

    /* If SubKey is specified open it */
    if (lpSubKey && RegOpenKeyExW(hKey,
                                  lpSubKey,
                                  0,
                                  KEY_QUERY_VALUE,
                                  &hKey) != ERROR_SUCCESS)
    {
        ConPrintf(StdErr, L"Warning! Cannot open %s. Last error: %lu.\n",
                  lpSubKey, GetLastError());
        return 0;
    }

    /* Query registry value and check its type */
    if (RegQueryValueExW(hKey,
                         lpValueName,
                         NULL,
                         &dwType,
                         (LPBYTE)lpBuf,
                         &dwBytes) != ERROR_SUCCESS ||
        (dwType != REG_SZ && dwType != REG_MULTI_SZ))
    {
        ConPrintf(StdErr, L"Warning! Cannot query %s. Last error: %lu, type: %lu.\n",
                  lpValueName, GetLastError(), dwType);
        dwBytes = 0;
    }
    else if (dwBytes == 0)
    {
        wcscpy(lpBuf, L"N/A");
        dwBytes = sizeof(L"N/A")-sizeof(WCHAR);
    }

    /* Close key if we opened it */
    if (lpSubKey)
        RegCloseKey(hKey);

    cChars = dwBytes/sizeof(WCHAR);

    /* NULL-terminate string */
    lpBuf[min(cchBuf-1, cChars)] = L'\0';

    /* Don't count NULL characters */
    while (cChars && !lpBuf[cChars-1])
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
        ConPrintf(StdErr, L"Warning! Cannot open %s. Last error: %lu.\n",
                  lpSubKey, GetLastError());
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
        ConPrintf(StdErr, L"Warning! Cannot query %s. Last err: %lu, type: %lu\n",
                  lpValueName, GetLastError(), dwType);
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
FormatBytes(LPWSTR lpBuf, UINT cBytes)
{
    WCHAR szMB[32];
    NUMBERFMTW fmt;
    UINT i;

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
    UINT i;
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
    ULONGLONG (WINAPI *pGetTickCount64)(VOID);
    ULONGLONG Ticks64;
    HMODULE hModule = GetModuleHandleW(L"kernel32.dll");

    pGetTickCount64 = (PVOID)GetProcAddress(hModule, "GetTickCount64");
    if (pGetTickCount64)
        return pGetTickCount64() / 1000;

    hModule = LoadLibraryW(L"kernel32_vista.dll");
    if (!hModule)
        return GetSecondsQPC();

    pGetTickCount64 = (PVOID)GetProcAddress(hModule, "GetTickCount64");
    if (pGetTickCount64)
        Ticks64 = pGetTickCount64() / 1000;
    else
        Ticks64 = GetSecondsQPC();

    FreeLibrary(hModule);
    return Ticks64;
}

/* Show usage */
static
VOID
Usage(VOID)
{
    ConResPrintf(StdOut, IDS_USAGE);
}

static
VOID
PrintRow(UINT nTitleID, BOOL bIndent, LPWSTR lpFormat, ...)
{
    va_list Args;
    UINT c;
    WCHAR Buf[BUFFER_SIZE];

    if (nTitleID)
    {
        c = LoadStringW(GetModuleHandle(NULL), nTitleID, Buf, _countof(Buf) - 2);
        if (!c)
            return;

        wcscpy(Buf + c, L": ");
    }
    else
    {
        Buf[0] = L'\0';
    }

    if (!bIndent)
        ConPrintf(StdOut, L"%-32s", Buf);
    else if (Buf[0])
        ConPrintf(StdOut, L"%38s%-16s", L"", Buf);
    else
        ConPrintf(StdOut, L"%38s", L"");

    va_start(Args, lpFormat);
    ConPrintfV(StdOut, lpFormat, Args);
    va_end(Args);

    ConPuts(StdOut, L"\n");
}

/* Print all system information */
VOID
AllSysInfo(VOID)
{
    DWORD dwCharCount, dwTimestamp, dwResult;
    OSVERSIONINFOW VersionInfo;
    SYSTEM_INFO SysInfo;
    WCHAR Buf[BUFFER_SIZE], Tmp[BUFFER_SIZE], szSystemDir[MAX_PATH];
    LPCWSTR lpcszSysType;
    LPWSTR lpBuffer;
    NETSETUP_JOIN_STATUS NetJoinStatus;
    MEMORYSTATUS MemoryStatus;
    UINT cSeconds, i, j;
    TIME_ZONE_INFORMATION TimeZoneInfo;
    HKEY hKey;
    PIP_ADAPTER_ADDRESSES pAdapters;
    ULONG cbAdapters;
    PVOID SMBiosBuf;
    PCHAR DmiStrings[ID_STRINGS_MAX] = { 0 };

    if (!GetSystemDirectoryW(szSystemDir, _countof(szSystemDir)))
    {
        ConPrintf(StdErr, L"Error! GetSystemDirectory failed.\n");
        return;
    }

    GetSystemInfo(&SysInfo);

    /* Getting computer name */
    dwCharCount = _countof(Buf);
    if (!GetComputerNameW(Buf, &dwCharCount))
        ConPrintf(StdErr, L"Error! GetComputerName failed.\n");
    else
        PrintRow(IDS_HOST_NAME, FALSE, L"%s", Buf);

    /* Open CurrentVersion key */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey) != ERROR_SUCCESS)
    {
        ConPrintf(StdErr, L"Error! RegOpenKeyEx failed.\n");
        return;
    }

    /* Getting OS Name */
    RegGetSZ(hKey, NULL, L"ProductName", Buf, _countof(Buf));
    PrintRow(IDS_OS_NAME, FALSE, L"%s", Buf);

    /* Getting OS Version */
    ZeroMemory(&VersionInfo, sizeof(VersionInfo));
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    GetVersionExW(&VersionInfo);

    if (!LoadStringW(GetModuleHandle(NULL), IDS_BUILD, Tmp, _countof(Tmp)))
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

    /* Getting OS Manufacturer */

    /* Getting OS Configuration */

    /* Getting OS Build Type */
    RegGetSZ(hKey, NULL, L"CurrentType", Buf, _countof(Buf));
    PrintRow(IDS_OS_BUILD_TYPE, FALSE, L"%s", Buf);

    /* Getting Registered Owner */
    RegGetSZ(hKey, NULL, L"RegisteredOwner", Buf, _countof(Buf));
    PrintRow(IDS_REG_OWNER, FALSE, L"%s", Buf);

    /* Getting Registered Organization */
    RegGetSZ(hKey, NULL, L"RegisteredOrganization", Buf, _countof(Buf));
    PrintRow(IDS_REG_ORG, FALSE, L"%s", Buf);

    /* Getting Product ID */
    RegGetSZ(hKey, NULL, L"ProductId", Buf, _countof(Buf));
    PrintRow(IDS_PRODUCT_ID, FALSE, L"%s", Buf);

    /* Getting Install Date */
    RegGetDWORD(hKey, NULL, L"InstallDate", &dwTimestamp);
    FormatDateTime((time_t)dwTimestamp, Buf);
    PrintRow(IDS_INST_DATE, FALSE, L"%s", Buf);

    /* Close Current Version key now */
    RegCloseKey(hKey);

    /* Getting System Up Time */
    cSeconds = GetSeconds();
    if (!LoadStringW(GetModuleHandle(NULL), IDS_UP_TIME_FORMAT, Tmp, _countof(Tmp)))
        Tmp[0] = L'\0';
    swprintf(Buf, Tmp, cSeconds / (60*60*24), (cSeconds / (60*60)) % 24, (cSeconds / 60) % 60, cSeconds % 60);
    PrintRow(IDS_UP_TIME, FALSE, L"%s", Buf);

    /* Prepare SMBIOS data */
    SMBiosBuf = LoadSMBiosData(DmiStrings);

    /* Getting System Manufacturer;
     * HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\OEMInformation\Manufacturer
     * for Win >= 6.0 */
    swprintf(Tmp, L"%s\\oeminfo.ini", szSystemDir);
    GetPrivateProfileStringW(L"General",
                             L"Manufacturer",
                             L"",
                             Buf,
                             _countof(Buf),
                             Tmp);
    if (wcslen(Buf) == 0 && SMBiosBuf)
    {
        GetSMBiosStringW(DmiStrings[SYS_VENDOR], Buf, _countof(Buf), FALSE);
    }
    PrintRow(IDS_SYS_MANUFACTURER, FALSE, L"%s", Buf);

    /* Getting System Model;
     * HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\OEMInformation\Model
     * for Win >= 6.0 */
    GetPrivateProfileStringW(L"General",
                             L"Model",
                             L"",
                             Buf,
                             _countof(Buf),
                             Tmp);
    if (wcslen(Buf) == 0 && SMBiosBuf)
    {
        GetSMBiosStringW(DmiStrings[SYS_PRODUCT], Buf, _countof(Buf), FALSE);
    }
    PrintRow(IDS_SYS_MODEL, FALSE, L"%s", Buf);

    /* Getting System type */
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

    /* Getting Processor(s) */
    if (!LoadStringW(GetModuleHandle(NULL), IDS_PROCESSORS_FORMAT, Tmp, _countof(Tmp)))
        Tmp[0] = L'\0';
    swprintf(Buf, Tmp, (UINT)SysInfo.dwNumberOfProcessors);
    PrintRow(IDS_PROCESSORS, FALSE, L"%s", Buf);
    for (i = 0; i < (UINT)SysInfo.dwNumberOfProcessors; i++)
    {
        swprintf(Tmp, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%u", i);
        j = swprintf(Buf, L"[%02u]: ", i + 1);

        j += RegGetSZ(HKEY_LOCAL_MACHINE, Tmp, L"Identifier", Buf + j, _countof(Buf) - j);
        if (j + 1 < _countof(Buf))
            Buf[j++] = L' ';
        RegGetSZ(HKEY_LOCAL_MACHINE, Tmp, L"VendorIdentifier", Buf + j, _countof(Buf) - j);

        PrintRow(0, FALSE, L"%s", Buf);
    }

    /* Getting BIOS Version */
    if (SMBiosBuf)
    {
        j = GetSMBiosStringW(DmiStrings[BIOS_VENDOR], Buf, _countof(Buf), TRUE);
        if (j + 1 < _countof(Buf))
        {
            Buf[j++] = L' ';
            Buf[j] = L'\0';
        }
        GetSMBiosStringW(DmiStrings[BIOS_VERSION], Buf + j, _countof(Buf) - j, TRUE);
    }
    else
    {
        RegGetSZ(HKEY_LOCAL_MACHINE,
                 L"HARDWARE\\DESCRIPTION\\System",
                 L"SystemBiosVersion",
                 Buf,
                 _countof(Buf));
    }
    PrintRow(IDS_BIOS_VERSION, FALSE, L"%s", Buf);

    /* Getting BIOS date */
    if (SMBiosBuf)
    {
        GetSMBiosStringW(DmiStrings[BIOS_DATE], Buf, _countof(Buf), TRUE);
    }
    else
    {
        RegGetSZ(HKEY_LOCAL_MACHINE,
                 L"HARDWARE\\DESCRIPTION\\System",
                 L"SystemBiosDate",
                 Buf,
                 _countof(Buf));
    }
    PrintRow(IDS_BIOS_DATE, FALSE, L"%s", Buf);

    /* Clean SMBIOS data */
    FreeSMBiosData(SMBiosBuf);

    /* Getting ReactOS Directory */
    if (!GetWindowsDirectoryW(Buf, _countof(Buf)))
        ConPrintf(StdErr, L"Error! GetWindowsDirectory failed.");
    else
        PrintRow(IDS_ROS_DIR, FALSE, L"%s", Buf);

    /* Getting System Directory */
    PrintRow(IDS_SYS_DIR, 0, L"%s", szSystemDir);

    /* Getting Boot Device */
    RegGetSZ(HKEY_LOCAL_MACHINE,
             L"SYSTEM\\Setup",
             L"SystemPartition",
             Buf,
             _countof(Buf));
    PrintRow(IDS_BOOT_DEV, FALSE, L"%s", Buf);

    /* Getting System Locale */
    if (GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_ILANGUAGE, Tmp, _countof(Tmp)))
    {
        if (RegGetSZ(HKEY_CLASSES_ROOT,
                     L"MIME\\Database\\Rfc1766",
                     Tmp,
                     Buf,
                     _countof(Buf)))
        {
            /* Get rid of @filename,resource */
            lpBuffer = wcschr(Buf, L';');
            if (lpBuffer)
                SHLoadIndirectString(lpBuffer+1, lpBuffer+1, _countof(Buf) - (lpBuffer-Buf) - 1, NULL);

            PrintRow(IDS_SYS_LOCALE, FALSE, L"%s", Buf);
        }
    }

    /* Getting Input Locale */
    if (RegGetSZ(HKEY_CURRENT_USER,
                 L"Keyboard Layout\\Preload",
                 L"1",
                 Tmp,
                 _countof(Tmp)) && wcslen(Tmp) > 4)
    {
        if (RegGetSZ(HKEY_CLASSES_ROOT,
                     L"MIME\\Database\\Rfc1766",
                     Tmp + 4,
                     Buf,
                     _countof(Buf)))
        {
            /* Get rid of @filename,resource */
            lpBuffer = wcschr(Buf, L';');
            if (lpBuffer)
                SHLoadIndirectString(lpBuffer+1, lpBuffer+1, _countof(Buf) - (lpBuffer-Buf) - 1, NULL);

            PrintRow(IDS_INPUT_LOCALE, FALSE, L"%s", Buf);
        }
    }

    /* Getting Time Zone */
    GetTimeZoneInformation(&TimeZoneInfo);

    /* Open Time Zones key */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
                      0,
                      KEY_ENUMERATE_SUB_KEYS|KEY_READ,
                      &hKey) == ERROR_SUCCESS)
    {
        /* Find current timezone */
        UINT i;
        dwCharCount = _countof(Tmp);
        for (i = 0; RegEnumKeyExW(hKey, i, Tmp, &dwCharCount, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; ++i, dwCharCount = 255)
        {
            RegGetSZ(hKey, Tmp, L"Std", Buf, _countof(Buf));

            if (!wcscmp(Buf, TimeZoneInfo.StandardName))
            {
                RegGetSZ(hKey, Tmp, L"Display", Buf, _countof(Buf));

                PrintRow(IDS_TIME_ZONE, FALSE, L"%s", Buf);

                break;
            }
        }
        RegCloseKey(hKey);
    }

    /* Getting Total Physical Memory */
    GlobalMemoryStatus(&MemoryStatus);
    FormatBytes(Buf, MemoryStatus.dwTotalPhys);
    PrintRow(IDS_TOTAL_PHYS_MEM, FALSE, L"%s", Buf);

    /* Getting Available Physical Memory */
    FormatBytes(Buf, MemoryStatus.dwAvailPhys);
    PrintRow(IDS_AVAIL_PHISICAL_MEM, FALSE, L"%s", Buf);

    /* Getting Virtual Memory: Max Size */
    FormatBytes(Buf, MemoryStatus.dwTotalVirtual);
    PrintRow(IDS_VIRT_MEM_MAX, FALSE, L"%s", Buf);

    /* Getting Virtual Memory: Available */
    FormatBytes(Buf, MemoryStatus.dwAvailVirtual);
    PrintRow(IDS_VIRT_MEM_AVAIL, FALSE, L"%s", Buf);

    /* Getting Virtual Memory: In Use */
    FormatBytes(Buf, MemoryStatus.dwTotalVirtual-MemoryStatus.dwAvailVirtual);
    PrintRow(IDS_VIRT_MEM_INUSE, FALSE, L"%s", Buf);

    /* Getting Page File Location(s) */
    if (RegGetSZ(HKEY_LOCAL_MACHINE,
                 L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management",
                 L"PagingFiles",
                 Buf,
                 _countof(Buf)))
    {
        UINT i;
        for (i = 0; Buf[i]; i++)
        {
            if (Buf[i] == L' ')
            {
                Buf[i] = L'\0';
                break;
            }
        }

        PrintRow(IDS_PAGEFILE_LOC, FALSE, L"%s", Buf);
    }

    /* Getting Domain */
    if (NetGetJoinInformation (NULL, &lpBuffer, &NetJoinStatus) == NERR_Success)
    {
        if (NetJoinStatus == NetSetupWorkgroupName || NetJoinStatus == NetSetupDomainName)
            PrintRow(IDS_DOMAIN, FALSE, L"%s", lpBuffer);

        NetApiBufferFree(lpBuffer);
    }

    /* Getting Logon Server */

    /* Getting NetWork Card(s) */
    cbAdapters = 4096;
    pAdapters = malloc(cbAdapters);
    while ((dwResult = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST, NULL, pAdapters, &cbAdapters)) == ERROR_BUFFER_OVERFLOW)
    {
        cbAdapters += 4096;
        pAdapters = (PIP_ADAPTER_ADDRESSES)realloc(pAdapters, cbAdapters);
    }

    if (dwResult == ERROR_SUCCESS)
    {
        PIP_ADAPTER_ADDRESSES pCurrentAdapter = pAdapters;
        UINT cAdapters = 0;

        /* Count adapters */
        for (i = 0; pCurrentAdapter; ++i)
        {
            if (pCurrentAdapter->IfType != IF_TYPE_SOFTWARE_LOOPBACK && pCurrentAdapter->IfType != IF_TYPE_TUNNEL)
                ++cAdapters;
            pCurrentAdapter = pCurrentAdapter->Next;
        }

        /* Print adapters count */
        if (!LoadStringW(GetModuleHandle(NULL), IDS_NETWORK_CARDS_FORMAT, Tmp, _countof(Tmp)))
            Tmp[0] = L'\0';
        swprintf(Buf, Tmp, cAdapters);
        PrintRow(IDS_NETWORK_CARDS, FALSE, L"%s", Buf);

        /* Show information about each adapter */
        pCurrentAdapter = pAdapters;
        for (i = 0; pCurrentAdapter; ++i)
        {
            if (pCurrentAdapter->IfType != IF_TYPE_SOFTWARE_LOOPBACK && pCurrentAdapter->IfType != IF_TYPE_TUNNEL)
            {
                PIP_ADAPTER_UNICAST_ADDRESS pAddress;

                PrintRow(0, FALSE, L"[%02u]: %s", i + 1, pCurrentAdapter->Description);
                PrintRow(IDS_CONNECTION_NAME, TRUE, L"%s", pCurrentAdapter->FriendlyName);
                if (!(pCurrentAdapter->Flags & IP_ADAPTER_DHCP_ENABLED))
                {
                    if (!LoadStringW(GetModuleHandle(NULL), IDS_NO, Buf, _countof(Buf)))
                        Buf[0] = L'\0';
                    PrintRow(IDS_DHCP_ENABLED, TRUE, Buf);
                }
                if (pCurrentAdapter->OperStatus == IfOperStatusDown)
                {
                    if (!LoadStringW(GetModuleHandle(NULL), IDS_MEDIA_DISCONNECTED, Buf, _countof(Buf)))
                        Buf[0] = L'\0';
                    PrintRow(IDS_STATUS, TRUE, Buf);
                }
                else
                {
                    if (!LoadStringW(GetModuleHandle(NULL), IDS_IP_ADDRESSES, Buf, _countof(Buf)))
                        Buf[0] = L'\0';
                    PrintRow(0, TRUE, Buf);
                    pAddress = pCurrentAdapter->FirstUnicastAddress;
                    for (j = 0; pAddress; ++j)
                    {
                        dwCharCount = _countof(Buf);
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
int wmain(int argc, wchar_t *argv[])
{
    WSADATA WsaData;
    int i;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    WSAStartup(MAKEWORD(2, 2), &WsaData);

    for (i = 1; i < argc; ++i)
    {
        if (!wcscmp(argv[i], L"/?") || !wcscmp(argv[i], L"-?"))
        {
            Usage();
            return 0;
        }
        else
        {
            ConPrintf(StdErr, L"Unsupported argument: %s\n", argv[i]);
            return -1;
        }
    }

    AllSysInfo();

    return 0;
}
