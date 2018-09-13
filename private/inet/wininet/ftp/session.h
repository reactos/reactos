/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    session.h

Abstract:

    Structures, prototypes for session.c

Author:

    Heath Hunnicutt (t-hheath) 21-Jun-1994

Revision History:

    21-Jun-1994 t-heathh
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// manifests
//

#define FTP_SESSION_SIGNATURE   0x53707446  // "FtpS" (when viewed via db/dc)

//
// macros
//

#if INET_DEBUG

#define SetSessionSignature(lpSessionInfo)  \
    (lpSessionInfo)->Signature = FTP_SESSION_SIGNATURE

#else

#define SetSessionSignature(lpSessionInfo)

#endif

#define SetSessionLastResponseCode(pSession, prc) \
    CopyMemory(&((pSession)->rcResponseOpenFile), (prc), sizeof(FTP_RESPONSE_CODE))

#define GetSessionLastResponseCode(pSession, prc) \
    CopyMemory((prc), &((pSession)->rcResponseOpenFile), sizeof(FTP_RESPONSE_CODE))

#define IsPassiveModeSession(lpSessionInfo) \
    (((lpSessionInfo)->Flags & FFTP_PASSIVE_MODE) ? TRUE : FALSE)

//
// types
//

typedef enum {
    FTP_SERVER_TYPE_UNKNOWN = 0,
    FTP_SERVER_TYPE_NT = 1,
    FTP_SERVER_TYPE_UNIX = 2
} FTP_SERVER_TYPE;

//
// FTP_SESSION_INFO - describes an FTP server and our connection to it
//

typedef struct {

    //
    // List - SESSION_INFOs are maintained on double-linked list
    //

    LIST_ENTRY List;

    //
    // Host - name of the server we are connected to. We only need this for
    // diagnositic purposes - e.g. knowing which server to talk to to
    // reproduce a problem
    //

    LPSTR Host;

    //
    // Port - the port at which the FTP server listens
    //

    INTERNET_PORT Port;

    //
    // socketListener - listening socket
    //

    ICSocket *socketListener;


    //
    // socketControl - control connection socket
    //

    ICSocket *socketControl;

    //
    // socketData - data connection socket
    //

    ICSocket *socketData;

    //
    // ServerType - type of FTP server, e.g. NT or *nix
    //

    FTP_SERVER_TYPE ServerType;

    //
    // Handle - internally identifies this FTP session
    //

    HANDLE Handle;

    //
    // Flags - bitmask of various flags - see below
    //

    DWORD Flags;

    //
    // ReferenceCount - keeps object alive whilst we are not holding
    // CriticalSection
    //

    LONG ReferenceCount;

    //
    // dwTransferAccess - Indicates, for an ongoing transfer, whether the
    // transfer was begun with GENERIC_READ or GENERIC_WRITE access.
    //
    // {dwTransferAccess} = {GENERIC_READ, GENERIC_WRITE}
    //

    DWORD dwTransferAccess;

    //
    // rcResponseOpenFile - The response code sent back when a data connection
    // was opened, either by FtpOpenFile or FtpCommand.
    //
    // Used by FtpCloseFile to determine whether the completion
    // code was already received.
    //

    FTP_RESPONSE_CODE rcResponseOpenFile;

    //
    // FindFileList - A linked-list of WIN32_FIND_DATA structures, formed by a
    // call to FtpFindFirstFile, used by FtpFindNextFile and
    // FtpFindClose.
    //

    LIST_ENTRY FindFileList;

    //
    // CriticalSection - Synchronize access to this structure's contents
    //

    CRITICAL_SECTION CriticalSection;

    //
    // dwFileSizeLow - Size of the file found on the FTP server, should be gotten
    //   from response data on openning a data connection
    //

    DWORD dwFileSizeLow;
    DWORD dwFileSizeHigh;

#if INET_DEBUG

    //
    // Signature - to help us know this is what its supposed to be in debug build
    //

    DWORD Signature;

#endif

} FTP_SESSION_INFO, *LPFTP_SESSION_INFO;

//
// Flags defines
//

//
// FFTP_PASSIVE_MODE - set if the session uses passive mode data connections
//

#define FFTP_PASSIVE_MODE       0x00000001

//
// FFTP_ABORT_TRANSFER - set if we have not completed a file transfer on this
// (data) connection, and therefore need to send an ABOR command when we close
// the connection
//

#define FFTP_ABORT_TRANSFER     0x00000002

//
// FFTP_FIND_ACTIVE - set when a directory listing is active on this session
//

#define FFTP_FIND_ACTIVE        0x00000004

//
// FFTP_IN_DESTRUCTOR - set when this session is being terminated
//

#define FFTP_IN_DESTRUCTOR      0x00000008

//
// FFTP_EOF - set when we have reached the end of a (receive) data connection
//

#define FFTP_EOF                0x00000010

//
// FFTP_FILE_ACTIVE - set when a file is open on this session
//

#define FFTP_FILE_ACTIVE        0x00000020

//
// FFTP_KNOWN_FILE_SIZE - set when we know the size of the file we're downloading
//

#define FFTP_KNOWN_FILE_SIZE    0x00000040

//
// prototypes
//

VOID
CleanupFtpSessions(
    VOID
    );

VOID
TerminateFtpSession(
    IN LPFTP_SESSION_INFO SessionInfo
    );

VOID
DereferenceFtpSession(
    IN LPFTP_SESSION_INFO SessionInfo
    );

DWORD
CreateFtpSession(
    IN LPSTR lpszHost,
    IN INTERNET_PORT Port,
    IN DWORD dwFlags,
    OUT LPFTP_SESSION_INFO* lpSessionInfo
    );

BOOL
FindFtpSession(
    IN HANDLE Handle,
    OUT LPFTP_SESSION_INFO* lpSessionInfo
    );

#if INET_DEBUG

VOID
FtpSessionInitialize(
    VOID
    );

VOID
FtpSessionTerminate(
    VOID
    );

VOID
AcquireFtpSessionList(
    VOID
    );

VOID
ReleaseFtpSessionList(
    VOID
    );

VOID
AcquireFtpSessionLock(
    IN LPFTP_SESSION_INFO SessionInfo
    );

VOID
ReleaseFtpSessionLock(
    IN LPFTP_SESSION_INFO SessionInfo
    );

#else

//
// one-line functions replaced by macros in retail version
//

extern SERIALIZED_LIST FtpSessionList;

#define FtpSessionInitialize() \
    InitializeSerializedList(&FtpSessionList)

#define FtpSessionTerminate() \
    TerminateSerializedList(&FtpSessionList)

#define AcquireFtpSessionList() \
    LockSerializedList(&FtpSessionList)

#define ReleaseFtpSessionList() \
    UnlockSerializedList(&FtpSessionList)

#define AcquireFtpSessionLock(lpSessionInfo) \
    EnterCriticalSection(&lpSessionInfo->CriticalSection)

#define ReleaseFtpSessionLock(lpSessionInfo) \
    LeaveCriticalSection(&lpSessionInfo->CriticalSection)

#endif // INET_DEBUG

#if defined(__cplusplus)
}
#endif
