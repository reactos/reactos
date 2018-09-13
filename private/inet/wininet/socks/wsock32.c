/*++

Copyright (c) 1997  Microsoft Corporation
Copyright (c) 1996  Hummingbird Corporation of Canada


Module Name:

    wsock32.c

Abstract:

    Contains Socks V4 support, written by Hummingbird corporation.  Licensed from
    Hummingbird for ulimited use by Microsoft.  Ported to WININET code base.

    Contents:
        FindSocket
        closesocket
        connect
        getpeername
        ALL WSOCK32.DLL exports.

Author:

    Arthur L Bierer (arthurbi) 13-Dec-1996

Environment:

    Win32 user-mode DLL

Revision History:

    13-Dec-1996 arthurbi
        Created, removed flagrent calls to CRTs, and unchecked memory allocations.

    29-Aug-1997 rfirth
        Further reduced from general-purpose SOCKS implementation to Wininet-
        specific SOCKS support

--*/


#define _WINSOCKAPI_
#include <windows.h>
#ifdef DO_FILE_CONFIG
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <malloc.h>
#include <ctype.h>
#include <stdlib.h>
#endif

#ifdef OLD_SOCKS

struct User {
    char *Name;
    struct User *Next;
};

struct Server {
    char *Name;
    struct Server *Next;
    unsigned short port;
};

struct Hosts {
    struct User *Users;
    struct Server *Servers;
    unsigned long dst;
    unsigned long mask;
    unsigned short port;
    unsigned char op;
    unsigned char type;
    struct Hosts *Next;
} *Head=NULL;

struct Sockets {
    int s;
    HWND hWnd;
    unsigned int wMsg;
    long lEvent;
    unsigned long Blocking;
    int type;
    unsigned long ip;
    unsigned short port;
    struct Sockets *Next;
    struct Sockets *Last;
    int Socked:1;
} *SHead=NULL;

HANDLE SMutex;

#define CREATE_MUTEX()  SMutex = CreateMutex(NULL, FALSE, NULL)
#define DELETE_MUTEX()  if (SMutex) CloseHandle(SMutex)
#define ENTER_MUTEX()   WaitForSingleObject(SMutex, INFINITE)
#define LEAVE_MUTEX()   ReleaseMutex(SMutex)

#else

struct Hosts {
    char * user;
    int userlen;
    unsigned long ip;
    unsigned short port;
} *Head = NULL;

struct Sockets {
    int s;
    int type;
    unsigned long ip;
    unsigned short port;
    struct Sockets * Next;
    struct Sockets * Last;
    int Socked : 1;
    int Blocking : 1;
} *SHead = NULL;

CRITICAL_SECTION    CritSec;

#define CREATE_MUTEX()  InitializeCriticalSection(&CritSec)
#define DELETE_MUTEX()  DeleteCriticalSection(&CritSec)
#define ENTER_MUTEX()   EnterCriticalSection(&CritSec)
#define LEAVE_MUTEX()   LeaveCriticalSection(&CritSec)

#endif

#define DENY    1
#define DIRECT  2
#define SOCKD   3

#define ANY 0
#define EQ  1
#define NEQ 2
#define LT  3
#define GT  4
#define LE  5
#define GE  6


/*
 * Internet address (old style... should be updated)
 */
struct in_addr {
        union {
                struct { unsigned char s_b1,s_b2,s_b3,s_b4; } S_un_b;
                struct { unsigned short s_w1,s_w2; } S_un_w;
                unsigned long S_addr;
        } S_un;
#define s_addr  S_un.S_addr
                                /* can be used for most tcp & ip code */
#define s_host  S_un.S_un_b.s_b2
                                /* host on imp */
#define s_net   S_un.S_un_b.s_b1
                                /* network */
#define s_imp   S_un.S_un_w.s_w2
                                /* imp */
#define s_impno S_un.S_un_b.s_b4
                                /* imp # */
#define s_lh    S_un.S_un_b.s_b3
                                /* logical host */
};

/*
 * Socket address, internet style.
 */
struct sockaddr_in {
        short   sin_family;
        unsigned short  sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8];
};

struct  servent {
        char    * s_name;           /* official service name */
        char    * * s_aliases;      /* alias list */
        short   s_port;             /* port # */
        char    * s_proto;          /* protocol to use */
};

struct  hostent {
        char    * h_name;           /* official name of host */
        char    * * h_aliases;      /* alias list */
        short   h_addrtype;         /* host address type */
        short   h_length;           /* length of address */
        char    * * h_addr_list;    /* list of addresses */
#define h_addr  h_addr_list[0]      /* address, for backward compat */
};


#define WSABASEERR              10000
#define WSAECONNREFUSED         (WSABASEERR+61)
#define WSAEWOULDBLOCK          (WSABASEERR+35)
#define WSAENOBUFS              (WSABASEERR+55)


#define SOCKET_ERROR            (-1)
#define INVALID_SOCKET  (int)(~0)

/*
 * Define flags to be used with the WSAAsyncSelect() call.
 */
#define FD_READ         0x01
#define FD_WRITE        0x02
#define FD_OOB          0x04
#define FD_ACCEPT       0x08
#define FD_CONNECT      0x10
#define FD_CLOSE        0x20

#define SOCK_STREAM     1               /* stream socket */

/*
 * Commands for ioctlsocket(),  taken from the BSD file fcntl.h.
 *
 *
 * Ioctl's have the command encoded in the lower word,
 * and the size of any in or out parameters in the upper
 * word.  The high 2 bits of the upper word are used
 * to encode the in/out status of the parameter; for now
 * we restrict parameters to at most 128 bytes.
 */
#define IOCPARM_MASK    0x7f            /* parameters must be < 128 bytes */
#define IOC_VOID        0x20000000      /* no parameters */
#define IOC_OUT         0x40000000      /* copy out parameters */
#define IOC_IN          0x80000000      /* copy in parameters */
#define IOC_INOUT       (IOC_IN|IOC_OUT)
                                        /* 0x20000000 distinguishes new &
                                           old ioctl's */
#define _IO(x,y)        (IOC_VOID|((x)<<8)|(y))

#define _IOR(x,y,t)     (IOC_OUT|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))

#define _IOW(x,y,t)     (IOC_IN|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))

#define FIONREAD    _IOR('f', 127, unsigned long) /* get # bytes to read */
#define FIONBIO     _IOW('f', 126, unsigned long) /* set/clear non-blocking i/o */
#define FIOASYNC    _IOW('f', 125, unsigned long) /* set/clear async i/o */

#define SO_SET_SOCKS_FIREWALL   0xF0000


DWORD (WINAPI * VArecv)(int a,int b,int c, int d);
DWORD (WINAPI * VAsend)(int a,int b,int c, int d);
DWORD (WINAPI * VEnumProtocolsA)(int a,int b,int c);
DWORD (WINAPI * VEnumProtocolsW)(int a,int b,int c);
DWORD (WINAPI * VGetAddressByNameA)(int a,int b,int c,int d,int e,int f,int g,int h,int i,int j);
DWORD (WINAPI * VGetAddressByNameW)(int a,int b,int c,int d,int e,int f,int g,int h,int i,int j);
DWORD (WINAPI * VGetNameByTypeA)(int a,int b,int c);
DWORD (WINAPI * VGetNameByTypeW)(int a,int b,int c);
DWORD (WINAPI * VGetServiceA)(int a,int b,int c,int d,int e,int f,int g);
DWORD (WINAPI * VGetServiceW)(int a,int b,int c,int d,int e,int f,int g);
DWORD (WINAPI * VGetTypeByNameA)(int a,int b);
DWORD (WINAPI * VGetTypeByNameW)(int a,int b);
DWORD (WINAPI * VNPLoadNameSpaces)(int a,int b,int c);
DWORD (WINAPI * VSetServiceA)(int a,int b,int c,int d,int e,int f);
DWORD (WINAPI * VSetServiceW)(int a,int b,int c,int d,int e,int f);
DWORD (WINAPI * VTransmitFile)(int a,int b,int c,int d,int e,int f,int g);
DWORD (WINAPI * VWSAAsyncGetHostByAddr)(int a,int b,int c,int d,int e,int f,int g);
DWORD (WINAPI * VWSAAsyncGetHostByName)(int a,int b,int c,int d,int e);
DWORD (WINAPI * VWSAAsyncGetProtoByName)(int a,int b,int c,int d,int e);
DWORD (WINAPI * VWSAAsyncGetProtoByNumber)(int a,int b,int c,int d,int e);
DWORD (WINAPI * VWSAAsyncGetServByName)(int a,int b,int c,int d,int e,int f);
DWORD (WINAPI * VWSAAsyncGetServByPort)(int a,int b,int c,int d,int e,int f);
DWORD (WINAPI * VWSAAsyncSelect)(int s, HWND hWnd, unsigned int wMsg, long lEvent);
DWORD (WINAPI * VWSACancelAsyncRequest)(int a);
DWORD (WINAPI * VWSACancelBlockingCall)(void);
DWORD (WINAPI * VWSACleanup)(void);
DWORD (WINAPI * VWSAGetLastError)(void);
DWORD (WINAPI * VWSAIsBlocking)(void);
DWORD (WINAPI * VWSARecvEx)(int a,int b,int c,int d);
DWORD (WINAPI * VWSASetBlockingHook)(int a);
DWORD (WINAPI * VWSASetLastError)(int a);
DWORD (WINAPI * VWSAStartup)(int a,int b);
DWORD (WINAPI * VWSAUnhookBlockingHook)(void);
DWORD ( * VWSHEnumProtocols)(int a,int b,int c,int d);
DWORD (WINAPI * VWsControl)(int a,int b,int c,int d,int e,int f);
DWORD (WINAPI * V__WSAFDIsSet)(int a,int b);
DWORD (WINAPI * Vaccept)(int a,int b,int c);
DWORD (WINAPI * Vbind)(int a,int b,int c);
DWORD (WINAPI * Vclosesocket)(int a);
DWORD (WINAPI * Vclosesockinfo)(int a);
DWORD (WINAPI * Vconnect)(int s, const struct sockaddr_in FAR *name, int namelen);
DWORD (WINAPI * Vdn_expand)(int a,int b,int c,int d,int e);
DWORD (WINAPI * Vgethostbyaddr)(int a,int b,int c);
struct hostent * (WINAPI * Vgethostbyname)(char *);
DWORD (WINAPI * Vgethostname)(int a,int b);
DWORD (WINAPI * Vgetnetbyname)(int a);
DWORD (WINAPI * Vgetpeername)(int s, struct sockaddr_in *name,int *namelen);
DWORD (WINAPI * Vgetprotobyname)(int a);
DWORD (WINAPI * Vgetprotobynumber)(int a);
struct servent * (WINAPI * Vgetservbyname)(const char FAR * name, const char FAR * proto);
DWORD (WINAPI * Vgetservbyport)(int a,int b);
DWORD (WINAPI * Vgetsockname)(int a,int b,int c);
DWORD (WINAPI * Vgetsockopt)(int a,int b,int c,int d,int e);
DWORD (WINAPI * Vhtonl)(int a);
DWORD (WINAPI * Vhtons)(int a);
DWORD (WINAPI * Vinet_addr)(char *p);
DWORD (WINAPI * Vinet_network)(int a);
DWORD (WINAPI * Vinet_ntoa)(int a);
DWORD (WINAPI * Vioctlsocket)(int s, long cmd, unsigned long *argp);
DWORD (WINAPI * Vlisten)(int a,int b);
DWORD (WINAPI * Vntohl)(int a);
DWORD (WINAPI * Vntohs)(int a);
DWORD (WINAPI * Vrcmd)(int a,int b,int c,int d,int e,int f);
DWORD (WINAPI * Vrecv)(int a,int b,int c,int d);
DWORD (WINAPI * Vrecvfrom)(int a,int b,int c,int d,int e,int f);
DWORD (WINAPI * Vrexec)(int a,int b,int c,int d,int e,int f);
DWORD (WINAPI * Vrresvport)(int a);
DWORD (WINAPI * Vs_perror)(int a,int b);
DWORD (WINAPI * Vselect)(int a,int b,int c,int d,int e);
DWORD (WINAPI * Vsend)(int a,int b,int c,int d);
DWORD (WINAPI * Vsendto)(int a,int b,int c,int d,int e,int f);
DWORD (WINAPI * Vsethostname)(int a,int b);
DWORD (WINAPI * Vsetsockopt)(int s,int level,int optname,const char FAR * optval, int optlen);
DWORD (WINAPI * Vshutdown)(int a,int b);
DWORD (WINAPI * Vsocket)(int a,int b,int c);
DWORD (WINAPI * VWEP)(void);
DWORD (WINAPI * VAcceptEx)(int a,int b,int c,int d,int e,int f,int g,int h);
DWORD (WINAPI * VGetAcceptExSockaddrs)(int a,int b,int c,int d,int e,int f,int g,int h);
DWORD (WINAPI * VMigrateWinsockConfiguration)(int a,int b, int c);
DWORD (WINAPI * VWSApSetPostRoutine)(void *a);

BOOL (WINAPI * VPostMessage)(HWND hWnd, unsigned int wMsg, WPARAM wPAram, LPARAM lParam) = NULL;

BOOL MyPostMessage(HWND hWnd, unsigned int wMsg, WPARAM wParam, LPARAM lParam) {
    if ( VPostMessage)
        return(VPostMessage(hWnd,wMsg, wParam, lParam));
    PostMessage(hWnd,wMsg, wParam, lParam);
}

struct Sockets *
FindSocket(
    int s
    )

/*++

Routine Description:

    Find or create SOCKS socket object. Returns with MUTEX held

Arguments:

    s   - associated socket handle

Return Value:

    struct Sockets *
        Success - address of Sockets object

        Failure - NULL

--*/

{
    struct Sockets *So;

    ENTER_MUTEX();

    So = SHead;
    while (So) {
        if (s == So->s) {
            return So;
        }
        So = So->Next;
    }
    if (So = (struct Sockets *)LocalAlloc(LPTR, sizeof(struct Sockets))) {
        So->s = s;
        So->Next = SHead;
        SHead = So;
        if (So->Next) {
            (So->Next)->Last = So;
        }
    }
    return So;
}

//LPSTR
//NewString(
//    IN LPCSTR String
//    )
//
///*++
//
//Routine Description:
//
//    kind of version of strdup() but using LocalAlloc to allocate memory
//
//Arguments:
//
//    String  - pointer to string to make copy of
//
//Return Value:
//
//    LPSTR
//        Success - pointer to duplicated string
//        Failure - NULL
//
//--*/
//
//{
//    int len = strlen(String) + 1;
//    LPSTR string;
//
//    if (string = (LPSTR)LocalAlloc(LMEM_FIXED, len)) {
//        CopyMemory(string, String, len);
//    }
//    return string;
//}


DWORD WINAPI __WSAFDIsSet(int a,int b) {
    return(V__WSAFDIsSet(a, b));
}

DWORD WINAPI accept(int a,int b,int c) {
    return(Vaccept(a, b, c));
}

DWORD WINAPI Arecv(int a,int b,int c,int d) {
    return(VArecv(a,b,c,d));
}

DWORD WINAPI Asend(int a,int b,int c,int d) {

    return(VAsend(a,b,c,d));
}

DWORD WINAPI bind(int a,int b,int c) {
    return(Vbind(a, b, c));
}

DWORD WINAPI AcceptEx(int a,int b,int c,int d,int e,int f,int g,int h) {
    return(VAcceptEx(a,b,c,d,e,f,g,h));
}

DWORD WINAPI GetAcceptExSockaddrs(int a,int b,int c,int d,int e,int f,int g,int h) {
    return(VGetAcceptExSockaddrs(a,b,c,d,e,f,g,h));
}

DWORD WINAPI MigrateWinsockConfiguration(int a,int b, int c) {
    return(VMigrateWinsockConfiguration(a,b,c));
}

DWORD WINAPI WSApSetPostRoutine(void *a) {
    VPostMessage=a;
    return(VWSApSetPostRoutine(a));
}

DWORD
WINAPI
closesocket(
    int s
    )

/*++

Routine Description:

    Closes socket handle and destroys associated Sockets object if found

Arguments:

    s   - socket handle

Return Value:

    int
        Success - 0

        Failure - -1

--*/

{
    struct Sockets * So = FindSocket(s);

    if (So == NULL) {
        VWSASetLastError(WSAENOBUFS);

        LEAVE_MUTEX();

        return SOCKET_ERROR;
    }
    if (So->Last == NULL) {
        SHead = So->Next;
    } else {
        (So->Last)->Next = So->Next;
    }
    if (So->Next) {
        (So->Next)->Last = So->Last;
    }

    LEAVE_MUTEX();

    LocalFree(So);
    return Vclosesocket(s);
}

DWORD WINAPI closesockinfo(int a) {
    return(Vclosesockinfo(a));
}

DWORD
WINAPI
connect(
    int s,
    const struct sockaddr_in FAR * name,
    int namelen
    )

/*++

Routine Description:

    Connect to remote host via SOCKS proxy. Modified from original. If we are
    here then we are going specifically via a known SOCKS proxy. There is now
    only one Hosts object, containing a single SOCKD socks proxy address and
    user name

Arguments:

    s       - socket to connect

    name    - sockaddr of remote host

    namelen - length of sockaddr

Return Value:

    int
        Success - 0

        Failure - -1

--*/

{
    unsigned long ip;
    unsigned short port;
    struct Hosts * pHost;
    int serr;
    int blocking;
    struct Sockets * pSocket;
    struct sockaddr_in sin;
    struct {
        unsigned char VN;
        unsigned char CD;
        unsigned short DSTPORT;
        unsigned long  DSTIP;
        char UserId[255];
    } request;
    int length;
    char response[256];
    int val;

    //
    // get IP address and port we want to connect to on other side of firewall
    //

    port = name->sin_port;
    ip = name->sin_addr.s_addr;

    //
    // initialize sockaddr for connecting to SOCKS firewall
    //

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = 2;

    //
    // initialize SOCKS request packet
    //

    request.VN = 4;
    request.CD = 1;
    request.DSTPORT = port;
    request.DSTIP = ip;

    pSocket = FindSocket(s);
    if (pSocket == NULL) {
        VWSASetLastError(WSAENOBUFS);

        LEAVE_MUTEX();

        return SOCKET_ERROR;
    }
    pHost = Head;
    if (!pHost || (pSocket->type != SOCK_STREAM) || (pSocket->Socked)) {

        LEAVE_MUTEX();

        return Vconnect(s, name, namelen);
    }

    //
    // get information from pSocket and pHost structures before releasing mutex
    //

    blocking = pSocket->Blocking;
    pSocket->port = port;
    pSocket->ip = ip;
    memcpy(request.UserId, pHost->user, pHost->userlen);
    length = pHost->userlen + 8; // 8 == sizeof fixed portion of request
    sin.sin_port = pHost->port;
    sin.sin_addr.s_addr = pHost->ip;

    //
    // from this point, we cannot touch pHost or pSocket until we take the mutex
    // again
    //

    LEAVE_MUTEX();

    //
    // put socket into blocking mode
    //

    val = 0;
    Vioctlsocket(s, FIONBIO, &val);

    //
    // communicate with SOCKS firewall: send SOCKS request & receive response
    //

    serr = Vconnect(s, &sin, sizeof(sin));
    if (serr != SOCKET_ERROR) {
        serr = Vsend(s, (int)&request, length, 0);
        if (serr == length) {
            serr = Vrecv(s, (int)response, sizeof(response), 0);
        }
    }

    //
    // if originally non-blocking, make socket non-blocking again
    //

    if (blocking) {
        Vioctlsocket(s, FIONBIO, &blocking);
    }

    //
    // if success, mark the socket as being connected through firewall
    //

    if ((serr == SOCKET_ERROR) || (response[1] != 90)) {
        VWSASetLastError(WSAECONNREFUSED);
        serr = SOCKET_ERROR;
    } else {

        //
        // if we can't find/crea
        //

        pSocket = FindSocket(s);
        if (pSocket) {
            pSocket->Socked = 1;
            serr = 0;
        } else {
            VWSASetLastError(WSAENOBUFS);
            serr = SOCKET_ERROR;
        }

        LEAVE_MUTEX();

    }
    return serr;
}

DWORD WINAPI dn_expand(int a,int b,int c,int d,int e) {
    return(Vdn_expand(a, b, c, d, e));
}

DWORD WINAPI EnumProtocolsA(int a,int b,int c) {
    return(VEnumProtocolsA(a, b, c));
}

DWORD WINAPI EnumProtocolsW(int a,int b,int c) {
    return(VEnumProtocolsW(a, b, c));
}

DWORD WINAPI GetAddressByNameA(int a,int b,int c,int d,int e,int f,int g,int h,int i,int j) {
    return(VGetAddressByNameA(a, b, c, d, e, f, g, h, i, j));
}

DWORD WINAPI GetAddressByNameW(int a,int b,int c,int d,int e,int f,int g,int h,int i,int j) {
    return(VGetAddressByNameW(a, b, c, d, e, f, g, h, i, j));
}

DWORD WINAPI gethostbyaddr(int a,int b,int c) {
    return(Vgethostbyaddr(a, b, c));
}

struct hostent FAR * WINAPI gethostbyname(char *a) {
    return(Vgethostbyname(a));
}

DWORD WINAPI gethostname(int a,int b) {
    return(Vgethostname(a, b));
}

DWORD WINAPI GetNameByTypeA(int a,int b,int c) {
    return(VGetNameByTypeA(a, b, c));
}

DWORD WINAPI GetNameByTypeW(int a,int b,int c) {
    return(VGetNameByTypeW(a, b, c));
}

DWORD WINAPI getnetbyname(int a) {
    return(Vgetnetbyname(a));
}

DWORD
WINAPI
getpeername(
    int s,
    struct sockaddr_in * name,
    int *namelen
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    s       -

    name    -

    namelen -

Return Value:

    int

--*/

{
    DWORD ret;
    struct Sockets *So;

    ret = Vgetpeername(s, name, namelen);
    if (ret == 0) {
        So = FindSocket(s);
        if (So) {
            if (So->Socked) {
                if (*namelen >= sizeof(struct sockaddr_in)) {
                    name->sin_port = So->port;
                    name->sin_addr.s_addr = So->ip;
                }
            }
        } else {
            VWSASetLastError(WSAENOBUFS);
            ret = SOCKET_ERROR;
        }

        LEAVE_MUTEX();
    }
    return ret;
}

DWORD WINAPI getprotobyname(int a) {
    return(Vgetprotobyname(a));
}

DWORD WINAPI getprotobynumber(int a) {
    return(Vgetprotobynumber(a));
}

struct servent * WINAPI getservbyname(const char FAR * name, const char FAR * proto) {
    return(Vgetservbyname(name, proto));
}

DWORD WINAPI getservbyport(int a,int b) {
    return(Vgetservbyport(a, b));
}

DWORD WINAPI GetServiceA(int a,int b,int c,int d,int e,int f,int g) {
    return(VGetServiceA(a, b, c, d, e, f, g));
}

DWORD WINAPI GetServiceW(int a,int b,int c,int d,int e,int f,int g) {
    return(VGetServiceW(a, b, c, d, e, f, g));
}

DWORD WINAPI getsockname(int a,int b,int c) {
    return(Vgetsockname(a, b, c));
}

DWORD WINAPI getsockopt(int a,int b,int c,int d,int e) {
    return(Vgetsockopt(a, b, c, d, e));
}

DWORD WINAPI GetTypeByNameA(int a,int b) {
    return(VGetTypeByNameA(a, b));
}

DWORD WINAPI GetTypeByNameW(int a,int b) {
    return(VGetTypeByNameW(a, b));
}

DWORD WINAPI htonl(int a) {
    return(Vhtonl(a));
}

DWORD WINAPI htons(int a) {
    return(Vhtons(a));
}

DWORD WINAPI inet_addr(char *p) {
    return(Vinet_addr(p));
}

DWORD WINAPI inet_network(int a) {
    return(Vinet_network(a));
}

DWORD WINAPI inet_ntoa(int a) {
    return(Vinet_ntoa(a));
}

DWORD WINAPI ioctlsocket(int s, long cmd, unsigned long *argp) {
    if (cmd == FIONBIO) {

        struct Sockets * So = FindSocket(s);

        if (So == NULL) {
            VWSASetLastError(WSAENOBUFS);

            LEAVE_MUTEX();

            return SOCKET_ERROR;
        }
        So->Blocking = *argp ? 1 : 0;

        LEAVE_MUTEX();

    }
    return Vioctlsocket(s, cmd, argp);
}

DWORD WINAPI listen(int a,int b) {
    return(Vlisten(a, b));
}

DWORD WINAPI NPLoadNameSpaces(int a,int b,int c) {
    return(VNPLoadNameSpaces(a, b, c));
}

DWORD WINAPI ntohl(int a) {
    return(Vntohl(a));
}

DWORD WINAPI ntohs(int a) {
    return(Vntohs(a));
}

DWORD WINAPI rcmd(int a,int b,int c,int d,int e,int f) {
    return(Vrcmd(a, b, c, d, e, f));
}

DWORD WINAPI recv(int a,int b,int c,int d) {
    return(Vrecv(a, b, c, d));
}

DWORD WINAPI recvfrom(int a,int b,int c,int d,int e,int f) {
    return(Vrecvfrom(a, b, c, d, e, f));
}

DWORD WINAPI rexec(int a,int b,int c,int d,int e,int f) {
    return(Vrexec(a, b, c, d, e, f));
}

DWORD WINAPI rresvport(int a) {
    return(Vrresvport(a));
}

DWORD WINAPI s_perror(int a,int b) {
    return(Vs_perror(a, b));
}

DWORD WINAPI select(int a,int b,int c,int d,int e) {
    return(Vselect(a, b, c, d, e));
}

DWORD WINAPI send(int a,int b,int c,int d) {
    return(Vsend(a, b, c, d));
}

DWORD WINAPI sendto(int a,int b,int c,int d,int e,int f) {
    return(Vsendto(a, b, c, d, e, f));
}

DWORD WINAPI sethostname(int a,int b) {
    return(Vsethostname(a, b));
}

DWORD WINAPI SetServiceA(int a,int b,int c,int d,int e,int f) {
    return(VSetServiceA(a, b, c, d, e, f));
}

DWORD WINAPI SetServiceW(int a,int b,int c,int d,int e,int f) {
    return(VSetServiceW(a, b, c, d, e, f));
}

DWORD
WINAPI
setsockopt(
    int s,
    int level,
    int optname,
    const char FAR * optval,
    int optlen
    )

/*++

Routine Description:

    If SO_SET_SOCKS_FIREWALL, create SOCKS information if it is new or changed
    from current, else pass on the request to wsock32!setsockopt()

Arguments:

    s       - socket on which to set option

    level   - option type parameter (SO_SET_SOCKS_FIREWALL)

    optname - option type sub-parameter (SOCKS firewall port # in host format)

    optval  - value to set (pointer to SOCKS information:
                DWORD ip address;
                LPSTR username
              )

    optlen  - length of value (8)

Return Value:

    DWORD
        Success - 0

        Failure - -1

--*/

{
    int rc;

    if (level != SO_SET_SOCKS_FIREWALL) {
        rc = Vsetsockopt(s, level, optname, optval, optlen);
    } else {

        struct Hosts * pHost;
        struct FirewallInfo {
            DWORD ipAddress;
            LPSTR userName;
        } * pInfo = (struct FirewallInfo *)optval;

        optname = Vhtons(optname);

        ENTER_MUTEX();

        if (pHost = Head) {
            if ((pHost->ip != pInfo->ipAddress)
            || (pHost->port != optname)
            || (pHost->user && lstrcmp(pHost->user, pInfo->userName))) {
//char buf[256];
//wsprintf(buf,
//         "throwing out: host: %d.%d.%d.%d:%d,%s; info: %d.%d.%d.%d:%d,%s\n",
//         pHost->ip & 0xff,
//         (pHost->ip >> 8) & 0xff,
//         (pHost->ip >> 16) & 0xff,
//         (pHost->ip >> 24) & 0xff,
//         Vhtons(pHost->port) & 0xffff,
//         pHost->user,
//         pInfo->ipAddress & 0xff,
//         (pInfo->ipAddress >> 8) & 0xff,
//         (pInfo->ipAddress >> 16) & 0xff,
//         (pInfo->ipAddress >> 24) & 0xff,
//         Vhtons(optname) & 0xffff,
//         pInfo->userName
//         );
//OutputDebugString(buf);
                LocalFree(pHost);
                pHost = NULL;
            }
        }
        if (!pHost) {

            int userlen = lstrlen(pInfo->userName) + 1;

            if (pHost = (struct Hosts *)LocalAlloc(LPTR,
                                                   sizeof(struct Hosts)
                                                   + userlen
                                                   )) {
                memcpy(pHost + 1, pInfo->userName, userlen);
                pHost->user = (LPSTR)(pHost + 1);
                pHost->userlen = userlen;
                pHost->ip = pInfo->ipAddress;
                pHost->port = (unsigned short)optname;
            }
        }
        Head = pHost;
        if (pHost) {
            rc = 0;
        } else {
            VWSASetLastError(WSAENOBUFS);
            rc = SOCKET_ERROR;
        }

        LEAVE_MUTEX();

    }
    return rc;
}

DWORD WINAPI shutdown(int a,int b) {
    return(Vshutdown(a, b));
}

DWORD WINAPI socket(int af,int type,int protocol) {

    struct Sockets * So;
    int s;

    s = Vsocket(af, type, protocol);
    if (s != INVALID_SOCKET) {
        So = FindSocket(s);
        if (So) {
            So->type = type;
        } else {
            Vclosesocket(s);
            VWSASetLastError(WSAENOBUFS);
            s = INVALID_SOCKET;
        }
        LEAVE_MUTEX();
    }
    return s;
}

DWORD WINAPI TransmitFile(int a,int b,int c,int d,int e,int f,int g) {
    return(VTransmitFile(a, b, c, d, e, f, g));
}

DWORD WINAPI WEP() {
    return(VWEP());
}

DWORD WINAPI WSAAsyncGetHostByAddr(int a,int b,int c,int d,int e,int f,int g) {
    return(VWSAAsyncGetHostByAddr(a, b, c, d, e, f, g));
}

DWORD WINAPI WSAAsyncGetHostByName(int a,int b,int c,int d,int e) {
    return(VWSAAsyncGetHostByName(a, b, c, d, e));
}

DWORD WINAPI WSAAsyncGetProtoByName(int a,int b,int c,int d,int e) {
    return(VWSAAsyncGetProtoByName(a, b, c, d, e));
}

DWORD WINAPI WSAAsyncGetProtoByNumber(int a,int b,int c,int d,int e) {
    return(VWSAAsyncGetProtoByNumber(a, b, c, d, e));
}

DWORD WINAPI WSAAsyncGetServByName(int a,int b,int c,int d,int e,int f) {
    return(VWSAAsyncGetServByName(a, b, c, d, e, f));
}

DWORD WINAPI WSAAsyncGetServByPort(int a,int b,int c,int d,int e,int f) {
    return(VWSAAsyncGetServByPort(a, b, c, d, e, f));
}

DWORD WINAPI WSAAsyncSelect(int s, HWND hWnd, unsigned int wMsg, long lEvent) {
    return(VWSAAsyncSelect(s,hWnd,wMsg,lEvent));
}

DWORD WINAPI WSACancelAsyncRequest(int a) {
    return(VWSACancelAsyncRequest(a));
}

DWORD WINAPI WSACancelBlockingCall() {
    return(VWSACancelBlockingCall());
}

DWORD WINAPI WSACleanup() {
    return(VWSACleanup());
}

DWORD WINAPI WSAGetLastError() {
    return(VWSAGetLastError());
}

DWORD WINAPI WSAIsBlocking() {
    return(VWSAIsBlocking());
}

DWORD WINAPI WSARecvEx(int a,int b,int c,int d) {
    return(VWSARecvEx(a, b, c, d));
}

DWORD WINAPI WSASetBlockingHook(int a) {
    return(VWSASetBlockingHook(a));
}

DWORD WINAPI WSASetLastError(int a) {
    return(VWSASetLastError(a));
}

DWORD WINAPI WSAStartup(int a,int b) {
    return(VWSAStartup(a, b));
}

DWORD WINAPI WSAUnhookBlockingHook() {
    return(VWSAUnhookBlockingHook());
}

DWORD WINAPI WsControl(int a,int b,int c,int d,int e,int f) {
    return(VWsControl(a,b,c,d,e,f));
}

DWORD WSHEnumProtocols(int a,int b, int c,int d) {
    return(VWSHEnumProtocols(a,b,c,d));
}

//#ifdef DO_FILE_CONFIG
//
//void
//ParseList(char *List,struct Server **Head,int IsSvr) {
//
//    char *p;
//    char *p1;
//    char *pTok;
//    struct Server *tmp,*Current=NULL;
//
//    *Head = NULL;
//
//    if ( *(List+1) != '=')
//        return;
//    pTok = List+2;
//    List=StrTokEx(&pTok,"\t ");
//    p = StrTokEx(&List,",");
//    while ( p) {
//        if (IsSvr) {
//            tmp = (struct Server *)LocalAlloc(LPTR, (sizeof(struct Server)));
//            if ( tmp == NULL )
//                return;
//
//            p1 = strchr(p,':');
//            if (p1) {
//                *p1++ = 0;
//                tmp->port = atoi(p1);
//            }
//            else
//                tmp->port = 1080;
//        }
//        else {
//            tmp = (struct Server *)LocalAlloc(LPTR, (sizeof(struct Server)));
//            if ( tmp == NULL )
//                return;
//        }
//        tmp->Name = NewString(p);
//        tmp->Next = NULL;
//        if (Current == NULL) {
//            Current = *Head = tmp;
//        }
//        else {
//            Current->Next = tmp;
//            Current=tmp;
//        }
//        p = StrTokEx(&List,",");
//    }
//}
//
//
//void
//LoadConfig(void) {
//
//    struct Hosts *Current=NULL,*tmp;
//    char Buffer[1024];
//    FILE *f;
//    char *p;
//    char *ServerList;
//    char *UserList;
//    struct Server *Default=NULL;
//    HKEY Key;
//
//    GetSystemDirectory(Buffer,sizeof(Buffer));
//    strcat(Buffer, "\\socks.cnf");
//    f = fopen(Buffer,"rt");
//    if ( f == NULL)
//        return;
//    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\HummingBird", 0, KEY_QUERY_VALUE, &Key) == ERROR_SUCCESS) {
//    int Type, Length=sizeof(Buffer);
//        if ( RegQueryValueEx(Key, "SOCKS_SERVER", NULL, &Type, Buffer, &Length) == ERROR_SUCCESS) {
//            Buffer[Length] = '\0';
//            Default=LocalAlloc(LPTR, sizeof(struct Server));
//            if ( Default == NULL )
//                return;
//
//            p = strchr(Buffer,':');
//            if (p) {
//                *p++ = 0;
//                Default->port = atoi(p);
//            }
//            else
//                Default->port = 1080;
//            Default->Name = NewString(Buffer);
//            Default->Next = NULL;
//        }
//        RegCloseKey(Key);
//    }
//
//    while ( fgets(Buffer,sizeof(Buffer)-1,f) != NULL) {
//        Buffer[strlen(Buffer)-1]='\0';
//        if ( Buffer[0] == '#')
//            continue;
//        tmp = (struct Hosts *) LocalAlloc(LPTR, sizeof(struct Hosts));
//        if ( tmp == NULL )
//            return;
//
//        memset(tmp,0,sizeof(struct Hosts));
//        ServerList=NULL;
//        UserList=NULL;
//        p = StrTokEx(&Buffer,"\t ");
//        if ( p == NULL) {
//            LocalFree(tmp);
//            continue;
//        }
//        if ( lstrcmpi(p,"DENY") == 0) {
//            tmp->type = DENY;
//        } else if (lstrcmpi(p,"DIRECT") == 0) {
//            tmp->type = DIRECT;
//        } else if (lstrcmpi(p,"SOCKD") == 0) {
//            tmp->type = SOCKD;
//        } else {
//            LocalFree(tmp);
//            continue;
//        }
//LookMore:
//        p = StrTokEx(&Buffer,"\t ");
//        if ( p == NULL) {
//            LocalFree(tmp);
//            continue;
//        }
//        if (*p == '*') {
//            UserList=p;
//            goto LookMore;
//        }
//        if (*p == '@') {
//            ServerList=p;
//            goto LookMore;
//        }
//        tmp->dst = Vinet_addr(p);
//        p = StrTokEx(&Buffer,"\t ");
//        if ( p == NULL) {
//            LocalFree(tmp);
//            continue;
//        }
//        tmp->mask = Vinet_addr(p);
//        p = StrTokEx(&Buffer,"\t ");
//        if (p) {
//            if ( lstrcmpi(p,"EQ") == 0)
//                tmp->op = EQ;
//            else if ( lstrcmpi(p,"NEQ") == 0)
//                tmp->op = NEQ;
//            else if ( lstrcmpi(p,"LT") == 0)
//                tmp->op = LT;
//            else if ( lstrcmpi(p,"GT") == 0)
//                tmp->op = GT;
//            else if ( lstrcmpi(p,"LE") == 0)
//                tmp->op = LE;
//            else if ( lstrcmpi(p,"GE") == 0)
//                tmp->op = GE;
//            else {
//                LocalFree(tmp);
//                continue;
//            }
//            p = StrTokEx(&Buffer,"\t ");
//            if ( p == NULL) {
//                LocalFree(tmp);
//                continue;
//            }
//            if ( isdigit(*p))
//                tmp->port = atoi(p);
//            else {
//            struct servent *se;
//                se=Vgetservbyname(p,"tcp");
//                if ( se == NULL) {
//                    LocalFree(tmp);
//                    continue;
//                }
//                tmp->port = se->s_port;
//            }
//        }
//        if ( UserList)
//            ParseList(UserList,(struct Server **)&tmp->Users,0);
//        if ( ServerList)
//            ParseList(ServerList,&tmp->Servers,1);
//        if ( (tmp->type == SOCKD) && (tmp->Servers == NULL))
//            tmp->Servers=Default;
//        if ( Current == NULL) {
//            Head = Current = tmp;
//        }
//        else {
//            Current->Next = tmp;
//            Current = tmp;
//        }
//    }
//    fclose(f);
//}
//
//#endif

HMODULE hModule = NULL;
int LoadCount = 0;

BOOL
WINAPI
DllMain(
    IN HINSTANCE hInstance,
    IN DWORD reason,
    IN LPVOID Reserved
    )
{
    HKEY hKey;
    TCHAR szRegBuf[MAX_PATH+1];
    DWORD dwRegBufSize = sizeof(szRegBuf);
    DWORD dwRegType;
    LONG lResult;

    switch(reason) {
    case DLL_PROCESS_DETACH:
        if (LoadCount == 0) {
            DELETE_MUTEX();
            return 1;
        }
        if (--LoadCount == 0) {
            FreeLibrary(hModule);
        }
        return 1;

    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
        if (++LoadCount == 1) {
            break;
        }

    default:
        return 1;
    }

    // Load an alternate Winsock DLL based on a registry value,
    // in the event that a customer wants to load a different wsock32.
    //
    if (ERROR_SUCCESS == (lResult = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
        0,
        KEY_QUERY_VALUE,
        &hKey
        )))
    {
        if (ERROR_SUCCESS == (lResult = RegQueryValueEx(
            hKey,
            TEXT("AlternateWinsock"),
            NULL,
            &dwRegType,
            (LPBYTE) szRegBuf,
            &dwRegBufSize
            )) && dwRegType == REG_SZ) // only allow type REG_SZ
        {
            // Found a string, so try to load it as the alternate Winsock DLL.
            hModule = LoadLibrary(szRegBuf);
        }
        RegCloseKey(hKey);
    }

    if (lResult != ERROR_SUCCESS)
    {
        hModule = LoadLibrary("WSOCK32.DLL");
    }

    if (hModule < (HMODULE) HINSTANCE_ERROR) {
        MessageBox(NULL,
                   "Unable to find old WSOCK32.DLL named \"WSOCK32.DLL\".",
                   "Microsoft/Hummingbird SOCKS Shim",
                   MB_OK
                   );
        LoadCount = 0;
        return 0;
    }

    (FARPROC) VArecv=GetProcAddress(hModule,"Arecv");
    (FARPROC) VAsend=GetProcAddress(hModule,"Asend");
    (FARPROC) VEnumProtocolsA=GetProcAddress(hModule,"EnumProtocolsA");
    (FARPROC) VEnumProtocolsW=GetProcAddress(hModule,"EnumProtocolsW");
    (FARPROC) VGetAddressByNameA=GetProcAddress(hModule,"GetAddressByNameA");
    (FARPROC) VGetAddressByNameW=GetProcAddress(hModule,"GetAddressByNameW");
    (FARPROC) VGetNameByTypeA=GetProcAddress(hModule,"GetNameByTypeA");
    (FARPROC) VGetNameByTypeW=GetProcAddress(hModule,"GetNameByTypeW");
    (FARPROC) VGetServiceA=GetProcAddress(hModule,"GetServiceA");
    (FARPROC) VGetServiceW=GetProcAddress(hModule,"GetServiceW");
    (FARPROC) VGetTypeByNameA=GetProcAddress(hModule,"GetTypeByNameA");
    (FARPROC) VGetTypeByNameW=GetProcAddress(hModule,"GetTypeByNameW");
    (FARPROC) VNPLoadNameSpaces=GetProcAddress(hModule,"NPLoadNameSpaces");
    (FARPROC) VSetServiceA=GetProcAddress(hModule,"SetServiceA");
    (FARPROC) VSetServiceW=GetProcAddress(hModule,"SetServiceW");
    (FARPROC) VTransmitFile=GetProcAddress(hModule,"TransmitFile");
    (FARPROC) VWSAAsyncGetHostByAddr=GetProcAddress(hModule,"WSAAsyncGetHostByAddr");
    (FARPROC) VWSAAsyncGetHostByName=GetProcAddress(hModule,"WSAAsyncGetHostByName");
    (FARPROC) VWSAAsyncGetProtoByName=GetProcAddress(hModule,"WSAAsyncGetProtoByName");
    (FARPROC) VWSAAsyncGetProtoByNumber=GetProcAddress(hModule,"WSAAsyncGetProtoByNumber");
    (FARPROC) VWSAAsyncGetServByName=GetProcAddress(hModule,"WSAAsyncGetServByName");
    (FARPROC) VWSAAsyncGetServByPort=GetProcAddress(hModule,"WSAAsyncGetServByPort");
    (FARPROC) VWSAAsyncSelect=GetProcAddress(hModule,"WSAAsyncSelect");
    (FARPROC) VWSACancelAsyncRequest=GetProcAddress(hModule,"WSACancelAsyncRequest");
    (FARPROC) VWSACancelBlockingCall=GetProcAddress(hModule,"WSACancelBlockingCall");
    (FARPROC) VWSACleanup=GetProcAddress(hModule,"WSACleanup");
    (FARPROC) VWSAGetLastError=GetProcAddress(hModule,"WSAGetLastError");
    (FARPROC) VWSAIsBlocking=GetProcAddress(hModule,"WSAIsBlocking");
    (FARPROC) VWSARecvEx=GetProcAddress(hModule,"WSARecvEx");
    (FARPROC) VWSASetBlockingHook=GetProcAddress(hModule,"WSASetBlockingHook");
    (FARPROC) VWSASetLastError=GetProcAddress(hModule,"WSASetLastError");
    (FARPROC) VWSAStartup=GetProcAddress(hModule,"WSAStartup");
    (FARPROC) VWSAUnhookBlockingHook=GetProcAddress(hModule,"WSAUnhookBlockingHook");
    (FARPROC) VWSHEnumProtocols=GetProcAddress(hModule,"WSHEnumProtocols");
    (FARPROC) VWsControl=GetProcAddress(hModule,"WsControl");
    (FARPROC) V__WSAFDIsSet=GetProcAddress(hModule,"__WSAFDIsSet");
    (FARPROC) Vaccept=GetProcAddress(hModule,"accept");
    (FARPROC) Vbind=GetProcAddress(hModule,"bind");
    (FARPROC) Vclosesocket=GetProcAddress(hModule,"closesocket");
    (FARPROC) Vclosesockinfo=GetProcAddress(hModule,"closesockinfo");
    (FARPROC) Vconnect=GetProcAddress(hModule,"connect");
    (FARPROC) Vdn_expand=GetProcAddress(hModule,"dn_expand");
    (FARPROC) Vgethostbyaddr=GetProcAddress(hModule,"gethostbyaddr");
    (FARPROC) Vgethostbyname=GetProcAddress(hModule,"gethostbyname");
    (FARPROC) Vgethostname=GetProcAddress(hModule,"gethostname");
    (FARPROC) Vgetnetbyname=GetProcAddress(hModule,"getnetbyname");
    (FARPROC) Vgetpeername=GetProcAddress(hModule,"getpeername");
    (FARPROC) Vgetprotobyname=GetProcAddress(hModule,"getprotobyname");
    (FARPROC) Vgetprotobynumber=GetProcAddress(hModule,"getprotobynumber");
    (FARPROC) Vgetservbyname=GetProcAddress(hModule,"getservbyname");
    (FARPROC) Vgetservbyport=GetProcAddress(hModule,"getservbyport");
    (FARPROC) Vgetsockname=GetProcAddress(hModule,"getsockname");
    (FARPROC) Vgetsockopt=GetProcAddress(hModule,"getsockopt");
    (FARPROC) Vhtonl=GetProcAddress(hModule,"htonl");
    (FARPROC) Vhtons=GetProcAddress(hModule,"htons");
    (FARPROC) Vinet_addr=GetProcAddress(hModule,"inet_addr");
    (FARPROC) Vinet_network=GetProcAddress(hModule,"inet_network");
    (FARPROC) Vinet_ntoa=GetProcAddress(hModule,"inet_ntoa");
    (FARPROC) Vioctlsocket=GetProcAddress(hModule,"ioctlsocket");
    (FARPROC) Vlisten=GetProcAddress(hModule,"listen");
    (FARPROC) Vntohl=GetProcAddress(hModule,"ntohl");
    (FARPROC) Vntohs=GetProcAddress(hModule,"ntohs");
    (FARPROC) Vrcmd=GetProcAddress(hModule,"rcmd");
    (FARPROC) Vrecv=GetProcAddress(hModule,"recv");
    (FARPROC) Vrecvfrom=GetProcAddress(hModule,"recvfrom");
    (FARPROC) Vrexec=GetProcAddress(hModule,"rexec");
    (FARPROC) Vrresvport=GetProcAddress(hModule,"rresvport");
    (FARPROC) Vs_perror=GetProcAddress(hModule,"s_perror");
    (FARPROC) Vselect=GetProcAddress(hModule,"select");
    (FARPROC) Vsend=GetProcAddress(hModule,"send");
    (FARPROC) Vsendto=GetProcAddress(hModule,"sendto");
    (FARPROC) Vsethostname=GetProcAddress(hModule,"sethostname");
    (FARPROC) Vsetsockopt=GetProcAddress(hModule,"setsockopt");
    (FARPROC) Vshutdown=GetProcAddress(hModule,"shutdown");
    (FARPROC) Vsocket=GetProcAddress(hModule,"socket");
    (FARPROC) VWEP=GetProcAddress(hModule,"WEP");
    (FARPROC) VAcceptEx = GetProcAddress(hModule,"AcceptEx");
    (FARPROC) VGetAcceptExSockaddrs = GetProcAddress(hModule,"GetAcceptExSockaddrs");
    (FARPROC) VMigrateWinsockConfiguration = GetProcAddress(hModule,"MigrateWinsockConfiguration");
    (FARPROC) VWSApSetPostRoutine = GetProcAddress(hModule,"WSApSetPostRoutine");

    CREATE_MUTEX();

#ifdef DO_FILE_CONFIG
    LoadConfig();
#endif

    return 1;
}
