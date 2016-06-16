/*
 * PROJECT:         ReactOS tcpip.sys
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tcpip/address.c
 * PURPOSE:         tcpip.sys: TCP_QUERY_INFORMATION_EX and TCP_SET_INFORMATION_EX ioctls implementation
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

typedef NTSTATUS (*QUERY_INFO_HANDLER)(
    _In_ TDIEntityID ID,
    _In_ PVOID Context,
    _Out_opt_ PVOID OutBuffer,
    _Inout_ ULONG* BufferSize);

typedef NTSTATUS (*SET_INFO_HANDLER)(
    _In_ TDIEntityID ID,
    _In_ PVOID InBuffer,
    _In_ ULONG BufferSize);

static
struct
{
    ULONG Entity, Class, Type, Id;
    QUERY_INFO_HANDLER QueryHandler;
    SET_INFO_HANDLER SetHandler;

} InfoHandlers[] =
{
    { GENERIC_ENTITY,   INFO_CLASS_GENERIC,     INFO_TYPE_PROVIDER,         ENTITY_LIST_ID,             QueryEntityList,            NULL                     },
    { IF_ENTITY,        INFO_CLASS_PROTOCOL,    INFO_TYPE_PROVIDER,         IP_MIB_STATS_ID,            QueryInterfaceEntry,        NULL                     },
    { CL_NL_ENTITY,     INFO_CLASS_PROTOCOL,    INFO_TYPE_PROVIDER,         IP_MIB_ADDRTABLE_ENTRY_ID,  QueryInterfaceAddrTable,    NULL                     },
    { ER_ENTITY,        INFO_CLASS_PROTOCOL,    INFO_TYPE_ADDRESS_OBJECT,   AO_OPTION_TTL,              NULL,                       AddressSetTtl },
    { ER_ENTITY,        INFO_CLASS_PROTOCOL,    INFO_TYPE_ADDRESS_OBJECT,   AO_OPTION_IP_DONTFRAGMENT,  NULL,                       AddressSetIpDontFragment },
    { (ULONG)-1, (ULONG)-1, (ULONG)-1, (ULONG)-1, NULL }
};


NTSTATUS
TcpIpQueryInformation(
    _Inout_ PIRP Irp
)
{
    PIO_STACK_LOCATION IrpSp;
    TCP_REQUEST_QUERY_INFORMATION_EX* Query;
    QUERY_INFO_HANDLER Handler = NULL;
    PMDL QueryMdl = NULL, OutputMdl = NULL;
    BOOL QueryLocked = FALSE, OutputLocked = FALSE;
    ULONG OutputBufferLength;
    PVOID OutputBuffer = NULL;
    NTSTATUS Status;
    ULONG i;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Check input buffer size */
    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(TCP_REQUEST_QUERY_INFORMATION_EX))
        return STATUS_INVALID_PARAMETER;

    /* Get the input buffer */
    Query = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    QueryMdl = IoAllocateMdl(Query, sizeof(*Query), FALSE, TRUE, NULL);
    if (!QueryMdl)
        return STATUS_INSUFFICIENT_RESOURCES;

    _SEH2_TRY
    {
        MmProbeAndLockPages(QueryMdl, Irp->RequestorMode, IoReadAccess);
        QueryLocked = TRUE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        goto Cleanup;
    } _SEH2_END

    /* Get the outputbuffer, if any */
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    if (OutputBufferLength)
    {
        OutputBuffer = Irp->UserBuffer;
        OutputMdl = IoAllocateMdl(OutputBuffer, OutputBufferLength, FALSE, TRUE, NULL);
        if (!OutputMdl)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        _SEH2_TRY
        {
            MmProbeAndLockPages(OutputMdl, Irp->RequestorMode, IoWriteAccess);
            OutputLocked = TRUE;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
            goto Cleanup;
        }
        _SEH2_END
    }

    /* Find the handler for this particular query */
    for (i = 0; InfoHandlers[i].Entity != (ULONG)-1; i++)
    {
        if ((InfoHandlers[i].Entity == Query->ID.toi_entity.tei_entity) &&
                (InfoHandlers[i].Class == Query->ID.toi_class) &&
                (InfoHandlers[i].Type == Query->ID.toi_type) &&
                (InfoHandlers[i].Id == Query->ID.toi_id))
        {
            Handler = InfoHandlers[i].QueryHandler;
            break;
        }
    }

    if (!Handler)
    {
        DPRINT1("TCPIP - Unknown query: entity 0x%x, class 0x%x, type 0x%x, Id 0x%x.\n",
            Query->ID.toi_entity.tei_entity, Query->ID.toi_class, Query->ID.toi_type, Query->ID.toi_id);
        Status = STATUS_NOT_IMPLEMENTED;
        goto Cleanup;
    }

    Status = Handler(Query->ID.toi_entity, Query->Context, OutputBuffer, &OutputBufferLength);

    Irp->IoStatus.Information = OutputBufferLength;

Cleanup:
    if (QueryMdl)
    {
        if (QueryLocked)
            MmUnlockPages(QueryMdl);
        IoFreeMdl(QueryMdl);
    }
    if (OutputMdl)
    {
        if (OutputLocked)
            MmUnlockPages(OutputMdl);
        IoFreeMdl(OutputMdl);
    }
    return Status;
}

NTSTATUS
TcpIpQueryKernelInformation(
    _Inout_ PIRP Irp
)
{
    PIO_STACK_LOCATION IrpSp;
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION Query;
    NTSTATUS Status;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    Query = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&IrpSp->Parameters;

    /* See what we are queried */
    switch (Query->QueryType)
    {
        case TDI_QUERY_MAX_DATAGRAM_INFO:
        {
            PTDI_MAX_DATAGRAM_INFO MaxDatagramInfo;

            if (MmGetMdlByteCount(Irp->MdlAddress) < sizeof(*MaxDatagramInfo))
            {
                DPRINT1("MDL buffer too small.\n");
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            MaxDatagramInfo = MmGetSystemAddressForMdl(Irp->MdlAddress);

            MaxDatagramInfo->MaxDatagramSize = 0xFFFF;

            Status = STATUS_SUCCESS;
            break;
        }
        default:
            DPRINT1("Unknown query: 0x%08x.\n", Query->QueryType);
            return STATUS_NOT_IMPLEMENTED;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return Status;
}

NTSTATUS
TcpIpSetInformation(
    _Inout_ PIRP Irp
)
{
    TCP_REQUEST_SET_INFORMATION_EX* Query;
    SET_INFO_HANDLER Handler = NULL;
    NTSTATUS Status;
    ULONG i;

    /* Get the input buffer */
    Query = Irp->AssociatedIrp.SystemBuffer;

    /* Find the handler for this particular query */
    for (i = 0; InfoHandlers[i].Entity != (ULONG)-1; i++)
    {
        if ((InfoHandlers[i].Entity == Query->ID.toi_entity.tei_entity) &&
                (InfoHandlers[i].Class == Query->ID.toi_class) &&
                (InfoHandlers[i].Type == Query->ID.toi_type) &&
                (InfoHandlers[i].Id == Query->ID.toi_id))
        {
            Handler = InfoHandlers[i].SetHandler;
            break;
        }
    }

    if (!Handler)
    {
        DPRINT1("TCPIP - Unknown query: entity 0x%x, class 0x%x, type 0x%x, Id 0x%x.\n",
            Query->ID.toi_entity.tei_entity, Query->ID.toi_class, Query->ID.toi_type, Query->ID.toi_id);
        return STATUS_NOT_IMPLEMENTED;
    }

    Status = Handler(Query->ID.toi_entity, &Query->Buffer, Query->BufferSize);

    if (NT_SUCCESS(Status))
        Irp->IoStatus.Information = Query->BufferSize;

    return Status;
}
