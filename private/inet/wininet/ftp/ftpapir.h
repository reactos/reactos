/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ftpapir.h

Abstract:

    Prototypes etc. for ftpapir.c

Author:

    Richard L Firth (rfirth) 09-Mar-1995

Revision History:

    09-Mar-1995 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// prototypes
//

DWORD
wFtpFindFirstFile(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszSearchFile,
    OUT LPWIN32_FIND_DATA lpFindFileData OPTIONAL,
    OUT LPHINTERNET lphInternet
    );

DWORD
wFtpDeleteFile(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszFileName
    );

DWORD
wFtpRenameFile(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszExisting,
    IN LPCSTR lpszNew
    );

DWORD
wFtpOpenFile(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    OUT LPHINTERNET lphInternet
    );

DWORD
wFtpCreateDirectory(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszDirectory
    );

DWORD
wFtpRemoveDirectory(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszDirectory
    );

DWORD
wFtpSetCurrentDirectory(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszDirectory
    );

DWORD
wFtpGetCurrentDirectory(
    IN HINTERNET hFtpSession,
    IN DWORD cchCurrentDirectory,
    OUT LPSTR lpszCurrentDirectory,
    OUT LPDWORD lpdwBytesReturned
    );

DWORD
wFtpCommand(
    IN HINTERNET hFtpSession,
    IN BOOL fExpectResponse,
    IN DWORD dwTransferType,
    IN LPCSTR lpszCommand
    );

DWORD
wFtpFindNextFile(
    IN HINTERNET hFtpSession,
    OUT LPWIN32_FIND_DATA lpFindFileData
    );

DWORD
wFtpFindClose(
    IN HINTERNET hFtpSession
    );

DWORD
wFtpReadFile(
    IN HINTERNET hFtpSession,
    IN LPVOID lpBuffer,
    IN DWORD nNumberOfBytesToRead,
    OUT LPDWORD lpNumberOfBytesRead
    );

DWORD
wFtpWriteFile(
    IN HINTERNET hFtpSession,
    IN LPVOID lpBuffer,
    IN DWORD nNumberOfBytesToWrite,
    OUT LPDWORD lpNumberOfBytesWritten
    );

DWORD
wFtpCloseFile(
    IN HINTERNET hFtpSession
    );

DWORD
wFtpGetFileSize(
    IN  HINTERNET hMappedFtpSession,
    IN  LPFTP_SESSION_INFO lpSessionInfo,
    OUT LPDWORD lpdwFileSizeLow,
    OUT LPDWORD lpdwFileSizeHigh
    );

DWORD
wFtpFindServerType(
    IN HINTERNET hFtpSession
    );

HINTERNET
InternalFtpFindFirstFileA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszSearchFile OPTIONAL,
    OUT LPWIN32_FIND_DATA lpFindFileData OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    IN BOOL  fCacheOnly,
    IN BOOL  fAllowEmpty = FALSE
    );

HINTERNET
InternalFtpOpenFileA(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    IN BOOL fCacheOnly
    );

#if defined(__cplusplus)
}
#endif
