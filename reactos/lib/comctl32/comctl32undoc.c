/*
 * Undocumented functions from COMCTL32.DLL
 *
 * Copyright 1998 Eric Kohl
 *           1998 Juergen Schmied <j.schmied@metronet.de>
 *           2000 Eric Kohl for CodeWeavers
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
 *
 * NOTES
 *     All of these functions are UNDOCUMENTED!! And I mean UNDOCUMENTED!!!!
 *     Do NOT rely on names or contents of undocumented structures and types!!!
 *     These functions are used by EXPLORER.EXE, IEXPLORE.EXE and
 *     COMCTL32.DLL (internally).
 *
 * TODO
 *     - Add more functions.
 *     - Write some documentation.
 */
#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h> /* atoi */
#include <ctype.h>
#include <limits.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "commctrl.h"
#include "objbase.h"
#include "winerror.h"

#include "wine/unicode.h"
#include "comctl32.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);

struct _DSA
{
    INT  nItemCount;
    LPVOID pData;
    INT  nMaxCount;
    INT  nItemSize;
    INT  nGrow;
};

struct _DPA
{
    INT    nItemCount;
    LPVOID   *ptrs;
    HANDLE hHeap;
    INT    nGrow;
    INT    nMaxCount;
};

typedef struct _STREAMDATA
{
    DWORD dwSize;
    DWORD dwData2;
    DWORD dwItems;
} STREAMDATA, *PSTREAMDATA;

typedef struct _LOADDATA
{
    INT   nCount;
    PVOID ptr;
} LOADDATA, *LPLOADDATA;

typedef HRESULT (CALLBACK *DPALOADPROC)(LPLOADDATA,IStream*,LPARAM);

/**************************************************************************
 * DPA_LoadStream [COMCTL32.9]
 *
 * Loads a dynamic pointer array from a stream
 *
 * PARAMS
 *     phDpa    [O] pointer to a handle to a dynamic pointer array
 *     loadProc [I] pointer to a callback function
 *     pStream  [I] pointer to a stream
 *     lParam   [I] application specific value
 *
 * NOTES
 *     No more information available yet!
 */

HRESULT WINAPI
DPA_LoadStream (HDPA *phDpa, DPALOADPROC loadProc, IStream *pStream, LPARAM lParam)
{
    HRESULT errCode;
    LARGE_INTEGER position;
    ULARGE_INTEGER newPosition;
    STREAMDATA  streamData;
    LOADDATA loadData;
    ULONG ulRead;
    HDPA hDpa;
    PVOID *ptr;

    FIXME ("phDpa=%p loadProc=%p pStream=%p lParam=%lx\n",
	   phDpa, loadProc, pStream, lParam);

    if (!phDpa || !loadProc || !pStream)
	return E_INVALIDARG;

    *phDpa = (HDPA)NULL;

    position.QuadPart = 0;

    /*
     * Zero out our streamData
     */
    memset(&streamData,0,sizeof(STREAMDATA));

    errCode = IStream_Seek (pStream, position, STREAM_SEEK_CUR, &newPosition);
    if (errCode != S_OK)
	return errCode;

    errCode = IStream_Read (pStream, &streamData, sizeof(STREAMDATA), &ulRead);
    if (errCode != S_OK)
	return errCode;

    FIXME ("dwSize=%lu dwData2=%lu dwItems=%lu\n",
	   streamData.dwSize, streamData.dwData2, streamData.dwItems);

    if ( ulRead < sizeof(STREAMDATA) ||
    lParam < sizeof(STREAMDATA) ||
	streamData.dwSize < sizeof(STREAMDATA) ||
	streamData.dwData2 < 1) {
	errCode = E_FAIL;
    }

    if (streamData.dwItems > (UINT_MAX / 2 / sizeof(VOID*))) /* 536870911 */
        return E_OUTOFMEMORY;

    /* create the dpa */
    hDpa = DPA_Create (streamData.dwItems);
    if (!hDpa)
	return E_OUTOFMEMORY;

    if (!DPA_Grow (hDpa, streamData.dwItems))
	return E_OUTOFMEMORY;

    /* load data from the stream into the dpa */
    ptr = hDpa->ptrs;
    for (loadData.nCount = 0; loadData.nCount < streamData.dwItems; loadData.nCount++) {
        errCode = (loadProc)(&loadData, pStream, lParam);
	if (errCode != S_OK) {
	    errCode = S_FALSE;
	    break;
	}

	*ptr = loadData.ptr;
	ptr++;
    }

    /* set the number of items */
    hDpa->nItemCount = loadData.nCount;

    /* store the handle to the dpa */
    *phDpa = hDpa;
    FIXME ("new hDpa=%p, errorcode=%lx\n", hDpa, errCode);

    return errCode;
}


/**************************************************************************
 * DPA_SaveStream [COMCTL32.10]
 *
 * Saves a dynamic pointer array to a stream
 *
 * PARAMS
 *     hDpa     [I] handle to a dynamic pointer array
 *     loadProc [I] pointer to a callback function
 *     pStream  [I] pointer to a stream
 *     lParam   [I] application specific value
 *
 * NOTES
 *     No more information available yet!
 */

HRESULT WINAPI
DPA_SaveStream (const HDPA hDpa, DPALOADPROC loadProc, IStream *pStream, LPARAM lParam)
{

    FIXME ("hDpa=%p loadProc=%p pStream=%p lParam=%lx\n",
	   hDpa, loadProc, pStream, lParam);

    return E_FAIL;
}


/**************************************************************************
 * DPA_Merge [COMCTL32.11]
 *
 * PARAMS
 *     hdpa1       [I] handle to a dynamic pointer array
 *     hdpa2       [I] handle to a dynamic pointer array
 *     dwFlags     [I] flags
 *     pfnCompare  [I] pointer to sort function
 *     pfnMerge    [I] pointer to merge function
 *     lParam      [I] application specific value
 *
 * NOTES
 *     No more information available yet!
 */

BOOL WINAPI
DPA_Merge (const HDPA hdpa1, const HDPA hdpa2, DWORD dwFlags,
	   PFNDPACOMPARE pfnCompare, PFNDPAMERGE pfnMerge, LPARAM lParam)
{
    INT nCount;
    LPVOID *pWork1, *pWork2;
    INT nResult, i;
    INT nIndex;

    TRACE("%p %p %08lx %p %p %08lx)\n",
	   hdpa1, hdpa2, dwFlags, pfnCompare, pfnMerge, lParam);

    if (IsBadWritePtr (hdpa1, sizeof(*hdpa1)))
	return FALSE;

    if (IsBadWritePtr (hdpa2, sizeof(*hdpa2)))
	return FALSE;

    if (IsBadCodePtr ((FARPROC)pfnCompare))
	return FALSE;

    if (IsBadCodePtr ((FARPROC)pfnMerge))
	return FALSE;

    if (!(dwFlags & DPAM_NOSORT)) {
	TRACE("sorting dpa's!\n");
	if (hdpa1->nItemCount > 0)
	DPA_Sort (hdpa1, pfnCompare, lParam);
	TRACE ("dpa 1 sorted!\n");
	if (hdpa2->nItemCount > 0)
	DPA_Sort (hdpa2, pfnCompare, lParam);
	TRACE ("dpa 2 sorted!\n");
    }

    if (hdpa2->nItemCount < 1)
	return TRUE;

    TRACE("hdpa1->nItemCount=%d hdpa2->nItemCount=%d\n",
	   hdpa1->nItemCount, hdpa2->nItemCount);


    /* working but untrusted implementation */

    pWork1 = &(hdpa1->ptrs[hdpa1->nItemCount - 1]);
    pWork2 = &(hdpa2->ptrs[hdpa2->nItemCount - 1]);

    nIndex = hdpa1->nItemCount - 1;
    nCount = hdpa2->nItemCount - 1;

    do
    {
        if (nIndex < 0) {
	    if ((nCount >= 0) && (dwFlags & DPAM_INSERT)) {
		/* Now insert the remaining new items into DPA 1 */
		TRACE("%d items to be inserted at start of DPA 1\n",
		      nCount+1);
		for (i=nCount; i>=0; i--) {
		    PVOID ptr;

		    ptr = (pfnMerge)(3, *pWork2, NULL, lParam);
		    if (!ptr)
			return FALSE;
		    DPA_InsertPtr (hdpa1, 0, ptr);
		    pWork2--;
		}
	    }
	    break;
	}
	nResult = (pfnCompare)(*pWork1, *pWork2, lParam);
	TRACE("compare result=%d, dpa1.cnt=%d, dpa2.cnt=%d\n",
	      nResult, nIndex, nCount);

	if (nResult == 0)
	{
	    PVOID ptr;

	    ptr = (pfnMerge)(1, *pWork1, *pWork2, lParam);
	    if (!ptr)
		return FALSE;

	    nCount--;
	    pWork2--;
	    *pWork1 = ptr;
	    nIndex--;
	    pWork1--;
	}
	else if (nResult > 0)
	{
	    /* item in DPA 1 missing from DPA 2 */
	    if (dwFlags & DPAM_DELETE)
	    {
		/* Now delete the extra item in DPA1 */
		PVOID ptr;

		ptr = DPA_DeletePtr (hdpa1, hdpa1->nItemCount - 1);

		(pfnMerge)(2, ptr, NULL, lParam);
	    }
	    nIndex--;
	    pWork1--;
	}
	else
	{
	    /* new item in DPA 2 */
	    if (dwFlags & DPAM_INSERT)
	    {
		/* Now insert the new item in DPA 1 */
		PVOID ptr;

		ptr = (pfnMerge)(3, *pWork2, NULL, lParam);
		if (!ptr)
		    return FALSE;
		DPA_InsertPtr (hdpa1, nIndex+1, ptr);
	    }
	    nCount--;
	    pWork2--;
	}

    }
    while (nCount >= 0);

    return TRUE;
}


/**************************************************************************
 * Alloc [COMCTL32.71]
 *
 * Allocates memory block from the dll's private heap
 *
 * PARAMS
 *     dwSize [I] size of the allocated memory block
 *
 * RETURNS
 *     Success: pointer to allocated memory block
 *     Failure: NULL
 */

LPVOID WINAPI Alloc (DWORD dwSize)
{
    return LocalAlloc( LMEM_ZEROINIT, dwSize );
}


/**************************************************************************
 * ReAlloc [COMCTL32.72]
 *
 * Changes the size of an allocated memory block or allocates a memory
 * block using the dll's private heap.
 *
 * PARAMS
 *     lpSrc  [I] pointer to memory block which will be resized
 *     dwSize [I] new size of the memory block.
 *
 * RETURNS
 *     Success: pointer to the resized memory block
 *     Failure: NULL
 *
 * NOTES
 *     If lpSrc is a NULL-pointer, then ReAlloc allocates a memory
 *     block like Alloc.
 */

LPVOID WINAPI ReAlloc (LPVOID lpSrc, DWORD dwSize)
{
    if (lpSrc)
        return LocalReAlloc( lpSrc, dwSize, LMEM_ZEROINIT );
    else
        return LocalAlloc( LMEM_ZEROINIT, dwSize);
}


/**************************************************************************
 * Free [COMCTL32.73]
 *
 * Frees an allocated memory block from the dll's private heap.
 *
 * PARAMS
 *     lpMem [I] pointer to memory block which will be freed
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI Free (LPVOID lpMem)
{
    return !LocalFree( lpMem );
}


/**************************************************************************
 * GetSize [COMCTL32.74]
 *
 * Retrieves the size of the specified memory block from the dll's
 * private heap.
 *
 * PARAMS
 *     lpMem [I] pointer to an allocated memory block
 *
 * RETURNS
 *     Success: size of the specified memory block
 *     Failure: 0
 */

DWORD WINAPI GetSize (LPVOID lpMem)
{
    return LocalSize( lpMem );
}


/**************************************************************************
 * The MRU-API is a set of functions to manipulate MRU(Most Recently Used)
 * lists.
 *
 * Stored in the reg. as a set of values under a single key.  Each item in the
 * list has a value name that is a single char. 'a' - 'z', '{', '|' or '}'.
 * The order of the list is stored with value name 'MRUList' which is a string
 * containing the value names (i.e. 'a', 'b', etc.) in the relevant order.
 */

typedef struct tagCREATEMRULISTA
{
    DWORD  cbSize;        /* size of struct */
    DWORD  nMaxItems;     /* max no. of items in list */
    DWORD  dwFlags;       /* see below */
    HKEY   hKey;          /* root reg. key under which list is saved */
    LPCSTR lpszSubKey;    /* reg. subkey */
    PROC   lpfnCompare;   /* item compare proc */
} CREATEMRULISTA, *LPCREATEMRULISTA;

typedef struct tagCREATEMRULISTW
{
    DWORD   cbSize;        /* size of struct */
    DWORD   nMaxItems;     /* max no. of items in list */
    DWORD   dwFlags;       /* see below */
    HKEY    hKey;          /* root reg. key under which list is saved */
    LPCWSTR lpszSubKey;    /* reg. subkey */
    PROC    lpfnCompare;   /* item compare proc */
} CREATEMRULISTW, *LPCREATEMRULISTW;

/* dwFlags */
#define MRUF_STRING_LIST  0 /* list will contain strings */
#define MRUF_BINARY_LIST  1 /* list will contain binary data */
#define MRUF_DELAYED_SAVE 2 /* only save list order to reg. is FreeMRUList */

/* If list is a string list lpfnCompare has the following prototype
 * int CALLBACK MRUCompareString(LPCSTR s1, LPCSTR s2)
 * for binary lists the prototype is
 * int CALLBACK MRUCompareBinary(LPCVOID data1, LPCVOID data2, DWORD cbData)
 * where cbData is the no. of bytes to compare.
 * Need to check what return value means identical - 0?
 */

typedef struct tagWINEMRUITEM
{
    DWORD          size;        /* size of data stored               */
    DWORD          itemFlag;    /* flags                             */
    BYTE           datastart;
} WINEMRUITEM, *LPWINEMRUITEM;

/* itemFlag */
#define WMRUIF_CHANGED   0x0001 /* this dataitem changed             */

typedef struct tagWINEMRULIST
{
    CREATEMRULISTW extview;     /* original create information       */
    BOOL           isUnicode;   /* is compare fn Unicode */
    DWORD          wineFlags;   /* internal flags                    */
    DWORD          cursize;     /* current size of realMRU           */
    LPSTR          realMRU;     /* pointer to string of index names  */
    LPWINEMRUITEM  *array;      /* array of pointers to data         */
                                /* in 'a' to 'z' order               */
} WINEMRULIST, *LPWINEMRULIST;

/* wineFlags */
#define WMRUF_CHANGED  0x0001   /* MRU list has changed              */

/**************************************************************************
 *              MRU_SaveChanged - Localize MRU saving code
 *
 */
VOID MRU_SaveChanged( LPWINEMRULIST mp )
{
    UINT i, err;
    HKEY newkey;
    WCHAR realname[2];
    LPWINEMRUITEM witem;
    WCHAR emptyW[] = {'\0'};

    /* or should we do the following instead of RegOpenKeyEx:
     */

    /* open the sub key */
    if ((err = RegOpenKeyExW( mp->extview.hKey, mp->extview.lpszSubKey,
			      0, KEY_WRITE, &newkey))) {
	/* not present - what to do ??? */
	ERR("Can not open key, error=%d, attempting to create\n",
	    err);
	if ((err = RegCreateKeyExW( mp->extview.hKey, mp->extview.lpszSubKey,
				    0,
				    emptyW,
				    REG_OPTION_NON_VOLATILE,
				    KEY_READ | KEY_WRITE,
				    0,
				    &newkey,
				    0))) {
	    ERR("failed to create key /%s/, err=%d\n",
		debugstr_w(mp->extview.lpszSubKey), err);
	    return;
	}
    }
    if (mp->wineFlags & WMRUF_CHANGED) {
	mp->wineFlags &= ~WMRUF_CHANGED;
	err = RegSetValueExA(newkey, "MRUList", 0, REG_SZ,
			     mp->realMRU, strlen(mp->realMRU) + 1);
	if (err) {
	    ERR("error saving MRUList, err=%d\n", err);
	}
	TRACE("saving MRUList=/%s/\n", mp->realMRU);
    }
    realname[1] = 0;
    for(i=0; i<mp->cursize; i++) {
	witem = mp->array[i];
	if (witem->itemFlag & WMRUIF_CHANGED) {
	    witem->itemFlag &= ~WMRUIF_CHANGED;
	    realname[0] = 'a' + i;
	    err = RegSetValueExW(newkey, realname, 0,
				 (mp->extview.dwFlags & MRUF_BINARY_LIST) ?
				 REG_BINARY : REG_SZ,
				 &witem->datastart, witem->size);
	    if (err) {
		ERR("error saving /%s/, err=%d\n", debugstr_w(realname), err);
	    }
	    TRACE("saving value for name /%s/ size=%ld\n",
		  debugstr_w(realname), witem->size);
	}
    }
    RegCloseKey( newkey );
}

/**************************************************************************
 *              FreeMRUList [COMCTL32.152]
 *
 * PARAMS
 *     hMRUList [I] Handle to list.
 *
 */
DWORD WINAPI
FreeMRUList (HANDLE hMRUList)
{
    LPWINEMRULIST mp = (LPWINEMRULIST)hMRUList;
    UINT i;

    TRACE("\n");
    if (mp->wineFlags & WMRUF_CHANGED) {
	/* need to open key and then save the info */
	MRU_SaveChanged( mp );
    }

    for(i=0; i<mp->extview.nMaxItems; i++) {
	if (mp->array[i])
	    Free(mp->array[i]);
    }
    Free(mp->realMRU);
    Free(mp->array);
    Free((LPWSTR)mp->extview.lpszSubKey);
    return Free(mp);
}


/**************************************************************************
 *                  FindMRUData [COMCTL32.169]
 *
 * Searches binary list for item that matches lpData of length cbData.
 * Returns position in list order 0 -> MRU and if lpRegNum != NULL then value
 * corresponding to item's reg. name will be stored in it ('a' -> 0).
 *
 * PARAMS
 *    hList [I] list handle
 *    lpData [I] data to find
 *    cbData [I] length of data
 *    lpRegNum [O] position in registry (maybe NULL)
 *
 * RETURNS
 *    Position in list 0 -> MRU.  -1 if item not found.
 */
INT WINAPI
FindMRUData (HANDLE hList, LPCVOID lpData, DWORD cbData, LPINT lpRegNum)
{
    LPWINEMRULIST mp = (LPWINEMRULIST)hList;
    UINT i, ret;
    LPSTR dataA = NULL;

    if (!mp->extview.lpfnCompare) {
	ERR("MRU list not properly created. No compare procedure.\n");
	return -1;
    }

    if(!(mp->extview.dwFlags & MRUF_BINARY_LIST) && !mp->isUnicode) {
        DWORD len = WideCharToMultiByte(CP_ACP, 0, lpData, -1,
					NULL, 0, NULL, NULL);
	dataA = Alloc(len);
	WideCharToMultiByte(CP_ACP, 0, lpData, -1, dataA, len, NULL, NULL);
    }

    for(i=0; i<mp->cursize; i++) {
	if (mp->extview.dwFlags & MRUF_BINARY_LIST) {
	    if (!mp->extview.lpfnCompare(lpData, &mp->array[i]->datastart,
					 cbData))
		break;
	}
	else {
	    if(mp->isUnicode) {
	        if (!mp->extview.lpfnCompare(lpData, &mp->array[i]->datastart))
		    break;
	    } else {
	        DWORD len = WideCharToMultiByte(CP_ACP, 0,
						(LPWSTR)&mp->array[i]->datastart, -1,
						NULL, 0, NULL, NULL);
		LPSTR itemA = Alloc(len);
		INT cmp;
		WideCharToMultiByte(CP_ACP, 0, (LPWSTR)&mp->array[i]->datastart, -1,
				    itemA, len, NULL, NULL);

	        cmp = mp->extview.lpfnCompare(dataA, itemA);
		Free(itemA);
		if(!cmp)
		    break;
	    }
	}
    }
    if(dataA)
        Free(dataA);
    if (i < mp->cursize)
	ret = i;
    else
	ret = -1;
    if (lpRegNum && (ret != -1))
	*lpRegNum = 'a' + i;

    TRACE("(%p, %p, %ld, %p) returning %d\n",
	   hList, lpData, cbData, lpRegNum, ret);

    return ret;
}


/**************************************************************************
 *              AddMRUData [COMCTL32.167]
 *
 * Add item to MRU binary list.  If item already exists in list then it is
 * simply moved up to the top of the list and not added again.  If list is
 * full then the least recently used item is removed to make room.
 *
 * PARAMS
 *     hList [I] Handle to list.
 *     lpData [I] ptr to data to add.
 *     cbData [I] no. of bytes of data.
 *
 * RETURNS
 *     No. corresponding to registry name where value is stored 'a' -> 0 etc.
 *     -1 on error.
 */
INT WINAPI
AddMRUData (HANDLE hList, LPCVOID lpData, DWORD cbData)
{
    LPWINEMRULIST mp = (LPWINEMRULIST)hList;
    LPWINEMRUITEM witem;
    INT i, replace, ret;

    if ((replace = FindMRUData (hList, lpData, cbData, NULL)) < 0) {
	/* either add a new entry or replace oldest */
	if (mp->cursize < mp->extview.nMaxItems) {
	    /* Add in a new item */
	    replace = mp->cursize;
	    mp->cursize++;
	}
	else {
	    /* get the oldest entry and replace data */
	    replace = mp->realMRU[mp->cursize - 1] - 'a';
	    Free(mp->array[replace]);
	}
    }
    else {
	/* free up the old data */
	Free(mp->array[replace]);
    }

    /* Allocate space for new item and move in the data */
    mp->array[replace] = witem = Alloc(cbData + sizeof(WINEMRUITEM));
    witem->itemFlag |= WMRUIF_CHANGED;
    witem->size = cbData;
    memcpy( &witem->datastart, lpData, cbData);

    /* now rotate MRU list */
    mp->wineFlags |= WMRUF_CHANGED;
    for(i=mp->cursize-1; i>=1; i--) {
	mp->realMRU[i] = mp->realMRU[i-1];
    }
    mp->realMRU[0] = replace + 'a';
    TRACE("(%p, %p, %ld) adding data, /%c/ now most current\n",
	  hList, lpData, cbData, replace+'a');
    ret = replace;

    if (!(mp->extview.dwFlags & MRUF_DELAYED_SAVE)) {
	/* save changed stuff right now */
	MRU_SaveChanged( mp );
    }

    return ret;
}

/**************************************************************************
 *              AddMRUStringW [COMCTL32.401]
 *
 * Add item to MRU string list.  If item already exists in list them it is
 * simply moved up to the top of the list and not added again.  If list is
 * full then the least recently used item is removed to make room.
 *
 * PARAMS
 *     hList [I] Handle to list.
 *     lpszString [I] ptr to string to add.
 *
 * RETURNS
 *     No. corresponding to registry name where value is stored 'a' -> 0 etc.
 *     -1 on error.
 */
INT WINAPI
AddMRUStringW(HANDLE hList, LPCWSTR lpszString)
{
    FIXME("(%p, %s) empty stub!\n", hList, debugstr_w(lpszString));

    return 0;
}

/**************************************************************************
 *              AddMRUStringA [COMCTL32.153]
 */
INT WINAPI
AddMRUStringA(HANDLE hList, LPCSTR lpszString)
{
    FIXME("(%p, %s) empty stub!\n", hList, debugstr_a(lpszString));

    return 0;
}

/**************************************************************************
 *              DelMRUString [COMCTL32.156]
 *
 * Removes item from either string or binary list (despite its name)
 *
 * PARAMS
 *    hList [I] list handle
 *    nItemPos [I] item position to remove 0 -> MRU
 *
 * RETURNS
 *    TRUE if successful, FALSE if nItemPos is out of range.
 */
BOOL WINAPI
DelMRUString(HANDLE hList, INT nItemPos)
{
    FIXME("(%p, %d): stub\n", hList, nItemPos);
    return TRUE;
}

/**************************************************************************
 *                  FindMRUStringW [COMCTL32.402]
 */
INT WINAPI
FindMRUStringW (HANDLE hList, LPCWSTR lpszString, LPINT lpRegNum)
{
  FIXME("stub\n");
  return -1;
}

/**************************************************************************
 *                  FindMRUStringA [COMCTL32.155]
 *
 * Searches string list for item that matches lpszString.
 * Returns position in list order 0 -> MRU and if lpRegNum != NULL then value
 * corresponding to item's reg. name will be stored in it ('a' -> 0).
 *
 * PARAMS
 *    hList [I] list handle
 *    lpszString [I] string to find
 *    lpRegNum [O] position in registry (maybe NULL)
 *
 * RETURNS
 *    Position in list 0 -> MRU.  -1 if item not found.
 */
INT WINAPI
FindMRUStringA (HANDLE hList, LPCSTR lpszString, LPINT lpRegNum)
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, lpszString, -1, NULL, 0);
    LPWSTR stringW = Alloc(len * sizeof(WCHAR));
    INT ret;

    MultiByteToWideChar(CP_ACP, 0, lpszString, -1, stringW, len);
    ret = FindMRUData(hList, stringW, len * sizeof(WCHAR), lpRegNum);
    Free(stringW);
    return ret;
}

/*************************************************************************
 *                 CreateMRUListLazy_common
 */
HANDLE CreateMRUListLazy_common(LPWINEMRULIST mp)
{
    UINT i, err;
    HKEY newkey;
    DWORD datasize, dwdisp;
    WCHAR realname[2];
    LPWINEMRUITEM witem;
    DWORD type;
    WCHAR emptyW[] = {'\0'};

    /* get space to save indices that will turn into names
     * but in order of most to least recently used
     */
    mp->realMRU = Alloc(mp->extview.nMaxItems + 2);

    /* get space to save pointers to actual data in order of
     * 'a' to 'z' (0 to n).
     */
    mp->array = Alloc(mp->extview.nMaxItems * sizeof(LPVOID));

    /* open the sub key */
    if ((err = RegCreateKeyExW( mp->extview.hKey, mp->extview.lpszSubKey,
			        0,
				emptyW,
				REG_OPTION_NON_VOLATILE,
				KEY_READ | KEY_WRITE,
                                0,
				&newkey,
				&dwdisp))) {
	/* error - what to do ??? */
	ERR("(%lu %lu %lx %lx \"%s\" %p): Can not open key, error=%d\n",
	    mp->extview.cbSize, mp->extview.nMaxItems, mp->extview.dwFlags,
	    (DWORD)mp->extview.hKey, debugstr_w(mp->extview.lpszSubKey),
				 mp->extview.lpfnCompare, err);
	return 0;
    }

    /* get values from key 'MRUList' */
    if (newkey) {
	datasize = mp->extview.nMaxItems + 1;
	if((err=RegQueryValueExA( newkey, "MRUList", 0, &type, mp->realMRU,
				  &datasize))) {
	    /* not present - set size to 1 (will become 0 later) */
	    datasize = 1;
	    *mp->realMRU = 0;
	}

	TRACE("MRU list = %s\n", mp->realMRU);

	mp->cursize = datasize - 1;
	/* datasize now has number of items in the MRUList */

	/* get actual values for each entry */
	realname[1] = 0;
	for(i=0; i<mp->cursize; i++) {
	    realname[0] = 'a' + i;
	    if(RegQueryValueExW( newkey, realname, 0, &type, 0, &datasize)) {
		/* not present - what to do ??? */
		ERR("Key %s not found 1\n", debugstr_w(realname));
	    }
	    mp->array[i] = witem = Alloc(datasize + sizeof(WINEMRUITEM));
	    witem->size = datasize;
	    if(RegQueryValueExW( newkey, realname, 0, &type,
				 &witem->datastart, &datasize)) {
		/* not present - what to do ??? */
		ERR("Key %s not found 2\n", debugstr_w(realname));
	    }
	}
	RegCloseKey( newkey );
    }
    else
	mp->cursize = 0;

    TRACE("(%lu %lu %lx %lx \"%s\" %p): Current Size = %ld\n",
	  mp->extview.cbSize, mp->extview.nMaxItems, mp->extview.dwFlags,
	  (DWORD)mp->extview.hKey, debugstr_w(mp->extview.lpszSubKey),
	  mp->extview.lpfnCompare, mp->cursize);
    return (HANDLE)mp;
}

/**************************************************************************
 *                  CreateMRUListLazyW [COMCTL32.404]
 */
HANDLE WINAPI
CreateMRUListLazyW (LPCREATEMRULISTW lpcml, DWORD dwParam2, DWORD dwParam3, DWORD dwParam4)
{
    LPWINEMRULIST mp;

    if (lpcml == NULL)
	return 0;

    if (lpcml->cbSize < sizeof(CREATEMRULISTW))
	return 0;

    mp = Alloc(sizeof(WINEMRULIST));
    memcpy(&mp->extview, lpcml, sizeof(CREATEMRULISTW));
    mp->extview.lpszSubKey = Alloc((strlenW(lpcml->lpszSubKey) + 1) * sizeof(WCHAR));
    strcpyW((LPWSTR)mp->extview.lpszSubKey, lpcml->lpszSubKey);
    mp->isUnicode = TRUE;

    return CreateMRUListLazy_common(mp);
}

/**************************************************************************
 *                  CreateMRUListLazyA [COMCTL32.157]
 */
HANDLE WINAPI
CreateMRUListLazyA (LPCREATEMRULISTA lpcml, DWORD dwParam2, DWORD dwParam3, DWORD dwParam4)
{
    LPWINEMRULIST mp;
    DWORD len;

    if (lpcml == NULL)
	return 0;

    if (lpcml->cbSize < sizeof(CREATEMRULISTA))
	return 0;

    mp = Alloc(sizeof(WINEMRULIST));
    memcpy(&mp->extview, lpcml, sizeof(CREATEMRULISTW));
    len = MultiByteToWideChar(CP_ACP, 0, lpcml->lpszSubKey, -1, NULL, 0);
    mp->extview.lpszSubKey = Alloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, lpcml->lpszSubKey, -1,
			(LPWSTR)mp->extview.lpszSubKey, len);
    mp->isUnicode = FALSE;
    return CreateMRUListLazy_common(mp);
}

/**************************************************************************
 *              CreateMRUListW [COMCTL32.400]
 *
 * PARAMS
 *     lpcml [I] ptr to CREATEMRULIST structure.
 *
 * RETURNS
 *     Handle to MRU list.
 */
HANDLE WINAPI
CreateMRUListW (LPCREATEMRULISTW lpcml)
{
    return CreateMRUListLazyW(lpcml, 0, 0, 0);
}

/**************************************************************************
 *              CreateMRUListA [COMCTL32.151]
 */
HANDLE WINAPI
CreateMRUListA (LPCREATEMRULISTA lpcml)
{
     return CreateMRUListLazyA (lpcml, 0, 0, 0);
}


/**************************************************************************
 *                EnumMRUListW [COMCTL32.403]
 *
 * Enumerate item in a list
 *
 * PARAMS
 *    hList [I] list handle
 *    nItemPos [I] item position to enumerate
 *    lpBuffer [O] buffer to receive item
 *    nBufferSize [I] size of buffer
 *
 * RETURNS
 *    For binary lists specifies how many bytes were copied to buffer, for
 *    string lists specifies full length of string.  Enumerating past the end
 *    of list returns -1.
 *    If lpBuffer == NULL or nItemPos is -ve return value is no. of items in
 *    the list.
 */
INT WINAPI EnumMRUListW(HANDLE hList, INT nItemPos, LPVOID lpBuffer,
DWORD nBufferSize)
{
    LPWINEMRULIST mp = (LPWINEMRULIST) hList;
    LPWINEMRUITEM witem;
    INT desired, datasize;

    if (nItemPos >= mp->cursize) return -1;
    if ((nItemPos < 0) || !lpBuffer) return mp->cursize;
    desired = mp->realMRU[nItemPos];
    desired -= 'a';
    TRACE("nItemPos=%d, desired=%d\n", nItemPos, desired);
    witem = mp->array[desired];
    datasize = min( witem->size, nBufferSize );
    memcpy( lpBuffer, &witem->datastart, datasize);
    TRACE("(%p, %d, %p, %ld): returning len=%d\n",
	  hList, nItemPos, lpBuffer, nBufferSize, datasize);
    return datasize;
}

/**************************************************************************
 *                EnumMRUListA [COMCTL32.154]
 *
 */
INT WINAPI EnumMRUListA(HANDLE hList, INT nItemPos, LPVOID lpBuffer,
DWORD nBufferSize)
{
    LPWINEMRULIST mp = (LPWINEMRULIST) hList;
    LPWINEMRUITEM witem;
    INT desired, datasize;
    DWORD lenA;

    if (nItemPos >= mp->cursize) return -1;
    if ((nItemPos < 0) || !lpBuffer) return mp->cursize;
    desired = mp->realMRU[nItemPos];
    desired -= 'a';
    TRACE("nItemPos=%d, desired=%d\n", nItemPos, desired);
    witem = mp->array[desired];
    if(mp->extview.dwFlags & MRUF_BINARY_LIST) {
        datasize = min( witem->size, nBufferSize );
	memcpy( lpBuffer, &witem->datastart, datasize);
    } else {
        lenA = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)&witem->datastart, -1,
				   NULL, 0, NULL, NULL);
	datasize = min( witem->size, nBufferSize );
	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)&witem->datastart, -1,
			    lpBuffer, datasize, NULL, NULL);
    }
    TRACE("(%p, %d, %p, %ld): returning len=%d\n",
	  hList, nItemPos, lpBuffer, nBufferSize, datasize);
    return datasize;
}


/**************************************************************************
 * Str_GetPtrA [COMCTL32.233]
 *
 * PARAMS
 *     lpSrc   [I]
 *     lpDest  [O]
 *     nMaxLen [I]
 *
 * RETURNS
 */

INT WINAPI
Str_GetPtrA (LPCSTR lpSrc, LPSTR lpDest, INT nMaxLen)
{
    INT len;

    TRACE("(%p %p %d)\n", lpSrc, lpDest, nMaxLen);

    if (!lpDest && lpSrc)
	return strlen (lpSrc);

    if (nMaxLen == 0)
	return 0;

    if (lpSrc == NULL) {
	lpDest[0] = '\0';
	return 0;
    }

    len = strlen (lpSrc);
    if (len >= nMaxLen)
	len = nMaxLen - 1;

    RtlMoveMemory (lpDest, lpSrc, len);
    lpDest[len] = '\0';

    return len;
}


/**************************************************************************
 * Str_SetPtrA [COMCTL32.234]
 *
 * PARAMS
 *     lppDest [O]
 *     lpSrc   [I]
 *
 * RETURNS
 */

BOOL WINAPI
Str_SetPtrA (LPSTR *lppDest, LPCSTR lpSrc)
{
    TRACE("(%p %p)\n", lppDest, lpSrc);

    if (lpSrc) {
	LPSTR ptr = ReAlloc (*lppDest, strlen (lpSrc) + 1);
	if (!ptr)
	    return FALSE;
	strcpy (ptr, lpSrc);
	*lppDest = ptr;
    }
    else {
	if (*lppDest) {
	    Free (*lppDest);
	    *lppDest = NULL;
	}
    }

    return TRUE;
}


/**************************************************************************
 * Str_GetPtrW [COMCTL32.235]
 *
 * PARAMS
 *     lpSrc   [I]
 *     lpDest  [O]
 *     nMaxLen [I]
 *
 * RETURNS
 */

INT WINAPI
Str_GetPtrW (LPCWSTR lpSrc, LPWSTR lpDest, INT nMaxLen)
{
    INT len;

    TRACE("(%p %p %d)\n", lpSrc, lpDest, nMaxLen);

    if (!lpDest && lpSrc)
	return strlenW (lpSrc);

    if (nMaxLen == 0)
	return 0;

    if (lpSrc == NULL) {
	lpDest[0] = L'\0';
	return 0;
    }

    len = strlenW (lpSrc);
    if (len >= nMaxLen)
	len = nMaxLen - 1;

    RtlMoveMemory (lpDest, lpSrc, len*sizeof(WCHAR));
    lpDest[len] = L'\0';

    return len;
}


/**************************************************************************
 * Str_SetPtrW [COMCTL32.236]
 *
 * PARAMS
 *     lpDest [O]
 *     lpSrc  [I]
 *
 * RETURNS
 */

BOOL WINAPI
Str_SetPtrW (LPWSTR *lppDest, LPCWSTR lpSrc)
{
    TRACE("(%p %p)\n", lppDest, lpSrc);

    if (lpSrc) {
	INT len = strlenW (lpSrc) + 1;
	LPWSTR ptr = ReAlloc (*lppDest, len * sizeof(WCHAR));
	if (!ptr)
	    return FALSE;
	strcpyW (ptr, lpSrc);
	*lppDest = ptr;
    }
    else {
	if (*lppDest) {
	    Free (*lppDest);
	    *lppDest = NULL;
	}
    }

    return TRUE;
}


/**************************************************************************
 * Str_GetPtrWtoA [internal]
 *
 * Converts a unicode string into a multi byte string
 *
 * PARAMS
 *     lpSrc   [I] Pointer to the unicode source string
 *     lpDest  [O] Pointer to caller supplied storage for the multi byte string
 *     nMaxLen [I] Size, in bytes, of the destination buffer
 *
 * RETURNS
 *     Length, in bytes, of the converted string.
 */

INT
Str_GetPtrWtoA (LPCWSTR lpSrc, LPSTR lpDest, INT nMaxLen)
{
    INT len;

    TRACE("(%s %p %d)\n", debugstr_w(lpSrc), lpDest, nMaxLen);

    if (!lpDest && lpSrc)
	return WideCharToMultiByte(CP_ACP, 0, lpSrc, -1, 0, 0, NULL, NULL);

    if (nMaxLen == 0)
	return 0;

    if (lpSrc == NULL) {
	lpDest[0] = '\0';
	return 0;
    }

    len = WideCharToMultiByte(CP_ACP, 0, lpSrc, -1, 0, 0, NULL, NULL);
    if (len >= nMaxLen)
	len = nMaxLen - 1;

    WideCharToMultiByte(CP_ACP, 0, lpSrc, -1, lpDest, len, NULL, NULL);
    lpDest[len] = '\0';

    return len;
}


/**************************************************************************
 * Str_SetPtrAtoW [internal]
 *
 * Converts a multi byte string to a unicode string.
 * If the pointer to the destination buffer is NULL a buffer is allocated.
 * If the destination buffer is too small to keep the converted multi byte
 * string the destination buffer is reallocated. If the source pointer is
 * NULL, the destination buffer is freed.
 *
 * PARAMS
 *     lppDest [I/O] pointer to a pointer to the destination buffer
 *     lpSrc   [I] pointer to a multi byte string
 *
 * RETURNS
 *     TRUE: conversion successful
 *     FALSE: error
 */

BOOL
Str_SetPtrAtoW (LPWSTR *lppDest, LPCSTR lpSrc)
{
    TRACE("(%p %s)\n", lppDest, lpSrc);

    if (lpSrc) {
	INT len = MultiByteToWideChar(CP_ACP,0,lpSrc,-1,NULL,0);
	LPWSTR ptr = ReAlloc (*lppDest, len*sizeof(WCHAR));

	if (!ptr)
	    return FALSE;
	MultiByteToWideChar(CP_ACP,0,lpSrc,-1,ptr,len);
	*lppDest = ptr;
    }
    else {
	if (*lppDest) {
	    Free (*lppDest);
	    *lppDest = NULL;
	}
    }

    return TRUE;
}


/**************************************************************************
 * The DSA-API is a set of functions to create and manipulate arrays of
 * fixed-size memory blocks. These arrays can store any kind of data
 * (strings, icons...).
 */

/**************************************************************************
 * DSA_Create [COMCTL32.320] Creates a dynamic storage array
 *
 * PARAMS
 *     nSize [I] size of the array elements
 *     nGrow [I] number of elements by which the array grows when it is filled
 *
 * RETURNS
 *     Success: pointer to an array control structure. Use this like a handle.
 *     Failure: NULL
 */

HDSA WINAPI
DSA_Create (INT nSize, INT nGrow)
{
    HDSA hdsa;

    TRACE("(size=%d grow=%d)\n", nSize, nGrow);

    hdsa = Alloc (sizeof(*hdsa));
    if (hdsa)
    {
	hdsa->nItemCount = 0;
        hdsa->pData = NULL;
	hdsa->nMaxCount = 0;
	hdsa->nItemSize = nSize;
	hdsa->nGrow = max(1, nGrow);
    }

    return hdsa;
}


/**************************************************************************
 * DSA_Destroy [COMCTL32.321] Destroys a dynamic storage array
 *
 * PARAMS
 *     hdsa [I] pointer to the array control structure
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DSA_Destroy (const HDSA hdsa)
{
    TRACE("(%p)\n", hdsa);

    if (!hdsa)
	return FALSE;

    if (hdsa->pData && (!Free (hdsa->pData)))
	return FALSE;

    return Free (hdsa);
}


/**************************************************************************
 * DSA_GetItem [COMCTL32.322]
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] number of the Item to get
 *     pDest  [O] destination buffer. Has to be >= dwElementSize.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DSA_GetItem (const HDSA hdsa, INT nIndex, LPVOID pDest)
{
    LPVOID pSrc;

    TRACE("(%p %d %p)\n", hdsa, nIndex, pDest);

    if (!hdsa)
	return FALSE;
    if ((nIndex < 0) || (nIndex >= hdsa->nItemCount))
	return FALSE;

    pSrc = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    memmove (pDest, pSrc, hdsa->nItemSize);

    return TRUE;
}


/**************************************************************************
 * DSA_GetItemPtr [COMCTL32.323]
 *
 * Retrieves a pointer to the specified item.
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] index of the desired item
 *
 * RETURNS
 *     Success: pointer to an item
 *     Failure: NULL
 */

LPVOID WINAPI
DSA_GetItemPtr (const HDSA hdsa, INT nIndex)
{
    LPVOID pSrc;

    TRACE("(%p %d)\n", hdsa, nIndex);

    if (!hdsa)
	return NULL;
    if ((nIndex < 0) || (nIndex >= hdsa->nItemCount))
	return NULL;

    pSrc = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);

    TRACE("-- ret=%p\n", pSrc);

    return pSrc;
}


/**************************************************************************
 * DSA_SetItem [COMCTL32.325]
 *
 * Sets the contents of an item in the array.
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] index for the item
 *     pSrc   [I] pointer to the new item data
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DSA_SetItem (const HDSA hdsa, INT nIndex, LPVOID pSrc)
{
    INT  nSize, nNewItems;
    LPVOID pDest, lpTemp;

    TRACE("(%p %d %p)\n", hdsa, nIndex, pSrc);

    if ((!hdsa) || nIndex < 0)
	return FALSE;

    if (hdsa->nItemCount <= nIndex) {
	/* within the old array */
	if (hdsa->nMaxCount > nIndex) {
	    /* within the allocated space, set a new boundary */
	    hdsa->nItemCount = nIndex + 1;
	}
	else {
	    /* resize the block of memory */
	    nNewItems =
		hdsa->nGrow * ((INT)(((nIndex + 1) - 1) / hdsa->nGrow) + 1);
	    nSize = hdsa->nItemSize * nNewItems;

	    lpTemp = ReAlloc (hdsa->pData, nSize);
	    if (!lpTemp)
		return FALSE;

	    hdsa->nMaxCount = nNewItems;
	    hdsa->nItemCount = nIndex + 1;
	    hdsa->pData = lpTemp;
	}
    }

    /* put the new entry in */
    pDest = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    TRACE("-- move dest=%p src=%p size=%d\n",
	   pDest, pSrc, hdsa->nItemSize);
    memmove (pDest, pSrc, hdsa->nItemSize);

    return TRUE;
}


/**************************************************************************
 * DSA_InsertItem [COMCTL32.324]
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] index for the new item
 *     pSrc   [I] pointer to the element
 *
 * RETURNS
 *     Success: position of the new item
 *     Failure: -1
 */

INT WINAPI
DSA_InsertItem (const HDSA hdsa, INT nIndex, LPVOID pSrc)
{
    INT   nNewItems, nSize;
    LPVOID  lpTemp, lpDest;

    TRACE("(%p %d %p)\n", hdsa, nIndex, pSrc);

    if ((!hdsa) || nIndex < 0)
	return -1;

    /* when nIndex >= nItemCount then append */
    if (nIndex >= hdsa->nItemCount)
 	nIndex = hdsa->nItemCount;

    /* do we need to resize ? */
    if (hdsa->nItemCount >= hdsa->nMaxCount) {
	nNewItems = hdsa->nMaxCount + hdsa->nGrow;
	nSize = hdsa->nItemSize * nNewItems;

	lpTemp = ReAlloc (hdsa->pData, nSize);
	if (!lpTemp)
	    return -1;

	hdsa->nMaxCount = nNewItems;
	hdsa->pData = lpTemp;
    }

    /* do we need to move elements ? */
    if (nIndex < hdsa->nItemCount) {
	lpTemp = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
	lpDest = (char *) lpTemp + hdsa->nItemSize;
	nSize = (hdsa->nItemCount - nIndex) * hdsa->nItemSize;
	TRACE("-- move dest=%p src=%p size=%d\n",
	       lpDest, lpTemp, nSize);
	memmove (lpDest, lpTemp, nSize);
    }

    /* ok, we can put the new Item in */
    hdsa->nItemCount++;
    lpDest = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    TRACE("-- move dest=%p src=%p size=%d\n",
	   lpDest, pSrc, hdsa->nItemSize);
    memmove (lpDest, pSrc, hdsa->nItemSize);

    return nIndex;
}


/**************************************************************************
 * DSA_DeleteItem [COMCTL32.326]
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] index for the element to delete
 *
 * RETURNS
 *     Success: number of the deleted element
 *     Failure: -1
 */

INT WINAPI
DSA_DeleteItem (const HDSA hdsa, INT nIndex)
{
    LPVOID lpDest,lpSrc;
    INT  nSize;

    TRACE("(%p %d)\n", hdsa, nIndex);

    if (!hdsa)
	return -1;
    if (nIndex < 0 || nIndex >= hdsa->nItemCount)
	return -1;

    /* do we need to move ? */
    if (nIndex < hdsa->nItemCount - 1) {
	lpDest = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
	lpSrc = (char *) lpDest + hdsa->nItemSize;
	nSize = hdsa->nItemSize * (hdsa->nItemCount - nIndex - 1);
	TRACE("-- move dest=%p src=%p size=%d\n",
	       lpDest, lpSrc, nSize);
	memmove (lpDest, lpSrc, nSize);
    }

    hdsa->nItemCount--;

    /* free memory ? */
    if ((hdsa->nMaxCount - hdsa->nItemCount) >= hdsa->nGrow) {
	nSize = hdsa->nItemSize * hdsa->nItemCount;

	lpDest = ReAlloc (hdsa->pData, nSize);
	if (!lpDest)
	    return -1;

	hdsa->nMaxCount = hdsa->nItemCount;
	hdsa->pData = lpDest;
    }

    return nIndex;
}


/**************************************************************************
 * DSA_DeleteAllItems [COMCTL32.327]
 *
 * Removes all items and reinitializes the array.
 *
 * PARAMS
 *     hdsa [I] pointer to the array control structure
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DSA_DeleteAllItems (const HDSA hdsa)
{
    TRACE("(%p)\n", hdsa);

    if (!hdsa)
	return FALSE;
    if (hdsa->pData && (!Free (hdsa->pData)))
	return FALSE;

    hdsa->nItemCount = 0;
    hdsa->pData = NULL;
    hdsa->nMaxCount = 0;

    return TRUE;
}


/**************************************************************************
 * The DPA-API is a set of functions to create and manipulate arrays of
 * pointers.
 */

/**************************************************************************
 * DPA_Destroy [COMCTL32.329] Destroys a dynamic pointer array
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the pointer array
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DPA_Destroy (const HDPA hdpa)
{
    TRACE("(%p)\n", hdpa);

    if (!hdpa)
	return FALSE;

    if (hdpa->ptrs && (!HeapFree (hdpa->hHeap, 0, hdpa->ptrs)))
	return FALSE;

    return HeapFree (hdpa->hHeap, 0, hdpa);
}


/**************************************************************************
 * DPA_Grow [COMCTL32.330]
 *
 * Sets the growth amount.
 *
 * PARAMS
 *     hdpa  [I] handle (pointer) to the existing (source) pointer array
 *     nGrow [I] number of items by which the array grows when it's too small
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DPA_Grow (const HDPA hdpa, INT nGrow)
{
    TRACE("(%p %d)\n", hdpa, nGrow);

    if (!hdpa)
	return FALSE;

    hdpa->nGrow = max(8, nGrow);

    return TRUE;
}


/**************************************************************************
 * DPA_Clone [COMCTL32.331]
 *
 * Copies a pointer array to an other one or creates a copy
 *
 * PARAMS
 *     hdpa    [I] handle (pointer) to the existing (source) pointer array
 *     hdpaNew [O] handle (pointer) to the destination pointer array
 *
 * RETURNS
 *     Success: pointer to the destination pointer array.
 *     Failure: NULL
 *
 * NOTES
 *     - If the 'hdpaNew' is a NULL-Pointer, a copy of the source pointer
 *       array will be created and it's handle (pointer) is returned.
 *     - If 'hdpa' is a NULL-Pointer, the original implementation crashes,
 *       this implementation just returns NULL.
 */

HDPA WINAPI
DPA_Clone (const HDPA hdpa, const HDPA hdpaNew)
{
    INT nNewItems, nSize;
    HDPA hdpaTemp;

    if (!hdpa)
	return NULL;

    TRACE("(%p %p)\n", hdpa, hdpaNew);

    if (!hdpaNew) {
	/* create a new DPA */
	hdpaTemp = (HDPA)HeapAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
				    sizeof(*hdpaTemp));
	hdpaTemp->hHeap = hdpa->hHeap;
	hdpaTemp->nGrow = hdpa->nGrow;
    }
    else
	hdpaTemp = hdpaNew;

    if (hdpaTemp->ptrs) {
	/* remove old pointer array */
	HeapFree (hdpaTemp->hHeap, 0, hdpaTemp->ptrs);
	hdpaTemp->ptrs = NULL;
	hdpaTemp->nItemCount = 0;
	hdpaTemp->nMaxCount = 0;
    }

    /* create a new pointer array */
    nNewItems = hdpaTemp->nGrow *
		((INT)((hdpa->nItemCount - 1) / hdpaTemp->nGrow) + 1);
    nSize = nNewItems * sizeof(LPVOID);
    hdpaTemp->ptrs =
	(LPVOID*)HeapAlloc (hdpaTemp->hHeap, HEAP_ZERO_MEMORY, nSize);
    hdpaTemp->nMaxCount = nNewItems;

    /* clone the pointer array */
    hdpaTemp->nItemCount = hdpa->nItemCount;
    memmove (hdpaTemp->ptrs, hdpa->ptrs,
	     hdpaTemp->nItemCount * sizeof(LPVOID));

    return hdpaTemp;
}


/**************************************************************************
 * DPA_GetPtr [COMCTL32.332]
 *
 * Retrieves a pointer from a dynamic pointer array
 *
 * PARAMS
 *     hdpa   [I] handle (pointer) to the pointer array
 *     nIndex [I] array index of the desired pointer
 *
 * RETURNS
 *     Success: pointer
 *     Failure: NULL
 */

LPVOID WINAPI
DPA_GetPtr (const HDPA hdpa, INT nIndex)
{
    TRACE("(%p %d)\n", hdpa, nIndex);

    if (!hdpa)
	return NULL;
    if (!hdpa->ptrs) {
	WARN("no pointer array.\n");
	return NULL;
    }
    if ((nIndex < 0) || (nIndex >= hdpa->nItemCount)) {
	WARN("not enough pointers in array (%d vs %d).\n",nIndex,hdpa->nItemCount);
	return NULL;
    }

    TRACE("-- %p\n", hdpa->ptrs[nIndex]);

    return hdpa->ptrs[nIndex];
}


/**************************************************************************
 * DPA_GetPtrIndex [COMCTL32.333]
 *
 * Retrieves the index of the specified pointer
 *
 * PARAMS
 *     hdpa   [I] handle (pointer) to the pointer array
 *     p      [I] pointer
 *
 * RETURNS
 *     Success: index of the specified pointer
 *     Failure: -1
 */

INT WINAPI
DPA_GetPtrIndex (const HDPA hdpa, LPVOID p)
{
    INT i;

    if (!hdpa || !hdpa->ptrs)
	return -1;

    for (i = 0; i < hdpa->nItemCount; i++) {
	if (hdpa->ptrs[i] == p)
	    return i;
    }

    return -1;
}


/**************************************************************************
 * DPA_InsertPtr [COMCTL32.334]
 *
 * Inserts a pointer into a dynamic pointer array
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the array
 *     i    [I] array index
 *     p    [I] pointer to insert
 *
 * RETURNS
 *     Success: index of the inserted pointer
 *     Failure: -1
 */

INT WINAPI
DPA_InsertPtr (const HDPA hdpa, INT i, LPVOID p)
{
    TRACE("(%p %d %p)\n", hdpa, i, p);

    if (!hdpa || i < 0) return -1;

    if (i >= 0x7fff)
	i = hdpa->nItemCount;

    if (i >= hdpa->nItemCount)
	return DPA_SetPtr(hdpa, i, p) ? i : -1;

    /* create empty spot at the end */
    if (!DPA_SetPtr(hdpa, hdpa->nItemCount, 0)) return -1;
    memmove (hdpa->ptrs + i + 1, hdpa->ptrs + i, (hdpa->nItemCount - i - 1) * sizeof(LPVOID));
    hdpa->ptrs[i] = p;
    return i;
}

/**************************************************************************
 * DPA_SetPtr [COMCTL32.335]
 *
 * Sets a pointer in the pointer array
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the pointer array
 *     i    [I] index of the pointer that will be set
 *     p    [I] pointer to be set
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DPA_SetPtr (const HDPA hdpa, INT i, LPVOID p)
{
    LPVOID *lpTemp;

    TRACE("(%p %d %p)\n", hdpa, i, p);

    if (!hdpa || i < 0 || i > 0x7fff)
	return FALSE;

    if (hdpa->nItemCount <= i) {
	/* within the old array */
	if (hdpa->nMaxCount <= i) {
	    /* resize the block of memory */
	    INT nNewItems =
		hdpa->nGrow * ((INT)(((i+1) - 1) / hdpa->nGrow) + 1);
	    INT nSize = nNewItems * sizeof(LPVOID);

	    if (hdpa->ptrs)
	        lpTemp = (LPVOID*)HeapReAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY, hdpa->ptrs, nSize);
	    else
		lpTemp = (LPVOID*)HeapAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY, nSize);
	    
	    if (!lpTemp)
		return FALSE;

	    hdpa->nMaxCount = nNewItems;
	    hdpa->ptrs = lpTemp;
	}
        hdpa->nItemCount = i+1;
    }

    /* put the new entry in */
    hdpa->ptrs[i] = p;

    return TRUE;
}


/**************************************************************************
 * DPA_DeletePtr [COMCTL32.336]
 *
 * Removes a pointer from the pointer array.
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the pointer array
 *     i    [I] index of the pointer that will be deleted
 *
 * RETURNS
 *     Success: deleted pointer
 *     Failure: NULL
 */

LPVOID WINAPI
DPA_DeletePtr (const HDPA hdpa, INT i)
{
    LPVOID *lpDest, *lpSrc, lpTemp = NULL;
    INT  nSize;

    TRACE("(%p %d)\n", hdpa, i);

    if ((!hdpa) || i < 0 || i >= hdpa->nItemCount)
	return NULL;

    lpTemp = hdpa->ptrs[i];

    /* do we need to move ?*/
    if (i < hdpa->nItemCount - 1) {
	lpDest = hdpa->ptrs + i;
	lpSrc = lpDest + 1;
	nSize = (hdpa->nItemCount - i - 1) * sizeof(LPVOID);
	TRACE("-- move dest=%p src=%p size=%x\n",
	       lpDest, lpSrc, nSize);
	memmove (lpDest, lpSrc, nSize);
    }

    hdpa->nItemCount --;

    /* free memory ?*/
    if ((hdpa->nMaxCount - hdpa->nItemCount) >= hdpa->nGrow) {
	INT nNewItems = max(hdpa->nGrow * 2, hdpa->nItemCount);
	nSize = nNewItems * sizeof(LPVOID);
	lpDest = (LPVOID)HeapReAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
				      hdpa->ptrs, nSize);
	if (!lpDest)
	    return NULL;

	hdpa->nMaxCount = nNewItems;
	hdpa->ptrs = (LPVOID*)lpDest;
    }

    return lpTemp;
}


/**************************************************************************
 * DPA_DeleteAllPtrs [COMCTL32.337]
 *
 * Removes all pointers and reinitializes the array.
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the pointer array
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DPA_DeleteAllPtrs (const HDPA hdpa)
{
    TRACE("(%p)\n", hdpa);

    if (!hdpa)
	return FALSE;

    if (hdpa->ptrs && (!HeapFree (hdpa->hHeap, 0, hdpa->ptrs)))
	return FALSE;

    hdpa->nItemCount = 0;
    hdpa->nMaxCount = hdpa->nGrow * 2;
    hdpa->ptrs = (LPVOID*)HeapAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
				     hdpa->nMaxCount * sizeof(LPVOID));

    return TRUE;
}


/**************************************************************************
 * DPA_QuickSort [Internal]
 *
 * Ordinary quicksort (used by DPA_Sort).
 *
 * PARAMS
 *     lpPtrs     [I] pointer to the pointer array
 *     l          [I] index of the "left border" of the partition
 *     r          [I] index of the "right border" of the partition
 *     pfnCompare [I] pointer to the compare function
 *     lParam     [I] user defined value (3rd parameter in compare function)
 *
 * RETURNS
 *     NONE
 */

static VOID
DPA_QuickSort (LPVOID *lpPtrs, INT l, INT r,
	       PFNDPACOMPARE pfnCompare, LPARAM lParam)
{
    INT m;
    LPVOID t;

    TRACE("l=%i r=%i\n", l, r);

    if (l==r)    /* one element is always sorted */
        return;
    if (r<l)     /* oops, got it in the wrong order */
	{
        DPA_QuickSort(lpPtrs, r, l, pfnCompare, lParam);
        return;
	}
    m = (l+r)/2; /* divide by two */
    DPA_QuickSort(lpPtrs, l, m, pfnCompare, lParam);
    DPA_QuickSort(lpPtrs, m+1, r, pfnCompare, lParam);

    /* join the two sides */
    while( (l<=m) && (m<r) )
    {
        if(pfnCompare(lpPtrs[l],lpPtrs[m+1],lParam)>0)
        {
            t = lpPtrs[m+1];
            memmove(&lpPtrs[l+1],&lpPtrs[l],(m-l+1)*sizeof(lpPtrs[l]));
            lpPtrs[l] = t;

            m++;
        }
        l++;
    }
}


/**************************************************************************
 * DPA_Sort [COMCTL32.338]
 *
 * Sorts a pointer array using a user defined compare function
 *
 * PARAMS
 *     hdpa       [I] handle (pointer) to the pointer array
 *     pfnCompare [I] pointer to the compare function
 *     lParam     [I] user defined value (3rd parameter of compare function)
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
DPA_Sort (const HDPA hdpa, PFNDPACOMPARE pfnCompare, LPARAM lParam)
{
    if (!hdpa || !pfnCompare)
	return FALSE;

    TRACE("(%p %p 0x%lx)\n", hdpa, pfnCompare, lParam);

    if ((hdpa->nItemCount > 1) && (hdpa->ptrs))
	DPA_QuickSort (hdpa->ptrs, 0, hdpa->nItemCount - 1,
		       pfnCompare, lParam);

    return TRUE;
}


/**************************************************************************
 * DPA_Search [COMCTL32.339]
 *
 * Searches a pointer array for a specified pointer
 *
 * PARAMS
 *     hdpa       [I] handle (pointer) to the pointer array
 *     pFind      [I] pointer to search for
 *     nStart     [I] start index
 *     pfnCompare [I] pointer to the compare function
 *     lParam     [I] user defined value (3rd parameter of compare function)
 *     uOptions   [I] search options
 *
 * RETURNS
 *     Success: index of the pointer in the array.
 *     Failure: -1
 *
 * NOTES
 *     Binary search taken from R.Sedgewick "Algorithms in C"!
 *     Function is NOT tested!
 *     If something goes wrong, blame HIM not ME! (Eric Kohl)
 */

INT WINAPI
DPA_Search (const HDPA hdpa, LPVOID pFind, INT nStart,
	    PFNDPACOMPARE pfnCompare, LPARAM lParam, UINT uOptions)
{
    if (!hdpa || !pfnCompare || !pFind)
	return -1;

    TRACE("(%p %p %d %p 0x%08lx 0x%08x)\n",
	   hdpa, pFind, nStart, pfnCompare, lParam, uOptions);

    if (uOptions & DPAS_SORTED) {
	/* array is sorted --> use binary search */
	INT l, r, x, n;
	LPVOID *lpPtr;

	TRACE("binary search\n");

	l = (nStart == -1) ? 0 : nStart;
	r = hdpa->nItemCount - 1;
	lpPtr = hdpa->ptrs;
	while (r >= l) {
	    x = (l + r) / 2;
	    n = (pfnCompare)(pFind, lpPtr[x], lParam);
	    if (n < 0)
		r = x - 1;
	    else
		l = x + 1;
	    if (n == 0) {
		TRACE("-- ret=%d\n", n);
		return n;
	    }
	}

	if (uOptions & DPAS_INSERTBEFORE) {
	    if (r == -1) r = 0;
	    TRACE("-- ret=%d\n", r);
	    return r;
	}

	if (uOptions & DPAS_INSERTAFTER) {
	    TRACE("-- ret=%d\n", l);
	    return l;
	}
    }
    else {
	/* array is not sorted --> use linear search */
	LPVOID *lpPtr;
	INT  nIndex;

	TRACE("linear search\n");

	nIndex = (nStart == -1)? 0 : nStart;
	lpPtr = hdpa->ptrs;
	for (; nIndex < hdpa->nItemCount; nIndex++) {
	    if ((pfnCompare)(pFind, lpPtr[nIndex], lParam) == 0) {
		TRACE("-- ret=%d\n", nIndex);
		return nIndex;
	    }
	}
    }

    TRACE("-- not found: ret=-1\n");
    return -1;
}


/**************************************************************************
 * DPA_CreateEx [COMCTL32.340]
 *
 * Creates a dynamic pointer array using the specified size and heap.
 *
 * PARAMS
 *     nGrow [I] number of items by which the array grows when it is filled
 *     hHeap [I] handle to the heap where the array is stored
 *
 * RETURNS
 *     Success: handle (pointer) to the pointer array.
 *     Failure: NULL
 */

HDPA WINAPI
DPA_CreateEx (INT nGrow, HANDLE hHeap)
{
    HDPA hdpa;

    TRACE("(%d %p)\n", nGrow, hHeap);

    if (hHeap)
	hdpa = (HDPA)HeapAlloc (hHeap, HEAP_ZERO_MEMORY, sizeof(*hdpa));
    else
	hdpa = Alloc (sizeof(*hdpa));

    if (hdpa) {
	hdpa->nGrow = max(8, nGrow);
	hdpa->hHeap = hHeap ? hHeap : GetProcessHeap();
	hdpa->nMaxCount = hdpa->nGrow * 2;
	hdpa->ptrs = HeapAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
				hdpa->nMaxCount * sizeof(LPVOID));
    }

    TRACE("-- %p\n", hdpa);

    return hdpa;
}


/**************************************************************************
 * DPA_Create [COMCTL32.328] Creates a dynamic pointer array
 *
 * PARAMS
 *     nGrow [I] number of items by which the array grows when it is filled
 *
 * RETURNS
 *     Success: handle (pointer) to the pointer array.
 *     Failure: NULL
 */

HDPA WINAPI
DPA_Create (INT nGrow)
{
    return DPA_CreateEx( nGrow, 0 );
}


/**************************************************************************
 * Notification functions
 */

typedef struct tagNOTIFYDATA
{
    HWND hwndFrom;
    HWND hwndTo;
    DWORD  dwParam3;
    DWORD  dwParam4;
    DWORD  dwParam5;
    DWORD  dwParam6;
} NOTIFYDATA, *LPNOTIFYDATA;


/**************************************************************************
 * DoNotify [Internal]
 */

static LRESULT
DoNotify (LPNOTIFYDATA lpNotify, UINT uCode, LPNMHDR lpHdr)
{
    NMHDR nmhdr;
    LPNMHDR lpNmh = NULL;
    UINT idFrom = 0;

    TRACE("(%p %p %d %p 0x%08lx)\n",
	   lpNotify->hwndFrom, lpNotify->hwndTo, uCode, lpHdr,
	   lpNotify->dwParam5);

    if (!lpNotify->hwndTo)
	return 0;

    if (lpNotify->hwndFrom == (HWND)-1) {
	lpNmh = lpHdr;
	idFrom = lpHdr->idFrom;
    }
    else {
	if (lpNotify->hwndFrom)
	    idFrom = GetDlgCtrlID (lpNotify->hwndFrom);

	lpNmh = (lpHdr) ? lpHdr : &nmhdr;

	lpNmh->hwndFrom = lpNotify->hwndFrom;
	lpNmh->idFrom = idFrom;
	lpNmh->code = uCode;
    }

    return SendMessageA (lpNotify->hwndTo, WM_NOTIFY, idFrom, (LPARAM)lpNmh);
}


/**************************************************************************
 * SendNotify [COMCTL32.341]
 *
 * PARAMS
 *     hwndTo   [I]
 *     hwndFrom [I]
 *     uCode    [I]
 *     lpHdr    [I]
 *
 * RETURNS
 *     Success: return value from notification
 *     Failure: 0
 */

LRESULT WINAPI SendNotify (HWND hwndTo, HWND hwndFrom, UINT uCode, LPNMHDR lpHdr)
{
    NOTIFYDATA notify;

    TRACE("(%p %p %d %p)\n",
	   hwndTo, hwndFrom, uCode, lpHdr);

    notify.hwndFrom = hwndFrom;
    notify.hwndTo   = hwndTo;
    notify.dwParam5 = 0;
    notify.dwParam6 = 0;

    return DoNotify (&notify, uCode, lpHdr);
}


/**************************************************************************
 * SendNotifyEx [COMCTL32.342]
 *
 * PARAMS
 *     hwndFrom [I]
 *     hwndTo   [I]
 *     uCode    [I]
 *     lpHdr    [I]
 *     dwParam5 [I]
 *
 * RETURNS
 *     Success: return value from notification
 *     Failure: 0
 */

LRESULT WINAPI SendNotifyEx (HWND hwndTo, HWND hwndFrom, UINT uCode,
                             LPNMHDR lpHdr, DWORD dwParam5)
{
    NOTIFYDATA notify;
    HWND hwndNotify;

    TRACE("(%p %p %d %p 0x%08lx)\n",
	   hwndFrom, hwndTo, uCode, lpHdr, dwParam5);

    hwndNotify = hwndTo;
    if (!hwndTo) {
	if (IsWindow (hwndFrom)) {
	    hwndNotify = GetParent (hwndFrom);
	    if (!hwndNotify)
		return 0;
	}
    }

    notify.hwndFrom = hwndFrom;
    notify.hwndTo   = hwndNotify;
    notify.dwParam5 = dwParam5;
    notify.dwParam6 = 0;

    return DoNotify (&notify, uCode, lpHdr);
}


/**************************************************************************
 * StrChrA [COMCTL32.350]
 *
 */

LPSTR WINAPI StrChrA (LPCSTR lpString, CHAR cChar)
{
    return strchr (lpString, cChar);
}


/**************************************************************************
 * StrStrIA [COMCTL32.355]
 */

LPSTR WINAPI StrStrIA (LPCSTR lpStr1, LPCSTR lpStr2)
{
    INT len1, len2, i;
    CHAR  first;

    if (*lpStr2 == 0)
	return ((LPSTR)lpStr1);
    len1 = 0;
    while (lpStr1[len1] != 0) ++len1;
    len2 = 0;
    while (lpStr2[len2] != 0) ++len2;
    if (len2 == 0)
	return ((LPSTR)(lpStr1 + len1));
    first = tolower (*lpStr2);
    while (len1 >= len2) {
	if (tolower(*lpStr1) == first) {
	    for (i = 1; i < len2; ++i)
		if (tolower (lpStr1[i]) != tolower(lpStr2[i]))
		    break;
	    if (i >= len2)
		return ((LPSTR)lpStr1);
        }
	++lpStr1; --len1;
    }
    return (NULL);
}

/**************************************************************************
 * StrToIntA [COMCTL32.357] Converts a string to a signed integer.
 */

INT WINAPI StrToIntA (LPSTR lpString)
{
    return atoi(lpString);
}

/**************************************************************************
 * StrStrIW [COMCTL32.363]
 */

LPWSTR WINAPI StrStrIW (LPCWSTR lpStr1, LPCWSTR lpStr2)
{
    INT len1, len2, i;
    WCHAR  first;

    if (*lpStr2 == 0)
	return ((LPWSTR)lpStr1);
    len1 = 0;
    while (lpStr1[len1] != 0) ++len1;
    len2 = 0;
    while (lpStr2[len2] != 0) ++len2;
    if (len2 == 0)
	return ((LPWSTR)(lpStr1 + len1));
    first = tolowerW (*lpStr2);
    while (len1 >= len2) {
	if (tolowerW (*lpStr1) == first) {
	    for (i = 1; i < len2; ++i)
		if (tolowerW (lpStr1[i]) != tolowerW(lpStr2[i]))
		    break;
	    if (i >= len2)
		return ((LPWSTR)lpStr1);
        }
	++lpStr1; --len1;
    }
    return (NULL);
}

/**************************************************************************
 * StrToIntW [COMCTL32.365] Converts a wide char string to a signed integer.
 */

INT WINAPI StrToIntW (LPWSTR lpString)
{
    return atoiW(lpString);
}


/**************************************************************************
 * DPA_EnumCallback [COMCTL32.385]
 *
 * Enumerates all items in a dynamic pointer array.
 *
 * PARAMS
 *     hdpa     [I] handle to the dynamic pointer array
 *     enumProc [I]
 *     lParam   [I]
 *
 * RETURNS
 *     none
 */

VOID WINAPI
DPA_EnumCallback (HDPA hdpa, PFNDPAENUMCALLBACK enumProc, LPVOID lParam)
{
    INT i;

    TRACE("(%p %p %p)\n", hdpa, enumProc, lParam);

    if (!hdpa)
	return;
    if (hdpa->nItemCount <= 0)
	return;

    for (i = 0; i < hdpa->nItemCount; i++) {
	if ((enumProc)(hdpa->ptrs[i], lParam) == 0)
	    return;
    }

    return;
}


/**************************************************************************
 * DPA_DestroyCallback [COMCTL32.386]
 *
 * Enumerates all items in a dynamic pointer array and destroys it.
 *
 * PARAMS
 *     hdpa     [I] handle to the dynamic pointer array
 *     enumProc [I]
 *     lParam   [I]
 *
 * RETURNS
 *     none
 */

void WINAPI
DPA_DestroyCallback (HDPA hdpa, PFNDPAENUMCALLBACK enumProc, LPVOID lParam)
{
    TRACE("(%p %p %p)\n", hdpa, enumProc, lParam);

    DPA_EnumCallback (hdpa, enumProc, lParam);
    DPA_Destroy (hdpa);
}


/**************************************************************************
 * DSA_EnumCallback [COMCTL32.387]
 *
 * Enumerates all items in a dynamic storage array.
 *
 * PARAMS
 *     hdsa     [I] handle to the dynamic storage array
 *     enumProc [I]
 *     lParam   [I]
 *
 * RETURNS
 *     none
 */

VOID WINAPI
DSA_EnumCallback (HDSA hdsa, PFNDSAENUMCALLBACK enumProc, LPVOID lParam)
{
    INT i;

    TRACE("(%p %p %p)\n", hdsa, enumProc, lParam);

    if (!hdsa)
	return;
    if (hdsa->nItemCount <= 0)
	return;

    for (i = 0; i < hdsa->nItemCount; i++) {
	LPVOID lpItem = DSA_GetItemPtr (hdsa, i);
	if ((enumProc)(lpItem, lParam) == 0)
	    return;
    }

    return;
}


/**************************************************************************
 * DSA_DestroyCallback [COMCTL32.388]
 *
 * Enumerates all items in a dynamic storage array and destroys it.
 *
 * PARAMS
 *     hdsa     [I] handle to the dynamic storage array
 *     enumProc [I]
 *     lParam   [I]
 *
 * RETURNS
 *     none
 */

void WINAPI
DSA_DestroyCallback (HDSA hdsa, PFNDSAENUMCALLBACK enumProc, LPVOID lParam)
{
    TRACE("(%p %p %p)\n", hdsa, enumProc, lParam);

    DSA_EnumCallback (hdsa, enumProc, lParam);
    DSA_Destroy (hdsa);
}

/**************************************************************************
 * StrCSpnA [COMCTL32.356]
 *
 */
INT WINAPI StrCSpnA( LPCSTR lpStr, LPCSTR lpSet)
{
  return strcspn(lpStr, lpSet);
}

/**************************************************************************
 * StrChrW [COMCTL32.358]
 *
 */
LPWSTR WINAPI StrChrW( LPCWSTR lpStart, WORD wMatch)
{
  return strchrW(lpStart, wMatch);
}

/**************************************************************************
 * StrCmpNA [COMCTL32.352]
 *
 */
INT WINAPI StrCmpNA( LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
  return strncmp(lpStr1, lpStr2, nChar);
}

/**************************************************************************
 * StrCmpNIA [COMCTL32.353]
 *
 */
INT WINAPI StrCmpNIA( LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
  return strncasecmp(lpStr1, lpStr2, nChar);
}

/**************************************************************************
 * StrCmpNW [COMCTL32.360]
 *
 */
INT WINAPI StrCmpNW( LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
  return strncmpW(lpStr1, lpStr2, nChar);
}

/**************************************************************************
 * StrCmpNIW [COMCTL32.361]
 *
 */
INT WINAPI StrCmpNIW( LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
  FIXME("(%s, %s, %i): stub\n", debugstr_w(lpStr1), debugstr_w(lpStr2), nChar);
  return 0;
}

/**************************************************************************
 * StrRChrA [COMCTL32.351]
 *
 */
LPSTR WINAPI StrRChrA( LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch )
{
    LPCSTR lpGotIt = NULL;
    BOOL dbcs = IsDBCSLeadByte( LOBYTE(wMatch) );

    TRACE("(%p, %p, %x)\n", lpStart, lpEnd, wMatch);

    if (!lpEnd) lpEnd = lpStart + strlen(lpStart);

    for(; lpStart < lpEnd; lpStart = CharNextA(lpStart))
    {
        if (*lpStart != LOBYTE(wMatch)) continue;
        if (dbcs && lpStart[1] != HIBYTE(wMatch)) continue;
        lpGotIt = lpStart;
    }
    return (LPSTR)lpGotIt;
}


/**************************************************************************
 * StrRChrW [COMCTL32.359]
 *
 */
LPWSTR WINAPI StrRChrW( LPCWSTR lpStart, LPCWSTR lpEnd, WORD wMatch)
{
    LPCWSTR lpGotIt = NULL;

    TRACE("(%p, %p, %x)\n", lpStart, lpEnd, wMatch);
    if (!lpEnd) lpEnd = lpStart + strlenW(lpStart);

    for(; lpStart < lpEnd; lpStart = CharNextW(lpStart))
        if (*lpStart == wMatch) lpGotIt = lpStart;

    return (LPWSTR)lpGotIt;
}


/**************************************************************************
 * StrStrA [COMCTL32.354]
 *
 */
LPSTR WINAPI StrStrA( LPCSTR lpFirst, LPCSTR lpSrch)
{
  return strstr(lpFirst, lpSrch);
}

/**************************************************************************
 * StrStrW [COMCTL32.362]
 *
 */
LPWSTR WINAPI StrStrW( LPCWSTR lpFirst, LPCWSTR lpSrch)
{
  return strstrW(lpFirst, lpSrch);
}

/**************************************************************************
 * StrSpnW [COMCTL32.364]
 *
 */
INT WINAPI StrSpnW( LPWSTR lpStr, LPWSTR lpSet)
{
  LPWSTR lpLoop = lpStr;

  /* validate ptr */
  if ((lpStr == 0) || (lpSet == 0)) return 0;

/* while(*lpLoop) { if lpLoop++; } */

  for(; (*lpLoop != 0); lpLoop++)
    if( strchrW(lpSet, *(WORD*)lpLoop))
      return (INT)(lpLoop-lpStr);

  return (INT)(lpLoop-lpStr);
}

/**************************************************************************
 * @ [COMCTL32.415]
 *
 * FIXME: What's this supposed to do?
 *        Parameter 1 is an HWND, you're on your own for the rest.
 */

BOOL WINAPI DrawTextWrap( HWND hwnd, DWORD b, DWORD c, DWORD d, DWORD e)
{

   FIXME("(%p, %lx, %lx, %lx, %lx): stub!\n", hwnd, b, c, d, e);

   return TRUE;
}

/**************************************************************************
 * @ [COMCTL32.417]
 *
 */
BOOL WINAPI ExtTextOutWrap(HDC hdc, INT x, INT y, UINT flags, const RECT *lprect,
                         LPCWSTR str, UINT count, const INT *lpDx)
{
    return ExtTextOutW(hdc, x, y, flags, lprect, str, count, lpDx);
}

/**************************************************************************
 * @ [COMCTL32.419]
 *
 * FIXME: What's this supposed to do?
 */

BOOL WINAPI GetTextExtentPointWrap( DWORD a, DWORD b, DWORD c, DWORD d)
{

   FIXME("(%lx, %lx, %lx, %lx): stub!\n", a, b, c, d);

   return TRUE;
}
