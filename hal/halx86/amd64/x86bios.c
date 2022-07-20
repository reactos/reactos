/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL, See COPYING in the top level directory
 * FILE:            hal/halx86/amd64/x86bios.c
 * PURPOSE:
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
//#define NDEBUG
#include <debug.h>

#include <fast486.h>

/* GLOBALS *******************************************************************/

/* This page serves as fallback for pages used by Mm */
PFN_NUMBER x86BiosFallbackPfn;

BOOLEAN x86BiosIsInitialized;
LONG x86BiosBufferIsAllocated = 0;
PUCHAR x86BiosMemoryMapping;

/* This the physical address of the bios buffer */
ULONG64 x86BiosBufferPhysical;

VOID
NTAPI
DbgDumpPage(PUCHAR MemBuffer, USHORT Segment)
{
    ULONG x, y, Offset;

    for (y = 0; y < 0x100; y++)
    {
        for (x = 0; x < 0x10; x++)
        {
            Offset = Segment * 16 + y * 16 + x;
            DbgPrint("%02x ", MemBuffer[Offset]);
        }
        DbgPrint("\n");
    }
}

VOID
NTAPI
HalInitializeBios(
    _In_ ULONG Phase,
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PPFN_NUMBER PfnArray;
    PFN_NUMBER Pfn, Last;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PLIST_ENTRY ListEntry;
    PMDL Mdl;
    ULONG64 PhysicalAddress;

    if (Phase == 0)
    {
        /* Allocate one page for a fallback mapping */
        PhysicalAddress = HalpAllocPhysicalMemory(LoaderBlock,
                                                  0x100000,
                                                  1,
                                                  FALSE);
        if (PhysicalAddress == 0)
        {
            ASSERT(FALSE);
        }

        x86BiosFallbackPfn = PhysicalAddress / PAGE_SIZE;
        ASSERT(x86BiosFallbackPfn != 0);

        /* Allocate a page for the buffer allocation */
        x86BiosBufferPhysical = HalpAllocPhysicalMemory(LoaderBlock,
                                                        0x100000,
                                                        1,
                                                        FALSE);
        if (x86BiosBufferPhysical == 0)
        {
            ASSERT(FALSE);
        }
    }
    else
    {

        /* Allocate an MDL for 1MB */
        Mdl = IoAllocateMdl(NULL, 0x100000, FALSE, FALSE, NULL);
        if (!Mdl)
        {
            ASSERT(FALSE);
        }

        /* Get pointer to the pfn array */
        PfnArray = MmGetMdlPfnArray(Mdl);

        /* Fill the array with the fallback page */
        for (Pfn = 0; Pfn < 0x100; Pfn++)
        {
            PfnArray[Pfn] = x86BiosFallbackPfn;
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

            /* Check if the memory is in the low 1 MB range */
            if (Descriptor->BasePage < 0x100)
            {
                /* Check if the memory type is firmware */
                if ((Descriptor->MemoryType == LoaderFirmwarePermanent) ||
                    (Descriptor->MemoryType == LoaderSpecialMemory))
                {
                    /* It's firmware, so map it! */
                    Last = min(Descriptor->BasePage + Descriptor->PageCount, 0x100);
                    for (Pfn = Descriptor->BasePage; Pfn < Last; Pfn++)
                    {
                        /* Set each physical page in the MDL */
                        PfnArray[Pfn] = Pfn;
                    }
                }
            }
        }

        /* Map this page proper, too */
        Pfn = x86BiosBufferPhysical / PAGE_SIZE;
        PfnArray[Pfn] = Pfn;

        Mdl->MdlFlags = MDL_PAGES_LOCKED;

        /* Map the MDL to system space */
        x86BiosMemoryMapping = MmGetSystemAddressForMdlSafe(Mdl, HighPagePriority);
        ASSERT(x86BiosMemoryMapping);

        DPRINT1("memory: %p, %p\n", *(PVOID*)x86BiosMemoryMapping, *(PVOID*)(x86BiosMemoryMapping + 8));
        //DbgDumpPage(x86BiosMemoryMapping, 0xc351);

        x86BiosIsInitialized = TRUE;

        HalpBiosDisplayReset();
    }
}

NTSTATUS
NTAPI
x86BiosAllocateBuffer(
    _In_ ULONG *Size,
    _In_ USHORT *Segment,
    _In_ USHORT *Offset)
{
    /* Check if the system is initialized and the buffer is large enough */
    if (!x86BiosIsInitialized || (*Size > PAGE_SIZE))
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
    *Segment = x86BiosBufferPhysical / 16;
    *Offset = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
x86BiosFreeBuffer(
    _In_ USHORT Segment,
    _In_ USHORT Offset)
{
    /* Check if the system is initialized and if the address matches */
    if (!x86BiosIsInitialized || (Segment != 0x2000) || (Offset != 0))
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
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
x86BiosReadMemory(
    _In_ USHORT Segment,
    _In_ USHORT Offset,
    _Out_writes_bytes_(Size) PVOID Buffer,
    _In_ ULONG Size)
{
    ULONG_PTR Address;

    /* Calculate the physical address */
    Address = (Segment << 4) + Offset;

    /* Check if it's valid */
    if (!x86BiosIsInitialized || ((Address + Size) > 0x100000))
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
x86BiosWriteMemory(
    _In_ USHORT Segment,
    _In_ USHORT Offset,
    _In_reads_bytes_(Size) PVOID Buffer,
    _In_ ULONG Size)
{
    ULONG_PTR Address;

    /* Calculate the physical address */
    Address = (Segment << 4) + Offset;

    /* Check if it's valid */
    if (!x86BiosIsInitialized || ((Address + Size) > 0x100000))
    {
        /* Invalid */
        return STATUS_INVALID_PARAMETER;
    }

    /* Copy the memory from the buffer */
    RtlCopyMemory(x86BiosMemoryMapping + Address, Buffer, Size);

    /* Return success */
    return STATUS_SUCCESS;
}

static
VOID
FASTCALL
x86MemRead(
    PFAST486_STATE State,
    ULONG Address,
    PVOID Buffer,
    ULONG Size)
{
    /* Validate the address range */
    if (((ULONG64)Address + Size) < 0x100000)
    {
        RtlCopyMemory(Buffer, x86BiosMemoryMapping + Address, Size);
    }
    else
    {
        RtlFillMemory(Buffer, Size, 0xCC);
        DPRINT1("x86MemRead: invalid read at 0x%lx (size 0x%lx)", Address, Size);
    }
}

static
VOID
FASTCALL
x86MemWrite(
    PFAST486_STATE State,
    ULONG Address,
    PVOID Buffer,
    ULONG Size)
{
    /* Validate the address range */
    if (((ULONG64)Address + Size) < 0x100000)
    {
        RtlCopyMemory(x86BiosMemoryMapping + Address, Buffer, Size);
    }
    else
    {
        DPRINT1("x86MemWrite: invalid write at 0x%lx (size 0x%lx)", Address, Size);
    }
}

static
BOOLEAN
ValidatePort(
    USHORT Port,
    UCHAR Size,
    BOOLEAN IsWrite)
{
    switch (Port)
    {
        // VGA: https://wiki.osdev.org/VGA_Hardware#Port_0x3C0
        case 0x3C0: return (Size == 1) && IsWrite;
        case 0x3C1: return (Size == 1) && !IsWrite;
        case 0x3C2: return (Size == 1) && IsWrite;
        case 0x3C4: return IsWrite;
        case 0x3C5: return (Size <= 2);
        case 0x3C7: return (Size == 1) && IsWrite;
        case 0x3CC: return (Size == 1) && !IsWrite;
        case 0x3CE: return IsWrite;
        case 0x3CF: return (Size <= 2);
        case 0x3D4: return IsWrite;
        case 0x3D5: return (Size <= 2);
        case 0x3C6: return (Size == 1);
        case 0x3C8: return (Size == 1) && IsWrite;
        case 0x3C9: return (Size == 1);
        case 0x3DA: return (Size == 1) && !IsWrite;

        // CHECKME!
        case 0x1CE: return (Size == 1) && IsWrite;
        case 0x1CF: return (Size == 1);
        case 0x3B6: return (Size <= 2);
    }

    /* Allow but report unknown ports, we trust the BIOS for now */
    DPRINT1("Unknown port 0x%x, size %d, write %d\n", Port, Size, IsWrite);
    return TRUE;
}

static
VOID
FASTCALL
x86IoRead(
    PFAST486_STATE State,
    USHORT Port,
    PVOID Buffer,
    ULONG DataCount,
    UCHAR DataSize)
{
    /* Validate the port */
    if (!ValidatePort(Port, DataSize, FALSE))
    {
        DPRINT1("Invalid IO port read access (port: 0x%x, count: 0x%x)\n", Port, DataSize);
    }

    switch (DataSize)
    {
        case 1: READ_PORT_BUFFER_UCHAR((PUCHAR)(ULONG_PTR)Port, Buffer, DataCount); return;
        case 2: READ_PORT_BUFFER_USHORT((PUSHORT)(ULONG_PTR)Port, Buffer, DataCount); return;
        case 4: READ_PORT_BUFFER_ULONG((PULONG)(ULONG_PTR)Port, Buffer, DataCount); return;
    }
}

static
VOID
FASTCALL
x86IoWrite(
    PFAST486_STATE State,
    USHORT Port,
    PVOID Buffer,
    ULONG DataCount,
    UCHAR DataSize)
{
    /* Validate the port */
    if (!ValidatePort(Port, DataSize, TRUE))
    {
        DPRINT1("Invalid IO port write access (port: 0x%x, count: 0x%x)\n", Port, DataSize);
    }

    switch (DataSize)
    {
        case 1: WRITE_PORT_BUFFER_UCHAR((PUCHAR)(ULONG_PTR)Port, Buffer, DataCount); return;
        case 2: WRITE_PORT_BUFFER_USHORT((PUSHORT)(ULONG_PTR)Port, Buffer, DataCount); return;
        case 4: WRITE_PORT_BUFFER_ULONG((PULONG)(ULONG_PTR)Port, Buffer, DataCount); return;
    }
}

static
VOID
FASTCALL
x86BOP(
    PFAST486_STATE State,
    UCHAR BopCode)
{
    ASSERT(FALSE);
}

static
UCHAR
FASTCALL
x86IntAck (
    PFAST486_STATE State)
{
    ASSERT(FALSE);
    return 0;
}

BOOLEAN
NTAPI
x86BiosCall(
    _In_ ULONG InterruptNumber,
    _Inout_ PX86_BIOS_REGISTERS Registers)
{
    FAST486_STATE EmulatorContext;
    struct
    {
        USHORT Ip;
        USHORT SegCs;
    } *Ivt;
    ULONG FlatIp;
    PUCHAR InstructionPointer;

    /* Initialize the emulator context */
    Fast486Initialize(&EmulatorContext,
                      x86MemRead,
                      x86MemWrite,
                      x86IoRead,
                      x86IoWrite,
                      x86BOP,
                      x86IntAck,
                      NULL,  // FpuCallback,
                      NULL); // Tlb

//RegisterBop(BOP_UNSIMULATE, CpuUnsimulateBop);

    /* Copy the registers */
    EmulatorContext.GeneralRegs[FAST486_REG_EAX].Long = Registers->Eax;
    EmulatorContext.GeneralRegs[FAST486_REG_EBX].Long = Registers->Ebx;
    EmulatorContext.GeneralRegs[FAST486_REG_ECX].Long = Registers->Ecx;
    EmulatorContext.GeneralRegs[FAST486_REG_EDX].Long = Registers->Edx;
    EmulatorContext.GeneralRegs[FAST486_REG_ESI].Long = Registers->Esi;
    EmulatorContext.GeneralRegs[FAST486_REG_EDI].Long = Registers->Edi;
    EmulatorContext.SegmentRegs[FAST486_REG_DS].Selector = Registers->SegDs;
    EmulatorContext.SegmentRegs[FAST486_REG_ES].Selector = Registers->SegEs;

    /* Set Eflags */
    EmulatorContext.Flags.Long = 0;
    EmulatorContext.Flags.AlwaysSet = 1;
    EmulatorContext.Flags.If = 1;

    /* Set the stack pointer */
    Fast486SetStack(&EmulatorContext, 0, 0x2000 - 2); // FIXME

    /* Set CS:EIP from the IVT entry */
    Ivt = (PVOID)x86BiosMemoryMapping;
    Fast486ExecuteAt(&EmulatorContext,
                     Ivt[InterruptNumber].SegCs,
                     Ivt[InterruptNumber].Ip);

    while (TRUE)
    {
        /* Step one instruction */
        Fast486StepInto(&EmulatorContext);

        /* Check for iret */
        FlatIp = (EmulatorContext.SegmentRegs[FAST486_REG_CS].Selector << 4) +
                 EmulatorContext.InstPtr.Long;
        if (FlatIp >= 0x100000)
        {
            DPRINT1("x86BiosCall: invalid IP (0x%lx) during BIOS execution", FlatIp);
            return FALSE;
        }

        /* Read the next instruction and check if it's IRET */
        InstructionPointer = x86BiosMemoryMapping + FlatIp;
        if (*InstructionPointer == 0xCF)
        {
            /* We are done! */
            break;
        }
    }

    /* Copy the registers back */
    Registers->Eax = EmulatorContext.GeneralRegs[FAST486_REG_EAX].Long;
    Registers->Ebx = EmulatorContext.GeneralRegs[FAST486_REG_EBX].Long;
    Registers->Ecx = EmulatorContext.GeneralRegs[FAST486_REG_ECX].Long;
    Registers->Edx = EmulatorContext.GeneralRegs[FAST486_REG_EDX].Long;
    Registers->Esi = EmulatorContext.GeneralRegs[FAST486_REG_ESI].Long;
    Registers->Edi = EmulatorContext.GeneralRegs[FAST486_REG_EDI].Long;
    Registers->SegDs = EmulatorContext.SegmentRegs[FAST486_REG_DS].Selector;
    Registers->SegEs = EmulatorContext.SegmentRegs[FAST486_REG_ES].Selector;

    return TRUE;
}

BOOLEAN
NTAPI
HalpBiosDisplayReset(VOID)
{
#if 0
    X86_BIOS_REGISTERS Registers;
    ULONG OldEflags;

    /* Save flags and disable interrupts */
    OldEflags = __readeflags();
    _disable();

    /* Set AH = 0 (Set video mode), AL = 0x12 (640x480x16 vga) */
    Registers.Eax = 0x12;

    /* Call INT 0x10 */
    x86BiosCall(0x10, &Registers);

    // FIXME: check result

    /* Restore previous flags */
    __writeeflags(OldEflags);
    return TRUE;
#else
    /* This x64 HAL does NOT currently handle display reset (TODO) */
    return FALSE;
#endif
}
