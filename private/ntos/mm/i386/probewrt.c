/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    probewrt.c

Abstract:

    This module contains the routine to support probe for write on
    the Intel 386.  The Intel 386 has the unique property that in
    kernel mode the writable bit of the PTE is ignored.  This allows
    the kernel to write user mode pages which may be read only.  Note,
    that copy-on-write pages are protected as read-only as well, hence
    the kernel could write to a user-mode copy on write page and the
    copy on write would not occur.


Author:

    Lou Perazzoli (loup) 6-Apr-1990

Environment:

    Kernel mode only.  Non-paged.

Revision History:

--*/

#include "mi.h"


VOID
MmProbeForWrite (
    IN PVOID Address,
    IN ULONG Length
    )

/*++

Routine Description:

    This function probes an address for write accessibility on
    the Intel 386.

Arguments:

    Address - Supplies a pointer to the structure to probe.

    Length - Supplies the length of the structure.

Return Value:

    None.  If the Address cannot be written an exception is raised.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMPTE PteContents;
    CCHAR Temp;

    //
    // Loop on the copy on write case until the page is only
    // writable.
    //

    if (Address >= (PVOID)MM_HIGHEST_USER_ADDRESS) {
        ExRaiseStatus(STATUS_ACCESS_VIOLATION);
    }

    PointerPte = MiGetPteAddress (Address);
    LastPte = MiGetPteAddress ((PVOID)((ULONG)Address + Length - 1));

    while (PointerPte <= LastPte) {

        for (;;) {

            //
            // Touch the address as a byte to check for readability and
            // get the PTE built.
            //

            do {
                Temp = *(volatile CCHAR *)Address;
                PteContents = *(volatile MMPTE *)PointerPte;
            } while (PteContents.u.Hard.Valid == 0);

            if (PteContents.u.Hard.Write == 1) {

                //
                // The PTE is writable and not copy on write.
                //

                break;
            }

            if (PteContents.u.Hard.CopyOnWrite == 1) {

                //
                // The PTE is copy on write.  Call the pager and let
                // it deal with this.  Once the page fault is complete,
                // this loop will again be repeated and the PTE will
                // again be checked for write access and copy-on-write
                // access.  The PTE could still be copy-on-write even
                // after the pager is called if the page table page
                // was removed from the working set at this time (unlikely,
                // but still possible).
                //

                if (!NT_SUCCESS (MmAccessFault (TRUE,
                                             Address,
                                             UserMode))) {

                    //
                    // Raise an access violation status.
                    //

                    ExRaiseStatus(STATUS_ACCESS_VIOLATION);

                }
            } else {

                //
                // Raise an access violation status.
                //

                ExRaiseStatus(STATUS_ACCESS_VIOLATION);

            }
        }
        PointerPte += 1;
        Address = (PVOID)((ULONG)Address + PAGE_SIZE);
    }
}
