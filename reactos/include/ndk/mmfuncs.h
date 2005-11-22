/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    mmfuncs.h

Abstract:

    Functions definitions for the Memory Manager.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _MMFUNCS_H
#define _MMFUNCS_H

//
// Dependencies
//

//
// Section Functions
//
NTSTATUS
NTAPI
MmUnmapViewOfSection(
    struct _EPROCESS* Process,
    PVOID BaseAddress
);

#endif
