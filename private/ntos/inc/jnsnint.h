/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    jensenint.h

Abstract:

    This module is the header file that describes hardware structure
    for the interrupt source and enable registers on the Alpha/Jensen

Author:

    David N. Cutler (davec) 6-May-1991
    Miche Baker-Harvey (miche) 18-May-1992

Revision History:

    Jeff McLeman (mcleman) 22-Jul-1992
      Add intack structure

--*/

#ifndef _JNSNINT_
#define _JNSNINT_

/*
//
// Define Interrupt register structure.
//

typedef struct _INTERRUPT_REGISTERS {
    UCHAR Source;
    UCHAR Fill1;
    USHORT Enable;
} INTERRUPT_REGISTERS, *PINTERRUPT_REGISTERS;
*/

typedef struct _INTACK_REGISTERS {
    union IntAckCycle {
       ULONG LowPart;
       ULONG HighPart;
     } IntAckCycle;
} INTACK_REGISTERS, *PINTACK_REGISTERS;


#endif // _JNSNINT_




