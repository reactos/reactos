/*++

Copyright (c) 1995 Intel Corp

File Name:

    dt_dll.h

Abstract:

    This header describes the interface to the WinSock 2 debug/trace
    DLL.  Please see the design spec for more information.

Author:

    Michael A. Grafton

--*/

#ifndef _DT_DLL_H
#define _DT_DLL_H

#include "warnoff.h"
#include <windows.h>


//
// This type defines a pointer to the Pre/PostApiNotify functions
//

typedef BOOL (WINAPIV * LPFNWSANOTIFY)(
    IN  INT    NotificationCode,
    OUT LPVOID ReturnCode,
    IN  LPSTR  LibraryName,
    ...);

//
// Function prototypes for Pre/PostApiNotify
//

BOOL WINAPIV
WSAPreApiNotify(
    IN  INT    NotificationCode,
    OUT LPVOID ReturnCode,
    IN  LPSTR  LibraryName,
    ...);

BOOL WINAPIV
WSAPostApiNotify(
    IN  INT    NotificationCode,
    OUT LPVOID ReturnCode,
    IN  LPSTR  LibraryName,
    ...);

//
// Pointer to an exception notification function.
//

typedef
VOID
(WINAPI * LPFNWSAEXCEPTIONNOTIFY)(
    IN LPEXCEPTION_POINTERS ExceptionPointers
    );

//
// Function prototype for exception notify.
//

VOID
WINAPI
WSAExceptionNotify(
    IN LPEXCEPTION_POINTERS ExceptionPointers
    );


//
// API function codes for Pre/PostApiNotify functions.  Note:  These must start
// at  1  or more and be fairly densely assigned.  Small gaps can be tolerated.
// Note  that  a  "MAX_DTCODE"  definition  should  be updated if new codes are
// added.
//

#define DTCODE_accept 1
#define DTCODE_bind 2
#define DTCODE_closesocket 3
#define DTCODE_connect 4
#define DTCODE_getpeername 5
#define DTCODE_getsockname 6
#define DTCODE_getsockopt 7
#define DTCODE_htonl 8
#define DTCODE_htons 9
#define DTCODE_ioctlsocket 10
#define DTCODE_listen 11
#define DTCODE_ntohl 12
#define DTCODE_ntohs 13
#define DTCODE_recv 14
#define DTCODE_recvfrom 15
#define DTCODE_select 16
#define DTCODE_send 17
#define DTCODE_sendto 18
#define DTCODE_setsockopt 19
#define DTCODE_shutdown 20
#define DTCODE_socket 21
#define DTCODE_WSAAccept 22
#define DTCODE_WSAAsyncSelect 23
#define DTCODE_WSACancelBlockingCall 24
#define DTCODE_WSACleanup 25
#define DTCODE_WSACloseEvent 26
#define DTCODE_WSAConnect 27
#define DTCODE_WSACreateEvent 28
#define DTCODE_WSADuplicateSocketA 29
#define DTCODE_WSAEnumNetworkEvents 30
#define DTCODE_WSAEnumProtocolsA 31
#define DTCODE_WSAEventSelect 32
#define DTCODE_WSAGetLastError 33
#define DTCODE_WSAGetOverlappedResult 34
#define DTCODE_WSAGetQOSByName 35
#define DTCODE_WSAHtonl 36
#define DTCODE_WSAHtons 37
#define DTCODE_WSAIoctl 38
#define DTCODE_WSAIsBlocking 39
#define DTCODE_WSAJoinLeaf 40
#define DTCODE_WSANtohl 41
#define DTCODE_WSANtohs 42
#define DTCODE_WSARecv 43
#define DTCODE_WSARecvDisconnect 44
#define DTCODE_WSARecvFrom 45
#define DTCODE_WSAResetEvent 46
#define DTCODE_WSASend 47
#define DTCODE_WSASendDisconnect 48
#define DTCODE_WSASendTo 49
#define DTCODE_WSASetBlockingHook 50
#define DTCODE_WSASetEvent 51
#define DTCODE_WSASetLastError 52
#define DTCODE_WSASocketA 53
#define DTCODE_WSAStartup 54
#define DTCODE_WSAUnhookBlockingHook 55
#define DTCODE_WSAWaitForMultipleEvents 56
#define DTCODE_gethostbyaddr 57
#define DTCODE_gethostbyname 58
#define DTCODE_gethostname 59
#define DTCODE_getprotobyname 60
#define DTCODE_getprotobynumber 61
#define DTCODE_getservbyname 62
#define DTCODE_getservbyport 63
#define DTCODE_inet_addr 64
#define DTCODE_inet_ntoa 65
#define DTCODE_WSAAsyncGetHostByAddr 66
#define DTCODE_WSAAsyncGetHostByName 67
#define DTCODE_WSAAsyncGetProtoByName 68
#define DTCODE_WSAAsyncGetProtoByNumber 69
#define DTCODE_WSAAsyncGetServByName 70
#define DTCODE_WSAAsyncGetServByPort 71
#define DTCODE_WSACancelAsyncRequest 72
#define DTCODE_WSPAccept 73
#define DTCODE_WSPAsyncSelect 74
#define DTCODE_WSPBind 75
#define DTCODE_WSPCancelBlockingCall 76
#define DTCODE_WSPCleanup 77
#define DTCODE_WSPCloseSocket 78
#define DTCODE_WSPConnect 79
#define DTCODE_WSPDuplicateSocket 80
#define DTCODE_WSPEnumNetworkEvents 81
#define DTCODE_WSPEventSelect 82
#define DTCODE_WSPGetOverlappedResult 83
#define DTCODE_WSPGetPeerName 84
// The  WSPGetProcTable  function  has  been removed, but the code numbers have
// been kept the same.
// #define DTCODE_WSPGetProcTable 85
#define DTCODE_WSPGetSockName 86
#define DTCODE_WSPGetSockOpt 87
#define DTCODE_WSPGetQOSByName 88
#define DTCODE_WSPIoctl 89
#define DTCODE_WSPJoinLeaf 90
#define DTCODE_WSPListen 91
#define DTCODE_WSPRecv 92
#define DTCODE_WSPRecvDisconnect 93
#define DTCODE_WSPRecvFrom 94
#define DTCODE_WSPSelect 95
#define DTCODE_WSPSend 96
#define DTCODE_WSPSendDisconnect 97
#define DTCODE_WSPSendTo 98
#define DTCODE_WSPSetSockOpt 99
#define DTCODE_WSPShutdown 100
#define DTCODE_WSPSocket 101
#define DTCODE_WSPStartup 102
#define DTCODE_WPUCloseEvent 103
#define DTCODE_WPUCloseSocketHandle 104
#define DTCODE_WPUCreateEvent 105
#define DTCODE_WPUCreateSocketHandle 106
#define DTCODE_WSCDeinstallProvider 107
#define DTCODE_WSCInstallProvider 108
#define DTCODE_WPUModifyIFSHandle 109
#define DTCODE_WPUQueryBlockingCallback 110
#define DTCODE_WPUQuerySocketHandleContext 111
#define DTCODE_WPUQueueApc 112
#define DTCODE_WPUResetEvent 113
#define DTCODE_WPUSetEvent 114
#define DTCODE_WSCEnumProtocols 115
#define DTCODE_WPUGetProviderPath 116
#define DTCODE_WPUPostMessage 117
#define DTCODE_WPUFDIsSet 118
#define DTCODE_WSADuplicateSocketW 119
#define DTCODE_WSAEnumProtocolsW 120
#define DTCODE_WSASocketW 121
#define DTCODE___WSAFDIsSet 122
#define DTCODE_WSAAddressToStringA 123
#define DTCODE_WSAAddressToStringW 124
#define DTCODE_WSAStringToAddressA 125
#define DTCODE_WSAStringToAddressW 126
#define DTCODE_WSALookupServiceBeginA 127
#define DTCODE_WSALookupServiceBeginW 128
#define DTCODE_WSALookupServiceNextA 129
#define DTCODE_WSALookupServiceNextW 130
#define DTCODE_WSALookupServiceEnd 131
//
// WSAGetAddressByName[AW] have been removed.
//
// #define DTCODE_WSAGetAddressByNameA 132
// #define DTCODE_WSAGetAddressByNameW 133
#define DTCODE_WSAInstallServiceClassA 134
#define DTCODE_WSAInstallServiceClassW 135
#define DTCODE_WSASetServiceA 136
#define DTCODE_WSASetServiceW 137
#define DTCODE_WSARemoveServiceClass 138
#define DTCODE_WSAGetServiceClassInfoA 139
#define DTCODE_WSAGetServiceClassInfoW 140
#define DTCODE_WSAEnumNameSpaceProvidersA 141
#define DTCODE_WSAEnumNameSpaceProvidersW 142
#define DTCODE_WSAGetServiceClassNameByClassIdA 143
#define DTCODE_WSAGetServiceClassNameByClassIdW 144
#define DTCODE_NSPAddressToString 145
#define DTCODE_NSPStringToAddress 146
#define DTCODE_NSPLookupServiceBegin 147
#define DTCODE_NSPLookupServiceNext 148
#define DTCODE_NSPLookupServiceEnd 149
#define DTCODE_NSPGetAddressByName 150
#define DTCODE_NSPInstallServiceClass 151
#define DTCODE_NSPSetService 152
#define DTCODE_NSPRemoveServiceClass 153
#define DTCODE_NSPGetServiceClassInfo 154
#define DTCODE_NSPEnumNameSpaceProviders 155
#define DTCODE_NSPGetServiceClassNameByClassId 156
#define DTCODE_WSCGetProviderPath 157
#define DTCODE_WSCInstallNameSpace 158
#define DTCODE_WSCUnInstallNameSpace 159
#define DTCODE_WSCEnableNSProvider 160
#define DTCODE_WSPAddressToString 161
#define DTCODE_WSPStringToAddress 162

#define MAX_DTCODE DTCODE_WSPStringToAddress

#endif

