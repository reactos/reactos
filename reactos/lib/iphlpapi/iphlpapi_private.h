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

#define NTOS_MODE_USER
#include <ntos.h>
#include <ddk/ntddk.h>
#include <rosrtl/string.h>
#include <ntdll/rtl.h>
#include <net/miniport.h>
#include <winsock2.h>
#include <nspapi.h>
#include <iptypes.h>
#include "iphlpapi.h"
#include "resinfo.h"
#include "wine/debug.h"

#include "net/tdiinfo.h"
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

typedef enum _IPHLPAddrType {
    IPAAddr, IPABcast, IPAMask, IFMtu, IFStatus
} IPHLPAddrType;

/** Prototypes **/
NTSTATUS openTcpFile(PHANDLE tcpFile);
VOID closeTcpFile(HANDLE tcpFile);
NTSTATUS tdiGetEntityIDSet( HANDLE tcpFile, TDIEntityID **entitySet,
			    PDWORD numEntities );
NTSTATUS tdiGetSetOfThings( HANDLE tcpFile, DWORD toiClass, DWORD toiType,
			    DWORD toiId, DWORD teiEntity, DWORD fixedPart,
			    DWORD entrySize, PVOID *tdiEntitySet, 
			    PDWORD numEntries );
VOID tdiFreeThingSet( PVOID things );
NTSTATUS getNthIpEntity( HANDLE tcpFile, DWORD index, TDIEntityID *ent );
NTSTATUS tdiGetIpAddrsForIpEntity( HANDLE tcpFile, TDIEntityID *ent,
				   IPAddrEntry **addrs, PDWORD numAddrs );

int GetLongestChildKeyName( HANDLE RegHandle );
LONG OpenChildKeyRead( HANDLE RegHandle,
		       PCHAR ChildKeyName,
		       PHKEY ReturnHandle );
PCHAR GetNthChildKeyName( HANDLE RegHandle, DWORD n );
void ConsumeChildKeyName( PCHAR Name );
PCHAR QueryRegistryValueString( HANDLE RegHandle, PCHAR ValueName );
void ConsumeRegValueString( PCHAR NameServer );

#include <w32api.h>
/* This is here until we switch to version 2.5 of the mingw headers */
#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)
BOOL WINAPI
GetComputerNameExA(COMPUTER_NAME_FORMAT,LPSTR,LPDWORD);
#endif

#endif/*IPPRIVATE_H*/
