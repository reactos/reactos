// history.
#ifndef _HISTAPI_
#define _HISTEAPI_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_HISTORYAPI_)
#define HISTORYAPI DECLSPEC_IMPORT
#else
#define HISTORYAPI
#endif

typedef struct _HISTORY_ITEM_INFO {
    DWORD dwVersion;		//Version of History System
    LPSTR lpszSourceUrlName;    // embedded pointer to the URL name string.
	DWORD HistoryItemType;       // cache type bit mask.  
    FILETIME LastAccessTime;    // last accessed time in GMT format
    LPSTR lpszTitle;			// embedded pointer to the History-Title: info.
	LPSTR lpszDependancies;	// list of URLs that this page requires to be functional, SPC delimited
    DWORD dwReserved;           // reserved for future use.
} HISTORY_ITEM_INFO, *LPHISTORY_ITEM_INFO;


HISTORYAPI
BOOL
WINAPI
FindCloseHistory (
    IN HANDLE hEnumHandle
    );


HISTORYAPI
BOOL
WINAPI
FindNextHistoryItem(
    IN HANDLE hEnumHandle,
    OUT LPHISTORY_ITEM_INFO lpHistoryItemInfo,
    IN OUT LPDWORD lpdwHistoryItemInfoBufferSize
    );



HISTORYAPI
HANDLE
WINAPI
FindFirstHistoryItem(
    IN LPCTSTR  lpszUrlSearchPattern,
    OUT LPHISTORY_ITEM_INFO lpFirstHistoryItemInfo,
    IN OUT LPDWORD lpdwFirstHistoryItemInfoBufferSize
    );

HISTORYAPI
BOOL
WINAPI
GetHistoryItemInfo (
    IN LPCTSTR lpszUrlName,
    OUT LPHISTORY_ITEM_INFO lpHistoryItemInfo,
    IN OUT LPDWORD lpdwHistoryItemInfoBufferSize
    );


HISTORYAPI
BOOL
WINAPI
RemoveHistoryItem (
    IN LPCTSTR lpszUrlName,
    IN DWORD dwReserved
    );


HISTORYAPI
BOOL
WINAPI
IsHistorical(
    IN LPCTSTR lpszUrlName
    );

HISTORYAPI
BOOL
WINAPI
AddHistoryItem(
    IN LPCTSTR lpszUrlName,		//direct correspondence in URLCACHE
    IN LPCTSTR lpszHistoryTitle,		// this needs to be added to lpHeaderInfo
	IN LPCTSTR lpszDependancies,
	IN DWORD dwFlags,
    IN DWORD dwReserved
    );




#ifdef __cplusplus
}
#endif


#endif  // _HISTAPI_








