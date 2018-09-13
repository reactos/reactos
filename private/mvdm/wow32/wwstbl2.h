/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WWSTBL2.h
 *  WOW32 16-bit Winsock API tables
 *
 *  History:
 *  Created 02-Oct-1992 by David Treadwell (davidtr)
 *
 *  This file is included into the master thunk table.
 *
--*/

    {W32FUN(UNIMPLEMENTEDAPI,              "DUMMYENTRY",           MOD_WINSOCK,    0)},
    {W32FUN(WWS32accept,                   "ACCEPT",               MOD_WINSOCK,    sizeof(ACCEPT16))},
    {W32FUN(WWS32bind,                     "BIND",                 MOD_WINSOCK,    sizeof(BIND16))},
    {W32FUN(WWS32closesocket,              "CLOSESOCKET",          MOD_WINSOCK,    sizeof(CLOSESOCKET16))},
    {W32FUN(WWS32connect,                  "CONNECT",              MOD_WINSOCK,    sizeof(CONNECT16))},
    {W32FUN(WWS32getpeername,              "GETPEERNAME",          MOD_WINSOCK,    sizeof(GETPEERNAME16))},
    {W32FUN(WWS32getsockname,              "GETSOCKNAME",          MOD_WINSOCK,    sizeof(GETSOCKNAME16))},
    {W32FUN(WWS32getsockopt,               "GETSOCKOPT",           MOD_WINSOCK,    sizeof(GETSOCKOPT16))},
    {W32FUN(WWS32htonl,                    "HTONL",                MOD_WINSOCK,    sizeof(HTONL16))},
    {W32FUN(WWS32htons,                    "HTONS",                MOD_WINSOCK,    sizeof(HTONS16))},

  /*** 0010 ***/
    {W32FUN(WWS32inet_addr,                "INET_ADDR",            MOD_WINSOCK,    sizeof(INET_ADDR16))},
    {W32FUN(WWS32inet_ntoa,                "INET_NTOA",            MOD_WINSOCK,    sizeof(INET_NTOA16))},
    {W32FUN(WWS32ioctlsocket,              "IOCTLSOCKET",          MOD_WINSOCK,    sizeof(IOCTLSOCKET16))},
    {W32FUN(WWS32listen,                   "LISTEN",               MOD_WINSOCK,    sizeof(LISTEN16))},
    {W32FUN(WWS32ntohl,                    "NTOHL",                MOD_WINSOCK,    sizeof(NTOHL16))},
    {W32FUN(WWS32ntohs,                    "NTOHS",                MOD_WINSOCK,    sizeof(NTOHS16))},
    {W32FUN(WWS32recv,                     "RECV",                 MOD_WINSOCK,    sizeof(RECV16))},
    {W32FUN(WWS32recvfrom,                 "RECVFROM",             MOD_WINSOCK,    sizeof(RECVFROM16))},
    {W32FUN(WWS32select,                   "SELECT",               MOD_WINSOCK,    sizeof(SELECT16))},
    {W32FUN(WWS32send,                     "SEND",                 MOD_WINSOCK,    sizeof(SEND16))},

  /*** 0020 ***/
    {W32FUN(WWS32sendto,                   "SENDTO",               MOD_WINSOCK,    sizeof(SENDTO16))},
    {W32FUN(WWS32setsockopt,               "SETSOCKOPT",           MOD_WINSOCK,    sizeof(SETSOCKOPT16))},
    {W32FUN(WWS32shutdown,                 "SHUTDOWN",             MOD_WINSOCK,    sizeof(SHUTDOWN16))},
    {W32FUN(WWS32socket,                   "SOCKET",               MOD_WINSOCK,    sizeof(SOCKET16))},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},

  /*** 0030 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},

  /*** 0040 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},

  /*** 0050 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(WWS32gethostbyaddr,            "GETHOSTBYADDR",        MOD_WINSOCK,    sizeof(GETHOSTBYADDR16))},
    {W32FUN(WWS32gethostbyname,            "GETHOSTBYNAME",        MOD_WINSOCK,    sizeof(GETHOSTBYNAME16))},
    {W32FUN(WWS32getprotobyname,           "GETPROTOBYNAME",       MOD_WINSOCK,    sizeof(GETPROTOBYNAME16))},
    {W32FUN(WWS32getprotobynumber,         "GETPROTOBYNUMBER",     MOD_WINSOCK,    sizeof(GETPROTOBYNUMBER16))},
    {W32FUN(WWS32getservbyname,            "GETSERVBYNAME",        MOD_WINSOCK,    sizeof(GETSERVBYNAME16))},
    {W32FUN(WWS32getservbyport,            "GETSERVBYPORT",        MOD_WINSOCK,    sizeof(GETSERVBYPORT16))},
    {W32FUN(WWS32gethostname,              "GETHOSTNAME",          MOD_WINSOCK,    sizeof(GETHOSTNAME16))},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},

  /*** 0060 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},

  /*** 0070 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                     MOD_WINSOCK,    0)},

  /*** 0080 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},

  /*** 0090 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},

  /*** 0100 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(WWS32WSAAsyncSelect,           "WSAASYNCSELECT",         MOD_WINSOCK,  sizeof(WSAASYNCSELECT16))},
    {W32FUN(WWS32WSAAsyncGetHostByAddr,    "WSAASYNCGETHOSTBYADDR",  MOD_WINSOCK,  sizeof(WSAASYNCGETHOSTBYADDR16))},
    {W32FUN(WWS32WSAAsyncGetHostByName,    "WSAASYNCGETHOSTBYNAME",  MOD_WINSOCK,  sizeof(WSAASYNCGETHOSTBYNAME16))},
    {W32FUN(WWS32WSAAsyncGetProtoByNumber, "WSAASYNCGETPROTOBYNUMBER",MOD_WINSOCK, sizeof(WSAASYNCGETPROTOBYNUMBER16))},
    {W32FUN(WWS32WSAAsyncGetProtoByName,   "WSAASYNCGETPROTOBYNAME", MOD_WINSOCK,  sizeof(WSAASYNCGETPROTOBYNAME16))},
    {W32FUN(WWS32WSAAsyncGetServByPort,    "WSAASYNCGETSERVBYPORT",  MOD_WINSOCK,  sizeof(WSAASYNCGETSERVBYPORT16))},
    {W32FUN(WWS32WSAAsyncGetServByName,    "WSAASYNCGETSERVBYNAME",  MOD_WINSOCK,  sizeof(WSAASYNCGETSERVBYNAME16))},
    {W32FUN(WWS32WSACancelAsyncRequest,    "WSACANCELASYNCREQUEST",  MOD_WINSOCK,  sizeof(WSACANCELASYNCREQUEST16))},
    {W32FUN(WWS32WSASetBlockingHook,       "WSASETBLOCKINGHOOK",     MOD_WINSOCK,  sizeof(WSASETBLOCKINGHOOK16))},

  /*** 0110 ***/
    {W32FUN(WWS32WSAUnhookBlockingHook,    "WSAUNHOOKBLOCKINGHOOK",  MOD_WINSOCK,  0)},
    {W32FUN(WWS32WSAGetLastError,          "WSAGETLASTERROR",        MOD_WINSOCK,  0)},
    {W32FUN(WWS32WSASetLastError,          "WSASETLASTERROR",        MOD_WINSOCK,  sizeof(WSASETLASTERROR16))},
    {W32FUN(WWS32WSACancelBlockingCall,    "WSACANCELBLOCKINGCALL",  MOD_WINSOCK,  0)},
    {W32FUN(WWS32WSAIsBlocking,            "WSAISBLOCKING",          MOD_WINSOCK,  0)},
    {W32FUN(WWS32WSAStartup,               "WSASTARTUP",             MOD_WINSOCK,  sizeof(WSASTARTUP16))},
    {W32FUN(WWS32WSACleanup,               "WSACLEANUP",             MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},

  /*** 0120 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},

  /*** 0130 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},

  /*** 0140 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},

  /*** 0150 ***/
    {W32FUN(UNIMPLEMENTEDAPI,              "",                       MOD_WINSOCK,  0)},
    {W32FUN(WWS32__WSAFDIsSet,             "__WSAFDISSET",           MOD_WINSOCK,  sizeof(__WSAFDISSET16))},
