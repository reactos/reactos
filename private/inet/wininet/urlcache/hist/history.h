// history.

typedef struct _zHISTORY_ITEM_INFO {
    DWORD dwVersion;		//Version of History System
    LPSTR lpszSourceUrlName;    // embedded pointer to the URL name string.
	DWORD HistoryItemType;       // cache type bit mask.  
    FILETIME LastAccessTime;    // last accessed time in GMT format
    LPSTR lpszTitle;			// embedded pointer to the History-Title: info.
	LPSTR lpszDependancies;	// list of URLs that this page requires to be functional, SPC delimited
    DWORD dwReserved;           // reserved for future use.
} HISTORY_ITEM_INFO, *LPHISTORY_ITEM_INFO;
