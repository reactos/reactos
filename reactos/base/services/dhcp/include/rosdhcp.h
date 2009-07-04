#ifndef ROSDHCP_H
#define ROSDHCP_H

#define WIN32_NO_STATUS
#include <winsock2.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <iprtrmib.h>
#include <iphlpapi.h>
#include <dhcpcsdk.h>
#include <stdio.h>
#include <io.h>
#include <setjmp.h>
#include "stdint.h"
#include "predec.h"
#include <dhcp/rosdhcp_public.h>
#include "debug.h"
#define IFNAMSIZ MAX_INTERFACE_NAME_LEN
#undef interface /* wine/objbase.h -- Grrr */

#undef IGNORE
#undef ACCEPT
#undef PREFER
#define DHCP_DISCOVER_INTERVAL 15
#define DHCP_REBOOT_TIMEOUT 300
#define DHCP_PANIC_TIMEOUT DHCP_REBOOT_TIMEOUT * 3
#define DHCP_BACKOFF_MAX 300
#define _PATH_DHCLIENT_PID "\\systemroot\\system32\\drivers\\etc\\dhclient.pid"
typedef void *VOIDPTR;

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#undef ssize_t
#ifdef _WIN64
#if defined(__GNUC__) && defined(__STRICT_ANSI__)
  typedef int ssize_t __attribute__ ((mode (DI)));
#else
  typedef __int64 ssize_t;
#endif
#else
  typedef int ssize_t;
#endif
#endif

typedef u_int32_t uintTIME;
#define TIME uintTIME
#include "dhcpd.h"

#define INLINE inline
#define PROTO(x) x

typedef void (*handler_t) PROTO ((struct packet *));

typedef struct _DHCP_ADAPTER {
    LIST_ENTRY     ListEntry;
    MIB_IFROW      IfMib;
    MIB_IPADDRROW  IfAddr;
    SOCKADDR       Address;
    ULONG NteContext,NteInstance;
    struct interface_info DhclientInfo;
    struct client_state DhclientState;
    struct client_config DhclientConfig;
    struct sockaddr_in ListenAddr;
    unsigned int BindStatus;
    unsigned char recv_buf[1];
} DHCP_ADAPTER, *PDHCP_ADAPTER;

typedef DWORD (*PipeSendFunc)( COMM_DHCP_REPLY *Reply );

#define random rand
#define srandom srand

void AdapterInit(VOID);
HANDLE PipeInit(VOID);
extern PDHCP_ADAPTER AdapterGetFirst();
extern PDHCP_ADAPTER AdapterGetNext(PDHCP_ADAPTER);
extern PDHCP_ADAPTER AdapterFindIndex( unsigned int AdapterIndex );
extern PDHCP_ADAPTER AdapterFindInfo( struct interface_info *info );
extern VOID ApiInit();
extern VOID ApiLock();
extern VOID ApiUnlock();
extern DWORD DSQueryHWInfo( PipeSendFunc Send, COMM_DHCP_REQ *Req );
extern DWORD DSLeaseIpAddress( PipeSendFunc Send, COMM_DHCP_REQ *Req );
extern DWORD DSRenewIpAddressLease( PipeSendFunc Send, COMM_DHCP_REQ *Req );
extern DWORD DSReleaseIpAddressLease( PipeSendFunc Send, COMM_DHCP_REQ *Req );
extern DWORD DSStaticRefreshParams( PipeSendFunc Send, COMM_DHCP_REQ *Req );
extern DWORD DSGetAdapterInfo( PipeSendFunc Send, COMM_DHCP_REQ *Req );
extern int inet_aton(const char *s, struct in_addr *addr);
int warn( char *format, ... );
#endif/*ROSDHCP_H*/
