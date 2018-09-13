/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    gdtsup.c

Abstract:

    This module implements interfaces that support manipulation of i386 Gdts.
    These entry points only exist on i386 machines.

Author:

    Dave Hastings (daveh) 28 May 1991

Environment:

    Kernel mode only.

Revision History:
    Fred Yang (intel) 20 Aug 1996 - based on ../i386/gdtsup.c but stripped
    out the code we don't need. IA64 will never set an entry in the GDT
    at runtime, for example...

--*/

#include "ki.h"

VOID
KeIA32GetGdtEntryThread(
    IN PKTHREAD Thread,
    IN ULONG Offset,
    OUT PKGDTENTRY Descriptor
    )
/*++

Routine Description:

    This routine returns the contents of an entry in the Gdt.  If the
    entry is thread specific, the entry for the specified thread is
    created and returned (KGDT_LDT, and KGDT_R3_TEB).  If the selector
    is processor dependent, the entry for the current processor is
    returned (KGDT_R0_PCR).

Arguments:

    Thread -- Supplies a pointer to the thread to return the entry for.

    Offset -- Supplies the offset in the Gdt.  This value must be 0
        mod 8.

    Descriptor -- Returns the contents of the Gdt descriptor

Return Value:

    None.

--*/

{
#if 0
    PKGDTENTRY Gdt;
    PKPROCESS Process;

    //
    // If the entry is out of range, don't return anything
    //

    if (Offset >= GDT_TABLE_SIZE) {
        return ;
    }

    if (Offset == KGDT_LDT) {

        //
        // Materialize Ldt selector
        //

        Process = Thread->ApcState.Process;

        //
        // Assume same process
        //
        ASSERT (&(PsGetCurrentProcess()->Pcb) == Process);

        *Descriptor = Process->LdtDescriptor;

    } else {

        //
        // Copy Selector from Gdt
        //
        // N.B. We will change the base later, if it is KGDT_R3_TEB
        //

        PTEB Teb = (PTEB)(Thread->Teb);

        *Descriptor = *((PKGDTENTRY)((PCHAR)(Teb->Gdt) + Offset));
       
        //
        // if it is the TEB selector, fix the base
        //

        if (Offset == KGDT_R3_TEB) {
            Descriptor->BaseLow = (USHORT)((ULONG_PTR)(Thread->Teb) & 0xFFFF);
            Descriptor->HighWord.Bytes.BaseMid =
                (UCHAR) ( ( (ULONG_PTR)(Thread->Teb) & ((ULONG_PTR)0xFF0000L)) >> 16);
            Descriptor->HighWord.Bytes.BaseHi =
                (CHAR)  ( ( (ULONG_PTR)(Thread->Teb) & ((ULONG_PTR)0xFF000000L)) >> 24);
        }
    }

    return ;
#endif
}
