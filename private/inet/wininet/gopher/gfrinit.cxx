/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    gfrinit.cxx

Abstract:

    Gopher protocol initialization. This used to be gfrdll.c. All DLL specific
    code has moved to internet\client\dll

    Contents:
        GopherInitialize
        GopherTerminate

Author:

    Richard L Firth (rfirth) 09-Jun-1995

Environment:

    Win32 user-mode

Revision History:

    09-Jun-1995 rfirth
        Created

--*/

#include <wininetp.h>
#include "gfrapih.h"

//
// functions
//


VOID
GopherInitialize(
    VOID
    )

/*++

Routine Description:

    Performs gopher-specific initialization

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // initialize any critical sections, lists, etc.
    //

    InitializeSerializedList(&SessionList);
}


VOID
GopherTerminate(
    VOID
    )

/*++

Routine Description:

    Performs gopher-specific termination/cleanup

Arguments:

    None.

Return Value:

    None.

--*/

{
    CleanupSessions();
    TerminateSerializedList(&SessionList);

    //
    // make sure we returned all gopher resources
    //

    ASSERT_NO_BUFFERS();
    ASSERT_NO_VIEWS();
    ASSERT_NO_SESSIONS();
}
