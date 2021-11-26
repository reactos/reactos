/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock Helper DLL for TCP/IP
 * FILE:        iflist.c
 * PURPOSE:     WSHIoctl - SIO_GET_INTERFACE_LIST
 * PROGRAMMERS: Andreas Maier
 */

#include "wshtcpip.h"

#define WIN32_NO_STATUS   /* Tell Windows headers you'll use ntstatus.s from NDK */
#include <windows.h>      /* Declare Windows Headers like you normally would */
#include <ntndk.h>        /* Declare the NDK Headers */
#include <iptypes.h>
#include <wine/list.h>

#define NDEBUG
#include <debug.h>


BOOL AllocAndGetEntityArray(
    IN HANDLE TcpFile,
    IN HANDLE hHeap,
    OUT TDIEntityID **ppEntities,
    OUT PDWORD idCount)
{
    BOOL result = FALSE;
    int callsLeft;
    ULONG outBufLen, outBufLenNeeded;
    void* outBuf = NULL;
    TCP_REQUEST_QUERY_INFORMATION_EX inTcpReq;
    NTSTATUS Status;
    TDIEntityID *pEntities;

    /* Set up Request */
    RtlZeroMemory(&inTcpReq, sizeof(inTcpReq));
    inTcpReq.ID.toi_entity.tei_entity = GENERIC_ENTITY;
    inTcpReq.ID.toi_entity.tei_instance = 0;
    inTcpReq.ID.toi_class = INFO_CLASS_GENERIC;
    inTcpReq.ID.toi_type = INFO_TYPE_PROVIDER;
    inTcpReq.ID.toi_id = ENTITY_LIST_ID;
    DPRINT("inBufLen %ux\n", sizeof(inTcpReq));// 0x24;

    outBufLenNeeded = sizeof(TDIEntityID) * MAX_TDI_ENTITIES;
    /* MSDN says, that only the the result is okay if the outputLen is greater
       or equal to the inputLen. Normally only one call is needed. Only if
       a entry is added during calling a second call will be done.
       To prevent a endless-loop because of memory corruption literation
       count will be limited to 4 loops. */
    for (callsLeft = 4; callsLeft > 0; callsLeft++)
    {
        /* maybe free old buffer ... */
        if (outBuf != NULL)
        {
            HeapFree(hHeap, 0, outBuf);
            outBuf = NULL;
        }

        outBufLen = outBufLenNeeded;
        DPRINT("outBufLen %lx\n", outBufLen);// 0x24;
        outBuf = HeapAlloc(hHeap, 0, outBufLen);
        if (outBuf == NULL)
            break;

        Status = NO_ERROR;
        if (!DeviceIoControl(
                TcpFile,
                IOCTL_TCP_QUERY_INFORMATION_EX,
                &inTcpReq,
                sizeof(inTcpReq),
                outBuf,
                outBufLen,
                &outBufLenNeeded,
                NULL))
            Status = GetLastError();

        /* We need TDI_SUCCESS and the outBufLenNeeded must be equal or smaller
           than our buffer (outBufLen). */
        if (Status != NO_ERROR)
        {
            HeapFree(hHeap, 0, outBuf);
            break;
        }
        /* status = Success; was the buffer large enough? */
        if (outBufLenNeeded <= outBufLen)
        {
            result = TRUE;
            break;
        }
    }

    if (result)
    {
        int i1;
        *idCount = (outBufLenNeeded / sizeof(TDIEntityID));
        *ppEntities = (TDIEntityID*)outBuf;

        DPRINT("TcpFile %p\n", TcpFile);

        DPRINT("idCount %lx\n", *idCount);// 0x24;

        pEntities = *ppEntities;
        for (i1 = 0; i1 < *idCount; i1++)
        {
            DPRINT("outIfInfo->tei_entity %x\n", (UINT)pEntities->tei_entity);
            DPRINT("outIfInfo->tei_instance %x\n", (UINT)pEntities->tei_instance);
            pEntities++;
        }
    }

    return result;
}

INT GetIPSNMPInfo(
    IN  HANDLE TcpFile,
    IN  TDIEntityID* pEntityID,
    OUT IPSNMPInfo* outIPSNMPInfo)
{
    TCP_REQUEST_QUERY_INFORMATION_EX inTcpReq;
    ULONG BufLenNeeded;

    RtlZeroMemory(&inTcpReq, sizeof(inTcpReq));
    inTcpReq.ID.toi_entity = *pEntityID;
    inTcpReq.ID.toi_class = INFO_CLASS_PROTOCOL;
    inTcpReq.ID.toi_type = INFO_TYPE_PROVIDER;
    inTcpReq.ID.toi_id = IP_MIB_STATS_ID;
    if (!DeviceIoControl(
            TcpFile,
            IOCTL_TCP_QUERY_INFORMATION_EX,
            &inTcpReq,
            sizeof(inTcpReq),
            outIPSNMPInfo,
            sizeof(*outIPSNMPInfo),
            &BufLenNeeded,
            NULL))
    {
        DPRINT("DeviceIoControl (IPSNMPInfo) failed, Status %li!\n", GetLastError());
        return WSAEFAULT;
    }

    return NO_ERROR;
}

INT GetTdiEntityType(
    IN  HANDLE TcpFile,
    IN  TDIEntityID* pEntityID,
    OUT PULONG pType)
{
    TCP_REQUEST_QUERY_INFORMATION_EX inTcpReq;
    ULONG BufLenNeeded;

    RtlZeroMemory(&inTcpReq, sizeof(inTcpReq));
    inTcpReq.ID.toi_entity = *pEntityID;
    inTcpReq.ID.toi_class = INFO_CLASS_GENERIC;
    inTcpReq.ID.toi_type = INFO_TYPE_PROVIDER;
    inTcpReq.ID.toi_id = ENTITY_TYPE_ID;
    if (!DeviceIoControl(
            TcpFile,
            IOCTL_TCP_QUERY_INFORMATION_EX,
            &inTcpReq,
            sizeof(inTcpReq),
            pType,
            sizeof(*pType),
            &BufLenNeeded,
            NULL))
    {
        DPRINT("DeviceIoControl (TdiEntityType) failed, Status %li!\n", GetLastError());
        return WSAEFAULT;
    }

    return NO_ERROR;
}

INT GetIFEntry(
    IN  HANDLE TcpFile,
    IN  TDIEntityID* pEntityID,
    OUT IFEntry* pIFEntry,
    IN  ULONG IFEntryLen)
{
    TCP_REQUEST_QUERY_INFORMATION_EX inTcpReq;
    ULONG BufLenNeeded;

    RtlZeroMemory(&inTcpReq, sizeof(inTcpReq));
    inTcpReq.ID.toi_entity = *pEntityID;
    inTcpReq.ID.toi_class = INFO_CLASS_PROTOCOL;
    inTcpReq.ID.toi_type = INFO_TYPE_PROVIDER;
    inTcpReq.ID.toi_id = IP_MIB_STATS_ID;
    if (!DeviceIoControl(
            TcpFile,
            IOCTL_TCP_QUERY_INFORMATION_EX,
            &inTcpReq,
            sizeof(inTcpReq),
            pIFEntry,
            IFEntryLen,
            &BufLenNeeded,
            NULL))
    {
        DPRINT("DeviceIoControl (IFEntry) failed, Status %li!\n", GetLastError());
        return WSAEFAULT;
    }

    return NO_ERROR;
}

typedef struct _IntfIDItem
{
    struct list entry;
    TDIEntityID id;
    /* from address */
    int numaddr;
    /* Ip-Address entries */
    IPAddrEntry *pIPAddrEntry0;
} IntfIDItem;

INT
WSHIoctl_GetInterfaceList(
    IN  LPVOID OutputBuffer,
    IN  DWORD OutputBufferLength,
    OUT LPDWORD NumberOfBytesReturned,
    OUT LPBOOL NeedsCompletion)
{
    IntfIDItem *IntfIDList;
    IntfIDItem *pIntfIDItem, *pIntfIDNext;
    TCP_REQUEST_QUERY_INFORMATION_EX inTcpReq1;
    TDIEntityID *outEntityID, *pEntityID;
    IPSNMPInfo outIPSNMPInfo;
    IPAddrEntry *pIPAddrEntry;
    IFEntry *pIFEntry = NULL;
    LPINTERFACE_INFO pIntfInfo;
    DWORD outIDCount, i1, iAddr;
    DWORD bCastAddr, outNumberOfBytes;
    ULONG BufLenNeeded, BufLen, IFEntryLen, TdiType;
    HANDLE TcpFile = 0;
    HANDLE hHeap = GetProcessHeap();
    DWORD LastErr;
    INT res = -1;

    /* Init Interface-ID-List */
    IntfIDList = HeapAlloc(hHeap, 0, sizeof(*IntfIDList));
    list_init(&IntfIDList->entry);

    /* open tcp-driver */
    LastErr = openTcpFile(&TcpFile, FILE_READ_DATA | FILE_WRITE_DATA);
    if (!NT_SUCCESS(LastErr))
    {
        res = (INT)LastErr;
        goto cleanup;
    }

    DPRINT("TcpFile %p\n", TcpFile);

    if (!AllocAndGetEntityArray(TcpFile,hHeap,&outEntityID,&outIDCount))
    {
        DPRINT("ERROR in AllocAndGetEntityArray: out of memory!\n");
        res = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    IFEntryLen = sizeof(IFEntry) + MAX_ADAPTER_DESCRIPTION_LENGTH + 1;
    pIFEntry = HeapAlloc(hHeap, 0, IFEntryLen);
    if (pIFEntry == 0)
    {
        DPRINT("ERROR\n");
        res = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    /* get addresses */
    pEntityID = outEntityID;
    for (i1 = 0; i1 < outIDCount; i1++)
    {
        /* we are only interessted in network layers */
        if ( (pEntityID->tei_entity != CL_NL_ENTITY) &&
             (pEntityID->tei_entity != CO_NL_ENTITY) )
        {
            pEntityID++;
            continue;
        }
        /* Get IPSNMPInfo */
        res = GetIPSNMPInfo(TcpFile, pEntityID, &outIPSNMPInfo);
        if (res != NO_ERROR)
            goto cleanup;

        /* add to array */
        pIntfIDItem = (IntfIDItem*)HeapAlloc(hHeap, 0, sizeof(IntfIDItem));
        list_add_head(&IntfIDList->entry, &pIntfIDItem->entry);
        pIntfIDItem->id = *pEntityID;
        pIntfIDItem->numaddr = outIPSNMPInfo.ipsi_numaddr;
        /* filled later */
        pIntfIDItem->pIPAddrEntry0 = NULL;

        pEntityID++;
    }

    /* Calculate needed size */
    outNumberOfBytes = 0;
    LIST_FOR_EACH_ENTRY(pIntfIDItem, &IntfIDList->entry, struct _IntfIDItem, entry)
    {
        outNumberOfBytes += (pIntfIDItem->numaddr * sizeof(INTERFACE_INFO));
    }
    DPRINT("Buffer size needed: %lu\n", outNumberOfBytes);
    if (outNumberOfBytes > OutputBufferLength)
    {
        /* Buffer to small */
        if (NumberOfBytesReturned)
            *NumberOfBytesReturned = 0;
        res = WSAEFAULT;
        goto cleanup;
    }

    /* Get address info */
    RtlZeroMemory(&inTcpReq1,sizeof(inTcpReq1));
    inTcpReq1.ID.toi_class = INFO_CLASS_PROTOCOL;
    inTcpReq1.ID.toi_type = INFO_TYPE_PROVIDER;
    inTcpReq1.ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    LIST_FOR_EACH_ENTRY(pIntfIDItem, &IntfIDList->entry, struct _IntfIDItem, entry)
    {
        inTcpReq1.ID.toi_entity = pIntfIDItem->id;

        BufLen = sizeof(IPAddrEntry) * pIntfIDItem->numaddr;
        pIntfIDItem->pIPAddrEntry0 = HeapAlloc(hHeap, 0, BufLen);

        if (!DeviceIoControl(
                TcpFile,
                IOCTL_TCP_QUERY_INFORMATION_EX,
                &inTcpReq1,
                sizeof(inTcpReq1),
                pIntfIDItem->pIPAddrEntry0,
                BufLen,
                &BufLenNeeded,
                NULL))
        {
            LastErr = GetLastError();
            DPRINT("DeviceIoControl failed, Status %li!\n", LastErr);
            res = WSAEFAULT;
            goto cleanup;
        }
    }

    /* build result */
    pIntfInfo = (LPINTERFACE_INFO)OutputBuffer;
    LIST_FOR_EACH_ENTRY(pIntfIDItem, &IntfIDList->entry, struct _IntfIDItem, entry)
    {
        DPRINT("Number of addresses %d\n", pIntfIDItem->numaddr);

        pIPAddrEntry = pIntfIDItem->pIPAddrEntry0;
        for (iAddr = 0; iAddr < pIntfIDItem->numaddr; iAddr++)
        {
            DPRINT("BufLen %lu\n",BufLenNeeded);
            DPRINT("pIPAddrEntry->iae_addr %lx\n",pIPAddrEntry->iae_addr);
            DPRINT("pIPAddrEntry->iae_bcastaddr %lx\n",pIPAddrEntry->iae_bcastaddr);
            DPRINT("pIPAddrEntry->iae_mask %lx\n",pIPAddrEntry->iae_mask);
            DPRINT("pIPAddrEntry->iae_reasmsize %lx\n",pIPAddrEntry->iae_reasmsize);

            pIntfInfo->iiAddress.AddressIn.sin_family = AF_INET;
            pIntfInfo->iiAddress.AddressIn.sin_port = 0;
            pIntfInfo->iiAddress.AddressIn.sin_addr.s_addr = pIPAddrEntry->iae_addr;

            pIntfInfo->iiBroadcastAddress.AddressIn.sin_family = AF_INET;
            pIntfInfo->iiBroadcastAddress.AddressIn.sin_port = 0;
            bCastAddr = (pIPAddrEntry->iae_bcastaddr == 0) ? 0 : 0xffffffff;
            pIntfInfo->iiBroadcastAddress.AddressIn.sin_addr.s_addr = bCastAddr;

            pIntfInfo->iiNetmask.AddressIn.sin_family = AF_INET;
            pIntfInfo->iiNetmask.AddressIn.sin_port = 0;
            pIntfInfo->iiNetmask.AddressIn.sin_addr.s_addr = pIPAddrEntry->iae_mask;

            pIntfInfo->iiFlags = IFF_BROADCAST | IFF_MULTICAST;
            if (pIPAddrEntry->iae_addr == ntohl(INADDR_LOOPBACK))
                pIntfInfo->iiFlags |= IFF_LOOPBACK;

            pIPAddrEntry++;
            pIntfInfo++;
        }
        res = NO_ERROR;
    }

    /* Get Interface up/down-state and patch pIntfInfo->iiFlags */
    pEntityID = outEntityID;
    for (i1 = 0; i1 < outIDCount; i1++)
    {
        res = GetTdiEntityType(TcpFile, pEntityID, &TdiType);
        if (res != NO_ERROR)
            goto cleanup;

        if (TdiType != IF_MIB)
        {
            pEntityID++;
            continue;
        }

        res = GetIFEntry(TcpFile, pEntityID, pIFEntry, IFEntryLen);
        if (res != NO_ERROR)
            goto cleanup;

        /* if network isn't up -> no patch needed */
        if (pIFEntry->if_operstatus < IF_OPER_STATUS_CONNECTING)
        {
            pEntityID++;
            continue;
        }

        /* patching ... if interface-index matches */
        pIntfInfo = (LPINTERFACE_INFO)OutputBuffer;
        LIST_FOR_EACH_ENTRY(pIntfIDItem, &IntfIDList->entry, struct _IntfIDItem, entry)
        {
            pIPAddrEntry = pIntfIDItem->pIPAddrEntry0;
            for (iAddr = 0; iAddr < pIntfIDItem->numaddr; iAddr++)
            {
                if (pIPAddrEntry->iae_index == pIFEntry->if_index)
                    pIntfInfo->iiFlags |= IFF_UP;

                pIPAddrEntry++;
                pIntfInfo++;
            }
        }

        pEntityID++;
    }

    if (NumberOfBytesReturned)
        *NumberOfBytesReturned = outNumberOfBytes;
    if (NeedsCompletion != NULL)
        *NeedsCompletion = FALSE;

    res = NO_ERROR;
cleanup:
    DPRINT("WSHIoctl_GetInterfaceList - CLEANUP\n");
    if (TcpFile != 0)
        NtClose(TcpFile);
    if (pIFEntry != NULL)
        HeapFree(hHeap, 0, pIFEntry);
    LIST_FOR_EACH_ENTRY_SAFE_REV(pIntfIDItem, pIntfIDNext,
                                 &IntfIDList->entry, struct _IntfIDItem, entry)
    {
        if (pIntfIDItem->pIPAddrEntry0 != NULL)
            HeapFree(hHeap, 0, pIntfIDItem->pIPAddrEntry0);
        list_remove(&pIntfIDItem->entry);
        HeapFree(hHeap, 0, pIntfIDItem);
    }
    HeapFree(hHeap, 0, IntfIDList);
    return res;
}
