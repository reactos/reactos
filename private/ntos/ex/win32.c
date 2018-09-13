/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    win32.c

Abstract:

   This module implements the definition of the executive Win32 objects.
   Functions to manage these objects are implemented in win32k.sys.

Author:

    James I. Anderson (jima) 14-June-1995

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

//
// Address of windowstation and desktop object type descriptors.
//

POBJECT_TYPE ExWindowStationObjectType;
POBJECT_TYPE ExDesktopObjectType;

/*
 * windowstation generic mapping
 */
GENERIC_MAPPING ExpWindowStationMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_REQUIRED
};

/*
 * desktop generic mapping
 */
GENERIC_MAPPING ExpDesktopMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_REQUIRED
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExpWin32Initialization)
#endif

BOOLEAN
ExpWin32Initialization (
    )

/*++

Routine Description:

    This function creates the Win32 object type descriptors at system
    initialization and stores the address of the object type descriptor
    in local static storage.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the Win32 object type descriptors are
    successfully created. Otherwise a value of FALSE is returned.

--*/

{

    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    NTSTATUS Status;
    UNICODE_STRING TypeName;

    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString(&TypeName, L"WindowStation");

    //
    // Create windowstation object type descriptor.
    //

    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.GenericMapping = ExpWindowStationMapping;
    ObjectTypeInitializer.SecurityRequired = TRUE;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK |
                                              OBJ_PERMANENT |
                                              OBJ_EXCLUSIVE;
    ObjectTypeInitializer.ValidAccessMask = STANDARD_RIGHTS_REQUIRED;
    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                (PSECURITY_DESCRIPTOR)NULL,
                                &ExWindowStationObjectType);

    //
    // If the windowstation object type descriptor was not successfully 
    // created, then return a value of FALSE.
    //

    if (!NT_SUCCESS(Status))
        return FALSE;

    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString(&TypeName, L"Desktop");

    //
    // Create windowstation object type descriptor.
    //

    ObjectTypeInitializer.GenericMapping = ExpDesktopMapping;
    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                (PSECURITY_DESCRIPTOR)NULL,
                                &ExDesktopObjectType);

    //
    // If the desktop object type descriptor was successfully created, then
    // return a value of TRUE. Otherwise return a value of FALSE.
    //

    return (BOOLEAN)(NT_SUCCESS(Status));
}
