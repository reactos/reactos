#ifndef ROSDHCP_H
#define ROSDHCP_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <dhcpcsdk.h>
#include <dhcp/rosdhcp_public.h>

#include "debug.h"

#define IFNAMSIZ MAX_INTERFACE_NAME_LEN
#undef interface /* wine/objbase.h -- Grrr */

#undef IGNORE
#undef ACCEPT
#undef PREFER
#define DHCP_DISCOVER_INTERVAL 5
#define DHCP_REBOOT_TIMEOUT 10
#define DHCP_PANIC_TIMEOUT 20
#define DHCP_BACKOFF_MAX 300
#define DHCP_DEFAULT_LEASE_TIME 43200 /* 12 hours */
#define _PATH_DHCLIENT_PID "\\systemroot\\system32\\drivers\\etc\\dhclient.pid"
typedef void *VOIDPTR;
typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned int u_int32_t;
typedef char *caddr_t;

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

struct iaddr;
struct interface_info;

typedef struct _DHCP_ADAPTER {
    LIST_ENTRY     ListEntry;
    MIB_IFROW      IfMib;
    MIB_IPFORWARDROW RouterMib;
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

typedef DWORD (*PipeSendFunc)(HANDLE CommPipe, COMM_DHCP_REPLY *Reply );

#define random rand
#define srandom srand

int  init_client(void);
void stop_client(void);

void AdapterInit(VOID);
HANDLE StartAdapterDiscovery(HANDLE hStopEvent);
void AdapterStop(VOID);
extern PDHCP_ADAPTER AdapterGetFirst(VOID);
extern PDHCP_ADAPTER AdapterGetNext(PDHCP_ADAPTER);
extern PDHCP_ADAPTER AdapterFindIndex( unsigned int AdapterIndex );
extern PDHCP_ADAPTER AdapterFindInfo( struct interface_info *info );
extern PDHCP_ADAPTER AdapterFindByHardwareAddress( u_int8_t haddr[16], u_int8_t hlen );
extern HANDLE PipeInit(HANDLE hStopEvent);
extern VOID ApiInit(VOID);
extern VOID ApiFree(VOID);
extern VOID ApiLock(VOID);
extern VOID ApiUnlock(VOID);
extern DWORD DSQueryHWInfo( PipeSendFunc Send, HANDLE CommPipe, COMM_DHCP_REQ *Req );
extern DWORD DSLeaseIpAddress( PipeSendFunc Send, HANDLE CommPipe, COMM_DHCP_REQ *Req );
extern DWORD DSRenewIpAddressLease( PipeSendFunc Send, HANDLE CommPipe, COMM_DHCP_REQ *Req );
extern DWORD DSReleaseIpAddressLease( PipeSendFunc Send, HANDLE CommPipe, COMM_DHCP_REQ *Req );
extern DWORD DSStaticRefreshParams( PipeSendFunc Send, HANDLE CommPipe, COMM_DHCP_REQ *Req );
extern DWORD DSGetAdapterInfo( PipeSendFunc Send, HANDLE CommPipe, COMM_DHCP_REQ *Req );
extern int inet_aton(const char *s, struct in_addr *addr);
int warn( char *format, ... );

#endif /* ROSDHCP_H */
