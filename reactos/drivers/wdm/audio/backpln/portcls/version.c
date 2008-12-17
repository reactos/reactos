#include "private.h"


typedef struct CResourceList
{
    IPortClsVersionVtbl *lpVtbl;
    LONG ref;
}IPortClsVersionImpl;

const GUID IID_IPortClsVersion;

//---------------------------------------------------------------
// IPortClsVersion interface functions
//

NTSTATUS
NTAPI
IPortClsVersion_fnQueryInterface(
    IPortClsVersion * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortClsVersionImpl * This = (IPortClsVersionImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IPortClsVersion) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IPortClsVersion_fnAddRef(
    IPortClsVersion * iface)
{
    IPortClsVersionImpl * This = (IPortClsVersionImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IPortClsVersion_fnRelease(
    IPortClsVersion * iface)
{
    IPortClsVersionImpl * This = (IPortClsVersionImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        ExFreePoolWithTag(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

DWORD
NTAPI
IPortClsVersion_fnGetVersion(
    IPortClsVersion * iface)
{
    return kVersionWinXP_UAAQFE;
}

static IPortClsVersionVtbl vt_PortClsVersion =
{
    /* IUnknown methods */
    IPortClsVersion_fnQueryInterface,
    IPortClsVersion_fnAddRef,
    IPortClsVersion_fnRelease,
    IPortClsVersion_fnGetVersion
};

NTSTATUS NewPortClsVersion(
    OUT PPORTCLSVERSION * OutVersion)
{
    IPortClsVersionImpl * This = ExAllocatePoolWithTag(NonPagedPool, sizeof(IPortClsVersionImpl), TAG_PORTCLASS);

    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtbl = &vt_PortClsVersion;
    This->ref = 1;
    *OutVersion = (PPORTCLSVERSION)&This->lpVtbl;

    return STATUS_SUCCESS;

}


