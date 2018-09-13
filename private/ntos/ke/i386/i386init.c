/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    i386init.c

Abstract:

    This module contains code to manipulate i386 hardware structures used
    only by the kernel.

Author:

    Bryan Willman  22 Feb 90

Revision History:

--*/

#include    "ki.h"

VOID
KiInitializeMachineType (
    VOID
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,KiInitializeGDT)
#pragma alloc_text(INIT,KiInitializeGdtEntry)
#pragma alloc_text(INIT,KiInitializeMachineType)
#endif


KIRQL   KiProfileIrql = PROFILE_LEVEL;
ULONG   KeI386MachineType = 0;
BOOLEAN KeI386NpxPresent;
BOOLEAN KeI386FxsrPresent;
ULONG   KeI386ForceNpxEmulation;
ULONG   KeI386CpuType;
ULONG   KeI386CpuStep;
PVOID   Ki387RoundModeTable;    // R3 emulators RoundingMode vector table
ULONG   KiBootFeatureBits;

ULONG KiInBiosCall = FALSE;
ULONG FlagState = 0;                    // bios calls shouldn't automatically turn interrupts back on.

KTRAP_FRAME KiBiosFrame;

#if DBG
UCHAR   MsgDpcTrashedEsp[] = "\n*** DPC routine %lx trashed ESP\n";
UCHAR   MsgDpcTimeout[]    = "\n*** DPC routine > 1 sec --- This is not a break in KeUpdateSystemTime\n";
UCHAR   MsgISRTimeout[]    = "\n*** ISR at %lx took over .5 second\n";
UCHAR   MsgISROverflow[]   = "\n*** ISR at %lx - %d interrupts per .5 second\n";

ULONG   KiDPCTimeout       = 110;
ULONG   KiISRTimeout       = 55;
ULONG   KiISROverflow      = 5500;
ULONG   KiSpinlockTimeout  = 55;
#endif



VOID
KiInitializeGDT (
    IN OUT PKGDTENTRY Gdt,
    IN USHORT GdtLimit,
    IN PKPCR Pcr,
    IN USHORT PcrLimit,
    IN PKTSS Tss,
    IN USHORT TssLimit,
    IN USHORT TebLimit
    )

/*++

Routine Description:

    This procedure initializes a GDT.  It will set standard values
    for all descriptors.

    It will not set PCR->GDT.

    It will set the PCR address in KGDT_PCR.

    KGDT_R3_TEB will be set to a base of 0, with a limit of 1 page.


Arguments:

    Gdt - Supplies a pointer to an array of KGDTENTRYs.

    GdtLimit - Supplies size (in bytes) of Gdt.  Used to detect a
            GDT which is too small (which will cause a BUGCHECK)

    Pcr - FLAT address of Pcr for processor Gdt is for.

    PcrLimit - Size Limit of PCR in bytes.

    Tss - FLAT adderss of TSS for processor Gdt is for.

    TssLimit - Size limit of TSS in bytes.

    TebLimit - Size limit of Teb in bytes.

Return Value:

    None.

--*/

{

    if ((KGDT_NUMBER * 8) > GdtLimit)
        KeBugCheck(MEMORY_MANAGEMENT);

    KiInitializeGdtEntry(&Gdt[KGDT_NULL], 0, 0, 0, 0, GRAN_PAGE);

    KiInitializeGdtEntry(
        &Gdt[KGDT_R0_CODE], 0, (ULONG)-1, TYPE_CODE, DPL_SYSTEM, GRAN_PAGE);

    KiInitializeGdtEntry(
        &Gdt[KGDT_R0_DATA], 0, (ULONG)-1, TYPE_DATA, DPL_SYSTEM, GRAN_PAGE);

    KiInitializeGdtEntry(&Gdt[KGDT_R3_CODE], 0,
        (ULONG)-1, TYPE_CODE, DPL_USER, GRAN_PAGE);

    KiInitializeGdtEntry(&Gdt[KGDT_R3_DATA], 0,
        (ULONG)-1, TYPE_DATA, DPL_USER, GRAN_PAGE);

    KiInitializeGdtEntry(
        &Gdt[KGDT_TSS], (ULONG)Tss, TssLimit-1,
        TYPE_TSS, DPL_SYSTEM, GRAN_BYTE);

    KiInitializeGdtEntry(
        &Gdt[KGDT_R0_PCR], (ULONG)Pcr, PcrLimit-1,
        TYPE_DATA, DPL_SYSTEM, GRAN_BYTE);

    KiInitializeGdtEntry(
        &Gdt[KGDT_R3_TEB], 0, TebLimit-1, TYPE_DATA, DPL_USER, GRAN_BYTE);
}

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
KiInitializeMachineType (
    VOID
    )

/*++

Routine Description:

    This function initializes machine type, i.e. MCA, ABIOS, ISA
    or EISA.
    N.B.  This is a temporary routine.  machine type:
          Byte 0 - Machine Type, ISA, EISA or MCA
          Byte 1 - CPU type, i386 or i486
          Byte 2 - Cpu Step, A or B ... etc.
          Highest bit indicates if NPX is present.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KeI386MachineType = KeLoaderBlock->u.I386.MachineType & 0x000ff;
}
