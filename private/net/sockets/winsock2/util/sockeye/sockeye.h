/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sockeye.h

Abstract:

    Header file for the winsock browser util

Author:

    Dan Knudson (DanKn)    29-Jul-1996

Revision History:

--*/


#include "windows.h"
#include "winsock2.h"
#include "ws2spi.h"


//
// Symbolic constants
//

#define DS_NONZEROFIELDS            0x00000001
#define DS_ZEROFIELDS               0x00000002
#define DS_BYTEDUMP                 0x00000004

#define WT_SOCKET                   1

#define PT_DWORD                    1
#define PT_FLAGS                    2
#define PT_POINTER                  3
#define PT_STRING                   4
#define PT_ORDINAL                  5
#define PT_WSAPROTOCOLINFO          6
#define PT_QOS                      7
#define PT_PTRNOEDIT                8

#define FT_DWORD                    1
#define FT_FLAGS                    2
#define FT_ORD                      3
#define FT_SIZE                     4
#define FT_OFFSET                   5

#define MAX_STRING_PARAM_SIZE       96

#define MAX_USER_BUTTONS            6

#define MAX_USER_BUTTON_TEXT_SIZE   8

#define TABSIZE                     4

#define WM_ASYNCREQUESTCOMPLETED    WM_USER+0x55
#define WM_NETWORKEVENT             WM_USER+0x56


//
//
//

typedef LONG (WSAAPI *PFN0)(void);
typedef LONG (WSAAPI *PFN1)(ULONG_PTR);
typedef LONG (WSAAPI *PFN2)(ULONG_PTR, ULONG_PTR);
typedef LONG (WSAAPI *PFN3)(ULONG_PTR, ULONG_PTR, ULONG_PTR);
typedef LONG (WSAAPI *PFN4)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR);
typedef LONG (WSAAPI *PFN5)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                            ULONG_PTR);
typedef LONG (WSAAPI *PFN6)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                            ULONG_PTR, ULONG_PTR);
typedef LONG (WSAAPI *PFN7)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                            ULONG_PTR, ULONG_PTR, ULONG_PTR);
typedef LONG (WSAAPI *PFN8)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                            ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR);
typedef LONG (WSAAPI *PFN9)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                            ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                            ULONG_PTR);
typedef LONG (WSAAPI *PFN10)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                             ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                             ULONG_PTR, ULONG_PTR);
typedef LONG (WSAAPI *PFN12)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                             ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                             ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR);

typedef struct _MYSOCKET
{
    SOCKET      Sock;

    BOOL        bWSASocket;

    DWORD       dwAddressFamily;

    DWORD       dwSocketType;

    DWORD       dwFlags;

} MYSOCKET, *PMYSOCKET;


typedef struct _LOOKUP
{
    DWORD       dwVal;

    char       *lpszVal;

} LOOKUP, *PLOOKUP;


typedef enum
{
    ws_accept,
    ws_bind,
    ws_closesocket,
    ws_connect,
    ws_gethostbyaddr,
    ws_gethostbyname,
    ws_gethostname,
    ws_getpeername,
    ws_getprotobyname,
    ws_getprotobynumber,
    ws_getservbyname,
    ws_getservbyport,
    ws_getsockname,
    ws_getsockopt,
    ws_htonl,
    ws_htons,
    ws_inet_addr,
    ws_inet_ntoa,
    ws_ioctlsocket,
    ws_listen,
    ws_ntohl,
    ws_ntohs,
    ws_recv,
    ws_recvfrom,
    ws_select,
    ws_send,
    ws_sendto,
    ws_setsockopt,
    ws_shutdown,
    ws_socket,
    ws_WSAAccept,
    ws_WSAAddressToStringA,
    ws_WSAAddressToStringW,
    ws_WSAAsyncGetHostByAddr,
    ws_WSAAsyncGetHostByName,
    ws_WSAAsyncGetProtoByName,
    ws_WSAAsyncGetProtoByNumber,
    ws_WSAAsyncGetServByName,
    ws_WSAAsyncGetServByPort,
    ws_WSAAsyncSelect,
    ws_WSACancelAsyncRequest,
//    ws_WSACancelBlockingCall,
    ws_WSACleanup,
    ws_WSACloseEvent,
    ws_WSAConnect,
    ws_WSACreateEvent,
    ws_WSADuplicateSocketA,
    ws_WSADuplicateSocketW,
    ws_WSAEnumNameSpaceProvidersA,
    ws_WSAEnumNameSpaceProvidersW,
    ws_WSAEnumNetworkEvents,
    ws_WSAEnumProtocolsA,
    ws_WSAEnumProtocolsW,
    ws_WSAEventSelect,
    ws_WSAGetLastError,
    ws_WSAGetOverlappedResult,
    ws_WSAGetQOSByName,
    ws_WSAGetServiceClassInfoA,
    ws_WSAGetServiceClassInfoW,
    ws_WSAGetServiceClassNameByClassIdA,
    ws_WSAGetServiceClassNameByClassIdW,
    ws_WSAHtonl,
    ws_WSAHtons,
    ws_WSAInstallServiceClassA,
    ws_WSAInstallServiceClassW,
    ws_WSAIoctl,
//    ws_WSAIsBlocking,
    ws_WSAJoinLeaf,
    ws_WSALookupServiceBeginA,
    ws_WSALookupServiceBeginW,
    ws_WSALookupServiceEnd,
    ws_WSALookupServiceNextA,
    ws_WSALookupServiceNextW,
    ws_WSANtohl,
    ws_WSANtohs,
    ws_WSARecv,
    ws_WSARecvDisconnect,
    ws_WSARecvFrom,
    ws_WSARemoveServiceClass,
    ws_WSAResetEvent,
    ws_WSASend,
    ws_WSASendDisconnect,
    ws_WSASendTo,
//    ws_WSASetBlockingHook,
    ws_WSASetEvent,
    ws_WSASetLastError,
    ws_WSASetServiceA,
    ws_WSASetServiceW,
    ws_WSASocketA,
    ws_WSASocketW,
    ws_WSAStartup,
    ws_WSAStringToAddressA,
    ws_WSAStringToAddressW,
//    ws_WSAUnhookBlockingHook,
    ws_WSAWaitForMultipleEvents,

    ws_WSCEnumProtocols,
    ws_WSCGetProviderPath,

    ws_EnumProtocolsA,
    ws_EnumProtocolsW,
    ws_GetAddressByNameA,
    ws_GetAddressByNameW,
    ws_GetNameByTypeA,
    ws_GetNameByTypeW,
    ws_GetServiceA,
    ws_GetServiceW,
    ws_GetTypeByNameA,
    ws_GetTypeByNameW,
    ws_SetServiceA,
    ws_SetServiceW,

//    CloseHandl,
//    DumpBuffer,

    MiscBegin,

    DefValues,
    WSAProtoInfo,
    ws_QOS

} FUNC_INDEX;


typedef struct _MYOVERLAPPED
{
    WSAOVERLAPPED   WSAOverlapped;

    FUNC_INDEX      FuncIndex;

    DWORD           dwFuncSpecific1;

} MYOVERLAPPED, *PMYOVERLAPPED;


typedef struct _FUNC_PARAM
{
    char far    *szName;

    DWORD       dwType;

    ULONG_PTR   dwValue;

    union
    {
        LPVOID      pLookup;

        char far    *buf;

        LPVOID      ptr;

        ULONG_PTR   dwDefValue;

    } u;

} FUNC_PARAM, *PFUNC_PARAM;


typedef struct _FUNC_PARAM_HEADER
{
    DWORD       dwNumParams;

    FUNC_INDEX  FuncIndex;

    PFUNC_PARAM aParams;

    union
    {
        PFN0    pfn0;
        PFN1    pfn1;
        PFN2    pfn2;
        PFN3    pfn3;
        PFN4    pfn4;
        PFN5    pfn5;
        PFN6    pfn6;
        PFN7    pfn7;
        PFN8    pfn8;
        PFN9    pfn9;
        PFN10   pfn10;
        PFN12   pfn12;

    } u;

} FUNC_PARAM_HEADER, *PFUNC_PARAM_HEADER;


typedef struct _STRUCT_FIELD
{
    char far    *szName;

    DWORD       dwType;

    DWORD       dwValue;

    LPVOID      pLookup;

} STRUCT_FIELD, *PSTRUCT_FIELD;


typedef struct _STRUCT_FIELD_HEADER
{
    LPVOID      pStruct;

    char far    *szName;

    DWORD       dwNumFields;

    PSTRUCT_FIELD   aFields;

} STRUCT_FIELD_HEADER, *PSTRUCT_FIELD_HEADER;


typedef struct _ASYNC_REQUEST_INFO
{
    HANDLE      hRequest;

    char FAR    *pszFuncName;

    FUNC_INDEX  FuncIndex;

    struct _ASYNC_REQUEST_INFO *pPrev;

    struct _ASYNC_REQUEST_INFO *pNext;

} ASYNC_REQUEST_INFO, *PASYNC_REQUEST_INFO;


//
// Func prototypes
//

INT_PTR
CALLBACK
MainWndProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

void
FAR
ShowStr(
    LPCSTR format,
    ...
    );

INT_PTR
CALLBACK
ParamsDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
CALLBACK
AboutDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
CALLBACK
IconDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

LONG
DoFunc(
    PFUNC_PARAM_HEADER pHeader
    );

BOOL
LetUserMungeParams(
    PFUNC_PARAM_HEADER pParamsHeader
    );

void
ShowLineFuncResult(
    LPSTR lpFuncName,
    LONG  lResult
    );

void
FuncDriver(
    FUNC_INDEX funcIndex
    );

INT_PTR
CALLBACK
UserButtonsDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

void
FAR
ShowHostEnt(
    struct hostent  *phe
    );

void
FAR
ShowProtoEnt(
    struct protoent *ppe
    );

void
FAR
ShowServEnt(
    struct servent  *pse
    );

void
PASCAL
QueueAsyncRequestInfo(
    PASYNC_REQUEST_INFO pAsyncReqInfo
    );

PASYNC_REQUEST_INFO
PASCAL
DequeueAsyncRequestInfo(
    HANDLE  hRequest
    );

void
PASCAL
ShowBytes(
    DWORD   dwSize,
    LPVOID  lp,
    DWORD   dwNumTabs
    );

void
PASCAL
ShowFlags(
    DWORD       dwValue,
    char FAR    *pszValueName,
    PLOOKUP     pLookup
    );

void
UpdateResults(
    BOOL bBegin
    );

LPSTR
PASCAL
GetStringFromOrdinalValue(
    DWORD   dwValue,
    PLOOKUP pLookup
    );

VOID
PASCAL
ShowProtoInfo(
    LPWSAPROTOCOL_INFOA pInfo,
    DWORD               dwIndex,
    BOOL                bAscii
    );
