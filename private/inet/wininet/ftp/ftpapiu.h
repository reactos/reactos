/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ftpapiu.h

Abstract:

    Header for ftpapiu.h

Author:

    Richard L Firth (rfirth) 31-May-1995

Revision History:

    31-May-1995 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// prototypes
//

DWORD
ParseFtpUrl(
    IN OUT LPHINTERNET hInternet,
    IN LPSTR Url,
    IN LPSTR Headers,
    IN DWORD HeadersLength,
    IN DWORD OpenFlags,
    IN DWORD Context
    );

#if defined(__cplusplus)
}
#endif
