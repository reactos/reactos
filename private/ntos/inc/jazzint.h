/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    jazzint.h

Abstract:

    This module is the header file that describes hardware structure
    for the interrupt source and enable registers on the Jazz R3000
    and R4000 system.

Author:

    David N. Cutler (davec) 6-May-1991

Revision History:

--*/

#ifndef _JAZZINT_
#define _JAZZINT_

//
// Define Interrupt register structure.
//

typedef struct _INTERRUPT_REGISTERS {
    UCHAR Source;
    UCHAR Fill1;
    USHORT Enable;
} INTERRUPT_REGISTERS, *PINTERRUPT_REGISTERS;

#endif // _JAZZINT_
