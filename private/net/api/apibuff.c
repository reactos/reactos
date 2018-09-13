/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    ApiBuff.c

Abstract:

    This module contains routines for allocating and freeing API buffers.

    BUGBUG: expand description; mention RPCability, one end-user API and
    one internal.

Author:

    John Rogers (JohnRo) 25-Jan-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    Parts of the commentary for this file are extracted from material written
    by Alec Barker (AlecB@Microsoft).

Revision History:

    15-Mar-91 JohnRo
        Use <netdebug.h> and netlib routines.
    25-Apr-1991 JohnRo
        Call MIDL_user_allocate and MIDL_user_free.  Delete tabs.
    03-Dec-1991 JohnRo
        Added public NetApiBufferAllocate, NetApiBufferReallocate, and
        NetApiBufferSize APIs.
        Make sure buffers are aligned for worst case use.
    10-May-1992 JohnRo
        Treat alloc and realloc of size zero as non-error (return NULL ptr).
        Use <prefix.h> equates.
        Include my own prototypes so compiler can check them.
    18-May-1992 JohnRo
        RAID 9258: return non-null pointer when allocating zero bytes.

--*/

// These must be included first:

#include <windef.h>     // IN, LPVOID, etc.
#include <lmcons.h>     // NET_API_FUNCTION, etc.
#include <rpc.h>        // rpc prototypes

// These may be included in any order:

#include <align.h>      // POINTER_IS_ALIGNED(), ALIGN_WORST.
#include <lmapibuf.h>   // My prototypes.
#include <netdebug.h>   // NetpAssert(), NetpKdPrint(()), FORMAT_.
#include <prefix.h>     // PREFIX_ equates.
#include <rpcutil.h>    // MIDL_user_allocate(), etc.
#include <winerror.h>   // NO_ERROR and ERROR_ equates.


NET_API_STATUS NET_API_FUNCTION
NetApiBufferAllocate(
    IN DWORD ByteCount,
    OUT LPVOID * Buffer
    )

/*++

Routine Description:

    NetApiBufferAllocate is an internal function that allocates buffers
    which the APIs will return to the application.  (Usually these are for
    get-info operations.)

Arguments:

    ByteCount - Supplies the size (in bytes) that must be allocated for this
        buffer.  This may be zero, in which case a non-null pointer is
        passed-back and NO_ERROR is returned.

    Buffer - On return a pointer to the allocated area is returned in the
        address pointed to by Buffer.  (This is set to NULL on error.)
        The allocated area is guaranteed to be worst-case aligned for any
        use whatsoever.

Return Value:

    NET_API_STATUS - NO_ERROR if size is zero or memory was allocated.
        ERROR_NOT_ENOUGH_MEMORY if memory is not available.
        ERROR_INVALID_PARAMETER if a parameter is in error.

--*/

{

    if (Buffer == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Allocate the space.  Note that MIDL_user_allocate must allow zero
    // bytes to be allocated.
    //
    *Buffer = MIDL_user_allocate(ByteCount);

    if (*Buffer == NULL) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }
    NetpAssert( POINTER_IS_ALIGNED( *Buffer, ALIGN_WORST) );

    return (NO_ERROR);

} // NetApiBufferAllocate



NET_API_STATUS NET_API_FUNCTION
NetApiBufferFree (
    IN LPVOID Buffer
    )

/*++

Routine Description:

    NetApiBufferFree is called to deallocate memory which was acquired by
    a previous Net API call (e.g. NetApiBufferAllocate, NetWkstaGetInfo, and
    so on).

Arguments:

    Buffer - Supplies a pointer to an API information buffer previously
        returned on a Net API call.  (This would have been allocated by
        NetapipAllocateBuffer on behalf of one of the end-user Net API calls,
        e.g. NetWkstaGetInfo.)

Return Value:

    NET_API_STATUS.  Returns NO_ERROR if Buffer is null or memory was freed.
        Returns ERROR_INVALID_PARAMETER if Buffer points to an unaligned area.

--*/

{
    if (Buffer == NULL) {
        return (NO_ERROR);
    }

    if ( !POINTER_IS_ALIGNED( Buffer, ALIGN_WORST ) ) {
        NetpKdPrint(( PREFIX_NETAPI "NetApiBufferFree: unaligned input ptr: "
                FORMAT_LPVOID "!\n", (LPVOID) Buffer ));
        return (ERROR_INVALID_PARAMETER);
    }

    MIDL_user_free(Buffer);

    return (NO_ERROR);

} // NetApiBufferFree


NET_API_STATUS NET_API_FUNCTION
NetApiBufferReallocate(
    IN LPVOID OldBuffer OPTIONAL,
    IN DWORD NewByteCount,
    OUT LPVOID * NewBuffer
    )
{
    LPVOID NewPointer;

    if ( (OldBuffer==NULL) && (NewByteCount==0) ) {
        *NewBuffer = NULL;
        return (NO_ERROR);
    }

    NewPointer = (void *)MIDL_user_reallocate(  // may alloc, realloc, or free.
            (void *) OldBuffer,
            (unsigned long) NewByteCount);

    if (NewByteCount == 0) {                    // free
        *NewBuffer = NULL;
        return (NO_ERROR);
    } else if (NewPointer == NULL) {            // out of memory
        *NewBuffer = OldBuffer;                 // (don't lose old buffer)
        return (ERROR_NOT_ENOUGH_MEMORY);
    } else {                                    // alloc or realloc
        *NewBuffer = NewPointer;
        return (NO_ERROR);
    }

    /*NOTREACHED*/


} // NetApiBufferReallocate


NET_API_STATUS NET_API_FUNCTION
NetApiBufferSize(
    IN LPVOID Buffer,
    OUT LPDWORD ByteCount
    )
{
    DWORD AllocedSize;

    if ( (Buffer==NULL) || (ByteCount==NULL) ) {
        return (ERROR_INVALID_PARAMETER);
    } else if (POINTER_IS_ALIGNED( ByteCount, ALIGN_DWORD ) == FALSE) {
        return (ERROR_INVALID_PARAMETER);
    } else if (POINTER_IS_ALIGNED( Buffer, ALIGN_WORST ) == FALSE) {
        // Caller didn't get this pointer from us!
        return (ERROR_INVALID_PARAMETER);
    }

    AllocedSize = (unsigned long)MIDL_user_size(
            (void *) Buffer);

    NetpAssert( AllocedSize > 0 );

    *ByteCount = AllocedSize;
    return (NO_ERROR);


} // NetApiBufferSize



NET_API_STATUS NET_API_FUNCTION
NetapipBufferAllocate (
    IN DWORD ByteCount,
    OUT LPVOID * Buffer
    )

/*++

Routine Description:

    NetapipBufferAllocate is an old internal function that allocates buffers
    which the APIs will return to the application.  All calls to this routine
    should eventually be replaced by calls to NetApiBufferAllocate.

Arguments:

    (Same as NetApiBufferAllocate.)

Return Value:

    (Same as NetApiBufferAllocate.)

--*/

{
    return (NetApiBufferAllocate( ByteCount, Buffer ));

} // NetapipBufferAllocate
