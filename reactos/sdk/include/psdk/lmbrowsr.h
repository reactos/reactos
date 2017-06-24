#ifndef _LMBROWSR_H
#define _LMBROWSR_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define BROWSER_ROLE_PDC 1
#define BROWSER_ROLE_BDC 2
typedef struct _BROWSER_STATISTICS {
	LARGE_INTEGER StatisticsStartTime;
	LARGE_INTEGER NumberOfServerAnnouncements;
	LARGE_INTEGER NumberOfDomainAnnouncements;
	ULONG NumberOfElectionPackets;
	ULONG NumberOfMailslotWrites;
	ULONG NumberOfGetBrowserServerListRequests;
	ULONG NumberOfServerEnumerations;
	ULONG NumberOfDomainEnumerations;
	ULONG NumberOfOtherEnumerations;
	ULONG NumberOfMissedServerAnnouncements;
	ULONG NumberOfMissedMailslotDatagrams;
	ULONG NumberOfMissedGetBrowserServerListRequests;
	ULONG NumberOfFailedServerAnnounceAllocations;
	ULONG NumberOfFailedMailslotAllocations;
	ULONG NumberOfFailedMailslotReceives;
	ULONG NumberOfFailedMailslotWrites;
	ULONG NumberOfFailedMailslotOpens;
	ULONG NumberOfDuplicateMasterAnnouncements;
	LARGE_INTEGER NumberOfIllegalDatagrams;
} BROWSER_STATISTICS,*PBROWSER_STATISTICS,*LPBROWSER_STATISTICS;
typedef struct _BROWSER_STATISTICS_100 {
	LARGE_INTEGER StartTime;
	LARGE_INTEGER NumberOfServerAnnouncements;
	LARGE_INTEGER NumberOfDomainAnnouncements;
	ULONG NumberOfElectionPackets;
	ULONG NumberOfMailslotWrites;
	ULONG NumberOfGetBrowserServerListRequests;
	LARGE_INTEGER NumberOfIllegalDatagrams;
} BROWSER_STATISTICS_100,*PBROWSER_STATISTICS_100;
typedef struct _BROWSER_STATISTICS_101 {
	LARGE_INTEGER StartTime;
	LARGE_INTEGER NumberOfServerAnnouncements;
	LARGE_INTEGER NumberOfDomainAnnouncements;
	ULONG NumberOfElectionPackets;
	ULONG NumberOfMailslotWrites;
	ULONG NumberOfGetBrowserServerListRequests;
	LARGE_INTEGER NumberOfIllegalDatagrams;
	ULONG NumberOfMissedServerAnnouncements;
	ULONG NumberOfMissedMailslotDatagrams;
	ULONG NumberOfMissedGetBrowserServerListRequests;
	ULONG NumberOfFailedServerAnnounceAllocations;
	ULONG NumberOfFailedMailslotAllocations;
	ULONG NumberOfFailedMailslotReceives;
	ULONG NumberOfFailedMailslotWrites;
	ULONG NumberOfFailedMailslotOpens;
	ULONG NumberOfDuplicateMasterAnnouncements;
} BROWSER_STATISTICS_101,*PBROWSER_STATISTICS_101;
typedef struct _BROWSER_EMULATED_DOMAIN {
	LPWSTR DomainName;
	LPWSTR EmulatedServerName;
	DWORD Role;
} BROWSER_EMULATED_DOMAIN,*PBROWSER_EMULATED_DOMAIN;

NET_API_STATUS WINAPI I_BrowserServerEnum(LPCWSTR,LPCWSTR,LPCWSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,DWORD,LPCWSTR,PDWORD);
NET_API_STATUS WINAPI I_BrowserServerEnumEx(LPCWSTR,LPCWSTR,LPCWSTR,DWORD,PBYTE*,DWORD,PDWORD,PDWORD,DWORD,LPCWSTR,LPCWSTR);
NET_API_STATUS WINAPI I_BrowserQueryEmulatedDomains(LPWSTR,PBROWSER_EMULATED_DOMAIN*,PDWORD);
NET_API_STATUS WINAPI I_BrowserQueryOtherDomains(LPCWSTR,PBYTE*,PDWORD,PDWORD);
NET_API_STATUS WINAPI I_BrowserResetNetlogonState(LPCWSTR);
NET_API_STATUS WINAPI I_BrowserSetNetlogonState(LPWSTR,LPWSTR,LPWSTR,DWORD);
NET_API_STATUS WINAPI I_BrowserQueryStatistics(LPCWSTR,LPBROWSER_STATISTICS*);
NET_API_STATUS WINAPI I_BrowserResetStatistics(LPCWSTR);
NET_API_STATUS WINAPI I_BrowserDebugTrace(PWCHAR,PCHAR);
NET_API_STATUS WINAPI NetBrowserStatisticsGet(PWSTR,DWORD,PBYTE*);
#ifdef __cplusplus
}
#endif
#endif
