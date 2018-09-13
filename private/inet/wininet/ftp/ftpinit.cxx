/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ftpinit.cxx

Abstract:

    FTP package-specific initialization & termination. Used to be libmain.c

    Contents:
        FtpInitialize
        FtpTerminate

Author:

    Richard L Firth (rfirth) 09-Jun-1995

Environment:

    Win32 user-mode

Revision History:

    09-Jun-1995 rfirth
        Created

--*/

#include <wininetp.h>
#include "ftpapih.h"

//
// functions
//


VOID
FtpInitialize(
    VOID
    )

/*++

Routine Description:

    Performs FTP-specific initialization

Arguments:

    None.

Return Value:

    None.

--*/

{
    FtpSessionInitialize();
}


VOID
FtpTerminate(
    VOID
    )

/*++

Routine Description:

    Performs FTP-specific termination/cleanup

Arguments:

    None.

Return Value:

    None.

--*/

{
    CleanupFtpSessions();
    FtpSessionTerminate();
}
