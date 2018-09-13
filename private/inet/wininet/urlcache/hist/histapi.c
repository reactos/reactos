
#include <windows.h>
#include <stdio.h>
#include <urlcache.h>

//#include "cache.hxx"

//#include "history.h"
#include "generic.h"

#define HISTORYAPI URLCACHEAPI

#define DEFAULT_CEI_BUFFER_SIZE		512
	// 1k ~> sizeof (CEI) + lpszSourceUrlName + lpHeaderInfo(~<255) + lpszLocalFileName(<255)

#define ASSERT(x) if (!(x)) DebugBreak();

LPCTSTR lpszHistoryPrefix = "Hist:";
DWORD cbHistoryPrefix = sizeof("Hist:") -1;

LPCTSTR lpszTitleHeader = "Title: ";
DWORD cbTitleHeader = sizeof("Title: ") -1;

LPCTSTR lpszFragmentHeader = "Frags: ";
DWORD cbFragmentHeader = sizeof("Frags: ") -1;

LPCTSTR lpszHistoryFileExtension = "HSD";

LPTSTR szCRLF = "\r\n";
DWORD cbCRLF = sizeof("\r\n") -1;
LPTSTR szSPC = " ";
LPTSTR szPND = "#";
LPTSTR szFRAGB = " (#";
LPTSTR szFRAGE = ")";

typedef struct _HISTORY_SEARCH_OBJ 
{
	HANDLE hEnum;
	LPTSTR lpszPrefixedUrl;
	LPTSTR lpszFragment;
	LPCACHE_ENTRY_INFO lpCEI;
	LPTSTR *aFrags;
	DWORD cFrags;
	DWORD iFrags;
} HISTORY_SEARCH_OBJ, *LPHISTORY_SEARCH_OBJ;

typedef struct _HISTORY_ITEM_INFO 
{
    DWORD dwVersion;		//Version of History System
    LPSTR lpszSourceUrlName;    // embedded pointer to the URL name string.
	DWORD HistoryItemType;       // cache type bit mask.  
    FILETIME LastAccessTime;    // last accessed time in GMT format
    LPSTR lpszTitle;			// embedded pointer to the History-Title: info.
	LPSTR lpszDependancies;	// list of URLs that this page requires to be functional, SPC delimited
    DWORD dwReserved;           // reserved for future use.
} HISTORY_ITEM_INFO, *LPHISTORY_ITEM_INFO;



LPTSTR
GetDependanciesFromCEI (LPCACHE_ENTRY_INFO lpCEI)
{
	LPTSTR buf = NULL;
	HANDLE file = NULL;
	DWORD size = 0;
	LPTSTR pch = NULL;

	ASSERT (lpCEI);

	file = CreateFile(lpCEI->lpszLocalFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (file == INVALID_HANDLE_VALUE)
		return NULL;

	size = GetFileSize(file, NULL);

	buf = (LPTSTR) LocalAlloc (LPTR, size + 1);
	if (!buf)
	{
		CloseHandle(file);
		return NULL;
	}

	buf[size] = '\0';

//  we are going to store these as URL\nURL\nURL\n so just look for \n and replace with space

	for (pch = buf; *pch; pch++)
	{
		if (*pch == '\n')
			*pch = ' ';
	}

	CloseHandle (file);
	return buf;
}

LPTSTR
MakeDependanciesFile (LPCTSTR lpszDeps)
{
	HANDLE file = NULL;
	LPTSTR pch = NULL;
	LPTSTR path = NULL;
	LPTSTR temp = NULL;
	DWORD size = 0;

	path = _tempnam (NULL, "HS");
	if (!path)
		return NULL;

	file = CreateFile(path,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		LocalFree (path);
		return NULL;
	}

//  we are going to store these as URL\nURL\nURL\n 
	
	if(lpszDeps)
	{
		size = lstrlen (lpszDeps);

		temp = LocalAlloc (LPTR, size + 1);
		if (!temp)
		{
			LocalFree (path);
			CloseHandle (file);
			return NULL;
		}

		lstrcpy (temp, lpszDeps);

		for (pch = temp; *pch; pch++)
		{
			if (*pch == ' ')
				*pch = '\n';
		}

		WriteFile (file, temp, size, &size, NULL);
	}

	CloseHandle (file);
	return path;
}

LPTSTR
ConvertToUnprefixedUrl (
						LPCTSTR lpszPrefixedUrl,
						LPCTSTR lpszFragment
						)
{
	DWORD size = 0;
	LPTSTR lpszUrl = NULL;
	LPTSTR temp = NULL;

	temp = (LPTSTR) (lpszPrefixedUrl + cbHistoryPrefix) ;

	size = lstrlen(temp);

	if (lpszFragment)
	{
		size += lstrlen (lpszFragment);
		size += 1;	// for the fragment '#'
	}

	lpszUrl = (LPTSTR) LocalAlloc (LPTR, size + 1);
	if (!lpszUrl)
		return NULL;

	lstrcpy (lpszUrl, temp);

	if (lpszFragment)
	{
		lstrcat (lpszUrl, szPND);
		lstrcat (lpszUrl, lpszFragment);
	}

	return lpszUrl;
}

BOOL
ConvertToPrefixedUrl (IN LPCTSTR lpszUrlName, 
					  OUT LPTSTR *lplpszPrefixedUrl, 
					  OUT LPTSTR *lplpszFragment)
{
	if (!lpszUrlName || !*lpszUrlName)
	{
		*lplpszPrefixedUrl = (LPTSTR) LocalAlloc (LPTR, cbHistoryPrefix + 1);
		if (!*lplpszPrefixedUrl)
			return FALSE;

		lstrcpy (*lplpszPrefixedUrl, lpszHistoryPrefix);
		return TRUE;
	}

	*lplpszPrefixedUrl = (LPTSTR) LocalAlloc (LPTR, cbHistoryPrefix + strlen (lpszUrlName) + 1);
	if (!*lplpszPrefixedUrl)
		return FALSE;

	lstrcpy (*lplpszPrefixedUrl, lpszHistoryPrefix);
	lstrcat (*lplpszPrefixedUrl, lpszUrlName);

	*lplpszFragment = strchr (*lplpszPrefixedUrl, '#');
	if(*lplpszFragment)
		*((*lplpszFragment)++) = '\0';

	return TRUE;
}

LPCACHE_ENTRY_INFO
RetrievePrefixedUrl (IN LPTSTR lpszUrl)
/*++

  The CEI returned must be freed and the lpszUrl unlocked
  
--*/
{
	LPCACHE_ENTRY_INFO lpCEI = NULL;
	DWORD cbCEI = 0;

	lpCEI = (LPCACHE_ENTRY_INFO) LocalAlloc (LPTR, DEFAULT_CEI_BUFFER_SIZE);
	if (!lpCEI)
		return NULL;

	cbCEI = DEFAULT_CEI_BUFFER_SIZE;

	while (!RetrieveUrlCacheEntryFile (lpszUrl, 
								lpCEI,
								&cbCEI,
								0))
	{
		if (GetLastError () == ERROR_NOT_ENOUGH_MEMORY)
		{
			LocalFree (lpCEI);

			lpCEI = (LPCACHE_ENTRY_INFO) LocalAlloc (LPTR, cbCEI);
			if (!lpCEI)
				return NULL;

		}
		else 
			return NULL;
	}

	return lpCEI;
}


LPCACHE_ENTRY_INFO
RetrievePrefixedUrlInfo (IN LPTSTR lpszUrl)
/*++

  The CEI returned must be freed and the lpszUrl unlocked
  
--*/
{
	LPCACHE_ENTRY_INFO lpCEI = NULL;
	DWORD cbCEI = 0;

	lpCEI = (LPCACHE_ENTRY_INFO) LocalAlloc (LPTR, DEFAULT_CEI_BUFFER_SIZE);
	if (!lpCEI)
		return NULL;

	cbCEI = DEFAULT_CEI_BUFFER_SIZE;

	while (!GetUrlCacheEntryInfo (lpszUrl, 
								lpCEI,
								&cbCEI
								))
	{
		if (GetLastError () == ERROR_NOT_ENOUGH_MEMORY)
		{
			LocalFree (lpCEI);

			lpCEI = (LPCACHE_ENTRY_INFO) LocalAlloc (LPTR, cbCEI);
			if (!lpCEI)
				return NULL;

		}
		else 
			return NULL;
	}

	return lpCEI;
}

LPTSTR
GetTitleFromCEI (IN LPCACHE_ENTRY_INFO lpCEI, LPCTSTR lpszFragment)
{
	LPTSTR pHeader, pCurr;
	DWORD size = 0;

	pHeader = (LPTSTR) MemFind ((LPVOID) lpCEI->lpHeaderInfo, 
		lpCEI->dwHeaderInfoSize, 
		(LPVOID) lpszTitleHeader, 
		cbTitleHeader);
	if (!pHeader)
	{
		SetLastError (ERROR_FILE_NOT_FOUND);
		return NULL;
	}

	//Header was found

		
	pCurr = (LPTSTR) MemFind ( (LPVOID) pHeader,
		(lpCEI->dwHeaderInfoSize) - ((DWORD) (pHeader - (LPTSTR)lpCEI->lpHeaderInfo)), 
		(LPVOID) szCRLF, 
		cbCRLF);
	if (!pCurr)
	{
		// BUGBUG do what now?? found the header, but the title is not in a recognized
		// format.  lets bail with a internal prob
		ASSERT (FALSE);
		SetLastError (ERROR_FILE_NOT_FOUND);
		return NULL;
	}

	*pCurr = '\0';

	pCurr = pHeader + cbTitleHeader;
	while (*pCurr == ' ')
		pCurr++;

	size = lstrlen (pCurr) ;

	if (lpszFragment)	//must also include the fragment in Title
		size += lstrlen (lpszFragment) + 4;

	pHeader = (LPTSTR) LocalAlloc (LPTR, size + 1);
	if (!pHeader)
		return NULL;
	
	lstrcpy (pHeader, pCurr);

	if (lpszFragment)
	{
		lstrcat (pHeader, szFRAGB);
		lstrcat (pHeader, lpszFragment);
		lstrcat (pHeader, szFRAGE);
	}

	return pHeader;
		
}


DWORD
GetFragmentsFromCEI(IN LPCACHE_ENTRY_INFO lpCEI, 
						  OUT LPTSTR **paFrags, 
						  OUT DWORD *pcFrags)
{
	LPTSTR pHeader, pCurr;

	//need to get the string from the CEI, then parse into args
	pHeader = (LPTSTR) MemFind (lpCEI->lpHeaderInfo, 
		lpCEI->dwHeaderInfoSize, 
		(LPVOID) lpszFragmentHeader, 
		cbFragmentHeader);
	if (!pHeader)
		return ERROR_FILE_NOT_FOUND;

	//Header was found

		
	pCurr = (LPTSTR) MemFind ( (LPVOID) pHeader,
		lpCEI->dwHeaderInfoSize - (pHeader - lpCEI->lpHeaderInfo), 
		(LPVOID) szCRLF, 
		cbCRLF);
	if (!pCurr)
	{
		//this is a corrupted Entry
		ASSERT (FALSE);
		return ERROR_FILE_NOT_FOUND;
	}

	*pCurr = '\0';

	//
	//	pHeader is now  zero terminated string
	//	we want to parse the args of that string
	//
	if (!ParseArgsDyn(pHeader + cbFragmentHeader, paFrags, pcFrags))
		return ERROR_NOT_ENOUGH_MEMORY;

	return ERROR_SUCCESS;

}

LPBYTE
GenerateHeaderInfo(LPCTSTR lpszTitle, LPCTSTR *aFrags, DWORD cFrags)
{
	DWORD size = 0;
	LPBYTE hi = NULL;
	LPTSTR curr;
	DWORD i;

	//first need to find the size required of HeaderInfo
	if (lpszTitle)
	{
		size += lstrlen (lpszTitle);
		size += cbTitleHeader;
		size += cbCRLF;
	}

	if (cFrags)
	{
		size += cbFragmentHeader;
		size += cFrags;

		for (i = 0; i < cFrags; i++)
			size += lstrlen(aFrags[i]);

		size += cbCRLF;
	}
	
	hi = (LPBYTE) LocalAlloc (LPTR, ++size);
	if (!hi)
		return NULL;

	curr = (LPTSTR) hi;
	*curr = '\0';


	if (lpszTitle)
	{
		lstrcat (curr, lpszTitleHeader);
		lstrcat (curr, lpszTitle);
		lstrcat (curr, szCRLF);
	}

	if (cFrags)
	{
		lstrcat (curr, lpszFragmentHeader);

		for(i = 0; i < cFrags; i++)
		{
			if (!*(aFrags[i]))
				continue;

			lstrcat(curr, szSPC);
			lstrcat(curr, aFrags[i]);
		}
		lstrcat (curr, szCRLF);
	}

	return hi;
}

DWORD
CopyCEItoHII (
			  LPCTSTR lpszFragment,
			  LPHISTORY_ITEM_INFO lpHII,
			  LPDWORD lpcbHII,
			  LPCACHE_ENTRY_INFO lpCEI
			  )
{
	DWORD Error = ERROR_SUCCESS;
	DWORD cbNeeded = sizeof (HISTORY_ITEM_INFO);
	DWORD cbUsed = cbNeeded;
	LPTSTR lpszUrl = NULL;
	DWORD cbUrl  = 0;
	LPTSTR lpszTitle = NULL;
	DWORD cbTitle = 0;
	LPTSTR lpszDependancies = NULL;
	DWORD cbDependancies = 0;


	ASSERT (lpCEI->lpszSourceUrlName);
//
//	need to determine the necessary size
//

	// need the unprefixed name

	lpszUrl = ConvertToUnprefixedUrl (lpCEI->lpszSourceUrlName, (LPCTSTR) lpszFragment);
	if (!lpszUrl)
	{
		Error = ERROR_INTERNAL_ERROR;
		goto quit;
	}
	cbUrl = lstrlen (lpszUrl);

	cbNeeded += cbUrl + 1;
	
	
	lpszTitle = GetTitleFromCEI (lpCEI, (LPCTSTR) lpszFragment);
	if (lpszTitle)
	{
		cbTitle = lstrlen (lpszTitle);
		cbNeeded += cbTitle + 1;
	}

	lpszDependancies = GetDependanciesFromCEI (lpCEI);
	if (lpszDependancies)
	{
		cbDependancies = lstrlen (lpszDependancies);
		cbNeeded += cbDependancies + 1;
	}

	if (cbNeeded > *lpcbHII)
	{
		Error = ERROR_NOT_ENOUGH_MEMORY;
		*lpcbHII = cbNeeded;
		goto quit;
	}
	
//
//	Add the other pieces
//

	lpHII->lpszSourceUrlName = (LPTSTR) (lpHII + cbUsed + 1);
	lstrcpy (lpHII->lpszSourceUrlName, lpszUrl);
	cbUsed += cbUrl + 1;

	if (lpszTitle)
	{
		lpHII->lpszTitle = (LPTSTR) (lpHII + cbUsed + 1);
		lstrcpy (lpHII->lpszTitle, lpszTitle);
		cbUsed += cbTitle + 1;
	}
	else 
		lpHII->lpszTitle = NULL;

	if (lpszDependancies)
	{
		lpHII->lpszDependancies = (LPTSTR) (lpHII + cbUsed + 1);
		lstrcpy (lpHII->lpszDependancies, lpszDependancies);
		cbUsed += cbDependancies + 1;
	}
	else 
		lpHII->lpszDependancies = NULL;

	lpHII->dwVersion = lpCEI->dwVersion;
	lpHII->HistoryItemType = lpCEI->CacheEntryType;
	lpHII->LastAccessTime.dwLowDateTime = lpCEI->LastAccessTime.dwLowDateTime;
	lpHII->LastAccessTime.dwHighDateTime = lpCEI->LastAccessTime.dwHighDateTime;
	lpHII->dwReserved = lpCEI->dwReserved;

quit:

	if (lpszUrl)
		LocalFree(lpszUrl);

	if (lpszTitle)
		LocalFree(lpszTitle);

	if (lpszDependancies)
		LocalFree(lpszDependancies);

	if (Error == ERROR_SUCCESS)
		*lpcbHII = cbUsed;
	
	return Error;
}




HISTORYAPI
BOOL
WINAPI
AddHistoryItem(
    IN LPCTSTR lpszUrlName,		//direct correspondence in URLCACHE
    IN LPCTSTR lpszHistoryTitle,		// this needs to be added to lpHeaderInfo
	IN LPCTSTR lpszDependancies,
	IN DWORD dwFlags,
    IN DWORD dwReserved
    )		
/*++

Routine Description:
	
	Places the specified URL into the history.
	
	If it does not exist, then it is created.  If it does exist it is overwritten.

Arguments:

    lpszUrlName			- The URL in question.

    lpszHistoryTitle	- pointer to the friendly title that should be associated
						with this URL. If NULL, no title will be added.

    Reserved			- Unused, for future implementations

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Extended error can be retrieved from GetLastError()

--*/


{
	LPBYTE NewHeaderInfo = NULL;
	DWORD cbNHI = 0;
	BOOL New = FALSE;
	
	LPTSTR lpszPrefixedUrl = NULL;
	LPTSTR lpszFragment = NULL;
	DWORD Error = ERROR_SUCCESS;
	LPCACHE_ENTRY_INFO lpCEI = NULL;
	FILETIME ftExpires;
	FILETIME ftModified;
	SYSTEMTIME st;
	LPTSTR *aFrags = NULL;
	DWORD cFrags = 0;
	DWORD i;
	BOOL found = FALSE;
	LPTSTR lpszDepsPath = NULL;
	DWORD type = NORMAL_CACHE_ENTRY;
	LPTSTR lpszOldTitle = NULL;


	if (!ConvertToPrefixedUrl (lpszUrlName, &lpszPrefixedUrl, &lpszFragment))
	{
		Error = ERROR_NOT_ENOUGH_MEMORY;
		goto quit;
	}

	lpCEI = RetrievePrefixedUrl (lpszPrefixedUrl);
	if (!lpCEI)
		New = TRUE;

	
	//  Buffer filled with data now
	//  BUGBUG must handle fragments

	if (!New)
	{
		type = lpCEI->CacheEntryType;
		GetFragmentsFromCEI (lpCEI, &aFrags, &cFrags);
		lpszOldTitle = GetTitleFromCEI (lpCEI, NULL);
	}

//	if (Error != ERROR_SUCCESS)

	if (lpszFragment)
	{
		for (i = 0; i < cFrags; i++)
		{
			if (lstrcmp (aFrags[i], lpszFragment) == 0)
			{
				found = TRUE;
				break;
			}
		}
		if (!found)
			AddArgvDyn (&aFrags, &cFrags, lpszFragment);
	}

	NewHeaderInfo = GenerateHeaderInfo (lpszHistoryTitle ? lpszHistoryTitle : lpszOldTitle, aFrags, cFrags);	
	cbNHI = lstrlen (NewHeaderInfo);

	lpszDepsPath = MakeDependanciesFile (lpszDependancies);
	if (!lpszDepsPath)
	{
		ASSERT(FALSE);
		Error = ERROR_INTERNAL_ERROR;
		goto quit;
	}

	GetLocalTime (&st);
	SystemTimeToFileTime(&st, &ftModified);

	st.wDay += 7;	//BUGBUG must get this setting from registry
	if(!SystemTimeToFileTime(&st, &ftExpires))
	{
		Error = GetLastError ();
		goto quit;
	}

	if (lpCEI)
	{
		UnlockUrlCacheEntryFile (lpCEI->lpszSourceUrlName, 0);
		LocalFree (lpCEI);
		lpCEI = NULL;
	}

	if (!CommitUrlCacheEntry(
		lpszPrefixedUrl,
		lpszDepsPath,	
		ftExpires,	
		ftModified,								//we dont care about last modified time
		type,	//this is set from dwFlags i think
		NewHeaderInfo,
		cbNHI ,
		lpszHistoryFileExtension,
		0))
	{
		Error = GetLastError ();
		goto quit;
	}
	// if we made it to here, we win!

quit:

	if (aFrags)
		LocalFree (aFrags);

	if (lpszDepsPath)
		LocalFree (lpszDepsPath);

	if (lpCEI)
	{
		UnlockUrlCacheEntryFile (lpCEI->lpszSourceUrlName, 0);
		LocalFree (lpCEI);
	}

	if (lpszPrefixedUrl)
		LocalFree (lpszPrefixedUrl);

	if (NewHeaderInfo)
		LocalFree (NewHeaderInfo);

	if (lpszOldTitle)
		LocalFree (lpszOldTitle);

	if (Error != ERROR_SUCCESS)
	{
		SetLastError (Error);
		return FALSE;
	}
	else 
		return TRUE;

}

	

HISTORYAPI
BOOL
WINAPI
IsHistorical(
    IN LPCTSTR lpszUrlName
    )

/*++

Routine Description:
	
	Checks to see if Url is a valid History item

Arguments:

    lpszUrlName			- The URL in question.

Return Value:

    BOOL
        Success		- TRUE.  Item is in History

        Failure		- FALSE. Extended error can be retrieved from GetLastError()
						ERROR_FILE_NOT_FOUND indicates the URL is not available
					

--*/

{
	LPTSTR lpszPrefixedUrl = NULL;
	LPTSTR lpszFragment = NULL;
	DWORD Error = ERROR_SUCCESS;
	LPCACHE_ENTRY_INFO lpCEI = NULL;
	LPTSTR *aFrags = NULL;
	DWORD cFrags = 0;
	DWORD i;

	if (!ConvertToPrefixedUrl (lpszUrlName, &lpszPrefixedUrl, &lpszFragment))
	{
		Error = ERROR_NOT_ENOUGH_MEMORY;
		goto quit;
	}


	lpCEI = RetrievePrefixedUrlInfo (lpszPrefixedUrl);
	if (!lpCEI)
	{
		Error = GetLastError ();
		goto quit;
	}

	if (lpszFragment)
	{

//
//	Need to check for IntraDocFrags
//

		Error = GetFragmentsFromCEI(lpCEI, & aFrags, & cFrags);
		if (Error != ERROR_SUCCESS)
			goto quit;

		for (i = 0; i < cFrags; i++)
		{
			if (strcmp(aFrags[i], lpszFragment) == 0)
				goto quit;
		}
		
		Error = ERROR_FILE_NOT_FOUND;
	}

quit:
	
	if (aFrags)
		LocalFree (aFrags);

	if (lpszPrefixedUrl)
		LocalFree (lpszPrefixedUrl);

	if (lpCEI)
	{
		LocalFree (lpCEI);
	}

	if (Error != ERROR_SUCCESS)
	{
		SetLastError (Error);
		return FALSE;
	}
	else 
		return TRUE;

}



HISTORYAPI
BOOL
WINAPI
RemoveHistoryItem (
    IN LPCTSTR lpszUrlName,
    IN DWORD dwReserved
    )
/*++

Routine Description:
	
	Changes an entry from an History Item to a normal cache entry.  Removing
	the Title at the same time.

Arguments:

    lpszUrlName			- The URL in question.

    dwReserved			- Unused.  for future usage

Return Value:

    BOOL
        Success		- TRUE.  Item found and removed

        Failure		- FALSE. Extended error can be retrieved from GetLastError()
						ERROR_FILE_NOT_FOUND indicates the URL is not available
					

--*/
{
	LPTSTR *aFrags = NULL;
	DWORD cFrags = 0;
	DWORD i;
	LPTSTR lpszTitle = NULL;
	LPBYTE NewHeaderInfo = NULL;
	
	LPTSTR lpszPrefixedUrl = NULL;
	LPTSTR lpszFragment = NULL;
	DWORD Error = ERROR_SUCCESS;
	LPCACHE_ENTRY_INFO lpCEI = NULL;

	if (!ConvertToPrefixedUrl (lpszUrlName, &lpszPrefixedUrl, &lpszFragment))
	{
		Error = ERROR_NOT_ENOUGH_MEMORY;
		goto quit;
	}


	lpCEI = RetrievePrefixedUrl (lpszPrefixedUrl);
	if (!lpCEI)
	{
		Error = GetLastError ();
		goto quit;
	}

	if (lpszFragment)
	{
		BOOL found = FALSE;
//
//	Need to check for IntraDocFrags
//

		Error = GetFragmentsFromCEI(lpCEI, & aFrags, & cFrags);
		if (Error != ERROR_SUCCESS)
			goto quit;

		for (i = 0; i < cFrags; i++)
		{
			if (strcmp(aFrags[i], lpszFragment) == 0)
			{
				//we need to delete this and reinsert

				*(aFrags[i]) = '\0';
				found = TRUE;
				break;
			}
		}	

		if (cFrags - 1 && found)
		{
			lpszTitle = GetTitleFromCEI (lpCEI, NULL);
			
			NewHeaderInfo = GenerateHeaderInfo (lpszTitle, aFrags, cFrags);

			if(!NewHeaderInfo)
			{
				Error = ERROR_NOT_ENOUGH_MEMORY;
				goto quit;
			}

			UnlockUrlCacheEntryFile(lpCEI->lpszSourceUrlName, 0);

			if (!CommitUrlCacheEntry(
				lpszPrefixedUrl,
				lpCEI->lpszLocalFileName,
				lpCEI->ExpireTime,
				lpCEI->LastModifiedTime,
				lpCEI->CacheEntryType ,		// only changes
				NewHeaderInfo,	// 
				lstrlen (NewHeaderInfo),
				lpCEI->lpszFileExtension,
				0))
			{
				Error = GetLastError ();
			}
			goto quit;
		}

		if (!found)
		{
			Error = ERROR_FILE_NOT_FOUND;
			goto quit;
		}
	}
//BUGBUG  looks like this will always delete a history item if there is only fragment
	//problem is we could have a frag and a unfragged Item
	UnlockUrlCacheEntryFile(lpCEI->lpszSourceUrlName, 0);

	if (!DeleteUrlCacheEntry(lpszPrefixedUrl))
	{
		Error = GetLastError ();
		goto quit;
	}



quit:
	if (aFrags)
		LocalFree (aFrags);

	if (lpszTitle)
		LocalFree (lpszTitle);

	if (lpCEI)
	{
		LocalFree (lpCEI);
	}

	if (NewHeaderInfo)
		LocalFree (NewHeaderInfo);

	if (lpszPrefixedUrl)
		LocalFree (lpszPrefixedUrl);

	if (Error != ERROR_SUCCESS)
	{
		SetLastError (Error);
		return FALSE;
	}
	else 
		return TRUE;

}


HISTORYAPI
BOOL
WINAPI
GetHistoryItemInfo (
    IN LPCTSTR lpszUrlName,
    OUT LPHISTORY_ITEM_INFO lpHistoryItemInfo,
    IN OUT LPDWORD lpdwHistoryItemInfoBufferSize
    )
/*++

Routine Description:
	
	Fills a buffer with a HISTORY_ITEM_INFO struct.

Arguments:

    lpszUrlName			- The URL in question.

    lpHistoryItemInfo	- Buffer that will hold the HISTORY_ITEM_INFO

	lpdwHistoryItemInfoBufferSize	- IN: size of the lpHistoryItemInfo buffer
									 OUT: size of filled struct when successful
										  or necessary buffer size when failed


Return Value:

    BOOL
        Success		- TRUE.  

        Failure		- FALSE. Extended error can be retrieved from GetLastError()
						ERROR_NOT_ENOUGH_MEMORY indicates the buffer is insufficient
					

--*/


{
	LPTSTR lpszPrefixedUrl = NULL;
	LPTSTR lpszFragment = NULL;
	DWORD Error = ERROR_SUCCESS;
	LPCACHE_ENTRY_INFO lpCEI = NULL;

	if (!ConvertToPrefixedUrl (lpszUrlName, &lpszPrefixedUrl, &lpszFragment))
	{
		Error = ERROR_NOT_ENOUGH_MEMORY;
		goto quit;
	}


	lpCEI = RetrievePrefixedUrlInfo (lpszPrefixedUrl);
	if (!lpCEI)
	{
		Error = GetLastError ();
		goto quit;
	}
	

	Error = CopyCEItoHII (lpszFragment, lpHistoryItemInfo, lpdwHistoryItemInfoBufferSize, lpCEI);

	

quit:

	if (lpszPrefixedUrl)
		LocalFree (lpszPrefixedUrl);

	if (lpCEI)
	{
		LocalFree (lpCEI);
	}

	if (Error != ERROR_SUCCESS)
	{
		SetLastError (Error);
		return FALSE;
	}
	else 
		return TRUE;

}


HISTORYAPI
HANDLE
WINAPI
FindFirstHistoryItem(
    IN LPCTSTR  lpszUrlSearchPattern,
    OUT LPHISTORY_ITEM_INFO lpFirstHistoryItemInfo,
    IN OUT LPDWORD lpdwFirstHistoryItemInfoBufferSize
    )

/*++

Routine Description:
	
	Searches through the History looking for URLs that match the search pattern,
	and copies the HISTORY_ITEM_INFO into the buffer.

Arguments:

    lpszUrlSearchPattern	- The URL in question.

    lpFirstHistoryItemInfo	- Buffer that will hold the HISTORY_ITEM_INFO

	lpdwFirstHistoryItemInfoBufferSize	- IN: size of the lpHistoryItemInfo buffer
									 OUT: size of filled struct when successful
										  or necessary buffer size when failed


Return Value:

    HANDLE
        Success		- Valid enumeration handle to pass into subsequent calls to
					FindNextHistoryItem ().

        Failure		- NULL. Extended error can be retrieved from GetLastError()
						ERROR_NOT_ENOUGH_MEMORY indicates the buffer is insufficient
					

--*/

{
	LPHISTORY_SEARCH_OBJ hso = NULL;
	LPCACHE_ENTRY_INFO lpCEI = NULL;
	DWORD cbCEI = 0;
	LPTSTR lpszFoundFragment = NULL;
	DWORD Error = ERROR_SUCCESS;
	BOOL found = FALSE;

	hso = (LPHISTORY_SEARCH_OBJ) LocalAlloc (LPTR, sizeof (HISTORY_SEARCH_OBJ));
	if (!hso)
	{
		Error = GetLastError ();
		goto quit;
	}

	hso->aFrags = NULL;
	hso->cFrags = 0;
	hso->iFrags = 0;
	hso->lpszPrefixedUrl = NULL;
	hso->lpszFragment = NULL;

	if (!ConvertToPrefixedUrl (lpszUrlSearchPattern, &(hso->lpszPrefixedUrl), &(hso->lpszFragment)))
	{
		Error = ERROR_NOT_ENOUGH_MEMORY;
		goto quit;
	}

	lpCEI = (LPCACHE_ENTRY_INFO) LocalAlloc (LPTR, DEFAULT_CEI_BUFFER_SIZE);
	if (!lpCEI)
	{
		Error = GetLastError ();
		goto quit;
	}

	while (TRUE)
	{
		hso->hEnum = FindFirstUrlCacheEntry (hso->lpszPrefixedUrl,
			lpCEI,
			&cbCEI);

		if (!hso->hEnum)
		{
			Error = GetLastError ();
			if (Error == ERROR_NOT_ENOUGH_MEMORY)
			{
				LocalFree (lpCEI);

				lpCEI = (LPCACHE_ENTRY_INFO) LocalAlloc (LPTR, cbCEI);
				if (!lpCEI)
				{
					Error = ERROR_INTERNAL_ERROR;
					goto quit;
				}
			}
			else 
				goto quit;
		}
		else break;
	}
	
	found = TRUE;	

	//BUGBUG have to handle enum of fragments
	Error = GetFragmentsFromCEI (lpCEI, &(hso->aFrags), &(hso->cFrags));
	switch (Error)
	{
	case ERROR_FILE_NOT_FOUND:	//only the default URL is used
		Error = ERROR_SUCCESS;
		break;

	case ERROR_SUCCESS:			//first return the default URL next call will get frags
		hso->lpCEI = lpCEI;
		break;

	default:
		goto quit;
		break;
	}

	if (hso->lpszFragment)
	{
		found = FALSE;

		for (; hso->iFrags < hso->cFrags; hso->iFrags++)
		{
			if (strncmp (hso->aFrags[hso->iFrags], hso->lpszFragment, lstrlen (hso->lpszFragment)) == 0)
			{
				found = TRUE;
				lpszFoundFragment = hso->aFrags[hso->iFrags];
				break;
			}
		}
	}

	if (!found)
	{
		Error = ERROR_FILE_NOT_FOUND;
		goto quit;
	}

	Error = CopyCEItoHII (
		lpszFoundFragment,
		lpFirstHistoryItemInfo,
		lpdwFirstHistoryItemInfoBufferSize,
		lpCEI);

quit:

	if (Error != ERROR_SUCCESS)
	{
		SetLastError (Error);
		
		if (hso->lpszPrefixedUrl)
			LocalFree (hso->lpszPrefixedUrl);

		if (hso->aFrags)
			LocalFree (hso->aFrags);

		if (hso->lpCEI)
		{
			UnlockUrlCacheEntryFile (hso->lpCEI->lpszSourceUrlName, 0);
			LocalFree(hso->lpCEI);
		}

		if (hso)
			LocalFree (hso);

		return NULL;
	}
	
	if (lpCEI && !hso->lpCEI)
		LocalFree (lpCEI);

	return (HANDLE) hso;

}



HISTORYAPI
BOOL
WINAPI
FindNextHistoryItem(
    IN HANDLE hEnumHandle,
    OUT LPHISTORY_ITEM_INFO lpHistoryItemInfo,
    IN OUT LPDWORD lpdwHistoryItemInfoBufferSize
    )

/*++

Routine Description:
	
	Searches through the History looking for URLs that match the search pattern,
	and copies the HISTORY_ITEM_INFO into the buffer.

Arguments:

    lpszUrlSearchPattern	- The URL in question.

    lpFirstHistoryItemInfo	- Buffer that will hold the HISTORY_ITEM_INFO

	lpdwFirstHistoryItemInfoBufferSize	- IN: size of the lpHistoryItemInfo buffer
									 OUT: size of filled struct when successful
										  or necessary buffer size when failed


Return Value:

    HANDLE
        Success		- Valid enumeration handle to pass into subsequent calls to
					FindNextHistoryItem ().

        Failure		- NULL. Extended error can be retrieved from GetLastError()
						ERROR_NOT_ENOUGH_MEMORY indicates the buffer is insufficient
					

--*/

{
	DWORD Error = ERROR_SUCCESS;
	LPCACHE_ENTRY_INFO lpCEI = NULL;
	DWORD cbCEI = 0;
	LPHISTORY_SEARCH_OBJ hso = NULL;
	BOOL found = FALSE;
	LPTSTR lpszFoundFragment;

	if (!hEnumHandle)
	{
		SetLastError (ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	hso = (LPHISTORY_SEARCH_OBJ) hEnumHandle;
	
	while (!found)
	{
		if (hso->aFrags)
		{
			//this means that there are only fragments to find
			for (lpszFoundFragment = NULL; hso->iFrags < hso->cFrags; hso->iFrags++)
			{
				if (hso->lpszFragment)
				{
					if (strncmp (hso->aFrags[hso->iFrags], hso->lpszFragment, lstrlen (hso->lpszFragment)) == 0)
					{
						found = TRUE;
						lpCEI = hso->lpCEI;
						break;
					}
				}
				else
				{
					found = TRUE;
					break;
				}
			}
			if (!found)
			{
				if (hso->lpszFragment)
				{
					Error = ERROR_FILE_NOT_FOUND;
					goto quit;
				}
				else 
				{
					//this means that we went through all the frags
					//we need to drop through and find the Cache Entry that matches
					Error = ERROR_SUCCESS;
					
					ASSERT (hso->lpCEI);
					ASSERT (hso->aFrags);
					
					lpCEI = hso->lpCEI;		//reuse the buffer if possible
					LocalFree (hso->aFrags);

					hso->lpCEI = NULL;
					hso->aFrags = NULL;
				}
			}
			else
			{
				lpszFoundFragment = hso->aFrags[hso->iFrags];
				lpCEI = hso->lpCEI;
			}
		}
		
		else
		{
			if (!lpCEI)
				lpCEI = (LPCACHE_ENTRY_INFO) LocalAlloc (LPTR, DEFAULT_CEI_BUFFER_SIZE);

			if (!lpCEI)
			{
				Error = ERROR_INTERNAL_ERROR;
				goto quit;
			}

			while (TRUE)
			{
				if (!FindNextUrlCacheEntry (hso->hEnum,
					lpCEI,
					&cbCEI))
				{
					Error = GetLastError ();
					if (Error == ERROR_NOT_ENOUGH_MEMORY)
					{
						LocalFree (lpCEI);

						lpCEI = (LPCACHE_ENTRY_INFO) LocalAlloc (LPTR, cbCEI);
						if (!lpCEI)
						{
							Error = ERROR_INTERNAL_ERROR;
							goto quit;
						}
					}
					else 
						goto quit;
				}
				else 
					break;
			}

			Error = GetFragmentsFromCEI (lpCEI, &(hso->aFrags), &(hso->cFrags));
			switch (Error)
			{
			case ERROR_FILE_NOT_FOUND:	//only the default URL is used
				found = TRUE;
				Error = ERROR_SUCCESS;
				break;

			case ERROR_SUCCESS:			//first return the default URL next call will get frags
				hso->lpCEI = lpCEI;
				found = TRUE;
				break;

			default:
				goto quit;
				break;
			}
		}
	}

	Error = CopyCEItoHII(
		lpszFoundFragment,
		lpHistoryItemInfo,
		lpdwHistoryItemInfoBufferSize,
		lpCEI);

quit:


	if (lpCEI && !hso->lpCEI)
	{
		UnlockUrlCacheEntryFile (lpCEI->lpszSourceUrlName, 0);
		LocalFree (lpCEI);
	}


	if (Error != ERROR_SUCCESS)
	{
		SetLastError (Error);
		return FALSE;
	}
	else 
		return TRUE;

}


HISTORYAPI
BOOL
WINAPI
FindCloseHistory (
    IN HANDLE hEnumHandle
    )

{
	LPHISTORY_SEARCH_OBJ hso;
	HANDLE hEnum;

	//possibly we should be keeping track of valid hso's i dunno
	if (!hEnumHandle)
	{
		SetLastError (ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	hso = (LPHISTORY_SEARCH_OBJ) hEnumHandle;

	hEnum = hso->hEnum;

	if (hso->aFrags)
		LocalFree (hso->aFrags);

	if (hso->lpszPrefixedUrl)
		LocalFree(hso->lpszPrefixedUrl);

	if (hso->lpCEI)
	{
		LocalFree (hso->lpCEI);
	}


	LocalFree (hso);

	return FindCloseUrlCache (hEnum);
}

BOOL
DLLHistoryEntry(
    IN HINSTANCE DllHandle,
    IN DWORD Reason,
    IN LPVOID Reserved
    )
/*++

Routine Description:

    Performs global initialization and termination for all protocol modules.

    This function only handles process attach and detach which are required for
    global initialization and termination, respectively. We disable thread
    attach and detach. New threads calling Wininet APIs will get an
    INTERNET_THREAD_INFO structure created for them by the first API requiring
    this structure

Arguments:

    DllHandle   - handle of this DLL. Unused

    Reason      - process attach/detach or thread attach/detach

    Reserved    - if DLL_PROCESS_ATTACH, NULL means DLL is being dynamically
                  loaded, else static. For DLL_PROCESS_DETACH, NULL means DLL
                  is being freed as a consequence of call to FreeLibrary()
                  else the DLL is being freed as part of process termination

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Failed to initialize

--*/
{
    BOOL ok;
    DWORD error;

//    UNREFERENCED_PARAMETER(DllHandle);

    //
    // perform global dll initialization, if any.
    //

    switch (Reason) {
    case DLL_PROCESS_ATTACH:

//        error = DllProcessAttachDiskCache();


        //
        // we switch off thread library calls to avoid taking a hit for every
        // thread creation/termination that happens in this process, regardless
        // of whether Internet APIs are called in the thread.
        //
        // If a new thread does make Internet API calls that require a per-thread
        // structure then the individual API will create one
        //

//        DisableThreadLibraryCalls(DllHandle);
        break;

    case DLL_PROCESS_DETACH:

        if (Reserved != NULL) {
                //
                //  Only Cleanup if there is a FreeLibrary() call.
                //
            break;
        }

//        DllProcessDetachDiskCache();

        break;
    }

    return (TRUE);
}
