/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for WSHIoctl:
 *                  - SIO_GET_INTERFACE_LIST
 * PROGRAMMERS:     Andreas Maier
 */

#include "ws2_32.h"

#include <iphlpapi.h>

void traceaddr(char* txt, sockaddr_gen a)
{
    trace("  %s.AddressIn.sin_family %x\n", txt, a.AddressIn.sin_family);
    trace("  %s.AddressIn.sin_port %x\n", txt, a.AddressIn.sin_port);
    trace("  %s.AddressIn.sin_addr.s_addr %lx\n", txt, a.AddressIn.sin_addr.s_addr);
}

BOOL Test_WSAIoctl_InitTest(
    OUT PMIB_IPADDRTABLE* ppTable)
{
    PMIB_IPADDRROW pRow;
    DWORD ret, i1;
    ULONG TableSize;
    PMIB_IPADDRTABLE pTable;

    TableSize = 0;
    *ppTable = NULL;
    ret = GetIpAddrTable(NULL, &TableSize, FALSE);
    if (ret != ERROR_INSUFFICIENT_BUFFER)
    {
        skip("GetIpAddrTable failed with %ld. Abort Testing.\n", ret);
        return FALSE;
    }

    /* get sorted ip-address table. Sort order is the ip-address. */
    pTable = (PMIB_IPADDRTABLE)malloc(TableSize);
    *ppTable = pTable;
    ret = GetIpAddrTable(pTable, &TableSize, TRUE);
    if (ret != NO_ERROR)
    {
        skip("GetIpAddrTable failed with %ld. Abort Testing.\n", ret);
        return FALSE;
    }

    if (winetest_debug >= 2)
    {
        trace("Result of GetIpAddrTable:\n");
        trace("Count: %ld\n", pTable->dwNumEntries);
        pRow = pTable->table;
        for (i1 = 0; i1 < pTable->dwNumEntries; i1++)
        {
            trace("** Entry %ld **\n", i1);
            trace("  dwAddr %lx\n", pRow->dwAddr);
            trace("  dwIndex %lx\n", pRow->dwIndex);
            trace("  dwMask %lx\n", pRow->dwMask);
            trace("  dwBCastAddr %lx\n", pRow->dwBCastAddr);
            trace("  dwReasmSize %lx\n", pRow->dwReasmSize);
            trace("  wType %x\n", pRow->wType);
            pRow++;
        }
        trace("END\n");
    }

    return TRUE;
}

void Test_WSAIoctl_GetInterfaceList()
{
    WSADATA wdata;
    INT iResult;
    SOCKET sck;
    ULONG buflen, BytesReturned, BCastAddr;
    ULONG infoCount, i1, j1, iiFlagsExpected;
    BYTE* buf = NULL;
    LPINTERFACE_INFO ifInfo;
    PMIB_IPADDRTABLE pTable = NULL;
    PMIB_IPADDRROW pRow;

    /* Get PMIB_IPADDRTABE - these results we should get from wshtcpip.dll too. */
    /* pTable is allocated by Test_WSAIoctl_InitTest! */
    if (!Test_WSAIoctl_InitTest(&pTable))
        goto cleanup;

    /* Start up Winsock */
    iResult = WSAStartup(MAKEWORD(2, 2), &wdata);
    ok(iResult == 0, "WSAStartup failed. iResult = %d\n", iResult);

    /* Create the socket */
    sck = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, 0, 0, 0);
    ok(sck != INVALID_SOCKET, "WSASocket failed. sck = %d\n", (INT)sck);
    if (sck == INVALID_SOCKET)
    {
        skip("Failed to create socket!\n");
        goto cleanup;
    }

    /* Do the Ioctl (buffer to small) */
    buflen = sizeof(INTERFACE_INFO)-1;
    buf = (BYTE*)HeapAlloc(GetProcessHeap(), 0, buflen);
    if (buf == NULL)
    {
        skip("Failed to allocate memory!\n");
        goto cleanup;
    }
    BytesReturned = 0;
    iResult = WSAIoctl(sck, SIO_GET_INTERFACE_LIST, 0, 0,
                       buf, buflen, &BytesReturned, 0, 0);
    ok(iResult == -1, "WSAIoctl/SIO_GET_INTERFACE_LIST did not fail! iResult = %d.\n", iResult);
    ok_hex(WSAGetLastError(), WSAEFAULT);
    ok(BytesReturned == 0, "WSAIoctl/SIO_GET_INTERFACE_LIST: BytesReturned should be 0, not %ld.\n", BytesReturned);
    HeapFree(GetProcessHeap(), 0, buf);
    buf = NULL;

    /* Do the Ioctl
       In most cases no loop is done.
       Only if WSAIoctl fails with WSAEFAULT (buffer to small) we need to retry with a
       larger buffer */
    i1 = 0;
    while (TRUE)
    {
        if (buf != NULL)
        {
            HeapFree(GetProcessHeap(), 0, buf);
            buf = NULL;
        }

        buflen = sizeof(INTERFACE_INFO) * (i1+1) * 32;
        buf = (BYTE*)HeapAlloc(GetProcessHeap(), 0, buflen);
        if (buf == NULL)
        {
            skip("Failed to allocate memory!\n");
            goto cleanup;
        }
        BytesReturned = 0;
        iResult = WSAIoctl(sck, SIO_GET_INTERFACE_LIST, 0, 0,
                           buf, buflen, &BytesReturned, 0, 0);
        /* we have what we want ... leave loop */
        if (iResult == NO_ERROR)
            break;
        /* only retry if buffer was to small */
        /* to avoid infinite loop we maximum retry count is 4 */
        if ((i1 >= 4) || (WSAGetLastError() != WSAEFAULT))
        {
            ok_hex(iResult, NO_ERROR);
            skip("WSAIoctl / SIO_GET_INTERFACE_LIST\n");
            goto cleanup;
        }
        /* buffer to small -> retry */
        i1++;
    }

    ok_dec(BytesReturned % sizeof(INTERFACE_INFO), 0);

    /* Calculate how many INTERFACE_INFO we got */
    infoCount = BytesReturned / sizeof(INTERFACE_INFO);
    ok(infoCount == pTable->dwNumEntries,
       "Wrong count of entries! Got %ld, expected %ld.\n", pTable->dwNumEntries, infoCount);

    if (winetest_debug >= 2)
    {
        trace("WSAIoctl\n");
        trace("  BytesReturned %ld - InfoCount %ld\n ", BytesReturned, infoCount);
        ifInfo = (LPINTERFACE_INFO)buf;
        for (i1 = 0; i1 < infoCount; i1++)
        {
            trace("entry-index %ld\n", i1);
            trace("  iiFlags %ld\n", ifInfo->iiFlags);
            traceaddr("ifInfo^.iiAddress", ifInfo->iiAddress);
            traceaddr("ifInfo^.iiBroadcastAddress", ifInfo->iiBroadcastAddress);
            traceaddr("ifInfo^.iiNetmask", ifInfo->iiNetmask);
            ifInfo++;
        }
    }

    /* compare entries */
    ifInfo = (LPINTERFACE_INFO)buf;
    for (i1 = 0; i1 < infoCount; i1++)
    {
        if (winetest_debug >= 2)
            trace("Entry %ld\n", i1);
        for (j1 = 0; j1 < pTable->dwNumEntries; j1++)
        {
            if (ifInfo[i1].iiAddress.AddressIn.sin_addr.s_addr == pTable->table[j1].dwAddr)
            {
                pRow = &pTable->table[j1];
                break;
            }
        }
        if (j1 == pTable->dwNumEntries)
        {
            skip("Skipping interface\n");
            continue;
        }

        /* iiFlags
         * Don't know if this value is fix for SIO_GET_INTERFACE_LIST! */
        iiFlagsExpected = IFF_BROADCAST | IFF_MULTICAST;
        if ((pRow->wType & MIB_IPADDR_DISCONNECTED) == 0)
            iiFlagsExpected |= IFF_UP;
        if (pRow->dwAddr == ntohl(INADDR_LOOPBACK))
        {
            iiFlagsExpected |= IFF_LOOPBACK;
            /* on Windows 7 loopback has broadcast flag cleared */
            //iiFlagsExpected &= ~IFF_BROADCAST;
        }

        ok_hex(ifInfo[i1].iiFlags, iiFlagsExpected);
        ok_hex(ifInfo[i1].iiAddress.AddressIn.sin_addr.s_addr, pRow->dwAddr);
        // dwBCastAddr is not the "real" Broadcast-Address.
        BCastAddr = (pRow->dwBCastAddr == 1 && (iiFlagsExpected & IFF_BROADCAST) != 0) ? 0xFFFFFFFF : 0x0;
        ok_hex(ifInfo[i1].iiBroadcastAddress.AddressIn.sin_addr.s_addr, BCastAddr);
        ok_hex(ifInfo[i1].iiNetmask.AddressIn.sin_addr.s_addr, pRow->dwMask);
    }

cleanup:
    if (sck != 0)
        closesocket(sck);
    if (pTable != NULL)
       free(pTable);
    if (buf != NULL)
       HeapFree(GetProcessHeap(), 0, buf);
    WSACleanup();
}

START_TEST(WSAIoctl)
{
    Test_WSAIoctl_GetInterfaceList();
}
