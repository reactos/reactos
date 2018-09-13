/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    group.c

Abstract:

    This module contains group management routines used by the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        SockInitializeGroupManager();
        SockTerminateGroupManager();
        SockAcquireGroupLock()
        SockReleaseGroupLock()
        SockReferenceGroup()
        SockDereferenceGroup()
        SockGetGroup()
        SockValidateConstrainedGroup()

Author:

    Keith Moore (keithmo) 11-Jul-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Ensure our assumptions are valid.
//

#if ( SG_UNCONSTRAINED_GROUP != 1 ) || ( SG_CONSTRAINED_GROUP != 2 )
#error "SG_UNCONSTRAINED_GROUP or SG_CONSTRAINED_GROUP have unexpected values!"
#endif


//
// Private constants.
//

#define SOCK_GROUP_MAPPING_NAME "WS2MAP Group Mapping"
#define SOCK_GROUP_MUTEX_NAME   "WS2MAP Group Mutex"

#define SOCK_GROUP_CONSTRAINED  0x80000000L


//
// Private types.
//

typedef struct _SOCK_GROUP_DATA {

    //
    // The next available group ID.
    //

    LONG NextGroup;

} SOCK_GROUP_DATA, *PSOCK_GROUP_DATA;


//
// Private globals.
//

PSOCK_GROUP_DATA SockGroupData = NULL;
HANDLE SockGroupDataMapping = NULL;
HANDLE SockGroupDataMutex = NULL;
BOOL SockGroupDataInitialized = FALSE;


//
// Public routines.
//


BOOL
SockInitializeGroupManager(
    VOID
    )

/*++

Routine Description:

    Performs global initialization of the group ID manager.

Arguments:

    None.

Return Value:

    BOOL - TRUE if successful, FALSE otherwise.

--*/

{

    //
    // Short-circuit if we've already been initialized.
    //

    if( SockGroupDataInitialized ) {

        return TRUE;

    }

    SockAcquireGlobalLock();

    if( SockGroupDataInitialized ) {

        goto complete;

    }

    //
    // Initialize everything to a known state.
    //

    SockGroupData = NULL;
    SockGroupDataMapping = NULL;
    SockGroupDataMutex = NULL;

    //
    // Create the file mapping that will contain the shared data.
    //

    SockGroupDataMapping = CreateFileMapping(
                               INVALID_HANDLE_VALUE,
                               NULL,
                               PAGE_READWRITE,
                               0,
                               sizeof(*SockGroupData),
                               SOCK_GROUP_MAPPING_NAME
                               );

    if( SockGroupDataMapping == NULL ) {

        goto complete;

    }

    //
    // Map the data into our process.
    //

    SockGroupData = MapViewOfFile(
                        SockGroupDataMapping,
                        FILE_MAP_WRITE,
                        0,
                        0,
                        0
                        );

    if( SockGroupData == NULL ) {

        goto complete;

    }

    //
    // Create the protective mutex.
    //

    SockGroupDataMutex = CreateMutex(
                             NULL,
                             TRUE,
                             SOCK_GROUP_MUTEX_NAME
                             );

    if( SockGroupDataMutex == NULL ) {

        goto complete;

    }

    //
    // Determine if we need to setup the initial group table.
    //

    if( GetLastError() != ERROR_ALREADY_EXISTS ) {

        SockGroupData->NextGroup = 3;

    }

    //
    // Success!
    //

    SockGroupDataInitialized = TRUE;
    SockReleaseGroupLock();

    IF_DEBUG(SHARED_DATA) {

        SOCK_PRINT((
            "SockInitializeSharedData: shared data @ %08lx\n",
            SockGroupData
            ));

    }

complete:

    if( !SockGroupDataInitialized ) {

        SockTerminateGroupManager();

    }

    SockReleaseGlobalLock();

    return SockGroupDataInitialized;

}   // SockInitializeGroupManager



VOID
SockTerminateGroupManager(
    VOID
    )

/*++

Routine Description:

    Performs any required global termination of the group ID manager.

Arguments:

    None.

Return Value:

    None.

--*/

{

    if( SockGroupDataInitialized ) {

        if( SockGroupData != NULL ) {

            UnmapViewOfFile( SockGroupData );
            SockGroupData = NULL;

        }

        if( SockGroupDataMapping != NULL ) {

            CloseHandle( SockGroupDataMapping );
            SockGroupDataMapping = NULL;

        }

        if( SockGroupDataMutex != NULL ) {

            CloseHandle( SockGroupDataMutex );
            SockGroupDataMutex = NULL;

        }

    }

}   // SockTerminateGroupManager



VOID
SockAcquireGroupLock(
    VOID
    )

/*++

Routine Description:

    Acquires the lock protecting the shared data area.

Arguments:

    None.

Return Value:

    None.

--*/

{

    DWORD result;

    //
    // Wait for ownership of the mutex.
    //

    result = WaitForSingleObject(
                 SockGroupDataMutex,
                 INFINITE
                 );

    if( result == WAIT_FAILED ) {

        SOCK_PRINT((
            "SockAcquireGroupLock: WaitForSingleObject failed, error %d\n",
            GetLastError()
            ));

    }

}   // SockAcquireGroupLock



VOID
SockReleaseGroupLock(
    VOID
    )

/*++

Routine Description:

    Releases the lock protecting the shared data area.

Arguments:

    None.

Return Value:

    None.

--*/

{

    BOOL result;

    //
    // Relinquish ownership of the mutex.
    //

    result = ReleaseMutex( SockGroupDataMutex );

    if( !result ) {

        SOCK_PRINT((
            "SockReleaseGroupLock: ReleaseMutex failed, error %d\n",
            GetLastError()
            ));

    }

}   // SockReleaseGroupLock



BOOL
SockReferenceGroup(
    GROUP Group,
    PSOCK_GROUP_TYPE GroupType
    )

/*++

Routine Description:

    Bumps the reference count associated with the given group ID.

Arguments:

    Group - The group ID to reference.

    GroupType - Returns the type of the group.

Returns:

    BOOL - TRUE if the group ID was valid, FALSE otherwise.

--*/

{

    if( (LONG)Group & SOCK_GROUP_CONSTRAINED ) {

        *GroupType = GroupTypeConstrained;

    } else {

        *GroupType = GroupTypeUnconstrained;

    }

    return TRUE;

}   // SockReferenceGroup



BOOL
SockDereferenceGroup(
    GROUP Group
    )

/*++

Routine Description:

    Decrements the reference count associated with the given group ID.
    If the ref count drops to zero, the group ID is freed.

Arguments:

    Group - The group ID to dereference.

Returns:

    BOOL - TRUE if the group ID was valid, FALSE otherwise.

--*/

{

    return TRUE;

}   // SockDereferenceGroup



BOOL
SockGetGroup(
    GROUP * Group,
    PSOCK_GROUP_TYPE GroupType
    )

/*++

Routine Description:

    Examines the incoming group. If is zero, then nothing is done. If it
    is SG_CONSTRAINED_GROUP, then a new constrained group ID is created.
    If it is SG_UNCONSTRAINED_GROUP, then a new unconstrained group ID is
    created. Otherwise, it must identify an existing group, so that group
    is referenced.

Arguments:

    Group - Points to the group ID to examine/modify.

    GroupType - Returns the type of the group.

Return Value:

    BOOL - TRUE if successful, FALSE otherwise.

--*/

{

    GROUP groupValue;
    GROUP newGroup;

    groupValue = *Group;

    //
    // Zero means "no group", so just ignore it.
    //

    if( groupValue == 0 ) {

        *GroupType = GroupTypeNeither;
        return TRUE;

    }

    //
    // If we're being asked to create a new group, do it.
    //

    if( groupValue == SG_CONSTRAINED_GROUP ||
        groupValue == SG_UNCONSTRAINED_GROUP ) {

        //
        // Lock the table.
        //

        SockAcquireGroupLock();

        //
        // Get a new group value.
        //

        do {

            newGroup = (GROUP)SockGroupData->NextGroup++;

        } while( newGroup < 3 );

        //
        // Munge if necessary.
        //

        if( groupValue == SG_CONSTRAINED_GROUP ) {

            newGroup |= SOCK_GROUP_CONSTRAINED;

        }

        *Group = newGroup;
        *GroupType = groupValue;

        SockReleaseGroupLock();
        return TRUE;

    }

    //
    // Otherwise, just reference the group.
    //

    return SockReferenceGroup( groupValue, GroupType );

}   // SockGetGroup



BOOL
SockValidateConstrainedGroup(
    GROUP Group,
    SOCK_GROUP_TYPE GroupType,
    SOCKADDR * Address,
    INT AddressLength
    )

/*++

Routine Description:

    This routine does all of the nasty stuff necessary for enforce
    constrained groups.

Arguments:

    Group - The group to validate.

    GroupType - The type of group (constrained or unconstrained).

    Address - The address we're trying to interact with.

    AddressLength - The length of Address.

Return Value:

    BOOL - TRUE if the group is consistent, FALSE otherwise.

--*/

{

    //
    // BUGBUG: If anyone really cares, we can do the right thing someday.
    //
    // The right thing will involve:
    //
    //     o Storing the remote address in the SOCKET_INFORMATION
    //       structure. This must be updated in connect() and accept().
    //
    //     o Grabbing the global lock, and scanning the list of all
    //       sockets, looking for those that belong to the same
    //       constrained group. For each found, the addresses must
    //       be compared in a "special way" to determine if they
    //       refer to the same interface (i.e. ignoring the port).
    //
    //     o We can optimize this somewhat by keeping a separate linked
    //       list of just those sockets that belong to constrained
    //       groups.
    //

    return TRUE;

}   // SockValidateConstrainedGroup

