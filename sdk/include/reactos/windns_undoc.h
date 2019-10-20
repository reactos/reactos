#ifndef _WINDNS_UNDOC_H_
#define _WINDNS_UNDOC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DNS_CACHE_ENTRY
{
    struct _DNS_CACHE_ENTRY *pNext; /* Pointer to next entry */
    PWSTR pszName;                  /* DNS Record Name */
    unsigned short wType;           /* DNS Record Type */
    unsigned short wUnknown;        /* Unknown */
    unsigned short wFlags;          /* DNS Record Flags */
} DNSCACHEENTRY, *PDNSCACHEENTRY;

BOOL
WINAPI
DnsFlushResolverCache(VOID);

BOOL
WINAPI
DnsGetCacheDataTable(
    _Out_ PDNSCACHEENTRY *DnsCache);

#ifdef __cplusplus
}
#endif

#endif /* _WINDNS_UNDOC_H_ */
