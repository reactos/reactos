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
        // AGENT-MODIFIED: Make HAL init non-fatal for UEFI systems
        /* Allocate one page for a fallback mapping */
        PhysicalAddress = HalpAllocPhysicalMemory(LoaderBlock,
                                                  0x100000,
                                                  1,
                                                  FALSE);
        if (PhysicalAddress == 0)
        {
            /* Allocation failed - x86 BIOS services will not be available */
            DPRINT1("HalInitializeBios: Failed to allocate fallback page\n");
            x86BiosIsInitialized = FALSE;
            return;
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
            /* Allocation failed - x86 BIOS services will not be available */
            DPRINT1("HalInitializeBios: Failed to allocate buffer page\n");
            x86BiosIsInitialized = FALSE;
            return;
        }
    }
    else
    {

        // AGENT-MODIFIED: Make HAL init non-fatal for UEFI systems
        /* Allocate an MDL for 1MB */
        Mdl = IoAllocateMdl(NULL, 0x100000, FALSE, FALSE, NULL);
        if (!Mdl)
        {
            /* MDL allocation failed - x86 BIOS services will not be available */
            DPRINT1("HalInitializeBios: Failed to allocate MDL\n");
            x86BiosIsInitialized = FALSE;
            return;
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
        // AGENT-MODIFIED: Make HAL init non-fatal for UEFI systems
        if (!x86BiosMemoryMapping)
        {
            /* MDL mapping failed - x86 BIOS services will not be available */
            DPRINT1("HalInitializeBios: Failed to map MDL to system space\n");
            IoFreeMdl(Mdl);
            x86BiosIsInitialized = FALSE;
            return;
        }

        DPRINT1("*x86BiosMemoryMapping: %p, %p\n",
                *(PVOID*)x86BiosMemoryMapping, *(PVOID*)(x86BiosMemoryMapping + 8));
        //DbgDumpPage(x86BiosMemoryMapping, 0xc351);

        x86BiosIsInitialized = TRUE;
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
    // AGENT-MODIFIED: Return STATUS_NOT_SUPPORTED on UEFI systems where x86bios is not initialized
    if (!x86BiosIsInitialized)
    {
        /* x86 BIOS services not available (UEFI system) */
        return STATUS_NOT_SUPPORTED;
    }
    
    if (*Size > PAGE_SIZE)
    {
        /* Buffer too large */
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
    // AGENT-MODIFIED: Return STATUS_NOT_SUPPORTED on UEFI systems where x86bios is not initialized
    if (!x86BiosIsInitialized)
    {
        /* x86 BIOS services not available (UEFI system) */
        return STATUS_NOT_SUPPORTED;
    }
    
    // AGENT-MODIFIED: Accept the segment returned by x86BiosAllocateBuffer (x86BiosBufferPhysical / 16)
    /* Check if the address matches the allocated buffer */
    if ((Segment != (x86BiosBufferPhysical / 16)) || (Offset != 0))
    {
        /* Invalid segment/offset */
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
    // AGENT-MODIFIED: Return STATUS_NOT_SUPPORTED on UEFI systems where x86bios is not initialized
    if (!x86BiosIsInitialized)
    {
        /* x86 BIOS services not available (UEFI system) */
        return STATUS_NOT_SUPPORTED;
    }
    
    if ((Address + Size) > 0x100000)
    {
        /* Invalid address range */
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
    // AGENT-MODIFIED: Return STATUS_NOT_SUPPORTED on UEFI systems where x86bios is not initialized
    if (!x86BiosIsInitialized)
    {
        /* x86 BIOS services not available (UEFI system) */
        return STATUS_NOT_SUPPORTED;
    }
    
    if ((Address + Size) > 0x100000)
    {
        /* Invalid address range */
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
        DPRINT1("x86MemRead: invalid read at 0x%lx (size 0x%lx)\n", Address, Size);
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
        DPRINT1("x86MemWrite: invalid write at 0x%lx (size 0x%lx)\n", Address, Size);
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

        // OVMF debug messages used by VBox / QEMU
        // https://www.virtualbox.org/svn/vbox/trunk/src/VBox/Devices/EFI/Firmware/OvmfPkg/README
        case 0x402: return (Size == 1) && IsWrite;

        // BOCHS VBE: https://forum.osdev.org/viewtopic.php?f=1&t=14639
        case 0x1CE: return (Size == 1) && IsWrite;
        case 0x1CF: return (Size == 1);

        // CHECKME!
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
    const ULONG StackBase = 0x2000;
    FAST486_STATE EmulatorContext;
    ULONG FlatIp;
    PUCHAR InstructionPointer;

    // AGENT-MODIFIED: Fail gracefully on UEFI systems where x86bios is not initialized
    if (!x86BiosIsInitialized)
    {
        DPRINT1("x86BiosCall: NOT_SUPPORTED (UEFI)\n");
        return FALSE;
    }

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

    /* Set up the INT stub */
    FlatIp = StackBase - 4;
    InstructionPointer = x86BiosMemoryMapping + FlatIp;
    InstructionPointer[0] = 0xCD; // INT instruction
    InstructionPointer[1] = (UCHAR)InterruptNumber;
    InstructionPointer[2] = 0x90; // NOP. We will stop at this address.

    /* Set the stack pointer */
    Fast486SetStack(&EmulatorContext, 0, StackBase - 8);

    /* Start execution at the INT stub */
    Fast486ExecuteAt(&EmulatorContext, 0x00, FlatIp);

    while (TRUE)
    {
        /* Get the current flat IP */
        FlatIp = (EmulatorContext.SegmentRegs[FAST486_REG_CS].Selector << 4) +
                 EmulatorContext.InstPtr.Long;

        /* Make sure we haven't left the allowed memory range */
        if (FlatIp >= 0x100000)
        {
            DPRINT1("x86BiosCall: invalid IP (0x%lx) during BIOS execution\n", FlatIp);
            return FALSE;
        }

        /* Check if we returned from our int stub */
        if (FlatIp == (StackBase - 2))
        {
            /* We are done! */
            break;
        }

        /* Emulate one instruction */
        Fast486StepInto(&EmulatorContext);
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

#ifdef _M_AMD64
BOOLEAN
NTAPI
HalpBiosDisplayReset(VOID)
{
    // AGENT-MODIFIED: Return FALSE for UEFI systems and when x86bios is not initialized
    if (!x86BiosIsInitialized)
    {
        // AGENT-MODIFIED: Use DPRINT for repeated messages
        DPRINT("HalpBiosDisplayReset: x86bios not initialized (UEFI system?)\n");
        return FALSE;
    }

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
    /* AMD64: Initialize VGA directly for 640x480 16-color mode (mode 0x12) */
    DPRINT1("HalpBiosDisplayReset: AMD64 - Initializing VGA for graphics mode\n");

    /* First, reset the VGA to a known state */

    /* Reset Attribute Controller */
    (VOID)READ_PORT_UCHAR((PUCHAR)0x3DA);  /* Reset flip-flop */
    WRITE_PORT_UCHAR((PUCHAR)0x3C0, 0x00); /* Disable video */

    /* Synchronous reset on */
    WRITE_PORT_UCHAR((PUCHAR)0x3C4, 0x00);
    WRITE_PORT_UCHAR((PUCHAR)0x3C5, 0x01);

    /* Write Miscellaneous Output Register */
    WRITE_PORT_UCHAR((PUCHAR)0x3C2, 0xE3);  /* 640x480, 25MHz clock, -hsync, -vsync */

    /* Sequencer registers */
    WRITE_PORT_UCHAR((PUCHAR)0x3C4, 0x01);
    WRITE_PORT_UCHAR((PUCHAR)0x3C5, 0x01);  /* Clock mode */
    WRITE_PORT_UCHAR((PUCHAR)0x3C4, 0x02);
    WRITE_PORT_UCHAR((PUCHAR)0x3C5, 0x0F);  /* Enable all planes */
    WRITE_PORT_UCHAR((PUCHAR)0x3C4, 0x03);
    WRITE_PORT_UCHAR((PUCHAR)0x3C5, 0x00);  /* Character map */
    WRITE_PORT_UCHAR((PUCHAR)0x3C4, 0x04);
    WRITE_PORT_UCHAR((PUCHAR)0x3C5, 0x06);  /* Chain-4 off, extended memory */

    /* Synchronous reset off */
    WRITE_PORT_UCHAR((PUCHAR)0x3C4, 0x00);
    WRITE_PORT_UCHAR((PUCHAR)0x3C5, 0x03);

    /* Unlock CRTC registers */
    WRITE_PORT_UCHAR((PUCHAR)0x3D4, 0x11);
    WRITE_PORT_UCHAR((PUCHAR)0x3D5, 0x00);

    /* CRTC registers for 640x480 */
    static const UCHAR crtc_regs[] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
        0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3, 0xFF
    };
    ULONG i;
    for (i = 0; i < sizeof(crtc_regs); i++) {
        WRITE_PORT_UCHAR((PUCHAR)0x3D4, (UCHAR)i);
        WRITE_PORT_UCHAR((PUCHAR)0x3D5, crtc_regs[i]);
    }

    /* Graphics Controller registers */
    WRITE_PORT_UCHAR((PUCHAR)0x3CE, 0x00);
    WRITE_PORT_UCHAR((PUCHAR)0x3CF, 0x00);  /* Set/Reset */
    WRITE_PORT_UCHAR((PUCHAR)0x3CE, 0x01);
    WRITE_PORT_UCHAR((PUCHAR)0x3CF, 0x00);  /* Enable Set/Reset */
    WRITE_PORT_UCHAR((PUCHAR)0x3CE, 0x02);
    WRITE_PORT_UCHAR((PUCHAR)0x3CF, 0x00);  /* Color Compare */
    WRITE_PORT_UCHAR((PUCHAR)0x3CE, 0x03);
    WRITE_PORT_UCHAR((PUCHAR)0x3CF, 0x00);  /* Data Rotate */
    WRITE_PORT_UCHAR((PUCHAR)0x3CE, 0x04);
    WRITE_PORT_UCHAR((PUCHAR)0x3CF, 0x00);  /* Read Map Select */
    WRITE_PORT_UCHAR((PUCHAR)0x3CE, 0x05);
    WRITE_PORT_UCHAR((PUCHAR)0x3CF, 0x00);  /* Graphics Mode: Write Mode 0 */
    WRITE_PORT_UCHAR((PUCHAR)0x3CE, 0x06);
    WRITE_PORT_UCHAR((PUCHAR)0x3CF, 0x05);  /* Miscellaneous: A0000-AFFFF, graphics mode */
    WRITE_PORT_UCHAR((PUCHAR)0x3CE, 0x07);
    WRITE_PORT_UCHAR((PUCHAR)0x3CF, 0x0F);  /* Color Don't Care */
    WRITE_PORT_UCHAR((PUCHAR)0x3CE, 0x08);
    WRITE_PORT_UCHAR((PUCHAR)0x3CF, 0xFF);  /* Bit Mask */

    /* Attribute Controller registers */
    static const UCHAR attr_regs[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
    };

    (VOID)READ_PORT_UCHAR((PUCHAR)0x3DA);  /* Reset flip-flop */
    for (i = 0; i < 16; i++) {
        WRITE_PORT_UCHAR((PUCHAR)0x3C0, (UCHAR)i);
        WRITE_PORT_UCHAR((PUCHAR)0x3C0, attr_regs[i]);
    }
    WRITE_PORT_UCHAR((PUCHAR)0x3C0, 0x10);
    WRITE_PORT_UCHAR((PUCHAR)0x3C0, 0x01);  /* Graphics mode */
    WRITE_PORT_UCHAR((PUCHAR)0x3C0, 0x11);
    WRITE_PORT_UCHAR((PUCHAR)0x3C0, 0x00);  /* Overscan color */
    WRITE_PORT_UCHAR((PUCHAR)0x3C0, 0x12);
    WRITE_PORT_UCHAR((PUCHAR)0x3C0, 0x0F);  /* Color plane enable */
    WRITE_PORT_UCHAR((PUCHAR)0x3C0, 0x13);
    WRITE_PORT_UCHAR((PUCHAR)0x3C0, 0x00);  /* Horizontal pixel panning */
    WRITE_PORT_UCHAR((PUCHAR)0x3C0, 0x14);
    WRITE_PORT_UCHAR((PUCHAR)0x3C0, 0x00);  /* Color select */

    /* Enable video display */
    WRITE_PORT_UCHAR((PUCHAR)0x3C0, 0x20);

    /* Set DAC mask */
    WRITE_PORT_UCHAR((PUCHAR)0x3C6, 0xFF);

    /* Clear screen by writing to VGA memory */
    /* On AMD64, we need to map the VGA memory properly */
    PHYSICAL_ADDRESS VgaPhysical;
    PUCHAR VgaBase;
    ULONG j;

    VgaPhysical.QuadPart = 0xA0000;
    VgaBase = (PUCHAR)MmMapIoSpace(VgaPhysical, 0x10000, MmNonCached);

    if (VgaBase) {
        /* Select all planes for writing */
        WRITE_PORT_UCHAR((PUCHAR)0x3C4, 0x02);
        WRITE_PORT_UCHAR((PUCHAR)0x3C5, 0x0F);

        /* Clear video memory */
        for (j = 0; j < 0x10000; j++) {
            WRITE_REGISTER_UCHAR(VgaBase + j, 0x00);
        }

        /* Unmap the memory */
        MmUnmapIoSpace(VgaBase, 0x10000);
    } else {
        DPRINT1("HalpBiosDisplayReset: Failed to map VGA memory\n");
    }

    DPRINT1("HalpBiosDisplayReset: AMD64 VGA graphics mode initialization complete\n");
    return TRUE;
#endif
}
#endif // _M_AMD64
