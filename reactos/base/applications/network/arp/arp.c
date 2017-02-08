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

/*
 * Globals
 */
const char SEPERATOR = '-';
int _CRT_glob = 0; // stop * from listing dir files in arp -d *

/*
 * function declerations
 */
DWORD DoFormatMessage(VOID);
INT PrintEntries(PMIB_IPNETROW pIpAddRow);
INT DisplayArpEntries(PTCHAR pszInetAddr, PTCHAR pszIfAddr);
INT Addhost(PTCHAR pszInetAddr, PTCHAR pszEthAddr, PTCHAR pszIfAddr);
INT Deletehost(PTCHAR pszInetAddr, PTCHAR pszIfAddr);
VOID Usage(VOID);

/*
 * convert error code into meaningful message
 */
DWORD DoFormatMessage(VOID)
{
    LPVOID lpMsgBuf;
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
            _tprintf(_T("%s"), (LPTSTR)lpMsgBuf);

            LocalFree(lpMsgBuf);
            /* return number of TCHAR's stored in output buffer
             * excluding '\0' - as FormatMessage does*/
            return RetVal;
        }
    }
    return 0;
}

/*
 *
 * Takes an ARP entry and prints the IP address,
 * the MAC address and the entry type to screen
 *
 */
INT PrintEntries(PMIB_IPNETROW pIpAddRow)
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
        case MIB_IPNET_TYPE_DYNAMIC : _tprintf(_T("dynamic\n"));
                                      break;
        case MIB_IPNET_TYPE_STATIC : _tprintf(_T("static\n"));
                                      break;
        case MIB_IPNET_TYPE_INVALID : _tprintf(_T("invalid\n"));
                                      break;
        case MIB_IPNET_TYPE_OTHER : _tprintf(_T("other\n"));
                                      break;
    }
    return EXIT_SUCCESS;
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
INT DisplayArpEntries(PTCHAR pszInetAddr, PTCHAR pszIfAddr)
{
    INT iRet;
    UINT i, k;
    PMIB_IPNETTABLE pIpNetTable = NULL;
    PMIB_IPADDRTABLE pIpAddrTable = NULL;
    ULONG Size = 0;
    struct in_addr inaddr, inaddr2;
    PTCHAR pszIpAddr;
    TCHAR szIntIpAddr[20];

    /* retrieve the IP-to-physical address mapping table */

    /* get table size */
    GetIpNetTable(pIpNetTable, &Size, 0);

    /* allocate memory for ARP address table */
    pIpNetTable = (PMIB_IPNETTABLE) HeapAlloc(GetProcessHeap(), 0, Size);
    if (pIpNetTable == NULL)
        goto cleanup;

    ZeroMemory(pIpNetTable, sizeof(*pIpNetTable));

    if (GetIpNetTable(pIpNetTable, &Size, TRUE) != NO_ERROR)
    {
        _tprintf(_T("failed to allocate memory for GetIpNetTable\n"));
        DoFormatMessage();
        goto cleanup;
    }

    /* check there are entries in the table */
    if (pIpNetTable->dwNumEntries == 0)
    {
        _tprintf(_T("No ARP entires found\n"));
        goto cleanup;
    }



    /* Retrieve the interface-to-ip address mapping
     * table to get the IP address for adapter */

    /* get table size */
    Size = 0;
    GetIpAddrTable(pIpAddrTable, &Size, 0);

    pIpAddrTable = (MIB_IPADDRTABLE *) HeapAlloc(GetProcessHeap(), 0, Size);
    if (pIpAddrTable == NULL)
        goto cleanup;

    ZeroMemory(pIpAddrTable, sizeof(*pIpAddrTable));

    if ((iRet = GetIpAddrTable(pIpAddrTable, &Size, TRUE)) != NO_ERROR)
    {
        _tprintf(_T("GetIpAddrTable failed: %d\n"), iRet);
        DoFormatMessage();
        goto cleanup;
    }


    for (k=0; k < pIpAddrTable->dwNumEntries; k++)
    {
        if (pIpNetTable->table[0].dwIndex == pIpAddrTable->table[k].dwIndex)
        {
            //printf("debug print: pIpAddrTable->table[?].dwIndex = %lx\n", pIpNetTable->table[k].dwIndex);
            inaddr2.s_addr = pIpAddrTable->table[k].dwAddr;
            pszIpAddr = inet_ntoa(inaddr2);
            strcpy(szIntIpAddr, pszIpAddr);
        }
    }


    /* print header, including interface IP address and index number */
    _tprintf(_T("\nInterface: %s --- 0x%lx \n"), szIntIpAddr, pIpNetTable->table[0].dwIndex);
    _tprintf(_T("  Internet Address      Physical Address      Type\n"));

    /* go through all ARP entries */
    for (i=0; i < pIpNetTable->dwNumEntries; i++)
    {

        /* if the user has supplied their own internet addesss *
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

    return EXIT_SUCCESS;

cleanup:
    if (pIpNetTable != NULL)
        HeapFree(GetProcessHeap(), 0, pIpNetTable);
    if (pIpAddrTable != NULL)
        HeapFree(GetProcessHeap(), 0, pIpAddrTable);
    return EXIT_FAILURE;
}

/*
 *
 * Takes an internet address, a MAC address and an optional interface
 * address as arguments and checks their validity.
 * Fill out an MIB_IPNETROW structure and insert the data into the
 * ARP cache as a static entry.
 *
 */
INT Addhost(PTCHAR pszInetAddr, PTCHAR pszEthAddr, PTCHAR pszIfAddr)
{
    PMIB_IPNETROW pAddHost = NULL;
    PMIB_IPNETTABLE pIpNetTable = NULL;
    DWORD dwIpAddr = 0;
    ULONG Size = 0;
    INT i, val, c;

    /* error checking */

    /* check IP address */
    if (pszInetAddr != NULL)
    {
        if ((dwIpAddr = inet_addr(pszInetAddr)) == INADDR_NONE)
        {
            _tprintf(_T("ARP: bad IP address: %s\n"), pszInetAddr);
            return EXIT_FAILURE;
        }
    }
    else
    {
        Usage();
        return EXIT_FAILURE;
    }

    /* check MAC address */
    if (strlen(pszEthAddr) != 17)
    {
        _tprintf(_T("ARP: bad argument: %s\n"), pszEthAddr);
        return EXIT_FAILURE;
    }
    for (i=0; i<17; i++)
    {
        if (pszEthAddr[i] == SEPERATOR)
            continue;

        if (!isxdigit(pszEthAddr[i]))
        {
            _tprintf(_T("ARP: bad argument: %s\n"), pszEthAddr);
            return EXIT_FAILURE;
        }
    }

    /* We need the IpNetTable to get the adapter index */
    /* Return required buffer size */
    GetIpNetTable(pIpNetTable, &Size, 0);

    /* allocate memory for ARP address table */
    pIpNetTable = (PMIB_IPNETTABLE) HeapAlloc(GetProcessHeap(), 0, Size);
    if (pIpNetTable == NULL)
        goto cleanup;

    ZeroMemory(pIpNetTable, sizeof(*pIpNetTable));

    if (GetIpNetTable(pIpNetTable, &Size, TRUE) != NO_ERROR)
    {
        _tprintf(_T("failed to allocate memory for GetIpNetTable\n"));
        DoFormatMessage();
        goto cleanup;
    }


    /* reserve memory on heap and zero */
    pAddHost = (MIB_IPNETROW *) HeapAlloc(GetProcessHeap(), 0, sizeof(MIB_IPNETROW));
    if (pAddHost == NULL)
        goto cleanup;

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
    for (i=0; i<6; i++)
    {
        val =0;
        c = toupper(pszEthAddr[i*3]);
        c = c - (isdigit(c) ? '0' : ('A' - 10));
        val += c;
        val = (val << 4);
        c = toupper(pszEthAddr[i*3 + 1]);
        c = c - (isdigit(c) ? '0' : ('A' - 10));
        val += c;
        pAddHost->bPhysAddr[i] = (BYTE)val;
    }


    /* copy converted IP address */
    pAddHost->dwAddr = dwIpAddr;


    /* set type to static */
    pAddHost->dwType = MIB_IPNET_TYPE_STATIC;


    /* Add the ARP entry */
    if (SetIpNetEntry(pAddHost) != NO_ERROR)
    {
        DoFormatMessage();
        goto cleanup;
    }

    HeapFree(GetProcessHeap(), 0, pAddHost);

    return EXIT_SUCCESS;

cleanup:
    if (pIpNetTable != NULL)
        HeapFree(GetProcessHeap(), 0, pIpNetTable);
    if (pAddHost != NULL)
        HeapFree(GetProcessHeap(), 0, pAddHost);
    return EXIT_FAILURE;
}

/*
 *
 * Takes an internet address and an optional interface address as
 * arguments and checks their validity.
 * Add the interface number and IP to an MIB_IPNETROW structure
 * and remove the entry from the ARP cache.
 *
 */
INT Deletehost(PTCHAR pszInetAddr, PTCHAR pszIfAddr)
{
    PMIB_IPNETROW pDelHost = NULL;
    PMIB_IPNETTABLE pIpNetTable = NULL;
    ULONG Size = 0;
    DWORD dwIpAddr = 0;
    BOOL bFlushTable = FALSE;

    /* error checking */

    /* check IP address */
    if (pszInetAddr != NULL)
    {
        /* if wildcard is given, set flag to delete all hosts */
        if (strncmp(pszInetAddr, "*", 1) == 0)
            bFlushTable = TRUE;
        else if ((dwIpAddr = inet_addr(pszInetAddr)) == INADDR_NONE)
        {
            _tprintf(_T("ARP: bad IP address: %s\n"), pszInetAddr);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        Usage();
        exit(EXIT_FAILURE);
    }

    /* We need the IpNetTable to get the adapter index */
    /* Return required buffer size */
    GetIpNetTable(NULL, &Size, 0);

    /* allocate memory for ARP address table */
    pIpNetTable = (PMIB_IPNETTABLE) HeapAlloc(GetProcessHeap(), 0, Size);
    if (pIpNetTable == NULL)
        goto cleanup;

    ZeroMemory(pIpNetTable, sizeof(*pIpNetTable));

    if (GetIpNetTable(pIpNetTable, &Size, TRUE) != NO_ERROR)
    {
        _tprintf(_T("failed to allocate memory for GetIpNetTable\n"));
        DoFormatMessage();
        goto cleanup;
    }

    /* reserve memory on heap and zero */
    pDelHost = (MIB_IPNETROW *) HeapAlloc(GetProcessHeap(), 0, sizeof(MIB_IPNETROW));
    if (pDelHost == NULL)
        goto cleanup;

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

    if (bFlushTable == TRUE)
    {
        /* delete arp cache */
        if (FlushIpNetTable(pDelHost->dwIndex) != NO_ERROR)
        {
            DoFormatMessage();
            goto cleanup;
        }
        else
        {
            HeapFree(GetProcessHeap(), 0, pDelHost);
            return EXIT_SUCCESS;
        }
    }
    else
        /* copy converted IP address */
        pDelHost->dwAddr = dwIpAddr;

    /* Add the ARP entry */
    if (DeleteIpNetEntry(pDelHost) != NO_ERROR)
    {
        DoFormatMessage();
        goto cleanup;
    }

    HeapFree(GetProcessHeap(), 0, pDelHost);

    return EXIT_SUCCESS;

cleanup:
    if (pIpNetTable != NULL)
        HeapFree(GetProcessHeap(), 0, pIpNetTable);
    if (pDelHost != NULL)
        HeapFree(GetProcessHeap(), 0, pDelHost);
    return EXIT_FAILURE;
}

/*
 *
 * print program usage to screen
 *
 */
VOID Usage(VOID)
{
    _tprintf(_T("\nDisplays and modifies the IP-to-Physical address translation tables used by\n"
                "address resolution protocol (ARP).\n"
                "\n"
                "ARP -s inet_addr eth_addr [if_addr]\n"
                "ARP -d inet_addr [if_addr]\n"
                "ARP -a [inet_addr] [-N if_addr]\n"
                "\n"
                "  -a            Displays current ARP entries by interrogating the current\n"
                "                protocol data.  If inet_addr is specified, the IP and Physical\n"
                "                addresses for only the specified computer are displayed.  If\n"
                "                more than one network interface uses ARP, entries for each ARP\n"
                "                table are displayed.\n"
                "  -g            Same as -a.\n"
                "  inet_addr     Specifies an internet address.\n"
                "  -N if_addr    Displays the ARP entries for the network interface specified\n"
                "                by if_addr.\n"
                "  -d            Deletes the host specified by inet_addr. inet_addr may be\n"
                "                wildcarded with * to delete all hosts.\n"
                "  -s            Adds the host and associates the Internet address inet_addr\n"
                "                with the Physical address eth_addr.  The Physical address is\n"
                "                given as 6 hexadecimal bytes separated by hyphens. The entry\n"
                "                is permanent.\n"
                "  eth_addr      Specifies a physical address.\n"
                "  if_addr       If present, this specifies the Internet address of the\n"
                "                interface whose address translation table should be modified.\n"
                "                If not present, the first applicable interface will be used.\n"
                "Example:\n"
                "  > arp -s 157.55.85.212   00-aa-00-62-c6-09  .... Adds a static entry.\n"
                "  > arp -a                                    .... Displays the arp table.\n\n"));
}

/*
 *
 * Program entry.
 * Parse command line and call the required function
 *
 */
INT main(int argc, char* argv[])
{
    if ((argc < 2) || (argc > 5))
    {
       Usage();
       return EXIT_FAILURE;
    }

    if (argv[1][0] == '-')
    {
        switch (argv[1][1])
        {
           case 'a': /* fall through */
           case 'g':
                     if (argc == 2)
                         DisplayArpEntries(NULL, NULL);
                     else if (argc == 3)
                         DisplayArpEntries(argv[2], NULL);
                     else if ((argc == 4) && ((strcmp(argv[2], "-N")) == 0))
                         DisplayArpEntries(NULL, argv[3]);
                     else if ((argc == 5) && ((strcmp(argv[3], "-N")) == 0))
                         DisplayArpEntries(argv[2], argv[4]);
                     else
                         Usage();
                         return EXIT_FAILURE;
                     break;
           case 'd': if (argc == 3)
                         Deletehost(argv[2], NULL);
                     else if (argc == 4)
                         Deletehost(argv[2], argv[3]);
                     else
                         Usage();
                         return EXIT_FAILURE;
                     break;
           case 's': if (argc == 4)
                         Addhost(argv[2], argv[3], NULL);
                     else if (argc == 5)
                         Addhost(argv[2], argv[3], argv[4]);
                     else
                         Usage();
                         return EXIT_FAILURE;
                     break;
           default:
              Usage();
              return EXIT_FAILURE;
        }
    }
    else
        Usage();

    return EXIT_SUCCESS;
}
