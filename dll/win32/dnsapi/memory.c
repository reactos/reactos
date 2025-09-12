/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/dnsapi/dnsapi/memory.c
 * PURPOSE:     DNSAPI functions built on the ADNS library.
 * PROGRAMER:   Art Yerkes
 * UPDATE HISTORY:
 *              12/15/03 -- Created
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

VOID
WINAPI
DnsApiFree(IN PVOID Data)
{
    RtlFreeHeap(RtlGetProcessHeap(), 0, Data);
}

PVOID
WINAPI
DnsApiAlloc(IN DWORD Size)
{
    return RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
}

PVOID
WINAPI
DnsQueryConfigAllocEx(IN DNS_CONFIG_TYPE Config,
                      OUT PVOID pBuffer,
                      IN OUT PDWORD pBufferLength)
{
    return NULL;
}

VOID WINAPI
DnsFree(PVOID Data,
        DNS_FREE_TYPE FreeType)
{
    switch(FreeType)
    {
        case DnsFreeFlat:
            RtlFreeHeap( RtlGetProcessHeap(), 0, Data );
            break;

        case DnsFreeRecordList:
            DnsIntFreeRecordList( (PDNS_RECORD)Data );
            break;

        case DnsFreeParsedMessageFields:
            /* assert( FALSE ); XXX arty not yet implemented. */
            break;
    }
}
