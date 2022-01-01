/*
 *  ReactOS Win32 Applications
 *  Copyright (C) 2005 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS arp utility
 * FILE:        base/applications/network/arp/arp.c
 * PURPOSE:     view and manipulate the ARP cache
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *   GM 27/06/05 Created
 *
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#define _INC_WINDOWS
#include <winsock2.h>
#include <iphlpapi.h>

#include <arp_msg.h>

/*
 * Globals
 */
const char SEPARATOR = '-';
int _CRT_glob = 0; // stop * from listing dir files in arp -d *

/*
 * function declarations
 */
DWORD DoFormatMessage(VOID);
DWORD PrintEntries(PMIB_IPNETROW pIpAddRow);
DWORD DisplayArpEntries(PTCHAR pszInetAddr, PTCHAR pszIfAddr);
DWORD Addhost(PTCHAR pszInetAddr, PTCHAR pszEthAddr, PTCHAR pszIfAddr);
DWORD Deletehost(PTCHAR pszInetAddr, PTCHAR pszIfAddr);
VOID Usage(VOID);

/*
 * convert error code into meaningful message
 */
DWORD DoFormatMessage(VOID)
{
    LPTSTR lpMsgBuf;
    DWORD RetVal;

    DWORD ErrorCode = GetLastError();

    if (ErrorCode != ERROR_SUCCESS)
    {
        RetVal = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL,
                               ErrorCode,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
                               (LPTSTR) &lpMsgBuf,
                               0,
                               NULL );

        if (RetVal != 0)
        {
            _tprintf(_T("%s"), lpMsgBuf);
            LocalFree(lpMsgBuf);
            /* return number of TCHAR's stored in output buffer
             * excluding '\0' - as FormatMessage does*/
            return RetVal;
        }
    }
    return 0;
}

VOID
PrintMessage(
    DWORD dwMessage)
{
    LPTSTR lpMsgBuf;
    DWORD RetVal;

    RetVal = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                           GetModuleHandleW(NULL),
                           dwMessage,
                           LANG_USER_DEFAULT,
                           (LPTSTR)&lpMsgBuf,
                           0,
                           NULL);
    if (RetVal != 0)
    {
        _tprintf(_T("%s"), lpMsgBuf);
        LocalFree(lpMsgBuf);
    }
}

VOID
PrintMessageV(
    DWORD dwMessage,
    ...)
{
    LPTSTR lpMsgBuf;
    va_list args = NULL;
    DWORD RetVal;

    va_start(args, dwMessage);

    RetVal = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                           GetModuleHandleW(NULL),
                           dwMessage,
                           LANG_USER_DEFAULT,
                           (LPTSTR)&lpMsgBuf,
                           0,
                           &args);
    va_end(args);

    if (RetVal != 0)
    {
        _tprintf(_T("%s"), lpMsgBuf);
        LocalFree(lpMsgBuf);
    }
}

/*
 *
 * Takes an ARP entry and prints the IP address,
 * the MAC address and the entry type to screen
 *
 */
DWORD PrintEntries(PMIB_IPNETROW pIpAddRow)
{
    IN_ADDR inaddr;
    TCHAR cMacAddr[20];

    /* print IP addresses */
    inaddr.S_un.S_addr = pIpAddRow->dwAddr;
    _tprintf(_T("  %-22s"), inet_ntoa(inaddr));

    /* print MAC address */
    _stprintf(cMacAddr, _T("%02x-%02x-%02x-%02x-%02x-%02x"),
        pIpAddRow->bPhysAddr[0],
        pIpAddRow->bPhysAddr[1],
        pIpAddRow->bPhysAddr[2],
        pIpAddRow->bPhysAddr[3],
        pIpAddRow->bPhysAddr[4],
        pIpAddRow->bPhysAddr[5]);
    _tprintf(_T("%-22s"), cMacAddr);

    /* print cache type */
    switch (pIpAddRow->dwType)
    {
        case MIB_IPNET_TYPE_DYNAMIC:
            PrintMessage(MSG_ARP_DYNAMIC);
            break;

        case MIB_IPNET_TYPE_STATIC:
            PrintMessage(MSG_ARP_STATIC);
            break;

        case MIB_IPNET_TYPE_INVALID:
            PrintMessage(MSG_ARP_INVALID);
            break;

        case MIB_IPNET_TYPE_OTHER:
            PrintMessage(MSG_ARP_OTHER);
            break;
    }
    _putts(_T(""));
    return NO_ERROR;
}

/*
 *
 * Takes optional parameters of an internet address and interface address.
 * Retrieve all entries in the ARP cache. If an internet address is
 * specified, display the ARP entry relating to that address. If an
 * interface address is specified, display all entries relating to
 * that interface.
 *
 */
/* FIXME: allow user to specify an interface address, via pszIfAddr */
DWORD DisplayArpEntries(PTCHAR pszInetAddr, PTCHAR pszIfAddr)
{
    DWORD i, k, dwCount;
    PMIB_IPNETTABLE pIpNetTable = NULL;
    PMIB_IPADDRTABLE pIpAddrTable = NULL;
    ULONG Size = 0;
    struct in_addr inaddr, inaddr2;
    PTCHAR pszIpAddr;
    TCHAR szIntIpAddr[20];
    DWORD dwError = NO_ERROR;

    /* retrieve the IP-to-physical address mapping table */

    /* get table size */
    GetIpNetTable(pIpNetTable, &Size, 0);

    /* allocate memory for ARP address table */
    pIpNetTable = (PMIB_IPNETTABLE)HeapAlloc(GetProcessHeap(), 0, Size);
    if (pIpNetTable == NULL)
    {
        PrintMessage(MSG_ARP_NO_MEMORY);
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    ZeroMemory(pIpNetTable, sizeof(*pIpNetTable));

    dwError = GetIpNetTable(pIpNetTable, &Size, TRUE);
    if (dwError != NO_ERROR)
    {
        _tprintf(_T("GetIpNetTable failed: %lu\n"), dwError);
        DoFormatMessage();
        goto cleanup;
    }

    /* check there are entries in the table */
    if (pIpNetTable->dwNumEntries == 0)
    {
        PrintMessage(MSG_ARP_NO_ENTRIES);
        goto cleanup;
    }

    /* Retrieve the interface-to-ip address mapping
     * table to get the IP address for adapter */

    /* get table size */
    Size = 0;
    GetIpAddrTable(pIpAddrTable, &Size, 0);

    pIpAddrTable = (PMIB_IPADDRTABLE)HeapAlloc(GetProcessHeap(), 0, Size);
    if (pIpAddrTable == NULL)
    {
        PrintMessage(MSG_ARP_NO_MEMORY);
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    ZeroMemory(pIpAddrTable, sizeof(*pIpAddrTable));

    dwError = GetIpAddrTable(pIpAddrTable, &Size, TRUE);
    if (dwError != NO_ERROR)
    {
        _tprintf(_T("GetIpAddrTable failed: %lu\n"), dwError);
        DoFormatMessage();
        goto cleanup;
    }

    for (k = 0; k < pIpAddrTable->dwNumEntries; k++)
    {
        if (pIpNetTable->table[0].dwIndex == pIpAddrTable->table[k].dwIndex)
        {
            //printf("debug print: pIpAddrTable->table[?].dwIndex = %lx\n", pIpNetTable->table[k].dwIndex);
            inaddr2.s_addr = pIpAddrTable->table[k].dwAddr;
            pszIpAddr = inet_ntoa(inaddr2);
            strcpy(szIntIpAddr, pszIpAddr);
        }
    }

    /* Count relevant ARP entries */
    dwCount = 0;
    for (i = 0; i < pIpNetTable->dwNumEntries; i++)
    {
        /* if the user has supplied their own internet address *
         * only count the arp entry which matches that */
        if (pszInetAddr)
        {
            inaddr.S_un.S_addr = pIpNetTable->table[i].dwAddr;
            pszIpAddr = inet_ntoa(inaddr);

            /* check if it matches, count it */
            if (strcmp(pszIpAddr, pszInetAddr) == 0)
                dwCount++;
        }
        else
        {
            /* if an address is not supplied, count all entries */
            dwCount++;
        }
    }

    /* Print message and leave if there are no relevant ARP entries */
    if (dwCount == 0)
    {
        PrintMessage(MSG_ARP_NO_ENTRIES);
        goto cleanup;
    }

    /* print header, including interface IP address and index number */
    PrintMessageV(MSG_ARP_INTERFACE, szIntIpAddr, pIpNetTable->table[0].dwIndex);

    /* go through all ARP entries */
    for (i = 0; i < pIpNetTable->dwNumEntries; i++)
    {

        /* if the user has supplied their own internet address *
         * only print the arp entry which matches that */
        if (pszInetAddr)
        {
            inaddr.S_un.S_addr = pIpNetTable->table[i].dwAddr;
            pszIpAddr = inet_ntoa(inaddr);

            /* check if it matches, print it */
            if (strcmp(pszIpAddr, pszInetAddr) == 0)
                PrintEntries(&pIpNetTable->table[i]);
        }
        else
            /* if an address is not supplied, print all entries */
            PrintEntries(&pIpNetTable->table[i]);
    }

cleanup:
    if (pIpNetTable != NULL)
        HeapFree(GetProcessHeap(), 0, pIpNetTable);
    if (pIpAddrTable != NULL)
        HeapFree(GetProcessHeap(), 0, pIpAddrTable);

    return dwError;
}

/*
 *
 * Takes an internet address, a MAC address and an optional interface
 * address as arguments and checks their validity.
 * Fill out an MIB_IPNETROW structure and insert the data into the
 * ARP cache as a static entry.
 *
 */
DWORD Addhost(PTCHAR pszInetAddr, PTCHAR pszEthAddr, PTCHAR pszIfAddr)
{
    PMIB_IPNETROW pAddHost = NULL;
    PMIB_IPNETTABLE pIpNetTable = NULL;
    DWORD dwIpAddr = 0;
    ULONG Size = 0;
    INT i, val, c;
    DWORD dwError = NO_ERROR;

    /* error checking */

    /* check IP address */
    if (pszInetAddr == NULL)
    {
        Usage();
        return ERROR_INVALID_PARAMETER;
    }

    dwIpAddr = inet_addr(pszInetAddr);
    if (dwIpAddr == INADDR_NONE)
    {
        PrintMessageV(MSG_ARP_BAD_IP_ADDRESS, pszInetAddr);
        return ERROR_INVALID_PARAMETER;
    }

    /* check MAC address */
    if (strlen(pszEthAddr) != 17)
    {
        PrintMessageV(MSG_ARP_BAD_ARGUMENT, pszEthAddr);
        return ERROR_INVALID_PARAMETER;
    }

    for (i = 0; i < 17; i++)
    {
        if (pszEthAddr[i] == SEPARATOR)
            continue;

        if (!isxdigit(pszEthAddr[i]))
        {
            PrintMessageV(MSG_ARP_BAD_ARGUMENT, pszEthAddr);
            return ERROR_INVALID_PARAMETER;
        }
    }

    /* We need the IpNetTable to get the adapter index */
    /* Return required buffer size */
    GetIpNetTable(pIpNetTable, &Size, 0);

    /* allocate memory for ARP address table */
    pIpNetTable = (PMIB_IPNETTABLE)HeapAlloc(GetProcessHeap(), 0, Size);
    if (pIpNetTable == NULL)
    {
        PrintMessage(MSG_ARP_NO_MEMORY);
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    ZeroMemory(pIpNetTable, sizeof(*pIpNetTable));

    dwError = GetIpNetTable(pIpNetTable, &Size, TRUE);
    if (dwError != NO_ERROR)
    {
        _tprintf(_T("GetIpNetTable failed: %lu\n"), dwError);
        DoFormatMessage();
        goto cleanup;
    }

    /* reserve memory on heap and zero */
    pAddHost = (PMIB_IPNETROW)HeapAlloc(GetProcessHeap(), 0, sizeof(MIB_IPNETROW));
    if (pAddHost == NULL)
    {
        PrintMessage(MSG_ARP_NO_MEMORY);
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    ZeroMemory(pAddHost, sizeof(MIB_IPNETROW));

    /* set dwIndex field to the index of a local IP address to
     * indicate the network on which the ARP entry applies */
    if (pszIfAddr)
    {
        if (sscanf(pszIfAddr, "%lx", &pAddHost->dwIndex) == EOF)
        {
            goto cleanup;
        }
    }
    else
    {
        //printf("debug print: pIpNetTable->table[0].dwIndex = %lx\n", pIpNetTable->table[0].dwIndex);
        /* needs testing. I get the correct index on my machine, but need others
         * to test their card index.  Any problems and we can use GetAdaptersInfo instead */
        pAddHost->dwIndex = pIpNetTable->table[0].dwIndex;
    }

    /* Set MAC address to 6 bytes (typical) */
    pAddHost->dwPhysAddrLen = 6;


    /* Encode bPhysAddr into correct byte array */
    for (i = 0; i < 6; i++)
    {
        val = 0;
        c = toupper(pszEthAddr[i * 3]);
        c = c - (isdigit(c) ? '0' : ('A' - 10));
        val += c;
        val = (val << 4);
        c = toupper(pszEthAddr[i * 3 + 1]);
        c = c - (isdigit(c) ? '0' : ('A' - 10));
        val += c;
        pAddHost->bPhysAddr[i] = (BYTE)val;
    }

    /* copy converted IP address */
    pAddHost->dwAddr = dwIpAddr;


    /* set type to static */
    pAddHost->dwType = MIB_IPNET_TYPE_STATIC;


    /* Add the ARP entry */
    dwError = SetIpNetEntry(pAddHost);
    if (dwError != NO_ERROR)
    {
        DoFormatMessage();
        goto cleanup;
    }

cleanup:
    if (pIpNetTable != NULL)
        HeapFree(GetProcessHeap(), 0, pIpNetTable);
    if (pAddHost != NULL)
        HeapFree(GetProcessHeap(), 0, pAddHost);

    return dwError;
}

/*
 *
 * Takes an internet address and an optional interface address as
 * arguments and checks their validity.
 * Add the interface number and IP to an MIB_IPNETROW structure
 * and remove the entry from the ARP cache.
 *
 */
DWORD Deletehost(PTCHAR pszInetAddr, PTCHAR pszIfAddr)
{
    PMIB_IPNETROW pDelHost = NULL;
    PMIB_IPNETTABLE pIpNetTable = NULL;
    ULONG Size = 0;
    DWORD dwIpAddr = 0;
    BOOL bFlushTable = FALSE;
    DWORD dwError = NO_ERROR;

    /* error checking */

    /* check IP address */
    if (pszInetAddr == NULL)
    {
        Usage();
        return ERROR_INVALID_PARAMETER;
    }

    /* if wildcard is given, set flag to delete all hosts */
    if (strncmp(pszInetAddr, "*", 1) == 0)
    {
        bFlushTable = TRUE;
    }
    else
    {
        dwIpAddr = inet_addr(pszInetAddr);
        if (dwIpAddr == INADDR_NONE)
        {
            PrintMessageV(MSG_ARP_BAD_IP_ADDRESS, pszInetAddr);
            return ERROR_INVALID_PARAMETER;
        }
    }

    /* We need the IpNetTable to get the adapter index */
    /* Return required buffer size */
    GetIpNetTable(NULL, &Size, 0);

    /* allocate memory for ARP address table */
    pIpNetTable = (PMIB_IPNETTABLE) HeapAlloc(GetProcessHeap(), 0, Size);
    if (pIpNetTable == NULL)
    {
        PrintMessage(MSG_ARP_NO_MEMORY);
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    ZeroMemory(pIpNetTable, sizeof(*pIpNetTable));

    dwError = GetIpNetTable(pIpNetTable, &Size, TRUE);
    if (dwError != NO_ERROR)
    {
        _tprintf(_T("GetIpNetTable failed: %lu\n"), dwError);
        DoFormatMessage();
        goto cleanup;
    }

    /* reserve memory on heap and zero */
    pDelHost = (MIB_IPNETROW *)HeapAlloc(GetProcessHeap(), 0, sizeof(MIB_IPNETROW));
    if (pDelHost == NULL)
    {
        PrintMessage(MSG_ARP_NO_MEMORY);
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    ZeroMemory(pDelHost, sizeof(MIB_IPNETROW));

    /* set dwIndex field to the index of a local IP address to
     * indicate the network on which the ARP entry applies */
    if (pszIfAddr)
    {
        if (sscanf(pszIfAddr, "%lx", &pDelHost->dwIndex) == EOF)
        {
            goto cleanup;
        }
    }
    else
    {
        /* needs testing. I get the correct index on my machine, but need others
         * to test their card index. Any problems and we can use GetAdaptersInfo instead */
        pDelHost->dwIndex = pIpNetTable->table[0].dwIndex;
    }

    if (bFlushTable != FALSE)
    {
        /* delete arp cache */
        dwError = FlushIpNetTable(pDelHost->dwIndex);
        if (dwError != NO_ERROR)
        {
            DoFormatMessage();
            goto cleanup;
        }
    }
    else
    {
        /* copy converted IP address */
        pDelHost->dwAddr = dwIpAddr;

        /* Delete the ARP entry */
        dwError = DeleteIpNetEntry(pDelHost);
        if (dwError != NO_ERROR)
        {
            DoFormatMessage();
            goto cleanup;
        }
    }

cleanup:
    if (pIpNetTable != NULL)
        HeapFree(GetProcessHeap(), 0, pIpNetTable);
    if (pDelHost != NULL)
        HeapFree(GetProcessHeap(), 0, pDelHost);

    return dwError;
}

/*
 *
 * print program usage to screen
 *
 */
VOID Usage(VOID)
{
    PrintMessage(MSG_ARP_SYNTAX);
}

/*
 *
 * Program entry.
 * Parse command line and call the required function
 *
 */
INT main(int argc, char* argv[])
{
    DWORD dwError = NO_ERROR;

    if ((argc < 2) || (argc > 5))
    {
       Usage();
       return EXIT_FAILURE;
    }

    if (argv[1][0] != '-')
    {
        Usage();
        return EXIT_SUCCESS;
    }

    switch (argv[1][1])
    {
        case 'a': /* fall through */
        case 'g':
            if (argc == 2)
                dwError = DisplayArpEntries(NULL, NULL);
            else if (argc == 3)
                dwError = DisplayArpEntries(argv[2], NULL);
            else if ((argc == 4) && ((strcmp(argv[2], "-N")) == 0))
                dwError = DisplayArpEntries(NULL, argv[3]);
            else if ((argc == 5) && ((strcmp(argv[3], "-N")) == 0))
                dwError = DisplayArpEntries(argv[2], argv[4]);
            else
            {
                Usage();
                dwError = ERROR_INVALID_PARAMETER;
            }
            break;

        case 'd':
            if (argc == 3)
                dwError = Deletehost(argv[2], NULL);
            else if (argc == 4)
                dwError = Deletehost(argv[2], argv[3]);
            else
            {
                Usage();
                dwError = ERROR_INVALID_PARAMETER;
            }
            break;

        case 's':
            if (argc == 4)
                dwError = Addhost(argv[2], argv[3], NULL);
            else if (argc == 5)
                dwError = Addhost(argv[2], argv[3], argv[4]);
            else
            {
                Usage();
                dwError = ERROR_INVALID_PARAMETER;
            }
            break;

        default:
            Usage();
            dwError = ERROR_INVALID_PARAMETER;
            break;
    }

    return (dwError == NO_ERROR) ? EXIT_SUCCESS : EXIT_FAILURE;
}
