/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    handle.cxx

Abstract:

    Contains functions to allocate and deallocate handle values for various
    Windows Internet Extensions DLL 'objects'

    Functions in this module generate pseudo-handle values and free them when
    no longer required. Each handle value is generated from its position within
    a (2K) bitmap (== 16K handles max.). We also maintain an array that is used
    to map the generated handle to the address of the handle object that it
    represents

    Contents:
        HandleInitialize
        HandleTerminate
        AllocateHandle
        FreeHandle
        MapHandleToAddress
        DereferenceObject
        (BitToIndex)

Author:

    Richard L Firth (rfirth) 31-Oct-1994

Revision History:

    11-Jan-1996 rfirth
        Use fixed memory instead of moveable (Win95 has a bug w/ LocalUnlock)

    31-Oct-1994 rfirth
        Created

--*/

#include <wininetp.h>

//
// manifests
//

#define BASE_HANDLE_VALUE       0x00cc0000
#define HANDLE_INCREMENT        4
#define BITS_PER_BYTE           8
#define BITS_IN_DWORD           (sizeof(DWORD) * BITS_PER_BYTE)
#define FULL_DWORD              ((DWORD)-1)
#define MAXIMUM_HANDLE_NUMBER   (64 K)
#define MAXIMUM_HANDLE_COUNT    ((MAXIMUM_HANDLE_NUMBER / HANDLE_INCREMENT) - 1)
#define MAXIMUM_DWORD_INDEX     ((MAXIMUM_HANDLE_COUNT + BITS_IN_DWORD - 1) / BITS_IN_DWORD)
#define MINIMUM_HANDLE_VALUE    (BASE_HANDLE_VALUE + HANDLE_INCREMENT)
#define MAXIMUM_HANDLE_VALUE    (BASE_HANDLE_VALUE + MAXIMUM_HANDLE_NUMBER - HANDLE_INCREMENT)
#define INITIAL_MAP_LENGTH      16  // 512 handles == 2048 bytes
#define HANDLE_MAP_INCREMENT    16

// Warning: In order for 64-bit compatibility, the range of handle
// values must be restricted to quantities representable by 32-bits.
// If maximum handle value >= 4GB the implementation has to change 
// use 64-bit integral types internally.

//
// macros
//

#define NEXT_HANDLE_VALUE(d, i) (BASE_HANDLE_VALUE + ((d) * BITS_IN_DWORD + ((i) + 1)) * HANDLE_INCREMENT)

//
// private prototypes
//

PRIVATE
inline
DWORD
BitToIndex(
    IN DWORD Bit
    );

//
// private data
//

PRIVATE CRITICAL_SECTION HandleMapCritSec;  // protects access to following variables
PRIVATE LPDWORD HandleMap = NULL;           // bitmap of allocated handles
PRIVATE DWORD HandleMapLength;              // number of DWORDs in HandleMap
PRIVATE DWORD NextHandleMapDword = 0;       // first bitmap DWORD to check
PRIVATE DWORD NextHandleMapBit = 1;         // map of first bit to check
PRIVATE DWORD NextHandleMapBitIndex = 0;    // position of first bit in first DWORD
PRIVATE DWORD NextHandleValue = MINIMUM_HANDLE_VALUE;
PRIVATE DWORD NextHandleIndex = 0;
PRIVATE BOOL Initialized = FALSE;
PRIVATE LPVOID * MapArray = NULL;
PRIVATE DWORD MapArrayLength = 0;

//
// functions
//


DWORD
HandleInitialize(
    VOID
    )

/*++

Routine Description:

    Performs initialization required by functions in this module

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - return code from LocalAlloc

--*/

{
    DEBUG_ENTER((DBG_HANDLE,
                Dword,
                "HandleInitialize",
                NULL
                ));

    InitializeCriticalSection(&HandleMapCritSec);
    HandleMapLength = INITIAL_MAP_LENGTH;

    DWORD error;

    //
    // ResizeBuffer() doesn't use LMEM_ZEROINIT
    //

    HandleMap = (LPDWORD)ALLOCATE_MEMORY(LMEM_ZEROINIT,
                                         HandleMapLength * sizeof(DWORD)
                                         );
    if (HandleMap != NULL) {
        MapArrayLength = INITIAL_MAP_LENGTH * BITS_IN_DWORD;
        MapArray = (LPVOID *)ALLOCATE_MEMORY(LMEM_ZEROINIT,
                                             MapArrayLength * sizeof(LPVOID)
                                             );
        if (MapArray != NULL) {
            Initialized = TRUE;
            error = ERROR_SUCCESS;
        }
    }

    if (!HandleMap || !MapArray) {
        error = GetLastError();
        HandleTerminate();
    }

    DEBUG_LEAVE(error);

    return error;
}


VOID
HandleTerminate(
    VOID
    )

/*++

Routine Description:

    Obverse of HandleInitialize - frees any system resources allocated by
    HandleInitialize

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HANDLE,
                None,
                "HandleTerminate",
                NULL
                ));

    if (Initialized) {

        //
        // there shouldn't be any other threads active when this function is
        // called but we'll grab the critical section anyway, just to make sure
        //

        EnterCriticalSection(&HandleMapCritSec);

        //
        // free up the memory occupied by the handle bitmap and map array
        //

        if (HandleMap != NULL) {
            HandleMap = (LPDWORD)FREE_MEMORY((HLOCAL)HandleMap);
        }

        INET_ASSERT(HandleMap == NULL);

        if (MapArray != NULL) {
            MapArray = (LPVOID *)FREE_MEMORY((HLOCAL)MapArray);
        }

        INET_ASSERT(MapArray == NULL);

        //
        // no longer initialized
        //

        Initialized = FALSE;

        //
        // and reset the variables
        //

        HandleMapLength = 0;
        NextHandleMapDword = 0;
        NextHandleMapBit = 1;
        NextHandleMapBitIndex = 0;
        NextHandleValue = MINIMUM_HANDLE_VALUE;
        NextHandleIndex = 0;
        MapArrayLength = 0;

        LeaveCriticalSection(&HandleMapCritSec);

        //
        // delete the critical section
        //

        DeleteCriticalSection(&HandleMapCritSec);
    }

    DEBUG_LEAVE(0);
}


DWORD
AllocateHandle(
    IN LPVOID Address,
    OUT LPHINTERNET lpHandle
    )

/*++

Routine Description:

    Generic handle allocator function which generates a unique handle value for
    any object. The handle value is simply a number, slightly massaged to give a
    value that can be easily differentiated from other Win32 handle ranges (for
    debugging purposes mainly, and also to protect against illegal use of
    handles (such as treating as a pointer and dereferencing it)).

    The range of handles are kept in a bitmap, composed of DWORDs. The next
    allocated handle is simply the index of the next free bit in the map. The
    map is extended if we run out of handles (up to 16K-1 handles (== 2K bytes)).

    If the map needs to be extended we don't go to the trouble to reduce it
    again. For the sake of 2K bytes max (currently), its not worth the effort.

    This function does not rely on knowing the type of object for which the
    handle is being generated, and only requires serialization via a critical
    section.

    This function can increase the values of these variables:

        NextHandleMapDword
        NextHandleMapBit
        NextHandleMapBitIndex
        NextHandleValue
        NextHandleIndex

Arguments:

    Address     - the (object address) value which will be associated with the
                  returned handle

    lpHandle    - place to return the allocated handle

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_OUT_OF_HANDLES
                    16K-1 (currently) handles are outstanding!

                  ERROR_NOT_ENOUGH_MEMORY etc.
                    problems with Win32 memory/heap management?

--*/

{
    DEBUG_ENTER((DBG_HANDLE,
                Dword,
                "AllocateHandle",
                "%#x, %#x",
                Address,
                lpHandle
                ));

    DWORD error;

    //
    // can't associate a NULL address with the generated handle
    //

    INET_ASSERT(Address != NULL);

    //
    // default returned handle
    //

    *lpHandle = NULL;

    //
    // in case this function is being called before we have initialized this
    // module, or after we have terminated it, return an error
    //

    if (!Initialized) {
        error = ERROR_INTERNET_SHUTDOWN;
        goto quit;
    }

    error = ERROR_INTERNET_OUT_OF_HANDLES;

    EnterCriticalSection(&HandleMapCritSec);

    if (HandleMap == NULL) {

        //
        // don't ever expect this?
        //

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto unlock_exit;
    }

    while (NextHandleMapDword != MAXIMUM_DWORD_INDEX) {

        //
        // if we reached the end of the map the last time, we must reallocate
        //

        if (NextHandleMapDword == HandleMapLength) {

            HLOCAL newHandleMap;

            newHandleMap = REALLOCATE_MEMORY((HLOCAL)HandleMap,
                                             (HandleMapLength + HANDLE_MAP_INCREMENT)
                                             * sizeof(DWORD),
                                             LMEM_MOVEABLE | LMEM_ZEROINIT
                                             );

            HLOCAL newMapArray;

            newMapArray = REALLOCATE_MEMORY((HLOCAL)MapArray,
                                            (HandleMapLength + HANDLE_MAP_INCREMENT)
                                            * BITS_IN_DWORD
                                            * sizeof(LPVOID),
                                            LMEM_MOVEABLE | LMEM_ZEROINIT
                                            );
            if ((newHandleMap != NULL) && (newMapArray != NULL)) {
                HandleMapLength += HANDLE_MAP_INCREMENT;
                HandleMap = (LPDWORD)newHandleMap;
                MapArrayLength += HANDLE_MAP_INCREMENT * BITS_IN_DWORD;
                MapArray = (LPVOID *)newMapArray;

                DEBUG_PRINT(HANDLE,
                            INFO,
                            ("re-allocated %d DWORDs: HandleMap = %#x MapArray = %#x\n",
                            HandleMapLength,
                            HandleMap,
                            MapArray
                            ));

            } else {

                error = GetLastError();

                DEBUG_PRINT(HANDLE,
                            ERROR,
                            ("REALLOCATE_MEMORY() returns %d\n",
                            error
                            ));

                break;
            }
        } else if (NextHandleValue <= MAXIMUM_HANDLE_VALUE) {
            HandleMap[NextHandleMapDword] |= NextHandleMapBit;

            //
            // first handle value returned is 0x00cc0004
            //

            DEBUG_PRINT(HANDLE,
                        INFO,
                        ("handle = %#x, index = %d\n",
                        NextHandleValue,
                        NextHandleIndex
                        ));

            *lpHandle = (HINTERNET)NextHandleValue;

            //
            // store it in the map array at the specified index
            //

            INET_ASSERT(MapArray[NextHandleIndex] == NULL);

            MapArray[NextHandleIndex] = Address;

            error = ERROR_SUCCESS;

            //
            // find the next available bit for the next caller. Search up
            // to the end of the currently allocated map. If we don't find
            // it, the next caller will attempt to allocate a new DWORD
            // (unless a lower handle gets freed meantime)
            //

            while (NextHandleMapDword < HandleMapLength) {
                if (HandleMap[NextHandleMapDword] != FULL_DWORD) {
                    while (HandleMap[NextHandleMapDword] & NextHandleMapBit) {
                        NextHandleMapBit <<= 1;
                        ++NextHandleMapBitIndex;
                        NextHandleValue += HANDLE_INCREMENT;
                        ++NextHandleIndex;
                    }
                } else {
                    NextHandleMapBit = 0;
                }
                if (NextHandleMapBit != 0) {
                    break;
                } else {

                    //
                    // reached the end of a DWORD. Start the next
                    //

                    ++NextHandleMapDword;
                    NextHandleMapBit = 1;
                    NextHandleMapBitIndex = 0;

                    //
                    // recalculate the next handle value
                    //

                    NextHandleValue = NEXT_HANDLE_VALUE(NextHandleMapDword,
                                                        NextHandleMapBitIndex
                                                        );

                    //
                    // and map array index
                    //

                    NextHandleIndex = NextHandleMapDword * BITS_IN_DWORD;
                }
            }
            break;
        } else {

            //
            // reached maximum handle value - return error
            //

            break;
        }
    }

unlock_exit:

    LeaveCriticalSection(&HandleMapCritSec);

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
FreeHandle(
    IN HINTERNET Handle
    )

/*++

Routine Description:

    The obverse function to AllocateHandle. Frees up a previously allocated
    handle value. If this handle has a lower index than the currently selected
    next index (Dword and Bit) then the next index is modified

    This function can reduce the values of these variables:

        NextHandleMapDword
        NextHandleMapBit
        NextHandleMapBitIndex
        NextHandleValue
        NextHandleIndex

Arguments:

    Handle  - handle value previously allocated via AllocateHandle

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE
                    We don't think this handle was generated by AllocateHandle
                    or the corresponding bit in the map is already 0

--*/

{
    DEBUG_ENTER((DBG_HANDLE,
                Dword,
                "FreeHandle",
                "%#x",
                Handle
                ));

    DWORD error;

    //
    // ensure that we are in the correct state
    //

    if (!Initialized) {
        error = ERROR_INTERNET_SHUTDOWN;
        goto quit;
    }

    error = ERROR_INVALID_HANDLE;

    //
    // error if 0x00cc0000 > Handle > 0x00ccfffc
    //

    if ((PtrToUlong(Handle) < MINIMUM_HANDLE_VALUE)
     || (PtrToUlong(Handle) > MAXIMUM_HANDLE_VALUE)) {
        goto quit;
    }

    //
    // get the map DWORD index and bit mask from the handle
    //
    DWORD index;

    index = (PtrToUlong(Handle) - MINIMUM_HANDLE_VALUE) / (BITS_IN_DWORD * HANDLE_INCREMENT);

    DWORD bit;

    bit = 1 << (((PtrToUlong(Handle) - MINIMUM_HANDLE_VALUE) / HANDLE_INCREMENT) & (BITS_IN_DWORD - 1));

    DWORD mapIndex;

    mapIndex = (PtrToUlong(Handle) - MINIMUM_HANDLE_VALUE) / HANDLE_INCREMENT;

    EnterCriticalSection(&HandleMapCritSec);

    //
    // the index may be in range, but greater than the currently allocated
    // map length, in which case its an error
    //

    if (index < HandleMapLength) {
        if (HandleMap != NULL) {
            if (HandleMap[index] & bit) {

                BOOL recalc = FALSE;

                DEBUG_PRINT(HANDLE,
                            INFO,
                            ("handle = %#x, index = %d, address = %#x\n",
                            Handle,
                            mapIndex,
                            MapArray[mapIndex]
                            ));

                HandleMap[index] &= ~bit;
                MapArray[mapIndex] = NULL;

                error = ERROR_SUCCESS;

                //
                // if we have cleared a bit lower in the bitmap than the current
                // index and bit indicators, then reset the indicators to the
                // new position
                //

                if (index < NextHandleMapDword) {

                    //
                    // new DWORD is lower than current: change all variables
                    // to those for this handle
                    //

                    NextHandleMapDword = index;
                    recalc = TRUE;
                } else if ((index == NextHandleMapDword) && (bit < NextHandleMapBit)) {

                    //
                    // same DWORD index, lower bit position
                    //

                    recalc = TRUE;
                }
                if (recalc) {
                    NextHandleMapBit = bit;
                    NextHandleMapBitIndex = BitToIndex(bit);

                    //
                    // recalculate the next handle value
                    //

                    NextHandleValue = NEXT_HANDLE_VALUE(NextHandleMapDword,
                                                        NextHandleMapBitIndex
                                                        );

                    //
                    // and map array index
                    //

                    NextHandleIndex = (NextHandleMapDword * BITS_IN_DWORD)
                                    + NextHandleMapBitIndex;
                }
            } else {

                DEBUG_PRINT(HANDLE,
                            ERROR,
                            ("Handle = %#x. HandleMap[%d].%#x not set\n",
                            Handle,
                            index,
                            bit
                            ));
            }
        } else {

            //
            // don't ever expect this to happen
            //

            error = ERROR_INTERNET_INTERNAL_ERROR;

            INET_ASSERT(FALSE);

        }
    } else {

        DEBUG_PRINT(HANDLE,
                    ERROR,
                    ("Handle = %#x, index = %d, HandleMapLength = %d\n",
                    Handle,
                    index,
                    HandleMapLength
                    ));

    }

    LeaveCriticalSection(&HandleMapCritSec);

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
MapHandleToAddress(
    IN HINTERNET Handle,
    OUT LPVOID * lpAddress,
    IN BOOL Invalidate
    )

/*++

Routine Description:

    Given a handle, retrieve its associated address from the map array. The
    handle object represented by Handle is referenced

    Assumes:    1. only HINTERNETs visible at the API are presented to this
                   function. Even though we AllocateHandle() for arbitrary
                   objects (e.g. gopher views) we never map their addresses

Arguments:

    Handle      - handle value generated by AllocateHandle()

    lpAddress   - place to store mapped address. If the handle has been closed
                  and unmapped, NULL is returned. If the handle is still
                  mapped, even though it has been invalidated, its address will
                  be returned, and its reference count incremented

    Invalidate  - TRUE if we are invalidating this handle

Return Value:

    LPVOID
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE
                    if *lpAddress == NULL then the handle has been closed and
                    unmapped, else it is still mapped, but invalidated. In
                    this case, we incremented the reference count

--*/

{
    DEBUG_ENTER((DBG_HANDLE,
                Dword,
                "MapHandleToAddress",
                "%#x, %#x, %B",
                Handle,
                lpAddress,
                Invalidate
                ));

    LPVOID address = NULL;
    DWORD error = ERROR_INVALID_HANDLE;

    //
    // error if 0x00cc0000 > Handle > 0x00ccfffc
    //

    if ((PtrToUlong(Handle) >= MINIMUM_HANDLE_VALUE)
    &&  (PtrToUlong(Handle) <= MAXIMUM_HANDLE_VALUE)
    && !InDllCleanup) {

        DWORD index = (PtrToUlong(Handle) - MINIMUM_HANDLE_VALUE) / HANDLE_INCREMENT;

        //
        // the caller could have supplied a value which has the correct range
        // but may not yet have been generated, causing us to index past the
        // end of the array
        //

        if (index < MapArrayLength) {

            //
            // we have to acquire the critical section in case another thread
            // is reallocating the array
            //

            EnterCriticalSection(&HandleMapCritSec);

            address = MapArray[index];

            DEBUG_PRINT(HANDLE,
                        INFO,
                        ("Handle %#x mapped to address %#x\n",
                        Handle,
                        address
                        ));

            if (address != NULL) {

                //
                // although we store addresses of arbitrary structures (e.g.
                // FTP_SESSION_INFO), we are only calling this function to map
                // pseudo-handles to object addresses at the API. Therefore it
                // should be safe to assume that the pointer references a handle
                // object
                // However, there's nothing to stop an app passing in a random
                // handle value that just happens to map to an FTP or gopher
                // session or gopher view, and since we don't want to treat that
                // as a handle object, we must make this test full-time
                //

                if (((HANDLE_OBJECT *)address)->IsValid(TypeWildHandle) == ERROR_SUCCESS) {

                    //
                    // this is also a very good time to increment the reference
                    // count. We are using the fact that we are serialized on the
                    // handle map critical section here. If Reference() returns
                    // ERROR_INVALID_HANDLE then the handle object has been
                    // invalidated, but its reference count will have been
                    // incremented. The caller should perform as little work as
                    // necessary and get out.
                    //
                    // If Refrerence() returns ERROR_ACCESS_DENIED, then the object
                    // is being destroyed (refcount already went to zero).
                    //
                    // If the reference count is incremented to 1 then there is
                    // another thread waiting to finish deleting this handle. It
                    // is virtually deleted, and if we return its address, the
                    // caller will have a deleted object
                    //

                    if (((HANDLE_OBJECT *)address)->ReferenceCount() == 0) {

                        DEBUG_PRINT(HANDLE,
                                    ERROR,
                                    ("handle %#x [%#x] about to be deleted\n",
                                    Handle,
                                    address
                                    ));

                        address = NULL;
                    } else {
                        error = ((HANDLE_OBJECT *)address)->Reference();
                        if (error == ERROR_SUCCESS) {
                            if (Invalidate) {

                                //
                                // we were called from a handle close API.
                                // Subsequent API calls will discover that the
                                // handle is already invalidated and will quit
                                //

                                ((HANDLE_OBJECT *)address)->Invalidate();
                            }
                        } else if (error == ERROR_ACCESS_DENIED) {
                            //
                            // if we get ERROR_ACCESS_DENIED, this means that the object is 
                            // being destructed, so we *have* to return NULL.
                            //
                            DEBUG_PRINT(HANDLE,
                                        ERROR,
                                        ("Reference() failed - handle %#x [%#x] about to be deleted\n",
                                        Handle,
                                        address
                                        ));
                            address = NULL;
                        } else {
                            DEBUG_PRINT(HANDLE,
                                        ERROR,
                                        ("Reference() returns %d\n",
                                        error
                                        ));

                            //
                            // if invalid and reference count already zero, we
                            // didn't increment ref count: handle already being
                            // deleted
                            //

                            if (((HANDLE_OBJECT *)address)->ReferenceCount() == 0) {
                                address = NULL;
                            }
                        }
                    }
                } else {

                    //
                    // we still want to know about it in debug version
                    //

                    DEBUG_PRINT(HANDLE,
                                ERROR,
                                ("invalid handle object: %#x [%#x]\n",
                                Handle,
                                address
                                ));

                    IF_DEBUG(INVALID_HANDLES) {
                        //INET_ASSERT(FALSE);
                    }
                }
            } else {

                //
                // lets also catch this one (NULL address)
                //

                DEBUG_PRINT(HANDLE,
                            ERROR,
                            ("NULL handle: %#x\n",
                            Handle
                            ));

                IF_DEBUG(INVALID_HANDLES) {
                    //INET_ASSERT(FALSE);
                }
            }
            LeaveCriticalSection(&HandleMapCritSec);
        }
    } else if (InDllCleanup) {
        error = ERROR_INTERNET_SHUTDOWN;
    } else {

        DEBUG_PRINT(HANDLE,
                    ERROR,
                    ("bad handle value: %#x\n",
                    Handle
                    ));

        IF_DEBUG(INVALID_HANDLES) {
            //INET_ASSERT(FALSE);
        }
    }

    *lpAddress = address;

    DEBUG_LEAVE(error);

    return error;
}


DWORD
DereferenceObject(
    IN LPVOID lpObject
    )

/*++

Routine Description:

    Undoes the reference added to the handle object by MapHandleToAddress(). May
    result in the handle object being deleted

Arguments:

    lpObject    - address of object to dereference. This MUST be the mapped
                  object address as returned by MapHandleToAddress()

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    The handle object was destroyed

        Failure - ERROR_INVALID_HANDLE
                    The object was not a valid handle

                  ERROR_INTERNET_HANDLE_EXISTS
                    The handle is still alive

--*/

{
    DEBUG_ENTER((DBG_HANDLE,
                Dword,
                "DereferenceObject",
                "%#x",
                lpObject
                ));

    INET_ASSERT(lpObject != NULL);

    HANDLE_OBJECT * object = (HANDLE_OBJECT *)lpObject;
    DWORD error = object->IsValid(TypeWildHandle);

    if (error == ERROR_SUCCESS) {
        if (!object->Dereference()) {
            error = ERROR_INTERNET_HANDLE_EXISTS;
        }
    } else {

        //
        // IsValid() should never return an error if the reference counts
        // are correct
        //

        INET_ASSERT(FALSE);

    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
inline
DWORD
BitToIndex(
    IN DWORD Bit
    )

/*++

Routine Description:

    Returns the index of the first bit set in a DWORD

Arguments:

    Bit - bitmap

Return Value:

    DWORD   - 0..(BITS_IN_DWORD - 1) if bit found, else BITS_IN_DWORD

--*/

{
    if (Bit == 0) {
        return BITS_IN_DWORD;
    } else {

        DWORD index;
        DWORD testBit;

        for (index = 0, testBit = 1; !(Bit & testBit); ++index, testBit <<= 1) {
            ;
        }
        return index;
    }
}
