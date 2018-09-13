/*Includes-----------------------------------------------------------*/
#include "msrating.h"
#pragma hdrstop

#include <npassert.h>
#include "ratings.h"

#include "roll.h"




/*IsUrlInFile---------------------------------------------------------------*/
/*
	Return VAlue length of Best Match
*/
static HRESULT IsUrlInFile(LPCTSTR pszTargetUrl, char **ppRating, const char* pFile, DWORD dwFile, HANDLE hAbortEvent, void* (WINAPI *MemAlloc)(long size))
{
	LocalListRecordHeader *pllrh;
	DWORD dwBytesRead;
	HRESULT hrRet;
	int nBest, nActual, nCmp;
	const char *pBest;
	BOOL fAbort;

	dwBytesRead = 0;
	nBest   = 0;
	nActual = strlenf(pszTargetUrl);
	fAbort  = FALSE;

	//Go through each recored until there is a match good enough or an abort
	while (!fAbort && nActual != nBest && dwBytesRead <= dwFile)
	{
		pllrh = (LocalListRecordHeader*) pFile;
		if (pllrh->nUrl > nBest && 
				(
					(pllrh->nUrl == nActual)
					||			
					(
						pllrh->nUrl < nActual && 
						(
							(pszTargetUrl[pllrh->nUrl] == '\\') 
							|| 
							(pszTargetUrl[pllrh->nUrl] == '/')
							|| 
							(pszTargetUrl[pllrh->nUrl] == ':')
						)
					)
				)				
			)
		{
			nCmp = strnicmpf(pFile+sizeof(LocalListRecordHeader), pszTargetUrl, pllrh->nUrl);
			if (0==nCmp)
			{
				nBest = pllrh->nUrl;
				pBest = pFile;
				hrRet = pllrh->hrRet;
			}
			//the local list is alphabetized
			else if (1==nCmp) break;

		}
		dwBytesRead += pllrh->nUrl + pllrh->nRating + sizeof(LocalListRecordHeader);
		pFile       += pllrh->nUrl + pllrh->nRating + sizeof(LocalListRecordHeader);
		fAbort       = (WAIT_OBJECT_0 == WaitForSingleObject(hAbortEvent, 0));
	}

	//was the match close enough??!?!?
	if (!fAbort && nBest)
	{
		//yes, now try to copy rating
		pllrh = (LocalListRecordHeader*) pBest;
		if (pllrh->nRating)
		{
			*ppRating = (char*) MemAlloc(pllrh->nRating+1);
			if (*ppRating)
			{
				CopyMemory(*ppRating, pBest + sizeof(LocalListRecordHeader) + pllrh->nUrl, pllrh->nRating);
				(*ppRating)[pllrh->nRating] = 0;
			}
		}
	}
	else
	{
		//no... oh well
        hrRet = E_RATING_NOT_FOUND;
    }

	return hrRet;
}



/*RatingObtainFromLocalList-------------------------------------------------*/
/*
	Grab rating information from local file.
	Should operate synchronously and take small amount of time.
	Doesn't check pOrd->fAbort too often.
*/



HRESULT RatingHelperProcLocalList(LPCTSTR pszTargetUrl, HANDLE hAbortEvent, void* (WINAPI *MemAlloc)(long size), char **ppRatingOut)
{
	DWORD dwFile;
    HRESULT hrRet = E_RATING_NOT_FOUND;
    HANDLE hFile, hMap;
	BOOL fAbort;
	const char *pFile;

	ASSERT(ppRatingOut);
	//Open and check from approved list
	hFile = CreateFile(
		FILE_NAME_LIST, GENERIC_READ, 
		FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		dwFile = GetFileSize(hFile, NULL);
		hMap   = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hMap)
		{
			pFile = (const char*) MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
			if (pFile) 
			{
				//check for correct file type
        if (BATCAVE_LOCAL_LIST_MAGIC_COOKIE == *((DWORD*) pFile))
        {
          pFile  += sizeof(DWORD);
          dwFile -= sizeof(DWORD);
          fAbort  = (WAIT_OBJECT_0 == WaitForSingleObject(hAbortEvent, 0));
          if (!fAbort) hrRet = IsUrlInFile(pszTargetUrl, ppRatingOut, pFile, dwFile, hAbortEvent, MemAlloc);
          pFile  -= sizeof(DWORD);
        }
			}
			dwFile = (DWORD) UnmapViewOfFile((LPVOID)pFile);
			CloseHandle(hMap);
		}
		CloseHandle(hFile);
	}
	return hrRet;
}


