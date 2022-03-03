/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port_wavertstream.cpp
 * PURPOSE:         WaveRTStream helper object
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

class CPortWaveRTStreamInit : public CUnknownImpl<IPortWaveRTStreamInit>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IPortWaveRTStreamInit;
    CPortWaveRTStreamInit(IUnknown *OuterUnknown) {}
    virtual ~CPortWaveRTStreamInit() {}

};

NTSTATUS
NTAPI
CPortWaveRTStreamInit::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{

    DPRINT("IPortWaveRTStream_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, IID_IPortWaveRTStream) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PPORTWAVERTSTREAM(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}


PMDL
NTAPI
CPortWaveRTStreamInit::AllocatePagesForMdl(
    IN PHYSICAL_ADDRESS HighAddress,
    IN SIZE_T TotalBytes)
{
    return MmAllocatePagesForMdl(RtlConvertUlongToLargeInteger(0), HighAddress, RtlConvertUlongToLargeInteger(0), TotalBytes);
}

PMDL
NTAPI
CPortWaveRTStreamInit::AllocateContiguousPagesForMdl(
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
        DPRINT("MmAllocateContiguousMemorySpecifyCache failed\n");
        return NULL;
    }

    Address = MmGetPhysicalAddress(Buffer);

    MmFreeContiguousMemorySpecifyCache(Buffer, TotalBytes, MmNonCached);

    Mdl = MmAllocatePagesForMdl(Address, HighAddress, RtlConvertUlongToLargeInteger(0), TotalBytes);
    if (!Mdl)
    {
        DPRINT("MmAllocatePagesForMdl failed\n");
        return NULL;
    }

    if (MmGetMdlByteCount(Mdl) < TotalBytes)
    {
        DPRINT("ByteCount %u Required %u\n", MmGetMdlByteCount(Mdl), TotalBytes);
        MmFreePagesFromMdl(Mdl);
        ExFreePool(Mdl);
        return NULL;
    }

    DPRINT("Result %p\n", Mdl);
    return Mdl;
}

PVOID
NTAPI
CPortWaveRTStreamInit::MapAllocatedPages(
    IN PMDL MemoryDescriptorList,
    IN MEMORY_CACHING_TYPE CacheType)
{
    return MmMapLockedPagesSpecifyCache(MemoryDescriptorList, KernelMode, CacheType, NULL, 0, NormalPagePriority);
}

VOID
NTAPI
CPortWaveRTStreamInit::UnmapAllocatedPages(
    IN PVOID   BaseAddress,
    IN PMDL MemoryDescriptorList)
{
    MmUnmapLockedPages(BaseAddress, MemoryDescriptorList);
}

VOID
NTAPI
CPortWaveRTStreamInit::FreePagesFromMdl(
    IN PMDL MemoryDescriptorList)
{
    MmFreePagesFromMdl(MemoryDescriptorList);
    ExFreePool(MemoryDescriptorList);
}

ULONG
NTAPI
CPortWaveRTStreamInit::GetPhysicalPagesCount(
    IN PMDL MemoryDescriptorList)
{
    return ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, MmGetMdlByteCount(MemoryDescriptorList));
}

PHYSICAL_ADDRESS
NTAPI
CPortWaveRTStreamInit::GetPhysicalPageAddress(
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
        DPRINT("OutOfBounds: Pages %u Index %u\n", Pages, Index);
        return RtlConvertUlongToLargeInteger(0);
    }

    Buffer = (PUCHAR)MmGetSystemAddressForMdlSafe(MemoryDescriptorList, LowPagePriority) + (Index * PAGE_SIZE);

    Addr = MmGetPhysicalAddress(Buffer);
    Address->QuadPart = Addr.QuadPart;
    Result.QuadPart = (ULONG_PTR)Address;

    return Result;
}


NTSTATUS
NewPortWaveRTStream(
    PPORTWAVERTSTREAM *OutStream)
{
    NTSTATUS Status;
    CPortWaveRTStreamInit* This = new(NonPagedPool, TAG_PORTCLASS) CPortWaveRTStreamInit(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = This->QueryInterface(IID_IPortWaveRTStream, (PVOID*)OutStream);

    if (!NT_SUCCESS(Status))
    {
        delete This;
        return Status;
    }

    *OutStream = (PPORTWAVERTSTREAM)This;
    return Status;
}
