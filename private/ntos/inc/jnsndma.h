/*++


Copyright (c) 1992  Digital Equipment Corporation

Module Name:

    jnsndma.h

Abstract:

    This module is the header file that describes the dma structures
    for the Jensen systems.

Author:


    Jeff McLeman (mcleman) 13-Jul-1992

Revision History:

    16-Jul-1992 Jeff McLeman (mcleman)
      QUASI_VIRTUAL_ADDRESS should be a PVOID

--*/

#ifndef _JENSENDMA_
#define _JENSENDMA_


//
// Define translation table entry structure.
//

typedef volatile struct _TRANSLATION_ENTRY {
    PVOID VirtualAddress;
    ULONG PhysicalAddress;
    ULONG Index;
} TRANSLATION_ENTRY, *PTRANSLATION_ENTRY;

//
// Define QUASI_VIRTUAL_ADDRESS
//
typedef PVOID QUASI_VIRTUAL_ADDRESS;


#endif // _JENSENDMA_

