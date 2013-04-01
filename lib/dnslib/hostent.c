/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS DNS Shared Library
 * FILE:        lib/dnslib/hostent.c
 * PURPOSE:     Functions for dealing with Host Entry structures
 */

/* INCLUDES ******************************************************************/
#include "precomp.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

PHOSTENT
WINAPI
Hostent_Init(IN PVOID *Buffer,
             IN WORD AddressFamily,
             IN ULONG AddressSize,
             IN ULONG AddressCount,
             IN ULONG AliasCount)
{
    PHOSTENT Hostent;
    ULONG_PTR BufferPosition = (ULONG_PTR)*Buffer;

    /* Align the hostent on the buffer's 4 byte boundary */
    BufferPosition += 3 & ~3;

    /* Set up the basic data */
    Hostent = (PHOSTENT)BufferPosition;
    Hostent->h_length = (WORD)AddressSize;
    Hostent->h_addrtype = AddressFamily;

    /* Put aliases after Hostent */
    Hostent->h_aliases = (PCHAR*)((ULONG_PTR)(Hostent + 1) & ~3);

    /* Zero it out */
    RtlZeroMemory(Hostent->h_aliases, AliasCount * sizeof(PCHAR));

    /* Put addresses after aliases */
    Hostent->h_addr_list = (PCHAR*)
                           ((ULONG_PTR)Hostent->h_aliases +
                            (AliasCount * sizeof(PCHAR)) + sizeof(PCHAR));

    /* Update the location */
    BufferPosition = (ULONG_PTR)Hostent->h_addr_list +
                     ((AddressCount * sizeof(PCHAR)) + sizeof(PCHAR));

    /* Send it back */
    *Buffer = (PVOID)BufferPosition;

    /* Return the hostent */
    return Hostent;
}

VOID
WINAPI
Dns_PtrArrayToOffsetArray(PCHAR *List,
                          ULONG_PTR Base)
{
    /* Loop every pointer in the list */
    do 
    {
        /* Update the pointer */
        *List = (PCHAR)((ULONG_PTR)*List - Base);
    } while(*List++);
}

VOID
WINAPI
Hostent_ConvertToOffsets(IN PHOSTENT Hostent)
{
    /* Do we have a name? */
    if (Hostent->h_name)
    {
        /* Update it */
        Hostent->h_name -= (ULONG_PTR)Hostent;
    }

    /* Do we have aliases? */
    if (Hostent->h_aliases)
    {
        /* Update the pointer */
        Hostent->h_aliases -= (ULONG_PTR)Hostent;

        /* Fix them up */
        Dns_PtrArrayToOffsetArray(Hostent->h_aliases, (ULONG_PTR)Hostent);
    }

    /* Do we have addresses? */
    if (Hostent->h_addr_list)
    {
        /* Fix them up */
        Dns_PtrArrayToOffsetArray(Hostent->h_addr_list, (ULONG_PTR)Hostent);
    }
}

