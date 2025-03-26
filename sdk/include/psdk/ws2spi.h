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

typedef
_Must_inspect_result_
SOCKET
(WSPAPI *LPWSPACCEPT)(
  _In_ SOCKET s,
  _Out_writes_bytes_to_opt_(*addrlen, *addrlen) struct sockaddr FAR *addr,
  _Inout_opt_ LPINT addrlen,
  _In_opt_ LPCONDITIONPROC lpfnCondition,
  _In_opt_ DWORD_PTR dwCallbackData,
  _Out_ LPINT lpErrno);

typedef VOID
(CALLBACK FAR *LPWSAUSERAPC)(
  DWORD_PTR dwContext);

typedef INT
(WSPAPI *LPWSPADDRESSTOSTRING)(
  _In_reads_bytes_(dwAddressLength) LPSOCKADDR lpsaAddress,
  _In_ DWORD dwAddressLength,
  _In_opt_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
  _Out_writes_to_(*lpdwAddressStringLength, *lpdwAddressStringLength) LPWSTR lpszAddressString,
  _Inout_ LPDWORD lpdwAddressStringLength,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPASYNCSELECT)(
  _In_ SOCKET s,
  _In_ HWND hWnd,
  _In_ unsigned int wMsg,
  _In_ long lEvent,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPBIND)(
  _In_ SOCKET s,
  _In_reads_bytes_(namelen) const struct sockaddr FAR *name,
  _In_ int namelen,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPCANCELBLOCKINGCALL)(
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPCLEANUP)(
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPCLOSESOCKET)(
  _In_ SOCKET s,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPCONNECT)(
  _In_ SOCKET s,
  _In_reads_bytes_(namelen) const struct sockaddr FAR *name,
  _In_ int namelen,
  _In_opt_ LPWSABUF lpCallerData,
  _Out_opt_ LPWSABUF lpCalleeData,
  _In_opt_ LPQOS lpSQOS,
  _In_opt_ LPQOS lpGQOS,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPDUPLICATESOCKET)(
  _In_ SOCKET s,
  _In_ DWORD dwProcessId,
  _Out_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPENUMNETWORKEVENTS)(
  _In_ SOCKET s,
  _In_ WSAEVENT hEventObject,
  _Out_ LPWSANETWORKEVENTS lpNetworkEvents,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPEVENTSELECT)(
  _In_ SOCKET s,
  _In_opt_ WSAEVENT hEventObject,
  _In_ long lNetworkEvents,
  _Out_ LPINT lpErrno);

typedef BOOL
(WSPAPI *LPWSPGETOVERLAPPEDRESULT)(
  _In_ SOCKET s,
  _In_ LPWSAOVERLAPPED lpOverlapped,
  _Out_ LPDWORD lpcbTransfer,
  _In_ BOOL fWait,
  _Out_ LPDWORD lpdwFlags,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPGETPEERNAME)(
  _In_ SOCKET s,
  _Out_writes_bytes_to_(*namelen, *namelen) struct sockaddr FAR *name,
  _Inout_ LPINT namelen,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPGETSOCKNAME)(
  _In_ SOCKET s,
  _Out_writes_bytes_to_(*namelen, *namelen) struct sockaddr FAR *name,
  _Inout_ LPINT namelen,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPGETSOCKOPT)(
  _In_ SOCKET s,
  _In_ int level,
  _In_ int optname,
  _Out_writes_bytes_(*optlen) char FAR *optval,
  _Inout_ LPINT optlen,
  _Out_ LPINT lpErrno);

typedef BOOL
(WSPAPI *LPWSPGETQOSBYNAME)(
  _In_ SOCKET s,
  _In_ LPWSABUF lpQOSName,
  _Out_ LPQOS lpQOS,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPIOCTL)(
  _In_ SOCKET s,
  _In_ DWORD dwIoControlCode,
  _In_reads_bytes_opt_(cbInBuffer) LPVOID lpvInBuffer,
  _In_ DWORD cbInBuffer,
  _Out_writes_bytes_to_opt_(cbOutBuffer, *lpcbBytesReturned) LPVOID lpvOutBuffer,
  _In_ DWORD cbOutBuffer,
  _Out_ LPDWORD lpcbBytesReturned,
  _Inout_opt_ LPWSAOVERLAPPED lpOverlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
  _In_opt_ LPWSATHREADID lpThreadId,
  _Out_ LPINT lpErrno);

typedef SOCKET
(WSPAPI *LPWSPJOINLEAF)(
  _In_ SOCKET s,
  _In_reads_bytes_(namelen) const struct sockaddr FAR *name,
  _In_ int namelen,
  _In_opt_ LPWSABUF lpCallerData,
  _Out_opt_ LPWSABUF lpCalleeData,
  _In_opt_ LPQOS lpSQOS,
  _In_opt_ LPQOS lpGQOS,
  _In_ DWORD dwFlags,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPLISTEN)(
  _In_ SOCKET s,
  _In_ int backlog,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPRECV)(
  _In_ SOCKET s,
  _In_reads_(dwBufferCount) LPWSABUF lpBuffers,
  _In_ DWORD dwBufferCount,
  _Out_opt_ LPDWORD lpNumberOfBytesRecvd,
  _Inout_ LPDWORD lpFlags,
  _Inout_opt_ LPWSAOVERLAPPED lpOverlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
  _In_opt_ LPWSATHREADID lpThreadId,
  _In_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPRECVDISCONNECT)(
  _In_ SOCKET s,
  _In_opt_ LPWSABUF lpInboundDisconnectData,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPRECVFROM)(
  _In_ SOCKET s,
  _In_reads_(dwBufferCount) LPWSABUF lpBuffers,
  _In_ DWORD dwBufferCount,
  _Out_opt_ LPDWORD lpNumberOfBytesRecvd,
  _Inout_ LPDWORD lpFlags,
  _Out_writes_bytes_to_opt_(*lpFromlen, *lpFromlen) struct sockaddr FAR *lpFrom,
  _Inout_opt_ LPINT lpFromlen,
  _Inout_opt_ LPWSAOVERLAPPED lpOverlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
  _In_opt_ LPWSATHREADID lpThreadId,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPSELECT)(
  _In_ int nfds,
  _Inout_opt_ fd_set FAR *readfds,
  _Inout_opt_ fd_set FAR *writefds,
  _Inout_opt_ fd_set FAR *exceptfds,
  _In_opt_ const struct timeval FAR *timeout,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPSEND)(
  _In_ SOCKET s,
  _In_reads_(dwBufferCount) LPWSABUF lpBuffers,
  _In_ DWORD dwBufferCount,
  _Out_opt_ LPDWORD lpNumberOfBytesSent,
  _In_ DWORD dwFlags,
  _Inout_opt_ LPWSAOVERLAPPED lpOverlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
  _In_opt_ LPWSATHREADID lpThreadId,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPSENDDISCONNECT)(
  _In_ SOCKET s,
  _In_opt_ LPWSABUF lpOutboundDisconnectData,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPSENDTO)(
  _In_ SOCKET s,
  _In_reads_(dwBufferCount) LPWSABUF lpBuffers,
  _In_ DWORD dwBufferCount,
  _Out_opt_ LPDWORD lpNumberOfBytesSent,
  _In_ DWORD dwFlags,
  _In_reads_bytes_opt_(iTolen) const struct sockaddr FAR *lpTo,
  _In_ int iTolen,
  _Inout_opt_ LPWSAOVERLAPPED lpOverlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
  _In_opt_ LPWSATHREADID lpThreadId,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPSETSOCKOPT)(
  _In_ SOCKET s,
  _In_ int level,
  _In_ int optname,
  _In_reads_bytes_opt_(optlen) const char FAR *optval,
  _In_ int optlen,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSPSHUTDOWN)(
  _In_ SOCKET s,
  _In_ int how,
  _Out_ LPINT lpErrno);

typedef
_Must_inspect_result_
SOCKET
(WSPAPI *LPWSPSOCKET)(
  _In_ int af,
  _In_ int type,
  _In_ int protocol,
  _In_opt_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
  _In_ GROUP g,
  _In_ DWORD dwFlags,
  _Out_ LPINT lpErrno);

typedef INT
(WSPAPI *LPWSPSTRINGTOADDRESS)(
  _In_ LPWSTR AddressString,
  _In_ INT AddressFamily,
  _In_opt_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
  _Out_writes_bytes_to_(*lpAddressLength, *lpAddressLength) LPSOCKADDR lpAddress,
  _Inout_ LPINT lpAddressLength,
  _Out_ LPINT lpErrno);

typedef BOOL
(WSPAPI *LPWPUCLOSEEVENT)(
  _In_ WSAEVENT hEvent,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWPUCLOSESOCKETHANDLE)(
  _In_ SOCKET s,
  _Out_ LPINT lpErrno);

typedef WSAEVENT
(WSPAPI *LPWPUCREATEEVENT)(
  _Out_ LPINT lpErrno);

typedef
_Must_inspect_result_
SOCKET
(WSPAPI *LPWPUCREATESOCKETHANDLE)(
  _In_ DWORD dwCatalogEntryId,
  _In_ DWORD_PTR dwContext,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWPUFDISSET)(
  _In_ SOCKET s,
  _In_ fd_set FAR *fdset);

typedef int
(WSPAPI *LPWPUGETPROVIDERPATH)(
  _In_ LPGUID lpProviderId,
  _Out_writes_(*lpProviderDllPathLen) WCHAR FAR *lpszProviderDllPath,
  _Inout_ LPINT lpProviderDllPathLen,
  _Out_ LPINT lpErrno);

typedef SOCKET
(WSPAPI *LPWPUMODIFYIFSHANDLE)(
  _In_ DWORD dwCatalogEntryId,
  _In_ SOCKET ProposedHandle,
  _Out_ LPINT lpErrno);

typedef BOOL
(WSPAPI *LPWPUPOSTMESSAGE)(
  _In_ HWND hWnd,
  _In_ UINT Msg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam);

typedef int
(WSPAPI *LPWPUQUERYBLOCKINGCALLBACK)(
  _In_ DWORD dwCatalogEntryId,
  _Out_ LPBLOCKINGCALLBACK FAR *lplpfnCallback,
  _Out_ PDWORD_PTR lpdwContext,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWPUQUERYSOCKETHANDLECONTEXT)(
  _In_ SOCKET s,
  _Out_ PDWORD_PTR lpContext,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWPUQUEUEAPC)(
  _In_ LPWSATHREADID lpThreadId,
  _In_ LPWSAUSERAPC lpfnUserApc,
  _In_ DWORD_PTR dwContext,
  _Out_ LPINT lpErrno);

typedef BOOL
(WSPAPI *LPWPURESETEVENT)(
  _In_ WSAEVENT hEvent,
  _Out_ LPINT lpErrno);

typedef BOOL
(WSPAPI *LPWPUSETEVENT)(
  _In_ WSAEVENT hEvent,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWPUOPENCURRENTTHREAD)(
  _Out_ LPWSATHREADID lpThreadId,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWPUCLOSETHREAD)(
  _In_ LPWSATHREADID lpThreadId,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWPUCOMPLETEOVERLAPPEDREQUEST)(
  _In_ SOCKET s,
  _Inout_ LPWSAOVERLAPPED lpOverlapped,
  _In_ DWORD dwError,
  _In_ DWORD cbTransferred,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSCENUMPROTOCOLS)(
  _In_opt_ LPINT lpiProtocols,
  _Out_writes_bytes_to_opt_(*lpdwBufferLength, *lpdwBufferlength) LPWSAPROTOCOL_INFOW lpProtocolBuffer,
  _Inout_ LPDWORD lpdwBufferLength,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSCDEINSTALLPROVIDER)(
  _In_ LPGUID lpProviderId,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSCINSTALLPROVIDER)(
  _In_ LPGUID lpProviderId,
  _In_z_ const WCHAR FAR *lpszProviderDllPath,
  _In_reads_(dwNumberOfEntries) const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  _In_ DWORD dwNumberOfEntries,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSCGETPROVIDERPATH)(
  _In_ LPGUID lpProviderId,
  _Out_writes_to_(*lpProviderDllPathLen, *lpProviderDllPathLen) WCHAR FAR *lpszProviderDllPath,
  _Inout_ LPINT lpProviderDllPathLen,
  _Out_ LPINT lpErrno);

typedef INT
(WSPAPI *LPWSCINSTALLNAMESPACE)(
  _In_ LPWSTR lpszIdentifier,
  _In_ LPWSTR lpszPathName,
  _In_ DWORD dwNameSpace,
  _In_ DWORD dwVersion,
  _In_ LPGUID lpProviderId);

typedef INT
(WSPAPI *LPWSCUNINSTALLNAMESPACE)(
  _In_ LPGUID lpProviderId);

typedef INT
(WSPAPI *LPWSCENABLENSPROVIDER)(
  _In_ LPGUID lpProviderId,
  _In_ BOOL fEnable);

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
  _In_ LPGUID lpProviderId);

typedef INT
(WSAAPI *LPNSPLOOKUPSERVICEBEGIN)(
  _In_ LPGUID lpProviderId,
  _In_ LPWSAQUERYSETW lpqsRestrictions,
  _In_ LPWSASERVICECLASSINFOW lpServiceClassInfo,
  _In_ DWORD dwControlFlags,
  _Out_ LPHANDLE lphLookup);

typedef INT
(WSAAPI *LPNSPLOOKUPSERVICENEXT)(
  _In_ HANDLE hLookup,
  _In_ DWORD dwControlFlags,
  _Inout_ LPDWORD lpdwBufferLength,
  _Out_writes_bytes_to_(*lpdwBufferLength, *lpdwBufferLength) LPWSAQUERYSETW lpqsResults);

#if(_WIN32_WINNT >= 0x0501)
typedef INT
(WSAAPI *LPNSPIOCTL)(
  _In_ HANDLE hLookup,
  _In_ DWORD dwControlCode,
  _In_reads_bytes_(cbInBuffer) LPVOID lpvInBuffer,
  _In_ DWORD cbInBuffer,
  _Out_writes_bytes_to_(cbOutBuffer, *lpcbBytesReturned) LPVOID lpvOutBuffer,
  _In_ DWORD cbOutBuffer,
  _Out_ LPDWORD lpcbBytesReturned,
  _In_opt_ LPWSACOMPLETION lpCompletion,
  _In_ LPWSATHREADID lpThreadId);
#endif

typedef INT
(WSAAPI *LPNSPLOOKUPSERVICEEND)(
  _In_ HANDLE hLookup);

typedef INT
(WSAAPI *LPNSPSETSERVICE)(
  _In_ LPGUID lpProviderId,
  _In_ LPWSASERVICECLASSINFOW lpServiceClassInfo,
  _In_ LPWSAQUERYSETW lpqsRegInfo,
  _In_ WSAESETSERVICEOP essOperation,
  _In_ DWORD dwControlFlags);

typedef INT
(WSAAPI *LPNSPINSTALLSERVICECLASS)(
  _In_ LPGUID lpProviderId,
  _In_ LPWSASERVICECLASSINFOW lpServiceClassInfo);

typedef INT
(WSAAPI *LPNSPREMOVESERVICECLASS)(
  _In_ LPGUID lpProviderId,
  _In_ LPGUID lpServiceClassId);

typedef INT
(WSAAPI *LPNSPGETSERVICECLASSINFO)(
  _In_ LPGUID lpProviderId,
  _In_ LPDWORD lpdwBufSize,
  _In_ LPWSASERVICECLASSINFOW lpServiceClassInfo);

typedef
_Must_inspect_result_
INT
(WSAAPI *LPNSPV2STARTUP)(
  _In_ LPGUID lpProviderId,
  _Outptr_ LPVOID *ppvClientSessionArg);

typedef INT
(WSAAPI *LPNSPV2CLEANUP)(
  _In_ LPGUID lpProviderId,
  _In_ LPVOID pvClientSessionArg);

typedef INT
(WSAAPI *LPNSPV2LOOKUPSERVICEBEGIN)(
  _In_ LPGUID lpProviderId,
  _In_ LPWSAQUERYSET2W lpqsRestrictions,
  _In_ DWORD dwControlFlags,
  _In_ LPVOID lpvClientSessionArg,
  _Out_ LPHANDLE lphLookup);

typedef VOID
(WSAAPI *LPNSPV2LOOKUPSERVICENEXTEX)(
  _In_ HANDLE hAsyncCall,
  _In_ HANDLE hLookup,
  _In_ DWORD dwControlFlags,
  _In_ LPDWORD lpdwBufferLength,
  _Out_ LPWSAQUERYSET2W lpqsResults);

typedef INT
(WSAAPI *LPNSPV2LOOKUPSERVICEEND)(
  _In_ HANDLE hLookup);

typedef VOID
(WSAAPI *LPNSPV2SETSERVICEEX)(
  _In_ HANDLE hAsyncCall,
  _In_ LPGUID lpProviderId,
  _In_ LPWSAQUERYSET2W lpqsRegInfo,
  _In_ WSAESETSERVICEOP essOperation,
  _In_ DWORD dwControlFlags,
  _In_ LPVOID lpvClientSessionArg);

typedef VOID
(WSAAPI *LPNSPV2CLIENTSESSIONRUNDOWN)(
  _In_ LPGUID lpProviderId,
  _In_ LPVOID pvClientSessionArg);

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

typedef
_Must_inspect_result_
int
(WSPAPI *LPWSPSTARTUP)(
  _In_ WORD wVersionRequested,
  _In_ LPWSPDATA lpWSPData,
  _In_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
  _In_ WSPUPCALLTABLE UpcallTable,
  _Out_ LPWSPPROC_TABLE lpProcTable);

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

typedef
_Must_inspect_result_
INT
(WSAAPI *LPNSPSTARTUP)(
  _In_ LPGUID lpProviderId,
  _Inout_ LPNSP_ROUTINE lpnspRoutines);

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

_Must_inspect_result_
int
WSPAPI
WSPStartup(
  _In_ WORD wVersionRequested,
  _In_ LPWSPDATA lpWSPData,
  _In_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
  _In_ WSPUPCALLTABLE UpcallTable,
  _Out_ LPWSPPROC_TABLE lpProcTable);

int
WSPAPI
WSCEnumProtocols(
  _In_opt_ LPINT lpiProtocols,
  _Out_writes_bytes_to_opt_(*lpdwBufferLength, *lpdwBufferLength) LPWSAPROTOCOL_INFOW lpProtocolBuffer,
  _Inout_ LPDWORD lpdwBufferLength,
  _Out_ LPINT lpErrno);

#if (_WIN32_WINNT >= 0x0501)

int
WSPAPI
WPUOpenCurrentThread(
  _Out_ LPWSATHREADID lpThreadId,
  _Out_ LPINT lpErrno);

int
WSPAPI
WPUCloseThread(
  _In_ LPWSATHREADID lpThreadId,
  _Out_ LPINT lpErrno);

#define WSCEnumNameSpaceProviders WSAEnumNameSpaceProvidersW
#define LPFN_WSCENUMNAMESPACEPROVIDERS LPFN_WSAENUMNAMESPACEPROVIDERSW

int
WSPAPI
WSCUpdateProvider(
  _In_ LPGUID lpProviderId,
  _In_z_ const WCHAR FAR *lpszProviderDllPath,
  _In_reads_(dwNumberOfEntries) const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  _In_ DWORD dwNumberOfEntries,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPWSCUPDATEPROVIDER)(
  _In_ LPGUID lpProviderId,
  _In_z_ const WCHAR FAR *lpszProviderDllPath,
  _In_reads_(dwNumberOfEntries) const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  _In_ DWORD dwNumberOfEntries,
  _Out_ LPINT lpErrno);

#if defined(_WIN64)

int
WSPAPI
WSCEnumProtocols32(
  _In_opt_ LPINT lpiProtocols,
  _Out_writes_bytes_(*lpdwBufferLength) LPWSAPROTOCOL_INFOW lpProtocolBuffer,
  _Inout_ LPDWORD lpdwBufferLength,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSCDeinstallProvider32(
  _In_ LPGUID lpProviderId,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSCInstallProvider64_32(
  _In_ LPGUID lpProviderId,
  _In_z_ const WCHAR FAR *lpszProviderDllPath,
  _In_reads_(dwNumberOfEntries) const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  _In_ DWORD dwNumberOfEntries,
  _Out_ LPINT lpErrno);

_Success_(return == 0)
int
WSPAPI
WSCGetProviderPath32(
  _In_ LPGUID lpProviderId,
  _Out_writes_to_(*lpProviderDllPathLen, *lpProviderDllPathLen) WCHAR FAR *lpszProviderDllPath,
  _Inout_ LPINT lpProviderDllPathLen,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSCUpdateProvider32(
  _In_ LPGUID lpProviderId,
  _In_z_ const WCHAR FAR *lpszProviderDllPath,
  _In_reads_(dwNumberOfEntries) const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  _In_ DWORD dwNumberOfEntries,
  _Out_ LPINT lpErrno);

INT
WSAAPI
WSCEnumNameSpaceProviders32(
  _Inout_ LPDWORD lpdwBufferLength,
  _Out_writes_bytes_(*lpdwBufferLength) LPWSANAMESPACE_INFOW lpnspBuffer);

INT
WSPAPI
WSCInstallNameSpace32(
  _In_ LPWSTR lpszIdentifier,
  _In_ LPWSTR lpszPathName,
  _In_ DWORD dwNameSpace,
  _In_ DWORD dwVersion,
  _In_ LPGUID lpProviderId);

INT
WSPAPI
WSCUnInstallNameSpace32(
  _In_ LPGUID lpProviderId);

INT
WSPAPI
WSCEnableNSProvider32(
  _In_ LPGUID lpProviderId,
  _In_ BOOL fEnable);

#endif /* defined(_WIN64) */

#endif /* defined(_WIN32_WINNT >= 0x0501) */

int
WSPAPI
WSCDeinstallProvider(
  _In_ LPGUID lpProviderId,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSCInstallProvider(
  _In_ LPGUID lpProviderId,
  _In_z_ const WCHAR FAR *lpszProviderDllPath,
  _In_reads_(dwNumberOfEntries) const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  _In_ DWORD dwNumberOfEntries,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSCGetProviderPath(
  _In_ LPGUID lpProviderId,
  _Out_writes_to_(*lpProviderDllPathLen, *lpProviderDllPathLen) WCHAR FAR *lpszProviderDllPath,
  _Inout_ LPINT lpProviderDllPathLen,
  _Out_ LPINT lpErrno);

#if (_WIN32_WINNT < 0x0600)

int
WSPAPI
WSCInstallQOSTemplate(
  _In_ const LPGUID Guid,
  _In_ LPWSABUF QosName,
  _In_ LPQOS Qos);

typedef int
(WSPAPI *LPWSCINSTALLQOSTEMPLATE)(
  _In_ const LPGUID Guid,
  _In_ LPWSABUF QosName,
  _In_ LPQOS Qos);

int
WSPAPI
WSCRemoveQOSTemplate(
  _In_ const LPGUID Guid,
  _In_ LPWSABUF QosName);

typedef int
(WSPAPI *LPWSCREMOVEQOSTEMPLATE)(
  _In_ const LPGUID Guid,
  _In_ LPWSABUF QosName);

#endif /* (_WIN32_WINNT < 0x0600) */

#if(_WIN32_WINNT >= 0x0600)

int
WSPAPI
WSCSetProviderInfo(
  _In_ LPGUID lpProviderId,
  _In_ WSC_PROVIDER_INFO_TYPE InfoType,
  _In_reads_bytes_(InfoSize) PBYTE Info,
  _In_ size_t InfoSize,
  _In_ DWORD Flags,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSCGetProviderInfo(
  _In_ LPGUID lpProviderId,
  _In_ WSC_PROVIDER_INFO_TYPE InfoType,
  _Out_writes_bytes_to_(*InfoSize, *InfoSize) PBYTE Info,
  _Inout_ size_t *InfoSize,
  _In_ DWORD Flags,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSCSetApplicationCategory(
  _In_reads_(PathLength) LPCWSTR Path,
  _In_ DWORD PathLength,
  _In_reads_opt_(ExtraLength) LPCWSTR Extra,
  _In_ DWORD ExtraLength,
  _In_ DWORD PermittedLspCategories,
  _Out_opt_ DWORD *pPrevPermLspCat,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSCGetApplicationCategory(
  _In_reads_(PathLength) LPCWSTR Path,
  _In_ DWORD PathLength,
  _In_reads_opt_(ExtraLength) LPCWSTR Extra,
  _In_ DWORD ExtraLength,
  _Out_ DWORD *pPermittedLspCategories,
  _Out_ LPINT lpErrno);

#define WSCEnumNameSpaceProvidersEx WSAEnumNameSpaceProvidersExW
#define LPFN_WSCENUMNAMESPACEPROVIDERSEX LPFN_WSAENUMNAMESPACEPROVIDERSEXW

INT
WSPAPI
WSCInstallNameSpaceEx(
  _In_ LPWSTR lpszIdentifier,
  _In_ LPWSTR lpszPathName,
  _In_ DWORD dwNameSpace,
  _In_ DWORD dwVersion,
  _In_ LPGUID lpProviderId,
  _In_ LPBLOB lpProviderSpecific);

INT
WSAAPI
WSAAdvertiseProvider(
  _In_ const GUID *puuidProviderId,
  _In_ const LPCNSPV2_ROUTINE pNSPv2Routine);

INT
WSAAPI
WSAUnadvertiseProvider(
  _In_ const GUID *puuidProviderId);

INT
WSAAPI
WSAProviderCompleteAsyncCall(
  _In_ HANDLE hAsyncCall,
  _In_ INT iRetCode);

#if defined(_WIN64)

int
WSPAPI
WSCSetProviderInfo32(
  _In_ LPGUID lpProviderId,
  _In_ WSC_PROVIDER_INFO_TYPE InfoType,
  _In_reads_bytes_(InfoSize) PBYTE Info,
  _In_ size_t InfoSize,
  _In_ DWORD Flags,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSCGetProviderInfo32(
  _In_ LPGUID lpProviderId,
  _In_ WSC_PROVIDER_INFO_TYPE InfoType,
  _Out_writes_bytes_to_(*InfoSize, *InfoSize) PBYTE Info,
  _Inout_ size_t *InfoSize,
  _In_ DWORD Flags,
  _Out_ LPINT lpErrno);

INT
WSAAPI
WSCEnumNameSpaceProvidersEx32(
  _Inout_ LPDWORD lpdwBufferLength,
  _Out_writes_bytes_(*lpdwBufferLength) LPWSANAMESPACE_INFOEXW lpnspBuffer);

INT
WSPAPI
WSCInstallNameSpaceEx32(
  _In_ LPWSTR lpszIdentifier,
  _In_ LPWSTR lpszPathName,
  _In_ DWORD dwNameSpace,
  _In_ DWORD dwVersion,
  _In_ LPGUID lpProviderId,
  _In_ LPBLOB lpProviderSpecific);

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
  _In_ LPGUID lpProviderId,
  _In_ const LPWSTR lpszProviderDllPath,
#if defined(_WIN64)
  _In_ const LPWSTR lpszProviderDllPath32,
#endif
  _In_ const LPWSTR lpszLspName,
  _In_ DWORD dwServiceFlags,
  _Inout_updates_(dwNumberOfEntries) LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  _In_ DWORD dwNumberOfEntries,
  _Out_opt_ LPDWORD lpdwCatalogEntryId,
  _Out_ LPINT lpErrno);

#endif /* (_WIN32_WINNT >= 0x0600) */

BOOL
WSPAPI
WPUCloseEvent(
  _In_ WSAEVENT hEvent,
  _Out_ LPINT lpErrno);

int
WSPAPI
WPUCloseSocketHandle(
  _In_ SOCKET s,
  _Out_ LPINT lpErrno);

WSAEVENT
WSPAPI
WPUCreateEvent(
  _Out_ LPINT lpErrno);

SOCKET
WSPAPI
WPUCreateSocketHandle(
  _In_ DWORD dwCatalogEntryId,
  _In_ DWORD_PTR dwContext,
  _Out_ LPINT lpErrno);

int
WSPAPI
WPUFDIsSet(
  _In_ SOCKET s,
  _In_ fd_set FAR *fdset);

int
WSPAPI
WPUGetProviderPath(
  _In_ LPGUID lpProviderId,
  _Out_writes_(*lpProviderDllPathLen) WCHAR FAR *lpszProviderDllPath,
  _Inout_ LPINT lpProviderDllPathLen,
  _Out_ LPINT lpErrno);

SOCKET
WSPAPI
WPUModifyIFSHandle(
  _In_ DWORD dwCatalogEntryId,
  _In_ SOCKET ProposedHandle,
  _Out_ LPINT lpErrno);

BOOL
WSPAPI
WPUPostMessage(
  _In_ HWND hWnd,
  _In_ UINT Msg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam);

int
WSPAPI
WPUQueryBlockingCallback(
  _In_ DWORD dwCatalogEntryId,
  _Out_ LPBLOCKINGCALLBACK FAR *lplpfnCallback,
  _Out_ PDWORD_PTR lpdwContext,
  _Out_ LPINT lpErrno);

int
WSPAPI
WPUQuerySocketHandleContext(
  _In_ SOCKET s,
  _Out_ PDWORD_PTR lpContext,
  _Out_ LPINT lpErrno);

int
WSPAPI
WPUQueueApc(
  _In_ LPWSATHREADID lpThreadId,
  _In_ LPWSAUSERAPC lpfnUserApc,
  _In_ DWORD_PTR dwContext,
  _Out_ LPINT lpErrno);

BOOL
WSPAPI
WPUResetEvent(
  _In_ WSAEVENT hEvent,
  _Out_ LPINT lpErrno);

BOOL
WSPAPI
WPUSetEvent(
  _In_ WSAEVENT hEvent,
  _Out_ LPINT lpErrno);

int
WSPAPI
WPUCompleteOverlappedRequest(
  _In_ SOCKET s,
  _Inout_ LPWSAOVERLAPPED lpOverlapped,
  _In_ DWORD dwError,
  _In_ DWORD cbTransferred,
  _Out_ LPINT lpErrno);

INT
WSPAPI
WSCInstallNameSpace(
  _In_ LPWSTR lpszIdentifier,
  _In_ LPWSTR lpszPathName,
  _In_ DWORD dwNameSpace,
  _In_ DWORD dwVersion,
  _In_ LPGUID lpProviderId);

INT
WSPAPI
WSCUnInstallNameSpace(
  _In_ LPGUID lpProviderId);

INT
WSPAPI
WSCEnableNSProvider(
  _In_ LPGUID lpProviderId,
  _In_ BOOL fEnable);

_Must_inspect_result_
INT
WSAAPI
NSPStartup(
  _In_ LPGUID lpProviderId,
  _Inout_ LPNSP_ROUTINE lpnspRoutines);

#if !defined(_WIN64)
#include <poppack.h>
#endif

#ifdef __cplusplus
}
#endif
