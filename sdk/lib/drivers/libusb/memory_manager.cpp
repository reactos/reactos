/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Driver Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/libusb/memory_manager.cpp
 * PURPOSE:     USB Common Driver Library.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "libusb.h"

#define NDEBUG
#include <debug.h>

class CDMAMemoryManager : public IDMAMemoryManager
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }

    // IDMAMemoryManager interface functions
    virtual NTSTATUS Initialize(IN PUSBHARDWAREDEVICE Device, IN PKSPIN_LOCK Lock, IN ULONG DmaBufferSize, IN PVOID VirtualBase, IN PHYSICAL_ADDRESS PhysicalAddress, IN ULONG DefaultBlockSize);
    virtual NTSTATUS Allocate(IN ULONG Size, OUT PVOID *OutVirtualBase, OUT PPHYSICAL_ADDRESS OutPhysicalAddress);
    virtual NTSTATUS Release(IN PVOID VirtualBase, IN ULONG Size);

    // constructor / destructor
    CDMAMemoryManager(IUnknown *OuterUnknown){}
    virtual ~CDMAMemoryManager(){}

protected:
    LONG m_Ref;
    PUSBHARDWAREDEVICE m_Device;
    PKSPIN_LOCK m_Lock;
    LONG m_DmaBufferSize;
    PVOID m_VirtualBase;
    PHYSICAL_ADDRESS m_PhysicalAddress;
    ULONG m_BlockSize;

    PULONG m_BitmapBuffer;
    RTL_BITMAP m_Bitmap;
};

//----------------------------------------------------------------------------------------
NTSTATUS
STDMETHODCALLTYPE
CDMAMemoryManager::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
CDMAMemoryManager::Initialize(
    IN PUSBHARDWAREDEVICE Device,
    IN PKSPIN_LOCK Lock,
    IN ULONG DmaBufferSize,
    IN PVOID VirtualBase,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG DefaultBlockSize)
{
    ULONG BitmapLength;

    //
    // sanity checks
    //
    PC_ASSERT(DmaBufferSize >= PAGE_SIZE);
    PC_ASSERT(DmaBufferSize % PAGE_SIZE == 0);
    PC_ASSERT(DefaultBlockSize == 32 || DefaultBlockSize == 64 || DefaultBlockSize == 128);

    //
    // calculate bitmap length
    //
    BitmapLength = (DmaBufferSize / DefaultBlockSize) / 8;

    //
    // allocate bitmap buffer
    //
    m_BitmapBuffer = (PULONG)ExAllocatePoolWithTag(NonPagedPool, BitmapLength, TAG_USBLIB);
    if (!m_BitmapBuffer)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize bitmap
    //
    RtlInitializeBitMap(&m_Bitmap, m_BitmapBuffer, BitmapLength * 8);

    //
    // clear all bits
    //
    RtlClearAllBits(&m_Bitmap);

    //
    // initialize rest of memory allocator
    //
    m_PhysicalAddress = PhysicalAddress;
    m_VirtualBase = VirtualBase;
    m_DmaBufferSize = DmaBufferSize;
    m_Lock = Lock;
    m_BlockSize = DefaultBlockSize;

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
CDMAMemoryManager::Allocate(
    IN ULONG Size,
    OUT PVOID *OutVirtualAddress,
    OUT PPHYSICAL_ADDRESS OutPhysicalAddress)
{
    ULONG Length, BlockCount, FreeIndex, StartPage, EndPage;
    KIRQL OldLevel;
    ULONG BlocksPerPage;

    //
    // sanity checks
    //
    ASSERT(Size <= PAGE_SIZE);
    //ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // align request
    //
    Length = (Size + m_BlockSize -1) & ~(m_BlockSize -1);

    //
    // sanity check
    //
    ASSERT(Length);

    //
    // convert to block count
    //
    BlockCount = Length / m_BlockSize;

    //
    // acquire lock
    //
    KeAcquireSpinLock(m_Lock, &OldLevel);

    //
    // helper variable
    //
    BlocksPerPage = PAGE_SIZE / m_BlockSize;

    //
    // start search
    //
    FreeIndex = 0;
    do
    {
        //
        // search for an free index
        //
        FreeIndex = RtlFindClearBits(&m_Bitmap, BlockCount, FreeIndex);

        //
        // check if there was a block found
        //
        if (FreeIndex == MAXULONG)
        {
           //
           // no free block found
           //
           break;
        }

        //
        // check that the allocation does not spawn over page boundaries
        //
        StartPage = (FreeIndex * m_BlockSize);
        StartPage = (StartPage != 0 ? StartPage / PAGE_SIZE : 0);
        EndPage = ((FreeIndex + BlockCount) * m_BlockSize) / PAGE_SIZE;

        //
        // does the request start and end on the same page
        //
        if (StartPage == EndPage)
        {
            //
            // reserve block
            //
            RtlSetBits(&m_Bitmap, FreeIndex, BlockCount);

            //
            // reserve block
            //
            break;
        }
        else if ((BlockCount == BlocksPerPage) && (FreeIndex % BlocksPerPage == 0))
        {
            //
            // the request equals PAGE_SIZE and is aligned at page boundary
            // reserve block
            //
            RtlSetBits(&m_Bitmap, FreeIndex, BlockCount);

            //
            // reserve block
            //
            break;
        }
        else
        {
            //
            // request spawned over page boundary
            // restart search on next page
            //
            FreeIndex = (EndPage *  PAGE_SIZE) / m_BlockSize;
        }
    }
    while(TRUE);

    //
    // release lock
    //
    KeReleaseSpinLock(m_Lock, OldLevel);

    //
    // did allocation succeed
    //
    if (FreeIndex == MAXULONG)
    {
        //
        // failed to allocate block, requestor must retry
        //
        return STATUS_UNSUCCESSFUL;
    }

    //
    // return result
    //
    *OutVirtualAddress = (PVOID)((ULONG_PTR)m_VirtualBase + FreeIndex * m_BlockSize);
    OutPhysicalAddress->QuadPart = m_PhysicalAddress.QuadPart + FreeIndex * m_BlockSize;

    //
    // clear block
    //
    RtlZeroMemory(*OutVirtualAddress, Length);

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
CDMAMemoryManager::Release(
    IN PVOID VirtualAddress,
    IN ULONG Size)
{
    KIRQL OldLevel;
    ULONG BlockOffset = 0, BlockLength, BlockCount;

    //
    // sanity checks
    //
    PC_ASSERT(VirtualAddress);
    PC_ASSERT((ULONG_PTR)VirtualAddress >= (ULONG_PTR)m_VirtualBase);
    PC_ASSERT((ULONG_PTR)m_VirtualBase + m_DmaBufferSize > (ULONG_PTR)m_VirtualBase);

    //
    // calculate block length
    //
    BlockLength = ((ULONG_PTR)VirtualAddress - (ULONG_PTR)m_VirtualBase);

    //
    // check if its the first block
    //
    if (BlockLength)
    {
        //
        // divide by base block size
        //
        BlockOffset = BlockLength / m_BlockSize;
    }

    //
    // align length to block size
    //
    Size = (Size + m_BlockSize - 1) & ~(m_BlockSize - 1);

    //
    // convert to blocks
    //
    BlockCount = Size / m_BlockSize;
    ASSERT(BlockCount);

    //
    // acquire lock
    //
    KeAcquireSpinLock(m_Lock, &OldLevel);

    //
    // sanity check
    //
    ASSERT(RtlAreBitsSet(&m_Bitmap, BlockOffset, BlockCount));

    //
    // release buffer
    //
    RtlClearBits(&m_Bitmap, BlockOffset, BlockCount);

    //
    // release lock
    //
    KeReleaseSpinLock(m_Lock, OldLevel);

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CreateDMAMemoryManager(
    PDMAMEMORYMANAGER *OutMemoryManager)
{
    CDMAMemoryManager* This;

    //
    // allocate controller
    //
    This = new(NonPagedPool, TAG_USBLIB) CDMAMemoryManager(0);
    if (!This)
    {
        //
        // failed to allocate
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // add reference count
    //
    This->AddRef();

    //
    // return result
    //
    *OutMemoryManager = (PDMAMEMORYMANAGER)This;

    //
    // done
    //
    return STATUS_SUCCESS;
}

