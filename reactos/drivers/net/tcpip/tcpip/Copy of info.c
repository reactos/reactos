/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/info.c
 * PURPOSE:     TDI query and set information routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/07-2000 Created
 */
#include <tcpip.h>
#include <routines.h>
#include <info.h>


TDI_STATUS IPTdiQueryInformationEx(
    PTDI_REQUEST Request,
    TDIObjectID *ID,
    PNDIS_BUFFER Buffer,
    PUINT BufferSize,
    PVOID Context)
/*
 * FUNCTION: Returns extended information about network layer
 * ARGUMENTS:
 *     Request    = Pointer to TDI request structure for the request
 *     ID         = TDI object ID
 *     Buffer     = Pointer to buffer with data to use. 
 *     BufferSize = Pointer to buffer with size of Buffer. On return
 *                  this is filled with number of bytes returned
 *     Context    = Pointer to context buffer
 * RETURNS:
 *     Status of operation
 */
{
    IPADDR_ENTRY IpAddress;
    IPSNMP_INFO SnmpInfo;
    PADDRESS_ENTRY ADE;
    PIP_INTERFACE IF;
    ULONG Temp;
    UINT Count;
    ULONG Entity;
    KIRQL OldIrql;
    UINT BufSize = *BufferSize;

    /* Make return parameters consistent every time */
    *BufferSize = 0;

    Entity = ID->toi_entity.tei_entity;
    if (Entity != CL_NL_ENTITY) {
        /* We can't handle this entity */
        return TDI_INVALID_PARAMETER;
    }

    if (ID->toi_entity.tei_instance != TL_INSTANCE)
        /* We only support a single instance */
        return TDI_INVALID_REQUEST;
    if (ID->toi_class == INFO_CLASS_GENERIC) {
        if (ID->toi_type == INFO_TYPE_PROVIDER && 
            ID->toi_id == ENTITY_TYPE_ID) {

            if (BufSize < sizeof(ULONG))
                return TDI_BUFFER_TOO_SMALL;
            Temp = CL_NL_IP;

            Count = CopyBufferToBufferChain(Buffer, 0, (PUCHAR)&Temp, sizeof(ULONG));

            return TDI_SUCCESS;
        }
        return TDI_INVALID_PARAMETER;
    }

    if (ID->toi_class == INFO_CLASS_PROTOCOL) {
        if (ID->toi_type != INFO_TYPE_PROVIDER)
            return TDI_INVALID_PARAMETER;

        switch (ID->toi_id) {
        case IP_MIB_ADDRTABLE_ENTRY_ID:
            Temp = 0;

            KeAcquireSpinLock(&InterfaceLock, &OldIrql);
/*
    /* Search the interface list */
    CurrentIFEntry = InterfaceListHead.Flink;
    while (CurrentIFEntry != &InterfaceListHead) {
	    CurrentIF = CONTAINING_RECORD(CurrentIFEntry, IP_INTERFACE, ListEntry);
        if (CurrentIF != Loopback) {
            /* Search the address entry list and return the first appropriate ADE found */
            CurrentADEEntry = CurrentIF->ADEListHead.Flink;
            while (CurrentADEEntry != &CurrentIF->ADEListHead) {
	            CurrentADE = CONTAINING_RECORD(CurrentADEEntry, ADDRESS_ENTRY, ListEntry);
                if (CurrentADE->Type == AddressType)
                    ReferenceAddress(CurrentADE->Address);
                    KeReleaseSpinLock(&InterfaceListLock, OldIrql);
                    return CurrentADE;
                }
                CurrentADEEntry = CurrentADEEntry->Flink;
        } else
            LoopbackIsRegistered = TRUE;
        CurrentIFEntry = CurrentIFEntry->Flink;
    }


            */
            for (IF = InterfaceList; IF != NULL; IF = IF->Next) {
                if (Temp + sizeof(IPADDR_ENTRY) > BufSize) {
                    KeReleaseSpinLock(&InterfaceLock, OldIrql);
                    return TDI_BUFFER_TOO_SMALL;
                }

                IpAddress.Addr      = 0;
                IpAddress.BcastAddr = 0;
                IpAddress.Mask      = 0;
                /* Locate the diffrent addresses and put them the right place */
                for (ADE = IF->ADE; ADE != NULL; ADE = ADE->Next) {
                    switch (ADE->Type) {
                    case ADE_UNICAST  : IpAddress.Addr      = ADE->Address->Address.IPv4Address;
                    case ADE_MULTICAST: IpAddress.BcastAddr = ADE->Address->Address.IPv4Address;
                    case ADE_ADDRMASK : IpAddress.Mask      = ADE->Address->Address.IPv4Address;
                    }
                }
                /* Pack the address information into IPADDR_ENTRY structure */
                IpAddress.Index     = 0;
                IpAddress.ReasmSize = 0;
                IpAddress.Context   = 0;
                IpAddress.Pad       = 0;

                Count = CopyBufferToBufferChain(Buffer, Temp, (PUCHAR)&IpAddress, sizeof(IPADDR_ENTRY));

                Temp += sizeof(IPADDR_ENTRY);
            }
            KeReleaseSpinLock(&InterfaceLock, OldIrql);
            return TDI_SUCCESS;

        case IP_MIB_STATS_ID:
            if (BufSize < sizeof(IPSNMP_INFO))
                return TDI_BUFFER_TOO_SMALL;

            RtlZeroMemory(&SnmpInfo, sizeof(IPSNMP_INFO));

            /* Count number of addresses */
            Count = 0;
            KeAcquireSpinLock(&InterfaceLock, &OldIrql);
            for (IF = InterfaceList; IF != NULL; IF = IF->Next)
                Count++;
            KeReleaseSpinLock(&InterfaceLock, OldIrql);

            SnmpInfo.NumAddr = Count;

            Count = CopyBufferToBufferChain(Buffer, 0, (PUCHAR)&SnmpInfo, sizeof(IPSNMP_INFO));

            return TDI_SUCCESS;

        default:
            /* We can't handle this ID */
            return TDI_INVALID_PARAMETER;
        }
    }

    return TDI_INVALID_PARAMETER;
}


TDI_STATUS InfoTdiQueryInformationEx(
    PTDI_REQUEST Request,
    TDIObjectID *ID,
    PNDIS_BUFFER Buffer,
    PUINT BufferSize,
    PVOID Context)
/*
 * FUNCTION: Returns extended information
 * ARGUMENTS:
 *     Request    = Pointer to TDI request structure for the request
 *     ID         = TDI object ID
 *     Buffer     = Pointer to buffer with data to use
 *     BufferSize = Pointer to buffer with size of Buffer. On return
 *                  this is filled with number of bytes returned
 *     Context    = Pointer to context buffer
 * RETURNS:
 *     Status of operation
 */
{
    PADDRESS_FILE AddrFile;
    PADDRESS_ENTRY ADE;
    ADDRESS_INFO Info;
    KIRQL OldIrql;
    UINT Entity;
    UINT Count;
    UINT Size;
    ULONG Temp;
    UINT Offset = 0;
    UINT BufSize = *BufferSize;

    /* Check wether it is a query for a list of entities */
    Entity = ID->toi_entity.tei_entity;
    if (Entity == GENERIC_ENTITY) {
        if (ID->toi_class  != INFO_CLASS_GENERIC ||
            ID->toi_type != INFO_TYPE_PROVIDER ||
            ID->toi_id != ENTITY_LIST_ID)
            return TDI_INVALID_PARAMETER;

        *BufferSize = 0;

        Size = EntityCount * sizeof(TDIEntityID);
        if (BufSize < Size)
            /* The buffer is too small to contain requested data */
            return TDI_BUFFER_TOO_SMALL;

        /* Return entity list */
        Count = CopyBufferToBufferChain(Buffer, 0, (PUCHAR)EntityList, Size);

        *BufferSize = Size;

        return TDI_SUCCESS;
    }

    if ((Entity != CL_TL_ENTITY) && (Entity != CO_TL_ENTITY)) {
        /* We can't handle this entity, pass it on */
        return IPTdiQueryInformationEx(
            Request, ID, Buffer, BufferSize, Context);
    }

    /* Make return parameters consistent every time */
    *BufferSize = 0;

    if (ID->toi_entity.tei_instance != TL_INSTANCE)
        /* We only support a single instance */
        return TDI_INVALID_REQUEST;

    if (ID->toi_class == INFO_CLASS_GENERIC) {

        if (ID->toi_type != INFO_TYPE_PROVIDER ||
            ID->toi_id != ENTITY_TYPE_ID)
            return TDI_INVALID_PARAMETER;

        if (BufSize < sizeof(ULONG))
            return TDI_BUFFER_TOO_SMALL;

        if (Entity == CL_TL_ENTITY)
            Temp = CL_TL_UDP;
        else if (Entity == CO_TL_ENTITY)
            Temp = CO_TL_TCP;
        else
            return TDI_INVALID_PARAMETER;

        Count = CopyBufferToBufferChain(Buffer, 0, (PUCHAR)&Temp, sizeof(ULONG));

        return TDI_SUCCESS;
    }

    if (ID->toi_class == INFO_CLASS_PROTOCOL) {

        if (ID->toi_type != INFO_TYPE_PROVIDER)
            return TDI_INVALID_PARAMETER;

        switch (ID->toi_id) {
        case UDP_MIB_STAT_ID:
            if (Entity != CL_TL_ENTITY)
                return TDI_INVALID_PARAMETER;

            if (BufSize < sizeof(UDPStats))
                return TDI_BUFFER_TOO_SMALL;

            Count = CopyBufferToBufferChain(Buffer, 0, (PUCHAR)&UDPStats, sizeof(UDP_STATISTICS));

            return TDI_SUCCESS;

        case UDP_MIB_TABLE_ID:
            if (Entity != CL_TL_ENTITY)
                return TDI_INVALID_PARAMETER;

            Offset = 0;

            KeAcquireSpinLock(&AddressFileLock, &OldIrql);

            for (AddrFile = AddressFileList;
                AddrFile->Next != NULL;
                AddrFile = AddrFile->Next) {

                if (Offset + sizeof(ADDRESS_INFO) > BufSize) {
                    KeReleaseSpinLock(&AddressFileLock, OldIrql);
                    *BufferSize = Offset;
                    return TDI_BUFFER_OVERFLOW;
                }

                for (ADE = AddrFile->ADE; ADE != NULL; ADE = ADE->Next) {
                    /* We only care about IPv4 unicast address */
                    if ((ADE->Type == ADE_UNICAST) &&
                        (ADE->Address->Type == IP_ADDRESS_V4))
                        Info.LocalAddress = ADE->Address->Address.IPv4Address;
                }

                Info.LocalPort = AddrFile->Port;

                Count = CopyBufferToBufferChain(Buffer, Offset, (PUCHAR)&Info, sizeof(ADDRESS_INFO));

                Offset += Count;
            }

            KeReleaseSpinLock(&AddressFileLock, OldIrql);

            *BufferSize = Offset;

            return STATUS_SUCCESS;

        default:
            /* We can't handle this ID */
            return TDI_INVALID_PARAMETER;
        }
    }

    return TDI_INVALID_PARAMETER;
}


TDI_STATUS InfoTdiSetInformationEx(
    PTDI_REQUEST Request,
    TDIObjectID *ID,
    PVOID Buffer,
    UINT BufferSize)
/*
 * FUNCTION: Sets extended information
 * ARGUMENTS:
 *     Request    = Pointer to TDI request structure for the request
 *     ID         = Pointer to TDI object ID
 *     Buffer     = Pointer to buffer with data to use
 *     BufferSize = Size of Buffer
 * RETURNS:
 *     Status of operation
 */
{
    /* FIXME: Set extended information */

    return TDI_INVALID_REQUEST;
}

/* EOF */
