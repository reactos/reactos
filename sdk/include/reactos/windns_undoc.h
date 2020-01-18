#ifndef _WINDNS_UNDOC_H_
#define _WINDNS_UNDOC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DNS_CACHE_ENTRY
{
    struct _DNS_CACHE_ENTRY *pNext; /* Pointer to next entry */
#if defined(__midl) || defined(__WIDL__)
    [string] PWSTR pszName;         /* DNS Record Name */
#else
    PWSTR pszName;                  /* DNS Record Name */
#endif
    unsigned short wType1;          /* DNS Record Type 1 */
    unsigned short wType2;          /* DNS Record Type 2 */
    unsigned short wFlags;          /* DNS Record Flags */
} DNS_CACHE_ENTRY, *PDNS_CACHE_ENTRY;


#ifndef __WIDL__
// Hack

BOOL
WINAPI
DnsFlushResolverCache(VOID);

BOOL
WINAPI
DnsGetCacheDataTable(
    _Out_ PDNS_CACHE_ENTRY *DnsCache);

DWORD
WINAPI
GetCurrentTimeInSeconds(VOID);

DNS_STATUS
WINAPI
Query_Main(
    LPCWSTR Name,
    WORD Type,
    DWORD Options,
    PDNS_RECORD *QueryResultSet);

#endif /* __WIDL__ */

#ifdef __cplusplus
}
#endif

#endif /* _WINDNS_UNDOC_H_ */
