/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWWSOCK.H
 *  16-bit Winsock API argument structures
 *
 *  History:
 *  Created 02-Oct-1992 by David Treadwell (davidtr)
--*/

//#include <windows.h>
//#include <winsock.h>

/* XLATOFF */
#pragma pack(1)
/* XLATON */

/*++
 *
 * Winsock data structures
 *
--*/

typedef WORD HSOCKET16;
typedef DWORD IN_ADDR16;

typedef struct _SOCKADDR16 {                           /* sa16 */
    WORD sa_family;
    BYTE sa_data[14];
} SOCKADDR16;
typedef SOCKADDR16 UNALIGNED *PSOCKADDR16;
typedef VPVOID VPSOCKADDR;

#define FD_SETSIZE      64

typedef struct _FD_SET16 {                             /* fd16 */
    WORD fd_count;
    HSOCKET16 fd_array[FD_SETSIZE];
} FD_SET16;
typedef FD_SET16 UNALIGNED *PFD_SET16;
typedef VPVOID VPFD_SET16;

#define WSADESCRIPTION_LEN      256
#define WSASYS_STATUS_LEN       128

typedef struct _WSADATA16 {                            /* wd16 */
    WORD wVersion;
    WORD wHighVersion;
    CHAR szDescription[WSADESCRIPTION_LEN+1];
    CHAR szSystemStatus[WSASYS_STATUS_LEN+1];
    INT16 iMaxSockets;
    INT16 iMaxUdpDg;
    VPBYTE lpVendorInfo;
} WSADATA16;
typedef WSADATA16 UNALIGNED *PWSADATA16;
typedef VPVOID VPWSADATA16;

typedef struct _TIMEVAL16 {                            /* tv16 */
    DWORD tv_sec;         /* seconds */
    DWORD tv_usec;        /* and microseconds */
} TIMEVAL16;
typedef TIMEVAL16 *PTIMEVAL16;
typedef VPVOID VPTIMEVAL16;

typedef struct _HOSTENT16 {                            /* he16 */
    VPSZ h_name;
    VPVOID h_aliases;
    WORD h_addrtype;
    WORD h_length;
    VPBYTE h_addr_list;
} HOSTENT16;
typedef HOSTENT16 *PHOSTENT16;
typedef VPVOID VPHOSTENT16;

typedef struct _PROTOENT16 {                           /* pe16 */
    VPSZ p_name;
    VPVOID p_aliases;
    WORD p_proto;
} PROTOENT16;
typedef PROTOENT16 *PPROTOENT16;
typedef VPVOID VPPROTOENT16;

typedef struct _SERVENT16 {                            /* se16 */
    VPSZ s_name;
    VPVOID s_aliases;
    WORD s_port;
    VPSZ s_proto;
} SERVENT16;
typedef SERVENT16 *PSERVENT16;
typedef VPVOID VPSERVENT16;

typedef struct _NETENT16 {                             /* ne16 */
    VPSZ n_name;
    VPVOID n_aliases;
    WORD n_addrtype;
    DWORD n_net;
} NETENT16;
typedef NETENT16 *PNETENT16;
typedef VPVOID VPNETENT16;

/*++
 *
 * Winsock API IDs (equal to ordinal numbers)
 *
--*/

#define FUN_ACCEPT                         1   //
#define FUN_BIND                           2   //
#define FUN_CLOSESOCKET                    3   //
#define FUN_CONNECT                        4   //
#define FUN_GETPEERNAME                    5   //
#define FUN_GETSOCKNAME                    6   //
#define FUN_GETSOCKOPT                     7   //
#define FUN_HTONL                          8   //
#define FUN_HTONS                          9   //
#define FUN_INET_ADDR                      10  //
#define FUN_INET_NTOA                      11  //
#define FUN_IOCTLSOCKET                    12  //
#define FUN_LISTEN                         13  //
#define FUN_NTOHL                          14  //
#define FUN_NTOHS                          15  //
#define FUN_RECV                           16  //
#define FUN_RECVFROM                       17  //
#define FUN_SELECT                         18  //
#define FUN_SEND                           19  //
#define FUN_SENDTO                         20  //
#define FUN_SETSOCKOPT                     21  //
#define FUN_SHUTDOWN                       22  //
#define FUN_SOCKET                         23  //

#define FUN_GETHOSTBYADDR                  51  //
#define FUN_GETHOSTBYNAME                  52  //
#define FUN_GETPROTOBYNAME                 53  //
#define FUN_GETPROTOBYNUMBER               54  //
#define FUN_GETSERVBYNAME                  55  //
#define FUN_GETSERVBYPORT                  56  //
#define FUN_GETHOSTNAME                    57  //

#define FUN_WSAASYNCSELECT                 101 //
#define FUN_WSAASYNCGETHOSTBYADDR          102 //
#define FUN_WSAASYNCGETHOSTBYNAME          103 //
#define FUN_WSAASYNCGETPROTOBYNUMBER       104 //
#define FUN_WSAASYNCGETPROTOBYNAME         105 //
#define FUN_WSAASYNCGETSERVBYPORT          106 //
#define FUN_WSAASYNCGETSERVBYNAME          107 //
#define FUN_WSACANCELASYNCREQUEST          108 //
#define FUN_WSASETBLOCKINGHOOK             109 //
#define FUN_WSAUNHOOKBLOCKINGHOOK          110 //
#define FUN_WSAGETLASTERROR                111 //
#define FUN_WSASETLASTERROR                112 //
#define FUN_WSACANCELBLOCKINGCALL          113 //
#define FUN_WSAISBLOCKING                  114 //
#define FUN_WSASTARTUP                     115 //
#define FUN_WSACLEANUP                     116 //

#define FUN___WSAFDISSET                   151 //

/*++

  Winsock function prototypes - the seemingly unimportant number in the
  comment on each function MUST match the ones in the list above!!!

  !! BE WARNED !!

--*/

typedef struct _ACCEPT16 {                             /* ws1 */
    VPWORD AddressLength;
    VPSOCKADDR Address;
    HSOCKET16 hSocket;
} ACCEPT16;
typedef ACCEPT16 UNALIGNED *PACCEPT16;

typedef struct _BIND16 {                               /* ws2 */
    WORD AddressLength;
    VPSOCKADDR Address;
    HSOCKET16 hSocket;
} BIND16;
typedef BIND16 UNALIGNED *PBIND16;

typedef struct _CLOSESOCKET16 {                        /* ws3 */
    HSOCKET16 hSocket;
} CLOSESOCKET16;
typedef CLOSESOCKET16 UNALIGNED *PCLOSESOCKET16;

typedef struct _CONNECT16 {                            /* ws4 */
    WORD AddressLength;
    VPSOCKADDR Address;
    HSOCKET16 hSocket;
} CONNECT16;
typedef CONNECT16 UNALIGNED *PCONNECT16;

typedef struct _GETPEERNAME16 {                        /* ws5 */
    VPWORD AddressLength;
    VPSOCKADDR Address;
    HSOCKET16 hSocket;
} GETPEERNAME16;
typedef GETPEERNAME16 UNALIGNED *PGETPEERNAME16;

typedef struct _GETSOCKNAME16 {                        /* ws6 */
    VPWORD AddressLength;
    VPSOCKADDR Address;
    HSOCKET16 hSocket;
} GETSOCKNAME16;
typedef GETSOCKNAME16 UNALIGNED *PGETSOCKNAME16;

typedef struct _GETSOCKOPT16 {                         /* ws7 */
    VPWORD OptionLength;
    VPBYTE OptionValue;
    WORD OptionName;
    WORD Level;
    HSOCKET16 hSocket;
} GETSOCKOPT16;
typedef GETSOCKOPT16 UNALIGNED *PGETSOCKOPT16;

typedef struct _HTONL16 {                              /* ws8 */
    DWORD HostLong;
} HTONL16;
typedef HTONL16 UNALIGNED *PHTONL16;

typedef struct _HTONS16 {                              /* ws9 */
    WORD HostShort;
} HTONS16;
typedef HTONS16 UNALIGNED *PHTONS16;

typedef struct _INET_ADDR16 {                          /* ws10 */
    VPSZ cp;
} INET_ADDR16;
typedef INET_ADDR16 UNALIGNED *PINET_ADDR16;

typedef struct _INET_NTOA16 {                          /* ws11 */
    IN_ADDR16 in;
} INET_NTOA16;
typedef INET_NTOA16 UNALIGNED *PINET_NTOA16;

typedef struct _IOCTLSOCKET16 {                        /* ws12 */
    VPDWORD Argument;
    DWORD Command;
    HSOCKET16 hSocket;
} IOCTLSOCKET16;
typedef IOCTLSOCKET16 UNALIGNED *PIOCTLSOCKET16;

typedef struct _LISTEN16 {                             /* ws13 */
    WORD Backlog;
    HSOCKET16 hSocket;
} LISTEN16;
typedef LISTEN16 UNALIGNED *PLISTEN16;

typedef struct _NTOHL16 {                              /* ws14 */
    DWORD NetLong;
} NTOHL16;
typedef NTOHL16 UNALIGNED *PNTOHL16;

typedef struct _NTOHS16 {                              /* ws15 */
    WORD NetShort;
} NTOHS16;
typedef NTOHS16 UNALIGNED *PNTOHS16;

typedef struct _RECV16 {                               /* ws16 */
    WORD Flags;
    WORD BufferLength;
    VPBYTE Buffer;
    HSOCKET16 hSocket;
} RECV16;
typedef RECV16 UNALIGNED *PRECV16;

typedef struct _RECVFROM16 {                           /* ws17 */
    VPWORD AddressLength;
    VPSOCKADDR Address;
    WORD Flags;
    WORD BufferLength;
    VPBYTE Buffer;
    HSOCKET16 hSocket;
} RECVFROM16;
typedef RECVFROM16 UNALIGNED *PRECVFROM16;

typedef struct _SELECT16 {                             /* ws18 */
    VPTIMEVAL16 Timeout;
    VPFD_SET16 Exceptfds;
    VPFD_SET16 Writefds;
    VPFD_SET16 Readfds;
    WORD HandleCount;
} SELECT16;
typedef SELECT16 UNALIGNED *PSELECT16;

typedef struct _SEND16 {                               /* ws19 */
    WORD Flags;
    WORD BufferLength;
    VPBYTE Buffer;
    HSOCKET16 hSocket;
} SEND16;
typedef SEND16 UNALIGNED *PSEND16;

typedef struct _SENDTO16 {                             /* ws20 */
    WORD AddressLength;
    VPSOCKADDR Address;
    WORD Flags;
    WORD BufferLength;
    VPBYTE Buffer;
    HSOCKET16 hSocket;
} SENDTO16;
typedef SENDTO16 UNALIGNED *PSENDTO16;

typedef struct _SETSOCKOPT16 {                         /* ws21 */
    WORD OptionLength;
    VPBYTE OptionValue;
    WORD OptionName;
    WORD Level;
    HSOCKET16 hSocket;
} SETSOCKOPT16;
typedef SETSOCKOPT16 UNALIGNED *PSETSOCKOPT16;

typedef struct _SHUTDOWN16 {                           /* ws22 */
    WORD How;
    HSOCKET16 hSocket;
} SHUTDOWN16;
typedef SHUTDOWN16 UNALIGNED *PSHUTDOWN16;

typedef struct _SOCKET16 {                             /* ws23 */
    WORD Protocol;
    WORD Type;
    WORD AddressFamily;
} SOCKET16;
typedef SOCKET16 UNALIGNED *PSOCKET16;

typedef struct _GETHOSTBYADDR16 {                      /* ws51 */
    WORD Type;
    WORD Length;
    VPBYTE Address;
} GETHOSTBYADDR16;
typedef GETHOSTBYADDR16 UNALIGNED *PGETHOSTBYADDR16;

typedef struct _GETHOSTBYNAME16 {                      /* ws52 */
    VPSZ Name;
} GETHOSTBYNAME16;
typedef GETHOSTBYNAME16 UNALIGNED *PGETHOSTBYNAME16;

typedef struct _GETPROTOBYNAME16 {                     /* ws53 */
    VPSZ Name;
} GETPROTOBYNAME16;
typedef GETPROTOBYNAME16 UNALIGNED *PGETPROTOBYNAME16;

typedef struct _GETPROTOBYNUMBER16 {                   /* ws54 */
    WORD Protocol;
} GETPROTOBYNUMBER16;
typedef GETPROTOBYNUMBER16 UNALIGNED *PGETPROTOBYNUMBER16;

typedef struct _GETSERVBYNAME16 {                      /* ws55 */
    VPSZ Protocol;
    VPSZ Name;
} GETSERVBYNAME16;
typedef GETSERVBYNAME16 UNALIGNED *PGETSERVBYNAME16;

typedef struct _GETSERVBYPORT16 {                      /* ws56 */
    VPSZ Protocol;
    WORD Port;
} GETSERVBYPORT16;
typedef GETSERVBYPORT16 UNALIGNED *PGETSERVBYPORT16;

typedef struct _GETHOSTNAME16 {                        /* ws57 */
    WORD NameLength;
    VPSZ Name;
} GETHOSTNAME16;
typedef GETHOSTNAME16 UNALIGNED *PGETHOSTNAME16;

typedef struct _WSAASYNCSELECT16 {                     /* ws101 */
    DWORD lEvent;
    WORD wMsg;
    HWND16 hWnd;
    HSOCKET16 hSocket;
} WSAASYNCSELECT16;
typedef WSAASYNCSELECT16 UNALIGNED *PWSAASYNCSELECT16;

typedef struct _WSAASYNCGETHOSTBYADDR16 {              /* ws102 */
    WORD BufferLength;
    VPBYTE Buffer;
    WORD Type;
    WORD Length;
    VPBYTE Address;
    WORD wMsg;
    HWND16 hWnd;
} WSAASYNCGETHOSTBYADDR16;
typedef WSAASYNCGETHOSTBYADDR16 UNALIGNED *PWSAASYNCGETHOSTBYADDR16;

typedef struct _WSAASYNCGETHOSTBYNAME16 {              /* ws103 */
    WORD BufferLength;
    VPBYTE Buffer;
    VPSZ Name;
    WORD wMsg;
    HWND16 hWnd;
} WSAASYNCGETHOSTBYNAME16;
typedef WSAASYNCGETHOSTBYNAME16 UNALIGNED *PWSAASYNCGETHOSTBYNAME16;

typedef struct _WSAASYNCGETPROTOBYNUMBER16 {           /* ws104 */
    WORD BufferLength;
    VPBYTE Buffer;
    WORD Number;
    WORD wMsg;
    HWND16 hWnd;
} WSAASYNCGETPROTOBYNUMBER16;
typedef WSAASYNCGETPROTOBYNUMBER16 UNALIGNED *PWSAASYNCGETPROTOBYNUMBER16;

typedef struct _WSAASYNCGETPROTOBYNAME16 {             /* ws105 */
    WORD BufferLength;
    VPBYTE Buffer;
    VPSZ Name;
    WORD wMsg;
    HWND16 hWnd;
} WSAASYNCGETPROTOBYNAME16;
typedef WSAASYNCGETPROTOBYNAME16 UNALIGNED *PWSAASYNCGETPROTOBYNAME16;

typedef struct _WSAASYNCGETSERVBYPORT16 {              /* ws106 */
    WORD BufferLength;
    VPBYTE Buffer;
    VPSZ Protocol;
    WORD Port;
    WORD wMsg;
    HWND16 hWnd;
} WSAASYNCGETSERVBYPORT16;
typedef WSAASYNCGETSERVBYPORT16 UNALIGNED *PWSAASYNCGETSERVBYPORT16;

typedef struct _WSAASYNCGETSERVBYNAME16 {              /* ws107 */
    WORD BufferLength;
    VPBYTE Buffer;
    VPSZ Protocol;
    VPSZ Name;
    WORD wMsg;
    HWND16 hWnd;
} WSAASYNCGETSERVBYNAME16;
typedef WSAASYNCGETSERVBYNAME16 UNALIGNED *PWSAASYNCGETSERVBYNAME16;

typedef struct _WSACANCELASYNCREQUEST16 {              /* ws108 */
    WORD hAsyncTaskHandle;
} WSACANCELASYNCREQUEST16;
typedef WSACANCELASYNCREQUEST16 UNALIGNED *PWSACANCELASYNCREQUEST16;

typedef struct _WSASETBLOCKINGHOOK16 {                 /* ws109 */
    VPWNDPROC lpBlockFunc;
} WSASETBLOCKINGHOOK16;
typedef WSASETBLOCKINGHOOK16 UNALIGNED *PWSASETBLOCKINGHOOK16;

typedef struct _WSASETLASTERROR16 {                    /* ws112 */
    WORD Error;
} WSASETLASTERROR16;
typedef WSASETLASTERROR16 UNALIGNED *PWSASETLASTERROR16;

typedef struct _WSASTARTUP16 {                         /* ws115 */
    VPWSADATA16 lpWSAData;
    WORD wVersionRequired;
} WSASTARTUP16;
typedef WSASTARTUP16 UNALIGNED *PWSASTARTUP16;

typedef struct ___WSAFDISSET16 {                       /* ws151 */
    VPFD_SET16 Set;
    HSOCKET16 hSocket;
} __WSAFDISSET16;
typedef __WSAFDISSET16 UNALIGNED *P__WSAFDISSET16;

/* XLATOFF */
#pragma pack()
/* XLATON */

#define FUN___WSAFDISSET                   151 //

