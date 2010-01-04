/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         See COPYING in the top level directory
 * FILE:            hal/halamd64/generic/x86bios.c
 * PURPOSE:         
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
//#define NDEBUG
#include <debug.h>

/* This page serves as fallback for pages used by Mm */
#define DEFAULT_PAGE 0x21

/* GLOBALS *******************************************************************/

BOOLEAN x86BiosIsInitialized;
LONG x86BiosBufferIsAllocated = 0;
PUCHAR x86BiosMemoryMapping;

VOID
NTAPI
HalInitializeBios(ULONG Unknown, PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PPFN_NUMBER PfnArray;
    PFN_NUMBER Pfn, Last;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PLIST_ENTRY ListEntry;
    PMDL Mdl;

    /* Allocate an MDL for 1MB */
    Mdl = IoAllocateMdl(NULL, 0x100000, FALSE, FALSE, NULL);
    if (!Mdl)
    {
        ASSERT(FALSE);
    }

    /* Get pointer to the pfn array */
    PfnArray = MmGetMdlPfnArray(Mdl);

    /* Fill the array with low memory PFNs */
    for (Pfn = 0; Pfn < 0x100; Pfn++)
    {
        PfnArray[Pfn] = Pfn;
    }

    /* Loop the memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Get the memory descriptor */
        Descriptor = CONTAINING_RECORD(ListEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        /* Check if the memory is in the low range */
        if (Descriptor->BasePage < 0x100)
        {
            /* Check if the memory type is firmware */
            if (Descriptor->MemoryType != LoaderFirmwarePermanent &&
                Descriptor->MemoryType != LoaderFirmwarePermanent)
            {
                /* It's something else, so don't use it! */
                Last = min(Descriptor->BasePage + Descriptor->PageCount, 0x100);
                for (Pfn = Descriptor->BasePage; Pfn < Last; Pfn++)
                {
                    /* Set each page to the default page */
                    PfnArray[Pfn] = DEFAULT_PAGE;
                }
            }
        }
    }

    /* Map the MDL to system space */
    x86BiosMemoryMapping = MmGetSystemAddressForMdlSafe(Mdl, HighPagePriority);
    ASSERT(x86BiosMemoryMapping);

    DPRINT1("memory: %p, %p\n", *(PVOID*)x86BiosMemoryMapping, *(PVOID*)(x86BiosMemoryMapping + 8));

    x86BiosIsInitialized = TRUE;
}

NTSTATUS
NTAPI
x86BiosAllocateBuffer (
    ULONG *Size,
    USHORT *Segment,
    USHORT *Offset)
{
    /* Check if the system is initialized and the buffer is large enough */
    if (!x86BiosIsInitialized || *Size > PAGE_SIZE)
    {
        /* Something was wrong, fail! */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Check if the buffer is already allocated */
    if (InterlockedBitTestAndSet(&x86BiosBufferIsAllocated, 0))
    {
        /* Buffer was already allocated, fail */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* The buffer is sufficient, return hardcoded address and size */
    *Size = PAGE_SIZE;
    *Segment = 0x2000;
    *Offset = 0;

    return STATUS_SUCCESS;;
}

NTSTATUS
NTAPI
x86BiosFreeBuffer (
    USHORT Segment,
    USHORT Offset)
{
    /* Check if the system is initialized and if the address matches */
    if (!x86BiosIsInitialized || Segment != 0x2000 || Offset != 0)
    {
        /* Something was wrong, fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if the buffer was allocated */
    if (!InterlockedBitTestAndReset(&x86BiosBufferIsAllocated, 0))
    {
        /* It was not, fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Buffer is freed, nothing more to do */
    return STATUS_SUCCESS;;
}

NTSTATUS
NTAPI
x86BiosReadMemory (
    USHORT Segment,
    USHORT Offset,
    PVOID Buffer,
    ULONG Size)
{
    ULONG_PTR Address;

    /* Calculate the physical address */
    Address = (Segment << 4) + Offset;

    /* Check if it's valid */
    if (!x86BiosIsInitialized || Address + Size > 0x100000)
    {
        /* Invalid */
        return STATUS_INVALID_PARAMETER;
    }

    /* Copy the memory to the buffer */
    RtlCopyMemory(Buffer, x86BiosMemoryMapping + Address, Size);

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
x86BiosWriteMemory (
    USHORT Segment,
    USHORT Offset,
    PVOID Buffer,
    ULONG Size)
{
    ULONG_PTR Address;

    /* Calculate the physical address */
    Address = (Segment << 4) + Offset;

    /* Check if it's valid */
    if (!x86BiosIsInitialized || Address + Size > 0x100000)
    {
        /* Invalid */
        return STATUS_INVALID_PARAMETER;
    }

    /* Copy the memory from the buffer */
    RtlCopyMemory(x86BiosMemoryMapping + Address, Buffer, Size);

    /* Return success */
    return STATUS_SUCCESS;
}

typedef struct
{
    union
    {
        ULONG Eax;
        USHORT Ax;
        struct
        {
            UCHAR Al;
            UCHAR Ah;
        };
    };
    union
    {
        ULONG Ecx;
        USHORT Cx;
        struct
        {
            UCHAR Cl;
            UCHAR Ch;
        };
    };
    union
    {
        ULONG Edx;
        USHORT Dx;
        struct
        {
            UCHAR Dl;
            UCHAR Dh;
        };
    };
    union
    {
        ULONG Ebx;
        USHORT Bx;
        struct
        {
            UCHAR Bl;
            UCHAR Bh;
        };
    };
    ULONG Ebp;
    ULONG Esi;
    ULONG Edi;
    USHORT SegDs;
    USHORT SegEs;

    /* Extended */
    union
    {
        ULONG Eip;
        USHORT Ip;
    };

    union
    {
        ULONG Esp;
        USHORT Sp;
    };

} X86_REGISTERS, *PX86_REGISTERS;

enum
{
    X86_VMFLAGS_RETURN_ON_IRET = 1,
};

typedef struct
{
    union
    {
        X86_BIOS_REGISTERS BiosRegisters;
        X86_REGISTERS Registers;
    };
    
    struct
    {
        ULONG ReturnOnIret:1;
    } Flags;
    
    PVOID MemBuffer;
} X86_VM_STATE, *PX86_VM_STATE;

BOOLEAN
NTAPI
x86BiosCall (
    ULONG InterruptNumber,
    X86_BIOS_REGISTERS *Registers)
{
    X86_VM_STATE VmState;

    /* Zero the VmState */
    RtlZeroMemory(&VmState, sizeof(VmState));

    /* Copy the registers */
    VmState.BiosRegisters = *Registers;

    /* Set the physical memory buffer */
    VmState.MemBuffer = x86BiosMemoryMapping;

    /* Initialize IP from the interrupt vector table */
    VmState.Registers.Ip = ((PUSHORT)x86BiosMemoryMapping)[InterruptNumber];

    /* Make the function return on IRET */
    VmState.Flags.ReturnOnIret = 1;

    /* Call the x86 emulator */
//    x86Emulator(&VmState);

    /* Copy registers back to caller */
    *Registers = VmState.BiosRegisters;

    return TRUE;
}

BOOLEAN
NTAPI
HalpBiosDisplayReset(VOID)
{
    UNIMPLEMENTED;
    return TRUE;
}

