/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    resolver.h

Abstract:

    Definitions etc. for Berkeley resolver

Author:

    Richard L Firth (rfirth) 15-Jun-1996

Revision History:

    15-Jun-1996 rfirth
        Created

--*/

//
// includes
//

#include <nameser.h>
#include <resolv.h>

//
// manifests
//

#if PACKETSZ > 1024
#define MAXPACKET       PACKETSZ
#else
#define MAXPACKET       1024
#endif

#define DBG_RESOLVER    DBG_SOCKETS
#define DBG_VXD_IO      DBG_SOCKETS

#define DLL_PRINT(x)    DEBUG_PRINT(SOCKETS, INFO, x)
#define WS_PRINT(x)     DEBUG_PRINT(SOCKETS, INFO, x)
#define WS_ASSERT       INET_ASSERT
#define DLL_ASSERT      INET_ASSERT

//
// macros
//
#ifndef unix
#define bcopy(s, d, c)  memcpy((u_char *)(d), (u_char *)(s), (c))
#define bzero(d, l)     memset((d), '\0', (l))
#endif /* unix */
#define bcmp(s1, s2, l) memcmp((s1), (s2), (l))

#define IS_DGRAM_SOCK(type)  (((type) == SOCK_DGRAM) || ((type) == SOCK_RAW))

//
// types
//

typedef union {
    HEADER hdr;
    unsigned char buf[MAXPACKET];
} querybuf;

typedef union {
    long al;
    char ac;
} align;

//extern char VTCPPARM[];
//extern char NTCPPARM[];
//extern char TCPPARM[];
//extern char TTCPPARM[];

#ifndef unix
typedef long                   daddr_t;
typedef char FAR *             caddr_t;
struct iovec {
    caddr_t iov_base;
    int     iov_len;
};

struct uio {
    struct  iovec *uio_iov;
    int     uio_iovcnt;
    int     uio_offset;
    int     uio_segflg;
    int     uio_resid;
};

enum    uio_rw { UIO_READ, UIO_WRITE };
#endif /* unix */
/*
 * Segment flag values (should be enum).
 */
#define UIO_USERSPACE   0               /* from user data space */
#define UIO_SYSSPACE    1               /* from system space */
#define UIO_USERISPACE  2               /* from user I space */

#define MAXALIASES      35
#define MAXADDRS        35

#define HOSTDB_SIZE     (_MAX_PATH + 7)   // 7 == strlen("\\hosts") + 1
#define PROTODB_SIZE    (_MAX_PATH + 10)
#define SERVDB_SIZE     (_MAX_PATH + 10)

typedef struct _WINSOCK_TLS_DATA {
    char * GETHOST_h_addr_ptrs[MAXADDRS + 1];
    struct hostent GETHOST_host;
    char * GETHOST_host_aliases[MAXALIASES];
    char GETHOST_hostbuf[BUFSIZ + 1];
    //struct in_addr GETHOST_host_addr;
    //char GETHOST_HOSTDB[HOSTDB_SIZE];
    //FILE *GETHOST_hostf;
    //char GETHOST_hostaddr[MAXADDRS];
    //char *GETHOST_host_addrs[2];
    //int GETHOST_stayopen;
    //char GETPROTO_PROTODB[PROTODB_SIZE];
    //FILE *GETPROTO_protof;
    //char GETPROTO_line[BUFSIZ+1];
    //struct protoent GETPROTO_proto;
    //char *GETPROTO_proto_aliases[MAXALIASES];
    //int GETPROTO_stayopen;
    //char GETSERV_SERVDB[SERVDB_SIZE];
    //FILE *GETSERV_servf;
    //char GETSERV_line[BUFSIZ+1];
    //struct servent GETSERV_serv;
    //char *GETSERV_serv_aliases[MAXALIASES];
    //int GETSERV_stayopen;
    struct state R_INIT__res;
    //char INTOA_Buffer[18];
    //CSOCKET * DnrSocketHandle;
    //BOOLEAN IsBlocking;
    BOOLEAN IoCancelled;
    //BOOLEAN ProcessingGetXByY;
    BOOLEAN GetXByYCancelled;
    //BOOLEAN EnableWinsNameResolution;
    //BOOLEAN DisableWinsNameResolution;
    //SOCKET SocketHandle;
    //PBLOCKING_HOOK BlockingHook;
    //HANDLE EventHandle;
    //ULONG CreateOptions;
    INT DnrErrorCode;
//#if DBG
//    ULONG IndentLevel;
//#endif
} WINSOCK_TLS_DATA, * LPWINSOCK_TLS_DATA;

//extern DWORD SockTlsSlot;

//#define ACCESS_THREAD_DATA(a, file) \
//            (((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))-> \
//                ## file ## _ ## a )

#define ACCESS_THREAD_DATA(a, file) (lpResolverInfo->file ## _ ## a)

#define _h_addr_ptrs    ACCESS_THREAD_DATA(h_addr_ptrs, GETHOST)
#define _host           ACCESS_THREAD_DATA(host, GETHOST)
#define _host_aliases   ACCESS_THREAD_DATA(host_aliases, GETHOST)
#define _hostbuf        ACCESS_THREAD_DATA(hostbuf, GETHOST)
#define _host_addr      ACCESS_THREAD_DATA(host_addr, GETHOST)
//#define HOSTDB          ACCESS_THREAD_DATA(HOSTDB, GETHOST)
//#define hostf           ACCESS_THREAD_DATA(hostf, GETHOST)
//#define hostaddr        ACCESS_THREAD_DATA(hostaddr, GETHOST)
//#define host_addrs      ACCESS_THREAD_DATA(host_addrs, GETHOST)
//#define stayopen        ACCESS_THREAD_DATA(stayopen, GETHOST)
#define _res            ACCESS_THREAD_DATA( _res, R_INIT )

//#define SockThreadProcessingGetXByY lpResolverInfo->ProcessingGetXByY
#define SockThreadGetXByYCancelled  lpResolverInfo->GetXByYCancelled
#define SockDnrSocket               lpResolverInfo->DnrSocketHandle
#define SockThreadDnrErrorCode      lpResolverInfo->DnrErrorCode

//#define SockThreadIsBlocking \
//            ( ((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->IsBlocking )
//
//#define SockThreadIoCancelled \
//            ( ((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->IoCancelled )
//
//#define SockThreadProcessingGetXByY \
//            ( ((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->ProcessingGetXByY )
//
//#define SockThreadGetXByYCancelled \
//            ( ((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->GetXByYCancelled )
//
//#define SockThreadSocketHandle \
//            ( ((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->SocketHandle )
//
//#define SockThreadBlockingHook \
//            ( ((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->BlockingHook )
//
//#define SockThreadEvent \
//            ( ((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->EventHandle )
//
//#define SockDnrSocket \
//            ( ((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->DnrSocketHandle )
//
//#define SockEnableWinsNameResolution \
//            ( ((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->EnableWinsNameResolution )
//
//#define SockDisableWinsNameResolution \
//            ( ((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->DisableWinsNameResolution )
//
//#define SockCreateOptions \
//            ( ((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->CreateOptions )
//
//#define SockThreadDnrErrorCode \
//            ( ((LPWINSOCK_TLS_DATA)TlsGetValue( SockTlsSlot ))->DnrErrorCode )

#define LPSOCK_THREAD   LPWINSOCK_TLS_DATA
#define GET_THREAD_DATA(p) p = InternetGetResolverInfo()
//#define I_SetLastError  SetLastError
#define DllAllocMem(n)  ALLOCATE_MEMORY(LMEM_FIXED, n)
#define DllFreeMem      FREE_MEMORY

//
// well-known DHCP VxD ID (from netvxd.h)
//

#define VDHCP_Device_ID     0x049A

//
// prototypes
//

int
dn_expand(
    IN  unsigned char *msg,
    IN  unsigned char *eomorig,
    IN  unsigned char *comp_dn,
    OUT unsigned char *exp_dn,
    IN  int            length
    );

static
int
dn_find(
    unsigned char  *exp_dn,
    unsigned char  *msg,
    unsigned char **dnptrs,
    unsigned char **lastdnptr
    );

int
dn_skipname(
    unsigned char *comp_dn,
    unsigned char *eom
    );

void
fp_query(
    char *msg,
    FILE *file
    );

//int
//gethostname(
//    OUT char *name,
//    IN int namelen
//    );

void
p_query(
    char *msg
    );

extern
void
putshort(
    u_short s,
    u_char *msgp
    );

void
putlong(
    u_long l,
    u_char *msgp
    );

void
_res_close(
    void
    );

//DWORD
//sendv(
//    CSOCKET *      s,           /* socket descriptor */
//    struct iovec  *iov,         /* array of vectors */
//    int            iovcnt       /* size of array */
//    );

//int
//strcasecmp(
//    char *s1,
//    char *s2
//    );
//
//int
//strncasecmp(
//    char *s1,
//    char *s2,
//    int   n
//    );
//
//struct hostent *
//myhostent (
//    void
//    );
//
//struct hostent *
//localhostent (
//    void
//    );
//
//struct hostent *
//dnshostent (
//    void
//    );
//
//BOOL
//querydnsaddrs (
//    IN LPDWORD *Array,
//    IN PVOID Buffer
//    );
//
//DWORD
//BytesInHostent (
//    PHOSTENT Hostent
//    );
//
//DWORD
//CopyHostentToBuffer (
//    char FAR *Buffer,
//    int BufferLength,
//    PHOSTENT Hostent
//    );
//
//struct hostent *
//_gethtbyname (
//    IN char *name
//    );

BOOL
OkToUseInternetAsyncGetHostByName(
    VOID
    );

LPHOSTENT
InternetAsyncGetHostByName(
    IN LPSTR lpszHostName,
    OUT LPDWORD lpdwTtl
    );

LPWINSOCK_TLS_DATA
InternetGetResolverInfo(
    VOID
    );

LPHOSTENT
getanswer(
    OUT querybuf *answer,
    OUT int      *ttl,
    IN int       anslen,
    IN int       iquery
    );

//ULONG
//SockNbtResolveName (
//    IN PCHAR Name
//    );

//PHOSTENT
//QueryHostentCache (
//    IN LPSTR Name OPTIONAL,
//    IN DWORD IpAddress OPTIONAL
//    );

//FILE *
//SockOpenNetworkDataBase(
//    IN  char *Database,
//    OUT char *Pathname,
//    IN  int   PathnameLen,
//    IN  char *OpenFlags
//    );
//
 

