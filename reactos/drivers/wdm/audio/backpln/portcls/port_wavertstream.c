/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port_wavertstream.c
 * PURPOSE:         WaveRTStream helper object
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IPortWaveRTStreamInitVtbl *lpVtbl;
    LONG ref;

}IPortWaveRTStreamImpl;

/*
 * @implemented
 */
static
NTSTATUS
NTAPI
IPortWaveRTStreamInit_fnQueryInterface(
    IPortWaveRTStreamInit * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortWaveRTStreamImpl * This = (IPortWaveRTStreamImpl*)iface;

    DPRINT("IPortWaveRTStream_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, &IID_IPortWaveRTStream) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
static
ULONG
NTAPI
IPortWaveRTStreamInit_fnAddRef(
    IPortWaveRTStreamInit * iface)
{
    IPortWaveRTStreamImpl * This = (IPortWaveRTStreamImpl*)iface;
    DPRINT("IPortWaveRTStream_fnAddRef entered\n");

    return InterlockedIncrement(&This->ref);
}

/*
 * @implemented
 */
static
ULONG
NTAPI
IPortWaveRTStreamInit_fnRelease(
    IPortWaveRTStreamInit * iface)
{
    IPortWaveRTStreamImpl * This = (IPortWaveRTStreamImpl*)iface;

    InterlockedDecrement(&This->ref);

    DPRINT("IPortWaveRTStream_fnRelease entered %u\n", This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

/*
 * @implemented
 */
static
PMDL
NTAPI
IPortWaveRTStreamInit_fnAllocatePagesForMdl(
    IN IPortWaveRTStreamInit * iface,
    IN PHYSICAL_ADDRESS HighAddress,
    IN SIZE_T TotalBytes)
{
    return MmAllocatePagesForMdl(RtlConvertUlongToLargeInteger(0), HighAddress, RtlConvertUlongToLargeInteger(0), TotalBytes);
}

/*
 * @implemented
 */
static
PMDL
NTAPI
IPortWaveRTStreamInit_fnAllocateContiguousPagesForMdl(
    IN IPortWaveRTStreamInit * iface,
    IN PHYSICAL_ADDRESS LowAddress,
    IN PHYSICAL_ADDRESS HighAddress,
    IN SIZE_T TotalBytes)
{
    PMDL Mdl;
    PVOID Buffer;
    PHYSICAL_ADDRESS Address;

    Buffer = MmAllocateContiguousMemorySpecifyCache(TotalBytes, LowAddress, HighAddress, RtlConvertUlongToLargeInteger(0), MmNonCached);
    if (!Buffer)
    {
        DPRINT1("MmAllocateContiguousMemorySpecifyCache failed\n");
        return NULL;
    }

    Address = MmGetPhysicalAddress(Buffer);

    MmFreeContiguousMemorySpecifyCache(Buffer, TotalBytes, MmNonCached);

    Mdl = MmAllocatePagesForMdl(Address, HighAddress, RtlConvertUlongToLargeInteger(0), TotalBytes);
    if (!Mdl)
    {
        DPRINT1("MmAllocatePagesForMdl failed\n");
        return NULL;
    }

    if (MmGetMdlByteCount(Mdl) < TotalBytes)
    {
        DPRINT1("ByteCount %u Required %u\n", MmGetMdlByteCount(Mdl), TotalBytes);
        MmFreePagesFromMdl(Mdl);
        ExFreePool(Mdl);
        return NULL;
    }

    DPRINT("Result %p\n", Mdl);
    return Mdl;
}

/*
 * @implemented
 */
static
PVOID
NTAPI
IPortWaveRTStreamInit_fnMapAllocatedPages(
    IN IPortWaveRTStreamInit * iface,
    IN PMDL MemoryDescriptorList,
    IN MEMORY_CACHING_TYPE CacheType)
{
    return MmMapLockedPagesSpecifyCache(MemoryDescriptorList, KernelMode, CacheType, NULL, 0, NormalPagePriority);
}

/*
 * @implemented
 */
static
VOID
NTAPI
IPortWaveRTStreamInit_fnUnmapAllocatedPages(
    IN IPortWaveRTStreamInit * iface,
    IN PVOID   BaseAddress,
    IN PMDL MemoryDescriptorList)
{
    MmUnmapLockedPages(BaseAddress, MemoryDescriptorList);
}

/*
 * @implemented
 */
static
VOID
NTAPI
IPortWaveRTStreamInit_fnFreePagesFromMdl(
    IN IPortWaveRTStreamInit * iface,
    IN PMDL MemoryDescriptorList)
{
    MmFreePagesFromMdl(MemoryDescriptorList);
    ExFreePool(MemoryDescriptorList);
}

/*
 * @implemented
 */
static
ULONG
NTAPI
IPortWaveRTStreamInit_fnGetPhysicalPagesCount(
    IN IPortWaveRTStreamInit * iface,
    IN PMDL MemoryDescriptorList)
{
    return ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, MmGetMdlByteCount(MemoryDescriptorList));
}

/*
 * @implemented
 */
static
PHYSICAL_ADDRESS
NTAPI
IPortWaveRTStreamInit_fnGetPhysicalPageAddress(
    IN IPortWaveRTStreamInit * iface,
    IN PPHYSICAL_ADDRESS Address,
    IN PMDL MemoryDescriptorList,
    IN ULONG Index)
{
    PVOID Buffer;
    ULONG Pages;
    PHYSICAL_ADDRESS Result, Addr;

    Pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, MmGetMdlByteCount(MemoryDescriptorList));
    if (Pages <= Index)
    {
        DPRINT1("OutOfBounds: Pages %u Index %u\n", Pages, Index);
        return RtlConvertUlongToLargeInteger(0);
    }

    Buffer = UlongToPtr(PtrToUlong(MmGetSystemAddressForMdl(MemoryDescriptorList)) + Index * PAGE_SIZE);

    Addr = MmGetPhysicalAddress(Buffer);
    Address->QuadPart = Addr.QuadPart;
    Result.QuadPart = (PtrToUlong(Address));

    return Result;
}

static IPortWaveRTStreamInitVtbl vt_PortWaveRTStream =
{
    IPortWaveRTStreamInit_fnQueryInterface,
    IPortWaveRTStreamInit_fnAddRef,
    IPortWaveRTStreamInit_fnRelease,
    IPortWaveRTStreamInit_fnAllocatePagesForMdl,
    IPortWaveRTStreamInit_fnAllocateContiguousPagesForMdl,
    IPortWaveRTStreamInit_fnMapAllocatedPages,
    IPortWaveRTStreamInit_fnUnmapAllocatedPages,
    IPortWaveRTStreamInit_fnFreePagesFromMdl,
    IPortWaveRTStreamInit_fnGetPhysicalPagesCount,
    IPortWaveRTStreamInit_fnGetPhysicalPageAddress
};

NTSTATUS
NewPortWaveRTStream(
    PPORTWAVERTSTREAM *OutStream)
{
    IPortWaveRTStreamImpl* This = AllocateItem(NonPagedPool, sizeof(IPortWaveRTStreamImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->ref = 1;
    This->lpVtbl = &vt_PortWaveRTStream;

    *OutStream = (PPORTWAVERTSTREAM)&This->lpVtbl;
    return STATUS_SUCCESS;
}
