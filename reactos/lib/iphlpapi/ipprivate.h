#ifndef IPPRIVATE_H
#define IPPRIVATE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#include <windows.h>
#include <windef.h>
#include <winbase.h>
#include <net/miniport.h>
#include <winsock2.h>
#include <nspapi.h>
#include <iptypes.h>
#include "iphlpapiextra.h"
#include "ipregprivate.h"
#include "iphlpapi.h"
#include "ifenum.h"
#include "ipstats.h"
#include "iphlp_res.h"
#include "wine/debug.h"

#undef TRACE
#define TRACE(fmt,args...) DbgPrint("(%s:%d - %s) " fmt, __FILE__, __LINE__, __FUNCTION__, ## args)

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

/* No caddr_t in reactos headers */
typedef char *caddr_t;

typedef union _IFEntrySafelySized {
    PCHAR MaxSize[sizeof(DWORD) + 
		  sizeof(IFEntry) + 
		  MAX_ADAPTER_DESCRIPTION_LENGTH + 1];
    struct {
	DWORD ProperlyOffsetTheStructure;
	IFEntry ent;
    } offset;
} IFEntrySafelySized;

/** Prototypes **/
NTSTATUS openTcpFile(PHANDLE tcpFile);
VOID closeTcpFile(HANDLE tcpFile);
NTSTATUS tdiGetEntityIDSet( HANDLE tcpFile, TDIEntityID **entitySet,
			    PDWORD numEntities );
VOID tdiFreeThingSet( PVOID things );

#endif/*IPPRIVATE_H*/
