/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    context.c

Abstract:

    This module implements functions for creating and manipulating context
    tables. Context tables are used in WinSock 2.0 for associating 32-bit
    context values with socket handles.

Author:

    Keith Moore (keithmo)       08-Nov-1995

Revision History:

--*/


#include "precomp.h"


//
//  Private constants.
//

//
// INITIAL_ARRAY_ENTRIES is the initial number of lookup array entries
// in a newly created context table.
//
// ARRAY_GROWTH_DELTA is the growth delta used when growing the lookup
// array.
//

#define INITIAL_ARRAY_ENTRIES   256
#define ARRAY_GROWTH_DELTA      256

//
// TABLE_TO_LOCK() returns a pointer to a CONTEXT_TABLE's CRITICAL_SECTION
// object. This macro is only valid if the WAH_CONTEXT_FLAG_SERIALIZE bit
// is set in the Flags DWORD.
//

#define TABLE_TO_LOCK(t)        ((LPCRITICAL_SECTION)((t) + 1))

//
// LOCK_TABLE locks a CONTEXT_TABLE, if the CONTEXT_TABLE is serialized.
//
// UNLOCK_TABLE unlocks a CONTEXT_TABLE, if the CONTEXT_TABLE is serialized.
//

#define LOCK_TABLE(t)                                           \
            if( (t)->Flags & WAH_CONTEXT_FLAG_SERIALIZE ) {     \
                EnterCriticalSection( TABLE_TO_LOCK(t) );       \
            } else

#define UNLOCK_TABLE(t)                                         \
            if( (t)->Flags & WAH_CONTEXT_FLAG_SERIALIZE ) {     \
                LeaveCriticalSection( TABLE_TO_LOCK(t) );       \
            } else

//
// Map a SOCKET to a KEY.
//

#ifdef CHICAGO
#define SOCKET_TO_KEY(s)        ((DWORD)(s))
#else
#define SOCKET_TO_KEY(s)        ((DWORD)(s) >> 2)
#endif


//
//  Private types.
//

typedef struct _CONTEXT_TABLE {

    DWORD Flags;
    DWORD LookupArraySize;
    LPVOID * LookupArray;

} CONTEXT_TABLE;


//
//  Public functions.
//


DWORD
WINAPI
WahCreateContextTable(
    LPCONTEXT_TABLE * Table,
    DWORD Flags
    )

/*++

Routine Description:

    Creates a new context table.

Arguments:

    Table - If successful, receives a pointer to the newly created context
        table.

    Flags - Flags to control the tables behaviour.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    DWORD newTableSize;
    LPCONTEXT_TABLE newTable;

    //
    // Create the new context table.
    //

    newTableSize = sizeof(*newTable);

    if( Flags & WAH_CONTEXT_FLAG_SERIALIZE ) {

        newTableSize += sizeof(CRITICAL_SECTION);

    }

    newTable = ALLOC_MEM( newTableSize );

    if( newTable != NULL ) {

        //
        // Initialize it.
        //

        if( Flags & WAH_CONTEXT_FLAG_SERIALIZE ) {

            InitializeCriticalSection( TABLE_TO_LOCK( newTable ) );

        }

        newTable->Flags = Flags;
        newTable->LookupArraySize = INITIAL_ARRAY_ENTRIES;

        newTable->LookupArray = ALLOC_MEM( INITIAL_ARRAY_ENTRIES * sizeof(LPVOID) );

        if( newTable->LookupArray != NULL ) {

            //
            // Success!
            //

            *Table = newTable;
            return NO_ERROR;

        }

        //
        // Allocated the CONTEXT_TABLE, but could not allocate the
        // lookup array. Bummer.
        //

        FREE_MEM( newTable );
        newTable = NULL;

    }

    *Table = newTable;
    return ERROR_NOT_ENOUGH_MEMORY;

}   // WahCreateContextTable


DWORD
WINAPI
WahDestroyContextTable(
    LPCONTEXT_TABLE Table
    )

/*++

Routine Description:

    Destroys an existing context table.

Arguments:

    Table - A pointer to the table to destroy.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    //
    // Delete the context table's critical section if necessary.
    //

    if( Table->Flags & WAH_CONTEXT_FLAG_SERIALIZE ) {

        DeleteCriticalSection( TABLE_TO_LOCK( Table ) );

    }

    //
    // Free the resources.
    //

    FREE_MEM( Table->LookupArray );
    FREE_MEM( Table );

    //
    // Success!
    //

    return NO_ERROR;

}   // WahDestroyContextTable


DWORD
WINAPI
WahSetContext(
    LPCONTEXT_TABLE Table,
    SOCKET Socket,
    LPVOID Context
    )

/*++

Routine Description:

    Associates the given context with the given key.

Arguments:

    Table - The table to contain the new context.

    Socket - The key to index the new context.

    Context - The new context.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    DWORD newLookupArraySize;
    LPVOID * newLookupArray;
    DWORD key = SOCKET_TO_KEY( Socket );

    //
    // Acquire the lock protecting the context table.
    //

    LOCK_TABLE( Table );

    //
    // Determine if we can do this without growing the lookup array.
    //

    if( Table->LookupArraySize > key ) {

        Table->LookupArray[key] = Context;
        UNLOCK_TABLE( Table );
        return NO_ERROR;

    }

    //
    // We'll need to grow the lookup array first.
    //

    newLookupArraySize = Table->LookupArraySize +
        ARRAY_GROWTH_DELTA *
            ( ( ( key - Table->LookupArraySize ) / ARRAY_GROWTH_DELTA ) + 1 );

    newLookupArray = REALLOC_MEM(
                         Table->LookupArray,
                         newLookupArraySize * sizeof(LPVOID)
                         );

    if( newLookupArray != NULL ) {

        Table->LookupArray = newLookupArray;
        Table->LookupArraySize = newLookupArraySize;
        Table->LookupArray[key] = Context;
        UNLOCK_TABLE( Table );
        return NO_ERROR;

    }

    //
    // Error growing the lookup array.
    //

    UNLOCK_TABLE( Table );
    return ERROR_NOT_ENOUGH_MEMORY;

}   // WahSetContext


DWORD
WINAPI
WahGetContext(
    LPCONTEXT_TABLE Table,
    SOCKET Socket,
    LPVOID * Context
    )

/*++

Routine Description:

    Retrieves the context associated with the given key.

Arguments:

    Table - The table to use for the context lookup.

    Socket - The key to lookup.

    Context - If successful, receives the context value.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    DWORD key = SOCKET_TO_KEY( Socket );

    //
    // Acquire the lock protecting the context table.
    //

    LOCK_TABLE( Table );

    //
    // Validate the key before indexing into the lookup array.
    //

    if( key < Table->LookupArraySize ) {

        *Context = Table->LookupArray[key];

        if( *Context != NULL ) {

            UNLOCK_TABLE( Table );
            return NO_ERROR;

        }

    }

    //
    // Invalid key.
    //

    UNLOCK_TABLE( Table );

    return ERROR_INVALID_PARAMETER;

}   // WahGetContext


DWORD
WINAPI
WahRemoveContextEx(
    LPCONTEXT_TABLE Table,
    SOCKET Socket,
    LPVOID Context
    )

/*++

Routine Description:

    Removes a context association from the given table.

Arguments:

    Table - The table to remove the context from.

    Socket - The key to remove.

    Context - the context that should be in the table

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    DWORD key = SOCKET_TO_KEY( Socket );
    DWORD err;

    //
    // Acquire the lock protecting the context table.
    //

    LOCK_TABLE( Table );

    //
    // Validate the key before indexing into the lookup array.
    //

    if( key < Table->LookupArraySize ) {

        if( !Context || ( Table->LookupArray[key] == Context ) ) {

            Table->LookupArray[key] = NULL;
            err = NO_ERROR;

        } else {

            if( Table->LookupArray[key] ) {

                err = ERROR_FILE_EXISTS;

            } else {

                err = ERROR_DEV_NOT_EXIST;

            }

        }

    } else {

        err = ERROR_INVALID_PARAMETER;

    }

    //
    // Cleanup and return
    //

    UNLOCK_TABLE( Table );
    return err;

}   // WahRemoveContextEx


DWORD
WINAPI
WahRemoveContext(
    LPCONTEXT_TABLE Table,
    SOCKET Socket
    )
{

    return WahRemoveContextEx( Table, Socket, NULL );

}   // WahRemoveContext

