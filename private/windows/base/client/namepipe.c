/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    namepipe.c

Abstract:

    This module contains the Win32 Named Pipe API

Author:

    Colin Watson (ColinW)  13-March-1991

Revision History:

--*/

#include "basedll.h"

#define DOS_LOCAL_PIPE_PREFIX   L"\\\\.\\pipe\\"
#define DOS_LOCAL_PIPE          L"\\DosDevices\\pipe\\"
#define DOS_REMOTE_PIPE         L"\\DosDevices\\UNC\\"

#define INVALID_PIPE_MODE_BITS  ~(PIPE_READMODE_BYTE    \
                                | PIPE_READMODE_MESSAGE \
                                | PIPE_WAIT             \
                                | PIPE_NOWAIT)
BOOL
NpGetUserNamep(
    HANDLE hNamedPipe,
    LPWSTR lpUserName,
    DWORD nMaxUserNameSize
    );

typedef
BOOL (WINAPI *REVERTTOSELF)( VOID );

typedef
BOOL (WINAPI *GETUSERNAMEW)( LPWSTR, LPDWORD );

typedef
BOOL (WINAPI *IMPERSONATENAMEDPIPECLIENT)( HANDLE );

HANDLE
APIENTRY
CreateNamedPipeA(
    LPCSTR lpName,
    DWORD dwOpenMode,
    DWORD dwPipeMode,
    DWORD nMaxInstances,
    DWORD nOutBufferSize,
    DWORD nInBufferSize,
    DWORD nDefaultTimeOut,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++
    Ansi thunk to CreateNamedPipeW.

--*/
{
    NTSTATUS Status;
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString,lpName);
    Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return INVALID_HANDLE_VALUE;
        }

    return CreateNamedPipeW(
            (LPCWSTR)Unicode->Buffer,
            dwOpenMode,
            dwPipeMode,
            nMaxInstances,
            nOutBufferSize,
            nInBufferSize,
            nDefaultTimeOut,
            lpSecurityAttributes);
}

HANDLE
APIENTRY
CreateNamedPipeW(
    LPCWSTR lpName,
    DWORD dwOpenMode,
    DWORD dwPipeMode,
    DWORD nMaxInstances,
    DWORD nOutBufferSize,
    DWORD nInBufferSize,
    DWORD nDefaultTimeOut,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )

/*++


Parameters:

    lpName --Supplies the pipe name Documented in "Pipe Names" section
        earlier.  This must be a local name.

    dwOpenMode --Supplies the set of flags that define the mode which the
        pipe is to be opened with.  The open mode consists of access
        flags (one of three values) logically ORed with a writethrough
        flag (one of two values) and an overlapped flag (one of two
        values), as described below.

        dwOpenMode Flags:

        PIPE_ACCESS_DUPLEX --Pipe is bidirectional.  (This is
            semantically equivalent to calling CreateFile with access
            flags of GENERIC_READ | GENERIC_WRITE.)

        PIPE_ACCESS_INBOUND --Data goes from client to server only.
            (This is semantically equivalent to calling CreateFile with
            access flags of GENERIC_READ.)

        PIPE_ACCESS_OUTBOUND --Data goes from server to client only.
            (This is semantically equivalent to calling CreateFile with
            access flags of GENERIC_WRITE.)

        PIPE_WRITETHROUGH --The redirector is not permitted to delay the
            transmission of data to the named pipe buffer on the remote
            server. This disables a performance enhancement for
            applications that need synchronization with every write
            operation.

        FILE_FLAG_OVERLAPPED --Indicates that the system should
            initialize the file so that ReadFile, WriteFile and other
            operations that may take a significant time to process will
            return ERROR_IO_PENDING. An event will be set to the
            signalled state when the operation completes.

        FILE_FLAG_WRITETHROUGH -- No intermediate buffering.

        WRITE_DAC --            Standard security desired access
        WRITE_OWNER --          ditto
        ACCESS_SYSTEM_SECURITY -- ditto

    dwPipeMode --Supplies the pipe-specific modes (as flags) of the pipe.
        This parameter is a combination of a read-mode flag, a type flag,
        and a wait flag.

        dwPipeMode Flags:

        PIPE_WAIT --Blocking mode is to be used for this handle.

        PIPE_NOWAIT --Nonblocking mode is to be used for this handle.

        PIPE_READMODE_BYTE --Read pipe as a byte stream.

        PIPE_READMODE_MESSAGE --Read pipe as a message stream.  Note that
            this is not allowed with PIPE_TYPE_BYTE.

        PIPE_TYPE_BYTE --Pipe is a byte-stream pipe.  Note that this is
            not allowed with PIPE_READMODE_MESSAGE.

        PIPE_TYPE_MESSAGE --Pipe is a message-stream pipe.

    nMaxInstances --Gives the maximum number of instances for this pipe.
        Acceptable values are 1 to PIPE_UNLIMITED_INSTANCES-1 and
        PIPE_UNLIMITED_INSTANCES.

        nMaxInstances Special Values:

        PIPE_UNLIMITED_INSTANCES --Unlimited instances of this pipe can
            be created.

    nOutBufferSize --Specifies an advisory on the number of bytes to
        reserve for the outgoing buffer.

    nInBufferSize --Specifies an advisory on the number of bytes to
        reserve for the incoming buffer.

    nDefaultTimeOut -- Specifies an optional pointer to a timeout value
        that is to be used if a timeout value is not specified when
        waiting for an instance of a named pipe. This parameter is only
        meaningful when the first instance of a named pipe is created. If
        neither CreateNamedPipe or WaitNamedPipe specify a timeout 50
        milliseconds will be used.

    lpSecurityAttributes --An optional parameter that, if present and
        supported on the target system, supplies a security descriptor
        for the named pipe.  This parameter includes an inheritance flag
        for the handle.  If this parameter is not present, the handle is
        not inherited by child processes.

Return Value:

    Returns one of the following:

    INVALID_HANDLE_VALUE --An error occurred.  Call GetLastError for more
    information.

    Anything else --Returns a handle for use in the server side of
    subsequent named pipe operations.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    LARGE_INTEGER Timeout;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    LPWSTR FilePart;
    ULONG CreateFlags;
    ULONG DesiredAccess;
    ULONG ShareAccess;
    ULONG MaxInstances;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL DefaultAcl = NULL;

    if ((nMaxInstances == 0) ||
        (nMaxInstances > PIPE_UNLIMITED_INSTANCES)) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
        }

    // Convert Win32 maximum Instances to Nt maximum instances.
    MaxInstances = (nMaxInstances == PIPE_UNLIMITED_INSTANCES)?
        0xffffffff : nMaxInstances;


    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpName,
                            &FileName,
                            &FilePart,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
        Obja.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        if ( lpSecurityAttributes->bInheritHandle ) {
            Obja.Attributes |= OBJ_INHERIT;
            }
        }

    if (Obja.SecurityDescriptor == NULL) {

        //
        // Apply default security if none specified (bug 131090)
        //

        Status = RtlDefaultNpAcl( &DefaultAcl );
        if (NT_SUCCESS( Status )) {
            RtlCreateSecurityDescriptor( &SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );
            RtlSetDaclSecurityDescriptor( &SecurityDescriptor, TRUE, DefaultAcl, FALSE );
            Obja.SecurityDescriptor = &SecurityDescriptor;
        } else {
            RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
            BaseSetLastNTError(Status);
            return INVALID_HANDLE_VALUE;
        }
    }

    //  End of code common with fileopcr.c CreateFile()

    CreateFlags = (dwOpenMode & FILE_FLAG_WRITE_THROUGH ? FILE_WRITE_THROUGH : 0 );
    CreateFlags |= (dwOpenMode & FILE_FLAG_OVERLAPPED ? 0 : FILE_SYNCHRONOUS_IO_NONALERT);

    //
    //  Determine the timeout. Convert from milliseconds to an Nt delta time
    //

    if ( nDefaultTimeOut ) {
        Timeout.QuadPart = - (LONGLONG)UInt32x32To64( 10 * 1000, nDefaultTimeOut );
        }
    else {
        //  Default timeout is 50 Milliseconds
        Timeout.QuadPart =  -10 * 1000 * 50;
        }

    //  Check no reserved bits are set by mistake.

    if (( dwOpenMode & ~(PIPE_ACCESS_DUPLEX |
                         FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH |
                         WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY ))||

        ( dwPipeMode & ~(PIPE_NOWAIT | PIPE_READMODE_MESSAGE |
                         PIPE_TYPE_MESSAGE ))) {

            RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
            if (DefaultAcl != NULL) {
                RtlFreeHeap(RtlProcessHeap(),0,DefaultAcl);
            }
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }

    //
    //  Translate the open mode into a sharemode to restrict the clients access
    //  and derive the appropriate local desired access.
    //

    switch ( dwOpenMode & PIPE_ACCESS_DUPLEX ) {
        case PIPE_ACCESS_INBOUND:
            ShareAccess = FILE_SHARE_WRITE;
            DesiredAccess = GENERIC_READ;
            break;

        case PIPE_ACCESS_OUTBOUND:
            ShareAccess = FILE_SHARE_READ;
            DesiredAccess = GENERIC_WRITE;
            break;

        case PIPE_ACCESS_DUPLEX:
            ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
            DesiredAccess = GENERIC_READ | GENERIC_WRITE;
            break;

        default:
            RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            if (DefaultAcl != NULL) {
                RtlFreeHeap(RtlProcessHeap(),0,DefaultAcl);
            }
            return INVALID_HANDLE_VALUE;
        }

    DesiredAccess |= SYNCHRONIZE |
         ( dwOpenMode & (WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY ));

    Status = NtCreateNamedPipeFile (
        &Handle,
        DesiredAccess,
        &Obja,
        &IoStatusBlock,
        ShareAccess,
        FILE_OPEN_IF,                   // Create first instance or subsequent
        CreateFlags,                    // Create Options
        dwPipeMode & PIPE_TYPE_MESSAGE ?
            FILE_PIPE_MESSAGE_TYPE : FILE_PIPE_BYTE_STREAM_TYPE,
        dwPipeMode & PIPE_READMODE_MESSAGE ?
            FILE_PIPE_MESSAGE_MODE : FILE_PIPE_BYTE_STREAM_MODE,
        dwPipeMode & PIPE_NOWAIT ?
            FILE_PIPE_COMPLETE_OPERATION : FILE_PIPE_QUEUE_OPERATION,
        MaxInstances,                   // Max instances
        nInBufferSize,                  // Inbound quota
        nOutBufferSize,                 // Outbound quota
        (PLARGE_INTEGER)&Timeout
        );

    if ( Status == STATUS_NOT_SUPPORTED ||
         Status == STATUS_INVALID_DEVICE_REQUEST ) {

        //
        // The request must have been processed by some other device driver
        // (other than NPFS).  Map the error to something reasonable.
        //

        Status = STATUS_OBJECT_NAME_INVALID;
    }

    RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
    if (DefaultAcl != NULL) {
        RtlFreeHeap(RtlProcessHeap(),0,DefaultAcl);
    }
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
        }

    return Handle;
}

BOOL
APIENTRY
ConnectNamedPipe(
    HANDLE hNamedPipe,
    LPOVERLAPPED lpOverlapped
    )

/*++

Routine Description:

    The ConnectNamedPipe function is used by the server side of a named pipe
    to wait for a client to connect to the named pipe with a CreateFile
    request. The handle provided with the call to ConnectNamedPipe must have
    been previously returned by a successful call to CreateNamedPipe. The pipe
    must be in the disconnected, listening or connected states for
    ConnectNamedPipe to succeed.

    The behavior of this call depends on the blocking/nonblocking mode selected
    with the PIPE_WAIT/PIPE_NOWAIT flags when the server end of the pipe was
    created with CreateNamedPipe.

    If blocking mode is specified, ConnectNamedPipe will change the state from
    disconnected to listening and block. When a client connects with a
    CreateFile, the state will be changed from listening to connected and the
    ConnectNamedPipe returns TRUE. When the file handle is created with
    FILE_FLAG_OVERLAPPED on a blocking mode pipe, the lpOverlapped parameter
    can be specified. This allows the caller to continue processing while the
    ConnectNamedPipe API awaits a connection. When the pipe enters the
    signalled state the event is set to the signalled state.

    When nonblocking is specified ConnectNamedPipe will not block. On the
    first call the state will change from disconnected to listening. When a
    client connects with an Open the state will be changed from listening to
    connected. The ConnectNamedPipe will return FALSE (with GetLastError
    returning ERROR_PIPE_LISTENING) until the state is changed to the listening
    state.

Arguments:

    hNamedPipe - Supplies a Handle to the server side of a named pipe.

    lpOverlapped - Supplies an overlap structure to be used with the request.
        If NULL then the API will not return until the operation completes. When
        FILE_FLAG_OVERLAPPED is specified when the handle was created,
        ConnectNamedPipe may return ERROR_IO_PENDING to allow the caller to
        continue processing while the operation completes. The event (or File
        handle if hEvent=NULL) will be set to the not signalled state before
        ERROR_IO_PENDING is returned. The event will be set to the signalled
        state upon completion of the request. GetOverlappedResult is used to
        determine the error status.

Return Value:

    TRUE -- The operation was successful, the pipe is in the
        connected state.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    if ( lpOverlapped ) {
        lpOverlapped->Internal = (DWORD)STATUS_PENDING;
        }
    Status = NtFsControlFile(
                hNamedPipe,
                (lpOverlapped==NULL)? NULL : lpOverlapped->hEvent,
                NULL,   // ApcRoutine
                lpOverlapped ? ((ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped) : NULL,
                (lpOverlapped==NULL) ? &Iosb : (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                FSCTL_PIPE_LISTEN,
                NULL,   // InputBuffer
                0,      // InputBufferLength,
                NULL,   // OutputBuffer
                0       // OutputBufferLength
                );

    if ( lpOverlapped == NULL && Status == STATUS_PENDING) {
        // Operation must complete before return & Iosb destroyed
        Status = NtWaitForSingleObject( hNamedPipe, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {
            Status = Iosb.Status;
            }
        }

    if (NT_SUCCESS( Status ) && Status != STATUS_PENDING ) {
        return TRUE;
        }
    else
        {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
APIENTRY
DisconnectNamedPipe(
    HANDLE hNamedPipe
    )

/*++

Routine Description:

    The DisconnectNamedPipe function can be used by the server side of a named
    pipe to force the client side to close the client side's handle. (Note that
    the client side must still call CloseFile to do this.)  The client will
    receive an error the next time it attempts to access the pipe. Disconnecting
    the pipe may cause data to be lost before the client reads it. (If the
    application wants to make sure that data is not lost, the serving side
    should call FlushFileBuffers before calling DisconnectNamedPipe.)

Arguments:

    hNamedPipe - Supplies a Handle to the server side of a named pipe.

Return Value:

    TRUE -- The operation was successful, the pipe is in the disconnected state.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    Status = NtFsControlFile(
                        hNamedPipe,
                        NULL,
                        NULL,   // ApcRoutine
                        NULL,   // ApcContext
                        &Iosb,
                        FSCTL_PIPE_DISCONNECT,
                        NULL,   // InputBuffer
                        0,      // InputBufferLength,
                        NULL,   // OutputBuffer
                        0       // OutputBufferLength
                        );

    if ( Status == STATUS_PENDING) {
        // Operation must complete before return & Iosb destroyed
        Status = NtWaitForSingleObject( hNamedPipe, FALSE, NULL );
        if ( NT_SUCCESS(Status)) {
            Status = Iosb.Status;
            }
        }

    if (NT_SUCCESS( Status )) {
        return TRUE;
        }
    else
        {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
APIENTRY
GetNamedPipeHandleStateA(
    HANDLE hNamedPipe,
    LPDWORD lpState,
    LPDWORD lpCurInstances,
    LPDWORD lpMaxCollectionCount,
    LPDWORD lpCollectDataTimeout,
    LPSTR lpUserName,
    DWORD nMaxUserNameSize
    )
/*++

Routine Description:

    Ansi thunk to GetNamedPipeHandleStateW

---*/
{
    if ( ARGUMENT_PRESENT( lpUserName ) ) {

        BOOL b;
        NTSTATUS Status;
        ANSI_STRING AnsiUserName;
        UNICODE_STRING UnicodeUserName;

        UnicodeUserName.MaximumLength = (USHORT)(nMaxUserNameSize << 1);
        UnicodeUserName.Buffer = RtlAllocateHeap(
                                        RtlProcessHeap(),MAKE_TAG( TMP_TAG ),
                                        UnicodeUserName.MaximumLength
                                        );

        AnsiUserName.Buffer = lpUserName;
        AnsiUserName.MaximumLength = (USHORT)nMaxUserNameSize;


        if ( !UnicodeUserName.Buffer ) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
            }


        b = GetNamedPipeHandleStateW(
                hNamedPipe,
                lpState,
                lpCurInstances,
                lpMaxCollectionCount,
                lpCollectDataTimeout,
                UnicodeUserName.Buffer,
                UnicodeUserName.MaximumLength/2);

        if ( b ) {

            //  Set length correctly in UnicodeUserName
            RtlInitUnicodeString(
                &UnicodeUserName,
                UnicodeUserName.Buffer
                );

            Status = RtlUnicodeStringToAnsiString(
                        &AnsiUserName,
                        &UnicodeUserName,
                        FALSE
                        );

            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                b = FALSE;
                }
            }

        if ( UnicodeUserName.Buffer ) {
            RtlFreeHeap(RtlProcessHeap(),0,UnicodeUserName.Buffer);
            }

        return b;
        }
    else {
        return GetNamedPipeHandleStateW(
                hNamedPipe,
                lpState,
                lpCurInstances,
                lpMaxCollectionCount,
                lpCollectDataTimeout,
                NULL,
                0);
        }

}

BOOL
APIENTRY
GetNamedPipeHandleStateW(
    HANDLE hNamedPipe,
    LPDWORD lpState,
    LPDWORD lpCurInstances,
    LPDWORD lpMaxCollectionCount,
    LPDWORD lpCollectDataTimeout,
    LPWSTR lpUserName,
    DWORD nMaxUserNameSize
    )
/*++

Routine Description:

The GetNamedPipeHandleState function retrieves information about a given
named pipe handle. The information returned by this function can vary during
the lifetime of an instance of a named pipe. The handle must be created with
the GENERIC_READ access rights.

Arguments:

    hNamedPipe - Supplies the handle of an opened named pipe.

    lpState - An optional parameter that if non-null, points to a DWORD which
        will be set with flags indicating the current state of the handle.
        The following flags may be specified:

        PIPE_NOWAIT
            Nonblocking mode is to be used for this handle.

        PIPE_READMODE_MESSAGE
            Read the pipe as a message stream. If this flag is not set, the pipe is
            read as a byte stream.

    lpCurInstances - An optional parameter that if non-null, points to a DWORD
        which will be set with the number of current pipe instances.

    lpMaxCollectionCount - If non-null, this points to a DWORD which will be
        set to the maximum number of bytes that will be collected on the clients
        machine before transmission to the server. This parameter must be NULL
        on a handle to the server end of a named pipe or when client and
        server applications are on the same machine.

    lpCollectDataTimeout - If non-null, this points to a DWORD which will be
        set to the maximum time (in milliseconds) that can pass before a
        remote named pipe transfers information over the network. This parameter
        must be NULL if the handle is for the server end of a named pipe or
        when client and server applications are on the same machine.

    lpUserName - An optional parameter on the server end of a named pipe.
        Points to an area which will be filled-in with the null-terminated
        string containing the name of the username of the client application.
        This parameter is invalid if not NULL on a handle to a client end of
        a named pipe.

    nMaxUserNameSize - Size in characters of the memory allocated at lpUserName.
        Ignored if lpUserName is NULL.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{

    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    if ( ARGUMENT_PRESENT( lpState ) ){
        FILE_PIPE_INFORMATION Common;

        Status = NtQueryInformationFile(
                    hNamedPipe,
                    &Iosb,
                    &Common,
                    sizeof(FILE_PIPE_INFORMATION),
                    FilePipeInformation );

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }

        *lpState = (Common.CompletionMode == FILE_PIPE_QUEUE_OPERATION) ?
            PIPE_WAIT : PIPE_NOWAIT;

        *lpState |= (Common.ReadMode == FILE_PIPE_BYTE_STREAM_MODE) ?
            PIPE_READMODE_BYTE : PIPE_READMODE_MESSAGE;
        }

    if (ARGUMENT_PRESENT( lpCurInstances ) ){
        FILE_PIPE_LOCAL_INFORMATION Local;

        Status = NtQueryInformationFile(
                    hNamedPipe,
                    &Iosb,
                    &Local,
                    sizeof(FILE_PIPE_LOCAL_INFORMATION),
                    FilePipeLocalInformation );

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }

        if (Local.CurrentInstances >= PIPE_UNLIMITED_INSTANCES) {
            *lpCurInstances = PIPE_UNLIMITED_INSTANCES;
            }
        else {
            *lpCurInstances = Local.CurrentInstances;
            }

        }

    if (ARGUMENT_PRESENT( lpMaxCollectionCount ) ||
        ARGUMENT_PRESENT( lpCollectDataTimeout ) ) {
        FILE_PIPE_REMOTE_INFORMATION Remote;

        Status = NtQueryInformationFile(
                    hNamedPipe,
                    &Iosb,
                    &Remote,
                    sizeof(FILE_PIPE_REMOTE_INFORMATION),
                    FilePipeRemoteInformation );

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }

        if (ARGUMENT_PRESENT( lpMaxCollectionCount ) ) {
            *lpMaxCollectionCount = Remote.MaximumCollectionCount;
            }

        if (ARGUMENT_PRESENT( lpCollectDataTimeout ) ) {
            LARGE_INTEGER TimeWorkspace;
            LARGE_INTEGER LiTemporary;

            // Convert delta NT LARGE_INTEGER to milliseconds delay

            LiTemporary.QuadPart = -Remote.CollectDataTime.QuadPart;
            TimeWorkspace = RtlExtendedLargeIntegerDivide (
                    LiTemporary,
                    10000,
                    NULL ); // Not interested in any remainder

            if ( TimeWorkspace.HighPart ) {

                //
                //  Timeout larger than we can return- but not infinity.
                //  Must have been set with the direct NT Interface.
                //

                *lpCollectDataTimeout = 0xfffffffe;   //  Maximum we can set
                }
            else {
                *lpCollectDataTimeout = TimeWorkspace.LowPart;
                }
            }
        }

    if ( ARGUMENT_PRESENT( lpUserName ) ) {
            return NpGetUserNamep(hNamedPipe, lpUserName, nMaxUserNameSize );
        }

    return TRUE;
}

BOOL
NpGetUserNamep(
    HANDLE hNamedPipe,
    LPWSTR lpUserName,
    DWORD nMaxUserNameSize
    )
/*++

Routine Description:

The NpGetUserNamep function retrieves user name for the client at the other
end of the named pipe indicated by the handle.

Arguments:

    hNamedPipe - Supplies the handle of an opened named pipe.

    lpUserName - Points to an area which will be filled-in with the null-terminated
        string containing the name of the username of the client application.
        This parameter is invalid if not NULL on a handle to a client end of
        a named pipe.

    nMaxUserNameSize - Size in characters of the memory allocated at lpUserName.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{
    HANDLE   hToken;
    NTSTATUS Status;
    DWORD Size = nMaxUserNameSize;
    BOOL res;
    HANDLE advapi32;

    REVERTTOSELF RevertToSelfp;

    GETUSERNAMEW GetUserNameWp;

    IMPERSONATENAMEDPIPECLIENT ImpersonateNamedPipeClientp;

    advapi32 = LoadLibrary("advapi32");

    if (advapi32 == NULL ) {
        return FALSE;
        }

    RevertToSelfp = (REVERTTOSELF)GetProcAddress(advapi32,"RevertToSelf");
    if ( RevertToSelfp == NULL) {
        FreeLibrary(advapi32);
        return FALSE;
        }

    GetUserNameWp = (GETUSERNAMEW)GetProcAddress(advapi32,"GetUserNameW");
    if ( GetUserNameWp == NULL) {
        FreeLibrary(advapi32);
        return FALSE;
        }

    ImpersonateNamedPipeClientp = (IMPERSONATENAMEDPIPECLIENT)GetProcAddress(advapi32,"ImpersonateNamedPipeClient");
    if ( ImpersonateNamedPipeClientp == NULL) {
        FreeLibrary(advapi32);
        return FALSE;
        }

    //  Save whoever the thread is currently impersonating.

    Status = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_IMPERSONATE,
                    TRUE,
                    &hToken
                    );

    (ImpersonateNamedPipeClientp)( hNamedPipe );

    res = (GetUserNameWp)( lpUserName, &Size );

    if ( !NT_SUCCESS( Status ) ) {
        //  We were not impersonating anyone

        (RevertToSelfp)();

    } else {

        //
        //  Set thread back to whoever it was originally impersonating.
        //  An error on this API overrides any error from  GetUserNameW
        //

        Status = NtSetInformationThread(
                     NtCurrentThread(),
                     ThreadImpersonationToken,
                     (PVOID)&hToken,
                     (ULONG)sizeof(HANDLE)
                     );

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            FreeLibrary(advapi32);
            return FALSE;
            }
        }

    FreeLibrary(advapi32);

    return res;
}

BOOL
APIENTRY
SetNamedPipeHandleState(
    HANDLE hNamedPipe,
    LPDWORD lpMode,
    LPDWORD lpMaxCollectionCount,
    LPDWORD lpCollectDataTimeout
    )
/*++

Routine Description:


    The SetNamedPipeHandleState function is used to set the read
    mode and the blocking mode of a named pipe. On the client end
    of a remote named pipe this function can also control local
    buffering. The handle must be created with the GENERIC_WRITE
    access rights.

Arguments:

    hNamedPipe - Supplies a handle to a named pipe.

    lpMode - If non-null, this points to a DWORD which supplies the new
        mode. The mode is a combination of a read-mode flag and a wait flag.
        The following values may be used:

    PIPE_READMODE_BYTE
        Read pipe as a byte stream.

    PIPE_READMODE_MESSAGE
        Read pipe as a message stream.

    PIPE_WAIT
        Blocking mode is to be used for this handle.

    PIPE_NOWAIT
        Nonblocking mode is to be used for this handle.

    lpMaxCollectionCount - If non-null, this points to
        a DWORD which supplies the maximum number of
        bytes that will be collected on the client machine before
        transmission to the server. This parameter must be NULL on
        a handle to the server end of a named pipe or when client
        and server applications are on the same machine. This parameter
        is ignored if the client specified write through
        when the handle was created.

    lpCollectDataTimeout - If non-null, this points to a DWORD which
        supplies the maximum time (in milliseconds) that can pass before
        a remote named pipe transfers information over the network. This
        parameter must be NULL if the handle is for the server end of a
        named pipe or when client and server applications are on the same
        machine. This parameter is ignored if the client specified write
        through when the handle was created.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{

    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    if ( ARGUMENT_PRESENT( lpMode ) ){
        FILE_PIPE_INFORMATION Common;

        if (*lpMode & INVALID_PIPE_MODE_BITS) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        Common.ReadMode = ( *lpMode & PIPE_READMODE_MESSAGE ) ?
            FILE_PIPE_MESSAGE_MODE: FILE_PIPE_BYTE_STREAM_MODE;

        Common.CompletionMode = ( *lpMode & PIPE_NOWAIT ) ?
            FILE_PIPE_COMPLETE_OPERATION : FILE_PIPE_QUEUE_OPERATION;

        Status = NtSetInformationFile(
                    hNamedPipe,
                    &Iosb,
                    &Common,
                    sizeof(FILE_PIPE_INFORMATION),
                    FilePipeInformation );

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }

    if ( ARGUMENT_PRESENT( lpMaxCollectionCount ) ||
         ARGUMENT_PRESENT( lpCollectDataTimeout ) ){
        FILE_PIPE_REMOTE_INFORMATION Remote;

        if ( ( lpMaxCollectionCount == NULL ) ||
             ( lpCollectDataTimeout == NULL ) ){

            //
            // User is setting only one of the two parameters so read
            // the other value. There is a small window where another
            // thread using the same handle could set the other value.
            // in this case the setting would be lost.
            //

            Status = NtQueryInformationFile(
                    hNamedPipe,
                    &Iosb,
                    &Remote,
                    sizeof(FILE_PIPE_REMOTE_INFORMATION),
                    FilePipeRemoteInformation );

            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                return FALSE;
                }
            }

        if (ARGUMENT_PRESENT( lpMaxCollectionCount ) ) {
            Remote.MaximumCollectionCount = *lpMaxCollectionCount;
            }

        if (ARGUMENT_PRESENT( lpCollectDataTimeout ) ) {

            //
            //  Convert from milliseconds to an Nt delta time.
            //

            Remote.CollectDataTime.QuadPart =
                        - (LONGLONG)UInt32x32To64( 10 * 1000, *lpCollectDataTimeout );
            }

        Status = NtSetInformationFile(
                    hNamedPipe,
                    &Iosb,
                    &Remote,
                    sizeof(FILE_PIPE_REMOTE_INFORMATION),
                    FilePipeRemoteInformation );

        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }


    return TRUE;
}

BOOL
APIENTRY
GetNamedPipeInfo(
    HANDLE hNamedPipe,
    LPDWORD lpFlags,
    LPDWORD lpOutBufferSize,
    LPDWORD lpInBufferSize,
    LPDWORD lpMaxInstances
    )
/*++

Routine Description:

    The GetNamedPipeInfo function retrieves information about a named
    pipe. The information returned by this API is preserved the lifetime
    of an instance of a named pipe. The handle must be created with the
    GENERIC_READ access rights.

Arguments:
    hNamedPipe - Supplies a handle to a named pipe.

    lpFlags - An optional parameter that if non-null, points to a DWORD
        which will be set with flags indicating the type of named pipe and handle.

        PIPE_END_SERVER
            The handle is the server end of a named pipe.

        PIPE_TYPE_MESSAGE
            The pipe is a message-stream pipe. If this flag is not set, the pipe is
            a byte-stream pipe.

    lpOutBufferSize - An optional parameter that if non-null, points to a
        DWORD which will be set with the size (in bytes) of the buffer for
        outgoing data. A return value of zero indicates the buffer is allocated
        as needed.

    lpInBufferSize - An optional parameter that if non-null, points to a DWORD
        which will be set with the size (in bytes) of the buffer for incoming
        data. A return
        value of zero indicates the buffer is allocated as needed.

    lpMaxInstances - An optional parameter that if non-null, points to a
        DWORD which will be set with the maximum number of pipe instances
        that can be created. Besides various numeric values, a special value
        may be returned for this.

        PIPE_UNLIMITED_INSTANCES
            Unlimited instances of the pipe can be created. This is an
            indicator that the maximum is requested; the value of the
            equate may be higher or lower than the actual implementation's limit,
            which may vary over time.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{

    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    FILE_PIPE_LOCAL_INFORMATION Local;

    Status = NtQueryInformationFile(
                    hNamedPipe,
                    &Iosb,
                    &Local,
                    sizeof(FILE_PIPE_LOCAL_INFORMATION),
                    FilePipeLocalInformation );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    if (ARGUMENT_PRESENT( lpFlags ) ) {
        *lpFlags = (Local.NamedPipeEnd == FILE_PIPE_CLIENT_END) ?
            PIPE_CLIENT_END : PIPE_SERVER_END;
        *lpFlags |= (Local.NamedPipeType == FILE_PIPE_BYTE_STREAM_TYPE) ?
            PIPE_TYPE_BYTE : PIPE_TYPE_MESSAGE;
        }

    if (ARGUMENT_PRESENT( lpOutBufferSize ) ) {
        *lpOutBufferSize = Local.OutboundQuota;
        }

    if (ARGUMENT_PRESENT( lpInBufferSize ) ) {
        *lpInBufferSize = Local.InboundQuota;
        }

    if (ARGUMENT_PRESENT( lpMaxInstances ) ) {
        if (Local.MaximumInstances >= PIPE_UNLIMITED_INSTANCES) {
            *lpMaxInstances = PIPE_UNLIMITED_INSTANCES;
            }
        else {
            *lpMaxInstances = Local.MaximumInstances;
            }
        }


    return TRUE;
}


BOOL
APIENTRY
PeekNamedPipe(
    HANDLE hNamedPipe,
    LPVOID lpBuffer,
    DWORD nBufferSize,
    LPDWORD lpBytesRead,
    LPDWORD lpTotalBytesAvail,
    LPDWORD lpBytesLeftThisMessage
    )
/*++

Routine Description:

    The PeekNamedPipe function copies a named pipe's data into a buffer for
    preview without removing it. The results of a PeekNamedPipe are similar to
    a ReadFile on the pipe except more information is returned, the function
    never blocks and if the pipe handle is reading in message mode, a partial
    message can be returned.

    A partial message peek'd on a message mode pipe will return TRUE.

    It is not an error if all of the pointers passed to this function are
    null. However, there is no reason for calling it this way.

    The NT peek call has the received data immediately after the state
    information so this routine needs to allocate an intermediate buffer
    large enough for the state information plus data.

Arguments:

    hNamedPipe - Supplies a handle to a named pipe.

    lpBuffer - If non-null, pointer to buffer to read data into.

    nBufferSize - Size of input buffer, in bytes. (Ignored if lpBuffer
        is null.)

    lpBytesRead - If non-null, this points to a DWORD which will be set
        with the number of bytes actually read.

    lpTotalBytesAvail - If non-null, this points to a DWORD which receives
        a value giving the number of bytes that were available to be read.

    lpBytesLeftThisMessage - If non-null, this points to a DWORD which
        will be set to the number of bytes left in this message. (This will
        be zero for a byte-stream pipe.)

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{

    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    PFILE_PIPE_PEEK_BUFFER PeekBuffer;
    DWORD IOLength;

    // Allocate enough for the users data and FILE_PIPE_PEEK_BUFFER

    IOLength = nBufferSize + FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]);
    PeekBuffer = RtlAllocateHeap(RtlProcessHeap(),MAKE_TAG( TMP_TAG ), IOLength);

    try {

        Status = NtFsControlFile(hNamedPipe,
                    NULL,
                    NULL,           // APC routine
                    NULL,           // APC Context
                    &Iosb,          // I/O Status block
                    FSCTL_PIPE_PEEK,// IoControlCode
                    NULL,           // Buffer for data to the FS
                    0,              // Length.
                    PeekBuffer,     // OutputBuffer for data from the FS
                    IOLength        // OutputBuffer Length
                    );

        if ( Status == STATUS_PENDING) {
            // Operation must complete before return & IoStatusBlock destroyed
            Status = NtWaitForSingleObject( hNamedPipe, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {
                Status = Iosb.Status;
                }
            }

        //
        //  Buffer overflow simply means that lpBytesLeftThisMessage != 0
        //

        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            Status = STATUS_SUCCESS;
        }

        //
        //  Peek is complete, package up data for caller ensuring that
        //  the PeekBuffer is deleted even if an invalid pointer was given.
        //

        if ( NT_SUCCESS(Status)) {

            try {

                if ( ARGUMENT_PRESENT( lpTotalBytesAvail ) ) {
                    *lpTotalBytesAvail = PeekBuffer->ReadDataAvailable;
                    }

                if ( ARGUMENT_PRESENT( lpBytesRead ) ) {
                    *lpBytesRead = (ULONG)(Iosb.Information - FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]));
                    }

                if ( ARGUMENT_PRESENT( lpBytesLeftThisMessage ) ) {
                    *lpBytesLeftThisMessage =
                        PeekBuffer->MessageLength -
                        (ULONG)(Iosb.Information - FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]));
                    }

                if ( ARGUMENT_PRESENT( lpBuffer ) ) {
                    RtlCopyMemory(
                        lpBuffer,
                        PeekBuffer->Data,
                        Iosb.Information - FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]));
                    }
                }
            except (EXCEPTION_EXECUTE_HANDLER) {
                Status = STATUS_ACCESS_VIOLATION;
                }
            }
        }

    finally {

        if ( PeekBuffer != NULL ) {
            RtlFreeHeap(RtlProcessHeap(),0,PeekBuffer);
            }
        }

    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}

BOOL
APIENTRY
TransactNamedPipe(
    HANDLE hNamedPipe,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesRead,
    LPOVERLAPPED lpOverlapped
    )
/*++

Routine Description:

    The TransactNamedPipe function writes data to and reads data from a named
    pipe. This function fails if the named pipe contains any unread data or if
    the named pipe is not in message mode. A named pipe's blocking state has no
    effect on the TransactNamedPipe function. This API does not complete until
    data is written into the InBuffer buffer. The lpOverlapped parameter is
    available to allow an application to continue processing while the operation
    takes place.

Arguments:
    hNamedPipe - Supplies a handle to a named pipe.

    lpInBuffer - Supplies the buffer containing the data that is written to
        the pipe.

    nInBufferSize - Supplies the size (in bytes) of the output buffer.

    lpOutBuffer - Supplies the buffer that receives the data read from the pipe.

    nOutBufferSize - Supplies the size (in bytes) of the input buffer.

    lpBytesRead - Points to a DWORD that receives the number of bytes actually
        read from the pipe.

    lpOverlapped - An optional parameter that supplies an overlap structure to
        be used with the request. If NULL or the handle was created without
        FILE_FLAG_OVERLAPPED then the TransactNamedPipe will not return until
        the operation completes.

        When lpOverlapped is supplied and FILE_FLAG_OVERLAPPED was specified
        when the handle was created, TransactNamedPipeFile may return
        ERROR_IO_PENDING to allow the caller to continue processing while the
        operation completes. The event (or File handle if hEvent == NULL) will
        be set to the not signalled state before ERROR_IO_PENDING is
        returned. The event will be set to the signalled state upon completion
        of the request. GetOverlappedResult is used to determine the result
        when ERROR_IO_PENDING is returned.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{

    NTSTATUS Status;

    if ( ARGUMENT_PRESENT( lpOverlapped ) ) {

        lpOverlapped->Internal = (DWORD)STATUS_PENDING;

        Status = NtFsControlFile(hNamedPipe,
                    lpOverlapped->hEvent,
                    NULL,           // APC routine
                    (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
                    (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
                    FSCTL_PIPE_TRANSCEIVE,// IoControlCode
                    lpInBuffer,    // Buffer for data to the FS
                    nInBufferSize,
                    lpOutBuffer,     // OutputBuffer for data from the FS
                    nOutBufferSize   // OutputBuffer Length
                    );

        if ( NT_SUCCESS(Status) && Status != STATUS_PENDING) {
            if ( ARGUMENT_PRESENT(lpBytesRead) ) {
                try {
                    *lpBytesRead = (DWORD)lpOverlapped->InternalHigh;
                    }
                except(EXCEPTION_EXECUTE_HANDLER) {
                    *lpBytesRead = 0;
                    }
                }
            return TRUE;
            }
        else {
            if ( NT_WARNING(Status) ) {
                if ( ARGUMENT_PRESENT(lpBytesRead) ) {
                    try {
                        *lpBytesRead = (DWORD)lpOverlapped->InternalHigh;
                        }
                    except(EXCEPTION_EXECUTE_HANDLER) {
                        *lpBytesRead = 0;
                        }
                    }
            }
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
    else
        {
        IO_STATUS_BLOCK Iosb;

        Status = NtFsControlFile(hNamedPipe,
                    NULL,
                    NULL,           // APC routine
                    NULL,           // APC Context
                    &Iosb,
                    FSCTL_PIPE_TRANSCEIVE,// IoControlCode
                    lpInBuffer,    // Buffer for data to the FS
                    nInBufferSize,
                    lpOutBuffer,     // OutputBuffer for data from the FS
                    nOutBufferSize   // OutputBuffer Length
                    );

        if ( Status == STATUS_PENDING) {
            // Operation must complete before return & Iosb destroyed
            Status = NtWaitForSingleObject( hNamedPipe, FALSE, NULL );
            if ( NT_SUCCESS(Status)) {
                Status = Iosb.Status;
                }
            }

        if ( NT_SUCCESS(Status) ) {
            *lpBytesRead = (DWORD)Iosb.Information;
            return TRUE;
            }
        else {
            if ( NT_WARNING(Status) ) {
                *lpBytesRead = (DWORD)Iosb.Information;
            }
            BaseSetLastNTError(Status);
            return FALSE;
            }
        }
}

BOOL
APIENTRY
CallNamedPipeA(
    LPCSTR lpNamedPipeName,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesRead,
    DWORD nTimeOut
    )
/*++

    ANSI thunk to CallNamedPipeW

--*/
{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString,lpNamedPipeName);
    Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            BaseSetLastNTError(Status);
            }
        return FALSE;
        }

    return ( CallNamedPipeW( (LPCWSTR)Unicode->Buffer,
                lpInBuffer,
                nInBufferSize,
                lpOutBuffer,
                nOutBufferSize,
                lpBytesRead,
                nTimeOut)
           );
}


BOOL
APIENTRY
CallNamedPipeW(
    LPCWSTR lpNamedPipeName,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesRead,
    DWORD nTimeOut
    )
/*++

Routine Description:

    CallNamedPipe is equivalent to a series of calls to CreateFile, perhaps
    WaitNamedPipe (if CreateFile can't open the pipe immediately),
    SetNamedPipeHandleState, TransactNamedPipe, and CloseFile. Refer to
    the documentation for those APIs for more information.

Arguments:

    lpNamedPipeName - Supplies the name of the named pipe.

    lpInBuffer - Supplies the buffer containing the data that is written to
        the pipe.

    nInBufferSize - Supplies the size (in bytes) of the output buffer.

    lpOutBuffer - Supplies the buffer that receives the data read from the pipe.

    nOutBufferSize - Supplies the size (in bytes) of the input buffer.

    lpBytesRead - Points to a DWORD that receives the number of bytes actually
        read from the pipe.

    nTimeOut - Gives a value (in milliseconds) that is the amount of time
        this function should wait for the pipe to become available. (Note
        that the function may take longer than that to execute, due to
        various factors.)

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{

    HANDLE Pipe;
    BOOL FirstChance = TRUE; //  Allow only one chance at WaitNamedPipe
    BOOL Result;

    while ( 1 ) {

        Pipe = CreateFileW(lpNamedPipeName,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,       // Security Attributes
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

        if ( Pipe != INVALID_HANDLE_VALUE ) {
            break;  //  Created a handle
            }

        if ( FirstChance == FALSE ) {
           //  Already called WaitNamedPipe once so give up.
           return FALSE;
        }

        WaitNamedPipeW(lpNamedPipeName, nTimeOut);

        FirstChance = FALSE;

        }


    try {
        DWORD ReadMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;

        //  Default open is readmode byte stream- change to message mode.
        Result = SetNamedPipeHandleState( Pipe, &ReadMode, NULL, NULL);

        if ( Result == TRUE ) {
            Result = TransactNamedPipe(
                Pipe,
                lpInBuffer,
                nInBufferSize,
                lpOutBuffer,
                nOutBufferSize,
                lpBytesRead,
                NULL);
            }
        }
    finally {
        CloseHandle( Pipe );
        }

    return Result;
}

BOOL
APIENTRY
WaitNamedPipeA(
    LPCSTR lpNamedPipeName,
    DWORD nTimeOut
    )
/*++

    Ansi thunk to WaitNamedPipeW

--*/
{
    UNICODE_STRING UnicodeString;
    BOOL b;

    if (!Basep8BitStringToDynamicUnicodeString( &UnicodeString, lpNamedPipeName )) {
        return FALSE;
    }

    b = WaitNamedPipeW( UnicodeString.Buffer, nTimeOut );

    RtlFreeUnicodeString(&UnicodeString);

    return b;

}


BOOL
APIENTRY
WaitNamedPipeW(
    LPCWSTR lpNamedPipeName,
    DWORD nTimeOut
    )
/*++

Routine Description:

    The WaitNamedPipe function waits for a named pipe to become available.

Arguments:

    lpNamedPipeName - Supplies the name of the named pipe.

    nTimeOut - Gives a value (in milliseconds) that is the amount of time
        this function should wait for the pipe to become available. (Note
        that the function may take longer than that to execute, due to
        various factors.)

    nTimeOut Special Values:

        NMPWAIT_WAIT_FOREVER
            No timeout.

        NMPWAIT_USE_DEFAULT_WAIT
            Use default timeout set in call to CreateNamedPipe.

Return Value:

    TRUE -- The operation was successful.

    FALSE -- The operation failed. Extended error status is available using
        GetLastError.

--*/
{

    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    RTL_PATH_TYPE PathType;
    ULONG WaitPipeLength;
    PFILE_PIPE_WAIT_FOR_BUFFER WaitPipe;
    PWSTR FreeBuffer;
    UNICODE_STRING FileSystem;
    UNICODE_STRING PipeName;
    UNICODE_STRING OriginalPipeName;
    UNICODE_STRING ValidUnicodePrefix;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    LPWSTR Pwc;
    ULONG Index;

    //
    //  Open a handle either to the redirector or the NPFS depending on
    //  the start of the pipe name. Split lpNamedPipeName into two
    //  halves as follows:
    //      \\.\pipe\pipename       \\.\pipe\ and pipename
    //      \\server\pipe\pipename  \\ and server\pipe\pipename
    //

    if (!RtlCreateUnicodeString( &OriginalPipeName, lpNamedPipeName)) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
        }

    //
    //  Change all the forward slashes into backward slashes.
    //

    for ( Index =0; Index < (OriginalPipeName.Length/sizeof(WCHAR)); Index++ ) {
        if (OriginalPipeName.Buffer[Index] == L'/') {
            OriginalPipeName.Buffer[Index] = L'\\';
            }
        }

    PipeName = OriginalPipeName;

    PathType = RtlDetermineDosPathNameType_U(lpNamedPipeName);

    FreeBuffer = NULL;

    switch ( PathType ) {
    case RtlPathTypeLocalDevice:

            //  Name should be of the form \\.\pipe\pipename (IgnoreCase)

            RtlInitUnicodeString( &ValidUnicodePrefix, DOS_LOCAL_PIPE_PREFIX);

            if (RtlPrefixString((PSTRING)&ValidUnicodePrefix,
                    (PSTRING)&PipeName,
                    TRUE) == FALSE) {
                RtlFreeUnicodeString(&ValidUnicodePrefix);
                RtlFreeUnicodeString(&OriginalPipeName);
                BaseSetLastNTError(STATUS_OBJECT_PATH_SYNTAX_BAD);
                return FALSE;
                }

            //  Skip first 9 characters "\\.\pipe\"
            PipeName.Buffer+=9;
            PipeName.Length-=9*sizeof(WCHAR);

            RtlInitUnicodeString( &FileSystem, DOS_LOCAL_PIPE);

            break;

        case RtlPathTypeUncAbsolute:
            //  Name is of the form \\server\pipe\pipename

            //  Find the pipe name.

            for ( Pwc = &PipeName.Buffer[2]; *Pwc != 0; Pwc++) {
                if ( *Pwc == L'\\') {
                    //  Found backslash after servername
                    break;
                    }
                }

            if ( (*Pwc != 0) &&
                 ( _wcsnicmp( Pwc + 1, L"pipe\\", 5 ) == 0 ) ) {

                // Temporarily, break this up into 2 strings
                //    string1 = \\server\pipe
                //    string2 = the-rest

                Pwc += (sizeof (L"pipe\\") / sizeof( WCHAR ) ) - 1;

            } else {

                // This is not a valid remote path name.

                RtlFreeUnicodeString(&OriginalPipeName);
                BaseSetLastNTError(STATUS_OBJECT_PATH_SYNTAX_BAD);
                return FALSE;
                }

            //  Pwc now points to the first path seperator after \\server\pipe.
            //  Attempt to open \DosDevices\Unc\Servername\Pipe.

            PipeName.Buffer = &PipeName.Buffer[2];
            PipeName.Length = (USHORT)((PCHAR)Pwc - (PCHAR)PipeName.Buffer);
            PipeName.MaximumLength = PipeName.Length;

            FileSystem.MaximumLength =
                (USHORT)sizeof( DOS_REMOTE_PIPE ) +
                PipeName.MaximumLength;

            FileSystem.Buffer = RtlAllocateHeap(
                                    RtlProcessHeap(),MAKE_TAG( TMP_TAG ),
                                    FileSystem.MaximumLength
                                    );

            if ( !FileSystem.Buffer ) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                RtlFreeUnicodeString(&OriginalPipeName);
                return FALSE;
                }
            FreeBuffer = FileSystem.Buffer;

            RtlCopyMemory(
                FileSystem.Buffer,
                DOS_REMOTE_PIPE,
                sizeof( DOS_REMOTE_PIPE ) - sizeof(WCHAR)
                );

            FileSystem.Length = sizeof( DOS_REMOTE_PIPE ) - sizeof(WCHAR);

            RtlAppendUnicodeStringToString( &FileSystem, &PipeName );

            // Set up pipe name, skip leading backslashes.

            RtlInitUnicodeString( &PipeName, (PWCH)Pwc + 1 );

            break;

        default:
            BaseSetLastNTError(STATUS_OBJECT_PATH_SYNTAX_BAD);
            RtlFreeUnicodeString(&OriginalPipeName);
            return FALSE;
        }


    InitializeObjectAttributes(
        &Obja,
        &FileSystem,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT
                );

    if (FreeBuffer != NULL) {
        RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
        }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        RtlFreeUnicodeString(&OriginalPipeName);
        return FALSE;
        }

    WaitPipeLength =
        FIELD_OFFSET(FILE_PIPE_WAIT_FOR_BUFFER, Name[0]) + PipeName.Length;
    WaitPipe = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( TMP_TAG ), WaitPipeLength);
    if ( !WaitPipe ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        RtlFreeUnicodeString(&OriginalPipeName);
        return FALSE;
        }

    if ( nTimeOut == NMPWAIT_USE_DEFAULT_WAIT ) {
        WaitPipe->TimeoutSpecified = FALSE;
        }
    else {
        if ( nTimeOut == NMPWAIT_WAIT_FOREVER ) {
            WaitPipe->Timeout.LowPart = 0;
            WaitPipe->Timeout.HighPart =0x80000000;
            }
        else {
            //
            //  Convert from milliseconds to an Nt delta time.
            //

            WaitPipe->Timeout.QuadPart =
                                - (LONGLONG)UInt32x32To64( 10 * 1000, nTimeOut );
            }
        WaitPipe->TimeoutSpecified = TRUE;
        }

    WaitPipe->NameLength = PipeName.Length;

    RtlCopyMemory(
        WaitPipe->Name,
        PipeName.Buffer,
        PipeName.Length
        );

    RtlFreeUnicodeString(&OriginalPipeName);

    Status = NtFsControlFile(Handle,
                        NULL,
                        NULL,           // APC routine
                        NULL,           // APC Context
                        &Iosb,
                        FSCTL_PIPE_WAIT,// IoControlCode
                        WaitPipe,       // Buffer for data to the FS
                        WaitPipeLength,
                        NULL,           // OutputBuffer for data from the FS
                        0               // OutputBuffer Length
                        );

    RtlFreeHeap(RtlProcessHeap(),0,WaitPipe);

    NtClose(Handle);

    if (NT_SUCCESS( Status ) ) {
        return TRUE;
        }
    else
        {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}
