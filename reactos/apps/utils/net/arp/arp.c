/* 
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS arp utility
 * FILE:        apps/utils/net/arp/arp.c
 * PURPOSE:     view and manipulate the ARP cache
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *   GM 27/06/05 Created
 *
 */
 
 
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <string.h>
#include <ctype.h>
#include <winsock2.h>
#include <iphlpapi.h>
 
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
 
/*
 * Globals
 */ 
const char SEPERATOR = '-';
 
 
 
/*
 * function declerations
 */ 
INT DisplayArpEntries(PTCHAR pszInetAddr, PTCHAR pszIfAddr);
INT PrintEntries(PMIB_IPNETROW pIpAddRow);
INT Addhost(PTCHAR pszInetAddr, PTCHAR pszEthAddr, PTCHAR pszIfAddr);
INT Deletehost(PTCHAR pszInetAddr, PTCHAR pszIfAddr);
VOID Usage(VOID);
 
 
 
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
    PMIB_IPNETTABLE pIpNetTable;
    PMIB_IPADDRTABLE pIpAddrTable;
    ULONG ulSize = 0;
    struct in_addr inaddr, inaddr2;
    DWORD dwSize = 0;
    PTCHAR pszIpAddr;
    TCHAR szIntIpAddr[20];
 
    /* Return required buffer size */
    GetIpNetTable(NULL, &ulSize, 0);
 
    /* allocate memory for ARP address table */
    pIpNetTable = (PMIB_IPNETTABLE) malloc(ulSize * sizeof(BYTE));
    ZeroMemory(pIpNetTable, sizeof(*pIpNetTable));
 
    /* get Arp address table */
    if (pIpNetTable != NULL) {
        GetIpNetTable(pIpNetTable, &ulSize, TRUE);
    } else {
        _tprintf(_T("failed to allocate memory for GetIpNetTable\n"));
        free(pIpNetTable);
        return -1;
    }
 
    /* check there are entries in the table */ 
    if (pIpNetTable->dwNumEntries == 0) {
        _tprintf(_T("No ARP entires found\n"));
        free(pIpNetTable);
        return -1;
    }
 
 
 
    /* try doing this in the way it's done above, it's clearer */
    /* Retrieve the interface-to-ip address mapping
     * table to get the IP address for adapter */    
    pIpAddrTable = (MIB_IPADDRTABLE *) malloc(dwSize);
    GetIpAddrTable(pIpAddrTable, &dwSize, 0);   // NULL ?
 

    pIpAddrTable = (MIB_IPADDRTABLE *) malloc(dwSize);
    //ZeroMemory(pIpAddrTable, sizeof(*pIpAddrTable));
 
    if ((iRet = GetIpAddrTable(pIpAddrTable, &dwSize, TRUE)) != NO_ERROR) { // NO_ERROR = 0
        _tprintf(_T("GetIpAddrTable failed: %d\n"), iRet);
        _tprintf(_T("error: %d\n"), WSAGetLastError());
    }
 
 
    for (k=0; k < pIpAddrTable->dwNumEntries; k++) {
        if (pIpNetTable->table[0].dwIndex == pIpAddrTable->table[k].dwIndex) {
            //printf("printing pIpAddrTable->table[?].dwIndex = %lx\n", pIpNetTable->table[k].dwIndex);
            inaddr2.s_addr = pIpAddrTable->table[k].dwAddr;
            pszIpAddr = inet_ntoa(inaddr2);
            strcpy(szIntIpAddr, pszIpAddr);
        }
    }   
 
 
    /* print header, including interface IP address and index number */
    _tprintf(_T("\nInterface: %s --- 0x%lx \n"), szIntIpAddr, pIpNetTable->table[0].dwIndex);
    _tprintf(_T("  Internet Address      Physical Address      Type\n"));
 
    /* go through all ARP entries */
    for (i=0; i < pIpNetTable->dwNumEntries; i++) {
 
        /* if the user has supplied their own internet addesss *
         * only print the arp entry which matches that */
        if (pszInetAddr) {
            inaddr.S_un.S_addr = pIpNetTable->table[i].dwAddr;        
            pszIpAddr = inet_ntoa(inaddr);
 
            /* check if it matches, print it */
            if (strcmp(pszIpAddr, pszInetAddr) == 0) {
                PrintEntries(&pIpNetTable->table[i]);
            }
        } else {
            /* if an address is not supplied, print all entries */
            PrintEntries(&pIpNetTable->table[i]);
        }
 
    }
 
    free(pIpNetTable);
    free(pIpAddrTable);
 
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
    _tprintf(_T("  %-22s"), inet_ntoa(inaddr));  //error checking
 
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
    switch (pIpAddRow->dwType) {
        case MIB_IPNET_TYPE_DYNAMIC : _tprintf(_T("dynamic\n"));
                                      break;
        case MIB_IPNET_TYPE_STATIC : _tprintf(_T("static\n"));
                                      break;
        case MIB_IPNET_TYPE_INVALID : _tprintf(_T("invalid\n"));
                                      break;
        case MIB_IPNET_TYPE_OTHER : _tprintf(_T("other\n"));
                                      break;
    }
    return 0;
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
    PMIB_IPNETROW pAddHost;
    PMIB_IPADDRTABLE pIpAddrTable;
    DWORD dwIpAddr;
    DWORD dwSize = 0;
    INT iRet, i, val;
    TCHAR c;
 
    /* error checking */
 
    /* check IP address */
    if (pszInetAddr != NULL) {
        if ((dwIpAddr = inet_addr(pszInetAddr)) == INADDR_NONE) {
            _tprintf(_T("ARP: bad IP address: %s\n"), pszInetAddr);
            return -1;
        }
    } else {
        Usage();
        return -1;
    }
 
    /* check MAC address */
    if (strlen(pszEthAddr) != 17) {
        _tprintf(_T("ARP: bad argument: %s\n"), pszEthAddr);
        return -1;
    }
    for (i=0; i<17; i++) {
        if (pszEthAddr[i] == SEPERATOR) {
            continue;
        }
        if (!isxdigit(pszEthAddr[i])) {
            _tprintf(_T("ARP: bad argument: %s\n"), pszEthAddr);
            return -1;
        }
    }
 
    /* reserve memory on heap and zero */
    pAddHost = (MIB_IPNETROW *) malloc(sizeof(MIB_IPNETROW));
    ZeroMemory(pAddHost, sizeof(MIB_IPNETROW));
 
 
 
 
    /* set dwIndex field to the index of a local IP address to 
     * indicate the network on which the ARP entry applies */
    if (pszIfAddr) {
        sscanf(pszIfAddr, "%lx", &pAddHost->dwIndex);
    } else {
        /* map the IP to the index */  
        pIpAddrTable = (MIB_IPADDRTABLE *) malloc(dwSize);
        GetIpAddrTable(pIpAddrTable, &dwSize, 0);
 
        pIpAddrTable = (MIB_IPADDRTABLE *) malloc(dwSize);
 
        if ((iRet = GetIpAddrTable(pIpAddrTable, &dwSize, TRUE)) != NO_ERROR) { // NO_ERROR = 0
            _tprintf(_T("GetIpAddrTable failed: %d\n"), iRet);
            _tprintf(_T("error: %d\n"), WSAGetLastError());
        }
        printf("printing pIpAddrTable->table[0].dwIndex = %lx\n", pIpAddrTable->table[0].dwIndex);
        pAddHost->dwIndex = 4;

        free(pIpAddrTable);
    }
 
    /* Set MAC address to 6 bytes (typical) */
    pAddHost->dwPhysAddrLen = 6;
 
 
    /* Encode bPhysAddr into correct byte array */
    for (i=0; i<6; i++) {
        val =0;
        c = toupper(pszEthAddr[i*3]);
        c = c - (isdigit(c) ? '0' : ('A' - 10));
        val += c;
        val = (val << 4);
        c = toupper(pszEthAddr[i*3 + 1]);
        c = c - (isdigit(c) ? '0' : ('A' - 10));
        val += c;
        pAddHost->bPhysAddr[i] = val;
 
    }
 
 
    /* copy converted IP address */
    pAddHost->dwAddr = dwIpAddr;
 
 
    /* set type to static */
    pAddHost->dwType = MIB_IPNET_TYPE_STATIC;
 
 
    /* Add the ARP entry */
    if ((iRet = SetIpNetEntry(pAddHost)) != NO_ERROR) {
        _tprintf(_T("The ARP entry addition failed: %d\n"), iRet);
        return -1;
    }
 
    free(pAddHost);
 
    return 0;
}
 
 
 
/*
 *
 * Takes an internet address and an optional interface address as 
 * arguments and checks their validity.
 * Add the interface number and IP to an MIB_IPNETROW structure 
 * and remove the entrty from the ARP cache.
 *
 */
INT Deletehost(PTCHAR pszInetAddr, PTCHAR pszIfAddr)
{
    PMIB_IPNETROW pDelHost;
    PMIB_IPADDRTABLE pIpAddrTable;
    DWORD dwIpAddr;
    DWORD dwSize = 0;
    INT iret;
 
    /* error checking */
 
    /* check IP address */
    if (pszInetAddr != NULL) {
        if ((dwIpAddr = inet_addr(pszInetAddr)) == INADDR_NONE) {
            _tprintf(_T("ARP: bad IP address: %s\n"), pszInetAddr);
            return -1;
        }
    } else {
        Usage();
        return -1;
    }
 
 
     pIpAddrTable = (MIB_IPADDRTABLE*) malloc(sizeof(MIB_IPADDRTABLE));
     pDelHost = (MIB_IPNETROW *) malloc(sizeof(MIB_IPNETROW));
     ZeroMemory(pIpAddrTable, sizeof(MIB_IPADDRTABLE));
     ZeroMemory(pDelHost, sizeof(MIB_IPNETROW));
    /* set dwIndex field to the index of a local IP address to 
     * indicate the network on which the ARP entry applies */
    if (pszIfAddr) {
        sscanf(pszIfAddr, "%lx", &pDelHost->dwIndex);
    } else {
        /* map the IP to the index */
        if (GetIpAddrTable(pIpAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
            pIpAddrTable = (MIB_IPADDRTABLE *) malloc(dwSize);
        }
        if ((iret = GetIpAddrTable(pIpAddrTable, &dwSize, TRUE)) != NO_ERROR) {
            _tprintf(_T("GetIpAddrTable failed: %d\n"), iret);
            _tprintf(_T("error: %d\n"), WSAGetLastError());
        }
        pDelHost->dwIndex = 4; //pIpAddrTable->table[0].dwIndex;
    }    
 
    /* copy converted IP address */
    pDelHost->dwAddr = dwIpAddr; 
 
    /* Add the ARP entry */
    if ((iret = DeleteIpNetEntry(pDelHost)) != NO_ERROR) {
        _tprintf(_T("The ARP entry deletion failed: %d\n"), iret);
        return -1;
    }
 
    free(pIpAddrTable);
    free(pDelHost);
 
    return 0;
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
    const char N[] = "-N";
 
    if ((argc < 2) || (argc > 5)) 
    {
       Usage();
       return FALSE;
    }
 
 
    if (argv[1][0] == '-') {
        switch (argv[1][1]) {
           /* FIX ME */
           /* need better control for -a, as -N might not be arg 4 */
           case 'a': if (argc == 2)
                         DisplayArpEntries(NULL, NULL);
                     else if (argc == 3)
                         DisplayArpEntries(argv[2], NULL);
                     else if ((argc == 5) && ((strcmp(argv[3], N)) == 0))
                         DisplayArpEntries(argv[2], argv[4]);
                     else
                         Usage();
                     break;
           case 'g': break;
           case 'd': if (argc == 3)
                         Deletehost(argv[2], NULL);
                     else if (argc == 4)
                         Deletehost(argv[2], argv[3]);
                     else
                         Usage();
                     break;
           case 's': if (argc == 4)
                         Addhost(argv[2], argv[3], NULL);
                     else if (argc == 5)
                         Addhost(argv[2], argv[3], argv[4]);
                     else
                         Usage();
                     break;
           default:
              Usage();
              return -1;
        }
    } else {
        Usage();
        return -1;
    }
 
    return 0;
}
