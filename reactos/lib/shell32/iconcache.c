/*
 *	shell icon cache (SIC)
 *
 * Copyright 1998, 1999 Juergen Schmied
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "heap.h"
#include "wine/debug.h"

#include "shellapi.h"
#include "objbase.h"
#include "shlguid.h"
#include "pidl.h"
#include "shell32_main.h"
#include "undocshell.h"
#include "shlwapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/********************** THE ICON CACHE ********************************/

#define INVALID_INDEX -1

typedef struct
{
	LPWSTR sSourceFile;	/* file (not path!) containing the icon */
	DWORD dwSourceIndex;	/* index within the file, if it is a resoure ID it will be negated */
	DWORD dwListIndex;	/* index within the iconlist */
	DWORD dwFlags;		/* GIL_* flags */
	DWORD dwAccessTime;
} SIC_ENTRY, * LPSIC_ENTRY;

static HDPA		sic_hdpa = 0;

static CRITICAL_SECTION SHELL32_SicCS;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &SHELL32_SicCS,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": SHELL32_SicCS") }
};
static CRITICAL_SECTION SHELL32_SicCS = { &critsect_debug, -1, 0, 0, 0, 0 };

/*****************************************************************************
 * SIC_CompareEntries
 *
 * NOTES
 *  Callback for DPA_Search
 */
static INT CALLBACK SIC_CompareEntries( LPVOID p1, LPVOID p2, LPARAM lparam)
{	TRACE("%p %p %8lx\n", p1, p2, lparam);

	if (((LPSIC_ENTRY)p1)->dwSourceIndex != ((LPSIC_ENTRY)p2)->dwSourceIndex) /* first the faster one*/
	  return 1;

	if (strcmpiW(((LPSIC_ENTRY)p1)->sSourceFile,((LPSIC_ENTRY)p2)->sSourceFile))
	  return 1;

	return 0;
}
/*****************************************************************************
 * SIC_IconAppend			[internal]
 *
 * NOTES
 *  appends an icon pair to the end of the cache
 */
static INT SIC_IconAppend (LPCWSTR sSourceFile, INT dwSourceIndex, HICON hSmallIcon, HICON hBigIcon)
{	LPSIC_ENTRY lpsice;
	INT ret, index, index1;
	WCHAR path[MAX_PATH];
	TRACE("%s %i %p %p\n", debugstr_w(sSourceFile), dwSourceIndex, hSmallIcon ,hBigIcon);

	lpsice = (LPSIC_ENTRY) SHAlloc (sizeof (SIC_ENTRY));

	GetFullPathNameW(sSourceFile, MAX_PATH, path, NULL);
	lpsice->sSourceFile = HeapAlloc( GetProcessHeap(), 0, (strlenW(path)+1)*sizeof(WCHAR) );
	strcpyW( lpsice->sSourceFile, path );

	lpsice->dwSourceIndex = dwSourceIndex;

	EnterCriticalSection(&SHELL32_SicCS);

	index = DPA_InsertPtr(sic_hdpa, 0x7fff, lpsice);
	if ( INVALID_INDEX == index )
	{
	  HeapFree(GetProcessHeap(), 0, lpsice->sSourceFile);
	  SHFree(lpsice);
	  ret = INVALID_INDEX;
	}
	else
	{
	  index = ImageList_AddIcon (ShellSmallIconList, hSmallIcon);
	  index1= ImageList_AddIcon (ShellBigIconList, hBigIcon);

	  if (index!=index1)
	  {
	    FIXME("iconlists out of sync 0x%x 0x%x\n", index, index1);
	  }
	  lpsice->dwListIndex = index;
	  ret = lpsice->dwListIndex;
	}

	LeaveCriticalSection(&SHELL32_SicCS);
	return ret;
}
/****************************************************************************
 * SIC_LoadIcon				[internal]
 *
 * NOTES
 *  gets small/big icon by number from a file
 */
static INT SIC_LoadIcon (LPCWSTR sSourceFile, INT dwSourceIndex)
{	HICON	hiconLarge=0;
	HICON	hiconSmall=0;

#if defined(__CYGWIN__) || defined (__MINGW32__) || defined(_MSC_VER)
	static UINT (WINAPI*PrivateExtractIconExW)(LPCWSTR,int,HICON*,HICON*,UINT) = NULL;

	if (!PrivateExtractIconExW) {
	    HMODULE hUser32 = GetModuleHandleA("user32");
	    PrivateExtractIconExW = (UINT(WINAPI*)(LPCWSTR,int,HICON*,HICON*,UINT)) GetProcAddress(hUser32, "PrivateExtractIconExW");
	}

        if (PrivateExtractIconExW)
	    PrivateExtractIconExW(sSourceFile, dwSourceIndex, &hiconLarge, &hiconSmall, 1);
	else
#endif
	{
	    PrivateExtractIconsW(sSourceFile, dwSourceIndex, 32, 32, &hiconLarge, NULL, 1, 0);
	    PrivateExtractIconsW(sSourceFile, dwSourceIndex, 16, 16, &hiconSmall, NULL, 1, 0);
	}

	if ( !hiconLarge ||  !hiconSmall)
	{
	  WARN("failure loading icon %i from %s (%p %p)\n", dwSourceIndex, debugstr_w(sSourceFile), hiconLarge, hiconSmall);
	  return -1;
	}
	return SIC_IconAppend (sSourceFile, dwSourceIndex, hiconSmall, hiconLarge);
}
/*****************************************************************************
 * SIC_GetIconIndex			[internal]
 *
 * Parameters
 *	sSourceFile	[IN]	filename of file containing the icon
 *	index		[IN]	index/resID (negated) in this file
 *
 * NOTES
 *  look in the cache for a proper icon. if not available the icon is taken
 *  from the file and cached
 */
INT SIC_GetIconIndex (LPCWSTR sSourceFile, INT dwSourceIndex )
{
	SIC_ENTRY sice;
	INT ret, index = INVALID_INDEX;
	WCHAR path[MAX_PATH];

	TRACE("%s %i\n", debugstr_w(sSourceFile), dwSourceIndex);

	GetFullPathNameW(sSourceFile, MAX_PATH, path, NULL);
	sice.sSourceFile = path;
	sice.dwSourceIndex = dwSourceIndex;

	EnterCriticalSection(&SHELL32_SicCS);

	if (NULL != DPA_GetPtr (sic_hdpa, 0))
	{
	  /* search linear from position 0*/
	  index = DPA_Search (sic_hdpa, &sice, 0, SIC_CompareEntries, 0, 0);
	}

	if ( INVALID_INDEX == index )
	{
	  ret = SIC_LoadIcon (sSourceFile, dwSourceIndex);
	}
	else
	{
	  TRACE("-- found\n");
	  ret = ((LPSIC_ENTRY)DPA_GetPtr(sic_hdpa, index))->dwListIndex;
	}

	LeaveCriticalSection(&SHELL32_SicCS);
	return ret;
}
/****************************************************************************
 * SIC_GetIcon				[internal]
 *
 * NOTES
 *  retrieves the specified icon from the iconcache.
 */
static HICON WINE_UNUSED SIC_GetIcon (LPCWSTR sSourceFile, INT dwSourceIndex, BOOL bSmallIcon )
{	INT index;

	TRACE("%s %i\n", debugstr_w(sSourceFile), dwSourceIndex);

	index = SIC_GetIconIndex(sSourceFile, dwSourceIndex);

	if (INVALID_INDEX == index)
	{
	  return (HICON)INVALID_INDEX;
	}

	if (bSmallIcon)
	  return ImageList_GetIcon(ShellSmallIconList, index, ILD_NORMAL);

	return ImageList_GetIcon(ShellBigIconList, index, ILD_NORMAL);
}
/*****************************************************************************
 * SIC_Initialize			[internal]
 *
 * NOTES
 *  hack to load the resources from the shell32.dll under a different dll name
 *  will be removed when the resource-compiler is ready
 */
BOOL SIC_Initialize(void)
{
	HICON		hSm, hLg;
	UINT		index;
	int		cx_small, cy_small;
	int		cx_large, cy_large;

	cx_small = GetSystemMetrics(SM_CXSMICON);
	cy_small = GetSystemMetrics(SM_CYSMICON);
	cx_large = GetSystemMetrics(SM_CXICON);
	cy_large = GetSystemMetrics(SM_CYICON);

	TRACE("\n");

	if (sic_hdpa)	/* already initialized?*/
	  return TRUE;

	sic_hdpa = DPA_Create(16);

	if (!sic_hdpa)
	{
	  return(FALSE);
	}

       ShellSmallIconList = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,0x20);
       ShellBigIconList = ImageList_Create(32,32,ILC_COLOR32|ILC_MASK,0,0x20);

       ImageList_SetBkColor(ShellSmallIconList, CLR_NONE);
       ImageList_SetBkColor(ShellBigIconList, CLR_NONE);

	for (index=1; index<39; index++)
	{
	  hSm = (HICON)LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(index), IMAGE_ICON, cx_small, cy_small, LR_SHARED);
	  hLg = (HICON)LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(index), IMAGE_ICON, cx_large, cy_large, LR_SHARED);

	  if(!hSm)
	  {
	    hSm = LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(1), IMAGE_ICON, cx_small, cy_small, LR_SHARED);
	    hLg = LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(1), IMAGE_ICON, cx_large, cy_large, LR_SHARED);
	  }
         SIC_IconAppend (swShell32Name, index, hSm, hLg);
	}

	TRACE("hIconSmall=%p hIconBig=%p\n",ShellSmallIconList, ShellBigIconList);

	return TRUE;
}
/*************************************************************************
 * SIC_Destroy
 *
 * frees the cache
 */
static INT CALLBACK sic_free( LPVOID ptr, LPVOID lparam )
{
	HeapFree(GetProcessHeap(), 0, ((LPSIC_ENTRY)ptr)->sSourceFile);
	SHFree(ptr);
	return TRUE;
}

void SIC_Destroy(void)
{
	TRACE("\n");

	EnterCriticalSection(&SHELL32_SicCS);

	if (sic_hdpa) DPA_DestroyCallback(sic_hdpa, sic_free, NULL );

	sic_hdpa = NULL;

	LeaveCriticalSection(&SHELL32_SicCS);
	DeleteCriticalSection(&SHELL32_SicCS);
}
/*************************************************************************
 * Shell_GetImageList			[SHELL32.71]
 *
 * PARAMETERS
 *  imglist[1|2] [OUT] pointer which recive imagelist handles
 *
 */
BOOL WINAPI Shell_GetImageList(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList)
{	TRACE("(%p,%p)\n",lpBigList,lpSmallList);
	if (lpBigList)
	{ *lpBigList = ShellBigIconList;
	}
	if (lpSmallList)
	{ *lpSmallList = ShellSmallIconList;
	}

	return TRUE;
}

/*************************************************************************
 * PidlToSicIndex			[INTERNAL]
 *
 * PARAMETERS
 *	sh	[IN]	IShellFolder
 *	pidl	[IN]
 *	bBigIcon [IN]
 *	uFlags	[IN]	GIL_*
 *	pIndex	[OUT]	index within the SIC
 *
 */
BOOL PidlToSicIndex (
	IShellFolder * sh,
	LPCITEMIDLIST pidl,
	BOOL bBigIcon,
	UINT uFlags,
	int * pIndex)
{
	IExtractIconW	*ei;
	WCHAR		szIconFile[MAX_PATH];	/* file containing the icon */
	INT		iSourceIndex;		/* index or resID(negated) in this file */
	BOOL		ret = FALSE;
	UINT		dwFlags = 0;

	TRACE("sf=%p pidl=%p %s\n", sh, pidl, bBigIcon?"Big":"Small");

	if (SUCCEEDED (IShellFolder_GetUIObjectOf(sh, 0, 1, &pidl, &IID_IExtractIconW, 0, (void **)&ei)))
	{
	  if (SUCCEEDED(IExtractIconW_GetIconLocation(ei, uFlags, szIconFile, MAX_PATH, &iSourceIndex, &dwFlags)))
	  {
	    *pIndex = SIC_GetIconIndex(szIconFile, iSourceIndex);
	    ret = TRUE;
	  }
	  IExtractIconW_Release(ei);
	}

	if (INVALID_INDEX == *pIndex)	/* default icon when failed */
	  *pIndex = 0;

	return ret;
}

/*************************************************************************
 * SHMapPIDLToSystemImageListIndex	[SHELL32.77]
 *
 * PARAMETERS
 *	sh	[IN]		pointer to an instance of IShellFolder
 *	pidl	[IN]
 *	pIndex	[OUT][OPTIONAL]	SIC index for big icon
 *
 */
int WINAPI SHMapPIDLToSystemImageListIndex(
	IShellFolder *sh,
	LPCITEMIDLIST pidl,
	int *pIndex)
{
	int Index;

	TRACE("(SF=%p,pidl=%p,%p)\n",sh,pidl,pIndex);
	pdump(pidl);

	if (pIndex)
	    if (!PidlToSicIndex ( sh, pidl, 1, 0, pIndex))
		*pIndex = -1;	   

	if (!PidlToSicIndex ( sh, pidl, 0, 0, &Index))
	    return -1;

	return Index;
}

/*************************************************************************
 * Shell_GetCachedImageIndex		[SHELL32.72]
 *
 */
INT WINAPI Shell_GetCachedImageIndexA(LPCSTR szPath, INT nIndex, BOOL bSimulateDoc)
{
	INT ret, len;
	LPWSTR szTemp;

	WARN("(%s,%08x,%08x) semi-stub.\n",debugstr_a(szPath), nIndex, bSimulateDoc);

	len = MultiByteToWideChar( CP_ACP, 0, szPath, -1, NULL, 0 );
	szTemp = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
	MultiByteToWideChar( CP_ACP, 0, szPath, -1, szTemp, len );

	ret = SIC_GetIconIndex( szTemp, nIndex );

	HeapFree( GetProcessHeap(), 0, szTemp );

	return ret;
}

INT WINAPI Shell_GetCachedImageIndexW(LPCWSTR szPath, INT nIndex, BOOL bSimulateDoc)
{
	WARN("(%s,%08x,%08x) semi-stub.\n",debugstr_w(szPath), nIndex, bSimulateDoc);

	return SIC_GetIconIndex(szPath, nIndex);
}

INT WINAPI Shell_GetCachedImageIndexAW(LPCVOID szPath, INT nIndex, BOOL bSimulateDoc)
{	if( SHELL_OsIsUnicode())
	  return Shell_GetCachedImageIndexW(szPath, nIndex, bSimulateDoc);
	return Shell_GetCachedImageIndexA(szPath, nIndex, bSimulateDoc);
}

/*************************************************************************
 * ExtractIconEx			[SHELL32.@]
 */
UINT WINAPI ExtractIconExAW(LPCVOID lpszFile, INT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIcons)
{	if (SHELL_OsIsUnicode())
	  return ExtractIconExW ( lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
	return ExtractIconExA ( lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}

/*************************************************************************
 * ExtractIconExW			[SHELL32.@]
 * RETURNS: 
 *  0 no icon found
 *  -1 file is not valid
 *  or number of icons extracted
 */
UINT WINAPI ExtractIconExW(LPCWSTR lpszFile, INT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIcons)
{
	/* get entry point of undocumented function PrivateExtractIconExW() in user32 */
#if defined(__CYGWIN__) || defined (__MINGW32__) || defined(_MSC_VER)
	static UINT (WINAPI*PrivateExtractIconExW)(LPCWSTR,int,HICON*,HICON*,UINT) = NULL;

	if (!PrivateExtractIconExW) {
	    HMODULE hUser32 = GetModuleHandleA("user32");
	    PrivateExtractIconExW = (UINT(WINAPI*)(LPCWSTR,int,HICON*,HICON*,UINT)) GetProcAddress(hUser32, "PrivateExtractIconExW");

	    if (!PrivateExtractIconExW)
		return 0;
	}
#endif

	TRACE("%s %i %p %p %i\n", debugstr_w(lpszFile), nIconIndex, phiconLarge, phiconSmall, nIcons);

	return PrivateExtractIconExW(lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}

/*************************************************************************
 * ExtractIconExA			[SHELL32.@]
 */
UINT WINAPI ExtractIconExA(LPCSTR lpszFile, INT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIcons)
{
    UINT ret;
    INT len = MultiByteToWideChar(CP_ACP, 0, lpszFile, -1, NULL, 0);
    LPWSTR lpwstrFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

    TRACE("%s %i %p %p %i\n", lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);

    MultiByteToWideChar(CP_ACP, 0, lpszFile, -1, lpwstrFile, len);
    ret = ExtractIconExW (lpwstrFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
    HeapFree(GetProcessHeap(), 0, lpwstrFile);
    return ret;
}

/*************************************************************************
 *				ExtractAssociatedIconA (SHELL32.@)
 *
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON WINAPI ExtractAssociatedIconA(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIcon)
{	
	HICON hIcon;
	WORD wDummyIcon = 0;
	
	TRACE("\n");

	if(lpiIcon == NULL)
	    lpiIcon = &wDummyIcon;

	hIcon = ExtractIconA(hInst, lpIconPath, *lpiIcon);

	if( hIcon < (HICON)2 )
	{ if( hIcon == (HICON)1 ) /* no icons found in given file */
	  { char  tempPath[0x80];
	    HINSTANCE uRet = FindExecutableA(lpIconPath,NULL,tempPath);

	    if( uRet > (HINSTANCE)32 && tempPath[0] )
	    { strcpy(lpIconPath,tempPath);
	      hIcon = ExtractIconA(hInst, lpIconPath, *lpiIcon);
	      if( hIcon > (HICON)2 )
	        return hIcon;
	    }
	    else hIcon = 0;
	  }

	  if( hIcon == (HICON)1 )
	    *lpiIcon = 2;   /* MSDOS icon - we found .exe but no icons in it */
	  else
	    *lpiIcon = 6;   /* generic icon - found nothing */

	  GetModuleFileNameA(hInst, lpIconPath, 0x80);
	  hIcon = LoadIconA( hInst, MAKEINTRESOURCEA(*lpiIcon));
	}
	return hIcon;
}

/*************************************************************************
 *				ExtractAssociatedIconExW (SHELL32.@)
 *
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON WINAPI ExtractAssociatedIconExW(HINSTANCE hInst, LPWSTR lpIconPath, LPWORD lpiIconIdx, LPWORD lpiIconId)
{
  FIXME("%p %s %p %p): stub\n", hInst, debugstr_w(lpIconPath), lpiIconIdx, lpiIconId);
  return 0;
}

/*************************************************************************
 *				ExtractAssociatedIconExA (SHELL32.@)
 *
 * Return icon for given file (either from file itself or from associated
 * executable) and patch parameters if needed.
 */
HICON WINAPI ExtractAssociatedIconExA(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIconIdx, LPWORD lpiIconId)
{
  HICON ret;
  INT len = MultiByteToWideChar( CP_ACP, 0, lpIconPath, -1, NULL, 0 );
  LPWSTR lpwstrFile = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );

  TRACE("%p %s %p %p)\n", hInst, lpIconPath, lpiIconIdx, lpiIconId);

  MultiByteToWideChar( CP_ACP, 0, lpIconPath, -1, lpwstrFile, len );
  ret = ExtractAssociatedIconExW(hInst, lpwstrFile, lpiIconIdx, lpiIconId);
  HeapFree(GetProcessHeap(), 0, lpwstrFile);
  return ret;
}


/****************************************************************************
 * SHDefExtractIconW		[SHELL32.@]
 */
HRESULT WINAPI SHDefExtractIconW(LPCWSTR pszIconFile, int iIndex, UINT uFlags,
                                 HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize)
{
	UINT ret;
	HICON hIcons[2];
	WARN("%s %d 0x%08x %p %p %d, semi-stub\n", debugstr_w(pszIconFile), iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);

	ret = PrivateExtractIconsW(pszIconFile, iIndex, nIconSize, nIconSize, hIcons, NULL, 2, LR_DEFAULTCOLOR);
	/* FIXME: deal with uFlags parameter which contains GIL_ flags */
	if (ret == 0xFFFFFFFF)
	  return E_FAIL;
	if (ret > 0) {
	  *phiconLarge = hIcons[0];
	  *phiconSmall = hIcons[1];
	  return S_OK;
	}
	return S_FALSE;
}

/****************************************************************************
 * SHDefExtractIconA		[SHELL32.@]
 */
HRESULT WINAPI SHDefExtractIconA(LPCSTR pszIconFile, int iIndex, UINT uFlags,
                                 HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize)
{
  HRESULT ret;
  INT len = MultiByteToWideChar(CP_ACP, 0, pszIconFile, -1, NULL, 0);
  LPWSTR lpwstrFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

  TRACE("%s %d 0x%08x %p %p %d\n", pszIconFile, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);

  MultiByteToWideChar(CP_ACP, 0, pszIconFile, -1, lpwstrFile, len);
  ret = SHDefExtractIconW(lpwstrFile, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);
  HeapFree(GetProcessHeap(), 0, lpwstrFile);
  return ret;
}
