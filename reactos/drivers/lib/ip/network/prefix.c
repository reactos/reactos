/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/ip.c
 * PURPOSE:     Internet Protocol module
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Art Yerkes (arty@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"


LIST_ENTRY PrefixListHead;
KSPIN_LOCK PrefixListLock;

/* --------- The Prefix List ---------- */

VOID InitPLE() {
    /* Initialize the prefix list and protecting lock */
    InitializeListHead(&PrefixListHead);
    TcpipInitializeSpinLock(&PrefixListLock);
}


PPREFIX_LIST_ENTRY CreatePLE(PIP_INTERFACE IF, PIP_ADDRESS Prefix, UINT Length)
/*
 * FUNCTION: Creates a prefix list entry and binds it to an interface
 * ARGUMENTS:
 *     IF     = Pointer to interface
 *     Prefix = Pointer to prefix
 *     Length = Length of prefix
 * RETURNS:
 *     Pointer to PLE, NULL if there was not enough free resources
 * NOTES:
 *     The prefix list entry retains a reference to the interface and
 *     the provided address.  The caller is responsible for providing
 *     these references
 */
{
    PPREFIX_LIST_ENTRY PLE;

    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X)  Prefix (0x%X)  Length (%d).\n", IF, Prefix, Length));

    TI_DbgPrint(DEBUG_IP, ("Prefix (%s).\n", A2S(Prefix)));

    /* Allocate space for an PLE and set it up */
    PLE = PoolAllocateBuffer(sizeof(PREFIX_LIST_ENTRY));
    if (!PLE) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NULL;
    }

    INIT_TAG(PLE, TAG('P','L','E',' '));
    PLE->Interface    = IF;
    PLE->Prefix       = Prefix;
    PLE->PrefixLength = Length;

    /* Add PLE to the global prefix list */
    ExInterlockedInsertTailList(&PrefixListHead, &PLE->ListEntry, &PrefixListLock);

    return PLE;
}


VOID DestroyPLE(
    PPREFIX_LIST_ENTRY PLE)
/*
 * FUNCTION: Destroys an prefix list entry
 * ARGUMENTS:
 *     PLE = Pointer to prefix list entry
 * NOTES:
 *     The prefix list lock must be held when called
 */
{
    TI_DbgPrint(DEBUG_IP, ("Called. PLE (0x%X).\n", PLE));

    TI_DbgPrint(DEBUG_IP, ("PLE (%s).\n", PLE->Prefix));

    /* Unlink the prefix list entry from the list */
    RemoveEntryList(&PLE->ListEntry);

    /* And free the PLE */
    PoolFreeBuffer(PLE);
}


VOID DestroyPLEs(
    VOID)
/*
 * FUNCTION: Destroys all prefix list entries
 */
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PPREFIX_LIST_ENTRY Current;

    TI_DbgPrint(DEBUG_IP, ("Called.\n"));

    TcpipAcquireSpinLock(&PrefixListLock, &OldIrql);

    /* Search the list and remove every PLE we find */
    CurrentEntry = PrefixListHead.Flink;
    while (CurrentEntry != &PrefixListHead) {
        NextEntry = CurrentEntry->Flink;
	Current = CONTAINING_RECORD(CurrentEntry, PREFIX_LIST_ENTRY, ListEntry);
        /* Destroy the PLE */
        DestroyPLE(Current);
        CurrentEntry = NextEntry;
    }
    TcpipReleaseSpinLock(&PrefixListLock, OldIrql);
}

