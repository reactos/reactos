/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    registry.h

Abstract:

    Contains definitions for manipulating registry parameters.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _REGISTRY_H_
#define _REGISTRY_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
LoadRegistryParameters(
    );

BOOL
LoadScalarParameters(
    );

INT
InitRegistryNotifiers(
    );

INT
WaitOnRegNotification(
    );

BOOL
UnloadRegistryParameters(
    );

#endif // _REGISTRY_H_