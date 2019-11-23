#ifndef _DNSRSLVR_PCH_
#define _DNSRSLVR_PCH_

#include <stdarg.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <winsvc.h>
#include <windns.h>
#include <windns_undoc.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <ndk/obfuncs.h>

#include <dnsrslvr_s.h>

#include <strsafe.h>

typedef struct _RESOLVER_CACHE_ENTRY
{
    LIST_ENTRY CacheLink;
    PDNS_RECORDW Record;
} RESOLVER_CACHE_ENTRY, *PRESOLVER_CACHE_ENTRY;

typedef struct _RESOLVER_CACHE
{
    LIST_ENTRY RecordList;
    CRITICAL_SECTION Lock;
} RESOLVER_CACHE, *PRESOLVER_CACHE;


/* cache.c */

VOID DnsIntCacheInitialize(VOID);
VOID DnsIntCacheRemoveEntryItem(PRESOLVER_CACHE_ENTRY CacheEntry);
VOID DnsIntCacheFree(VOID);
VOID DnsIntCacheFlush(VOID);

DNS_STATUS
DnsIntCacheGetEntryByName(
    LPCWSTR Name,
    WORD wType,
    DWORD dwFlags,
    PDNS_RECORDW *Record);

VOID DnsIntCacheAddEntry(PDNS_RECORDW Record);
BOOL DnsIntCacheRemoveEntryByName(LPCWSTR Name);



/* hostsfile.c */

BOOL
ReadHostsFile(VOID);

#endif /* _DNSRSLVR_PCH_ */
