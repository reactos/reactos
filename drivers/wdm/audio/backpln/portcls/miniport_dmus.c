#include "private.h"

typedef struct
{
    IMiniportDMusVtbl *lpVtbl;
    LONG ref;
    CLSID ClassId;

}IMiniportDMusImpl;



/* IUnknown methods */

NTSTATUS
STDMETHODCALLTYPE
IMiniportDMus_fnQueryInterface(
    IMiniportDMus* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IMiniportDMusImpl * This = (IMiniportDMusImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IMiniportDMus))
    {
        *Output = &This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

ULONG
STDMETHODCALLTYPE
IMiniportDMus_fnAddRef(
    IMiniportDMus* iface)
{
    IMiniportDMusImpl * This = (IMiniportDMusImpl*)iface;

    return _InterlockedIncrement(&This->ref);
}

ULONG
STDMETHODCALLTYPE
IMiniportDMust_fnRelease(
    IMiniportDMus* iface)
{
    IMiniportDMusImpl * This = (IMiniportDMusImpl*)iface;

    _InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

/* IMiniport methods */

NTSTATUS
NTAPI
IMiniportDMus_fnDataRangeIntersection(
    IN IMiniportDMus * iface,
    IN ULONG  PinId,
    IN PKSDATARANGE  DataRange,
    IN PKSDATARANGE  MatchingDataRange,
    IN ULONG OutputBufferLength,
    OUT PVOID ResultantFormat  OPTIONAL,
    OUT PULONG ResultantFormatLength)
{
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
IMiniportDMus_fnGetDescription(
    IN IMiniportDMus * iface,
    OUT PPCFILTER_DESCRIPTOR  *Description
    )
{
    return STATUS_UNSUCCESSFUL;
}

/* IMinIMiniportDMus methods */

HRESULT
NTAPI
IMiniportDMus_fnInit(
    IN IMiniportDMus * iface,
    IN PUNKNOWN  pUnknownAdapter,
    IN PRESOURCELIST  pResourceList,
    IN PPORTDMUS  pPort,
    OUT PSERVICEGROUP  *ppServiceGroup
    )
{
    return STATUS_UNSUCCESSFUL;
}

HRESULT
NTAPI
IMiniportDMus_fnNewStream(
    IN IMiniportDMus * iface,
    OUT PMXF  *ppMXF,
    IN PUNKNOWN  pOuterUnknown  OPTIONAL,
    IN POOL_TYPE  PoolType,
    IN ULONG  uPinId,
    IN DMUS_STREAM_TYPE  StreamType,
    IN PKSDATAFORMAT  pDataFormat,
    OUT PSERVICEGROUP  *ppServiceGroup,
    IN PAllocatorMXF  pAllocatorMXF,
    IN PMASTERCLOCK  pMasterClock,
    OUT PULONGLONG  puuSchedulePreFetch
    )
{
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
IMiniportDMus_fnService(
    IN IMiniportDMus * iface)
{

}

static IMiniportDMusVtbl vt_PortDMusVtbl =
{
    /* IUnknown */
    IMiniportDMus_fnQueryInterface,
    IMiniportDMus_fnAddRef,
    IMiniportDMust_fnRelease,
    /* IMiniport */
    IMiniportDMus_fnGetDescription,
    IMiniportDMus_fnDataRangeIntersection,
    /* IMiniportDMus */
    IMiniportDMus_fnInit,
    IMiniportDMus_fnService,
    IMiniportDMus_fnNewStream
};

NTSTATUS
NewMiniportDMusUART(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId)
{
    IMiniportDMusImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IMiniportDMusImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize IMiniportDMus */
    RtlCopyMemory(&This->ClassId, ClassId, sizeof(CLSID));
    This->ref = 1;
    This->lpVtbl = &vt_PortDMusVtbl;
    *OutMiniport = (PMINIPORT)&This->lpVtbl;

    return STATUS_SUCCESS;
}


