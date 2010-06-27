/*++ NDK Version: 0098

Copyright (c) ReactOS Portable Systems Group.  All rights reserved.

Header Name:

    vftypes.h

Abstract:

    Type definitions for the Driver Verifier.

Author:

    ReactOS Portable Systems Group (ros.arm@reactos.org) - Created - 27-Jun-2010

--*/

#ifndef _VFTYPES_H
#define _VFTYPES_H

//
// Dependencies
//
#include <umtypes.h>

//
// Failure Classes
//
typedef enum _VF_FAILURE_CLASS
{
    VFFAILURE_FAIL_IN_FIELD,
    VFFAILURE_FAIL_LOGO,
    VFFAILURE_FAIL_UNDER_DEBUGGER
} VF_FAILURE_CLASS, *PVF_FAILURE_CLASS;

//
// Object Types
//
typedef enum _VF_OBJECT_TYPE
{
    VFOBJTYPE_DRIVER,
    VFOBJTYPE_DEVICE,
    VFOBJTYPE_SYSTEM_BIOS
} VF_OBJECT_TYPE, PVF_OBJECT_TYPE;

#endif // _VFTYPES_H
