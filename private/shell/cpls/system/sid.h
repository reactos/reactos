/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    sid.h

Abstract:

    Public declarations for SID management functions

Author:

    (davidc) 26-Aug-1992

Revision History:

    17-Oct-1997 scotthal
        Split public declarations into separate header

--*/
#ifndef _SYSDM_SID_H_
#define _SYSDM_SID_H_

//
// Public function prototypes
//
LPTSTR 
GetSidString(
    void
);

VOID 
DeleteSidString(
    IN LPTSTR SidString
);

PSID 
GetUserSid(
    void
);

VOID 
DeleteUserSid(
    IN PSID Sid
);

#endif // _SYSDM_SID_H_
