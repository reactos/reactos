/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    SockType.h

Abstract:

    Contains type definitions for the WinSock DLL.

Author:

    David Treadwell (davidtr)    25-Feb-1992

Revision History:

--*/

#ifndef _SOCKTYPE_
#define _SOCKTYPE_

//
// A typedef for blocking hook functions.
//

typedef
BOOL
(*PBLOCKING_HOOK) (
    VOID
    );

//
// Structures, etc. to support per-thread variable storage in this DLL.
//

#define MAXALIASES      35
#define MAXADDRS        35

#define HOSTDB_SIZE     (_MAX_PATH + 7)   // 7 == strlen("\\hosts") + 1

typedef struct _WINSOCK_TLS_DATA {
    char *GETHOST_h_addr_ptrs[MAXADDRS + 1];
    struct hostent GETHOST_host;
    char *GETHOST_host_aliases[MAXALIASES];
    char GETHOST_hostbuf[BUFSIZ+1];
    struct in_addr GETHOST_host_addr;
    char GETHOST_HOSTDB[HOSTDB_SIZE];
    FILE *GETHOST_hostf;
    char GETHOST_hostaddr[MAXADDRS*sizeof(ULONG)];
    char *GETHOST_host_addrs[2];
    int GETHOST_stayopen;
    char GETPROTO_PROTODB[PROTODB_SIZE];
    FILE *GETPROTO_protof;
    char GETPROTO_line[BUFSIZ+1];
    struct protoent GETPROTO_proto;
    char *GETPROTO_proto_aliases[MAXALIASES];
    int GETPROTO_stayopen;
    char GETSERV_SERVDB[SERVDB_SIZE];
    FILE *GETSERV_servf;
    char GETSERV_line[BUFSIZ+1];
    struct servent GETSERV_serv;
    char *GETSERV_serv_aliases[MAXALIASES];
    int GETSERV_stayopen;
    struct state R_INIT__res;
    char INTOA_Buffer[16];
    BOOLEAN ProcessingGetXByY;
    BOOLEAN GetXByYCancelled;
    BOOLEAN DisableWinsNameResolution;
    ULONG CreateOptions;
    INT DnrErrorCode;
    LONG  lDNREpoch;
    int     BigBufSize;
    char ** GETHOST_h_addr_ptrsBigBuf;
    char *  GETHOST_hostaddrBigBuf;
#if DBG
    ULONG IndentLevel;
#endif
} WINSOCK_TLS_DATA, *PWINSOCK_TLS_DATA;

#define ACCESS_THREAD_DATA(a,file) \
            ( ((PWINSOCK_TLS_DATA)GET_THREAD_DATA())-> \
                ## file ## _ ## a )

#define _res ACCESS_THREAD_DATA( _res, R_INIT )

#define SockThreadProcessingGetXByY \
            ( ((PWINSOCK_TLS_DATA)GET_THREAD_DATA())->ProcessingGetXByY )

#define SockThreadGetXByYCancelled \
            ( ((PWINSOCK_TLS_DATA)GET_THREAD_DATA())->GetXByYCancelled )

#define SockThreadBlockingHook \
            ( ((PWINSOCK_TLS_DATA)GET_THREAD_DATA())->BlockingHook )

#define SockDisableWinsNameResolution \
            ( ((PWINSOCK_TLS_DATA)GET_THREAD_DATA())->DisableWinsNameResolution )

#define SockCreateOptions \
            ( ((PWINSOCK_TLS_DATA)GET_THREAD_DATA())->CreateOptions )

#define SockThreadDnrErrorCode \
            ( ((PWINSOCK_TLS_DATA)GET_THREAD_DATA())->DnrErrorCode )

#if DBG
#define SockIndentLevel \
            ( ((PWINSOCK_TLS_DATA)GET_THREAD_DATA())->IndentLevel )
#endif

#define SockBigBufSize \
            ( ((PWINSOCK_TLS_DATA)GET_THREAD_DATA())->BigBufSize )

typedef struct _WINSOCK_CONTEXT_BLOCK {

    LIST_ENTRY AsyncThreadQueueListEntry;
    DWORD TaskHandle;
    DWORD OpCode;

    union {

        struct {
            HWND hWnd;
            unsigned int wMsg;
            PCHAR Filter;
            int Length;
            int Type;
            PCHAR Buffer;
            int BufferLength;
        } AsyncGetHost;

        struct {
            HWND hWnd;
            unsigned int wMsg;
            PCHAR Filter;
            PCHAR Buffer;
            int BufferLength;
        } AsyncGetProto;

        struct {
            HWND hWnd;
            unsigned int wMsg;
            PCHAR Filter;
            PCHAR Protocol;
            PCHAR Buffer;
            int BufferLength;
        } AsyncGetServ;

    } Overlay;

} WINSOCK_CONTEXT_BLOCK, *PWINSOCK_CONTEXT_BLOCK;

//
// Opcodes for processing by the winsock asynchronous processing
// thread.
//

#define WS_OPCODE_GET_HOST_BY_ADDR    0x01
#define WS_OPCODE_GET_HOST_BY_NAME    0x02
#define WS_OPCODE_GET_PROTO_BY_NUMBER 0x03
#define WS_OPCODE_GET_PROTO_BY_NAME   0x04
#define WS_OPCODE_GET_SERV_BY_PORT    0x05
#define WS_OPCODE_GET_SERV_BY_NAME    0x06
#define WS_OPCODE_TERMINATE           0x07

//
// Prototype for a post message routine.  This must have the same prototype
// as PostMessage().
//

typedef
BOOL
(*PWINSOCK_POST_ROUTINE) (
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

#endif // ndef _SOCKTYPE_

