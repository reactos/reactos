/* $Id: niclist.c,v 1.1 2002/07/09 19:34:41 robd Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS packet driver interface list utility
 * FILE:        apps/net/niclist/niclist.c
 * PURPOSE:     Network information utility
 * PROGRAMMERS: Robert Dickenson (robert_dickenson@users.sourceforge.net)
 * REVISIONS:
 *   RDD  10/07/2002 Created from bochs sources
 */
/*
 For this program and for win32 ethernet, the winpcap library is required.
 Download it from http://netgroup-serv.polito.it/winpcap.
 */
#ifdef MSC_VER
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ADAPTERS 10
#define NIC_BUFFER_SIZE 2048


// structure to hold the adapter name and description
typedef struct {
    LPWSTR wstrName;
    LPSTR strDesc;
} NIC_INFO_NT;

// array of structures to hold information for our adapters
NIC_INFO_NT nic_info[MAX_ADAPTERS];

// pointer to exported function in winpcap library
BOOLEAN (*PacketGetAdapterNames)(PTSTR, PULONG) = NULL;


int main(int argc, char **argv)
{
    char AdapterInfo[NIC_BUFFER_SIZE] = { '\0','\0' };
    unsigned long AdapterLength = NIC_BUFFER_SIZE;
    LPWSTR wstrName;
    LPSTR strDesc;
    int nAdapterCount;
    int i;


    // Attemp to load the WinPCap dynamic link library
    HINSTANCE hPacket = LoadLibrary("PACKET.DLL");
    if (hPacket) {
        PacketGetAdapterNames = (BOOLEAN (*)(PTSTR, PULONG))GetProcAddress(hPacket, "PacketGetAdapterNames");
    } else {
        printf("Could not load WinPCap driver! for more information goto:\n");
        printf ("http://netgroup-serv.polito.it/winpcap\n");
        return 1;
    }
    PacketGetAdapterNames(AdapterInfo, &AdapterLength);
    wstrName = (LPWSTR)AdapterInfo;

    // Enumerate all the adapters names found...
    nAdapterCount = 0;
    while ((*wstrName)) {
        nic_info[nAdapterCount].wstrName = wstrName;
        wstrName += lstrlenW(wstrName) + 1;
        nAdapterCount++;
    }
    strDesc = (LPSTR)++wstrName;

    // And obtain the adapter description strings....
    for (i = 0; i < nAdapterCount; i++) {
        nic_info[i].strDesc = strDesc;
        strDesc += lstrlen(strDesc) + 1;

        // display adapter info
        printf("%d: %s\n", i + 1, nic_info[i].strDesc);
        wprintf(L"     Device: %s\n", nic_info[i].wstrName);
    }
    return 0;
}
