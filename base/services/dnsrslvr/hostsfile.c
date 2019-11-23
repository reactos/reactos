/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        base/services/dnsrslvr/hostsfile.c
 * PURPOSE:     HOSTS file routines
 * PROGRAMERS:  Art Yerkes
 *              Eric Kohl
 */

#include "precomp.h"


#define NDEBUG
#include <debug.h>


static
PWSTR
AnsiToUnicode(
    PSTR NarrowString)
{
    PWSTR WideString;
    int WideLen;

    WideLen = MultiByteToWideChar(CP_ACP,
                                  0,
                                  NarrowString,
                                  -1,
                                  NULL,
                                  0);
    if (WideLen == 0)
        return NULL;

    WideString = HeapAlloc(GetProcessHeap(),
                           0,
                           WideLen * sizeof(WCHAR));
    if (WideString == NULL)
        return NULL;

    MultiByteToWideChar(CP_ACP,
                        0,
                        NarrowString,
                        -1,
                        WideString,
                        WideLen);

    return WideString;
}


static
BOOL
ParseV4Address(
    LPCSTR AddressString,
    OUT PDWORD pAddress)
{
    CHAR *cp;
    DWORD val, base;
    unsigned char c;
    DWORD parts[4], *pp = parts;

    cp = (CHAR *)AddressString;

    if (!AddressString)
        return FALSE;

    if (!isdigit(*cp))
        return FALSE;

again:
    /*
    * Collect number up to ``.''.
    * Values are specified as for C:
    * 0x=hex, 0=octal, other=decimal.
    */
    val = 0; base = 10;
    if (*cp == '0')
    {
        if (*++cp == 'x' || *cp == 'X')
            base = 16, cp++;
        else
            base = 8;
    }

    while ((c = *cp))
    {
        if (isdigit(c))
        {
            val = (val * base) + (c - '0');
            cp++;
            continue;
        }

        if (base == 16 && isxdigit(c))
        {
            val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
            cp++;
            continue;
        }
        break;
    }

    if (*cp == '.')
    {
        /*
        * Internet format:
        *    a.b.c.d
        */
        if (pp >= parts + 4)
            return FALSE;
        *pp++ = val;
        cp++;
        goto again;
    }

    /* Check for trailing characters */
    if (*cp && *cp > ' ')
        return FALSE;

    if (pp >= parts + 4)
        return FALSE;

    *pp++ = val;
    /*
    * Concoct the address according to
    * the number of parts specified.
    */
    if ((DWORD)(pp - parts) != 4)
        return FALSE;

    if (parts[0] > 0xff || parts[1] > 0xff || parts[2] > 0xff || parts[3] > 0xff)
        return FALSE;

    val = (parts[3] << 24) | (parts[2] << 16) | (parts[1] << 8) | parts[0];

    if (pAddress)
        *pAddress = val;

    return TRUE;
}


static
VOID
AddV4HostEntries(
    PWSTR pszHostName,
    DWORD dwIpAddress)
{
    DNS_RECORDW ARecord, PtrRecord;
    WCHAR szInAddrArpaName[32];

    /* Prepare the A record */
    ZeroMemory(&ARecord, sizeof(DNS_RECORDW));

    ARecord.pName = pszHostName;
    ARecord.wType = DNS_TYPE_A;
    ARecord.wDataLength = sizeof(DNS_A_DATA);
    ARecord.dwTtl = 86400;

    ARecord.Data.A.IpAddress = dwIpAddress;

    swprintf(szInAddrArpaName,
             L"%u.%u.%u.%u.in-addr.arpa",
             (dwIpAddress >> 24) & 0xFF,
             (dwIpAddress >> 16) & 0xFF,
             (dwIpAddress >> 8) & 0xFF,
             dwIpAddress & 0xFF);

    /* Prepare the PTR record */
    ZeroMemory(&PtrRecord, sizeof(DNS_RECORDW));

    PtrRecord.pName = szInAddrArpaName;
    PtrRecord.wType = DNS_TYPE_PTR;
    PtrRecord.wDataLength = sizeof(DNS_PTR_DATA);
    PtrRecord.dwTtl = 86400;

    PtrRecord.Data.PTR.pNameHost = pszHostName;

    DnsIntCacheAddEntry(&ARecord);
    DnsIntCacheAddEntry(&PtrRecord);
}


static
FILE *
OpenHostsFile(VOID)
{
    PWSTR ExpandedPath;
    PWSTR DatabasePath;
    HKEY DatabaseKey;
    DWORD RegSize = 0;
    size_t StringLength;
    FILE *pHostsFile;
    DWORD dwError;

    ExpandedPath = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    if (ExpandedPath == NULL)
        return NULL;

    /* Open the database path key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                            0,
                            KEY_READ,
                            &DatabaseKey);
    if (dwError == ERROR_SUCCESS)
    {
        /* Read the actual path */
        RegQueryValueExW(DatabaseKey,
                         L"DatabasePath",
                         NULL,
                         NULL,
                         NULL,
                         &RegSize);

        DatabasePath = HeapAlloc(GetProcessHeap(), 0, RegSize);
        if (DatabasePath == NULL)
        {
            HeapFree(GetProcessHeap(), 0, ExpandedPath);
            RegCloseKey(DatabaseKey);
            return NULL;
        }

        /* Read the actual path */
        dwError = RegQueryValueExW(DatabaseKey,
                                   L"DatabasePath",
                                   NULL,
                                   NULL,
                                   (LPBYTE)DatabasePath,
                                   &RegSize);

        /* Close the key */
        RegCloseKey(DatabaseKey);

        if (dwError != ERROR_SUCCESS)
        {
            HeapFree(GetProcessHeap(), 0, DatabasePath);
            HeapFree(GetProcessHeap(), 0, ExpandedPath);
            return NULL;
        }

        /* Expand the name */
        ExpandEnvironmentStringsW(DatabasePath, ExpandedPath, MAX_PATH);

        HeapFree(GetProcessHeap(), 0, DatabasePath);
    }
    else
    {
        /* Use defalt path */
        GetSystemDirectoryW(ExpandedPath, MAX_PATH);

        StringCchLengthW(ExpandedPath, MAX_PATH, &StringLength);
        if (ExpandedPath[StringLength - 1] != L'\\')
        {
            /* It isn't, so add it ourselves */
            StringCchCatW(ExpandedPath, MAX_PATH, L"\\");
        }

        StringCchCatW(ExpandedPath, MAX_PATH, L"drivers\\etc\\");
    }

    /* Make sure that the path is backslash-terminated */
    StringCchLengthW(ExpandedPath, MAX_PATH, &StringLength);
    if (ExpandedPath[StringLength - 1] != L'\\')
    {
        /* It isn't, so add it ourselves */
        StringCchCatW(ExpandedPath, MAX_PATH, L"\\");
    }

    /* Add the database name */
    StringCchCatW(ExpandedPath, MAX_PATH, L"hosts");

    /* Open the hosts file */
    pHostsFile = _wfopen(ExpandedPath, L"r");

    HeapFree(GetProcessHeap(), 0, ExpandedPath);

    return pHostsFile;
}


BOOL
ReadHostsFile(VOID)
{
    CHAR szLineBuffer[512];
    FILE *pHostFile = NULL;
    CHAR *Ptr, *NameStart, *NameEnd, *AddressStart, *AddressEnd;
    DWORD Address;
    PWSTR pszHostName;

    pHostFile = OpenHostsFile();
    if (pHostFile == NULL)
        return FALSE;

    for (;;)
    {
        /* Read a line */
        if (fgets(szLineBuffer, sizeof(szLineBuffer), pHostFile) == NULL)
            break;

        NameStart = NameEnd = NULL;
        AddressStart = AddressEnd = NULL;

        /* Search for the start of the ip address */
        Ptr = szLineBuffer;
        for (;;)
        {
            if (*Ptr == 0 || *Ptr == '#')
                break;

            if (!isspace(*Ptr))
            {
                AddressStart = Ptr;
                Ptr = Ptr + 1;
                break;
            }

            Ptr = Ptr + 1;
        }

        /* Search for the end of the ip address */
        for (;;)
        {
            if (*Ptr == 0 || *Ptr == '#')
                break;

            if (isspace(*Ptr))
            {
                AddressEnd = Ptr;
                Ptr = Ptr + 1;
                break;
            }

            Ptr = Ptr + 1;
        }

        /* Search for the start of the name */
        for (;;)
        {
            if (*Ptr == 0 || *Ptr == '#')
                break;

            if (!isspace(*Ptr))
            {
                NameStart = Ptr;
                Ptr = Ptr + 1;
                break;
            }

            Ptr = Ptr + 1;
        }

        /* Search for the end of the name */
        for (;;)
        {
            if (*Ptr == 0 || *Ptr == '#')
                break;

            if (isspace(*Ptr))
            {
                NameEnd = Ptr;
                break;
            }

            Ptr = Ptr + 1;
        }

        if (AddressStart == NULL || AddressEnd == NULL ||
            NameStart == NULL || NameEnd == NULL)
            continue;

        *AddressEnd = 0;
        *NameEnd = 0;

        DPRINT("%s ==> %s\n", NameStart, AddressStart);

        if (ParseV4Address(AddressStart, &Address))
        {
            DPRINT("IP4: %s ==> 0x%08lx\n", AddressStart, Address);

            pszHostName = AnsiToUnicode(NameStart);
            if (pszHostName != NULL)
            {
                AddV4HostEntries(pszHostName, Address);

                HeapFree(GetProcessHeap(), 0, pszHostName);
            }
        }
    }

    fclose(pHostFile);

    return TRUE;
}

/* EOF */
