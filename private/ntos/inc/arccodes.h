/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    arc.h

Abstract:

    This header file defines the ARC status codes.

Author:

    David N. Cutler (davec) 20-Sep-1991


Revision History:

--*/

#ifndef _ARCCODES_
#define _ARCCODES_

//
// Define ARC status codes.
//

typedef enum _ARC_CODES {
    ESUCCESS,       //  0
    E2BIG,          //  1
    EACCES,         //  2
    EAGAIN,         //  3
    EBADF,          //  4
    EBUSY,          //  5
    EFAULT,         //  6
    EINVAL,         //  7
    EIO,            //  8
    EISDIR,         //  9
    EMFILE,         // 10
    EMLINK,         // 11
    ENAMETOOLONG,   // 12
    ENODEV,         // 13
    ENOENT,         // 14
    ENOEXEC,        // 15
    ENOMEM,         // 16
    ENOSPC,         // 17
    ENOTDIR,        // 18
    ENOTTY,         // 19
    ENXIO,          // 20
    EROFS,          // 21
    EMAXIMUM        // 22
    } ARC_CODES;

#endif // ARCCODES
