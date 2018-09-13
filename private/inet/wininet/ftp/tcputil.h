/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    tcputil.h

Abstract:

    Contains prototypes etc. for tcputil.c

Author:

    Heath Hunnicutt (t-hheath) 21-Jun-1994

Revision History:

--*/

#if defined(__cplusplus)
extern "C" {
#endif

DWORD
FtpOpenServer(
    IN LPFTP_SESSION_INFO SessionInfo
    );

BOOL
ResetSocket(
    IN ICSocket * Socket
    );

#if defined(__cplusplus)
}
#endif
