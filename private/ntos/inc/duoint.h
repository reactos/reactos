/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    duoint.h

Abstract:

    This module is the header file that describes hardware structure
    for the interrupt source and enable registers on the DUO system.

Author:

    Lluis Abello (lluis) 20-Apr-1993

Revision History:

--*/

#ifndef _DUOINT_
#define _DUOINT_

//
// Define Interrupt register structure.
//

typedef struct _INTERRUPT_REGISTERS {
    USHORT Fill;
    USHORT Enable;
} INTERRUPT_REGISTERS, *PINTERRUPT_REGISTERS;

#endif // _DUOINT_
