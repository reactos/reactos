/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 DLL
 * FILE:        include/ws2spi.h
 * PURPOSE:     Header file for the WinSock 2 DLL
 *              and WinSock 2 Service Providers
 */
#ifndef __WS2SPI_H
#define __WS2SPI_H

#include <winsock2.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define WSPAPI WSAAPI


#define WSPDESCRIPTION_LEN 255

typedef struct WSPData {
    WORD wVersion;
    WORD wHighVersion;
    WCHAR szDescription[WSPDESCRIPTION_LEN + 1];
} WSPDATA, FAR * LPWSPDATA;


typedef struct _WSATHREADID {
    HANDLE ThreadHandle;
    DWORD Reserved;
} WSATHREADID, FAR* LPWSATHREADID;


typedef BOOL (CALLBACK FAR* LPBLOCKINGCALLBACK)(
    DWORD dwContext);

typedef VOID (CALLBACK FAR* LPWSAUSERAPC)(
    DWORD dwContext);


/* Prototypes for service provider procedure table */

typedef SOCKET (WSPAPI * LPWSPACCEPT)(
    IN      SOCKET s,
    OUT     LPSOCKADDR addr,
    IN OUT  LPINT addrlen,
    IN      LPCONDITIONPROC lpfnCondition,
    IN      DWORD dwCallbackData,
    OUT     LPINT lpErrno);

typedef INT (WSPAPI * LPWSPADDRESSTOSTRING)(
    IN      LPSOCKADDR lpsaAddress,
    IN      DWORD dwAddressLength,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPWSTR lpszAddressString,
    IN OUT  LPDWORD lpdwAddressStringLength,
    OUT     LPINT lpErrno);

typedef INT (WSPAPI * LPWSPASYNCSELECT)(
    IN  SOCKET s, 
    IN  HWND hWnd, 
    IN  UINT wMsg, 
    IN  LONG lEvent, 
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPBIND)(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name, 
    IN  INT namelen, 
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPCANCELBLOCKINGCALL)(
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPCLEANUP)(
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPCLOSESOCKET)(
    IN	SOCKET s,
    OUT	LPINT lpErrno);

typedef INT (WSPAPI * LPWSPCONNECT)(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
    IN  INT namelen,
    IN  LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN  LPQOS lpSQOS,
    IN  LPQOS lpGQOS,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPDUPLICATESOCKET)(
    IN  SOCKET s,
    IN  DWORD dwProcessId,
    OUT LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPENUMNETWORKEVENTS)(
    IN  SOCKET s,
    IN  WSAEVENT hEventObject,
    OUT LPWSANETWORKEVENTS lpNetworkEvents,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPEVENTSELECT)(
    IN  SOCKET s,
    IN  WSAEVENT hEventObject,
    IN  LONG lNetworkEvents,
    OUT LPINT lpErrno);

typedef BOOL (WSPAPI * LPWSPGETOVERLAPPEDRESULT)(
    IN  SOCKET s,
    IN  LPWSAOVERLAPPED lpOverlapped,
    OUT LPDWORD lpcbTransfer,
    IN  BOOL fWait,
    OUT LPDWORD lpdwFlags,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPGETPEERNAME)(
    IN      SOCKET s, 
    OUT     LPSOCKADDR name, 
    IN OUT  LPINT namelen, 
    OUT     LPINT lpErrno);

typedef BOOL (WSPAPI * LPWSPGETQOSBYNAME)(
    IN      SOCKET s,
    IN OUT  LPWSABUF lpQOSName,
    OUT     LPQOS lpQOS,
    OUT     LPINT lpErrno);

typedef INT (WSPAPI * LPWSPGETSOCKNAME)(
    IN      SOCKET s,
    OUT     LPSOCKADDR name,
    IN OUT  LPINT namelen,
    OUT     LPINT lpErrno);

typedef INT (WSPAPI * LPWSPGETSOCKOPT)(
    IN      SOCKET s,
    IN      INT level,
    IN      INT optname,
    OUT	    CHAR FAR* optval,
    IN OUT  LPINT optlen,
    OUT     LPINT lpErrno);

typedef INT (WSPAPI * LPWSPIOCTL)(
    IN  SOCKET s,
    IN  DWORD dwIoControlCode,
    IN  LPVOID lpvInBuffer,
    IN  DWORD cbInBuffer,
    OUT LPVOID lpvOutBuffer,
    IN  DWORD cbOutBuffer,
    OUT LPDWORD lpcbBytesReturned,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno);

typedef SOCKET (WSPAPI * LPWSPJOINLEAF)(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
    IN  INT namelen,
    IN  LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN  LPQOS lpSQOS,
    IN  LPQOS lpGQOS,
    IN  DWORD dwFlags,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPLISTEN)(
    IN  SOCKET s,
    IN  INT backlog,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPRECV)(
    IN      SOCKET s,
    IN OUT  LPWSABUF lpBuffers,
    IN      DWORD dwBufferCount,
    OUT     LPDWORD lpNumberOfBytesRecvd,
    IN OUT  LPDWORD lpFlags,
    IN      LPWSAOVERLAPPED lpOverlapped,
    IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN      LPWSATHREADID lpThreadId,
    OUT     LPINT lpErrno);

typedef INT (WSPAPI * LPWSPRECVDISCONNECT)(
    IN  SOCKET s,
    OUT LPWSABUF lpInboundDisconnectData,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPRECVFROM)(
    IN      SOCKET s,
    IN OUT  LPWSABUF lpBuffers,
    IN      DWORD dwBufferCount,
    OUT     LPDWORD lpNumberOfBytesRecvd,
    IN OUT  LPDWORD lpFlags,
    OUT     LPSOCKADDR lpFrom,
    IN OUT  LPINT lpFromlen,
    IN      LPWSAOVERLAPPED lpOverlapped,
    IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN      LPWSATHREADID lpThreadId,
    OUT     LPINT lpErrno);

typedef INT (WSPAPI * LPWSPSELECT)(
    IN      INT nfds,
    IN OUT  LPFD_SET readfds,
    IN OUT  LPFD_SET writefds,
    IN OUT  LPFD_SET exceptfds,
    IN      CONST LPTIMEVAL timeout,
    OUT     LPINT lpErrno);

typedef INT (WSPAPI * LPWSPSEND)(
    IN  SOCKET s,
    IN  LPWSABUF lpBuffers,
    IN  DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesSent,
    IN  DWORD dwFlags,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPSENDDISCONNECT)(
    IN  SOCKET s,
    IN  LPWSABUF lpOutboundDisconnectData,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPSENDTO)(
    IN  SOCKET s,
    IN  LPWSABUF lpBuffers,
    IN  DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesSent,
    IN  DWORD dwFlags,
    IN  CONST LPSOCKADDR lpTo,
    IN  INT iTolen,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPSETSOCKOPT)(
    IN  SOCKET s,
    IN  INT level,
    IN  INT optname,
    IN  CONST CHAR FAR* optval,
    IN  INT optlen,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPSHUTDOWN)(
    IN  SOCKET s,
    IN  INT how,
    OUT LPINT lpErrno);

typedef SOCKET (WSPAPI * LPWSPSOCKET)(
    IN  INT af,
    IN  INT type,
    IN  INT protocol,
    IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN  GROUP g,
    IN  DWORD dwFlags,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWSPSTRINGTOADDRESS)(
    IN      LPWSTR AddressString,
    IN      INT AddressFamily,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPSOCKADDR lpAddress,
    IN OUT  LPINT lpAddressLength,
    OUT     LPINT lpErrno);


/* Service provider procedure table */
typedef struct _WSPPROC_TABLE {
    LPWSPACCEPT                 lpWSPAccept;
    LPWSPADDRESSTOSTRING        lpWSPAddressToString;
    LPWSPASYNCSELECT            lpWSPAsyncSelect;
    LPWSPBIND                   lpWSPBind;
    LPWSPCANCELBLOCKINGCALL     lpWSPCancelBlockingCall;
    LPWSPCLEANUP                lpWSPCleanup;
    LPWSPCLOSESOCKET            lpWSPCloseSocket;
    LPWSPCONNECT                lpWSPConnect;
    LPWSPDUPLICATESOCKET        lpWSPDuplicateSocket;
    LPWSPENUMNETWORKEVENTS      lpWSPEnumNetworkEvents;
    LPWSPEVENTSELECT            lpWSPEventSelect;
    LPWSPGETOVERLAPPEDRESULT    lpWSPGetOverlappedResult;
    LPWSPGETPEERNAME            lpWSPGetPeerName;
    LPWSPGETSOCKNAME            lpWSPGetSockName;
    LPWSPGETSOCKOPT             lpWSPGetSockOpt;
    LPWSPGETQOSBYNAME           lpWSPGetQOSByName;
    LPWSPIOCTL                  lpWSPIoctl;
    LPWSPJOINLEAF               lpWSPJoinLeaf;
    LPWSPLISTEN                 lpWSPListen;
    LPWSPRECV                   lpWSPRecv;
    LPWSPRECVDISCONNECT         lpWSPRecvDisconnect;
    LPWSPRECVFROM               lpWSPRecvFrom;
    LPWSPSELECT                 lpWSPSelect;
    LPWSPSEND                   lpWSPSend;
    LPWSPSENDDISCONNECT         lpWSPSendDisconnect;
    LPWSPSENDTO                 lpWSPSendTo;
    LPWSPSETSOCKOPT             lpWSPSetSockOpt;
    LPWSPSHUTDOWN               lpWSPShutdown;
    LPWSPSOCKET                 lpWSPSocket;
    LPWSPSTRINGTOADDRESS        lpWSPStringToAddress;
} WSPPROC_TABLE, FAR* LPWSPPROC_TABLE;


/* Prototypes for service provider upcall procedure table */

typedef BOOL (WSPAPI * LPWPUCLOSEEVENT)(
    IN  WSAEVENT hEvent,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWPUCLOSESOCKETHANDLE)(
    IN  SOCKET s,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWPUCLOSETHREAD)(
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno);

typedef WSAEVENT (WSPAPI * LPWPUCREATEEVENT)(
    OUT LPINT lpErrno);

typedef SOCKET (WSPAPI * LPWPUCREATESOCKETHANDLE)(
    IN  DWORD dwCatalogEntryId,
    IN  DWORD dwContext,
    OUT LPINT lpErrno);

typedef SOCKET (WSPAPI * LPWPUFDISSET)(
    IN  SOCKET s,
    IN  LPFD_SET set);

typedef INT (WSPAPI * LPWPUGETPROVIDERPATH)(
    IN      LPGUID lpProviderId,
    OUT     LPWSTR lpszProviderDllPath,
    IN OUT  LPINT lpProviderDllPathLen,
    OUT     LPINT lpErrno);

typedef SOCKET (WSPAPI * LPWPUMODIFYIFSHANDLE)(
    IN  DWORD dwCatalogEntryId,
    IN  SOCKET ProposedHandle,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWPUOPENCURRENTTHREAD)(
    OUT LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno);

typedef BOOL (WSPAPI * LPWPUPOSTMESSAGE)(
    IN  HWND hWnd,
    IN  UINT Msg,
    IN  WPARAM wParam,
    IN  LPARAM lParam);

typedef INT (WSPAPI * LPWPUQUERYBLOCKINGCALLBACK)(
    IN  DWORD dwCatalogEntryId,
    OUT LPBLOCKINGCALLBACK FAR* lplpfnCallback,
    OUT LPDWORD lpdwContext,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWPUQUERYSOCKETHANDLECONTEXT)(
    IN  SOCKET s,
    OUT LPDWORD lpContext,
    OUT LPINT lpErrno);

typedef INT (WSPAPI * LPWPUQUEUEAPC)(
    IN  LPWSATHREADID lpThreadId,
    IN  LPWSAUSERAPC lpfnUserApc,
    IN  DWORD dwContext,
    OUT LPINT lpErrno);

typedef BOOL (WSPAPI * LPWPURESETEVENT)(
    IN  WSAEVENT hEvent,
    OUT LPINT lpErrno);

typedef BOOL (WSPAPI * LPWPUSETEVENT)(
    IN  WSAEVENT hEvent,
    OUT LPINT lpErrno);


/* Available only directly from the DLL */

typedef INT (WSPAPI * LPWPUCOMPLETEOVERLAPPEDREQUEST)(
    SOCKET s,
    LPWSAOVERLAPPED lpOverlapped,
    DWORD dwError,
    DWORD cbTransferred,
    LPINT lpErrno);


/* Service Provider upcall table */

typedef struct _WSPUPCALLTABLE {
    LPWPUCLOSEEVENT                 lpWPUCloseEvent;
    LPWPUCLOSESOCKETHANDLE          lpWPUCloseSocketHandle;
    LPWPUCREATEEVENT                lpWPUCreateEvent;
    LPWPUCREATESOCKETHANDLE         lpWPUCreateSocketHandle;
    LPWPUFDISSET                    lpWPUFDIsSet;
    LPWPUGETPROVIDERPATH            lpWPUGetProviderPath;
    LPWPUMODIFYIFSHANDLE            lpWPUModifyIFSHandle;
    LPWPUPOSTMESSAGE                lpWPUPostMessage;
    LPWPUQUERYBLOCKINGCALLBACK      lpWPUQueryBlockingCallback;
    LPWPUQUERYSOCKETHANDLECONTEXT   lpWPUQuerySocketHandleContext;
    LPWPUQUEUEAPC                   lpWPUQueueApc;
    LPWPURESETEVENT                 lpWPUResetEvent;
    LPWPUSETEVENT                   lpWPUSetEvent;
    LPWPUOPENCURRENTTHREAD          lpWPUOpenCurrentThread;
    LPWPUCLOSETHREAD                lpWPUCloseThread;
} WSPUPCALLTABLE, FAR* LPWSPUPCALLTABLE;


typedef INT (WSPAPI * LPWSPSTARTUP)(
    IN  WORD wVersionRequested,
    OUT LPWSPDATA lpWSPData,
    IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN  WSPUPCALLTABLE UpcallTable,
    OUT LPWSPPROC_TABLE lpProcTable);


/* Prototypes for service provider namespace procedure table */

typedef INT (WSPAPI * LPNSPCLEANUP)(
    IN  LPGUID lpProviderId);

typedef INT (WSPAPI * LPNSPGETSERVICECLASSINFO)(
    IN      LPGUID lpProviderId,
    IN OUT  LPDWORD lpdwBufSize,
    IN OUT  LPWSASERVICECLASSINFOW lpServiceClassInfo);

typedef INT (WSPAPI * LPNSPINSTALLSERVICECLASS)(
    IN  LPGUID lpProviderId,
    IN  LPWSASERVICECLASSINFOW lpServiceClassInfo);

typedef INT (WSPAPI * LPNSPLOOKUPSERVICEBEGIN)(
    IN  LPGUID lpProviderId,
    IN  LPWSAQUERYSETW lpqsRestrictions,
    IN  LPWSASERVICECLASSINFOW lpServiceClassInfo,
    IN  DWORD dwControlFlags,
    OUT LPHANDLE lphLookup);

typedef INT (WSPAPI * LPNSPLOOKUPSERVICEEND)(
    IN  HANDLE hLookup);

typedef INT (WSPAPI * LPNSPLOOKUPSERVICENEXT)(
    IN      HANDLE hLookup,
    IN      DWORD dwControlFlags,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSAQUERYSET lpqsResults);

typedef INT (WSPAPI * LPNSPREMOVESERVICECLASS)(
    IN  LPGUID lpProviderId,
    IN  LPGUID lpServiceClassId);

typedef INT (WSPAPI * LPNSPSETSERVICE)(
    IN  LPGUID lpProviderId,
    IN  LPWSASERVICECLASSINFOW lpServiceClassInfo,
    IN  LPWSAQUERYSETW lpqsRegInfo,
    IN  WSAESETSERVICEOP essOperation,
    IN  DWORD dwControlFlags);


typedef struct _NSP_ROUTINE {
    DWORD                       cbSize;
    DWORD                       dwMajorVersion;
    DWORD                       dwMinorVersion;
    LPNSPCLEANUP                NSPCleanup;
    LPNSPLOOKUPSERVICEBEGIN     NSPLookupServiceBegin;
    LPNSPLOOKUPSERVICENEXT      NSPLookupServiceNext;
    LPNSPLOOKUPSERVICEEND       NSPLookupServiceEnd;
    LPNSPSETSERVICE             NSPSetService;
    LPNSPINSTALLSERVICECLASS    NSPInstallServiceClass;
    LPNSPREMOVESERVICECLASS     NSPRemoveServiceClass;
    LPNSPGETSERVICECLASSINFO    NSPGetServiceClassInfo;
} NSP_ROUTINE, *PNSP_ROUTINE, *LPNSP_ROUTINE; 


INT
WSPAPI
NSPStartup(
    IN  LPGUID lpProviderId,
    OUT LPNSP_ROUTINE lpNspRoutines);


/* WinSock 2 DLL function prototypes */

INT
WSPAPI
WPUCompleteOverlappedRequest(
    IN  SOCKET s,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  DWORD dwError,
    IN  DWORD cbTransferred,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPStartup(
    IN  WORD wVersionRequested,
    OUT LPWSPDATA lpWSPData,
    IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN  WSPUPCALLTABLE UpcallTable,
    OUT LPWSPPROC_TABLE lpProcTable);

INT
WSPAPI
WSCDeinstallProvider(
    IN  LPGUID lpProviderId,
    OUT LPINT lpErrno);

INT
WSPAPI
WSCEnumProtocols(
    IN      LPINT lpiProtocols,
    OUT     LPWSAPROTOCOL_INFOW lpProtocolBuffer,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSCGetProviderPath(
    IN      LPGUID lpProviderId,
    OUT     LPWSTR lpszProviderDllPath,
    IN OUT  LPINT lpProviderDllPathLen,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSCInstallProvider(
    IN  CONST LPGUID lpProviderId,
    IN  CONST LPWSTR lpszProviderDllPath,
    IN  CONST LPWSAPROTOCOL_INFOW lpProtocolInfoList,
    IN  DWORD dwNumberOfEntries,
    OUT LPINT lpErrno);

INT
WSPAPI
WSCEnableNSProvider(
    IN  LPGUID lpProviderId,
    IN  BOOL fEnable);

INT
WSPAPI
WSCInstallNameSpace(
    IN  LPWSTR lpszIdentifier,
    IN  LPWSTR lpszPathName,
    IN  DWORD dwNameSpace,
    IN  DWORD dwVersion,
    IN  LPGUID lpProviderId);

INT
WSPAPI
WSCUnInstallNameSpace(
    IN  LPGUID lpProviderId);

INT
WSPAPI
WSCWriteProviderOrder(
    IN  LPDWORD lpwdCatalogEntryId,
    IN  DWORD dwNumberOfEntries);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __WS2SPI_H */

/* EOF */
