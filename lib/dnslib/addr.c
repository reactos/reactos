/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS DNS Shared Library
 * FILE:        lib/dnslib/addr.c
 * PURPOSE:     Contains the Address Family Information Tables
 */

/* INCLUDES ******************************************************************/
#include "precomp.h"

/* DATA **********************************************************************/

DNS_FAMILY_INFO AddrFamilyTable[3] =
{
    {
        AF_INET,
        DNS_TYPE_A,
        sizeof(IP4_ADDRESS),
        sizeof(SOCKADDR_IN),
        FIELD_OFFSET(SOCKADDR_IN, sin_addr)
    },
    {
        AF_INET6,
        DNS_TYPE_AAAA,
        sizeof(IP6_ADDRESS),
        sizeof(SOCKADDR_IN6),
        FIELD_OFFSET(SOCKADDR_IN6, sin6_addr)
    },
    {
        AF_ATM,
        DNS_TYPE_ATMA,
        sizeof(ATM_ADDRESS),
        sizeof(SOCKADDR_ATM),
        FIELD_OFFSET(SOCKADDR_ATM, satm_number)
    }
};

/* FUNCTIONS *****************************************************************/

PDNS_FAMILY_INFO
WINAPI
FamilyInfo_GetForFamily(IN WORD AddressFamily)
{
    /* Check which family this is */
    switch (AddressFamily)
    {
        case AF_INET:
            /* Return IPv4 Family Info */
            return &AddrFamilyTable[0];
        
        case AF_INET6:
            /* Return IPv6 Family Info */
            return &AddrFamilyTable[1];
        
        case AF_ATM:
            /* Return ATM Family Info */
            return &AddrFamilyTable[2];
        
        default:
            /* Invalid family */
            return NULL;
    }

}

