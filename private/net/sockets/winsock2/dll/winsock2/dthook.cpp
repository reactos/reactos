/*++

  Copyright (c) 1995 Intel Corp

  Module Name:

    dthook.cpp

  Abstract:

    This module contains the hooks that allow specially-compiled
    versions of the WinSock 2 DLL call into the debug/trace DLL.

    For each function in the WinSock 2 API, this module has a hook
    function called DT_<function>, which, if WinSock 2 is compiled
    with the DEBUG_TRACING symbol defined, is exported under the name
    of the original function.  The hook function calls into the
    debug/trace DLL through the two entry points,
    WSA[Pre|Post]ApiNotify, which are wrapped around the real call to
    the API function.  Please see the debug/trace documentation for
    more details.

  Author:

    Michael A. Grafton

--*/

#include "precomp.h"

#if defined(DEBUG_TRACING)

//
// Static Globals
//

// Function pointers to the Debug/Trace DLL entry points
static LPFNWSANOTIFY PreApiNotifyFP = NULL;
static LPFNWSANOTIFY PostApiNotifyFP = NULL;
static LPFNWSAEXCEPTIONNOTIFY ExceptionNotifyFP = NULL;

// Handle to the Debug/Trace DLL module
static HMODULE       DTDll = NULL;
// Lock that facilitates module initialization on demand
// instead of doing this inside of DLLMain
static CRITICAL_SECTION DTHookSynchronization;
static BOOL          DTHookInitialized = FALSE;

// Static string to pass to Debug/Trace notification functions --
// The value of this variable should never be changed; however, the
// keyword 'const' causes bizarre compilation errors...
static char LibName[] = "WinSock2";


//
// Goodies to make it easier to catch exceptions in WS2_32.DLL and the
// service providers.
//

extern "C" {

LONG
DtExceptionFilter(
    LPEXCEPTION_POINTERS ExceptionPointers,
    LPSTR Routine
    );

} // extern "C"

// From ntrtl.h
extern "C" {

#if defined(_M_IX86)

typedef USHORT (WINAPI * LPFNRTLCAPTURESTACKBACKTRACE) (
   IN ULONG FramesToSkip,
   IN ULONG FramesToCapture,
   OUT PVOID *BackTrace,
   OUT PULONG BackTraceHash
   );


// Stack backtrace function pointer available on NT
LPFNRTLCAPTURESTACKBACKTRACE pRtlCaptureStackBackTrace = NULL;

#define RECORD_SOCKET_CREATOR(s)                                        \
    if ((s!=INVALID_SOCKET) &&                                          \
            (pRtlCaptureStackBackTrace!=NULL)) {                        \
        PDSOCKET    socket;                                             \
        socket = DSOCKET::GetCountedDSocketFromSocketNoExport(s);       \
        if (socket!=NULL) {                                             \
            ULONG   ignore;                                             \
            pRtlCaptureStackBackTrace(                                  \
                        1,                                              \
                        sizeof (socket->m_CreatorBackTrace) /           \
                            sizeof (socket->m_CreatorBackTrace[0]),     \
                        &socket->m_CreatorBackTrace[0],                 \
                        &ignore);                                       \
            socket->DropDSocketReference ();                            \
        }                                                               \
    }
#else
#if defined(_M_ALPHA) || defined(_M_AXP64) || defined(_M_IA64)

PVOID
_ReturnAddress (
    VOID
    );

#pragma intrinsic(_ReturnAddress)

#define RECORD_SOCKET_CREATOR(s)                                        \
    if (s!=INVALID_SOCKET) {                                            \
        PDSOCKET    socket;                                             \
        socket = DSOCKET::GetCountedDSocketFromSocketNoExport(s);       \
        if (socket!=NULL) {                                             \
            socket->m_CreatorBackTrace[0] = _ReturnAddress ();          \
            socket->DropDSocketReference ();                            \
        }                                                               \
    }

#endif
#endif

} // extern "C"


VOID
DoDTHookInitialization (
    VOID
    );

#ifndef NOTHING
#define NOTHING
#endif

#define INVOKE_ROUTINE(routine)                                         \
            __try {                                                     \
                routine                                                 \
            } __except( DtExceptionFilter(                              \
                        GetExceptionInformation (),                     \
                        #routine                                        \
                        ) ) {                                           \
                NOTHING;                                                \
            }


//
// Functions
//


LPFNWSANOTIFY
GetPreApiNotifyFP(void)
/*++

  Function Description:

      Returns a pointer to the WSAPreApiNotify function exported by
      the Debug/Trace DLL.  This variable is global to this file only,
      and is initialized during DT_Initialize().

  Arguments:

      None.

  Return Value:

      Returns whatever is stored in PreApiNotifyFP.

--*/
{
    if (!DTHookInitialized) {
        DoDTHookInitialization ();
    }
    return(PreApiNotifyFP);
}





LPFNWSANOTIFY
GetPostApiNotifyFP(void)
/*++

  Function Description:

      Returns a pointer to the WSAPreApiNotify function exported by
      the Debug/Trace DLL.  This variable is global to this file only,
      and is initialized during DT_Initialize().

  Arguments:

      None.

  Return Value:

      Returns whatever is stored in PreApiNotifyFP.

--*/
{
    if (!DTHookInitialized) {
        DoDTHookInitialization ();
    }
    return(PostApiNotifyFP);
}





void
DTHookInitialize(void)
/*++

  Function Description:

      This function must be called from DLLMain to let
      this module initialize its critical section that protects
      the initialization below.

  Arguments:

      None.

  Return Value:

      None.

--*/
{
    InitializeCriticalSection (&DTHookSynchronization);
}

VOID
DoDTHookInitialization (
    VOID
    )
/*++

  Function Description:

      Intializes this hook module.  Loads the Debug/Trace DLL, if
      possible, and sets the global function pointers to point to the
      entry points exported by that DLL.  If the DLL can't be loaded,
      the function just returns and the function pointers are left at
      NULL.

      This function MUST be called before any of the hook functions
      are called, or the hook functions will not work.

  Arguments:

      None.

  Return Value:

      None.

--*/
{
    EnterCriticalSection (&DTHookSynchronization);
    if (!DTHookInitialized) {
#if defined (_M_IX86)
        //
        // If we are running on NT, get pointer to stack back
        // trace recording function and keep record of socket creators
        //
        HMODULE hNtDll;

        hNtDll = GetModuleHandle (TEXT("ntdll.dll"));
        if (hNtDll!=NULL) {
            pRtlCaptureStackBackTrace = 
                (LPFNRTLCAPTURESTACKBACKTRACE)GetProcAddress (
                                                hNtDll,
                                                "RtlCaptureStackBackTrace");
        }
#endif


        DTDll = (HMODULE)LoadLibrary("dt_dll");

        if (DTDll != NULL) {

            PreApiNotifyFP = (LPFNWSANOTIFY)GetProcAddress(
                DTDll,
                "WSAPreApiNotify");

            PostApiNotifyFP = (LPFNWSANOTIFY)GetProcAddress(
                DTDll,
                "WSAPostApiNotify");

            ExceptionNotifyFP = (LPFNWSAEXCEPTIONNOTIFY)GetProcAddress(
                DTDll,
                "WSAExceptionNotify");
        }
        DTHookInitialized = TRUE;
    }
    LeaveCriticalSection (&DTHookSynchronization);
}



void
DTHookShutdown(void)
/*++

  Function Description:

      Should be called to shutdown Debug/Tracing.  The function
      pointers are set to NULL, and the DLL is unloaded from memory.

  Arguments:

      None.

  Return Value:

      None.

--*/
{
    if ((DTDll != NULL && DTDll!=INVALID_HANDLE_VALUE)) {
        FreeLibrary(DTDll);
    }

    PreApiNotifyFP = NULL;
    PostApiNotifyFP = NULL;
    DTHookInitialized = FALSE;
    DeleteCriticalSection (&DTHookSynchronization);
}



//
// Hook Functions for WinSock2 API.
//

// This comment serves as a function comment for all the hook
// functions.  There  is  one hook function for each function exported
// by the WinSock 2 API.  Each hook  function has the exact same
// parameter profile of the corresponding API function.   Each one
// calls PREAPINOTIFY and POSTAPINOTIFY macros, which call into   the
// Debug/Trace   DLL,   if  it  was  loaded  successfully.   After
// PREAPINOTIFY,  the  hook  functions  call  the  real WS2 functions.
// See the Debug/Trace documentation for more information.
//
// Note  that  all of the debug-hook functions must be declared with C
// language binding, while the internally-used initialization and
// utility functions must not be.

extern "C" {


SOCKET WSAAPI
DTHOOK_accept (
    SOCKET s,
    struct sockaddr FAR *addr,
    int FAR *addrlen)
{
    SOCKET ReturnValue;

    if (PREAPINOTIFY((DTCODE_accept,
                       &ReturnValue,
                       LibName,
                       &s,
                       &addr,
                       &addrlen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = accept(s, addr, addrlen);
        );

    RECORD_SOCKET_CREATOR(ReturnValue);

    POSTAPINOTIFY((DTCODE_accept,
                    &ReturnValue,
                    LibName,
                    &s,
                    &addr,
                    &addrlen));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_bind (
    SOCKET s,
    const struct sockaddr FAR *name,
    int namelen)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_bind,
                       &ReturnValue,
                       LibName,
                       &s,
                       &name,
                       &namelen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = bind(s, name, namelen);
        );

    POSTAPINOTIFY((DTCODE_bind,
                    &ReturnValue,
                    LibName,
                    &s,
                    &name,
                    &namelen));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_closesocket (
    SOCKET s)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_closesocket,
                       &ReturnValue,
                       LibName,
                       &s))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = closesocket(s);
        );

    POSTAPINOTIFY((DTCODE_closesocket,
                    &ReturnValue,
                    LibName,
                    &s));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_connect (
    SOCKET s,
    const struct sockaddr FAR *name,
    int namelen)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_connect,
                       &ReturnValue,
                       LibName,
                       &s,
                       &name,
                       &namelen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = connect(s, name, namelen);
        );

    POSTAPINOTIFY((DTCODE_connect,
                    &ReturnValue,
                    LibName,
                    &s,
                    &name,
                    &namelen));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_ioctlsocket (
    SOCKET s,
    long cmd,
    u_long FAR *argp)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_ioctlsocket,
                       &ReturnValue,
                       LibName,
                       &s,
                       &cmd,
                       &argp))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = ioctlsocket(s, cmd, argp);
        );

    POSTAPINOTIFY((DTCODE_ioctlsocket,
                    &ReturnValue,
                    LibName,
                    &s,
                    &cmd,
                    &argp));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_getpeername (
    SOCKET s,
    struct sockaddr FAR *name,
    int FAR * namelen)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_getpeername,
                       &ReturnValue,
                       LibName,
                       &s,
                       &name,
                       &namelen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = getpeername(s, name, namelen);
        );

    POSTAPINOTIFY((DTCODE_getpeername,
                    &ReturnValue,
                    LibName,
                    &s,
                    &name,
                    &namelen));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_getsockname (
    SOCKET s,
    struct sockaddr FAR *name,
    int FAR * namelen)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_getsockname,
                       &ReturnValue,
                       LibName,
                       &s,
                       &name,
                       &namelen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = getsockname(s, name, namelen);
        );

    POSTAPINOTIFY((DTCODE_getsockname,
                    &ReturnValue,
                    LibName,
                    &s,
                    &name,
                    &namelen));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_getsockopt (
    SOCKET s,
    int level,
    int optname,
    char FAR * optval,
    int FAR *optlen)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_getsockopt,
                       &ReturnValue,
                       LibName,
                       &s,
                       &level,
                       &optname,
                       &optval,
                       &optlen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = getsockopt(s, level, optname, optval, optlen);
        );

    POSTAPINOTIFY((DTCODE_getsockopt,
                    &ReturnValue,
                    LibName,
                    &s,
                    &level,
                    &optname,
                    &optval,
                    &optlen));

    return(ReturnValue);
}




u_long WSAAPI
DTHOOK_htonl (
    u_long hostlong)
{
    u_long ReturnValue;

    if (PREAPINOTIFY((DTCODE_htonl,
                       &ReturnValue,
                       LibName,
                       &hostlong))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = htonl(hostlong);
        );

    POSTAPINOTIFY((DTCODE_htonl,
                    &ReturnValue,
                    LibName,
                    &hostlong));

    return(ReturnValue);
}




u_short WSAAPI
DTHOOK_htons (
    u_short hostshort)
{
    u_short ReturnValue;

    if (PREAPINOTIFY((DTCODE_htons,
                       &ReturnValue,
                       LibName,
                       &hostshort))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = htons(hostshort);
        );

    POSTAPINOTIFY((DTCODE_htons,
                    &ReturnValue,
                    LibName,
                    &hostshort));

    return(ReturnValue);
}




unsigned long WSAAPI
DTHOOK_inet_addr (
    const char FAR * cp)
{
    unsigned long ReturnValue;

    if (PREAPINOTIFY((DTCODE_inet_addr,
                       &ReturnValue,
                       LibName,
                       &cp))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = inet_addr(cp);
        );

    POSTAPINOTIFY((DTCODE_inet_addr,
                    &ReturnValue,
                    LibName,
                    &cp));

    return(ReturnValue);
}




char FAR * WSAAPI
DTHOOK_inet_ntoa (
    struct in_addr in)
{
    char FAR *ReturnValue;

    if (PREAPINOTIFY((DTCODE_inet_ntoa,
                       &ReturnValue,
                       LibName,
                       &in))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = inet_ntoa(in);
        );

    POSTAPINOTIFY((DTCODE_inet_ntoa,
                    &ReturnValue,
                    LibName,
                    &in));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_listen (
    SOCKET s,
    int backlog)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_listen,
                       &ReturnValue,
                       LibName,
                       &s,
                       &backlog))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = listen(s, backlog);
        );

    POSTAPINOTIFY((DTCODE_listen,
                    &ReturnValue,
                    LibName,
                    &s,
                    &backlog));

    return(ReturnValue);
}




u_long WSAAPI
DTHOOK_ntohl (
    u_long netlong)
{
    u_long ReturnValue;

    if (PREAPINOTIFY((DTCODE_ntohl,
                       &ReturnValue,
                       LibName,
                       &netlong))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = ntohl(netlong);
        );

    POSTAPINOTIFY((DTCODE_ntohl,
                    &ReturnValue,
                    LibName,
                    &netlong));

    return(ReturnValue);
}




u_short WSAAPI
DTHOOK_ntohs (
    u_short netshort)
{
    u_short ReturnValue;

    if (PREAPINOTIFY((DTCODE_ntohs,
                       &ReturnValue,
                       LibName,
                       &netshort))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = ntohs(netshort);
        );

    POSTAPINOTIFY((DTCODE_ntohs,
                    &ReturnValue,
                    LibName,
                    &netshort));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_recv (
    SOCKET s,
    char FAR * buf,
    int len,
    int flags)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_recv,
                       &ReturnValue,
                       LibName,
                       &s,
                       &buf,
                       &len,
                       &flags))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = recv(s, buf, len, flags);
        );

    POSTAPINOTIFY((DTCODE_recv,
                    &ReturnValue,
                    LibName,
                    &s,
                    &buf,
                    &len,
                    &flags));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_recvfrom (
    SOCKET s,
    char FAR * buf,
    int len,
    int flags,
    struct sockaddr FAR *from,
    int FAR * fromlen)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_recvfrom,
                       &ReturnValue,
                       LibName,
                       &s,
                       &buf,
                       &len,
                       &flags,
                       &from,
                       &fromlen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = recvfrom(s, buf, len, flags, from, fromlen);
        );

    POSTAPINOTIFY((DTCODE_recvfrom,
                    &ReturnValue,
                    LibName,
                    &s,
                    &buf,
                    &len,
                    &flags,
                    &from,
                    &fromlen));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_select (
    int nfds,
    fd_set FAR *readfds,
    fd_set FAR *writefds,
    fd_set FAR *exceptfds,
    const struct timeval FAR *timeout)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_select,
                       &ReturnValue,
                       LibName,
                       &nfds,
                       &readfds,
                       &writefds,
                       &exceptfds,
                       &timeout))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = select(nfds, readfds, writefds, exceptfds, timeout);
        );

    POSTAPINOTIFY((DTCODE_select,
                    &ReturnValue,
                    LibName,
                    &nfds,
                    &readfds,
                    &writefds,
                    &exceptfds,
                    &timeout));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_send (
    SOCKET s,
    const char FAR * buf,
    int len,
    int flags)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_send,
                       &ReturnValue,
                       LibName,
                       &s,
                       &buf,
                       &len,
                       &flags))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = send(s, buf, len, flags);
        );

    POSTAPINOTIFY((DTCODE_send,
                    &ReturnValue,
                    LibName,
                    &s,
                    &buf,
                    &len,
                    &flags));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_sendto (
    SOCKET s,
    const char FAR * buf,
    int len,
    int flags,
    const struct sockaddr FAR *to,
    int tolen)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_sendto,
                       &ReturnValue,
                       LibName,
                       &s,
                       &buf,
                       &len,
                       &flags,
                       &to,
                       &tolen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = sendto(s, buf, len, flags, to, tolen);
        );

    POSTAPINOTIFY((DTCODE_sendto,
                    &ReturnValue,
                    LibName,
                    &s,
                    &buf,
                    &len,
                    &flags,
                    &to,
                    &tolen));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_setsockopt (
    SOCKET s,
    int level,
    int optname,
    const char FAR * optval,
    int optlen)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_setsockopt,
                       &ReturnValue,
                       LibName,
                       &s,
                       &level,
                       &optname,
                       &optval,
                       &optlen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = setsockopt(s, level, optname, optval, optlen);
        );

    POSTAPINOTIFY((DTCODE_setsockopt,
                    &ReturnValue,
                    LibName,
                    &s,
                    &level,
                    &optname,
                    &optval,
                    &optlen));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_shutdown (
    SOCKET s,
    int how)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_shutdown,
                       &ReturnValue,
                       LibName,
                       &s,
                       &how))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = shutdown(s, how);
        );

    POSTAPINOTIFY((DTCODE_shutdown,
                    &ReturnValue,
                    LibName,
                    &s,
                    &how));

    return(ReturnValue);
}




SOCKET WSAAPI
DTHOOK_socket (
    int af,
    int type,
    int protocol)
{
    SOCKET ReturnValue;

    if (PREAPINOTIFY((DTCODE_socket,
                       &ReturnValue,
                       LibName,
                       &af,
                       &type,
                       &protocol))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = socket(af, type, protocol);
        );

    RECORD_SOCKET_CREATOR(ReturnValue);

    POSTAPINOTIFY((DTCODE_socket,
                    &ReturnValue,
                    LibName,
                    &af,
                    &type,
                    &protocol));

    return(ReturnValue);
}




SOCKET WSAAPI
DTHOOK_WSAAccept (
    SOCKET s,
    struct sockaddr FAR *addr,
    LPINT addrlen,
    LPCONDITIONPROC lpfnCondition,
    DWORD dwCallbackData)
{
    SOCKET ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAAccept,
                       &ReturnValue,
                       LibName,
                       &s,
                       &addr,
                       &addrlen,
                       &lpfnCondition,
                       &dwCallbackData))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAAccept(s, addr, addrlen, lpfnCondition,
                                dwCallbackData);
        );

    RECORD_SOCKET_CREATOR(ReturnValue);

    POSTAPINOTIFY((DTCODE_WSAAccept,
                    &ReturnValue,
                    LibName,
                    &s,
                    &addr,
                    &addrlen,
                    &lpfnCondition,
                    &dwCallbackData));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSAAsyncSelect(
    SOCKET s,
    HWND hWnd,
    u_int wMsg,
    long lEvent)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAAsyncSelect,
                       &ReturnValue,
                       LibName,
                       &s,
                       &hWnd,
                       &wMsg,
                       &lEvent))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAAsyncSelect(s, hWnd, wMsg, lEvent);
        );

    POSTAPINOTIFY((DTCODE_WSAAsyncSelect,
                    &ReturnValue,
                    LibName,
                    &s,
                    &hWnd,
                    &wMsg,
                    &lEvent));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSACleanup(
    void)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSACleanup,
                       &ReturnValue,
                       LibName))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSACleanup();
        );

    POSTAPINOTIFY((DTCODE_WSACleanup,
                    &ReturnValue,
                    LibName));

    return(ReturnValue);
}




BOOL WSAAPI
DTHOOK_WSACloseEvent (
    WSAEVENT hEvent)
{
    BOOL ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSACloseEvent,
                       &ReturnValue,
                       LibName,
                       &hEvent))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSACloseEvent(hEvent);
        );

    POSTAPINOTIFY((DTCODE_WSACloseEvent,
                    &ReturnValue,
                    LibName,
                    &hEvent));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSAConnect (
    SOCKET s,
    const struct sockaddr FAR *name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAConnect,
                       &ReturnValue,
                       LibName,
                       &s,
                       &name,
                       &namelen,
                       &lpCallerData,
                       &lpCalleeData,
                       &lpSQOS,
                       &lpGQOS))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAConnect(s, name, namelen, lpCallerData, lpCalleeData,
                                 lpSQOS, lpGQOS);
        );

    POSTAPINOTIFY((DTCODE_WSAConnect,
                    &ReturnValue,
                    LibName,
                    &s,
                    &name,
                    &namelen,
                    &lpCallerData,
                    &lpCalleeData,
                    &lpSQOS,
                    &lpGQOS));

    return(ReturnValue);
}




WSAEVENT WSAAPI
DTHOOK_WSACreateEvent (
    void)
{
    WSAEVENT ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSACreateEvent,
                       &ReturnValue,
                       LibName))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSACreateEvent();
        );

    POSTAPINOTIFY((DTCODE_WSACreateEvent,
                    &ReturnValue,
                    LibName));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSADuplicateSocketA (
    SOCKET s,
    DWORD dwProcessId,
    LPWSAPROTOCOL_INFOA lpProtocolInfo)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSADuplicateSocketA,
                       &ReturnValue,
                       LibName,
                       &s,
                       &dwProcessId,
                       &lpProtocolInfo))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSADuplicateSocketA(s, dwProcessId, lpProtocolInfo);
        );

    POSTAPINOTIFY((DTCODE_WSADuplicateSocketA,
                    &ReturnValue,
                    LibName,
                    &s,
                    &dwProcessId,
                    &lpProtocolInfo));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSADuplicateSocketW (
    SOCKET s,
    DWORD dwProcessId,
    LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSADuplicateSocketW,
                       &ReturnValue,
                       LibName,
                       &s,
                       &dwProcessId,
                       &lpProtocolInfo))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSADuplicateSocketW(s, dwProcessId, lpProtocolInfo);
        );

    POSTAPINOTIFY((DTCODE_WSADuplicateSocketW,
                    &ReturnValue,
                    LibName,
                    &s,
                    &dwProcessId,
                    &lpProtocolInfo));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSAEnumNetworkEvents (
    SOCKET s,
    WSAEVENT hEventObject,
    LPWSANETWORKEVENTS lpNetworkEvents)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAEnumNetworkEvents,
                       &ReturnValue,
                       LibName,
                       &s,
                       &hEventObject,
                       &lpNetworkEvents))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAEnumNetworkEvents(s, hEventObject, lpNetworkEvents);
        );

    POSTAPINOTIFY((DTCODE_WSAEnumNetworkEvents,
                    &ReturnValue,
                    LibName,
                    &s,
                    &hEventObject,
                    &lpNetworkEvents));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSAEnumProtocolsA (
    LPINT lpiProtocols,
    LPWSAPROTOCOL_INFOA lpProtocolBuffer,
    LPDWORD lpdwBufferLength)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAEnumProtocolsA,
                       &ReturnValue,
                       LibName,
                       &lpiProtocols,
                       &lpProtocolBuffer,
                       &lpdwBufferLength))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAEnumProtocolsA(lpiProtocols, lpProtocolBuffer,
                                        lpdwBufferLength);
        );

    POSTAPINOTIFY((DTCODE_WSAEnumProtocolsA,
                    &ReturnValue,
                    LibName,
                    &lpiProtocols,
                    &lpProtocolBuffer,
                    &lpdwBufferLength));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSAEnumProtocolsW (
    LPINT lpiProtocols,
    LPWSAPROTOCOL_INFOW lpProtocolBuffer,
    LPDWORD lpdwBufferLength)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAEnumProtocolsW,
                       &ReturnValue,
                       LibName,
                       &lpiProtocols,
                       &lpProtocolBuffer,
                       &lpdwBufferLength))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAEnumProtocolsW(lpiProtocols, lpProtocolBuffer,
                                        lpdwBufferLength);
        );

    POSTAPINOTIFY((DTCODE_WSAEnumProtocolsW,
                    &ReturnValue,
                    LibName,
                    &lpiProtocols,
                    &lpProtocolBuffer,
                    &lpdwBufferLength));

    return(ReturnValue);
}





int WSPAPI
DTHOOK_WSCEnumProtocols (
    LPINT lpiProtocols,
    LPWSAPROTOCOL_INFOW lpProtocolBuffer,
    LPDWORD lpdwBufferLength,
    LPINT  lpErrno)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSCEnumProtocols,
                       &ReturnValue,
                       LibName,
                       &lpiProtocols,
                       &lpProtocolBuffer,
                       &lpdwBufferLength,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSCEnumProtocols(lpiProtocols, lpProtocolBuffer,
                                       lpdwBufferLength, lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WSCEnumProtocols,
                    &ReturnValue,
                    LibName,
                    &lpiProtocols,
                    &lpProtocolBuffer,
                    &lpdwBufferLength,
                    &lpErrno));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSAEventSelect (
    SOCKET s,
    WSAEVENT hEventObject,
    long lNetworkEvents)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAEventSelect,
                       &ReturnValue,
                       LibName,
                       &s,
                       &hEventObject,
                       &lNetworkEvents))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAEventSelect(s, hEventObject, lNetworkEvents);
        );

    POSTAPINOTIFY((DTCODE_WSAEventSelect,
                    &ReturnValue,
                    LibName,
                    &s,
                    &hEventObject,
                    &lNetworkEvents));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSAGetLastError(
    void)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAGetLastError,
                       &ReturnValue,
                       LibName))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAGetLastError();
        );

    POSTAPINOTIFY((DTCODE_WSAGetLastError,
                    &ReturnValue,
                    LibName));

    return(ReturnValue);
}




BOOL WSAAPI
DTHOOK_WSAGetOverlappedResult (
    SOCKET s,
    LPWSAOVERLAPPED lpOverlapped,
    LPDWORD lpcbTransfer,
    BOOL fWait,
    LPDWORD lpdwFlags)
{
    BOOL ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAGetOverlappedResult,
                       &ReturnValue,
                       LibName,
                       &s,
                       &lpOverlapped,
                       &lpcbTransfer,
                       &fWait,
                       &lpdwFlags))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAGetOverlappedResult(s, lpOverlapped, lpcbTransfer,
                                             fWait, lpdwFlags);
        );

    POSTAPINOTIFY((DTCODE_WSAGetOverlappedResult,
                    &ReturnValue,
                    LibName,
                    &s,
                    &lpOverlapped,
                    &lpcbTransfer,
                    &fWait,
                    &lpdwFlags));

    return(ReturnValue);
}




BOOL WSAAPI
DTHOOK_WSAGetQOSByName (
    SOCKET s,
    LPWSABUF lpQOSName,
    LPQOS lpQOS)
{
    BOOL ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAGetQOSByName,
                       &ReturnValue,
                       LibName,
                       &s,
                       &lpQOSName,
                       &lpQOS))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAGetQOSByName(s, lpQOSName, lpQOS);
        );

    POSTAPINOTIFY((DTCODE_WSAGetQOSByName,
                    &ReturnValue,
                    LibName,
                    &s,
                    &lpQOSName,
                    &lpQOS));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSAHtonl (
    SOCKET s,
    u_long hostlong,
    u_long FAR * lpnetlong)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAHtonl,
                       &ReturnValue,
                       LibName,
                       &s,
                       &hostlong,
                       &lpnetlong))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAHtonl(s, hostlong, lpnetlong);
        );

    POSTAPINOTIFY((DTCODE_WSAHtonl,
                    &ReturnValue,
                    LibName,
                    &s,
                    &hostlong,
                    &lpnetlong));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSAHtons (
    SOCKET s,
    u_short hostshort,
    u_short FAR * lpnetshort)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAHtons,
                       &ReturnValue,
                       LibName,
                       &s,
                       &hostshort,
                       &lpnetshort))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAHtons(s, hostshort, lpnetshort);
        );

    POSTAPINOTIFY((DTCODE_WSAHtons,
                    &ReturnValue,
                    LibName,
                    &s,
                    &hostshort,
                    &lpnetshort));

    return(ReturnValue);
}




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
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAIoctl,
                       &ReturnValue,
                       LibName,
                       &s,
                       &dwIoControlCode,
                       &lpvInBuffer,
                       &cbInBuffer,
                       &lpvOutBuffer,
                       &cbOutBuffer,
                       &lpcbBytesReturned,
                       &lpOverlapped,
                       &lpCompletionRoutine))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAIoctl(s, dwIoControlCode, lpvInBuffer, cbInBuffer,
                               lpvOutBuffer, cbOutBuffer, lpcbBytesReturned,
                               lpOverlapped, lpCompletionRoutine);
        );

    POSTAPINOTIFY((DTCODE_WSAIoctl,
                    &ReturnValue,
                    LibName,
                    &s,
                    &dwIoControlCode,
                    &lpvInBuffer,
                    &cbInBuffer,
                    &lpvOutBuffer,
                    &cbOutBuffer,
                    &lpcbBytesReturned,
                    &lpOverlapped,
                    &lpCompletionRoutine));

    return(ReturnValue);
}




SOCKET WSAAPI
DTHOOK_WSAJoinLeaf (
    SOCKET s,
    const struct sockaddr FAR * name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS,
    DWORD dwFlags)
{
    SOCKET ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAJoinLeaf,
                       &ReturnValue,
                       LibName,
                       &s,
                       &name,
                       &namelen,
                       &lpCallerData,
                       &lpCalleeData,
                       &lpSQOS,
                       &lpGQOS,
                       &dwFlags))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAJoinLeaf(s, name, namelen, lpCallerData, lpCalleeData,
                                  lpSQOS, lpGQOS, dwFlags);
        );

    RECORD_SOCKET_CREATOR(ReturnValue);

    POSTAPINOTIFY((DTCODE_WSAJoinLeaf,
                    &ReturnValue,
                    LibName,
                    &s,
                    &name,
                    &namelen,
                    &lpCallerData,
                    &lpCalleeData,
                    &lpSQOS,
                    &lpGQOS,
                    &dwFlags));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSANtohl (
    SOCKET s,
    u_long netlong,
    u_long FAR * lphostlong)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSANtohl,
                       &ReturnValue,
                       LibName,
                       &s,
                       &netlong,
                       &lphostlong))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSANtohl(s, netlong, lphostlong);
        );

    POSTAPINOTIFY((DTCODE_WSANtohl,
                    &ReturnValue,
                    LibName,
                    &s,
                    &netlong,
                    &lphostlong));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSANtohs (
    SOCKET s,
    u_short netshort,
    u_short FAR * lphostshort)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSANtohs,
                       &ReturnValue,
                       LibName,
                       &s,
                       &netshort,
                       &lphostshort))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSANtohs(s, netshort, lphostshort);
        );

    POSTAPINOTIFY((DTCODE_WSANtohs,
                    &ReturnValue,
                    LibName,
                    &s,
                    &netshort,
                    &lphostshort));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSARecv (
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRecvd,
    LPDWORD lpFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSARecv,
                       &ReturnValue,
                       LibName,
                       &s,
                       &lpBuffers,
                       &dwBufferCount,
                       &lpNumberOfBytesRecvd,
                       &lpFlags,
                       &lpOverlapped,
                       &lpCompletionRoutine))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd,
                              lpFlags, lpOverlapped, lpCompletionRoutine);
        );

    POSTAPINOTIFY((DTCODE_WSARecv,
                    &ReturnValue,
                    LibName,
                    &s,
                    &lpBuffers,
                    &dwBufferCount,
                    &lpNumberOfBytesRecvd,
                    &lpFlags,
                    &lpOverlapped,
                    &lpCompletionRoutine));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSARecvDisconnect (
    SOCKET s,
    LPWSABUF lpInboundDisconnectData)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSARecvDisconnect,
                       &ReturnValue,
                       LibName,
                       &s,
                       &lpInboundDisconnectData))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSARecvDisconnect(s, lpInboundDisconnectData);
        );

    POSTAPINOTIFY((DTCODE_WSARecvDisconnect,
                    &ReturnValue,
                    LibName,
                    &s,
                    &lpInboundDisconnectData));

    return(ReturnValue);
}




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
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSARecvFrom,
                       &ReturnValue,
                       LibName,
                       &s,
                       &lpBuffers,
                       &dwBufferCount,
                       &lpNumberOfBytesRecvd,
                       &lpFlags,
                       &lpFrom,
                       &lpFromlen,
                       &lpOverlapped,
                       &lpCompletionRoutine))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSARecvFrom(s, lpBuffers, dwBufferCount,
                                  lpNumberOfBytesRecvd, lpFlags, lpFrom,
                                  lpFromlen, lpOverlapped,
                                  lpCompletionRoutine);
        );

    POSTAPINOTIFY((DTCODE_WSARecvFrom,
                    &ReturnValue,
                    LibName,
                    &s,
                    &lpBuffers,
                    &dwBufferCount,
                    &lpNumberOfBytesRecvd,
                    &lpFlags,
                    &lpFrom,
                    &lpFromlen,
                    &lpOverlapped,
                    &lpCompletionRoutine));

    return(ReturnValue);
}




BOOL WSAAPI
DTHOOK_WSAResetEvent (
    WSAEVENT hEvent)
{
    BOOL ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAResetEvent,
                       &ReturnValue,
                       LibName,
                       &hEvent))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAResetEvent(hEvent);
        );

    POSTAPINOTIFY((DTCODE_WSAResetEvent,
                    &ReturnValue,
                    LibName,
                    &hEvent));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSASend (
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD dwFlags,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSASend,
                       &ReturnValue,
                       LibName,
                       &s,
                       &lpBuffers,
                       &dwBufferCount,
                       &lpNumberOfBytesSent,
                       &dwFlags,
                       &lpOverlapped,
                       &lpCompletionRoutine))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSASend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent,
                              dwFlags, lpOverlapped, lpCompletionRoutine);
        );

    POSTAPINOTIFY((DTCODE_WSASend,
                    &ReturnValue,
                    LibName,
                    &s,
                    &lpBuffers,
                    &dwBufferCount,
                    &lpNumberOfBytesSent,
                    &dwFlags,
                    &lpOverlapped,
                    &lpCompletionRoutine));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSASendDisconnect (
    SOCKET s,
    LPWSABUF lpOutboundDisconnectData)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSASendDisconnect,
                       &ReturnValue,
                       LibName,
                       &s,
                       &lpOutboundDisconnectData))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSASendDisconnect(s, lpOutboundDisconnectData);
        );

    POSTAPINOTIFY((DTCODE_WSASendDisconnect,
                    &ReturnValue,
                    LibName,
                    &s,
                    &lpOutboundDisconnectData));

    return(ReturnValue);
}




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
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSASendTo,
                       &ReturnValue,
                       LibName,
                       &s,
                       &lpBuffers,
                       &dwBufferCount,
                       &lpNumberOfBytesSent,
                       &dwFlags,
                       &lpTo,
                       &iTolen,
                       &lpOverlapped,
                       &lpCompletionRoutine))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSASendTo(s, lpBuffers, dwBufferCount,
                                lpNumberOfBytesSent, dwFlags, lpTo, iTolen,
                                lpOverlapped, lpCompletionRoutine);
        );

    POSTAPINOTIFY((DTCODE_WSASendTo,
                    &ReturnValue,
                    LibName,
                    &s,
                    &lpBuffers,
                    &dwBufferCount,
                    &lpNumberOfBytesSent,
                    &dwFlags,
                    &lpTo,
                    &iTolen,
                    &lpOverlapped,
                    &lpCompletionRoutine));

    return(ReturnValue);
}




BOOL WSAAPI
DTHOOK_WSASetEvent(
    WSAEVENT hEvent)
{
    BOOL ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSASetEvent,
                       &ReturnValue,
                       LibName,
                       &hEvent))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSASetEvent(hEvent);
        );

    POSTAPINOTIFY((DTCODE_WSASetEvent,
                    &ReturnValue,
                    LibName,
                    &hEvent));

    return(ReturnValue);
}




void WSAAPI
DTHOOK_WSASetLastError(
    int iError)
{
    if (PREAPINOTIFY((DTCODE_WSASetLastError,
                       NULL,
                       LibName,
                       &iError))) {

        return;
    }

    WSASetLastError(iError);

    POSTAPINOTIFY((DTCODE_WSASetLastError,
                    NULL,
                    LibName,
                    &iError));

    return;
}




SOCKET WSAAPI
DTHOOK_WSASocketA(
    int af,
    int type,
    int protocol,
    LPWSAPROTOCOL_INFOA lpProtocolInfo,
    GROUP g,
    DWORD dwFlags)
{

    SOCKET ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSASocketA,
                       &ReturnValue,
                       LibName,
                       &af,
                       &type,
                       &protocol,
                       &lpProtocolInfo,
                       &g,
                       &dwFlags))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSASocketA(af, type, protocol, lpProtocolInfo, g,
                                 dwFlags);
        );

    RECORD_SOCKET_CREATOR(ReturnValue);

    POSTAPINOTIFY((DTCODE_WSASocketA,
                    &ReturnValue,
                    LibName,
                    &af,
                    &type,
                    &protocol,
                    &lpProtocolInfo,
                    &g,
                    &dwFlags));

    return(ReturnValue);
}




SOCKET WSAAPI
DTHOOK_WSASocketW(
    int af,
    int type,
    int protocol,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    GROUP g,
    DWORD dwFlags)
{

    SOCKET ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSASocketW,
                       &ReturnValue,
                       LibName,
                       &af,
                       &type,
                       &protocol,
                       &lpProtocolInfo,
                       &g,
                       &dwFlags))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSASocketW(af, type, protocol, lpProtocolInfo, g,
                                 dwFlags);
        );

   RECORD_SOCKET_CREATOR(ReturnValue);

   POSTAPINOTIFY((DTCODE_WSASocketW,
                    &ReturnValue,
                    LibName,
                    &af,
                    &type,
                    &protocol,
                    &lpProtocolInfo,
                    &g,
                    &dwFlags));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSAStartup(
    WORD wVersionRequested,
    LPWSADATA lpWSAData)
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAStartup,
                       &ReturnValue,
                       LibName,
                       &wVersionRequested,
                       &lpWSAData))) {
        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAStartup(wVersionRequested, lpWSAData);
        );

    POSTAPINOTIFY((DTCODE_WSAStartup,
                    &ReturnValue,
                    LibName,
                    &wVersionRequested,
                    &lpWSAData));

    return(ReturnValue);
}




DWORD WSAAPI
DTHOOK_WSAWaitForMultipleEvents(
    DWORD cEvents,
    const WSAEVENT FAR * lphEvents,
    BOOL fWaitAll,
    DWORD dwTimeout,
    BOOL fAlertable)
{

    DWORD ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAWaitForMultipleEvents,
                       &ReturnValue,
                       LibName,
                       &cEvents,
                       &lphEvents,
                       &fWaitAll,
                       &dwTimeout,
                       &fAlertable))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAWaitForMultipleEvents(cEvents, lphEvents, fWaitAll,
                                               dwTimeout, fAlertable);
        );

    POSTAPINOTIFY((DTCODE_WSAWaitForMultipleEvents,
                    &ReturnValue,
                    LibName,
                    &cEvents,
                    &lphEvents,
                    &fWaitAll,
                    &dwTimeout,
                    &fAlertable));

    return(ReturnValue);
}




struct hostent FAR * WSAAPI
DTHOOK_gethostbyaddr(
    const char FAR * addr,
    int len,
    int type)
{

    struct hostent FAR *ReturnValue;

    if (PREAPINOTIFY((DTCODE_gethostbyaddr,
                       &ReturnValue,
                       LibName,
                       &addr,
                       &len,
                       &type))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = gethostbyaddr(addr, len, type);
        );

    POSTAPINOTIFY((DTCODE_gethostbyaddr,
                    &ReturnValue,
                    LibName,
                    &addr,
                    &len,
                    &type));

    return(ReturnValue);
}




struct hostent FAR * WSAAPI
DTHOOK_gethostbyname(
    const char FAR * name)
{

    struct hostent FAR *ReturnValue;

    if (PREAPINOTIFY((DTCODE_gethostbyname,
                       &ReturnValue,
                       LibName,
                       &name))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = gethostbyname(name);
        );

    POSTAPINOTIFY((DTCODE_gethostbyname,
                    &ReturnValue,
                    LibName,
                    &name));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_gethostname (
    char FAR * name,
    int namelen)
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_gethostname,
                       &ReturnValue,
                       LibName,
                       &name,
                       &namelen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = gethostname(name, namelen);
        );

    POSTAPINOTIFY((DTCODE_gethostname,
                    &ReturnValue,
                    LibName,
                    &name,
                    &namelen));

    return(ReturnValue);
}




struct protoent FAR * WSAAPI
DTHOOK_getprotobyname(
    const char FAR * name)
{

    struct protoent FAR *ReturnValue;

    if (PREAPINOTIFY((DTCODE_getprotobyname,
                       &ReturnValue,
                       LibName,
                       &name))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = getprotobyname(name);
        );

    POSTAPINOTIFY((DTCODE_getprotobyname,
                    &ReturnValue,
                    LibName,
                    &name));

    return(ReturnValue);
}




struct protoent FAR * WSAAPI
DTHOOK_getprotobynumber(
    int number)
{

    struct protoent FAR *ReturnValue;

    if (PREAPINOTIFY((DTCODE_getprotobynumber,
                       &ReturnValue,
                       LibName,
                       &number))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = getprotobynumber(number);
        );

    POSTAPINOTIFY((DTCODE_getprotobynumber,
                    &ReturnValue,
                    LibName,
                    &number));

    return(ReturnValue);
}




struct servent FAR * WSAAPI
DTHOOK_getservbyname(
    const char FAR * name,
    const char FAR * proto)
{

    struct servent FAR *ReturnValue;

    if (PREAPINOTIFY((DTCODE_getservbyname,
                       &ReturnValue,
                       LibName,
                       &name,
                       &proto))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = getservbyname(name, proto);
        );

    POSTAPINOTIFY((DTCODE_getservbyname,
                    &ReturnValue,
                    LibName,
                    &name,
                    &proto));

    return(ReturnValue);
}




struct servent FAR * WSAAPI
DTHOOK_getservbyport(
    int port,
    const char FAR * proto)
{

    struct servent FAR *ReturnValue;

    if (PREAPINOTIFY((DTCODE_getservbyport,
                       &ReturnValue,
                       LibName,
                       &port,
                       &proto))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = getservbyport(port, proto);
        );

    POSTAPINOTIFY((DTCODE_getservbyport,
                    &ReturnValue,
                    LibName,
                    &port,
                    &proto));

    return(ReturnValue);
}




HANDLE WSAAPI
DTHOOK_WSAAsyncGetHostByAddr(
    HWND hWnd,
    u_int wMsg,
    const char FAR * addr,
    int len,
    int type,
    char FAR * buf,
    int buflen)
{

    HANDLE ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAAsyncGetHostByAddr,
                       &ReturnValue,
                       LibName,
                       &hWnd,
                       &wMsg,
                       &addr,
                       &len,
                       &type,
                       &buf,
                       &buflen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAAsyncGetHostByAddr(hWnd, wMsg, addr, len, type, buf,
                                            buflen);
        );

    POSTAPINOTIFY((DTCODE_WSAAsyncGetHostByAddr,
                    &ReturnValue,
                    LibName,
                    &hWnd,
                    &wMsg,
                    &addr,
                    &len,
                    &type,
                    &buf,
                    &buflen));

    return(ReturnValue);
}




HANDLE WSAAPI
DTHOOK_WSAAsyncGetHostByName(
    HWND hWnd,
    u_int wMsg,
    const char FAR * name,
    char FAR * buf,
    int buflen)
{

    HANDLE ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAAsyncGetHostByName,
                       &ReturnValue,
                       LibName,
                       &hWnd,
                       &wMsg,
                       &name,
                       &buf,
                       &buflen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAAsyncGetHostByName(hWnd, wMsg, name, buf, buflen);
        );

    POSTAPINOTIFY((DTCODE_WSAAsyncGetHostByName,
                    &ReturnValue,
                    LibName,
                    &hWnd,
                    &wMsg,
                    &name,
                    &buf,
                    &buflen));

    return(ReturnValue);
}




HANDLE WSAAPI
DTHOOK_WSAAsyncGetProtoByName(
    HWND hWnd,
    u_int wMsg,
    const char FAR * name,
    char FAR * buf,
    int buflen)
{

    HANDLE ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAAsyncGetProtoByName,
                       &ReturnValue,
                       LibName,
                       &hWnd,
                       &wMsg,
                       &name,
                       &buf,
                       &buflen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAAsyncGetProtoByName(hWnd, wMsg, name, buf, buflen);
        );

    POSTAPINOTIFY((DTCODE_WSAAsyncGetProtoByName,
                    &ReturnValue,
                    LibName,
                    &hWnd,
                    &wMsg,
                    &name,
                    &buf,
                    &buflen));

    return(ReturnValue);
}




HANDLE WSAAPI
DTHOOK_WSAAsyncGetProtoByNumber(
    HWND hWnd,
    u_int wMsg,
    int number,
    char FAR * buf,
    int buflen)
{

    HANDLE ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAAsyncGetProtoByNumber,
                       &ReturnValue,
                       LibName,
                       &hWnd,
                       &wMsg,
                       &number,
                       &buf,
                       &buflen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAAsyncGetProtoByNumber(hWnd, wMsg, number, buf,
                                               buflen);
        );

    POSTAPINOTIFY((DTCODE_WSAAsyncGetProtoByNumber,
                    &ReturnValue,
                    LibName,
                    &hWnd,
                    &wMsg,
                    &number,
                    &buf,
                    &buflen));

    return(ReturnValue);
}




HANDLE WSAAPI
DTHOOK_WSAAsyncGetServByName(
    HWND hWnd,
    u_int wMsg,
    const char FAR * name,
    const char FAR * proto,
    char FAR * buf,
    int buflen)
{

    HANDLE ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAAsyncGetServByName,
                       &ReturnValue,
                       LibName,
                       &hWnd,
                       &wMsg,
                       &name,
                       &proto,
                       &buf,
                       &buflen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAAsyncGetServByName(hWnd, wMsg, name, proto, buf,
                                            buflen);
        );

    POSTAPINOTIFY((DTCODE_WSAAsyncGetServByName,
                    &ReturnValue,
                    LibName,
                    &hWnd,
                    &wMsg,
                    &name,
                    &proto,
                    &buf,
                    &buflen));

    return(ReturnValue);
}




HANDLE WSAAPI
DTHOOK_WSAAsyncGetServByPort(
    HWND hWnd,
    u_int wMsg,
    int port,
    const char FAR * proto,
    char FAR * buf,
    int buflen)
{

    HANDLE ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAAsyncGetServByPort,
                       &ReturnValue,
                       LibName,
                       &hWnd,
                       &wMsg,
                       &port,
                       &proto,
                       &buf,
                       &buflen))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAAsyncGetServByPort(hWnd, wMsg, port, proto, buf,
                                            buflen);
        );

    POSTAPINOTIFY((DTCODE_WSAAsyncGetServByPort,
                    &ReturnValue,
                    LibName,
                    &hWnd,
                    &wMsg,
                    &port,
                    &proto,
                    &buf,
                    &buflen));

    return(ReturnValue);
}




int WSAAPI
DTHOOK_WSACancelAsyncRequest(
    HANDLE hAsyncTaskHandle)
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSACancelAsyncRequest,
                       &ReturnValue,
                       LibName,
                       &hAsyncTaskHandle))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSACancelAsyncRequest(hAsyncTaskHandle);
        );

    POSTAPINOTIFY((DTCODE_WSACancelAsyncRequest,
                    &ReturnValue,
                    LibName,
                    &hAsyncTaskHandle));

    return(ReturnValue);
}




BOOL WSPAPI
DTHOOK_WPUCloseEvent(
    WSAEVENT hEvent,
    LPINT lpErrno )
{

    BOOL ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUCloseEvent,
                       &ReturnValue,
                       LibName,
                       &hEvent,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUCloseEvent(hEvent, lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WPUCloseEvent,
                    &ReturnValue,
                    LibName,
                    &hEvent,
                    &lpErrno));

    return(ReturnValue);
}




int WSPAPI
DTHOOK_WPUCloseSocketHandle(
    SOCKET s,
    LPINT lpErrno )
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUCloseSocketHandle,
                       &ReturnValue,
                       LibName,
                       &s,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUCloseSocketHandle(s, lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WPUCloseSocketHandle,
                    &ReturnValue,
                    LibName,
                    &s,
                    &lpErrno));

    return(ReturnValue);
}




WSAEVENT WSPAPI
DTHOOK_WPUCreateEvent(
    LPINT lpErrno )
{

    WSAEVENT ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUCreateEvent,
                       &ReturnValue,
                       LibName,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUCreateEvent(lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WPUCreateEvent,
                    &ReturnValue,
                    LibName,
                    &lpErrno));

    return(ReturnValue);
}




SOCKET WSPAPI
DTHOOK_WPUCreateSocketHandle(
    DWORD dwCatalogEntryId,
    DWORD_PTR dwContext,
    LPINT lpErrno)
{

    SOCKET ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUCreateSocketHandle,
                       &ReturnValue,
                       LibName,
                       &dwCatalogEntryId,
                       &dwContext,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUCreateSocketHandle(dwCatalogEntryId, dwContext,
                                            lpErrno);
        );

    RECORD_SOCKET_CREATOR (ReturnValue);
    POSTAPINOTIFY((DTCODE_WPUCreateSocketHandle,
                    &ReturnValue,
                    LibName,
                    &dwCatalogEntryId,
                    &dwContext,
                    &lpErrno));

    return(ReturnValue);
}




SOCKET WSPAPI
DTHOOK_WPUModifyIFSHandle(
    DWORD dwCatalogEntryId,
    SOCKET ProposedHandle,
    LPINT lpErrno)
{

    SOCKET ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUModifyIFSHandle,
                       &ReturnValue,
                       LibName,
                       &dwCatalogEntryId,
                       &ProposedHandle,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUModifyIFSHandle(dwCatalogEntryId, ProposedHandle,
                                         lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WPUModifyIFSHandle,
                    &ReturnValue,
                    LibName,
                    &dwCatalogEntryId,
                    &ProposedHandle,
                    &lpErrno));

    return(ReturnValue);
}




int WSPAPI
DTHOOK_WPUQueryBlockingCallback(
    DWORD dwCatalogEntryId,
    LPBLOCKINGCALLBACK FAR * lplpfnCallback,
    PDWORD_PTR lpdwContext,
    LPINT lpErrno)
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUQueryBlockingCallback,
                       &ReturnValue,
                       LibName,
                       &dwCatalogEntryId,
                       &lplpfnCallback,
                       &lpdwContext,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUQueryBlockingCallback(dwCatalogEntryId, lplpfnCallback,
                                               lpdwContext, lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WPUQueryBlockingCallback,
                    &ReturnValue,
                    LibName,
                    &dwCatalogEntryId,
                    &lplpfnCallback,
                    &lpdwContext,
                    &lpErrno));

    return(ReturnValue);
}




int WSPAPI
DTHOOK_WPUQuerySocketHandleContext(
    SOCKET s,
    PDWORD_PTR lpContext,
    LPINT lpErrno )
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUQuerySocketHandleContext,
                       &ReturnValue,
                       LibName,
                       &s,
                       &lpContext,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUQuerySocketHandleContext(s, lpContext, lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WPUQuerySocketHandleContext,
                    &ReturnValue,
                    LibName,
                    &s,
                    &lpContext,
                    &lpErrno));

    return(ReturnValue);
}




int WSPAPI
DTHOOK_WPUQueueApc(
    LPWSATHREADID lpThreadId,
    LPWSAUSERAPC lpfnUserApc,
    DWORD_PTR dwContext,
    LPINT lpErrno)
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUQueueApc,
                       &ReturnValue,
                       LibName,
                       &lpThreadId,
                       &lpfnUserApc,
                       &dwContext,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUQueueApc(lpThreadId, lpfnUserApc, dwContext, lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WPUQueueApc,
                    &ReturnValue,
                    LibName,
                    &lpThreadId,
                    &lpfnUserApc,
                    &dwContext,
                    &lpErrno));

    return(ReturnValue);
}




BOOL WSPAPI
DTHOOK_WPUResetEvent(
    WSAEVENT hEvent,
    LPINT lpErrno)
{

    BOOL ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUResetEvent,
                       &ReturnValue,
                       LibName,
                       &hEvent,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUResetEvent(hEvent, lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WPUResetEvent,
                    &ReturnValue,
                    LibName,
                    &hEvent,
                    &lpErrno));

    return(ReturnValue);
}




BOOL WSPAPI
DTHOOK_WPUSetEvent(
    WSAEVENT hEvent,
    LPINT lpErrno)
{

    BOOL ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUSetEvent,
                       &ReturnValue,
                       LibName,
                       &hEvent,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUSetEvent(hEvent, lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WPUSetEvent,
                    &ReturnValue,
                    LibName,
                    &hEvent,
                    &lpErrno));

    return(ReturnValue);
}




int WSPAPI
DTHOOK_WSCDeinstallProvider(
    LPGUID lpProviderId,
    LPINT lpErrno)
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSCDeinstallProvider,
                       &ReturnValue,
                       LibName,
                       &lpProviderId,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSCDeinstallProvider(lpProviderId, lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WSCDeinstallProvider,
                    &ReturnValue,
                    LibName,
                    &lpProviderId,
                    &lpErrno));

    return(ReturnValue);
}




int WSPAPI
DTHOOK_WSCInstallProvider(
    LPGUID lpProviderId,
    const WCHAR FAR * lpszProviderDllPath,
    const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
    DWORD dwNumberOfEntries,
    LPINT lpErrno)
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSCInstallProvider,
                       &ReturnValue,
                       LibName,
                       &lpProviderId,
                       &lpszProviderDllPath,
                       &lpProtocolInfoList,
                       &dwNumberOfEntries,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSCInstallProvider(lpProviderId, lpszProviderDllPath,
                                         lpProtocolInfoList, dwNumberOfEntries,
                                         lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WSCInstallProvider,
                    &ReturnValue,
                    LibName,
                    &lpProviderId,
                    &lpszProviderDllPath,
                    &lpProtocolInfoList,
                    &dwNumberOfEntries,
                    &lpErrno));

    return(ReturnValue);
}




int WSPAPI
DTHOOK_WPUGetProviderPath(
    IN     LPGUID     lpProviderId,
    OUT    WCHAR FAR * lpszProviderDllPath,
    IN OUT LPINT      lpProviderDllPathLen,
    OUT    LPINT      lpErrno
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUGetProviderPath,
                       &ReturnValue,
                       LibName,
                       &lpProviderId,
                       &lpszProviderDllPath,
                       &lpProviderDllPathLen,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUGetProviderPath(lpProviderId, lpszProviderDllPath,
                                         lpProviderDllPathLen, lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WPUGetProviderPath,
                       &ReturnValue,
                       LibName,
                       &lpProviderId,
                       &lpszProviderDllPath,
                       &lpProviderDllPathLen,
                       &lpErrno));

    return(ReturnValue);
} // DTHOOK_WPUGetProviderPath




BOOL WSPAPI
DTHOOK_WPUPostMessage(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    BOOL ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUPostMessage,
                       &ReturnValue,
                       LibName,
                       &hWnd,
                       &Msg,
                       &wParam,
                       &lParam
                       ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUPostMessage(hWnd, Msg, wParam, lParam);
        );

    POSTAPINOTIFY((DTCODE_WPUPostMessage,
                       &ReturnValue,
                       LibName,
                       &hWnd,
                       &Msg,
                       &wParam,
                       &lParam
                       ));

    return(ReturnValue);
} // DTHOOK_WPUPostMessage




int WSPAPI
DTHOOK_WPUFDIsSet(
    SOCKET       s,
    fd_set FAR * set
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WPUFDIsSet,
                       &ReturnValue,
                       LibName,
                       &s,
                       &set
                       ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WPUFDIsSet(s, set);
        );

    POSTAPINOTIFY((DTCODE_WPUFDIsSet,
                       &ReturnValue,
                       LibName,
                       &s,
                       &set
                       ));

    return(ReturnValue);
} // DTHOOK_WPUFDIsSet




int WSPAPI
DTHOOK___WSAFDIsSet(
    SOCKET       s,
    fd_set FAR * set
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE___WSAFDIsSet,
                       &ReturnValue,
                       LibName,
                       &s,
                       &set
                       ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = __WSAFDIsSet(s, set);
        );

    POSTAPINOTIFY((DTCODE___WSAFDIsSet,
                       &ReturnValue,
                       LibName,
                       &s,
                       &set
                       ));

    return(ReturnValue);
} // DTHOOK___WSAFDIsSet

INT
WSPAPI
DTHOOK_WSAAddressToStringA(
    IN     LPSOCKADDR          lpsaAddress,
    IN     DWORD               dwAddressLength,
    IN     LPWSAPROTOCOL_INFOA lpProtocolInfo,
    IN OUT LPSTR               lpszAddressString,
    IN OUT LPDWORD             lpdwAddressStringLength
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAAddressToStringA,
                      &ReturnValue,
                      LibName,
                      &lpsaAddress,
                      &dwAddressLength,
                      &lpProtocolInfo,
                      &lpszAddressString,
                      &lpdwAddressStringLength))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAAddressToStringA(lpsaAddress, dwAddressLength,
                                          lpProtocolInfo, lpszAddressString,
                                          lpdwAddressStringLength);
        );

    POSTAPINOTIFY( (DTCODE_WSAAddressToStringA,
                   &ReturnValue,
                   LibName,
                   &lpsaAddress,
                   &dwAddressLength,
                   &lpProtocolInfo,
                   &lpszAddressString,
                   &lpdwAddressStringLength
                   ) );

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSAAddressToStringW(
    IN     LPSOCKADDR          lpsaAddress,
    IN     DWORD               dwAddressLength,
    IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN OUT LPWSTR              lpszAddressString,
    IN OUT LPDWORD             lpdwAddressStringLength
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAAddressToStringW,
                      &ReturnValue,
                      LibName,
                      &lpsaAddress,
                      &dwAddressLength,
                      &lpProtocolInfo,
                      &lpszAddressString,
                      &lpdwAddressStringLength))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAAddressToStringW(lpsaAddress, dwAddressLength,
                                          lpProtocolInfo, lpszAddressString,
                                          lpdwAddressStringLength);
        );

    POSTAPINOTIFY( (DTCODE_WSAAddressToStringW,
                   &ReturnValue,
                   LibName,
                   &lpsaAddress,
                   &dwAddressLength,
                   &lpProtocolInfo,
                   &lpszAddressString,
                   &lpdwAddressStringLength
                   ) );

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSAStringToAddressA(
    IN     LPSTR               AddressString,
    IN     INT                 AddressFamily,
    IN     LPWSAPROTOCOL_INFOA  lpProtocolInfo,
    IN OUT LPSOCKADDR          lpAddress,
    IN OUT LPINT               lpAddressLength
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAStringToAddressA,
                      &ReturnValue,
                      LibName,
                      &AddressString,
                      &AddressFamily,
                      &lpProtocolInfo,
                      &lpAddress,
                      &lpAddressLength
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAStringToAddressA(AddressString, AddressFamily,
                                          lpProtocolInfo, lpAddress,
                                          lpAddressLength);
        );

    POSTAPINOTIFY((DTCODE_WSAStringToAddressA,
                   &ReturnValue,
                   LibName,
                   &AddressString,
                   &AddressFamily,
                   &lpProtocolInfo,
                   &lpAddress,
                   &lpAddressLength));
    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSAStringToAddressW(
    IN     LPWSTR              AddressString,
    IN     INT                 AddressFamily,
    IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN OUT LPSOCKADDR          lpAddress,
    IN OUT LPINT               lpAddressLength
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAStringToAddressW,
                      &ReturnValue,
                      LibName,
                      &AddressString,
                      &AddressFamily,
                      &lpProtocolInfo,
                      &lpAddress,
                      &lpAddressLength
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAStringToAddressW(AddressString, AddressFamily,
                                          lpProtocolInfo, lpAddress,
                                          lpAddressLength);
        );

    POSTAPINOTIFY((DTCODE_WSAStringToAddressW,
                   &ReturnValue,
                   LibName,
                   &AddressString,
                   &AddressFamily,
                   &lpProtocolInfo,
                   &lpAddress,
                   &lpAddressLength));
    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSALookupServiceBeginA(
    IN  LPWSAQUERYSETA lpqsRestrictions,
    IN  DWORD          dwControlFlags,
    OUT LPHANDLE       lphLookup
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSALookupServiceBeginA,
                      &ReturnValue,
                      LibName,
                      &lpqsRestrictions,
                      &dwControlFlags,
                      &lphLookup
                      ))) {
        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSALookupServiceBeginA(lpqsRestrictions, dwControlFlags,
                                             lphLookup);
        );

    POSTAPINOTIFY((DTCODE_WSALookupServiceBeginA,
                   &ReturnValue,
                   LibName,
                   &lpqsRestrictions,
                   &dwControlFlags,
                   &lphLookup));

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSALookupServiceBeginW(
    IN  LPWSAQUERYSETW lpqsRestrictions,
    IN  DWORD          dwControlFlags,
    OUT LPHANDLE       lphLookup
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSALookupServiceBeginW,
                      &ReturnValue,
                      LibName,
                      &lpqsRestrictions,
                      &dwControlFlags,
                      &lphLookup
                      ))) {
        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSALookupServiceBeginW(lpqsRestrictions, dwControlFlags,
                                             lphLookup);
        );

    POSTAPINOTIFY((DTCODE_WSALookupServiceBeginW,
                   &ReturnValue,
                   LibName,
                   &lpqsRestrictions,
                   &dwControlFlags,
                   &lphLookup));

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSALookupServiceNextA(
    IN     HANDLE           hLookup,
    IN     DWORD            dwControlFlags,
    IN OUT LPDWORD          lpdwBufferLength,
    OUT    LPWSAQUERYSETA   lpqsResults
    )
{
       int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSALookupServiceNextA,
                      &ReturnValue,
                      LibName,
                      &hLookup,
                      &dwControlFlags,
                      &lpdwBufferLength,
                      &lpqsResults
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSALookupServiceNextA(hLookup, dwControlFlags,
                                            lpdwBufferLength, lpqsResults);
        );

    POSTAPINOTIFY((DTCODE_WSALookupServiceNextA,
                   &ReturnValue,
                   LibName,
                   &hLookup,
                   &dwControlFlags,
                   &lpdwBufferLength,
                   &lpqsResults
                   ));

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSALookupServiceNextW(
    IN     HANDLE           hLookup,
    IN     DWORD            dwControlFlags,
    IN OUT LPDWORD          lpdwBufferLength,
    OUT    LPWSAQUERYSETW   lpqsResults
    )
{
       int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSALookupServiceNextW,
                      &ReturnValue,
                      LibName,
                      &hLookup,
                      &dwControlFlags,
                      &lpdwBufferLength,
                      &lpqsResults
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSALookupServiceNextW(hLookup, dwControlFlags,
                                            lpdwBufferLength, lpqsResults);
        );

    POSTAPINOTIFY((DTCODE_WSALookupServiceNextW,
                   &ReturnValue,
                   LibName,
                   &hLookup,
                   &dwControlFlags,
                   &lpdwBufferLength,
                   &lpqsResults
                   ));

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSALookupServiceEnd(
    IN HANDLE  hLookup
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSALookupServiceEnd,
                      &ReturnValue,
                      LibName,
                      &hLookup
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSALookupServiceEnd(hLookup);
        );

    POSTAPINOTIFY((DTCODE_WSALookupServiceEnd,
                   &ReturnValue,
                   LibName,
                   &hLookup
                   ));

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSAInstallServiceClassA(
    IN  LPWSASERVICECLASSINFOA   lpServiceClassInfo
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAInstallServiceClassA,
                      &ReturnValue,
                      LibName,
                      &lpServiceClassInfo
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAInstallServiceClassA(lpServiceClassInfo);
        );

    POSTAPINOTIFY((DTCODE_WSAInstallServiceClassA,
                   &ReturnValue,
                   LibName,
                   &lpServiceClassInfo));

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSAInstallServiceClassW(
    IN  LPWSASERVICECLASSINFOW   lpServiceClassInfo
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAInstallServiceClassW,
                      &ReturnValue,
                      LibName,
                      &lpServiceClassInfo
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAInstallServiceClassW(lpServiceClassInfo);
        );

    POSTAPINOTIFY((DTCODE_WSAInstallServiceClassW,
                   &ReturnValue,
                   LibName,
                   &lpServiceClassInfo));

    return(ReturnValue);
}



INT WSPAPI
DTHOOK_WSASetServiceA(
    IN  LPWSAQUERYSETA    lpqsRegInfo,
    IN  WSAESETSERVICEOP  essOperation,
    IN  DWORD             dwControlFlags
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSASetServiceA,
                      &ReturnValue,
                      LibName,
                      &lpqsRegInfo,
                      &essOperation,
                      &dwControlFlags
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSASetServiceA(lpqsRegInfo, essOperation,
                                     dwControlFlags);
        );

    POSTAPINOTIFY((DTCODE_WSASetServiceA,
                   &ReturnValue,
                   LibName,
                   &lpqsRegInfo,
                   &essOperation,
                   &dwControlFlags));

    return(ReturnValue);
}


INT WSPAPI
DTHOOK_WSASetServiceW(
    IN  LPWSAQUERYSETW    lpqsRegInfo,
    IN  WSAESETSERVICEOP  essOperation,
    IN  DWORD             dwControlFlags
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSASetServiceW,
                      &ReturnValue,
                      LibName,
                      &lpqsRegInfo,
                      &essOperation,
                      &dwControlFlags
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSASetServiceW(lpqsRegInfo, essOperation,
                                     dwControlFlags);
        );

    POSTAPINOTIFY((DTCODE_WSASetServiceW,
                   &ReturnValue,
                   LibName,
                   &lpqsRegInfo,
                   &essOperation,
                   &dwControlFlags));

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSARemoveServiceClass(
    IN  LPGUID  lpServiceClassId
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSARemoveServiceClass,
                      &ReturnValue,
                      LibName,
                      &lpServiceClassId))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSARemoveServiceClass(lpServiceClassId);
        );

    POSTAPINOTIFY((DTCODE_WSARemoveServiceClass,
                   &ReturnValue,
                   LibName,
                   &lpServiceClassId));

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSAGetServiceClassInfoA(
    IN     LPGUID                  lpProviderId,
    IN     LPGUID                  lpServiceClassId,
    IN OUT LPDWORD                 lpdwBufSize,
    OUT    LPWSASERVICECLASSINFOA   lpServiceClassInfo
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAGetServiceClassInfoA,
                      &ReturnValue,
                      LibName,
                      &lpProviderId,
                      &lpServiceClassId,
                      &lpdwBufSize,
                      &lpServiceClassInfo
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAGetServiceClassInfoA(lpProviderId, lpServiceClassId,
                                              lpdwBufSize, lpServiceClassInfo);
        );

    POSTAPINOTIFY((DTCODE_WSAGetServiceClassInfoA,
                   &ReturnValue,
                   LibName,
                   &lpProviderId,
                   &lpServiceClassId,
                   &lpdwBufSize,
                   &lpServiceClassInfo));

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSAGetServiceClassInfoW(
    IN     LPGUID                  lpProviderId,
    IN     LPGUID                  lpServiceClassId,
    IN OUT LPDWORD                 lpdwBufSize,
    OUT    LPWSASERVICECLASSINFOW   lpServiceClassInfo
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAGetServiceClassInfoW,
                      &ReturnValue,
                      LibName,
                      &lpProviderId,
                      &lpServiceClassId,
                      &lpdwBufSize,
                      &lpServiceClassInfo
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAGetServiceClassInfoW(lpProviderId, lpServiceClassId,
                                              lpdwBufSize, lpServiceClassInfo);
        );

    POSTAPINOTIFY((DTCODE_WSAGetServiceClassInfoW,
                   &ReturnValue,
                   LibName,
                   &lpProviderId,
                   &lpServiceClassId,
                   &lpdwBufSize,
                   &lpServiceClassInfo));

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSAEnumNameSpaceProvidersA(
    IN OUT LPDWORD              lpdwBufferLength,
    IN     LPWSANAMESPACE_INFOA  Lpnspbuffer
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAEnumNameSpaceProvidersA,
                      &ReturnValue,
                      LibName,
                      &lpdwBufferLength,
                      &Lpnspbuffer))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAEnumNameSpaceProvidersA(lpdwBufferLength, Lpnspbuffer);
        );

    POSTAPINOTIFY((DTCODE_WSAEnumNameSpaceProvidersA,
                   &ReturnValue,
                   LibName,
                   &lpdwBufferLength,
                   &Lpnspbuffer));

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSAEnumNameSpaceProvidersW(
    IN OUT LPDWORD              lpdwBufferLength,
    IN     LPWSANAMESPACE_INFOW  Lpnspbuffer
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAEnumNameSpaceProvidersW,
                      &ReturnValue,
                      LibName,
                      &lpdwBufferLength,
                      &Lpnspbuffer))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAEnumNameSpaceProvidersW(lpdwBufferLength, Lpnspbuffer);
        );

    POSTAPINOTIFY((DTCODE_WSAEnumNameSpaceProvidersW,
                   &ReturnValue,
                   LibName,
                   &lpdwBufferLength,
                   &Lpnspbuffer));

    return(ReturnValue);
}


INT
WSPAPI
DTHOOK_WSAGetServiceClassNameByClassIdA(
    IN      LPGUID  lpServiceClassId,
    OUT     LPSTR   lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAGetServiceClassNameByClassIdA,
                      &ReturnValue,
                      LibName,
                      &lpServiceClassId,
                      &lpszServiceClassName,
                      &lpdwBufferLength
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAGetServiceClassNameByClassIdA(lpServiceClassId,
                                                       lpszServiceClassName,
                                                       lpdwBufferLength);
        );

    POSTAPINOTIFY((DTCODE_WSAGetServiceClassNameByClassIdA,
                   &ReturnValue,
                   LibName,
                   &lpServiceClassId,
                   &lpszServiceClassName,
                   &lpdwBufferLength ));

    return(ReturnValue);
}

INT
WSPAPI
DTHOOK_WSAGetServiceClassNameByClassIdW(
    IN      LPGUID  lpServiceClassId,
    OUT     LPWSTR   lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength
    )
{
    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSAGetServiceClassNameByClassIdW,
                      &ReturnValue,
                      LibName,
                      &lpServiceClassId,
                      &lpszServiceClassName,
                      &lpdwBufferLength
                      ))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAGetServiceClassNameByClassIdW(lpServiceClassId,
                                                       lpszServiceClassName,
                                                       lpdwBufferLength);
        );

    POSTAPINOTIFY((DTCODE_WSAGetServiceClassNameByClassIdW,
                   &ReturnValue,
                   LibName,
                   &lpServiceClassId,
                   &lpszServiceClassName,
                   &lpdwBufferLength ));

    return(ReturnValue);
}

INT
WSAAPI
DTHOOK_WSACancelBlockingCall(
    VOID
    )
{
    INT ReturnValue;

    if( PREAPINOTIFY((
            DTCODE_WSACancelBlockingCall,
            &ReturnValue,
            LibName
            )) ) {
        return ReturnValue;
    }

    INVOKE_ROUTINE(
        ReturnValue = WSACancelBlockingCall();
        );

    POSTAPINOTIFY((
        DTCODE_WSACancelBlockingCall,
        &ReturnValue,
        LibName
        ));

    return ReturnValue;
}

FARPROC
WSAAPI
DTHOOK_WSASetBlockingHook(
    FARPROC lpBlockFunc
    )
{
    FARPROC ReturnValue;

    if( PREAPINOTIFY((
            DTCODE_WSASetBlockingHook,
            &ReturnValue,
            LibName,
            &lpBlockFunc
            )) ) {
        return ReturnValue;
    }

    INVOKE_ROUTINE(
        ReturnValue = WSASetBlockingHook( lpBlockFunc );
        );

    POSTAPINOTIFY((
        DTCODE_WSASetBlockingHook,
        &ReturnValue,
        LibName,
        &lpBlockFunc
        ));

    return ReturnValue;
}

INT
WSAAPI
DTHOOK_WSAUnhookBlockingHook(
    VOID
    )
{
    INT ReturnValue;

    if( PREAPINOTIFY((
            DTCODE_WSAUnhookBlockingHook,
            &ReturnValue,
            LibName
            )) ) {
        return ReturnValue;
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAUnhookBlockingHook();
        );

    POSTAPINOTIFY((
        DTCODE_WSAUnhookBlockingHook,
        &ReturnValue,
        LibName
        ));

    return ReturnValue;
}

BOOL
WSAAPI
DTHOOK_WSAIsBlocking(
    VOID
    )
{
    BOOL ReturnValue;

    if( PREAPINOTIFY((
            DTCODE_WSAIsBlocking,
            &ReturnValue,
            LibName
            )) ) {
        return ReturnValue;
    }

    INVOKE_ROUTINE(
        ReturnValue = WSAIsBlocking();
        );

    POSTAPINOTIFY((
        DTCODE_WSAIsBlocking,
        &ReturnValue,
        LibName
        ));

    return ReturnValue;
}


int WSPAPI
DTHOOK_WSCGetProviderPath(
    LPGUID lpProviderId,
    WCHAR FAR * lpszProviderDllPath,
    LPINT lpProviderDllPathLen,
    LPINT lpErrno)
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSCGetProviderPath,
                       &ReturnValue,
                       LibName,
                       &lpProviderId,
                       &lpszProviderDllPath,
                       &lpProviderDllPathLen,
                       &lpErrno))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSCGetProviderPath(lpProviderId, lpszProviderDllPath,
                                         lpProviderDllPathLen, lpErrno);
        );

    POSTAPINOTIFY((DTCODE_WSCGetProviderPath,
                    &ReturnValue,
                    LibName,
                    &lpProviderId,
                    &lpszProviderDllPath,
                    &lpProviderDllPathLen,
                    &lpErrno));

    return(ReturnValue);
}


int WSPAPI
DTHOOK_WSCInstallNameSpace(
    LPWSTR lpszIdentifier,
    LPWSTR lpszPathName,
    DWORD dwNameSpace,
    DWORD dwVersion,
    LPGUID lpProviderId)
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSCInstallNameSpace,
                       &ReturnValue,
                       LibName,
                       &lpszIdentifier,
                       &lpszPathName,
                       &dwNameSpace,
                       &dwVersion,
                       &lpProviderId))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSCInstallNameSpace(lpszIdentifier, lpszPathName,
                                          dwNameSpace, dwVersion, lpProviderId);
        );

    POSTAPINOTIFY((DTCODE_WSCInstallNameSpace,
                    &ReturnValue,
                    LibName,
                    &lpszIdentifier,
                    &lpszPathName,
                    &dwNameSpace,
                    &dwVersion,
                    &lpProviderId));

    return(ReturnValue);
}


int WSPAPI
DTHOOK_WSCUnInstallNameSpace(
    LPGUID lpProviderId
    )
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSCUnInstallNameSpace,
                       &ReturnValue,
                       LibName,
                       &lpProviderId))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSCUnInstallNameSpace(lpProviderId);
        );

    POSTAPINOTIFY((DTCODE_WSCUnInstallNameSpace,
                    &ReturnValue,
                    LibName,
                    &lpProviderId));

    return(ReturnValue);
}


int WSPAPI
DTHOOK_WSCEnableNSProvider(
    LPGUID lpProviderId,
    BOOL fEnable
    )
{

    int ReturnValue;

    if (PREAPINOTIFY((DTCODE_WSCEnableNSProvider,
                       &ReturnValue,
                       LibName,
                       &lpProviderId,
                       &fEnable))) {

        return(ReturnValue);
    }

    INVOKE_ROUTINE(
        ReturnValue = WSCEnableNSProvider(lpProviderId, fEnable);
        );

    POSTAPINOTIFY((DTCODE_WSCEnableNSProvider,
                    &ReturnValue,
                    LibName,
                    &lpProviderId,
                    &fEnable));

    return(ReturnValue);
}


LONG
DtExceptionFilter(
    LPEXCEPTION_POINTERS ExceptionPointers,
    LPSTR                Routine
    )
{

    //
    // Protect ourselves in case the process is totally screwed.
    //

    __try {

        //
        // Whine about the exception.
        //

        PrintDebugString(
            "Exception: %08lx @ %08lx, caught in %s\n",
            ExceptionPointers->ExceptionRecord->ExceptionCode,
            ExceptionPointers->ExceptionRecord->ExceptionAddress,
            Routine
            );

        //
        // Call the debug/trace exception routine if installed.
        //

        if( ExceptionNotifyFP != NULL ) {

            (ExceptionNotifyFP)( ExceptionPointers );

        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // Not much we can do here...
        //

        NOTHING;

    }

    //
    // We don't actually want to suppress exceptions, just whine about them.
    // So, we return EXCEPTION_CONTINUE_SEARCH here so that the exception will
    // be seen by the app/debugger/whatever.
    //

    return EXCEPTION_CONTINUE_SEARCH;

}   // DtExceptionFilter

} // extern "C"

#endif  // DEBUG_TRACING

