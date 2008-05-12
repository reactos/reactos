#ifndef IPPRIVATE_H
#define IPPRIVATE_H

#define NtCurrentTeb NtXCurrentTeb

#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
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

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x500
#define WIN32_NO_STATUS
#include <winsock2.h>
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <nspapi.h>
#include <iptypes.h>
#include "iphlpapi.h"
#include "resinfo.h"
#include "wine/debug.h"

#include "ddk/tdiinfo.h"
#include "tcpioctl.h"

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

#define IP_MIB_ROUTETABLE_ENTRY_ID   0x101

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

typedef union _TCP_REQUEST_SET_INFORMATION_EX_SAFELY_SIZED {
    CHAR MaxSize[sizeof(TCP_REQUEST_SET_INFORMATION_EX) - 1 +
		 sizeof(IPRouteEntry)];
    TCP_REQUEST_SET_INFORMATION_EX Req;
} TCP_REQUEST_SET_INFORMATION_EX_SAFELY_SIZED,
    *PTCP_REQUEST_SET_INFORMATION_EX_SAFELY_SIZED;

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
NTSTATUS openTcpFile(PHANDLE tcpFile);
VOID closeTcpFile(HANDLE tcpFile);
NTSTATUS tdiGetEntityIDSet( HANDLE tcpFile, TDIEntityID **entitySet,
			    PDWORD numEntities );
NTSTATUS tdiGetSetOfThings( HANDLE tcpFile, DWORD toiClass, DWORD toiType,
			    DWORD toiId, DWORD teiEntity, DWORD teiInstance,
			    DWORD fixedPart,
			    DWORD entrySize, PVOID *tdiEntitySet,
			    PDWORD numEntries );
VOID tdiFreeThingSet( PVOID things );
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
void ConsumeRegValueString( PWCHAR NameServer );
BOOL isInterface( TDIEntityID *if_maybe );
BOOL hasArp( HANDLE tcpFile, TDIEntityID *arp_maybe );

#include <w32api.h>
/* This is here until we switch to version 2.5 of the mingw headers */
#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)
BOOL WINAPI
GetComputerNameExA(COMPUTER_NAME_FORMAT,LPSTR,LPDWORD);
#endif

#ifdef FORCE_DEBUG
#undef DPRINT
#define DPRINT(fmt,x...) DbgPrint("%s:%d:%s: " fmt, __FILE__, __LINE__, __FUNCTION__, ## x)
#endif

#endif/*IPPRIVATE_H*/
