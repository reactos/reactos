/*++
  
  Copyright (c) 1995 Intel Corp
  
  Module Name:
  
    handlers.cpp
  
  Abstract:
  
    Contains handler functions for each possible API or SPI hook.
  
  Author:
    
    Michael A. Grafton 
  
--*/

#include "nowarn.h"  /* turn off benign warnings */
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif
#include <windows.h>
#include "nowarn.h"  /* some warnings may have been turned back on */
#include <winsock2.h>
#include <ws2spi.h>

#include "handlers.h"
#include "dt_dll.h"
#include "dt.h"

// turn off unreferenced local variable warning
#pragma warning(disable: 4100)



//
// Function Definitions
//


BOOL
DTHandlerInit(
    OUT LPLPFNDTHANDLER HandlerFuncTable,
    int                    NumEntries)
/*++
  
  Function Description:
  
      Fills in the given HandlerFuncTable with pointers to all the
      appropriate handler functions, based on notification code.
  
  Arguments:
  
      HandlerFuncTable -- an uninitialized array of pointers of type
      LPFNDTHANDLER, which point to handler functions (see handlers.h)

      NumEntries -- the number of entries in the array.  Currently
      only used to zero out the array.
        
  Return Value:
  
      Always returns TRUE.
  
--*/
{
    memset((char *)HandlerFuncTable, 0, NumEntries * sizeof(LPFNDTHANDLER));

    HandlerFuncTable[DTCODE_accept] = DTHandler_accept;
    HandlerFuncTable[DTCODE_bind] = DTHandler_bind;
    HandlerFuncTable[DTCODE_closesocket] = DTHandler_closesocket;
    HandlerFuncTable[DTCODE_connect] = DTHandler_connect;
    HandlerFuncTable[DTCODE_getpeername] = DTHandler_getpeername;
    HandlerFuncTable[DTCODE_getsockname] = DTHandler_getsockname;
    HandlerFuncTable[DTCODE_getsockopt] = DTHandler_getsockopt;
    HandlerFuncTable[DTCODE_htonl] = DTHandler_htonl;
    HandlerFuncTable[DTCODE_htons] = DTHandler_htons;
    HandlerFuncTable[DTCODE_ioctlsocket] = DTHandler_ioctlsocket;
    HandlerFuncTable[DTCODE_listen] = DTHandler_listen;
    HandlerFuncTable[DTCODE_ntohl] = DTHandler_ntohl;
    HandlerFuncTable[DTCODE_ntohs] = DTHandler_ntohs;
    HandlerFuncTable[DTCODE_recv] = DTHandler_recv;
    HandlerFuncTable[DTCODE_recvfrom] = DTHandler_recvfrom;
    HandlerFuncTable[DTCODE_select] = DTHandler_select;
    HandlerFuncTable[DTCODE_send] = DTHandler_send;
    HandlerFuncTable[DTCODE_sendto] = DTHandler_sendto;
    HandlerFuncTable[DTCODE_setsockopt] = DTHandler_setsockopt;
    HandlerFuncTable[DTCODE_shutdown] = DTHandler_shutdown;
    HandlerFuncTable[DTCODE_socket] = DTHandler_socket;
    HandlerFuncTable[DTCODE_WSAAccept] = DTHandler_WSAAccept;
    HandlerFuncTable[DTCODE_WSAAsyncSelect] = DTHandler_WSAAsyncSelect;
    HandlerFuncTable[DTCODE_WSACancelBlockingCall] = 
      DTHandler_WSACancelBlockingCall;
    HandlerFuncTable[DTCODE_WSACleanup] = DTHandler_WSACleanup;
    HandlerFuncTable[DTCODE_WSACloseEvent] = DTHandler_WSACloseEvent;
    HandlerFuncTable[DTCODE_WSAConnect] = DTHandler_WSAConnect;
    HandlerFuncTable[DTCODE_WSACreateEvent] = DTHandler_WSACreateEvent;
    HandlerFuncTable[DTCODE_WSADuplicateSocketA] =
      DTHandler_WSADuplicateSocketA;
    HandlerFuncTable[DTCODE_WSADuplicateSocketW] =
      DTHandler_WSADuplicateSocketW;
    HandlerFuncTable[DTCODE_WSAEnumNetworkEvents] = 
      DTHandler_WSAEnumNetworkEvents;
    HandlerFuncTable[DTCODE_WSAEnumProtocolsA] = DTHandler_WSAEnumProtocolsA;
    HandlerFuncTable[DTCODE_WSAEnumProtocolsW] = DTHandler_WSAEnumProtocolsW;
    HandlerFuncTable[DTCODE_WSAEventSelect] = DTHandler_WSAEventSelect;
    HandlerFuncTable[DTCODE_WSAGetLastError] = DTHandler_WSAGetLastError;
    HandlerFuncTable[DTCODE_WSAGetOverlappedResult] = 
      DTHandler_WSAGetOverlappedResult;
    HandlerFuncTable[DTCODE_WSAGetQOSByName] = DTHandler_WSAGetQOSByName;
    HandlerFuncTable[DTCODE_WSAHtonl] = DTHandler_WSAHtonl;
    HandlerFuncTable[DTCODE_WSAHtons] = DTHandler_WSAHtons;
    HandlerFuncTable[DTCODE_WSAIoctl] = DTHandler_WSAIoctl;
    HandlerFuncTable[DTCODE_WSAIsBlocking] = DTHandler_WSAIsBlocking;
    HandlerFuncTable[DTCODE_WSAJoinLeaf] = DTHandler_WSAJoinLeaf;
    HandlerFuncTable[DTCODE_WSANtohl] = DTHandler_WSANtohl;
    HandlerFuncTable[DTCODE_WSANtohs] = DTHandler_WSANtohs;
    HandlerFuncTable[DTCODE_WSARecv] = DTHandler_WSARecv;
    HandlerFuncTable[DTCODE_WSARecvDisconnect] = 
      DTHandler_WSARecvDisconnect;
    HandlerFuncTable[DTCODE_WSARecvFrom] = DTHandler_WSARecvFrom;
    HandlerFuncTable[DTCODE_WSAResetEvent] = DTHandler_WSAResetEvent;
    HandlerFuncTable[DTCODE_WSASend] = DTHandler_WSASend;
    HandlerFuncTable[DTCODE_WSASendDisconnect] = 
      DTHandler_WSASendDisconnect;
    HandlerFuncTable[DTCODE_WSASendTo] = DTHandler_WSASendTo;
    HandlerFuncTable[DTCODE_WSASetBlockingHook] = 
      DTHandler_WSASetBlockingHook;
    HandlerFuncTable[DTCODE_WSASetEvent] = DTHandler_WSASetEvent;
    HandlerFuncTable[DTCODE_WSASetLastError] = DTHandler_WSASetLastError;
    HandlerFuncTable[DTCODE_WSASocketA] = DTHandler_WSASocketA;
    HandlerFuncTable[DTCODE_WSASocketW] = DTHandler_WSASocketW;
    HandlerFuncTable[DTCODE_WSAStartup] = DTHandler_WSAStartup;
    HandlerFuncTable[DTCODE_WSAUnhookBlockingHook] = 
      DTHandler_WSAUnhookBlockingHook;
    HandlerFuncTable[DTCODE_WSAWaitForMultipleEvents] = 
      DTHandler_WSAWaitForMultipleEvents;
    HandlerFuncTable[DTCODE_gethostbyaddr] = DTHandler_gethostbyaddr;
    HandlerFuncTable[DTCODE_gethostbyname] = DTHandler_gethostbyname;
    HandlerFuncTable[DTCODE_gethostname] = DTHandler_gethostname;
    HandlerFuncTable[DTCODE_getprotobyname] = DTHandler_getprotobyname;
    HandlerFuncTable[DTCODE_getprotobynumber] = DTHandler_getprotobynumber;
    HandlerFuncTable[DTCODE_getservbyname] = DTHandler_getservbyname;
    HandlerFuncTable[DTCODE_getservbyport] = DTHandler_getservbyport;
    HandlerFuncTable[DTCODE_inet_addr] = DTHandler_inet_addr;
    HandlerFuncTable[DTCODE_inet_ntoa] = DTHandler_inet_ntoa;
    HandlerFuncTable[DTCODE_WSAAsyncGetHostByAddr] = 
      DTHandler_WSAAsyncGetHostByAddr;
    HandlerFuncTable[DTCODE_WSAAsyncGetHostByName] = 
      DTHandler_WSAAsyncGetHostByName;
    HandlerFuncTable[DTCODE_WSAAsyncGetProtoByName] = 
      DTHandler_WSAAsyncGetProtoByName;
    HandlerFuncTable[DTCODE_WSAAsyncGetProtoByNumber] = 
      DTHandler_WSAAsyncGetProtoByNumber;
    HandlerFuncTable[DTCODE_WSAAsyncGetServByName] = 
      DTHandler_WSAAsyncGetServByName;
    HandlerFuncTable[DTCODE_WSAAsyncGetServByPort] = 
      DTHandler_WSAAsyncGetServByPort;
    HandlerFuncTable[DTCODE_WSACancelAsyncRequest] = 
      DTHandler_WSACancelAsyncRequest;
    HandlerFuncTable[DTCODE_WSPAccept] = DTHandler_WSPAccept;
    HandlerFuncTable[DTCODE_WSPAsyncSelect] = DTHandler_WSPAsyncSelect;
    HandlerFuncTable[DTCODE_WSPBind] = DTHandler_WSPBind;
    HandlerFuncTable[DTCODE_WSPCancelBlockingCall] = 
      DTHandler_WSPCancelBlockingCall;
    HandlerFuncTable[DTCODE_WSPCleanup] = DTHandler_WSPCleanup;
    HandlerFuncTable[DTCODE_WSPCloseSocket] = DTHandler_WSPCloseSocket;
    HandlerFuncTable[DTCODE_WSPConnect] = DTHandler_WSPConnect;
    HandlerFuncTable[DTCODE_WSPDuplicateSocket] = 
      DTHandler_WSPDuplicateSocket;
    HandlerFuncTable[DTCODE_WSPEnumNetworkEvents] = 
      DTHandler_WSPEnumNetworkEvents;
    HandlerFuncTable[DTCODE_WSPEventSelect] = DTHandler_WSPEventSelect;
    HandlerFuncTable[DTCODE_WSPGetOverlappedResult] = 
      DTHandler_WSPGetOverlappedResult;
    HandlerFuncTable[DTCODE_WSPGetPeerName] = DTHandler_WSPGetPeerName;
    HandlerFuncTable[DTCODE_WSPGetSockName] = DTHandler_WSPGetSockName;
    HandlerFuncTable[DTCODE_WSPGetSockOpt] = DTHandler_WSPGetSockOpt;
    HandlerFuncTable[DTCODE_WSPGetQOSByName] = DTHandler_WSPGetQOSByName;
    HandlerFuncTable[DTCODE_WSPIoctl] = DTHandler_WSPIoctl;
    HandlerFuncTable[DTCODE_WSPJoinLeaf] = DTHandler_WSPJoinLeaf;
    HandlerFuncTable[DTCODE_WSPListen] = DTHandler_WSPListen;
    HandlerFuncTable[DTCODE_WSPRecv] = DTHandler_WSPRecv;
    HandlerFuncTable[DTCODE_WSPRecvDisconnect] = 
      DTHandler_WSPRecvDisconnect;
    HandlerFuncTable[DTCODE_WSPRecvFrom] = DTHandler_WSPRecvFrom;
    HandlerFuncTable[DTCODE_WSPSelect] = DTHandler_WSPSelect;
    HandlerFuncTable[DTCODE_WSPSend] = DTHandler_WSPSend;
    HandlerFuncTable[DTCODE_WSPSendDisconnect] = 
      DTHandler_WSPSendDisconnect;
    HandlerFuncTable[DTCODE_WSPSendTo] = DTHandler_WSPSendTo;
    HandlerFuncTable[DTCODE_WSPSetSockOpt] = DTHandler_WSPSetSockOpt;
    HandlerFuncTable[DTCODE_WSPShutdown] = DTHandler_WSPShutdown;
    HandlerFuncTable[DTCODE_WSPSocket] = DTHandler_WSPSocket;
    HandlerFuncTable[DTCODE_WSPStartup] = DTHandler_WSPStartup;
    HandlerFuncTable[DTCODE_WPUCloseEvent] = DTHandler_WPUCloseEvent;
    HandlerFuncTable[DTCODE_WPUCloseSocketHandle] = 
      DTHandler_WPUCloseSocketHandle;
    HandlerFuncTable[DTCODE_WPUCreateEvent] = DTHandler_WPUCreateEvent;
    HandlerFuncTable[DTCODE_WPUCreateSocketHandle] = 
      DTHandler_WPUCreateSocketHandle;
    HandlerFuncTable[DTCODE_WSCDeinstallProvider] = 
      DTHandler_WSCDeinstallProvider;
    HandlerFuncTable[DTCODE_WSCInstallProvider] = 
      DTHandler_WSCInstallProvider;
    HandlerFuncTable[DTCODE_WPUModifyIFSHandle] = 
      DTHandler_WPUModifyIFSHandle;
    HandlerFuncTable[DTCODE_WPUQueryBlockingCallback] = 
      DTHandler_WPUQueryBlockingCallback;
    HandlerFuncTable[DTCODE_WPUQuerySocketHandleContext] = 
      DTHandler_WPUQuerySocketHandleContext;
    HandlerFuncTable[DTCODE_WPUQueueApc] = DTHandler_WPUQueueApc;
    HandlerFuncTable[DTCODE_WPUResetEvent] = DTHandler_WPUResetEvent;
    HandlerFuncTable[DTCODE_WPUSetEvent] = DTHandler_WPUSetEvent;
    HandlerFuncTable[DTCODE_WSCEnumProtocols] = DTHandler_WSCEnumProtocols;
    HandlerFuncTable[DTCODE_WPUGetProviderPath] = DTHandler_WPUGetProviderPath;
    HandlerFuncTable[DTCODE_WPUPostMessage] = DTHandler_WPUPostMessage;
    HandlerFuncTable[DTCODE_WPUFDIsSet] = DTHandler_WPUFDIsSet;
    HandlerFuncTable[DTCODE___WSAFDIsSet] = DTHandler___WSAFDIsSet;
    HandlerFuncTable[DTCODE_WSAAddressToStringA] =
        DTHandler_WSAAddressToStringA;
    HandlerFuncTable[DTCODE_WSAAddressToStringW] =
        DTHandler_WSAAddressToStringW;
    HandlerFuncTable[DTCODE_WSAStringToAddressA] =
        DTHandler_WSAStringToAddressA;
    HandlerFuncTable[DTCODE_WSAStringToAddressW] =
        DTHandler_WSAStringToAddressW;
    HandlerFuncTable[DTCODE_WSALookupServiceBeginA] =
        DTHandler_WSALookupServiceBeginA;
    HandlerFuncTable[DTCODE_WSALookupServiceBeginW] =
        DTHandler_WSALookupServiceBeginW;
    HandlerFuncTable[DTCODE_WSALookupServiceNextA] =
        DTHandler_WSALookupServiceNextA;
    HandlerFuncTable[DTCODE_WSALookupServiceNextW] =
        DTHandler_WSALookupServiceNextW;
    HandlerFuncTable[DTCODE_WSALookupServiceEnd] =
        DTHandler_WSALookupServiceEnd;
//    HandlerFuncTable[DTCODE_WSAGetAddressByNameA] =
//        DTHandler_WSAGetAddressByNameA;
//    HandlerFuncTable[DTCODE_WSAGetAddressByNameW] =
//        DTHandler_WSAGetAddressByNameW;
    HandlerFuncTable[DTCODE_WSAInstallServiceClassA] =
        DTHandler_WSAInstallServiceClassA;
    HandlerFuncTable[DTCODE_WSAInstallServiceClassW] =
        DTHandler_WSAInstallServiceClassW;
    HandlerFuncTable[DTCODE_WSASetServiceA] =
        DTHandler_WSASetServiceA;
    HandlerFuncTable[DTCODE_WSASetServiceW] =
        DTHandler_WSASetServiceW;
    HandlerFuncTable[DTCODE_WSARemoveServiceClass] =
        DTHandler_WSARemoveServiceClass;
    HandlerFuncTable[DTCODE_WSAGetServiceClassInfoA] =
        DTHandler_WSAGetServiceClassInfoA;
    HandlerFuncTable[DTCODE_WSAGetServiceClassInfoW] =
        DTHandler_WSAGetServiceClassInfoW;
    HandlerFuncTable[DTCODE_WSAEnumNameSpaceProvidersA] =
        DTHandler_WSAEnumNameSpaceProvidersA;
    HandlerFuncTable[DTCODE_WSAEnumNameSpaceProvidersW] =
        DTHandler_WSAEnumNameSpaceProvidersW;
    HandlerFuncTable[DTCODE_WSAGetServiceClassNameByClassIdA] =
        DTHandler_WSAGetServiceClassNameByClassIdA;
    HandlerFuncTable[DTCODE_WSAGetServiceClassNameByClassIdW] =
        DTHandler_WSAGetServiceClassNameByClassIdW;
    HandlerFuncTable[DTCODE_NSPLookupServiceBegin] =
        DTHandler_NSPLookupServiceBegin;
    HandlerFuncTable[DTCODE_NSPLookupServiceNext] =
        DTHandler_NSPLookupServiceNext;
    HandlerFuncTable[DTCODE_NSPLookupServiceEnd] =
        DTHandler_NSPLookupServiceEnd;
    HandlerFuncTable[DTCODE_NSPInstallServiceClass] =
        DTHandler_NSPInstallServiceClass;
    HandlerFuncTable[DTCODE_NSPSetService] =
        DTHandler_NSPSetService;
    HandlerFuncTable[DTCODE_NSPRemoveServiceClass] =
        DTHandler_NSPRemoveServiceClass;
    HandlerFuncTable[DTCODE_NSPGetServiceClassInfo] =
        DTHandler_NSPGetServiceClassInfo;
 
    return(TRUE);
}

/*++
  
  Note:

      This comment applies to all DTHandler_xxxx functions.
  
  Function Description:
  
      For the present, each handler function simply adds "xxxx
      called." to the end of the string passed in, and calls
      DTTextOut to output the whole string to the Debug Window.  This
      allows handler functions to be disabled easily, or to output
      more information (like the arguments) via multiple calls to
      DTTextOut, etc.  See the Debug/Trace documentation for more
      information. 
  
  Arguments:
  
      vl -- Variable used to strip arguments off the stack of 
      WSA[Pre|Post]ApiNotify, the calling function.

      ReturnValue -- A void pointer to the return value for the
      original API/SPI function.  Use a local variable to cast it to
      the appropriate type.

      LibraryName -- For API functions, this should just be
      "WinSock2", but for SPI functions it is the name of the service
      provider.  This is to distinguish among the multiple service
      providers that WinSock2 may be calling.

      Buffer -- A string containg some info about the thread and
      function call # of this call.  

      Index -- How many characters are in the above.
      
      BufLen -- How big is the buffer.

      PreOrPost -- TRUE if called by a WSAPreApiNotify, FALSE if
      called by WSAPostApiNotify.  
        
  Return Value:
  
      If the PreOrPost argument is FALSE, then the return value is
      ignored.  If it's TRUE, then the return value indicates to
      WSAPreApiNotify whether or not we want to short-circuit the API
      or SPI function.  TRUE means short-circuit it, FALSE mean don't.
  
--*/


BOOL CALLBACK
DTHandler_accept(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    SOCKET *RetVal = (SOCKET *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **addr = va_arg(vl, struct sockaddr FAR **);
    int FAR **addrlen = va_arg(vl, int FAR **);

    wsprintf(Buffer + Index, "accept() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_bind(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **name = va_arg(vl, struct sockaddr FAR **);
    int *namelen = va_arg(vl, int *);

    wsprintf(Buffer + Index, "bind() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_closesocket(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);

    wsprintf(Buffer + Index, "closesocket() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_connect(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **name = va_arg(vl, struct sockaddr FAR **);
    int *namelen = va_arg(vl, int *);

    wsprintf(Buffer + Index, "connect() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_getpeername(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **name = va_arg(vl, struct sockaddr FAR **);
    int FAR **namelen = va_arg(vl, int FAR **);

    wsprintf(Buffer + Index, "getpeername() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_getsockname(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **name = va_arg(vl, struct sockaddr FAR **);
    int FAR **namelen = va_arg(vl, int FAR **);

    wsprintf(Buffer + Index, "getsockname() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_getsockopt(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    int *level = va_arg(vl, int *);
    int *optname = va_arg(vl, int *);
    char FAR **optval = va_arg(vl, char FAR **);
    int FAR **optlen = va_arg(vl, int FAR **);

    wsprintf(Buffer + Index, "getsockopt() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_htonl(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    u_long *RetVal = (u_long *)ReturnValue;
    u_long *hostlong = va_arg(vl, u_long *);

    wsprintf(Buffer + Index, "htonl() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_htons(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    u_short *RetVal = (u_short *)ReturnValue;
    u_short *hostshort = va_arg(vl, u_short *);

    wsprintf(Buffer + Index, "htons() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_ioctlsocket(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    long *cmd = va_arg(vl, long *);
    u_long FAR **argp = va_arg(vl, u_long FAR **);

    wsprintf(Buffer + Index, "ioctlsocket() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_listen(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    int *backlog = va_arg(vl, int *);

    wsprintf(Buffer + Index, "listen() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_ntohl(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    u_long *RetVal = (u_long *)ReturnValue;
    u_long *netlong = va_arg(vl, u_long *);

    wsprintf(Buffer + Index, "ntohl() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_ntohs(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    u_short *RetVal = (u_short *)ReturnValue;
    u_short *netshort = va_arg(vl, u_short *);

    wsprintf(Buffer + Index, "ntohs() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_recv(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    char FAR **buf = va_arg(vl, char FAR **);
    int *len = va_arg(vl, int *);
    int *flags = va_arg(vl, int *);

    wsprintf(Buffer + Index, "recv() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_recvfrom(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    char FAR **buf = va_arg(vl, char FAR **);
    int *len = va_arg(vl, int *);
    int *flags = va_arg(vl, int *);
    struct sockaddr FAR **from = va_arg(vl, struct sockaddr FAR **);
    int FAR **fromlen = va_arg(vl, int FAR **);

    wsprintf(Buffer + Index, "recvfrom() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_select(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    int *nfds = va_arg(vl, int *);
    fd_set FAR **readfds = va_arg(vl, fd_set FAR **);
    fd_set FAR **writefds = va_arg(vl, fd_set FAR **);
    fd_set FAR **exceptfds = va_arg(vl, fd_set FAR **);
    struct timeval FAR **timeout = va_arg(vl, struct timeval FAR **);

    wsprintf(Buffer + Index, "select() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_send(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    char FAR **buf = va_arg(vl, char FAR **);
    int *len = va_arg(vl, int *);
    int *flags = va_arg(vl, int *);

    wsprintf(Buffer + Index, "send() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_sendto(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    char FAR **buf = va_arg(vl, char FAR **);
    int *len = va_arg(vl, int *);
    int *flags = va_arg(vl, int *);
    struct sockaddr FAR **to = va_arg(vl, struct sockaddr FAR **);
    int *tolen = va_arg(vl, int *);

    wsprintf(Buffer + Index, "sendto() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_setsockopt(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    int *level = va_arg(vl, int *);
    int *optname = va_arg(vl, int *);
    char FAR **optval = va_arg(vl, char FAR **);
    int *optlen = va_arg(vl, int *);

    wsprintf(Buffer + Index, "setsockopt() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_shutdown(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    int *how = va_arg(vl, int *);

    wsprintf(Buffer + Index, "shutdown() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_socket(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    SOCKET *RetVal = (SOCKET *)ReturnValue;
    int *af = va_arg(vl, int *);
    int *type = va_arg(vl, int *);
    int *protocol = va_arg(vl, int *);

    wsprintf(Buffer + Index, "socket() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAAccept(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    SOCKET *RetVal = (SOCKET *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **addr = va_arg(vl, struct sockaddr FAR **);
    LPINT *addrlen = va_arg(vl, LPINT *);
    LPCONDITIONPROC *lpfnCondition = va_arg(vl, LPCONDITIONPROC *);
    DWORD *dwCallbackData = va_arg(vl, DWORD *);

    wsprintf(Buffer + Index, "WSAAccept() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAAsyncSelect(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    HWND *hWnd = va_arg(vl, HWND *);
    u_int *wMsg = va_arg(vl, u_int *);
    long *lEvent = va_arg(vl, long *);

    wsprintf(Buffer + Index, "WSAAsyncSelect() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSACancelBlockingCall(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    
    wsprintf(Buffer + Index, "WSACancelBlockingCall() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSACleanup(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    
    wsprintf(Buffer + Index, "WSACleanup() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSACloseEvent(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    BOOL *RetVal = (BOOL *)ReturnValue;
    WSAEVENT *hEvent = va_arg(vl, WSAEVENT *);

    wsprintf(Buffer + Index, "WSACloseEvent() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAConnect(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **name = va_arg(vl, struct sockaddr FAR **);
    int *namelen = va_arg(vl, int *);
    LPWSABUF *lpCallerData = va_arg(vl, LPWSABUF *);
    LPWSABUF *lpCalleeData = va_arg(vl, LPWSABUF *);
    LPQOS *lpSQOS = va_arg(vl, LPQOS *);
    LPQOS *lpGQOS = va_arg(vl, LPQOS *);

    wsprintf(Buffer + Index, "WSAConnect() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSACreateEvent(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    WSAEVENT *RetVal = (WSAEVENT *)ReturnValue;
    
    wsprintf(Buffer + Index, "WSACreateEvent() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSADuplicateSocketA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    DWORD *dwProcessId = va_arg(vl, DWORD *);
    LPWSAPROTOCOL_INFOA *lpProtocolInfo = va_arg(vl, LPWSAPROTOCOL_INFOA *);

    wsprintf(Buffer + Index, "WSADuplicateSocketA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSADuplicateSocketW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    DWORD *dwProcessId = va_arg(vl, DWORD *);
    LPWSAPROTOCOL_INFOW *lpProtocolInfo = va_arg(vl, LPWSAPROTOCOL_INFOW *);

    wsprintf(Buffer + Index, "WSADuplicateSocketW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAEnumNetworkEvents(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    WSAEVENT *hEventObject = va_arg(vl, WSAEVENT *);
    LPWSANETWORKEVENTS *lpNetworkEvents = va_arg(vl, LPWSANETWORKEVENTS *);

    wsprintf(Buffer + Index, "WSAEnumNetworkEvents() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAEnumProtocolsA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPINT *lpiProtocols = va_arg(vl, LPINT *);
    LPWSAPROTOCOL_INFOA *lpProtocolBuffer = va_arg(vl, LPWSAPROTOCOL_INFOA *);
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD *);

    wsprintf(Buffer + Index, "WSAEnumProtocolsA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAEnumProtocolsW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPINT *lpiProtocols = va_arg(vl, LPINT *);
    LPWSAPROTOCOL_INFOW *lpProtocolBuffer = va_arg(vl, LPWSAPROTOCOL_INFOW *);
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD *);

    wsprintf(Buffer + Index, "WSAEnumProtocolsW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAEventSelect(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    WSAEVENT *hEventObject = va_arg(vl, WSAEVENT *);
    long *lNetworkEvents = va_arg(vl, long *);

    wsprintf(Buffer + Index, "WSAEventSelect() %s.\r\n",
         PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAGetLastError(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    
    wsprintf(Buffer + Index, "WSAGetLastError() %s.\r\n",
         PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAGetOverlappedResult(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    BOOL *RetVal = (BOOL *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSAOVERLAPPED *lpOverlapped = va_arg(vl, LPWSAOVERLAPPED *);
    LPDWORD *lpcbTransfer = va_arg(vl, LPDWORD *);
    BOOL *fWait = va_arg(vl, BOOL *);
    LPDWORD *lpdwFlags = va_arg(vl, LPDWORD *);

    wsprintf(Buffer + Index, "WSAGetOverlappedResult() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAGetQOSByName(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    BOOL *RetVal = (BOOL *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpQOSName = va_arg(vl, LPWSABUF *);
    LPQOS *lpQOS = va_arg(vl, LPQOS *);

    wsprintf(Buffer + Index, "WSAGetQOSByName() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAHtonl(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    u_long *hostlong = va_arg(vl, u_long *);
    u_long FAR **lpnetlong = va_arg(vl, u_long FAR **);

    wsprintf(Buffer + Index, "WSAHtonl() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAHtons(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    u_short *hostshort = va_arg(vl, u_short *);
    u_short FAR **lpnetshort = va_arg(vl, u_short FAR **);

    wsprintf(Buffer + Index, "WSAHtons() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAIoctl(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    DWORD *dwIoControlCode = va_arg(vl, DWORD *);
    LPVOID *lpvInBuffer = va_arg(vl, LPVOID *);
    DWORD *cbInBuffer = va_arg(vl, DWORD *);
    LPVOID *lpvOutBuffer = va_arg(vl, LPVOID *);
    DWORD *cbOutBuffer = va_arg(vl, DWORD *);
    LPDWORD *lpcbBytesReturned = va_arg(vl, LPDWORD *);
    LPWSAOVERLAPPED *lpOverlapped = va_arg(vl, LPWSAOVERLAPPED *);
    LPWSAOVERLAPPED_COMPLETION_ROUTINE *lpCompletionRoutine = 
      va_arg(vl, LPWSAOVERLAPPED_COMPLETION_ROUTINE *);

    wsprintf(Buffer + Index, "WSAIoctl() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAIsBlocking(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    BOOL *RetVal = (BOOL *)ReturnValue;
    
    wsprintf(Buffer + Index, "WSAIsBlocking() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAJoinLeaf(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    SOCKET *RetVal = (SOCKET *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **name = va_arg(vl, struct sockaddr FAR **);
    int *namelen = va_arg(vl, int *);
    LPWSABUF *lpCallerData = va_arg(vl, LPWSABUF *);
    LPWSABUF *lpCalleeData = va_arg(vl, LPWSABUF *);
    LPQOS *lpSQOS = va_arg(vl, LPQOS *);
    LPQOS *lpGQOS = va_arg(vl, LPQOS *);
    DWORD *dwFlags = va_arg(vl, DWORD *);

    wsprintf(Buffer + Index, "WSAJoinLeaf() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSANtohl(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    u_long *netlong = va_arg(vl, u_long *);
    u_long FAR **lphostlong = va_arg(vl, u_long FAR **);

    wsprintf(Buffer + Index, "WSANtohl() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSANtohs(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    u_short *netshort = va_arg(vl, u_short *);
    u_short FAR **lphostshort = va_arg(vl, u_short FAR **);

    wsprintf(Buffer + Index, "WSANtohs() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSARecv(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpBuffers = va_arg(vl, LPWSABUF *);
    DWORD *dwBufferCount = va_arg(vl, DWORD *);
    LPDWORD *lpNumberOfBytesRecvd = va_arg(vl, LPDWORD *);
    LPDWORD *lpFlags = va_arg(vl, LPDWORD *);
    LPWSAOVERLAPPED *lpOverlapped = va_arg(vl, LPWSAOVERLAPPED *);
    LPWSAOVERLAPPED_COMPLETION_ROUTINE *lpCompletionRoutine = 
      va_arg(vl, LPWSAOVERLAPPED_COMPLETION_ROUTINE *);

    wsprintf(Buffer + Index, "WSARecv() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSARecvDisconnect(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpInboundDisconnectData = va_arg(vl, LPWSABUF *);

    wsprintf(Buffer + Index, "WSARecvDisconnect() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSARecvFrom(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpBuffers = va_arg(vl, LPWSABUF *);
    DWORD *dwBufferCount = va_arg(vl, DWORD *);
    LPDWORD *lpNumberOfBytesRecvd = va_arg(vl, LPDWORD *);
    LPDWORD *lpFlags = va_arg(vl, LPDWORD *);
    struct sockaddr FAR **lpFrom = va_arg(vl, struct sockaddr FAR **);
    LPINT *lpFromlen = va_arg(vl, LPINT *);
    LPWSAOVERLAPPED *lpOverlapped = va_arg(vl, LPWSAOVERLAPPED *);
    LPWSAOVERLAPPED_COMPLETION_ROUTINE *lpCompletionRoutine = 
      va_arg(vl, LPWSAOVERLAPPED_COMPLETION_ROUTINE *);

    wsprintf(Buffer + Index, "WSARecvFrom() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAResetEvent(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    BOOL *RetVal = (BOOL *)ReturnValue;
    WSAEVENT *hEvent = va_arg(vl, WSAEVENT *);

    wsprintf(Buffer + Index, "WSAResetEvent() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSASend(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpBuffers = va_arg(vl, LPWSABUF *);
    DWORD *dwBufferCount = va_arg(vl, DWORD *);
    LPDWORD *lpNumberOfBytesSent = va_arg(vl, LPDWORD *);
    DWORD *dwFlags = va_arg(vl, DWORD *);
    LPWSAOVERLAPPED *lpOverlapped = va_arg(vl, LPWSAOVERLAPPED *);
    LPWSAOVERLAPPED_COMPLETION_ROUTINE *lpCompletionRoutine = 
      va_arg(vl, LPWSAOVERLAPPED_COMPLETION_ROUTINE *);

    wsprintf(Buffer + Index, "WSASend() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSASendDisconnect(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpOutboundDisconnectData = va_arg(vl, LPWSABUF *);

    wsprintf(Buffer + Index, "WSASendDisconnect() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSASendTo(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpBuffers = va_arg(vl, LPWSABUF *);
    DWORD *dwBufferCount = va_arg(vl, DWORD *);
    LPDWORD *lpNumberOfBytesSent = va_arg(vl, LPDWORD *);
    DWORD *dwFlags = va_arg(vl, DWORD *);
    struct sockaddr FAR **lpTo = va_arg(vl, struct sockaddr FAR **);
    int *iTolen = va_arg(vl, int *);
    LPWSAOVERLAPPED *lpOverlapped = va_arg(vl, LPWSAOVERLAPPED *);
    LPWSAOVERLAPPED_COMPLETION_ROUTINE *lpCompletionRoutine = 
      va_arg(vl, LPWSAOVERLAPPED_COMPLETION_ROUTINE *);

    wsprintf(Buffer + Index, "WSASendTo() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSASetBlockingHook(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    FARPROC *RetVal = (FARPROC *)ReturnValue;
    FARPROC *lpBlockFunc = va_arg(vl, FARPROC *);

    wsprintf(Buffer + Index, "WSASetBlockingHook() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSASetEvent(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    BOOL *RetVal = (BOOL *)ReturnValue;
    WSAEVENT *hEvent = va_arg(vl, WSAEVENT *);

    wsprintf(Buffer + Index, "WSASetEvent() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSASetLastError(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    // WSASetLastError returns void, so ReturnValue should be NULL
    int *iError = va_arg(vl, int *);

    wsprintf(Buffer + Index, "WSASetLastError() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSASocketA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    SOCKET *RetVal = (SOCKET *)ReturnValue;
    int *af = va_arg(vl, int *);
    int *type = va_arg(vl, int *);
    int *protocol = va_arg(vl, int *);
    LPWSAPROTOCOL_INFOA *lpProtocolInfo = va_arg(vl, LPWSAPROTOCOL_INFOA *);
    GROUP *g = va_arg(vl, GROUP *);
    DWORD *dwFlags = va_arg(vl, DWORD *);

    wsprintf(Buffer + Index, "WSASocketA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSASocketW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    SOCKET *RetVal = (SOCKET *)ReturnValue;
    int *af = va_arg(vl, int *);
    int *type = va_arg(vl, int *);
    int *protocol = va_arg(vl, int *);
    LPWSAPROTOCOL_INFOW *lpProtocolInfo = va_arg(vl, LPWSAPROTOCOL_INFOW *);
    GROUP *g = va_arg(vl, GROUP *);
    DWORD *dwFlags = va_arg(vl, DWORD *);

    wsprintf(Buffer + Index, "WSASocketW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAStartup(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    WORD *wVersionRequested = va_arg(vl, WORD *);
    LPWSADATA *lpWSAData = va_arg(vl, LPWSADATA *);

    wsprintf(Buffer + Index, "WSAStartup() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAUnhookBlockingHook(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    
    wsprintf(Buffer + Index, "WSAUnhookBlockingHook() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAWaitForMultipleEvents(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    DWORD *RetVal = (DWORD *)ReturnValue;
    DWORD *cEvents = va_arg(vl, DWORD *);
    WSAEVENT FAR **lphEvents = va_arg(vl, WSAEVENT FAR **);
    BOOL *fWaitAll = va_arg(vl, BOOL *);
    DWORD *dwTimeout = va_arg(vl, DWORD *);
    BOOL *fAlertable = va_arg(vl, BOOL *);

    wsprintf(Buffer + Index, "WSAWaitForMultipleEvents() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_gethostbyaddr(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    struct hostent FAR **RetVal = (struct hostent FAR **)ReturnValue;
    char FAR **addr = va_arg(vl, char FAR **);
    int *len = va_arg(vl, int *);
    int *type = va_arg(vl, int *);

    wsprintf(Buffer + Index, "gethostbyaddr() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_gethostbyname(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    struct hostent FAR **RetVal = (struct hostent FAR **)ReturnValue;
    char FAR **name = va_arg(vl, char FAR **);

    wsprintf(Buffer + Index, "gethostbyname() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_gethostname(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    char FAR **name = va_arg(vl, char FAR **);
    int *namelen = va_arg(vl, int *);

    wsprintf(Buffer + Index, "gethostname() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_getprotobyname(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    struct protoent FAR **RetVal = (struct protoent FAR **)ReturnValue;
    char FAR **name = va_arg(vl, char FAR **);

    wsprintf(Buffer + Index, "getprotobyname() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_getprotobynumber(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    struct protoent FAR **RetVal = (struct protoent FAR **)ReturnValue;
    int *number = va_arg(vl, int *);

    wsprintf(Buffer + Index, "getprotobynumber() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_getservbyname(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    struct servent FAR **RetVal = (struct servent FAR **)ReturnValue;
    char FAR **name = va_arg(vl, char FAR **);
    char FAR **proto = va_arg(vl, char FAR **);

    wsprintf(Buffer + Index, "getservbyname() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_getservbyport(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    struct servent FAR **RetVal = (struct servent FAR **)ReturnValue;
    int *port = va_arg(vl, int *);
    char FAR **proto = va_arg(vl, char FAR **);

    wsprintf(Buffer + Index, "getservbyport() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_inet_addr(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    unsigned long *RetVal = (unsigned long *)ReturnValue;
    char FAR **cp = va_arg(vl, char FAR **);

    wsprintf(Buffer + Index, "inet_addr() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_inet_ntoa(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    char FAR **RetVal = (char FAR **)ReturnValue;
    struct in_addr *in = va_arg(vl, struct in_addr *);

    wsprintf(Buffer + Index, "inet_ntoa() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAAsyncGetHostByAddr(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    HANDLE *RetVal = (HANDLE *)ReturnValue;
    HWND *hWnd = va_arg(vl, HWND *);
    u_int *wMsg = va_arg(vl, u_int *);
    char FAR **addr = va_arg(vl, char FAR **);
    int *len = va_arg(vl, int *);
    int *type = va_arg(vl, int *);
    char FAR **buf = va_arg(vl, char FAR **);
    int *buflen = va_arg(vl, int *);

    wsprintf(Buffer + Index, "WSAAsyncGetHostByAddr() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAAsyncGetHostByName(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    HANDLE *RetVal = (HANDLE *)ReturnValue;
    HWND *hWnd = va_arg(vl, HWND *);
    u_int *wMsg = va_arg(vl, u_int *);
    char FAR **name = va_arg(vl, char FAR **);
    char FAR **buf = va_arg(vl, char FAR **);
    int *buflen = va_arg(vl, int *);

    wsprintf(Buffer + Index, "WSAAsyncGetHostByName() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAAsyncGetProtoByName(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    HANDLE *RetVal = (HANDLE *)ReturnValue;
    HWND *hWnd = va_arg(vl, HWND *);
    u_int *wMsg = va_arg(vl, u_int *);
    char FAR **name = va_arg(vl, char FAR **);
    char FAR **buf = va_arg(vl, char FAR **);
    int *buflen = va_arg(vl, int *);

    wsprintf(Buffer + Index, "WSAAsyncGetProtoByName() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAAsyncGetProtoByNumber(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    HANDLE *RetVal = (HANDLE *)ReturnValue;
    HWND *hWnd = va_arg(vl, HWND *);
    u_int *wMsg = va_arg(vl, u_int *);
    int *number = va_arg(vl, int *);
    char FAR **buf = va_arg(vl, char FAR **);
    int *buflen = va_arg(vl, int *);

    wsprintf(Buffer + Index, "WSAAsyncGetProtoByNumber() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAAsyncGetServByName(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    HANDLE *RetVal = (HANDLE *)ReturnValue;
    HWND *hWnd = va_arg(vl, HWND *);
    u_int *wMsg = va_arg(vl, u_int *);
    char FAR **name = va_arg(vl, char FAR **);
    char FAR **proto = va_arg(vl, char FAR **);
    char FAR **buf = va_arg(vl, char FAR **);
    int *buflen = va_arg(vl, int *);

    wsprintf(Buffer + Index, "WSAAsyncGetServByName() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAAsyncGetServByPort(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    HANDLE *RetVal = (HANDLE *)ReturnValue;
    HWND *hWnd = va_arg(vl, HWND *);
    u_int *wMsg = va_arg(vl, u_int *);
    int *port = va_arg(vl, int *);
    char FAR **proto = va_arg(vl, char FAR **);
    char FAR **buf = va_arg(vl, char FAR **);
    int *buflen = va_arg(vl, int *);

    wsprintf(Buffer + Index, "WSAAsyncGetServByPort() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSACancelAsyncRequest(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    HANDLE *hAsyncTaskHandle = va_arg(vl, HANDLE *);

    wsprintf(Buffer + Index, "WSACancelAsyncRequest() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPAccept(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    SOCKET *RetVal = (SOCKET *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **addr = va_arg(vl, struct sockaddr FAR **);
    INT FAR **addrlen = va_arg(vl, INT FAR **);
    LPCONDITIONPROC *lpfnCondition = va_arg(vl, LPCONDITIONPROC *);
    DWORD *dwCallbackData = va_arg(vl, DWORD *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPAccept() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPAsyncSelect(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    HWND *hWnd = va_arg(vl, HWND *);
    unsigned int *wMsg = va_arg(vl, unsigned int *);
    long *lEvent = va_arg(vl, long *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPAsyncSelect() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPBind(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **name = va_arg(vl, struct sockaddr FAR **);
    INT *namelen = va_arg(vl, INT *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPBind() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPCancelBlockingCall(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPCancelBlockingCall() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPCleanup(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPCleanup() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPCloseSocket(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPCloseSocket() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPConnect(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **name = va_arg(vl, struct sockaddr FAR **);
    INT *namelen = va_arg(vl, INT *);
    LPWSABUF *lpCallerData = va_arg(vl, LPWSABUF *);
    LPWSABUF *lpCalleeData = va_arg(vl, LPWSABUF *);
    LPQOS *lpSQOS = va_arg(vl, LPQOS *);
    LPQOS *lpGQOS = va_arg(vl, LPQOS *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPConnect() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPDuplicateSocket(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    DWORD *dwProcessID = va_arg(vl, DWORD *);
    LPWSAPROTOCOL_INFO *lpProtocolInfo = va_arg(vl, LPWSAPROTOCOL_INFO *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPDuplicateSocket() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPEnumNetworkEvents(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    WSAEVENT *hEventObject = va_arg(vl, WSAEVENT *);
    LPWSANETWORKEVENTS *lpNetworkEvents = va_arg(vl, LPWSANETWORKEVENTS *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPEnumNetworkEvents() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPEventSelect(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    WSAEVENT *hEventObject = va_arg(vl, WSAEVENT *);
    long *lNetworkEvents = va_arg(vl, long *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPEventSelect() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPGetOverlappedResult(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSAOVERLAPPED *lpOverlapped = va_arg(vl, LPWSAOVERLAPPED *);
    LPDWORD *lpcbTransfer = va_arg(vl, LPDWORD *);
    BOOL *fWait = va_arg(vl, BOOL *);
    LPDWORD *lpdwFlags = va_arg(vl, LPDWORD *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPGetOverlappedResult() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPGetPeerName(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **name = va_arg(vl, struct sockaddr FAR **);
    INT FAR **namelen = va_arg(vl, INT FAR **);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPGetPeerName() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_WSPGetSockName(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **name = va_arg(vl, struct sockaddr FAR **);
    INT FAR **namelen = va_arg(vl, INT FAR **);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPGetSockName() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPGetSockOpt(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    INT *level = va_arg(vl, INT *);
    INT *optname = va_arg(vl, INT *);
    char FAR **optval = va_arg(vl, char FAR **);
    INT FAR **optlen = va_arg(vl, INT FAR **);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPGetSockOpt() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPGetQOSByName(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpQOSName = va_arg(vl, LPWSABUF *);
    LPQOS *lpQOS = va_arg(vl, LPQOS *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPGetQOSByName() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPIoctl(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    DWORD *dwIoControlCode = va_arg(vl, DWORD *);
    LPVOID *lpvInBuffer = va_arg(vl, LPVOID *);
    DWORD *cbInBuffer = va_arg(vl, DWORD *);
    LPVOID *lpvOutBuffer = va_arg(vl, LPVOID *);
    DWORD *cbOutBuffer = va_arg(vl, DWORD *);
    LPDWORD *lpcbBytesReturned = va_arg(vl, LPDWORD *);
    LPWSAOVERLAPPED *lpOverlapped = va_arg(vl, LPWSAOVERLAPPED *);
    LPWSAOVERLAPPED_COMPLETION_ROUTINE *lpCompletionRoutine = 
      va_arg(vl, LPWSAOVERLAPPED_COMPLETION_ROUTINE *);
    LPWSATHREADID *lpThreadId = va_arg(vl, LPWSATHREADID *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPIoctl() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPJoinLeaf(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    struct sockaddr FAR **name = va_arg(vl, struct sockaddr FAR **);
    INT *namelen = va_arg(vl, INT *);
    LPWSABUF *lpCallerData = va_arg(vl, LPWSABUF *);
    LPWSABUF *lpCalleeData = va_arg(vl, LPWSABUF *);
    LPQOS *lpSQOS = va_arg(vl, LPQOS *);
    LPQOS *lpGQOS = va_arg(vl, LPQOS *);
    DWORD *dwFlags = va_arg(vl, DWORD *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPJoinLeaf() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPListen(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    INT *backlog = va_arg(vl, INT *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPListen() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPRecv(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpBuffers = va_arg(vl, LPWSABUF *);
    DWORD *dwBufferCount = va_arg(vl, DWORD *);
    LPDWORD *lpNumberOfBytesRecvd = va_arg(vl, LPDWORD *);
    LPDWORD *lpFlags = va_arg(vl, LPDWORD *);
    LPWSAOVERLAPPED *lpOverlapped = va_arg(vl, LPWSAOVERLAPPED *);
    LPWSAOVERLAPPED_COMPLETION_ROUTINE *lpCompletionRoutine = 
      va_arg(vl, LPWSAOVERLAPPED_COMPLETION_ROUTINE *);
    LPWSATHREADID *lpThreadId = va_arg(vl, LPWSATHREADID *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPRecv() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPRecvDisconnect(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpInboundDisconnectData = va_arg(vl, LPWSABUF *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPRecvDisconnect() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPRecvFrom(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpBuffers = va_arg(vl, LPWSABUF *);
    DWORD *dwBufferCount = va_arg(vl, DWORD *);
    LPDWORD *lpNumberOfBytesRecvd = va_arg(vl, LPDWORD *);
    LPDWORD *lpFlags = va_arg(vl, LPDWORD *);
    struct sockaddr FAR **lpFrom = va_arg(vl, struct sockaddr FAR **);
    LPINT *lpFromlen = va_arg(vl, LPINT *);
    LPWSAOVERLAPPED *lpOverlapped = va_arg(vl, LPWSAOVERLAPPED *);
    LPWSAOVERLAPPED_COMPLETION_ROUTINE *lpCompletionRoutine = 
      va_arg(vl, LPWSAOVERLAPPED_COMPLETION_ROUTINE *);
    LPWSATHREADID *lpThreadId = va_arg(vl, LPWSATHREADID *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPRecvFrom() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPSelect(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    INT *nfds = va_arg(vl, INT *);
    fd_set FAR **readfds = va_arg(vl, fd_set FAR **);
    fd_set FAR **writefds = va_arg(vl, fd_set FAR **);
    fd_set FAR **exceptfds = va_arg(vl, fd_set FAR **);
    struct timeval FAR **timeout = va_arg(vl, struct timeval FAR **);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPSelect() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPSend(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpBuffers = va_arg(vl, LPWSABUF *);
    DWORD *dwBufferCount = va_arg(vl, DWORD *);
    LPDWORD *lpNumberOfBytesSent = va_arg(vl, LPDWORD *);
    DWORD *dwFlags = va_arg(vl, DWORD *);
    LPWSAOVERLAPPED *lpOverlapped = va_arg(vl, LPWSAOVERLAPPED *);
    LPWSAOVERLAPPED_COMPLETION_ROUTINE *lpCompletionRoutine = 
      va_arg(vl, LPWSAOVERLAPPED_COMPLETION_ROUTINE *);
    LPWSATHREADID *lpThreadId = va_arg(vl, LPWSATHREADID *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPSend() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPSendDisconnect(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpOutboundDisconnectData = va_arg(vl, LPWSABUF *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPSendDisconnect() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPSendTo(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPWSABUF *lpBuffers = va_arg(vl, LPWSABUF *);
    DWORD *dbBufferCount = va_arg(vl, DWORD *);
    LPDWORD *lpNumberOfBytesSent = va_arg(vl, LPDWORD *);
    DWORD *dwFlags = va_arg(vl, DWORD *);
    struct sockaddr FAR **lpTo = va_arg(vl, struct sockaddr FAR **);
    INT *iTolen = va_arg(vl, INT *);
    LPWSAOVERLAPPED *lpOverlapped = va_arg(vl, LPWSAOVERLAPPED *);
    LPWSAOVERLAPPED_COMPLETION_ROUTINE *lpCompletionRoutine = 
      va_arg(vl, LPWSAOVERLAPPED_COMPLETION_ROUTINE *);
    LPWSATHREADID *lpThreadId = va_arg(vl, LPWSATHREADID *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPSendTo() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPSetSockOpt(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    INT *level = va_arg(vl, INT *);
    INT *optname = va_arg(vl, INT *);
    char FAR **optval = va_arg(vl, char FAR **);
    INT *optlen = va_arg(vl, INT *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPSetSockOpt() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPShutdown(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    INT *how = va_arg(vl, INT *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPShutdown() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPSocket(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    INT *RetVal = (INT *)ReturnValue;
    int *af = va_arg(vl, int *);
    int *type = va_arg(vl, int *);
    int *protocol = va_arg(vl, int *);
    LPWSAPROTOCOL_INFO *lpProtocolInfo = va_arg(vl, LPWSAPROTOCOL_INFO *);
    GROUP *g = va_arg(vl, GROUP *);
    DWORD *dwFlags = va_arg(vl, DWORD *);
    INT FAR **lpErrno = va_arg(vl, INT FAR **);

    wsprintf(Buffer + Index, "WSPSocket() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSPStartup(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    WORD *wVersionRequested = va_arg(vl, WORD *);
    LPWSPDATA *lpWSPData = va_arg(vl, LPWSPDATA *);
    LPWSAPROTOCOL_INFOA *lpProtocolInfo = va_arg(vl, LPWSAPROTOCOL_INFOA *);
    WSPUPCALLTABLE *UpcallTable = va_arg(vl, WSPUPCALLTABLE *);
    LPWSPPROC_TABLE *lpProcTable = va_arg(vl, LPWSPPROC_TABLE *);

    wsprintf(Buffer + Index, "WSPStartup() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WPUCloseEvent(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    BOOL *RetVal = (BOOL *)ReturnValue;
    WSAEVENT *hEvent = va_arg(vl, WSAEVENT *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WPUCloseEvent() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WPUCloseSocketHandle(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WPUCloseSocketHandle() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WPUCreateEvent(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    WSAEVENT *RetVal = (WSAEVENT *)ReturnValue;
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WPUCreateEvent() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WPUCreateSocketHandle(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    SOCKET *RetVal = (SOCKET *)ReturnValue;
    DWORD *dwCatalogEntryId = va_arg(vl, DWORD *);
    DWORD *dwContext = va_arg(vl, DWORD *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WPUCreateSocketHandle() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSCDeinstallProvider(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    DWORD *dwProviderId = va_arg(vl, DWORD *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WSCDeinstallProvider() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSCInstallProvider(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    char FAR **lpszProviderName = va_arg(vl, char FAR **);
    char FAR **lpszProviderDllPath = va_arg(vl, char FAR **);
    LPWSAPROTOCOL_INFO *lpProtocolInfoList = va_arg(vl, LPWSAPROTOCOL_INFO *);
    DWORD *dwNumberOfEntries = va_arg(vl, DWORD *);
    LPDWORD *lpdwProviderId = va_arg(vl, LPDWORD *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WSCInstallProvider() %s.\r\n",
         PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WPUModifyIFSHandle(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    SOCKET *RetVal = (SOCKET *)ReturnValue;
    DWORD *dwCatalogEntryId = va_arg(vl, DWORD *);
    SOCKET *ProposedHandle = va_arg(vl, SOCKET *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WPUModifyIFSHandle() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WPUQueryBlockingCallback(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    DWORD *dwCatalogEntryId = va_arg(vl, DWORD *);
    LPBLOCKINGCALLBACK FAR **lplpfnCallback = 
      va_arg(vl, LPBLOCKINGCALLBACK FAR **);
    LPDWORD *lpdwContext = va_arg(vl, LPDWORD *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WPUQueryBlockingCallback() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WPUQuerySocketHandleContext(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    LPDWORD *lpContext = va_arg(vl, LPDWORD *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WPUQuerySocketHandleContext() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WPUQueueApc(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSATHREADID *lpThreadId = va_arg(vl, LPWSATHREADID *);
    LPWSAUSERAPC *lpfnUserApc = va_arg(vl, LPWSAUSERAPC *);
    DWORD *dwContext = va_arg(vl, DWORD *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WPUQueueApc() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WPUResetEvent(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    BOOL *RetVal = (BOOL *)ReturnValue;
    WSAEVENT *hEvent = va_arg(vl, WSAEVENT *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WPUResetEvent() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WPUSetEvent(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    BOOL *RetVal = (BOOL *)ReturnValue;
    WSAEVENT *hEvent = va_arg(vl, WSAEVENT *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WPUSetEvent() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_WPUFDIsSet(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    fd_set FAR **set = va_arg(vl, fd_set FAR **);

    wsprintf(Buffer + Index, "WPUFDIsSet() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WPUGetProviderPath(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    DWORD *dwProviderId = va_arg(vl, DWORD *);
    char FAR **lpszProviderDllPath = va_arg(vl, char FAR **);
    LPINT *lpProviderDllPathLen = va_arg(vl, LPINT *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WPUGetProviderPath() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WPUPostMessage(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    BOOL *RetVal = (BOOL *)ReturnValue;
    HWND *hWnd = va_arg(vl, HWND *);
    UINT *Msg = va_arg(vl, UINT *);
    WPARAM *wParam = va_arg(vl, WPARAM *);
    LPARAM *lParam = va_arg(vl, LPARAM *);

    wsprintf(Buffer + Index, "WPUPostMessage() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSCEnumProtocols(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPINT *lpiProtocols = va_arg(vl, LPINT *);
    LPWSAPROTOCOL_INFO *lpProtocolBuffer = va_arg(vl, LPWSAPROTOCOL_INFO *);
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD *);
    LPINT *lpErrno = va_arg(vl, LPINT *);

    wsprintf(Buffer + Index, "WSCEnumProtocols() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler___WSAFDIsSet(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    SOCKET *s = va_arg(vl, SOCKET *);
    fd_set FAR **set = va_arg(vl, fd_set FAR **);

    wsprintf(Buffer + Index, "__WSAFDIsSet() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAAddressToStringA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPSOCKADDR *lpsaAddress = va_arg(vl, LPSOCKADDR *); 
    DWORD *dwAddressLength= va_arg(vl, DWORD *);
    LPWSAPROTOCOL_INFOA *lpProtocolInfo= va_arg(vl, LPWSAPROTOCOL_INFOA *);
    LPSTR *lpszAddressString= va_arg(vl, LPSTR *);
    LPDWORD *lpdwAddressStringLength= va_arg(vl, LPDWORD *);
      
    wsprintf(Buffer + Index, "WSAAddressToStringA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_WSAAddressToStringW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPSOCKADDR *lpsaAddress = va_arg(vl, LPSOCKADDR *); 
    DWORD *dwAddressLength= va_arg(vl, DWORD *);
    LPWSAPROTOCOL_INFOW *lpProtocolInfo= va_arg(vl, LPWSAPROTOCOL_INFOW *);
    LPWSTR *lpszAddressString= va_arg(vl, LPWSTR *);
    LPDWORD *lpdwAddressStringLength= va_arg(vl, LPDWORD *);
      
    wsprintf(Buffer + Index, "WSAAddressToStringW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_WSAStringToAddressA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPSTR *AddressString = va_arg(vl, LPSTR*);
    INT *AddressFamily = va_arg(vl, INT *);
    LPWSAPROTOCOL_INFOA *lpProtocolInfo = va_arg(vl, LPWSAPROTOCOL_INFOA*);
    LPSOCKADDR *lpAddress = va_arg(vl, LPSOCKADDR*);
    LPINT *lpAddressLength = va_arg(vl, LPINT*);
    
    wsprintf(Buffer + Index, "WSAStringToAddressA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAStringToAddressW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSTR *AddressString = va_arg(vl, LPWSTR*);
    INT *AddressFamily = va_arg(vl, INT *);
    LPWSAPROTOCOL_INFOW *lpProtocolInfo = va_arg(vl, LPWSAPROTOCOL_INFOW*);
    LPSOCKADDR *lpAddress = va_arg(vl, LPSOCKADDR*);
    LPINT *lpAddressLength = va_arg(vl, LPINT*);
    
    wsprintf(Buffer + Index, "WSAStringToAddressW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_WSALookupServiceBeginA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSAQUERYSETA *lpqsRestrictions = va_arg(vl, LPWSAQUERYSETA*);
    DWORD *dwControlFlags = va_arg(vl, DWORD*);
    LPHANDLE *lphLookup  = va_arg(vl, LPHANDLE*);
    
    wsprintf(Buffer + Index, "WSALookupServiceBeginA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSALookupServiceBeginW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSAQUERYSETW *lpqsRestrictions = va_arg(vl, LPWSAQUERYSETW*);
    DWORD *dwControlFlags = va_arg(vl, DWORD*);
    LPHANDLE *lphLookup  = va_arg(vl, LPHANDLE*);
    
    wsprintf(Buffer + Index, "WSALookupServiceBeginW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_WSALookupServiceNextA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    HANDLE *hLookup = va_arg(vl, HANDLE*);
    DWORD *dwControlFlags = va_arg(vl, DWORD*);
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD*);
    LPWSAQUERYSETA *lpqsResults = va_arg(vl, LPWSAQUERYSETA*);
 
    wsprintf(Buffer + Index, "WSALookupServiceNextA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSALookupServiceNextW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    HANDLE *hLookup = va_arg(vl, HANDLE*);
    DWORD *dwControlFlags = va_arg(vl, DWORD*);
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD*);
    LPWSAQUERYSETW *lpqsResults = va_arg(vl, LPWSAQUERYSETW*);
 
    wsprintf(Buffer + Index, "WSALookupServiceNextW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}




BOOL CALLBACK
DTHandler_WSALookupServiceEnd(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    HANDLE *hLookup = va_arg(vl, HANDLE*);
    
    wsprintf(Buffer + Index, "WSALookupServiceEnd() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}




BOOL CALLBACK
DTHandler_WSAGetAddressByNameA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPSTR *lpszServiceInstanceName = va_arg(vl, LPSTR*);
    LPGUID *lpServiceClassId = va_arg(vl, LPGUID*);
    DWORD *dwNameSpace = va_arg(vl, DWORD*);
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD*);
    LPWSAQUERYSETA *lpqsResults = va_arg(vl, LPWSAQUERYSETA*);
    DWORD *dwResolution = va_arg(vl, DWORD*);
    LPSTR *lpszAliasBuffer = va_arg(vl, LPSTR*);
    LPDWORD *lpdwAliasBufferLength  = va_arg(vl, LPDWORD*);
 
    wsprintf(Buffer + Index, "WSAGetAddressByNameA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAGetAddressByNameW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSTR *lpszServiceInstanceName = va_arg(vl, LPWSTR*);
    LPGUID *lpServiceClassId = va_arg(vl, LPGUID*);
    DWORD *dwNameSpace = va_arg(vl, DWORD*);
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD*);
    LPWSAQUERYSETW *lpqsResults = va_arg(vl, LPWSAQUERYSETW*);
    DWORD *dwResolution = va_arg(vl, DWORD*);
    LPWSTR *lpszAliasBuffer = va_arg(vl, LPWSTR*);
    LPDWORD *lpdwAliasBufferLength  = va_arg(vl, LPDWORD*);
 
    wsprintf(Buffer + Index, "WSAGetAddressByNameW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}




BOOL CALLBACK
DTHandler_WSAInstallServiceClassA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSASERVICECLASSINFOA *lpServiceClassInfo =
        va_arg(vl, LPWSASERVICECLASSINFOA*);
 
    wsprintf(Buffer + Index, "WSAInstallServiceClassA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAInstallServiceClassW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSASERVICECLASSINFOW *lpServiceClassInfo =
        va_arg(vl, LPWSASERVICECLASSINFOW*);
 
    wsprintf(Buffer + Index, "WSAInstallServiceClassW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_WSASetServiceA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSAQUERYSETA *lpqsRegInfo = va_arg(vl, LPWSAQUERYSETA*);
    WSAESETSERVICEOP *essOperation = va_arg(vl, WSAESETSERVICEOP*);
    DWORD *dwControlFlags = va_arg(vl, DWORD*);
 
    wsprintf(Buffer + Index, "WSASetServiceA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_WSASetServiceW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSAQUERYSETW *lpqsRegInfo = va_arg(vl, LPWSAQUERYSETW*);
    WSAESETSERVICEOP *essOperation = va_arg(vl, WSAESETSERVICEOP*);
    DWORD *dwControlFlags = va_arg(vl, DWORD*);
 
    wsprintf(Buffer + Index, "WSASetServiceW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_WSARemoveServiceClass(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPGUID *lpServiceClassId = va_arg(vl, LPGUID*);

    wsprintf(Buffer + Index, "WSARemoveServiceClass() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}




BOOL CALLBACK
DTHandler_WSAGetServiceClassInfoA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPGUID *lpProviderId = va_arg(vl, LPGUID*);
    LPGUID *lpServiceClassId = va_arg(vl, LPGUID*);
    LPDWORD *lpdwBufSize = va_arg(vl, LPDWORD*);
    LPWSASERVICECLASSINFOA *lpServiceClassInfo =
        va_arg(vl, LPWSASERVICECLASSINFOA*);

    wsprintf(Buffer + Index, "WSAGetServiceClassInfoA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_WSAGetServiceClassInfoW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPGUID *lpProviderId = va_arg(vl, LPGUID*);
    LPGUID *lpServiceClassId = va_arg(vl, LPGUID*);
    LPDWORD *lpdwBufSize = va_arg(vl, LPDWORD*);
    LPWSASERVICECLASSINFOW *lpServiceClassInfo =
        va_arg(vl, LPWSASERVICECLASSINFOW*);

    wsprintf(Buffer + Index, "WSAGetServiceClassInfoW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}




BOOL CALLBACK
DTHandler_WSAEnumNameSpaceProvidersA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD*);
    LPWSANAMESPACE_INFOA *Lpnspbuffer = va_arg(vl,  LPWSANAMESPACE_INFOA*);
 
    wsprintf(Buffer + Index, "WSAEnumNameSpaceProvidersA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAEnumNameSpaceProvidersW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD*);
    LPWSANAMESPACE_INFOW *Lpnspbuffer = va_arg(vl,  LPWSANAMESPACE_INFOW*);
 
    wsprintf(Buffer + Index, "WSAEnumNameSpaceProvidersW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}




BOOL CALLBACK
DTHandler_WSAGetServiceClassNameByClassIdA(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPGUID *lpServiceClassId = va_arg(vl, LPGUID*);
    LPSTR *lpszServiceClassName = va_arg(vl, LPSTR*);
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD*); 
     
    wsprintf(Buffer + Index, "WSAGetServiceClassNameByClassIdA() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_WSAGetServiceClassNameByClassIdW(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPGUID *lpServiceClassId = va_arg(vl, LPGUID*);
    LPWSTR *lpszServiceClassName = va_arg(vl, LPWSTR*);
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD*); 
     
    wsprintf(Buffer + Index, "WSAGetServiceClassNameByClassIdW() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}

BOOL CALLBACK
DTHandler_NSPAddressToString(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPSOCKADDR *lpsaAddress = va_arg(vl, LPSOCKADDR *); 
    DWORD *dwAddressLength= va_arg(vl, DWORD *);
    LPWSAPROTOCOL_INFOW *lpProtocolInfo= va_arg(vl, LPWSAPROTOCOL_INFOW *);
    LPWSTR *lpszAddressString= va_arg(vl, LPWSTR *);
    LPDWORD *lpdwAddressStringLength= va_arg(vl, LPDWORD *);
      
    wsprintf(Buffer + Index, "NSPAddressToString() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_NSPStringToAddress(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSTR *AddressString = va_arg(vl, LPWSTR*);
    INT *AddressFamily = va_arg(vl, INT *);
    LPWSAPROTOCOL_INFOW *lpProtocolInfo = va_arg(vl, LPWSAPROTOCOL_INFOW*);
    LPSOCKADDR *lpAddress = va_arg(vl, LPSOCKADDR*);
    LPINT *lpAddressLength = va_arg(vl, LPINT*);
    
    wsprintf(Buffer + Index, "NSPStringToAddress() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_NSPLookupServiceBegin(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSAQUERYSETW *lpqsRestrictions = va_arg(vl, LPWSAQUERYSETW*);
    DWORD *dwControlFlags = va_arg(vl, DWORD*);
    LPHANDLE *lphLookup  = va_arg(vl, LPHANDLE*);
    
    wsprintf(Buffer + Index, "NSPLookupServiceBegin() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_NSPLookupServiceNext(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    HANDLE *hLookup = va_arg(vl, HANDLE*);
    DWORD *dwControlFlags = va_arg(vl, DWORD*);
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD*);
    LPWSAQUERYSETW *lpqsResults = va_arg(vl, LPWSAQUERYSETW*);
 
    wsprintf(Buffer + Index, "NSPLookupServiceNext() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}




BOOL CALLBACK
DTHandler_NSPLookupServiceEnd(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    HANDLE *hLookup = va_arg(vl, HANDLE*);
    
    wsprintf(Buffer + Index, "NSPLookupServiceEnd() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_NSPGetAddressByName(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSTR *lpszServiceInstanceName = va_arg(vl, LPWSTR*);
    LPGUID *lpServiceClassId = va_arg(vl, LPGUID*);
    DWORD *dwNameSpace = va_arg(vl, DWORD*);
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD*);
    LPWSAQUERYSETW *lpqsResults = va_arg(vl, LPWSAQUERYSETW*);
    DWORD *dwResolution = va_arg(vl, DWORD*);
    LPWSTR *lpszAliasBuffer = va_arg(vl, LPWSTR*);
    LPDWORD *lpdwAliasBufferLength  = va_arg(vl, LPDWORD*);
 
    wsprintf(Buffer + Index, "NSPGetAddressByName() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_NSPInstallServiceClass(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSASERVICECLASSINFOW *lpServiceClassInfo =
        va_arg(vl, LPWSASERVICECLASSINFOW*);
 
    wsprintf(Buffer + Index, "NSPInstallServiceClass() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_NSPSetService(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPWSAQUERYSETW *lpqsRegInfo = va_arg(vl, LPWSAQUERYSETW*);
    WSAESETSERVICEOP *essOperation = va_arg(vl, WSAESETSERVICEOP*);
    DWORD *dwControlFlags = va_arg(vl, DWORD*);
 
    wsprintf(Buffer + Index, "NSPSetService() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_NSPRemoveServiceClass(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPGUID *lpServiceClassId = va_arg(vl, LPGUID*);

    wsprintf(Buffer + Index, "NSPRemoveServiceClass() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}



BOOL CALLBACK
DTHandler_NSPGetServiceClassInfo(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPGUID *lpProviderId = va_arg(vl, LPGUID*);
    LPGUID *lpServiceClassId = va_arg(vl, LPGUID*);
    LPDWORD *lpdwBufSize = va_arg(vl, LPDWORD*);
    LPWSASERVICECLASSINFOW *lpServiceClassInfo =
        va_arg(vl, LPWSASERVICECLASSINFOW*);

    wsprintf(Buffer + Index, "NSPGetServiceClassInfo() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}


BOOL CALLBACK
DTHandler_NSPGetServiceClassNameByClassId(
    IN     va_list vl,
    IN OUT LPVOID  ReturnValue,
    IN     LPSTR   LibraryName,
    OUT    char    *Buffer,
    IN     int     Index,
    IN     int     BufLen,
    IN     BOOL    PreOrPost)
{
    int *RetVal = (int *)ReturnValue;
    LPGUID *lpServiceClassId = va_arg(vl, LPGUID*);
    LPWSTR *lpszServiceClassName = va_arg(vl, LPWSTR*);
    LPDWORD *lpdwBufferLength = va_arg(vl, LPDWORD*); 
     
    wsprintf(Buffer + Index, "NSPGetServiceClassNameByClassId() %s.\r\n",
             PreOrPost ? "called" : "returned");
    DTTextOut(DebugWindow, LogFileHandle, Buffer, OutputStyle);
    return(FALSE);
}




