#ifndef IPPRIVATE_H
#define IPPRIVATE_H

#define NtCurrentTeb NtXCurrentTeb

#include <stdio.h>
#include <stdlib.h>
//#include <sys/types.h>
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winnls.h>
#include <ws2tcpip.h>
#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <iphlpapi.h>
#include "resinfo.h"
#include <wine/debug.h>

#include "dhcp.h"
#include <dhcpcsdk.h>
#include <dhcpcapi.h>

#include <tdiinfo.h>
#include <tcpioctl.h>

#include <tdilib.h>

#include "ifenum.h"
#include "ipstats.h"
#include "route.h"

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifndef INADDR_NONE
#define INADDR_NONE (~0U)
#endif

#ifndef IFENT_SOFTWARE_LOOPBACK
#define IFENT_SOFTWARE_LOOPBACK 24 /* This is an SNMP constant from rfc1213 */
#endif/*IFENT_SOFTWARE_LOOPBACK*/

#define INDEX_IS_LOOPBACK 0x00800000

/* Type declarations */

#ifndef IFNAMSIZ
#define IFNAMSIZ 0x20
#endif/*IFNAMSIZ*/

#define TCP_REQUEST_QUERY_INFORMATION_INIT { { { 0 } } }
#define TCP_REQUEST_SET_INFORMATION_INIT { { 0 } }

/* FIXME: ROS headers suck */
#ifndef GAA_FLAG_SKIP_UNICAST
#define GAA_FLAG_SKIP_UNICAST      0x0001
#endif

#ifndef GAA_FLAG_SKIP_FRIENDLY_NAME
#define GAA_FLAG_SKIP_FRIENDLY_NAME 0x0020
#endif

// As in the mib from RFC 1213

typedef struct _IPRouteEntry {
    ULONG ire_dest;
    ULONG ire_index;            //matches if_index in IFEntry and iae_index in IPAddrEntry
    ULONG ire_metric1;
    ULONG ire_metric2;
    ULONG ire_metric3;
    ULONG ire_metric4;
    ULONG ire_gw;
    ULONG ire_type;
    ULONG ire_proto;
    ULONG ire_age;
    ULONG ire_mask;
    ULONG ire_metric5;
    ULONG ire_info;
} IPRouteEntry;

/* No caddr_t in reactos headers */
typedef char *caddr_t;

typedef union _IFEntrySafelySized {
    CHAR MaxSize[sizeof(DWORD) +
		 sizeof(IFEntry) +
		 MAX_ADAPTER_DESCRIPTION_LENGTH + 1];
    IFEntry ent;
} IFEntrySafelySized;

typedef union _TCP_REQUEST_SET_INFORMATION_EX_ROUTE_ENTRY {
    CHAR MaxSize[sizeof(TCP_REQUEST_SET_INFORMATION_EX) - 1 +
		 sizeof(IPRouteEntry)];
    TCP_REQUEST_SET_INFORMATION_EX Req;
} TCP_REQUEST_SET_INFORMATION_EX_ROUTE_ENTRY,
    *PTCP_REQUEST_SET_INFORMATION_EX_ROUTE_ENTRY;

typedef union _TCP_REQUEST_SET_INFORMATION_EX_ARP_ENTRY {
    CHAR MaxSize[sizeof(TCP_REQUEST_SET_INFORMATION_EX) - 1 +
		 sizeof(MIB_IPNETROW)];
    TCP_REQUEST_SET_INFORMATION_EX Req;
} TCP_REQUEST_SET_INFORMATION_EX_ARP_ENTRY,
    *PTCP_REQUEST_SET_INFORMATION_EX_ARP_ENTRY;

/* Encapsulates information about an interface */
typedef struct _IFInfo {
    TDIEntityID        entity_id;
    IFEntrySafelySized if_info;
    IPAddrEntry        ip_addr;
} IFInfo;

typedef struct _IP_SET_DATA {
    ULONG NteContext;
    ULONG NewAddress;
    ULONG NewNetmask;
} IP_SET_DATA, *PIP_SET_DATA;

typedef enum _IPHLPAddrType {
    IPAAddr, IPABcast, IPAMask, IFMtu, IFStatus
} IPHLPAddrType;

/** Prototypes **/
NTSTATUS getNthIpEntity( HANDLE tcpFile, DWORD index, TDIEntityID *ent );
NTSTATUS tdiGetIpAddrsForIpEntity( HANDLE tcpFile, TDIEntityID *ent,
				   IPAddrEntry **addrs, PDWORD numAddrs );
int GetLongestChildKeyName( HANDLE RegHandle );
LONG OpenChildKeyRead( HANDLE RegHandle,
		       PWCHAR ChildKeyName,
		       PHKEY ReturnHandle );
PWCHAR GetNthChildKeyName( HANDLE RegHandle, DWORD n );
void ConsumeChildKeyName( PWCHAR Name );
PWCHAR QueryRegistryValueString( HANDLE RegHandle, PWCHAR ValueName );
PWCHAR *QueryRegistryValueStringMulti( HANDLE RegHandle, PWCHAR ValueName );
void ConsumeRegValueString( PWCHAR NameServer );
BOOL isInterface( TDIEntityID *if_maybe );
BOOL hasArp( HANDLE tcpFile, TDIEntityID *arp_maybe );

typedef VOID (*EnumNameServersFunc)( PWCHAR Interface,
				     PWCHAR NameServer,
				     PVOID Data );
LSTATUS EnumNameServers( HKEY RegHandle, LPWSTR Interface, PVOID Data, EnumNameServersFunc cb );
NTSTATUS getIPAddrEntryForIf(HANDLE tcpFile,
                             char *name,
                             DWORD index,
                             IFInfo *ifInfo);
DWORD TCPSendIoctl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, PULONG pInBufferSize, LPVOID lpOutBuffer, PULONG pOutBufferSize);

#include <w32api.h>
/* This is here until we switch to version 2.5 of the mingw headers */
#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)
BOOL WINAPI
GetComputerNameExA(COMPUTER_NAME_FORMAT,LPSTR,LPDWORD);
#endif

#endif /* IPPRIVATE_H */
