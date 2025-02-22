/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS DNS Shared Library
 * FILE:        lib/dnslib/memory.c
 * PURPOSE:     DNS Memory Manager Implementation and Heap.
 */

/* INCLUDES ******************************************************************/
#include "precomp.h"

/* DATA **********************************************************************/

typedef PVOID
(WINAPI *PDNS_ALLOC_FUNCTION)(IN SIZE_T Size);
typedef VOID
(WINAPI *PDNS_FREE_FUNCTION)(IN PVOID Buffer);

PDNS_ALLOC_FUNCTION pDnsAllocFunction;
PDNS_FREE_FUNCTION pDnsFreeFunction;

/* FUNCTIONS *****************************************************************/

VOID
WINAPI
Dns_Free(IN PVOID Address)
{
    /* Check if whoever imported us specified a special free function */
    if (pDnsFreeFunction)
    {
        /* Use it */
        pDnsFreeFunction(Address);
    }
    else
    {
        /* Use our own */
        LocalFree(Address);
    }
}

PVOID
WINAPI
Dns_AllocZero(IN SIZE_T Size)
{
    PVOID Buffer;

    /* Check if whoever imported us specified a special allocation function */
    if (pDnsAllocFunction)
    {
        /* Use it to allocate the memory */
        Buffer = pDnsAllocFunction(Size);
        if (Buffer)
        {
            /* Zero it out */
            RtlZeroMemory(Buffer, Size);
        }
    }
    else
    {
        /* Use our default */
        Buffer = LocalAlloc(LMEM_ZEROINIT, Size);
    }

    /* Return the allocate pointer */
    return Buffer;
}

