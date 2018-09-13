/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ldtsup.c

Abstract:

    This module implements interfaces that support manipulation of i386 Ldts.
    These entry points only exist on i386 and EM machines.

Author:

    Bryan M. Willman (bryanwi) 14-May-1991

Environment:

    Kernel mode only.

Revision History:
    Charles Spirakis (intel) 5 Jan 1996 - Make sure when the LDT is set,
    that the address pointed to is part of the current process. This is
    needed for MIXISA support.
--*/

#include "ki.h"

//
// Low level assembler support procedures
//


VOID
KeIA32SetLdtProcess (
    PKPROCESS   Process,
    PLDT_ENTRY  Ldt,
    ULONG       Limit
    )
/*++

Routine Description:

    The specified LDT (which may be null) will be made the active Ldt of
    the specified process, for all threads thereof, on whichever
    processors they are running.  The change will take effect before the
    call returns.

    An Ldt address of NULL or a Limit of 0 will cause the process to
    receive the NULL Ldt.

    This function only exists on i386 and i386 compatible processors.

    No checking is done on the validity of Ldt entries.


Arguments:

    Process - Pointer to KPROCESS object describing the process for
        which the Ldt is to be set.

    Ldt - Pointer to an array of LDT_ENTRYs (that is, a pointer to an
        Ldt.)

    Limit - Ldt limit (must be 0 mod 8)

Return Value:

    None.

--*/

{
#if 0
    KGDTENTRY LdtDescriptor;
    KXDESCRIPTOR XDescriptor;
    PTEB Teb;


    //
    // Compute the contents of the Ldt descriptor
    //

    if ((Ldt == NULL) || (Limit == 0)) {

        //
        //  Set up an empty descriptor
        //

        LdtDescriptor.LimitLow = 0;
        LdtDescriptor.BaseLow = 0;
        LdtDescriptor.HighWord.Bytes.BaseMid = 0;
        LdtDescriptor.HighWord.Bytes.Flags1 = 0;
        LdtDescriptor.HighWord.Bytes.Flags2 = 0;
        LdtDescriptor.HighWord.Bytes.BaseHi = 0;

        XDescriptor.Words.DescriptorWords = (ULONGLONG) 0;

    } else {

        //
        // Insure that the unfilled fields of the selector are zero
        // N.B.  If this is not done, random values appear in the high
        //       portion of the Ldt limit.
        //

        LdtDescriptor.HighWord.Bytes.Flags1 = 0;
        LdtDescriptor.HighWord.Bytes.Flags2 = 0;

        //
        //  Set the limit and base
        //

        LdtDescriptor.LimitLow = (USHORT) ((ULONG) Limit - 1);
        LdtDescriptor.BaseLow = (USHORT)  ((ULONG_PTR) Ldt & (ULONG_PTR)0xffff);
        LdtDescriptor.HighWord.Bytes.BaseMid = (UCHAR) (((ULONG_PTR)Ldt & (ULONG_PTR)0xff0000) >> 16);
        LdtDescriptor.HighWord.Bytes.BaseHi =  (UCHAR) (((ULONG_PTR)Ldt & (ULONG_PTR)0xff000000) >> 24);

        //
        //  Type is LDT, DPL = 0
        //

        LdtDescriptor.HighWord.Bits.Type = TYPE_LDT;
        LdtDescriptor.HighWord.Bits.Dpl = DPL_SYSTEM;

        //
        // Make it present
        //

        LdtDescriptor.HighWord.Bits.Pres = 1;

        //
        // Handcraft an unscrambled format that matches the above information
        //
        XDescriptor.Words.DescriptorWords = (ULONGLONG) 0;
        XDescriptor.Words.Bits.Base = (ULONG_PTR) Ldt;
        XDescriptor.Words.Bits.Limit = Limit - 1;
        XDescriptor.Words.Bits.Type = TYPE_LDT;
        XDescriptor.Words.Bits.Dpl = DPL_USER;
        XDescriptor.Words.Bits.Pres = 1;

    }

    //
    // Set the Ldt fields (scrambled and unscrambled) in the process object
    //

    Process->LdtDescriptor = LdtDescriptor;
    Process->UnscrambledLdtDescriptor = XDescriptor.Words.DescriptorWords;

    //
    // If it isn't for the current process, we have a problem
    // as we need to update the thread's copy of the GDT. For the IA64
    // version of VDM support, we can make sure that only the current
    // process touches it's own LDT (and that the first thread of the
    // process creates the LDT), but it would be safer to write
    // the general purpose case...
    //

    ASSERT (Process == &(PsGetCurrentProcess()->Pcb));

    //
    // Put LdtDescriptor into Gdt table of current thread
    // (a better algorithm would be to put the LDT descriptor into
    // all of the GDT's for the passed in PROCESS, but for IA64,
    // that is overkill (unless it's easy to do))...
    //

    Teb = PsGetCurrentThread()->Tcb.Teb;
    *(KGDTENTRY *)(&Teb->Gdt[KGDT_LDT>3]) = LdtDescriptor;

    //
    // And update the LDTR field used by ISA transition stub routines
    //
    Teb->LdtDescriptor = XDescriptor.Words.DescriptorWords;

    return;
#endif
}
