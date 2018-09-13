/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    protocol.h

Abstract:

    Prototypes, etc. for protocol.c

Author:

    Richard L Firth (rfirth) 16-Mar-1995

Revision History:

    16-Mar-1995
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// prototypes
//

DWORD
Command(
    IN OUT LPFTP_SESSION_INFO lpSessionInfo,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN OUT FTP_RESPONSE_CODE * prcResponse,
    IN LPCSTR lpszCommandFormat,
    IN ...
    );

DWORD
I_Command(
    IN LPFTP_SESSION_INFO lpSessionInfo,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN FTP_RESPONSE_CODE * prcResponse,
    IN LPCSTR lpszCommandFormat,
    IN va_list arglist
    );

DWORD
__cdecl
NegotiateDataConnection(
    IN LPFTP_SESSION_INFO lpSessionInfo,
    IN DWORD dwFlags,
    OUT FTP_RESPONSE_CODE * prcResponse,
    IN LPCSTR lpszCommandFormat,
    IN ...
    );

DWORD
GetReply(
    IN LPFTP_SESSION_INFO lpSessionInfo,
    OUT FTP_RESPONSE_CODE * prcResponse
    );

DWORD
ReceiveFtpResponse(
    IN ICSocket * Socket,
    OUT LPVOID * lpBuffer,
    OUT LPDWORD lpdwBufferLength,
    IN BOOL bEndOfLineCheck,
    IN FTP_RESPONSE_CODE * prcResponse
    );

DWORD
AbortTransfer(
    IN LPFTP_SESSION_INFO lpSessionInfo
    );

#if defined(__cplusplus)
}
#endif
