// ############################################################################
// Miscellaneous support routines
#include "pch.hpp"
#include "phbk.h"

#define irgMaxSzs 5
char szStrTable[irgMaxSzs][256];

// ############################################################################
LPSTR GetSz(WORD wszID)
{
	static int iSzTable=0;
	LPSTR psz = (LPSTR)&szStrTable[iSzTable][0];
	
	iSzTable++;
	if (iSzTable >= irgMaxSzs)
		iSzTable = 0;
		
	if (!LoadString(g_hInstDll, wszID, psz, 256))
	{
		Dprintf("LoadString failed %d\n", (DWORD) wszID);
		*psz = 0;
	}
		
	return (psz);
}

// ############################################################################
void SzCanonicalFromAE (LPSTR psz, PACCESSENTRY pAE, LPLINECOUNTRYENTRY pLCE)
{
	if (NO_AREA_CODE == pAE->dwAreaCode)
	{
		wsprintf(psz, "+%ld %s", pLCE->dwCountryCode, pAE->szAccessNumber);
	}
	else
	{
		wsprintf(psz, "+%ld (%s) %s", pLCE->dwCountryCode, pAE->szAreaCode, pAE->szAccessNumber);
	}
	
	return;
}

// ############################################################################
int MyStrcmp(LPVOID pv1, LPVOID pv2)
{
	LPSTR pc1 = (LPSTR) pv1;
	LPSTR pc2 = (LPSTR) pv2;
	int iRC = 0;
	// loop while not pointed at the ending NULL character and no difference has been found
	while (*pc1 && *pc2 && !iRC)
	{
		iRC = (int)(*pc1 - *pc2);
		pc1++;
		pc2++;
	}

	// if we exited because we got to the end of one string before we found a difference
	// return -1 if pv1 is longer, else return the character pointed at by pv2.  If pv2
	// is longer than pv1 then the value at pv2 will be greater than 0.  If both strings
	// ended at the same time, then pv2 will point to 0.
	if (!iRC)
	{
		iRC = (*pc1) ? -1 : (*pc2);
	}
	return iRC;
}
// ############################################################################
int __cdecl Compare950Entry(const void*pv1, const void*pv2)
{
	return (((NPABLOCK *) pv1)->wAreaCode - ((NPABLOCK *) pv2)->wAreaCode);
}

// ############################################################################
int __cdecl CompareIDLookUpElements(const void *e1, const void *e2)
{
	if (((LPIDLOOKUPELEMENT)e1)->dwID > ((LPIDLOOKUPELEMENT)e2)->dwID)
		return 1;
	if (((LPIDLOOKUPELEMENT)e1)->dwID < ((LPIDLOOKUPELEMENT)e2)->dwID)
		return -1;
	return 0;
}

// ############################################################################
int __cdecl CompareCntryNameLookUpElements(const void *e1, const void *e2)
{
	LPCNTRYNAMELOOKUPELEMENT pCUE1 = (LPCNTRYNAMELOOKUPELEMENT)e1;
	LPCNTRYNAMELOOKUPELEMENT pCUE2 = (LPCNTRYNAMELOOKUPELEMENT)e2;

#ifdef WIN16
	return lstrcmpi(pCUE1->psCountryName, pCUE2->psCountryName);
#else		
	return CompareString(LOCALE_USER_DEFAULT,0,pCUE1->psCountryName,
		pCUE1->dwNameSize,pCUE2->psCountryName,
		pCUE2->dwNameSize) - 2;
//	return CompareString(LOCALE_USER_DEFAULT,0,((LPCNTRYNAMELOOKUPELEMENT)*e1)->psCountryName,
//		((LPCNTRYNAMELOOKUPELEMENT)*e1)->dwNameSize,((LPCNTRYNAMELOOKUPELEMENT)*e2)->psCountryName,
//		((LPCNTRYNAMELOOKUPELEMENT)*e2)->dwNameSize) - 2;
#endif
}

// ############################################################################
int __cdecl CompareIdxLookUpElements(const void *e1, const void *e2)
{
	if (((LPIDXLOOKUPELEMENT)e1)->dwIndex > ((LPIDXLOOKUPELEMENT)e2)->dwIndex)
		return 1;
	if (((LPIDXLOOKUPELEMENT)e1)->dwIndex < ((LPIDXLOOKUPELEMENT)e2)->dwIndex)
		return -1;
	return 0;
}

// ############################################################################
int __cdecl CompareIdxLookUpElementsFileOrder(const void *pv1, const void *pv2)
{
	PACCESSENTRY pae1, pae2;
	int iSort;

	pae1 = ((LPIDXLOOKUPELEMENT)pv1)->pAE;
	pae2 = ((LPIDXLOOKUPELEMENT)pv2)->pAE;

	// sort empty enteries to the end of the list
	if (!(pae1 && pae2))
	{
		return (pae1 ? -1 : (pae2 ? 1 : 0));
	}

	// country ASC, state ASC, city ASC, toll free DESC, flip DESC, con spd max DESC
	if (pae1->dwCountryID != pae2->dwCountryID)
	{
		return (int)(pae1->dwCountryID - pae2->dwCountryID);
	}
	
	if (pae1->wStateID != pae2->wStateID)
	{
		return (pae1->wStateID - pae2->wStateID);
	}

	iSort  = MyStrcmp((LPVOID)pae1->szCity, (LPVOID)pae2->szCity);
	if (iSort)
	{
		return (iSort);
	}

	if (pae1->fType != pae2->fType)
	{
		return (pae2->fType - pae1->fType);
	}

	if (pae1->bFlipFactor != pae2->bFlipFactor)
	{
		return (pae2->bFlipFactor - pae1->bFlipFactor);
	}

	if (pae1->dwConnectSpeedMax != pae2->dwConnectSpeedMax)
	{
		return (int)(pae2->dwConnectSpeedMax - pae1->dwConnectSpeedMax);
	}

	return 0;
}

// ############################################################################
//inline BOOL FSz2Dw(PCSTR pSz,DWORD *dw)
BOOL FSz2Dw(LPCSTR pSz,DWORD far *dw)
{
	DWORD val = 0;
	while (*pSz)
	{
		if (*pSz >= '0' && *pSz <= '9')
		{
			val *= 10;
			val += *pSz++ - '0';
		}
		else
		{
			return FALSE;  //bad number
		}
	}
	*dw = val;
	return (TRUE);
}

// ############################################################################
//inline BOOL FSz2W(PCSTR pSz,WORD *w)
BOOL FSz2W(LPCSTR pSz,WORD far *w)
{
	DWORD dw;
	if (FSz2Dw(pSz,&dw))
	{
		*w = (WORD)dw;
		return TRUE;
	}
	return FALSE;
}

// ############################################################################
//inline BOOL FSz2B(PCSTR pSz,BYTE *pb)
BOOL FSz2B(LPCSTR pSz,BYTE far *pb)
{
	DWORD dw;
	if (FSz2Dw(pSz,&dw))
	{
		*pb = (BYTE)dw;
		return TRUE;
	}
	return FALSE;
}

// ############################################################################
HRESULT ReleaseBold(HWND hwnd)
{
	HFONT hfont = NULL;

	hfont = (HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
	if (hfont) DeleteObject(hfont);
	return ERROR_SUCCESS;
}

// ############################################################################
HRESULT MakeBold (HWND hwnd)
{
	HRESULT hr = ERROR_SUCCESS;
	HFONT hfont = NULL;
	HFONT hnewfont = NULL;
	LOGFONT far * plogfont = NULL;

	if (!hwnd) goto MakeBoldExit;

	hfont = (HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
	if (!hfont)
	{
		hr = GetLastError();
		goto MakeBoldExit;
	}
    
	plogfont = (LOGFONT far *)GlobalAlloc(GPTR,sizeof(LOGFONT));
	if (!plogfont)
	{
		hr = GetLastError();
		goto MakeBoldExit;
	}

	if (!GetObject(hfont,sizeof(LOGFONT),(LPVOID)plogfont))
	{
		hr = GetLastError();
		goto MakeBoldExit;
	}

	if (plogfont->lfHeight < 24)
	{
		plogfont->lfHeight = plogfont->lfHeight + (plogfont->lfHeight / 4);
	}

	plogfont->lfWeight = FW_BOLD;

	if (!(hnewfont = CreateFontIndirect(plogfont)))
	{
		hr = GetLastError();
		goto MakeBoldExit;
	}

	SendMessage(hwnd,WM_SETFONT,(WPARAM)hnewfont,MAKELPARAM(FALSE,0));

	GlobalFree(plogfont);
	plogfont = NULL;
	
MakeBoldExit:
	//if (hfont) DeleteObject(hfont);
	// BUG:? Do I need to delete hnewfont at some time?
	return hr;
}

#if !defined(WIN16)
//+----------------------------------------------------------------------------
//
//	Function:	DWGetWin32Platform
//
//	Synopsis:	Return a value to determine win32 platform
//
//	Arguements:	None
//
//	Returns:	platform enumeration (see GetVersionEx for details)
//
//	History:	8/8/96	ChrisK	Created
//
//-----------------------------------------------------------------------------
DWORD DWGetWin32Platform()
{
	OSVERSIONINFO osver;
	ZeroMemory(&osver,sizeof(osver));
	osver.dwOSVersionInfoSize = sizeof(osver);
	if (GetVersionEx(&osver))
		return osver.dwPlatformId;
	AssertSz(0,"GetVersionEx failed.\r\n");
	return 0;
}

//+----------------------------------------------------------------------------
//
//	Function:	DWGetWin32BuildNumber
//
//	Synopsis:	Return a value to determine win32 build
//
//	Arguements:	None
//
//	Returns:	build number
//
//	History:	9/26/96	ChrisK	Created
//
//-----------------------------------------------------------------------------
DWORD DWGetWin32BuildNumber()
{
	OSVERSIONINFO osver;
	ZeroMemory(&osver,sizeof(osver));
	osver.dwOSVersionInfoSize = sizeof(osver);
	if (GetVersionEx(&osver))
		// dwBuildNumber
		// Identifies the build number of the operating system in the low-order
		// word. (The high-order word contains the major and minor version numbers.)
		return (osver.dwBuildNumber & 0xFFFF);
	AssertSz(0,"GetVersionEx failed.\r\n");
	return 0;
}

#endif
