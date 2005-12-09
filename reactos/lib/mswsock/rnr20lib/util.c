/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

PVOID
WSPAPI
Temp_AllocZero(IN DWORD Size)
{
    PVOID Data;
    
    /* Allocate the memory */
    Data = DnsApiAlloc(Size);
    if (Data)
    {
        /* Zero it out */
        RtlZeroMemory(Data, Size);
    }

    /* Return it */
    return Data;
}

