/*++


Module Name:

    ia32init.c

Abstract:

    This module contains code to support iA32 resources structures 

Author:


Revision History:

--*/

#include    "ki.h"

#if defined(WX86)

VOID
KiInitializeGdtEntry (
    OUT PKGDTENTRY GdtEntry,
    IN ULONG Base,
    IN ULONG Limit,
    IN USHORT Type,
    IN USHORT Dpl,
    IN USHORT Granularity
    )

/*++

Routine Description:

    This function initializes a GDT entry.  Base, Limit, Type (code,
    data), and Dpl (0 or 3) are set according to parameters.  All other
    fields of the entry are set to match standard system values.

Arguments:

    GdtEntry - GDT descriptor to be filled in.

    Base - Linear address of the first byte mapped by the selector.

    Limit - Size of the selector in pages.  Note that 0 is 1 page
            while 0xffffff is 1 megapage = 4 gigabytes.

    Type - Code or Data.  All code selectors are marked readable,
            all data selectors are marked writeable.

    Dpl - User (3) or System (0)

    Granularity - 0 for byte, 1 for page

Return Value:

    Pointer to the GDT entry.

--*/

{
    GdtEntry->LimitLow = (USHORT)(Limit & 0xffff);
    GdtEntry->BaseLow = (USHORT)(Base & 0xffff);
    GdtEntry->HighWord.Bytes.BaseMid = (UCHAR)((Base & 0xff0000) >> 16);
    GdtEntry->HighWord.Bits.Type = Type;
    GdtEntry->HighWord.Bits.Dpl = Dpl;
    GdtEntry->HighWord.Bits.Pres = 1;
    GdtEntry->HighWord.Bits.LimitHi = (Limit & 0xf0000) >> 16;
    GdtEntry->HighWord.Bits.Sys = 0;
    GdtEntry->HighWord.Bits.Reserved_0 = 0;
    GdtEntry->HighWord.Bits.Default_Big = 1;
    GdtEntry->HighWord.Bits.Granularity = Granularity;
    GdtEntry->HighWord.Bytes.BaseHi = (UCHAR)((Base & 0xff000000) >> 24);
}


VOID
KiInitializeXDescriptor (
    OUT PKXDESCRIPTOR Descriptor,
    IN ULONG Base,
    IN ULONG Limit,
    IN USHORT Type,
    IN USHORT Dpl,
    IN USHORT Granularity
    )

/*++

Routine Description:

    This function initializes a unscrambled Descriptor.  
    Base, Limit, Type (code, data), and Dpl (0 or 3) are set according 
    to parameters.  All other fields of the entry are set to match 
    standard system values.
    The Descriptor will be initialized to 0 first, and then setup as requested

Arguments:

    Descriptor - descriptor to be filled in.

    Base - Linear address of the first byte mapped by the selector.

    Limit - Size of the selector in pages.  Note that 0 is 1 page
            while 0xffffff is 1 megapage = 4 gigabytes.

    Type - Code or Data.  All code selectors are marked readable,
            all data selectors are marked writeable.

    Dpl - User (3) or System (0)

    Granularity - 0 for byte, 1 for page

Return Value:

    Pointer to the Descriptor

--*/

{
    Descriptor->Words.DescriptorWords = 0;  

    Descriptor->Words.Bits.Base = Base;
    Descriptor->Words.Bits.Limit = Limit;
    Descriptor->Words.Bits.Type = Type;
    Descriptor->Words.Bits.Dpl = Dpl;
    Descriptor->Words.Bits.Pres = 1;
    Descriptor->Words.Bits.Default_Big = 1;
    Descriptor->Words.Bits.Granularity = Granularity;
}

VOID
KeiA32ResourceCreate (
    IN PEPROCESS Process,
    IN PTEB Teb
    )
/*++

Routine Description:

    To initial Gdt Table for the target user thread
    Only called from KiInitializeContextThread()

Arguments:

    Thread - Pointer to KTHREAD object 

Return value:
    
    None

--*/
{
    ULONG i;
    PUCHAR Gdt;


    ASSERT (sizeof(KTRAP_FRAME) == sizeof(KIA32_FRAME));

    //
    // Initialize each required Gdt entry
    //

    Gdt = (PUCHAR) Teb->Gdt;

    RtlZeroMemory(Gdt, GDT_TABLE_SIZE);

    KiInitializeGdtEntry((PKGDTENTRY)(Gdt + KGDT_R3_CODE), 0,
        (ULONG)-1, TYPE_CODE_USER, DPL_USER, GRAN_PAGE);

    KiInitializeGdtEntry((PKGDTENTRY)(Gdt + KGDT_R3_DATA), 0,
        (ULONG)-1, TYPE_DATA_USER, DPL_USER, GRAN_PAGE);

    //
    // Set user TEB descriptor
    //

    KiInitializeGdtEntry((PKGDTENTRY)(Gdt + KGDT_R3_TEB), Teb,
        sizeof(TEB)-1, TYPE_DATA_USER, DPL_USER, GRAN_BYTE);

    //
    // The FS descriptor for ISA transitions. This needs to be in
    // unscrambled format
    //

    KiInitializeXDescriptor((PKXDESCRIPTOR)&(Teb->FsDescriptor), Teb,
        sizeof(TEB)-1, TYPE_DATA_USER, DPL_USER, GRAN_BYTE);

    // Set BIOS descriptor

    //
    // Set LDT descriptor, just copy whatever the Proecss has
    //
    *(PKGDTENTRY)(Gdt+KGDT_LDT) = (Process->Pcb).LdtDescriptor;

    // Set Video display buffer descriptor

    //
    // Setup ISA transition GdtDescriptor - needs to be unscrambled
    // But according tot he seamless EAS, only the base and limit
    // are actually used...
    //

    KiInitializeXDescriptor((PKXDESCRIPTOR)&(Teb->GdtDescriptor), (ULONG)Gdt,
        GDT_TABLE_SIZE-1, TYPE_DATA_USER, DPL_USER, GRAN_BYTE);
    //
    // Setup ISA transition LdtDescriptor - needs to be unscrambled
    //

    Teb->LdtDescriptor = (ULONGLONG)((Process->Pcb).UnscrambledLdtDescriptor);
}

#endif  // defined(WX86)
