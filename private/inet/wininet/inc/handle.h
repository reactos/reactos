/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    handle.h

Abstract:

    Header file for common\handle.cxx

Author:

    Richard L Firth (rfirth) 01-Nov-1994

Revision History:

    01-Nov-1994 rfirth
        Created

--*/

//
// prototypes
//

#if defined(__cplusplus)
extern "C" {
#endif

DWORD
HandleInitialize(
    VOID
    );

VOID
HandleTerminate(
    VOID
    );

DWORD
AllocateHandle(
    IN LPVOID Address,
    OUT LPHINTERNET lpHandle
    );

DWORD
FreeHandle(
    IN HINTERNET Handle
    );

DWORD
MapHandleToAddress(
    IN HINTERNET Handle,
    OUT LPVOID * lpAddress,
    IN BOOL Invalidate
    );

DWORD
DereferenceObject(
    IN LPVOID lpObject
    );

#if defined(__cplusplus)
}
#endif
