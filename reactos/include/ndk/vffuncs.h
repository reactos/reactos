/*++ NDK Version: 0098

Copyright (c) ReactOS Portable Systems Group.  All rights reserved.

Header Name:

    vffuncs.h

Abstract:

    Function definitions for the Driver Verifier.

Author:

    ReactOS Portable Systems Group (ros.arm@reactos.org) - Created - 27-Jun-2010

--*/

#ifndef _VFFUNCS_H
#define _VFFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <vftypes.h>

#ifndef NTOS_MODE_USER

//
// Verifier Device Driver Interface
//
BOOLEAN
NTAPI
VfIsVerificationEnabled(
    _In_ VF_OBJECT_TYPE VfObjectType,
    _In_opt_ PVOID Object
);

VOID
VfFailDeviceNode(
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ ULONG BugCheckMajorCode,
    _In_ ULONG BugCheckMinorCode,
    _In_ VF_FAILURE_CLASS FailureClass,
    _Inout_ PULONG AssertionControl,
    _In_ PSTR DebuggerMessageText,
    _In_ PSTR ParameterFormatString,
    ...
);
#endif

#endif
