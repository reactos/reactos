/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    iopm.c

Abstract:

    This module implements interfaces that support manipulation of i386
    i/o access maps (IOPMs).

    These entry points only exist on i386 and IA64 machines.

Author:

    Bryan M. Willman (bryanwi) 18-Sep-91

Environment:

    Kernel mode only.

Revision History:

    Charles Spirakis (intel): 25 Jul 1996 - Don't have TSS anymore,
        so don't need TSS handling or any synchronization of the TSS.

--*/

#include "ki.h"

BOOLEAN KiIA32SetPortMemory(PKPROCESS Process);

//
// For IA64, only need to worry about synchronization for SetIoAccessMap
// since there is no longer a TSS and no worry about partial TSS states
// (and no need for any IPI's to do updates...)
//

//
// Holds all of the possible IO maps (currently 1)
//
static PKIO_ACCESS_MAP VdmIoMaps[IOPM_COUNT];


BOOLEAN
KeIA32SetIoAccessMap (
    ULONG MapNumber,
    PKIO_ACCESS_MAP IoAccessMap
    )

/*++

Routine Description:

    The specified i/o access map will be set to match the
    definition specified by IoAccessMap (i.e. enable/disable
    those ports) before the call returns.  The change will take
    effect on all processors.

    KeIA32SetIoAccessMap does not give any process enhanced I/O
    access, it merely defines a particular access map.

Arguments:

    MapNumber - Number of access map to set.  Map 0 is fixed.

    IoAccessMap - Pointer to bitvector (64K bits, 8K bytes) which
           defines the specified access map.  Must be in
           non-paged pool.

Return Value:

    TRUE if successful.  FALSE if failure (attempt to set a map
    which does not exist, attempt to set map 0)

--*/

{

    PKPROCESS CurrentProcess;
    KIRQL OldIrql;
    PVOID pt;
    PUCHAR p;
    ULONG i;
    BOOLEAN res;

    //
    // Reject illegal requests
    //

    if ((MapNumber > IOPM_COUNT) || (MapNumber == IO_ACCESS_MAP_NONE)) {
        return FALSE;
    }

    //
    // If the map N hasn't been allocated, then allocate it now.
    // Don't need to ever free this since it is a system wide
    // resource... Once allocated, stays till reboot.
    // This implies we might want to allocate it early, but we can
    // deal with that later...
    //

    if (VdmIoMaps[MapNumber - 1] == NULL) {
        // Allocate a little extra to handle wrap-around...
        p = (PUCHAR) ExAllocatePool(NonPagedPool, PIOPM_SIZE);
        if (p == NULL) {
            return FALSE;
        }

        //
        // And set it all to "no-access"
		// (and don't forget the little extra)
        //
		RtlFillMemory((PVOID) p, PIOPM_SIZE, 0x0ff);

        VdmIoMaps[MapNumber - 1] = p;
    }

    //
    // Acquire the context swap lock so a context switch will not occur.
    //
    // BUGBUG: Is this really needed?
    //

    KiLockContextSwap(&OldIrql);

    //
    // Copy the IOPM map and load the map for the current process.
    //

    pt = VdmIoMaps[MapNumber-1];
    RtlMoveMemory(pt, (PVOID)IoAccessMap, IOPM_SIZE);
    CurrentProcess = PsGetCurrentProcess();

    CurrentProcess->IOCurMap = MapNumber;

	//
	// BUGBUG: Need to set the EFLAGS.iopl to 3 for this process
	// and all threads in the process as well?
	//

    res = KiIA32SetPortMemory(CurrentProcess);

    //
    // Restore IRQL and unlock the context swap lock.
    //

    KiUnlockContextSwap(OldIrql);
    return res;
}


BOOLEAN
KeIA32QueryIoAccessMap (
    ULONG MapNumber,
    PKIO_ACCESS_MAP IoAccessMap
    )

/*++

Routine Description:

    The specified i/o access map will be dumped into the buffer.
    map 0 is a constant, but will be dumped anyway.

Arguments:

    MapNumber - Number of access map to set.  map 0 is fixed.

    IoAccessMap - Pointer to buffer (64K bits, 8K bytes) which
           is to receive the definition of the access map.
           Must be in non-paged pool.

Return Value:

    TRUE if successful.  FALSE if failure (attempt to query a map
    which does not exist)

--*/

{
    PVOID Map;
    KIRQL OldIrql;

    //
    // Reject illegal requests
    //

    if (MapNumber > IOPM_COUNT) {
        return FALSE;
    }

    //
    // BUGBUG: Do we care if a context switch occurs? What if someone
    // else is modifying map N? (just an issue with MP systems...)
    //

    //
    // Copy out the map
    //

    if (MapNumber == IO_ACCESS_MAP_NONE) {

        //
        // no access case, simply return a map of all 1s
        //
		RtlFillMemory((PVOID) IoAccessMap, IOPM_SIZE, 0x0ff);

    } else {

        //
        // normal case, just copy the bits
        //

        Map = VdmIoMaps[MapNumber-1];
        ASSERT(Map != NULL);
        RtlMoveMemory((PVOID)IoAccessMap, Map, IOPM_SIZE);
    }

    return TRUE;
}


BOOLEAN
KeIA32IoSetAccessProcess (
    PKPROCESS Process,
    ULONG MapNumber
    )
/*++

Routine Description:

    Set the i/o access map which controls user mode i/o access
    for a particular process.

Arguments:

    Process - Pointer to kernel process object describing the
    process which for which a map is to be set.

    MapNumber - Number of the map to set.  Value of map is
    defined by KeIA32IoSetAccessProcess.  Setting MapNumber
    to IO_ACCESS_MAP_NONE will disallow any user mode i/o
    access from the process.

Return Value:

    TRUE if success, FALSE if failure (illegal MapNumber)

--*/

{
    //
    // Reject illegal requests
    //

    if (MapNumber > IOPM_COUNT) {
        return FALSE;
    }

    //
    // Store new offset in process object,  compute current set of
    // active processors for process, if this cpu is one, set IOPM.
    //

    Process->IOCurMap = MapNumber;
    if (Process->IOCurMap == IO_ACCESS_MAP_NONE) {
	//
	// BUGBUG: Need to set the EFLAGS.iopl back to 0 for this process
	// And all its threads?
	//
	}
	else {
	//
	// BUGBUG: Need to set the EFLAGS.iopl to 3 for this process
	// And all its threads?
	//
	}

    return (KiIA32SetPortMemory(Process));
}


BOOLEAN
KeIA32PortIsAccessable(
    PKPROCESS Process,
    ULONG Port,
    ULONG Size
    )
/*++

Routine Description:

    Checks to see if a particular port is accessable by a particular process

Arguments:

    Process - Pointer to kernel process object

    Port - port number to check

    Size - size of access (1, 2 or 4 bytes)

Return Value:

    TRUE if port is accessable, false if port is not

--*/
{
    ULONG byte;
    ULONG bit;
    ULONG val;
    ULONG ToTest;
    PUCHAR ioBits;

    if (Process->IOCurMap == IO_ACCESS_MAP_NONE) {
        return FALSE;
    }

    //
    // Make sure everything is legit...
    //
    ASSERT (Process->IOCurMap <= IOPM_COUNT);
    ASSERT (Port < (64 * 1024));
    ASSERT((Size == 1) || (Size == 2) || (Size == 4));

    ioBits = VdmIoMaps[Process->IOCurMap - 1];

    //
    // Get the byte to be tested
    //
    byte = Port >> 3;

    //
    // This handles boundary conditions...
    //
    ToTest = ioBits[byte] | (ioBits[byte + 1] << 8);

    //
    // And get the specific bit
    //
    bit = Port & 0x07;

    //
    // Really want Size to be 1, 2, or 3 so we can use it for shifting and
    // we want val to be a mask for the port access
    // (2^Size -1 == mask for size of port access)...
    //
    if (Size == 4) {
        val = 7 << bit;
    }
    else {
        val = ((1 << Size) - 1) << bit;
    }

    return (! (ToTest & val));
}


BOOLEAN
KiIA32SetPortMemory(
    PKPROCESS Process
    )
/*++

Routine Description:

    Based on the IO access allowed for a particular process, set the memory
    map accordingly...

Arguments:

    Process - Pointer to kernel process object

Return Value:

    TRUE if able to setup the memory pages

Notes:
	It would be a lot faster to use a bit searching algorithm
	looking for the first 0 bit (accessible) instead of 
	doing 8192 calls to the memory system... Of course, we may
	do this infrequently enough that it may not matter...

--*/
{
    PUCHAR ioBits;
    ULONG i;
    PVOID VirtualPort;

    if (Process->IOCurMap == IO_ACCESS_MAP_NONE) {
        // Unmap all of the IOBASE memory?
        // Or set them all as Inaccesable?

        return TRUE;
    }

    ioBits = VdmIoMaps[Process->IOCurMap - 1];

    for (i = 0; i < IOPM_SIZE; i++) {
        // VirtualPort = (i << SHIFT_8K) + IOBASE;
        if ((*ioBits & 0x000f) == 0) {
            // These four ports can be set read/writable
            // Call memory manager to set read/writable + IO special
        }
        else {
            // Call memory manager to set as inaccessable/unmap
        }

        // VirtualPort = (i << SHIFT_8K) + IOBASE + (4K_PAGE);
        if ((*ioBits++ & 0x00f0) == 0) {
            // These four ports can be set read/writable
            // Call memory manager to set read/writable + IO special
        }
        else {
            // Call memory manager to set as inaccessable/unmap
        }
    }

    return TRUE;
}
