/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    startup.c

Abstract:

    This module contains the startup and cleanup code for winsock2 DLL

Author:

    dirk@mink.intel.com  14-JUN-1995

Revision History:

    22-Aug-1995 dirk@mink.intel.com
        Cleanup after code review.

--*/

#include "precomp.h"


INT
CheckForHookersOrChainers();

static
CRITICAL_SECTION  Startup_Synchro;
    // Startup_Synchro  is  used  as  a  synchronization  mechanism  to prevent
    // multiple  threads  from  overlapping  execution  of  the  WSAStartup and
    // WSACleanup procedures.




VOID
CreateStartupSynchronization()
/*++

Routine Description:

    This procedure creates the Startup/Cleanup synchronization mechanism.  This
    must  be  called  once  before  the  WSAStartup  procedure  may  be called.
    Typically, this is called from the DLL_PROCESS_ATTACH branch of DllMain, as
    the  only  reliable  way to guarantee that it gets called before any thread
    calls WSAStartup.

Arguments:

    None

Return Value:

    None
--*/
{
    DEBUGF(
        DBG_TRACE,
        ("Initializing Startup/Cleanup critical section\n"));

    InitializeCriticalSection(
        & Startup_Synchro
        );
}  // CreateStartupSynchronization




VOID
DestroyStartupSynchronization()
/*++

Routine Description:

    This  procedure  destroys  the  Startup/Cleanup  synchronization mechanism.
    This  must  be  called once after the final WSACleanup procedure is called.
    Typically, this is called from the DLL_PROCESS_DETACH branch of DllMain, as
    the  only  reliable  way  to guarantee that it gets called after any thread
    calls WSACleanup.

Arguments:

    None

Return Value:

    None
--*/
{
    DEBUGF(
        DBG_TRACE,
        ("Deleting Startup/Cleanup critical section\n"));

    DeleteCriticalSection(
        & Startup_Synchro
        );
}  // DestroyStartupSynchronization



int WSAAPI
WSAStartup(
    IN WORD wVersionRequired,
    OUT LPWSADATA lpWSAData
    )
/*++
Routine Description:

    Winsock  DLL initialization routine.  A Process must successfully call this
    routine before calling any other winsock API function.

Arguments:

    wVersionRequested - The  highest version of WinSock support that the caller
                        can  use.   The  high  order  byte  specifies the minor
                        version (revision) number; the low-order byte specifies
                        the major version number.

    lpWSAData         - A  pointer  to  the  WSADATA  data structure that is to
                        receive details of the WinSock implementation.

Returns:

    Zero if sucessful or an error code as listed in the specification.

Implementation Notes:

    check versions for validity
    enter critical section
        current_proc = get current process
        if failed to get current process then
            dprocess class initialize
            dthread class initialize
            current_proc = get current process
        endif
        current_proc->increment_ref_count
    leave critical section
--*/
{
    int ReturnCode = ERROR_SUCCESS;
    BOOL ContinueInit = TRUE;
    WORD SupportedVersion=0;
    WORD MajorVersion=0;
    WORD MinorVersion=0;

    // Our DLL initialization routine has not been called yet
    if (gDllHandle==NULL)
        return WSASYSNOTREADY;

    // Extract the version number from the user request
    MajorVersion = LOBYTE(wVersionRequired);
    MinorVersion = HIBYTE(wVersionRequired);

    // Check  the  version the user requested and see if we can support it.  If
    // the requested version is less than 2.0 then we can support it
    // Extract the version number from the user request
    MajorVersion = LOBYTE(wVersionRequired);
    MinorVersion = HIBYTE(wVersionRequired);

    //
    // Version checks
    //

    switch (MajorVersion) {

    case 0:

        ReturnCode = WSAVERNOTSUPPORTED;
        break;

    case 1:

        if( MinorVersion == 0 ) {
            SupportedVersion = MAKEWORD(1,0);
        } else {
            MinorVersion = 1;
            SupportedVersion = MAKEWORD(1,1);
        }

        break;

    case 2:

        if( MinorVersion <= 2 ) {
            SupportedVersion = MAKEWORD(2,(BYTE)MinorVersion);
        } else {
            MinorVersion = 2;
            SupportedVersion = MAKEWORD(2,2);
        }

        break;

    default:

        MajorVersion =
        MinorVersion = 2;
        SupportedVersion = MAKEWORD(2,2);
        break;
    }


    __try {
        //
        // Fill in the user structure
        //
        lpWSAData->wVersion = SupportedVersion;
        lpWSAData->wHighVersion = WINSOCK_HIGH_API_VERSION;

        // Fill in the required fields from 1.0 and 1.1 these fields are
        // ignored in 2.0 and later versions of API spec
        if (MajorVersion == 1) {

            // WinSock  1.1  under  NT  always  set iMaxSockets=32767.  WinSock 1.1
            // under  Windows  95  always  set  iMaxSockets=256.   Either  value is
            // actually  incorrect,  since there was no fixed upper limit.  We just
            // use  32767,  since  it  is likely to damage the fewest number of old
            // applications.
            lpWSAData->iMaxSockets = 32767;

            // WinSock 1.1 under Windows 95 and early versions of NT used the value
            // 65535-68  for  iMaxUdpDg.   This  number  is  also  meaningless, but
            // preserving  the  same value is likely to damage the fewest number of
            // old applications.
            lpWSAData->iMaxUdpDg = 65535 - 68;
        } //if
        else {

            // iMaxSockets  and  iMaxUdpDg  are no longer relevant in WinSock 2 and
            // later.  No applications should depend on their values.  We use 0 for
            // both  of  these  as  a  means  of  flushing  out  applications  that
            // incorrectly  depend  on  the  values.   This is NOT a bug.  If a bug
            // report  is  ever  issued  against  these 0 values, the bug is in the
            // caller's code that is incorrectly depending on the values.
            lpWSAData->iMaxSockets = 0;
            lpWSAData->iMaxUdpDg = 0;
        } // else


        (void) lstrcpy(
            lpWSAData->szDescription,
            "WinSock 2.0");
    #if defined(TRACING) && defined(BUILD_TAG_STRING)
        (void) lstrcat(
            lpWSAData->szDescription,
            " Alpha BUILD_TAG=");
        (void) lstrcat(
            lpWSAData->szDescription,
            BUILD_TAG_STRING);
    #endif  // TRACING && BUILD_TAG_STRING

        //TODO: Think up a good value for "system status"
        (void) lstrcpy(
            lpWSAData->szSystemStatus,
            "Running");

        //
        // The following line is commented-out due to annoying and totally
        // nasty alignment problems in WINSOCK[2].H. The exact location of
        // the lpVendorInfo field of the WSAData structure is dependent on
        // the structure alignment used when compiling the source. Since we
        // cannot change the structure alignment of existing apps, the best
        // way to handle this mess is to just not set this value. This turns
        // out to not be too bad a solution, as neither the WinNT nor the Win95
        // WinSock implementations set this value, and nobody appears to pay
        // any attention to it anyway.
        //
        // lpWSAData->lpVendorInfo = NULL;
        //
    }
    __except (WS2_EXCEPTION_FILTER()) {
        if (ReturnCode==ERROR_SUCCESS)
            ReturnCode = WSAEFAULT;
    }

    if (ReturnCode==ERROR_SUCCESS) {
        // Do this outside of critical section
        // because it does GetModuleHandle and GetProcAddress
        // which take loader lock.
        if (CheckForHookersOrChainers() == ERROR_SUCCESS) {

            BOOL process_class_init_done = FALSE;
            BOOL thread_class_init_done = FALSE;
            BOOL socket_class_init_done = FALSE;
            PDPROCESS CurrentProcess=NULL;
            PDTHREAD CurrentThread=NULL;

            EnterCriticalSection(
                & Startup_Synchro
                );

            while (1) {
                CurrentProcess = DPROCESS::GetCurrentDProcess();

                // GetCurrentDProcess  has  a  most-likely "normal" failure case in the
                // case  where  this  is  the first time WSAStartup is called.

                if (CurrentProcess != NULL) {
                    break;
                }

                ReturnCode = DPROCESS::DProcessClassInitialize();
                if (ReturnCode != ERROR_SUCCESS) {
                    break;
                }
                process_class_init_done = TRUE;

                ReturnCode = DSOCKET::DSocketClassInitialize();
                if (ReturnCode != ERROR_SUCCESS) {
                    break;
                }
                socket_class_init_done = TRUE;

                ReturnCode = DTHREAD::DThreadClassInitialize();
                if (ReturnCode != ERROR_SUCCESS) {
                    break;
                }
                thread_class_init_done = TRUE;

                CurrentProcess = DPROCESS::GetCurrentDProcess();
                if (CurrentProcess==NULL) {
                    ReturnCode = WSASYSNOTREADY;
                    break;
                }

                // We   don't   need   a   reference  to  the  current  thread.
                // Nevertheless,  we retrieve the current thread here just as a
                // means  of  validating  that  initialization  has  gotten far
                // enough   to   be   able  to  retrieve  the  current  thread.
                // Otherwise,  we might detect a peculiar failure at some later
                // time when the client tries to do some real operation.
                ReturnCode = DTHREAD::GetCurrentDThread(
                    CurrentProcess,    // Process
                    & CurrentThread);  // CurrentThread
            } // while (1)

            if (ReturnCode == ERROR_SUCCESS) {

                //
                // Save the version number. If the new version is 1.x,
                // set the API prolog to the old, inefficient prolog.
                // If the new version is NOT 1.x, don't touch the prolog
                // pointer because:
                //
                //     1. It defaults to the 2.x prolog.
                //
                //     2. The process may have already negotiated version
                //        1.x in anticipation of using 1.x-specific features
                //        (such as blocking hooks) and we don't want to
                //        overwrite the prolog pointer with the 2.x prolog.
                //

                CurrentProcess->SetVersion( wVersionRequired );

                if( CurrentProcess->GetMajorVersion() == 1 ) {

                    PrologPointer = &Prolog_v1;

                }

                //
                // Bump the ref count.
                //

                CurrentProcess->IncrementRefCount();
            }  // if success so far

            else {  // some failure occurred, cleanup
                INT dont_care;
                if (thread_class_init_done) {
                    DTHREAD::DThreadClassCleanup();
                } // if thread init done
                if (socket_class_init_done) {
                    dont_care = DSOCKET::DSocketClassCleanup();
                }
                if (process_class_init_done) {
                    if (CurrentProcess != NULL) {
                        delete CurrentProcess;
                    }  // if CurrentProcess is non-null
                } // if process init done
            }  // else

            LeaveCriticalSection(
                & Startup_Synchro
                );
        }
        else {
            ReturnCode = WSASYSNOTREADY;
        }

    }  // if ReturnCode==ERROR_SUCCESS

    return(ReturnCode);
}





extern BOOL SockAsyncThreadInitialized;

int WSAAPI
WSACleanup(
    void
    )
/*++
Routine Description:

     Terminate use of the WinSock DLL.

Arguments:

    None

Returns:

    Zero  on  success  else  SOCKET_ERROR.   The  error  code  is  stored  with
    SetErrorCode().

Implementation Notes:

    enter critical section
        current_proc = get current process
        current_proc->decrement_ref_count
        if current count is zero then
            destroy the process
            dthread class cleanup
        endif
    leave critical section

--*/
{
    INT ReturnValue;
    PDPROCESS CurrentProcess;
    PDTHREAD CurrentThread;
    INT      ErrorCode;
    DWORD    CurrentRefCount;


    EnterCriticalSection(
        & Startup_Synchro
        );

    ErrorCode = PROLOG(&CurrentProcess,
                        &CurrentThread);
    if (ErrorCode == ERROR_SUCCESS) {

        CurrentRefCount = CurrentProcess->DecrementRefCount();

        if (CurrentRefCount == 0) {
            delete CurrentProcess;
        }  // if ref count is zero

        else if (CurrentRefCount == 1  &&  SockAsyncThreadInitialized ) {

            SockTerminateAsyncThread();
        }

        ReturnValue = ERROR_SUCCESS;

    }  // if prolog succeeded
    else {
        SetLastError(ErrorCode);
        ReturnValue = SOCKET_ERROR;
    }

    LeaveCriticalSection(
        & Startup_Synchro
        );

    return(ReturnValue);

}  // WSACleanup


PWINSOCK_POST_ROUTINE
GetSockPostRoutine(
    VOID
    )
{
    EnterCriticalSection(
        & Startup_Synchro
        );

    if (SockPostRoutine==NULL) {
        //
        // We use delayload option with user32.dll, hence
        // the exception handler here.
        //
        __try {
            SockPostRoutine = PostMessage;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            SockPostRoutine = NULL;
        }
    }
    LeaveCriticalSection(
        & Startup_Synchro
        );

    return SockPostRoutine;

}   // InitializeSockPostRoutine


int
PASCAL
WSApSetPostRoutine (
    IN PVOID PostRoutine
    )
{

    EnterCriticalSection(
        & Startup_Synchro
        );

    //
    // Save the routine locally.
    //

    SockPostRoutine = (LPFN_POSTMESSAGE)PostRoutine;

    LeaveCriticalSection(
        & Startup_Synchro
        );
    return ERROR_SUCCESS;

}   // WSApSetPostRoutine


extern "C" {

#if DBG


SOCKET WSAAPI
DTHOOK_accept (
    SOCKET s,
    struct sockaddr FAR *addr,
    int FAR *addrlen);

int WSAAPI
DTHOOK_bind (
    SOCKET s,
    const struct sockaddr FAR *name,
    int namelen);

int WSAAPI
DTHOOK_closesocket (
    SOCKET s);

int WSAAPI
DTHOOK_connect (
    SOCKET s,
    const struct sockaddr FAR *name,
    int namelen);

int WSAAPI
DTHOOK_getpeername (
    SOCKET s,
    struct sockaddr FAR *name,
    int FAR * namelen);

int WSAAPI
DTHOOK_getsockname (
    SOCKET s,
    struct sockaddr FAR *name,
    int FAR * namelen);

int WSAAPI
DTHOOK_getsockopt (
    SOCKET s,
    int level,
    int optname,
    char FAR * optval,
    int FAR *optlen);

u_long WSAAPI
DTHOOK_htonl (
    u_long hostlong);

u_short WSAAPI
DTHOOK_htons (
    u_short hostshort);

int WSAAPI
DTHOOK_ioctlsocket (
    SOCKET s,
    long cmd,
    u_long FAR *argp);

unsigned long WSAAPI
DTHOOK_inet_addr (
    const char FAR * cp);

char FAR * WSAAPI
DTHOOK_inet_ntoa (
    struct in_addr in);

int WSAAPI
DTHOOK_listen (
    SOCKET s,
    int backlog);

u_long WSAAPI
DTHOOK_ntohl (
    u_long netlong);

u_short WSAAPI
DTHOOK_ntohs (
    u_short netshort);

int WSAAPI
DTHOOK_recv (
    SOCKET s,
    char FAR * buf,
    int len,
    int flags);

int WSAAPI
DTHOOK_recvfrom (
    SOCKET s,
    char FAR * buf,
    int len,
    int flags,
    struct sockaddr FAR *from,
    int FAR * fromlen);

int WSAAPI
DTHOOK_select (
    int nfds,
    fd_set FAR *readfds,
    fd_set FAR *writefds,
    fd_set FAR *exceptfds,
    const struct timeval FAR *timeout);

int WSAAPI
DTHOOK_send (
    SOCKET s,
    const char FAR * buf,
    int len,
    int flags);

int WSAAPI
DTHOOK_sendto (
    SOCKET s,
    const char FAR * buf,
    int len,
    int flags,
    const struct sockaddr FAR *to,
    int tolen);

int WSAAPI
DTHOOK_setsockopt (
    SOCKET s,
    int level,
    int optname,
    const char FAR * optval,
    int optlen);

int WSAAPI
DTHOOK_shutdown (
    SOCKET s,
    int how);

SOCKET WSAAPI
DTHOOK_socket (
    int af,
    int type,
    int protocol);


SOCKET WSAAPI
DTHOOK_WSAAccept (
    SOCKET s,
    struct sockaddr FAR *addr,
    LPINT addrlen,
    LPCONDITIONPROC lpfnCondition,
    DWORD dwCallbackData);

int WSAAPI
DTHOOK_WSAAsyncSelect(
    SOCKET s,
    HWND hWnd,
    u_int wMsg,
    long lEvent);

int WSAAPI
DTHOOK_WSACleanup(
    void);

BOOL WSAAPI
DTHOOK_WSACloseEvent (
    WSAEVENT hEvent);

int WSAAPI
DTHOOK_WSAConnect (
    SOCKET s,
    const struct sockaddr FAR *name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS);

WSAEVENT WSAAPI
DTHOOK_WSACreateEvent (
    void);

int WSAAPI
DTHOOK_WSADuplicateSocketA (
    SOCKET s,
    DWORD dwProcessId,
    LPWSAPROTOCOL_INFOA lpProtocolInfo);

int WSAAPI
DTHOOK_WSADuplicateSocketW (
    SOCKET s,
    DWORD dwProcessId,
    LPWSAPROTOCOL_INFOW lpProtocolInfo);

int WSAAPI
DTHOOK_WSAEnumNetworkEvents (
    SOCKET s,
    WSAEVENT hEventObject,
    LPWSANETWORKEVENTS lpNetworkEvents);

int WSAAPI
DTHOOK_WSAEnumProtocolsA (
    LPINT lpiProtocols,
    LPWSAPROTOCOL_INFOA lpProtocolBuffer,
    LPDWORD lpdwBufferLength);

int WSAAPI
DTHOOK_WSAEnumProtocolsW (
    LPINT lpiProtocols,
    LPWSAPROTOCOL_INFOW lpProtocolBuffer,
    LPDWORD lpdwBufferLength);

int WSPAPI
DTHOOK_WSCEnumProtocols (
    LPINT lpiProtocols,
    LPWSAPROTOCOL_INFOW lpProtocolBuffer,
    LPDWORD lpdwBufferLength,
    LPINT  lpErrno);

int WSAAPI
DTHOOK_WSAEventSelect (
    SOCKET s,
    WSAEVENT hEventObject,
    long lNetworkEvents);

int WSAAPI
DTHOOK_WSAGetLastError(
    void);

BOOL WSAAPI
DTHOOK_WSAGetOverlappedResult (
    SOCKET s,
    LPWSAOVERLAPPED lpOverlapped,
    LPDWORD lpcbTransfer,
    BOOL fWait,
    LPDWORD lpdwFlags);

BOOL WSAAPI
DTHOOK_WSAGetQOSByName (
    SOCKET s,
    LPWSABUF lpQOSName,
    LPQOS lpQOS);

int WSAAPI
DTHOOK_WSAHtonl (
    SOCKET s,
    u_long hostlong,
    u_long FAR * lpnetlong);

int WSAAPI
DTHOOK_WSAHtons (
    SOCKET s,
    u_short hostshort,
    u_short FAR * lpnetshort);

int WSAAPI
DTHOOK_WSAIoctl (
    SOCKET s,
    DWORD dwIoControlCode,
    LPVOID lpvInBuffer,
    DWORD cbInBuffer,
    LPVOID lpvOutBuffer,
    DWORD cbOutBuffer,
    LPDWORD lpcbBytesReturned,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

SOCKET WSAAPI
DTHOOK_WSAJoinLeaf (
    SOCKET s,
    const struct sockaddr FAR * name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS,
    DWORD dwFlags);

int WSAAPI
DTHOOK_WSANtohl (
    SOCKET s,
    u_long netlong,
    u_long FAR * lphostlong);

int WSAAPI
DTHOOK_WSANtohs (
    SOCKET s,
    u_short netshort,
    u_short FAR * lphostshort);

int WSAAPI
DTHOOK_WSARecv (
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRecvd,
    LPDWORD lpFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

int WSAAPI
DTHOOK_WSARecvDisconnect (
    SOCKET s,
    LPWSABUF lpInboundDisconnectData);

int WSAAPI
DTHOOK_WSARecvFrom (
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRecvd,
    LPDWORD lpFlags,
    struct sockaddr FAR * lpFrom,
    LPINT lpFromlen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

BOOL WSAAPI
DTHOOK_WSAResetEvent (
    WSAEVENT hEvent);

int WSAAPI
DTHOOK_WSASend (
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD dwFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

int WSAAPI
DTHOOK_WSASendDisconnect (
    SOCKET s,
    LPWSABUF lpOutboundDisconnectData);

int WSAAPI
DTHOOK_WSASendTo (
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD dwFlags,
    const struct sockaddr FAR * lpTo,
    int iTolen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

BOOL WSAAPI
DTHOOK_WSASetEvent(
    WSAEVENT hEvent);

void WSAAPI
DTHOOK_WSASetLastError(
    int iError);

SOCKET WSAAPI
DTHOOK_WSASocketA(
    int af,
    int type,
    int protocol,
    LPWSAPROTOCOL_INFOA lpProtocolInfo,
    GROUP g,
    DWORD dwFlags);

SOCKET WSAAPI
DTHOOK_WSASocketW(
    int af,
    int type,
    int protocol,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    GROUP g,
    DWORD dwFlags);

int WSAAPI
DTHOOK_WSAStartup(
    WORD wVersionRequested,
    LPWSADATA lpWSAData);

DWORD WSAAPI
DTHOOK_WSAWaitForMultipleEvents(
    DWORD cEvents,
    const WSAEVENT FAR * lphEvents,
    BOOL fWaitAll,
    DWORD dwTimeout,
    BOOL fAlertable);

struct hostent FAR * WSAAPI
DTHOOK_gethostbyaddr(
    const char FAR * addr,
    int len,
    int type);

struct hostent FAR * WSAAPI
DTHOOK_gethostbyname(
    const char FAR * name);

int WSAAPI
DTHOOK_gethostname (
    char FAR * name,
    int namelen);

struct protoent FAR * WSAAPI
DTHOOK_getprotobyname(
    const char FAR * name);

struct protoent FAR * WSAAPI
DTHOOK_getprotobynumber(
    int number);

struct servent FAR * WSAAPI
DTHOOK_getservbyname(
    const char FAR * name,
    const char FAR * proto);

struct servent FAR * WSAAPI
DTHOOK_getservbyport(
    int port,
    const char FAR * proto);

HANDLE WSAAPI
DTHOOK_WSAAsyncGetHostByAddr(
    HWND hWnd,
    u_int wMsg,
    const char FAR * addr,
    int len,
    int type,
    char FAR * buf,
    int buflen);

HANDLE WSAAPI
DTHOOK_WSAAsyncGetHostByName(
    HWND hWnd,
    u_int wMsg,
    const char FAR * name,
    char FAR * buf,
    int buflen);

HANDLE WSAAPI
DTHOOK_WSAAsyncGetProtoByName(
    HWND hWnd,
    u_int wMsg,
    const char FAR * name,
    char FAR * buf,
    int buflen);

HANDLE WSAAPI
DTHOOK_WSAAsyncGetProtoByNumber(
    HWND hWnd,
    u_int wMsg,
    int number,
    char FAR * buf,
    int buflen);

HANDLE WSAAPI
DTHOOK_WSAAsyncGetServByName(
    HWND hWnd,
    u_int wMsg,
    const char FAR * name,
    const char FAR * proto,
    char FAR * buf,
    int buflen);

HANDLE WSAAPI
DTHOOK_WSAAsyncGetServByPort(
    HWND hWnd,
    u_int wMsg,
    int port,
    const char FAR * proto,
    char FAR * buf,
    int buflen);

int WSAAPI
DTHOOK_WSACancelAsyncRequest(
    HANDLE hAsyncTaskHandle);

BOOL WSPAPI
DTHOOK_WPUCloseEvent(
    WSAEVENT hEvent,
    LPINT lpErrno );

int WSPAPI
DTHOOK_WPUCloseSocketHandle(
    SOCKET s,
    LPINT lpErrno );

WSAEVENT WSPAPI
DTHOOK_WPUCreateEvent(
    LPINT lpErrno );

SOCKET WSPAPI
DTHOOK_WPUCreateSocketHandle(
    DWORD dwCatalogEntryId,
    DWORD_PTR dwContext,
    LPINT lpErrno);

SOCKET WSPAPI
DTHOOK_WPUModifyIFSHandle(
    DWORD dwCatalogEntryId,
    SOCKET ProposedHandle,
    LPINT lpErrno);

int WSPAPI
DTHOOK_WPUQueryBlockingCallback(
    DWORD dwCatalogEntryId,
    LPBLOCKINGCALLBACK FAR * lplpfnCallback,
    PDWORD_PTR lpdwContext,
    LPINT lpErrno);

int WSPAPI
DTHOOK_WPUQuerySocketHandleContext(
    SOCKET s,
    PDWORD_PTR lpContext,
    LPINT lpErrno );

int WSPAPI
DTHOOK_WPUQueueApc(
    LPWSATHREADID lpThreadId,
    LPWSAUSERAPC lpfnUserApc,
    DWORD_PTR dwContext,
    LPINT lpErrno);

BOOL WSPAPI
DTHOOK_WPUResetEvent(
    WSAEVENT hEvent,
    LPINT lpErrno);

BOOL WSPAPI
DTHOOK_WPUSetEvent(
    WSAEVENT hEvent,
    LPINT lpErrno);

int WSPAPI
DTHOOK_WSCDeinstallProvider(
    LPGUID lpProviderId,
    LPINT lpErrno);

int WSPAPI
DTHOOK_WSCInstallProvider(
    LPGUID lpProviderId,
    const WCHAR FAR * lpszProviderDllPath,
    const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
    DWORD dwNumberOfEntries,
    LPINT lpErrno);

int WSPAPI
DTHOOK_WPUGetProviderPath(
    IN     LPGUID     lpProviderId,
    OUT    WCHAR FAR * lpszProviderDllPath,
    IN OUT LPINT      lpProviderDllPathLen,
    OUT    LPINT      lpErrno
    );

BOOL WSPAPI
DTHOOK_WPUPostMessage(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

int WSPAPI
DTHOOK_WPUFDIsSet(
    SOCKET       s,
    fd_set FAR * set
    );

int WSPAPI
DTHOOK___WSAFDIsSet(
    SOCKET       s,
    fd_set FAR * set
    );

INT
WSPAPI
DTHOOK_WSAAddressToStringA(
    IN     LPSOCKADDR          lpsaAddress,
    IN     DWORD               dwAddressLength,
    IN     LPWSAPROTOCOL_INFOA lpProtocolInfo,
    IN OUT LPSTR               lpszAddressString,
    IN OUT LPDWORD             lpdwAddressStringLength
    );

INT
WSPAPI
DTHOOK_WSAAddressToStringW(
    IN     LPSOCKADDR          lpsaAddress,
    IN     DWORD               dwAddressLength,
    IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN OUT LPWSTR              lpszAddressString,
    IN OUT LPDWORD             lpdwAddressStringLength
    );

INT
WSPAPI
DTHOOK_WSAStringToAddressA(
    IN     LPSTR               AddressString,
    IN     INT                 AddressFamily,
    IN     LPWSAPROTOCOL_INFOA  lpProtocolInfo,
    IN OUT LPSOCKADDR          lpAddress,
    IN OUT LPINT               lpAddressLength
    );

INT
WSPAPI
DTHOOK_WSAStringToAddressW(
    IN     LPWSTR              AddressString,
    IN     INT                 AddressFamily,
    IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN OUT LPSOCKADDR          lpAddress,
    IN OUT LPINT               lpAddressLength
    );

INT
WSPAPI
DTHOOK_WSALookupServiceBeginA(
    IN  LPWSAQUERYSETA lpqsRestrictions,
    IN  DWORD          dwControlFlags,
    OUT LPHANDLE       lphLookup
    );

INT
WSPAPI
DTHOOK_WSALookupServiceBeginW(
    IN  LPWSAQUERYSETW lpqsRestrictions,
    IN  DWORD          dwControlFlags,
    OUT LPHANDLE       lphLookup
    );

INT
WSPAPI
DTHOOK_WSALookupServiceNextA(
    IN     HANDLE           hLookup,
    IN     DWORD            dwControlFlags,
    IN OUT LPDWORD          lpdwBufferLength,
    OUT    LPWSAQUERYSETA   lpqsResults
    );

INT
WSPAPI
DTHOOK_WSALookupServiceNextW(
    IN     HANDLE           hLookup,
    IN     DWORD            dwControlFlags,
    IN OUT LPDWORD          lpdwBufferLength,
    OUT    LPWSAQUERYSETW   lpqsResults
    );

INT
WSPAPI
DTHOOK_WSALookupServiceEnd(
    IN HANDLE  hLookup
    );

INT
WSPAPI
DTHOOK_WSAInstallServiceClassA(
    IN  LPWSASERVICECLASSINFOA   lpServiceClassInfo
    );

INT
WSPAPI
DTHOOK_WSAInstallServiceClassW(
    IN  LPWSASERVICECLASSINFOW   lpServiceClassInfo
    );

INT WSPAPI
DTHOOK_WSASetServiceA(
    IN  LPWSAQUERYSETA    lpqsRegInfo,
    IN  WSAESETSERVICEOP  essOperation,
    IN  DWORD             dwControlFlags
    );

INT WSPAPI
DTHOOK_WSASetServiceW(
    IN  LPWSAQUERYSETW    lpqsRegInfo,
    IN  WSAESETSERVICEOP  essOperation,
    IN  DWORD             dwControlFlags
    );

INT
WSPAPI
DTHOOK_WSARemoveServiceClass(
    IN  LPGUID  lpServiceClassId
    );

INT
WSPAPI
DTHOOK_WSAGetServiceClassInfoA(
    IN     LPGUID                  lpProviderId,
    IN     LPGUID                  lpServiceClassId,
    IN OUT LPDWORD                 lpdwBufSize,
    OUT    LPWSASERVICECLASSINFOA   lpServiceClassInfo
    );

INT
WSPAPI
DTHOOK_WSAGetServiceClassInfoW(
    IN     LPGUID                  lpProviderId,
    IN     LPGUID                  lpServiceClassId,
    IN OUT LPDWORD                 lpdwBufSize,
    OUT    LPWSASERVICECLASSINFOW   lpServiceClassInfo
    );

INT
WSPAPI
DTHOOK_WSAEnumNameSpaceProvidersA(
    IN OUT LPDWORD              lpdwBufferLength,
    IN     LPWSANAMESPACE_INFOA  Lpnspbuffer
    );

INT
WSPAPI
DTHOOK_WSAEnumNameSpaceProvidersW(
    IN OUT LPDWORD              lpdwBufferLength,
    IN     LPWSANAMESPACE_INFOW  Lpnspbuffer
    );

INT
WSPAPI
DTHOOK_WSAGetServiceClassNameByClassIdA(
    IN      LPGUID  lpServiceClassId,
    OUT     LPSTR   lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength
    );

INT
WSPAPI
DTHOOK_WSAGetServiceClassNameByClassIdW(
    IN      LPGUID  lpServiceClassId,
    OUT     LPWSTR   lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength
    );

INT
WSAAPI
DTHOOK_WSACancelBlockingCall(
    VOID
    );

FARPROC
WSAAPI
DTHOOK_WSASetBlockingHook(
    FARPROC lpBlockFunc
    );

INT
WSAAPI
DTHOOK_WSAUnhookBlockingHook(
    VOID
    );

BOOL
WSAAPI
DTHOOK_WSAIsBlocking(
    VOID
    );

int WSPAPI
DTHOOK_WSCGetProviderPath(
    LPGUID lpProviderId,
    WCHAR FAR * lpszProviderDllPath,
    LPINT lpProviderDllPathLen,
    LPINT lpErrno);

int WSPAPI
DTHOOK_WSCInstallNameSpace(
    LPWSTR lpszIdentifier,
    LPWSTR lpszPathName,
    DWORD dwNameSpace,
    DWORD dwVersion,
    LPGUID lpProviderId);

int WSPAPI
DTHOOK_WSCUnInstallNameSpace(
    LPGUID lpProviderId
    );

int WSPAPI
DTHOOK_WSCEnableNSProvider(
    LPGUID lpProviderId,
    BOOL fEnable
    );

#endif

} // extern "C"

int
WSPAPI
WSCWriteProviderOrder (
    IN LPDWORD lpwdCatalogEntryId,
    IN DWORD dwNumberOfEntries
    );

int
WSPAPI
WSCWriteNameSpaceOrder (
    IN LPGUID lpProviderId,
    IN DWORD dwNumberOfEntries
    );

LPVOID apfns[] =
{
#if DBG
    (LPVOID) DTHOOK_accept,
    (LPVOID) DTHOOK_bind,
    (LPVOID) DTHOOK_closesocket,
    (LPVOID) DTHOOK_connect,
    (LPVOID) DTHOOK_getpeername,
    (LPVOID) DTHOOK_getsockname,
    (LPVOID) DTHOOK_getsockopt,
    (LPVOID) DTHOOK_htonl,
    (LPVOID) DTHOOK_htons,
    (LPVOID) DTHOOK_ioctlsocket,
    (LPVOID) DTHOOK_inet_addr,
    (LPVOID) DTHOOK_inet_ntoa,
    (LPVOID) DTHOOK_listen,
    (LPVOID) DTHOOK_ntohl,
    (LPVOID) DTHOOK_ntohs,
    (LPVOID) DTHOOK_recv,
    (LPVOID) DTHOOK_recvfrom,
    (LPVOID) DTHOOK_select,
    (LPVOID) DTHOOK_send,
    (LPVOID) DTHOOK_sendto,
    (LPVOID) DTHOOK_setsockopt,
    (LPVOID) DTHOOK_shutdown,
    (LPVOID) DTHOOK_socket,
    (LPVOID) DTHOOK_gethostbyaddr,
    (LPVOID) DTHOOK_gethostbyname,
    (LPVOID) DTHOOK_getprotobyname,
    (LPVOID) DTHOOK_getprotobynumber,
    (LPVOID) DTHOOK_getservbyname,
    (LPVOID) DTHOOK_getservbyport,
    (LPVOID) DTHOOK_gethostname,
    (LPVOID) DTHOOK_WSAAsyncSelect,
    (LPVOID) DTHOOK_WSAAsyncGetHostByAddr,
    (LPVOID) DTHOOK_WSAAsyncGetHostByName,
    (LPVOID) DTHOOK_WSAAsyncGetProtoByNumber,
    (LPVOID) DTHOOK_WSAAsyncGetProtoByName,
    (LPVOID) DTHOOK_WSAAsyncGetServByPort,
    (LPVOID) DTHOOK_WSAAsyncGetServByName,
    (LPVOID) DTHOOK_WSACancelAsyncRequest,
    (LPVOID) DTHOOK_WSASetBlockingHook,
    (LPVOID) DTHOOK_WSAUnhookBlockingHook,
    (LPVOID) DTHOOK_WSAGetLastError,
    (LPVOID) DTHOOK_WSASetLastError,
    (LPVOID) DTHOOK_WSACancelBlockingCall,
    (LPVOID) DTHOOK_WSAIsBlocking,
    (LPVOID) DTHOOK_WSAStartup,
    (LPVOID) DTHOOK_WSACleanup,

    (LPVOID) DTHOOK_WSAAccept,
    (LPVOID) DTHOOK_WSACloseEvent,
    (LPVOID) DTHOOK_WSAConnect,
    (LPVOID) DTHOOK_WSACreateEvent,
    (LPVOID) DTHOOK_WSADuplicateSocketA,
    (LPVOID) DTHOOK_WSADuplicateSocketW,
    (LPVOID) DTHOOK_WSAEnumNetworkEvents,
    (LPVOID) DTHOOK_WSAEnumProtocolsA,
    (LPVOID) DTHOOK_WSAEnumProtocolsW,
    (LPVOID) DTHOOK_WSAEventSelect,
    (LPVOID) DTHOOK_WSAGetOverlappedResult,
    (LPVOID) DTHOOK_WSAGetQOSByName,
    (LPVOID) DTHOOK_WSAHtonl,
    (LPVOID) DTHOOK_WSAHtons,
    (LPVOID) DTHOOK_WSAIoctl,
    (LPVOID) DTHOOK_WSAJoinLeaf,
    (LPVOID) DTHOOK_WSANtohl,
    (LPVOID) DTHOOK_WSANtohs,
    (LPVOID) DTHOOK_WSARecv,
    (LPVOID) DTHOOK_WSARecvDisconnect,
    (LPVOID) DTHOOK_WSARecvFrom,
    (LPVOID) DTHOOK_WSAResetEvent,
    (LPVOID) DTHOOK_WSASend,
    (LPVOID) DTHOOK_WSASendDisconnect,
    (LPVOID) DTHOOK_WSASendTo,
    (LPVOID) DTHOOK_WSASetEvent,
    (LPVOID) DTHOOK_WSASocketA,
    (LPVOID) DTHOOK_WSASocketW,
    (LPVOID) DTHOOK_WSAWaitForMultipleEvents,

    (LPVOID) DTHOOK_WSAAddressToStringA,
    (LPVOID) DTHOOK_WSAAddressToStringW,
    (LPVOID) DTHOOK_WSAStringToAddressA,
    (LPVOID) DTHOOK_WSAStringToAddressW,
    (LPVOID) DTHOOK_WSALookupServiceBeginA,
    (LPVOID) DTHOOK_WSALookupServiceBeginW,
    (LPVOID) DTHOOK_WSALookupServiceNextA,
    (LPVOID) DTHOOK_WSALookupServiceNextW,
    (LPVOID) DTHOOK_WSALookupServiceEnd,
    (LPVOID) DTHOOK_WSAInstallServiceClassA,
    (LPVOID) DTHOOK_WSAInstallServiceClassW,
    (LPVOID) DTHOOK_WSARemoveServiceClass,
    (LPVOID) DTHOOK_WSAGetServiceClassInfoA,
    (LPVOID) DTHOOK_WSAGetServiceClassInfoW,
    (LPVOID) DTHOOK_WSAEnumNameSpaceProvidersA,
    (LPVOID) DTHOOK_WSAEnumNameSpaceProvidersW,
    (LPVOID) DTHOOK_WSAGetServiceClassNameByClassIdA,
    (LPVOID) DTHOOK_WSAGetServiceClassNameByClassIdW,
    (LPVOID) DTHOOK_WSASetServiceA,
    (LPVOID) DTHOOK_WSASetServiceW,

    (LPVOID) DTHOOK_WSCDeinstallProvider,
    (LPVOID) DTHOOK_WSCInstallProvider,
    (LPVOID) DTHOOK_WSCEnumProtocols,
    (LPVOID) DTHOOK_WSCGetProviderPath,
    (LPVOID) DTHOOK_WSCInstallNameSpace,
    (LPVOID) DTHOOK_WSCUnInstallNameSpace,
    (LPVOID) DTHOOK_WSCEnableNSProvider,
#else
    (LPVOID) accept,
    (LPVOID) bind,
    (LPVOID) closesocket,
    (LPVOID) connect,
    (LPVOID) getpeername,
    (LPVOID) getsockname,
    (LPVOID) getsockopt,
    (LPVOID) htonl,
    (LPVOID) htons,
    (LPVOID) ioctlsocket,
    (LPVOID) inet_addr,
    (LPVOID) inet_ntoa,
    (LPVOID) listen,
    (LPVOID) ntohl,
    (LPVOID) ntohs,
    (LPVOID) recv,
    (LPVOID) recvfrom,
    (LPVOID) select,
    (LPVOID) send,
    (LPVOID) sendto,
    (LPVOID) setsockopt,
    (LPVOID) shutdown,
    (LPVOID) socket,
    (LPVOID) gethostbyaddr,
    (LPVOID) gethostbyname,
    (LPVOID) getprotobyname,
    (LPVOID) getprotobynumber,
    (LPVOID) getservbyname,
    (LPVOID) getservbyport,
    (LPVOID) gethostname,
    (LPVOID) WSAAsyncSelect,
    (LPVOID) WSAAsyncGetHostByAddr,
    (LPVOID) WSAAsyncGetHostByName,
    (LPVOID) WSAAsyncGetProtoByNumber,
    (LPVOID) WSAAsyncGetProtoByName,
    (LPVOID) WSAAsyncGetServByPort,
    (LPVOID) WSAAsyncGetServByName,
    (LPVOID) WSACancelAsyncRequest,
    (LPVOID) WSASetBlockingHook,
    (LPVOID) WSAUnhookBlockingHook,
    (LPVOID) WSAGetLastError,
    (LPVOID) WSASetLastError,
    (LPVOID) WSACancelBlockingCall,
    (LPVOID) WSAIsBlocking,
    (LPVOID) WSAStartup,
    (LPVOID) WSACleanup,

    (LPVOID) WSAAccept,
    (LPVOID) WSACloseEvent,
    (LPVOID) WSAConnect,
    (LPVOID) WSACreateEvent,
    (LPVOID) WSADuplicateSocketA,
    (LPVOID) WSADuplicateSocketW,
    (LPVOID) WSAEnumNetworkEvents,
    (LPVOID) WSAEnumProtocolsA,
    (LPVOID) WSAEnumProtocolsW,
    (LPVOID) WSAEventSelect,
    (LPVOID) WSAGetOverlappedResult,
    (LPVOID) WSAGetQOSByName,
    (LPVOID) WSAHtonl,
    (LPVOID) WSAHtons,
    (LPVOID) WSAIoctl,
    (LPVOID) WSAJoinLeaf,
    (LPVOID) WSANtohl,
    (LPVOID) WSANtohs,
    (LPVOID) WSARecv,
    (LPVOID) WSARecvDisconnect,
    (LPVOID) WSARecvFrom,
    (LPVOID) WSAResetEvent,
    (LPVOID) WSASend,
    (LPVOID) WSASendDisconnect,
    (LPVOID) WSASendTo,
    (LPVOID) WSASetEvent,
    (LPVOID) WSASocketA,
    (LPVOID) WSASocketW,
    (LPVOID) WSAWaitForMultipleEvents,

    (LPVOID) WSAAddressToStringA,
    (LPVOID) WSAAddressToStringW,
    (LPVOID) WSAStringToAddressA,
    (LPVOID) WSAStringToAddressW,
    (LPVOID) WSALookupServiceBeginA,
    (LPVOID) WSALookupServiceBeginW,
    (LPVOID) WSALookupServiceNextA,
    (LPVOID) WSALookupServiceNextW,
    (LPVOID) WSALookupServiceEnd,
    (LPVOID) WSAInstallServiceClassA,
    (LPVOID) WSAInstallServiceClassW,
    (LPVOID) WSARemoveServiceClass,
    (LPVOID) WSAGetServiceClassInfoA,
    (LPVOID) WSAGetServiceClassInfoW,
    (LPVOID) WSAEnumNameSpaceProvidersA,
    (LPVOID) WSAEnumNameSpaceProvidersW,
    (LPVOID) WSAGetServiceClassNameByClassIdA,
    (LPVOID) WSAGetServiceClassNameByClassIdW,
    (LPVOID) WSASetServiceA,
    (LPVOID) WSASetServiceW,

    (LPVOID) WSCDeinstallProvider,
    (LPVOID) WSCInstallProvider,
    (LPVOID) WSCEnumProtocols,
    (LPVOID) WSCGetProviderPath,
    (LPVOID) WSCInstallNameSpace,
    (LPVOID) WSCUnInstallNameSpace,
    (LPVOID) WSCEnableNSProvider,
#endif
    (LPVOID) WPUCompleteOverlappedRequest,
    (LPVOID) WSAProviderConfigChange,
    (LPVOID) WSCWriteProviderOrder,
    (LPVOID) WSCWriteNameSpaceOrder
};

static char *aszFuncNames[] =
{
    "accept",
    "bind",
    "closesocket",
    "connect",
    "getpeername",
    "getsockname",
    "getsockopt",
    "htonl",
    "htons",
    "ioctlsocket",
    "inet_addr",
    "inet_ntoa",
    "listen",
    "ntohl",
    "ntohs",
    "recv",
    "recvfrom",
    "select",
    "send",
    "sendto",
    "setsockopt",
    "shutdown",
    "socket",
    "gethostbyaddr",
    "gethostbyname",
    "getprotobyname",
    "getprotobynumber",
    "getservbyname",
    "getservbyport",
    "gethostname",
    "WSAAsyncSelect",
    "WSAAsyncGetHostByAddr",
    "WSAAsyncGetHostByName",
    "WSAAsyncGetProtoByNumber",
    "WSAAsyncGetProtoByName",
    "WSAAsyncGetServByPort",
    "WSAAsyncGetServByName",
    "WSACancelAsyncRequest",
    "WSASetBlockingHook",
    "WSAUnhookBlockingHook",
    "WSAGetLastError",
    "WSASetLastError",
    "WSACancelBlockingCall",
    "WSAIsBlocking",
    "WSAStartup",
    "WSACleanup",

    "WSAAccept",
    "WSACloseEvent",
    "WSAConnect",
    "WSACreateEvent",
    "WSADuplicateSocketA",
    "WSADuplicateSocketW",
    "WSAEnumNetworkEvents",
    "WSAEnumProtocolsA",
    "WSAEnumProtocolsW",
    "WSAEventSelect",
    "WSAGetOverlappedResult",
    "WSAGetQOSByName",
    "WSAHtonl",
    "WSAHtons",
    "WSAIoctl",
    "WSAJoinLeaf",
    "WSANtohl",
    "WSANtohs",
    "WSARecv",
    "WSARecvDisconnect",
    "WSARecvFrom",
    "WSAResetEvent",
    "WSASend",
    "WSASendDisconnect",
    "WSASendTo",
    "WSASetEvent",
    "WSASocketA",
    "WSASocketW",
    "WSAWaitForMultipleEvents",

    "WSAAddressToStringA",
    "WSAAddressToStringW",
    "WSAStringToAddressA",
    "WSAStringToAddressW",
    "WSALookupServiceBeginA",
    "WSALookupServiceBeginW",
    "WSALookupServiceNextA",
    "WSALookupServiceNextW",
    "WSALookupServiceEnd",
    "WSAInstallServiceClassA",
    "WSAInstallServiceClassW",
    "WSARemoveServiceClass",
    "WSAGetServiceClassInfoA",
    "WSAGetServiceClassInfoW",
    "WSAEnumNameSpaceProvidersA",
    "WSAEnumNameSpaceProvidersW",
    "WSAGetServiceClassNameByClassIdA",
    "WSAGetServiceClassNameByClassIdW",
    "WSASetServiceA",
    "WSASetServiceW",

    "WSCDeinstallProvider",
    "WSCInstallProvider",
    "WSCEnumProtocols",
    "WSCGetProviderPath",
    "WSCInstallNameSpace",
    "WSCUnInstallNameSpace",
    "WSCEnableNSProvider",

    "WPUCompleteOverlappedRequest",

    "WSAProviderConfigChange",

    "WSCWriteProviderOrder",
    "WSCWriteNameSpaceOrder",

    NULL
};


INT
CheckForHookersOrChainers()
/*++

Routine Description:

    This procedure checks to see if there are any ws2_32 hookers or chainers
    out there, returning ERROR_SUCCESS if not or SOCKET_ERROR if so.

Arguments:

    None

Return Value:

    None
--*/
{
    LPVOID pfnXxx;
    int i;

    DEBUGF(DBG_TRACE, ("Checking for ws2_32 hookers or chainers...\n"));

    for (i = 0; aszFuncNames[i]; i++)
    {
        if (!(pfnXxx = (LPVOID) GetProcAddress (gDllHandle, aszFuncNames[i])) ||
            pfnXxx != apfns[i])
        {
            DEBUGF(DBG_ERR, ("Hooker or chainer found, failing init\n"));

            return SOCKET_ERROR;
        }
    }

    DEBUGF(DBG_TRACE, ("No ws2_32 hookers or chainers found\n"));

    return ERROR_SUCCESS;

}  // CheckForHookersOrChainers
