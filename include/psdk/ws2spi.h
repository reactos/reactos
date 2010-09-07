/*
 * ws2spi.h
 *
 * Winsock 2 Service Provider interface.
 *
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#pragma once

#define _WS2SPI_H

#ifndef _WINSOCK2API_
#include <winsock2.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_WIN64)
#include <pshpack4.h>
#endif

#define WSPDESCRIPTION_LEN 255

#define WSS_OPERATION_IN_PROGRESS 0x00000103L

#define WSPAPI WSAAPI

typedef struct _WSATHREADID {
  HANDLE ThreadHandle;
  DWORD_PTR Reserved;
} WSATHREADID, FAR *LPWSATHREADID;

typedef struct WSPData {
  WORD wVersion;
  WORD wHighVersion;
  WCHAR szDescription[WSPDESCRIPTION_LEN+1];
} WSPDATA, FAR *LPWSPDATA;

typedef BOOL
(CALLBACK FAR *LPBLOCKINGCALLBACK)(
  DWORD_PTR dwContext);

typedef SOCKET
(WSPAPI *LPWSPACCEPT)(
  IN SOCKET s,
  OUT struct sockaddr FAR *addr OPTIONAL,
  IN OUT LPINT addrlen OPTIONAL,
  IN LPCONDITIONPROC lpfnCondition OPTIONAL,
  IN DWORD_PTR dwCallbackData OPTIONAL,
  OUT LPINT lpErrno);

typedef VOID
(CALLBACK FAR *LPWSAUSERAPC)(
  DWORD_PTR dwContext);

typedef INT
(WSPAPI *LPWSPADDRESSTOSTRING)(
  IN LPSOCKADDR lpsaAddress,
  IN DWORD dwAddressLength,
  IN LPWSAPROTOCOL_INFOW lpProtocolInfo OPTIONAL,
  OUT LPWSTR lpszAddressString,
  IN OUT LPDWORD lpdwAddressStringLength,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPASYNCSELECT)(
  IN SOCKET s,
  IN HWND hWnd,
  IN unsigned int wMsg,
  IN long lEvent,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPBIND)(
  IN SOCKET s,
  IN const struct sockaddr FAR *name,
  IN int namelen,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPCANCELBLOCKINGCALL)(
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPCLEANUP)(
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPCLOSESOCKET)(
  IN SOCKET s,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPCONNECT)(
  IN SOCKET s,
  IN const struct sockaddr FAR *name,
  IN int namelen,
  IN LPWSABUF lpCallerData OPTIONAL,
  OUT LPWSABUF lpCalleeData OPTIONAL,
  IN LPQOS lpSQOS OPTIONAL,
  IN LPQOS lpGQOS OPTIONAL,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPDUPLICATESOCKET)(
  IN SOCKET s,
  IN DWORD dwProcessId,
  OUT LPWSAPROTOCOL_INFOW lpProtocolInfo,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPENUMNETWORKEVENTS)(
  IN SOCKET s,
  IN WSAEVENT hEventObject,
  OUT LPWSANETWORKEVENTS lpNetworkEvents,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPEVENTSELECT)(
  IN SOCKET s,
  IN WSAEVENT hEventObject,
  IN long lNetworkEvents,
  OUT LPINT lpErrno);

typedef BOOL
(WSPAPI *LPWSPGETOVERLAPPEDRESULT)(
  IN SOCKET s,
  IN LPWSAOVERLAPPED lpOverlapped,
  OUT LPDWORD lpcbTransfer,
  IN BOOL fWait,
  OUT LPDWORD lpdwFlags,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPGETPEERNAME)(
  IN SOCKET s,
  OUT struct sockaddr FAR *name,
  IN OUT LPINT namelen,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPGETSOCKNAME)(
  IN SOCKET s,
  OUT struct sockaddr FAR *name,
  IN OUT LPINT namelen,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPGETSOCKOPT)(
  IN SOCKET s,
  IN int level,
  IN int optname,
  OUT char FAR *optval,
  IN OUT LPINT optlen,
  OUT LPINT lpErrno);

typedef BOOL
(WSPAPI *LPWSPGETQOSBYNAME)(
  IN SOCKET s,
  IN LPWSABUF lpQOSName,
  OUT LPQOS lpQOS,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPIOCTL)(
  IN SOCKET s,
  IN DWORD dwIoControlCode,
  IN LPVOID lpvInBuffer OPTIONAL,
  IN DWORD cbInBuffer,
  OUT LPVOID lpvOutBuffer OPTIONAL,
  IN DWORD cbOutBuffer,
  OUT LPDWORD lpcbBytesReturned,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  IN LPWSATHREADID lpThreadId OPTIONAL,
  OUT LPINT lpErrno);

typedef SOCKET
(WSPAPI *LPWSPJOINLEAF)(
  IN SOCKET s,
  IN const struct sockaddr FAR *name,
  IN int namelen,
  IN LPWSABUF lpCallerData OPTIONAL,
  OUT LPWSABUF lpCalleeData OPTIONAL,
  IN LPQOS lpSQOS OPTIONAL,
  IN LPQOS lpGQOS OPTIONAL,
  IN DWORD dwFlags,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPLISTEN)(
  IN SOCKET s,
  IN int backlog,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPRECV)(
  IN SOCKET s,
  IN LPWSABUF lpBuffers,
  IN DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesRecvd OPTIONAL,
  IN OUT LPDWORD lpFlags,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  IN LPWSATHREADID lpThreadId OPTIONAL,
  IN LPINT lpErrno);

typedef int
(WSPAPI *LPWSPRECVDISCONNECT)(
  IN SOCKET s,
  IN LPWSABUF lpInboundDisconnectData OPTIONAL,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPRECVFROM)(
  IN SOCKET s,
  IN LPWSABUF lpBuffers,
  IN DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesRecvd OPTIONAL,
  IN OUT LPDWORD lpFlags,
  OUT struct sockaddr FAR *lpFrom OPTIONAL,
  IN OUT LPINT lpFromlen OPTIONAL,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  IN LPWSATHREADID lpThreadId OPTIONAL,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPSELECT)(
  IN int nfds,
  IN OUT fd_set FAR *readfds OPTIONAL,
  IN OUT fd_set FAR *writefds OPTIONAL,
  IN OUT fd_set FAR *exceptfds OPTIONAL,
  IN const struct timeval FAR *timeout OPTIONAL,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPSEND)(
  IN SOCKET s,
  IN LPWSABUF lpBuffers,
  IN DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesSent OPTIONAL,
  IN DWORD dwFlags,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  IN LPWSATHREADID lpThreadId OPTIONAL,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPSENDDISCONNECT)(
  IN SOCKET s,
  IN LPWSABUF lpOutboundDisconnectData OPTIONAL,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPSENDTO)(
  IN SOCKET s,
  IN LPWSABUF lpBuffers,
  IN DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesSent OPTIONAL,
  IN DWORD dwFlags,
  IN const struct sockaddr FAR *lpTo OPTIONAL,
  IN int iTolen,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL,
  IN LPWSATHREADID lpThreadId OPTIONAL,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPSETSOCKOPT)(
  IN SOCKET s,
  IN int level,
  IN int optname,
  IN const char FAR *optval OPTIONAL,
  IN int optlen,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSPSHUTDOWN)(
  IN SOCKET s,
  IN int how,
  OUT LPINT lpErrno);

typedef SOCKET
(WSPAPI *LPWSPSOCKET)(
  IN int af,
  IN int type,
  IN int protocol,
  IN LPWSAPROTOCOL_INFOW lpProtocolInfo OPTIONAL,
  IN GROUP g,
  IN DWORD dwFlags,
  OUT LPINT lpErrno);

typedef INT
(WSPAPI *LPWSPSTRINGTOADDRESS)(
  IN LPWSTR AddressString,
  IN INT AddressFamily,
  IN LPWSAPROTOCOL_INFOW lpProtocolInfo OPTIONAL,
  OUT LPSOCKADDR lpAddress,
  IN OUT LPINT lpAddressLength,
  OUT LPINT lpErrno);

typedef BOOL
(WSPAPI *LPWPUCLOSEEVENT)(
  IN WSAEVENT hEvent,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWPUCLOSESOCKETHANDLE)(
  IN SOCKET s,
  OUT LPINT lpErrno);

typedef WSAEVENT
(WSPAPI *LPWPUCREATEEVENT)(
  OUT LPINT lpErrno);

typedef SOCKET
(WSPAPI *LPWPUCREATESOCKETHANDLE)(
  IN DWORD dwCatalogEntryId,
  IN DWORD_PTR dwContext,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWPUFDISSET)(
  IN SOCKET s,
  IN fd_set FAR *fdset);

typedef int
(WSPAPI *LPWPUGETPROVIDERPATH)(
  IN LPGUID lpProviderId,
  OUT WCHAR FAR *lpszProviderDllPath,
  IN OUT LPINT lpProviderDllPathLen,
  OUT LPINT lpErrno);

typedef SOCKET
(WSPAPI *LPWPUMODIFYIFSHANDLE)(
  IN DWORD dwCatalogEntryId,
  IN SOCKET ProposedHandle,
  OUT LPINT lpErrno);

typedef BOOL
(WSPAPI *LPWPUPOSTMESSAGE)(
  IN HWND hWnd,
  IN UINT Msg,
  IN WPARAM wParam,
  IN LPARAM lParam);

typedef int
(WSPAPI *LPWPUQUERYBLOCKINGCALLBACK)(
  IN DWORD dwCatalogEntryId,
  OUT LPBLOCKINGCALLBACK FAR *lplpfnCallback,
  OUT PDWORD_PTR lpdwContext,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWPUQUERYSOCKETHANDLECONTEXT)(
  IN SOCKET s,
  OUT PDWORD_PTR lpContext,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWPUQUEUEAPC)(
  IN LPWSATHREADID lpThreadId,
  IN LPWSAUSERAPC lpfnUserApc,
  IN DWORD_PTR dwContext,
  OUT LPINT lpErrno);

typedef BOOL
(WSPAPI *LPWPURESETEVENT)(
  IN WSAEVENT hEvent,
  OUT LPINT lpErrno);

typedef BOOL
(WSPAPI *LPWPUSETEVENT)(
  IN WSAEVENT hEvent,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWPUOPENCURRENTTHREAD)(
  OUT LPWSATHREADID lpThreadId,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWPUCLOSETHREAD)(
  IN LPWSATHREADID lpThreadId,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWPUCOMPLETEOVERLAPPEDREQUEST)(
  IN SOCKET s,
  IN OUT LPWSAOVERLAPPED lpOverlapped,
  IN DWORD dwError,
  IN DWORD cbTransferred,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSCENUMPROTOCOLS)(
  IN LPINT lpiProtocols OPTIONAL,
  OUT LPWSAPROTOCOL_INFOW lpProtocolBuffer OPTIONAL,
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSCDEINSTALLPROVIDER)(
  IN LPGUID lpProviderId,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSCINSTALLPROVIDER)(
  IN LPGUID lpProviderId,
  IN const WCHAR FAR *lpszProviderDllPath,
  IN const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  IN DWORD dwNumberOfEntries,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSCGETPROVIDERPATH)(
  IN LPGUID lpProviderId,
  OUT WCHAR FAR *lpszProviderDllPath,
  IN OUT LPINT lpProviderDllPathLen,
  OUT LPINT lpErrno);

typedef INT
(WSPAPI *LPWSCINSTALLNAMESPACE)(
  IN LPWSTR lpszIdentifier,
  IN LPWSTR lpszPathName,
  IN DWORD dwNameSpace,
  IN DWORD dwVersion,
  IN LPGUID lpProviderId);

typedef INT
(WSPAPI *LPWSCUNINSTALLNAMESPACE)(
  IN LPGUID lpProviderId);

typedef INT
(WSPAPI *LPWSCENABLENSPROVIDER)(
  IN LPGUID lpProviderId,
  IN BOOL fEnable);

/* Service provider procedure table */
typedef struct _WSPPROC_TABLE {
  LPWSPACCEPT lpWSPAccept;
  LPWSPADDRESSTOSTRING lpWSPAddressToString;
  LPWSPASYNCSELECT lpWSPAsyncSelect;
  LPWSPBIND lpWSPBind;
  LPWSPCANCELBLOCKINGCALL lpWSPCancelBlockingCall;
  LPWSPCLEANUP lpWSPCleanup;
  LPWSPCLOSESOCKET lpWSPCloseSocket;
  LPWSPCONNECT lpWSPConnect;
  LPWSPDUPLICATESOCKET lpWSPDuplicateSocket;
  LPWSPENUMNETWORKEVENTS lpWSPEnumNetworkEvents;
  LPWSPEVENTSELECT lpWSPEventSelect;
  LPWSPGETOVERLAPPEDRESULT lpWSPGetOverlappedResult;
  LPWSPGETPEERNAME lpWSPGetPeerName;
  LPWSPGETSOCKNAME lpWSPGetSockName;
  LPWSPGETSOCKOPT lpWSPGetSockOpt;
  LPWSPGETQOSBYNAME lpWSPGetQOSByName;
  LPWSPIOCTL lpWSPIoctl;
  LPWSPJOINLEAF lpWSPJoinLeaf;
  LPWSPLISTEN lpWSPListen;
  LPWSPRECV lpWSPRecv;
  LPWSPRECVDISCONNECT lpWSPRecvDisconnect;
  LPWSPRECVFROM lpWSPRecvFrom;
  LPWSPSELECT lpWSPSelect;
  LPWSPSEND lpWSPSend;
  LPWSPSENDDISCONNECT lpWSPSendDisconnect;
  LPWSPSENDTO lpWSPSendTo;
  LPWSPSETSOCKOPT lpWSPSetSockOpt;
  LPWSPSHUTDOWN lpWSPShutdown;
  LPWSPSOCKET lpWSPSocket;
  LPWSPSTRINGTOADDRESS lpWSPStringToAddress;
} WSPPROC_TABLE, FAR* LPWSPPROC_TABLE;

typedef INT
(WSAAPI *LPNSPCLEANUP)(
  IN LPGUID lpProviderId);

typedef INT
(WSAAPI *LPNSPLOOKUPSERVICEBEGIN)(
  IN LPGUID lpProviderId,
  IN LPWSAQUERYSETW lpqsRestrictions,
  IN LPWSASERVICECLASSINFOW lpServiceClassInfo,
  IN DWORD dwControlFlags,
  OUT LPHANDLE lphLookup);

typedef INT
(WSAAPI *LPNSPLOOKUPSERVICENEXT)(
  IN HANDLE hLookup,
  IN DWORD dwControlFlags,
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSAQUERYSETW lpqsResults);

#if(_WIN32_WINNT >= 0x0501)
typedef INT
(WSAAPI *LPNSPIOCTL)(
  IN HANDLE hLookup,
  IN DWORD dwControlCode,
  IN LPVOID lpvInBuffer,
  IN DWORD cbInBuffer,
  OUT LPVOID lpvOutBuffer,
  IN DWORD cbOutBuffer,
  OUT LPDWORD lpcbBytesReturned,
  IN LPWSACOMPLETION lpCompletion OPTIONAL,
  IN LPWSATHREADID lpThreadId);
#endif

typedef INT
(WSAAPI *LPNSPLOOKUPSERVICEEND)(
  IN HANDLE hLookup);

typedef INT
(WSAAPI *LPNSPSETSERVICE)(
  IN LPGUID lpProviderId,
  IN LPWSASERVICECLASSINFOW lpServiceClassInfo,
  IN LPWSAQUERYSETW lpqsRegInfo,
  IN WSAESETSERVICEOP essOperation,
  IN DWORD dwControlFlags);

typedef INT
(WSAAPI *LPNSPINSTALLSERVICECLASS)(
  IN LPGUID lpProviderId,
  IN LPWSASERVICECLASSINFOW lpServiceClassInfo);

typedef INT
(WSAAPI *LPNSPREMOVESERVICECLASS)(
  IN LPGUID lpProviderId,
  IN LPGUID lpServiceClassId);

typedef INT
(WSAAPI *LPNSPGETSERVICECLASSINFO)(
  IN LPGUID lpProviderId,
  IN LPDWORD lpdwBufSize,
  IN LPWSASERVICECLASSINFOW lpServiceClassInfo);

typedef INT
(WSAAPI *LPNSPV2STARTUP)(
  IN LPGUID lpProviderId,
  OUT LPVOID *ppvClientSessionArg);

typedef INT
(WSAAPI *LPNSPV2CLEANUP)(
  IN LPGUID lpProviderId,
  IN LPVOID pvClientSessionArg);

typedef INT
(WSAAPI *LPNSPV2LOOKUPSERVICEBEGIN)(
  IN LPGUID lpProviderId,
  IN LPWSAQUERYSET2W lpqsRestrictions,
  IN DWORD dwControlFlags,
  IN LPVOID lpvClientSessionArg,
  OUT LPHANDLE lphLookup);

typedef VOID
(WSAAPI *LPNSPV2LOOKUPSERVICENEXTEX)(
  IN HANDLE hAsyncCall,
  IN HANDLE hLookup,
  IN DWORD dwControlFlags,
  IN LPDWORD lpdwBufferLength,
  OUT LPWSAQUERYSET2W lpqsResults);

typedef INT
(WSAAPI *LPNSPV2LOOKUPSERVICEEND)(
  IN HANDLE hLookup);

typedef VOID
(WSAAPI *LPNSPV2SETSERVICEEX)(
  IN HANDLE hAsyncCall,
  IN LPGUID lpProviderId,
  IN LPWSAQUERYSET2W lpqsRegInfo,
  IN WSAESETSERVICEOP essOperation,
  IN DWORD dwControlFlags,
  IN LPVOID lpvClientSessionArg);

typedef VOID
(WSAAPI *LPNSPV2CLIENTSESSIONRUNDOWN)(
  IN LPGUID lpProviderId,
  IN LPVOID pvClientSessionArg);

/* Service Provider upcall table */
typedef struct _WSPUPCALLTABLE {
  LPWPUCLOSEEVENT lpWPUCloseEvent;
  LPWPUCLOSESOCKETHANDLE lpWPUCloseSocketHandle;
  LPWPUCREATEEVENT lpWPUCreateEvent;
  LPWPUCREATESOCKETHANDLE lpWPUCreateSocketHandle;
  LPWPUFDISSET lpWPUFDIsSet;
  LPWPUGETPROVIDERPATH lpWPUGetProviderPath;
  LPWPUMODIFYIFSHANDLE lpWPUModifyIFSHandle;
  LPWPUPOSTMESSAGE lpWPUPostMessage;
  LPWPUQUERYBLOCKINGCALLBACK lpWPUQueryBlockingCallback;
  LPWPUQUERYSOCKETHANDLECONTEXT lpWPUQuerySocketHandleContext;
  LPWPUQUEUEAPC lpWPUQueueApc;
  LPWPURESETEVENT lpWPUResetEvent;
  LPWPUSETEVENT lpWPUSetEvent;
  LPWPUOPENCURRENTTHREAD lpWPUOpenCurrentThread;
  LPWPUCLOSETHREAD lpWPUCloseThread;
} WSPUPCALLTABLE, FAR* LPWSPUPCALLTABLE;

typedef int
(WSPAPI *LPWSPSTARTUP)(
  IN WORD wVersionRequested,
  IN LPWSPDATA lpWSPData,
  IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
  IN WSPUPCALLTABLE UpcallTable,
  OUT LPWSPPROC_TABLE lpProcTable);

#if (_WIN32_WINNT >= 0x0600)

#define LSP_SYSTEM             0x80000000
#define LSP_INSPECTOR          0x00000001
#define LSP_REDIRECTOR         0x00000002
#define LSP_PROXY              0x00000004
#define LSP_FIREWALL           0x00000008
#define LSP_INBOUND_MODIFY     0x00000010
#define LSP_OUTBOUND_MODIFY    0x00000020
#define LSP_CRYPTO_COMPRESS    0x00000040
#define LSP_LOCAL_CACHE        0x00000080

typedef enum _WSC_PROVIDER_INFO_TYPE {
  ProviderInfoLspCategories,
  ProviderInfoAudit
} WSC_PROVIDER_INFO_TYPE ;

typedef struct _WSC_PROVIDER_AUDIT_INFO {
  DWORD RecordSize;
  PVOID Reserved;
} WSC_PROVIDER_AUDIT_INFO;

#endif /* (_WIN32_WINNT >= 0x0600) */

typedef struct _NSP_ROUTINE {
  DWORD cbSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  LPNSPCLEANUP NSPCleanup;
  LPNSPLOOKUPSERVICEBEGIN NSPLookupServiceBegin;
  LPNSPLOOKUPSERVICENEXT NSPLookupServiceNext;
  LPNSPLOOKUPSERVICEEND NSPLookupServiceEnd;
  LPNSPSETSERVICE NSPSetService;
  LPNSPINSTALLSERVICECLASS NSPInstallServiceClass;
  LPNSPREMOVESERVICECLASS NSPRemoveServiceClass;
  LPNSPGETSERVICECLASSINFO NSPGetServiceClassInfo;
  LPNSPIOCTL NSPIoctl;
} NSP_ROUTINE, *PNSP_ROUTINE, FAR* LPNSP_ROUTINE;

typedef INT
(WSAAPI *LPNSPSTARTUP)(
  IN LPGUID lpProviderId,
  IN OUT LPNSP_ROUTINE lpnspRoutines);

typedef struct _NSPV2_ROUTINE {
  DWORD cbSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  LPNSPV2STARTUP NSPv2Startup;
  LPNSPV2CLEANUP NSPv2Cleanup;
  LPNSPV2LOOKUPSERVICEBEGIN NSPv2LookupServiceBegin;  
  LPNSPV2LOOKUPSERVICENEXTEX NSPv2LookupServiceNextEx;
  LPNSPV2LOOKUPSERVICEEND NSPv2LookupServiceEnd;  
  LPNSPV2SETSERVICEEX NSPv2SetServiceEx;
  LPNSPV2CLIENTSESSIONRUNDOWN NSPv2ClientSessionRundown;
} NSPV2_ROUTINE, *PNSPV2_ROUTINE, *LPNSPV2_ROUTINE;
typedef const NSPV2_ROUTINE *PCNSPV2_ROUTINE, *LPCNSPV2_ROUTINE;

int
WSPAPI
WSPStartup(
  IN WORD wVersionRequested,
  IN LPWSPDATA lpWSPData,
  IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
  IN WSPUPCALLTABLE UpcallTable,
  OUT LPWSPPROC_TABLE lpProcTable);

int
WSPAPI
WSCEnumProtocols(
  IN LPINT lpiProtocols OPTIONAL,
  OUT LPWSAPROTOCOL_INFOW lpProtocolBuffer OPTIONAL,
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPINT lpErrno);

#if (_WIN32_WINNT >= 0x0501)

int
WSPAPI
WPUOpenCurrentThread(
  OUT LPWSATHREADID lpThreadId,
  OUT LPINT lpErrno);

int
WSPAPI
WPUCloseThread(
  IN LPWSATHREADID lpThreadId,
  OUT LPINT lpErrno);

#define WSCEnumNameSpaceProviders WSAEnumNameSpaceProvidersW
#define LPFN_WSCENUMNAMESPACEPROVIDERS LPFN_WSAENUMNAMESPACEPROVIDERSW

int
WSPAPI
WSCUpdateProvider(
  IN LPGUID lpProviderId,
  IN const WCHAR FAR *lpszProviderDllPath,
  IN const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  IN DWORD dwNumberOfEntries,
  OUT LPINT lpErrno);

typedef int
(WSPAPI *LPWSCUPDATEPROVIDER)(
  IN LPGUID lpProviderId,
  IN const WCHAR FAR *lpszProviderDllPath,
  IN const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  IN DWORD dwNumberOfEntries,
  OUT LPINT lpErrno);

#if defined(_WIN64)

int
WSPAPI
WSCEnumProtocols32(
  IN LPINT lpiProtocols OPTIONAL,
  OUT LPWSAPROTOCOL_INFOW lpProtocolBuffer,
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPINT lpErrno);

int
WSPAPI
WSCDeinstallProvider32(
  IN LPGUID lpProviderId,
  OUT LPINT lpErrno);

int
WSPAPI
WSCInstallProvider64_32(
  IN LPGUID lpProviderId,
  IN const WCHAR FAR *lpszProviderDllPath,
  IN const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  IN DWORD dwNumberOfEntries,
  OUT LPINT lpErrno);

int
WSPAPI
WSCGetProviderPath32(
  IN LPGUID lpProviderId,
  OUT WCHAR FAR *lpszProviderDllPath,
  IN OUT LPINT lpProviderDllPathLen,
  OUT LPINT lpErrno);

int
WSPAPI
WSCUpdateProvider32(
  IN LPGUID lpProviderId,
  IN const WCHAR FAR *lpszProviderDllPath,
  IN const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  IN DWORD dwNumberOfEntries,
  OUT LPINT lpErrno);

INT
WSAAPI
WSCEnumNameSpaceProviders32(
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSANAMESPACE_INFOW lpnspBuffer);

INT
WSPAPI
WSCInstallNameSpace32(
  IN LPWSTR lpszIdentifier,
  IN LPWSTR lpszPathName,
  IN DWORD dwNameSpace,
  IN DWORD dwVersion,
  IN LPGUID lpProviderId);

INT
WSPAPI
WSCUnInstallNameSpace32(
  IN LPGUID lpProviderId);

INT
WSPAPI
WSCEnableNSProvider32(
  IN LPGUID lpProviderId,
  IN BOOL fEnable);

#endif /* defined(_WIN64) */

#endif /* defined(_WIN32_WINNT >= 0x0501) */

int
WSPAPI
WSCDeinstallProvider(
  IN LPGUID lpProviderId,
  OUT LPINT lpErrno);

int
WSPAPI
WSCInstallProvider(
  IN LPGUID lpProviderId,
  IN const WCHAR FAR *lpszProviderDllPath,
  IN const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  IN DWORD dwNumberOfEntries,
  OUT LPINT lpErrno);

int
WSPAPI
WSCGetProviderPath(
  IN LPGUID lpProviderId,
  OUT WCHAR FAR *lpszProviderDllPath,
  IN OUT LPINT lpProviderDllPathLen,
  OUT LPINT lpErrno);

#if (_WIN32_WINNT < 0x0600)

int
WSPAPI
WSCInstallQOSTemplate(
  IN const LPGUID Guid,
  IN LPWSABUF QosName,
  IN LPQOS Qos);

typedef int
(WSPAPI *LPWSCINSTALLQOSTEMPLATE)(
  IN const LPGUID Guid,
  IN LPWSABUF QosName,
  IN LPQOS Qos);

int
WSPAPI
WSCRemoveQOSTemplate(
  IN const LPGUID Guid,
  IN LPWSABUF QosName);

typedef int
(WSPAPI *LPWSCREMOVEQOSTEMPLATE)(
  IN const LPGUID Guid,
  IN LPWSABUF QosName);

#endif /* (_WIN32_WINNT < 0x0600) */

#if(_WIN32_WINNT >= 0x0600)

int
WSPAPI
WSCSetProviderInfo(
  IN LPGUID lpProviderId,
  IN WSC_PROVIDER_INFO_TYPE InfoType,
  IN PBYTE Info,
  IN size_t InfoSize,
  IN DWORD Flags,
  OUT LPINT lpErrno);

int
WSPAPI
WSCGetProviderInfo(
  IN LPGUID lpProviderId,
  IN WSC_PROVIDER_INFO_TYPE InfoType,
  OUT PBYTE Info,
  IN OUT *InfoSize,
  IN DWORD Flags,
  OUT LPINT lpErrno);

int
WSPAPI
WSCSetApplicationCategory(
  IN LPCWSTR Path,
  IN DWORD PathLength,
  IN LPCWSTR Extra OPTIONAL,
  IN DWORD ExtraLength,
  IN DWORD PermittedLspCategories,
  OUT DWORD *pPrevPermLspCat OPTIONAL,
  OUT LPINT lpErrno);

int
WSPAPI
WSCGetApplicationCategory(
  IN LPCWSTR Path,
  IN DWORD PathLength,
  IN LPCWSTR Extra OPTIONAL,
  IN DWORD ExtraLength,
  OUT DWORD *pPermittedLspCategories,
  OUT LPINT lpErrno);

#define WSCEnumNameSpaceProvidersEx WSAEnumNameSpaceProvidersExW
#define LPFN_WSCENUMNAMESPACEPROVIDERSEX LPFN_WSAENUMNAMESPACEPROVIDERSEXW

INT
WSPAPI
WSCInstallNameSpaceEx(
  IN LPWSTR lpszIdentifier,
  IN LPWSTR lpszPathName,
  IN DWORD dwNameSpace,
  IN DWORD dwVersion,
  IN LPGUID lpProviderId,
  IN LPBLOB lpProviderSpecific);

INT
WSAAPI
WSAAdvertiseProvider(
  IN const GUID *puuidProviderId,
  IN const LPCNSPV2_ROUTINE pNSPv2Routine);

INT
WSAAPI
WSAUnadvertiseProvider(
  IN const GUID *puuidProviderId);

INT
WSAAPI
WSAProviderCompleteAsyncCall(
  IN HANDLE hAsyncCall,
  IN INT iRetCode);

#if defined(_WIN64)

int
WSPAPI
WSCSetProviderInfo32(
  IN LPGUID lpProviderId,
  IN WSC_PROVIDER_INFO_TYPE InfoType,
  IN PBYTE Info,
  IN size_t InfoSize,
  IN DWORD Flags,
  OUT LPINT lpErrno);

int
WSPAPI
WSCGetProviderInfo32(
  IN LPGUID lpProviderId,
  IN WSC_PROVIDER_INFO_TYPE InfoType,
  OUT PBYTE Info,
  IN OUT size_t *InfoSize,
  IN DWORD Flags,
  OUT LPINT lpErrno);

INT
WSAAPI
WSCEnumNameSpaceProvidersEx32(
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSANAMESPACE_INFOEXW lpnspBuffer);

INT
WSPAPI
WSCInstallNameSpaceEx32(
  IN LPWSTR lpszIdentifier,
  IN LPWSTR lpszPathName,
  IN DWORD dwNameSpace,
  IN DWORD dwVersion,
  IN LPGUID lpProviderId,
  IN LPBLOB lpProviderSpecific);

#endif /* (_WIN64) */

#if defined(_WIN64)
int
WSPAPI
WSCInstallProviderAndChains64_32(
#else
int
WSPAPI
WSCInstallProviderAndChains(
#endif
  IN LPGUID lpProviderId,
  IN const LPWSTR lpszProviderDllPath,
#if defined(_WIN64)
  IN const LPWSTR lpszProviderDllPath32,
#endif
  IN const LPWSTR lpszLspName,
  IN DWORD dwServiceFlags,
  IN OUT LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  IN DWORD dwNumberOfEntries,
  OUT LPDWORD lpdwCatalogEntryId OPTIONAL,
  OUT LPINT lpErrno);

#endif /* (_WIN32_WINNT >= 0x0600) */

BOOL
WSPAPI
WPUCloseEvent(
  IN WSAEVENT hEvent,
  OUT LPINT lpErrno);

int
WSPAPI
WPUCloseSocketHandle(
  IN SOCKET s,
  OUT LPINT lpErrno);

WSAEVENT
WSPAPI
WPUCreateEvent(
  OUT LPINT lpErrno);

SOCKET
WSPAPI
WPUCreateSocketHandle(
  IN DWORD dwCatalogEntryId,
  IN DWORD_PTR dwContext,
  OUT LPINT lpErrno);

int
WSPAPI
WPUFDIsSet(
  IN SOCKET s,
  IN fd_set FAR *fdset);

int
WSPAPI
WPUGetProviderPath(
  IN LPGUID lpProviderId,
  OUT WCHAR FAR *lpszProviderDllPath,
  IN OUT LPINT lpProviderDllPathLen,
  OUT LPINT lpErrno);

SOCKET
WSPAPI
WPUModifyIFSHandle(
  IN DWORD dwCatalogEntryId,
  IN SOCKET ProposedHandle,
  OUT LPINT lpErrno);

BOOL
WSPAPI
WPUPostMessage(
  IN HWND hWnd,
  IN UINT Msg,
  IN WPARAM wParam,
  IN LPARAM lParam);

int
WSPAPI
WPUQueryBlockingCallback(
  IN DWORD dwCatalogEntryId,
  OUT LPBLOCKINGCALLBACK FAR *lplpfnCallback,
  OUT PDWORD_PTR lpdwContext,
  OUT LPINT lpErrno);

int
WSPAPI
WPUQuerySocketHandleContext(
  IN SOCKET s,
  OUT PDWORD_PTR lpContext,
  OUT LPINT lpErrno);

int
WSPAPI
WPUQueueApc(
  IN LPWSATHREADID lpThreadId,
  IN LPWSAUSERAPC lpfnUserApc,
  IN DWORD_PTR dwContext,
  OUT LPINT lpErrno);

BOOL
WSPAPI
WPUResetEvent(
  IN WSAEVENT hEvent,
  OUT LPINT lpErrno);

BOOL
WSPAPI
WPUSetEvent(
  IN WSAEVENT hEvent,
  OUT LPINT lpErrno);

int
WSPAPI
WPUCompleteOverlappedRequest(
  IN SOCKET s,
  IN OUT LPWSAOVERLAPPED lpOverlapped,
  IN DWORD dwError,
  IN DWORD cbTransferred,
  OUT LPINT lpErrno);

INT
WSPAPI
WSCInstallNameSpace(
  IN LPWSTR lpszIdentifier,
  IN LPWSTR lpszPathName,
  IN DWORD dwNameSpace,
  IN DWORD dwVersion,
  IN LPGUID lpProviderId);

INT
WSPAPI
WSCUnInstallNameSpace(
  IN LPGUID lpProviderId);

INT
WSPAPI
WSCEnableNSProvider(
  IN LPGUID lpProviderId,
  IN BOOL fEnable);

INT
WSAAPI
NSPStartup(
  IN LPGUID lpProviderId,
  IN OUT LPNSP_ROUTINE lpnspRoutines);

#if !defined(_WIN64)
#include <poppack.h>
#endif

#ifdef __cplusplus
}
#endif
