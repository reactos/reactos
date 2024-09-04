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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
 *     All of these functions are UNDOCUMENTED!! And I mean UNDOCUMENTED!!!!
 *     Do NOT rely on names or contents of undocumented structures and types!!!
 *     These functions are used by EXPLORER.EXE, IEXPLORE.EXE and
 *     COMCTL32.DLL (internally).
 *
 */

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "commctrl.h"
#include "objbase.h"
#include "winerror.h"

#include "comctl32.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);

static const WCHAR strMRUList[] = { 'M','R','U','L','i','s','t',0 };

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
        return LocalReAlloc( lpSrc, dwSize, LMEM_ZEROINIT | LMEM_MOVEABLE );
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
 * MRU-Functions  {COMCTL32}
 *
 * NOTES
 * The MRU-API is a set of functions to manipulate lists of M.R.U. (Most Recently
 * Used) items. It is an undocumented API that is used (at least) by the shell
 * and explorer to implement their recent documents feature.
 *
 * Since these functions are undocumented, they are unsupported by MS and
 * may change at any time.
 *
 * Internally, the list is implemented as a last in, last out list of items
 * persisted into the system registry under a caller chosen key. Each list
 * item is given a one character identifier in the Ascii range from 'a' to
 * '}'. A list of the identifiers in order from newest to oldest is stored
 * under the same key in a value named "MRUList".
 *
 * Items are re-ordered by changing the order of the values in the MRUList
 * value. When a new item is added, it becomes the new value of the oldest
 * identifier, and that identifier is moved to the front of the MRUList value.
 * 
 * Wine stores MRU-lists in the same registry format as Windows, so when
 * switching between the builtin and native comctl32.dll no problems or
 * incompatibilities should occur.
 *
 * The following undocumented structure is used to create an MRU-list:
 *|typedef INT (CALLBACK *MRUStringCmpFn)(LPCTSTR lhs, LPCTSTR rhs);
 *|typedef INT (CALLBACK *MRUBinaryCmpFn)(LPCVOID lhs, LPCVOID rhs, DWORD length);
 *|
 *|typedef struct tagMRUINFO
 *|{
 *|    DWORD   cbSize;
 *|    UINT    uMax;
 *|    UINT    fFlags;
 *|    HKEY    hKey;
 *|    LPTSTR  lpszSubKey;
 *|    PROC    lpfnCompare;
 *|} MRUINFO, *LPMRUINFO;
 *
 * MEMBERS
 *  cbSize      [I] The size of the MRUINFO structure. This must be set
 *                  to sizeof(MRUINFO) by the caller.
 *  uMax        [I] The maximum number of items allowed in the list. Because
 *                  of the limited number of identifiers, this should be set to
 *                  a value from 1 to 30 by the caller.
 *  fFlags      [I] If bit 0 is set, the list will be used to store binary
 *                  data, otherwise it is assumed to store strings. If bit 1
 *                  is set, every change made to the list will be reflected in
 *                  the registry immediately, otherwise changes will only be
 *                  written when the list is closed.
 *  hKey        [I] The registry key that the list should be written under.
 *                  This must be supplied by the caller.
 *  lpszSubKey  [I] A caller supplied name of a subkey under hKey to write
 *                  the list to. This may not be blank.
 *  lpfnCompare [I] A caller supplied comparison function, which may be either
 *                  an MRUStringCmpFn if dwFlags does not have bit 0 set, or a
 *                  MRUBinaryCmpFn otherwise.
 *
 * FUNCTIONS
 *  - Create an MRU-list with CreateMRUList() or CreateMRUListLazy().
 *  - Add items to an MRU-list with AddMRUString() or AddMRUData().
 *  - Remove items from an MRU-list with DelMRUString().
 *  - Find data in an MRU-list with FindMRUString() or FindMRUData().
 *  - Iterate through an MRU-list with EnumMRUList().
 *  - Free an MRU-list with FreeMRUList().
 */

typedef INT (CALLBACK *MRUStringCmpFnA)(LPCSTR lhs, LPCSTR rhs);
typedef INT (CALLBACK *MRUStringCmpFnW)(LPCWSTR lhs, LPCWSTR rhs);
typedef INT (CALLBACK *MRUBinaryCmpFn)(LPCVOID lhs, LPCVOID rhs, DWORD length);

typedef struct tagMRUINFOA
{
    DWORD  cbSize;
    UINT   uMax;
    UINT   fFlags;
    HKEY   hKey;
    LPSTR  lpszSubKey;
    union
    {
        MRUStringCmpFnA string_cmpfn;
        MRUBinaryCmpFn  binary_cmpfn;
    } u;
} MRUINFOA, *LPMRUINFOA;

typedef struct tagMRUINFOW
{
    DWORD   cbSize;
    UINT    uMax;
    UINT    fFlags;
    HKEY    hKey;
    LPWSTR  lpszSubKey;
    union
    {
        MRUStringCmpFnW string_cmpfn;
        MRUBinaryCmpFn  binary_cmpfn;
    } u;
} MRUINFOW, *LPMRUINFOW;

/* MRUINFO.fFlags */
#define MRU_STRING     0 /* list will contain strings */
#define MRU_BINARY     1 /* list will contain binary data */
#define MRU_CACHEWRITE 2 /* only save list order to reg. is FreeMRUList */

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
    MRUINFOW       extview;     /* original create information       */
    BOOL           isUnicode;   /* is compare fn Unicode */
    DWORD          wineFlags;   /* internal flags                    */
    DWORD          cursize;     /* current size of realMRU           */
    LPWSTR         realMRU;     /* pointer to string of index names  */
    LPWINEMRUITEM  *array;      /* array of pointers to data         */
                                /* in 'a' to 'z' order               */
} WINEMRULIST, *LPWINEMRULIST;

/* wineFlags */
#define WMRUF_CHANGED  0x0001   /* MRU list has changed              */

/**************************************************************************
 *              MRU_SaveChanged (internal)
 *
 * Local MRU saving code
 */
static void MRU_SaveChanged ( LPWINEMRULIST mp )
{
    UINT i, err;
    HKEY newkey;
    WCHAR realname[2];
    LPWINEMRUITEM witem;

    /* or should we do the following instead of RegOpenKeyEx:
     */

    /* open the sub key */
    if ((err = RegOpenKeyExW( mp->extview.hKey, mp->extview.lpszSubKey,
			      0, KEY_WRITE, &newkey))) {
	/* not present - what to do ??? */
	ERR("Could not open key, error=%d, attempting to create\n",
	    err);
	if ((err = RegCreateKeyExW( mp->extview.hKey, mp->extview.lpszSubKey,
				    0,
				    NULL,
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
	err = RegSetValueExW(newkey, strMRUList, 0, REG_SZ, (LPBYTE)mp->realMRU,
			     (lstrlenW(mp->realMRU) + 1)*sizeof(WCHAR));
	if (err) {
	    ERR("error saving MRUList, err=%d\n", err);
	}
	TRACE("saving MRUList=/%s/\n", debugstr_w(mp->realMRU));
    }
    realname[1] = 0;
    for(i=0; i<mp->cursize; i++) {
	witem = mp->array[i];
	if (witem->itemFlag & WMRUIF_CHANGED) {
	    witem->itemFlag &= ~WMRUIF_CHANGED;
	    realname[0] = 'a' + i;
	    err = RegSetValueExW(newkey, realname, 0,
				 (mp->extview.fFlags & MRU_BINARY) ?
				 REG_BINARY : REG_SZ,
				 &witem->datastart, witem->size);
	    if (err) {
		ERR("error saving /%s/, err=%d\n", debugstr_w(realname), err);
	    }
            TRACE("saving value for name /%s/ size=%d\n",
		  debugstr_w(realname), witem->size);
	}
    }
    RegCloseKey( newkey );
}

/**************************************************************************
 *              FreeMRUList [COMCTL32.152]
 *
 * Frees a most-recently-used items list.
 *
 * PARAMS
 *     hMRUList [I] Handle to list.
 *
 * RETURNS
 *     Nothing.
 */
void WINAPI FreeMRUList (HANDLE hMRUList)
{
    LPWINEMRULIST mp = hMRUList;
    UINT i;

    TRACE("(%p)\n", hMRUList);
    if (!hMRUList)
        return;

    if (mp->wineFlags & WMRUF_CHANGED) {
	/* need to open key and then save the info */
	MRU_SaveChanged( mp );
    }

    for(i=0; i<mp->extview.uMax; i++)
        Free(mp->array[i]);

    Free(mp->realMRU);
    Free(mp->array);
    Free(mp->extview.lpszSubKey);
    Free(mp);
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
INT WINAPI FindMRUData (HANDLE hList, LPCVOID lpData, DWORD cbData,
                        LPINT lpRegNum)
{
    const WINEMRULIST *mp = hList;
    INT ret;
    UINT i;
    LPSTR dataA = NULL;

    if (!mp || !mp->extview.u.string_cmpfn)
	return -1;

    if(!(mp->extview.fFlags & MRU_BINARY) && !mp->isUnicode) {
        DWORD len = WideCharToMultiByte(CP_ACP, 0, lpData, -1,
					NULL, 0, NULL, NULL);
	dataA = Alloc(len);
	WideCharToMultiByte(CP_ACP, 0, lpData, -1, dataA, len, NULL, NULL);
    }

    for(i=0; i<mp->cursize; i++) {
	if (mp->extview.fFlags & MRU_BINARY) {
	    if (!mp->extview.u.binary_cmpfn(lpData, &mp->array[i]->datastart, cbData))
		break;
	}
	else {
	    if(mp->isUnicode) {
	        if (!mp->extview.u.string_cmpfn(lpData, (LPWSTR)&mp->array[i]->datastart))
		    break;
	    } else {
	        DWORD len = WideCharToMultiByte(CP_ACP, 0,
						(LPWSTR)&mp->array[i]->datastart, -1,
						NULL, 0, NULL, NULL);
		LPSTR itemA = Alloc(len);
		INT cmp;
		WideCharToMultiByte(CP_ACP, 0, (LPWSTR)&mp->array[i]->datastart, -1,
				    itemA, len, NULL, NULL);

	        cmp = mp->extview.u.string_cmpfn((LPWSTR)dataA, (LPWSTR)itemA);
		Free(itemA);
		if(!cmp)
		    break;
	    }
	}
    }
    Free(dataA);
    if (i < mp->cursize)
	ret = i;
    else
	ret = -1;
    if (lpRegNum && (ret != -1))
	*lpRegNum = 'a' + i;

    TRACE("(%p, %p, %d, %p) returning %d\n",
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
INT WINAPI AddMRUData (HANDLE hList, LPCVOID lpData, DWORD cbData)
{
    LPWINEMRULIST mp = hList;
    LPWINEMRUITEM witem;
    INT i, replace;

    if ((replace = FindMRUData (hList, lpData, cbData, NULL)) >= 0) {
        /* Item exists, just move it to the front */
        LPWSTR pos = wcschr(mp->realMRU, replace + 'a');
        while (pos > mp->realMRU)
        {
            pos[0] = pos[-1];
            pos--;
        }
    }
    else {
	/* either add a new entry or replace oldest */
	if (mp->cursize < mp->extview.uMax) {
	    /* Add in a new item */
	    replace = mp->cursize;
	    mp->cursize++;
	}
	else {
	    /* get the oldest entry and replace data */
	    replace = mp->realMRU[mp->cursize - 1] - 'a';
	    Free(mp->array[replace]);
	}

        /* Allocate space for new item and move in the data */
        mp->array[replace] = witem = Alloc(cbData + sizeof(WINEMRUITEM));
        witem->itemFlag |= WMRUIF_CHANGED;
        witem->size = cbData;
        memcpy( &witem->datastart, lpData, cbData);

        /* now rotate MRU list */
        for(i=mp->cursize-1; i>=1; i--)
            mp->realMRU[i] = mp->realMRU[i-1];
    }

    /* The new item gets the front spot */
    mp->wineFlags |= WMRUF_CHANGED;
    mp->realMRU[0] = replace + 'a';

    TRACE("(%p, %p, %d) adding data, /%c/ now most current\n",
          hList, lpData, cbData, replace+'a');

    if (!(mp->extview.fFlags & MRU_CACHEWRITE)) {
	/* save changed stuff right now */
	MRU_SaveChanged( mp );
    }

    return replace;
}

/**************************************************************************
 *              AddMRUStringW [COMCTL32.401]
 *
 * Add an item to an MRU string list.
 *
 * PARAMS
 *     hList      [I] Handle to list.
 *     lpszString [I] The string to add.
 *
 * RETURNS
 *   Success: The number corresponding to the registry name where the string
 *            has been stored (0 maps to 'a', 1 to 'b' and so on).
 *   Failure: -1, if hList is NULL or memory allocation fails. If lpszString
 *            is invalid, the function returns 0, and GetLastError() returns
 *            ERROR_INVALID_PARAMETER. The last error value is set only in
 *            this case.
 *
 * NOTES
 *  -If lpszString exists in the list already, it is moved to the top of the
 *   MRU list (it is not duplicated).
 *  -If the list is full the least recently used list entry is replaced with
 *   lpszString.
 *  -If this function returns 0 you should check the last error value to
 *   ensure the call really succeeded.
 */
INT WINAPI AddMRUStringW(HANDLE hList, LPCWSTR lpszString)
{
    TRACE("(%p,%s)\n", hList, debugstr_w(lpszString));

    if (!hList)
        return -1;

    if (!lpszString || IsBadStringPtrW(lpszString, -1))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    return AddMRUData(hList, lpszString,
                      (lstrlenW(lpszString) + 1) * sizeof(WCHAR));
}

/**************************************************************************
 *              AddMRUStringA [COMCTL32.153]
 *
 * See AddMRUStringW.
 */
INT WINAPI AddMRUStringA(HANDLE hList, LPCSTR lpszString)
{
    DWORD len;
    LPWSTR stringW;
    INT ret;

    TRACE("(%p,%s)\n", hList, debugstr_a(lpszString));

    if (!hList)
        return -1;

    if (IsBadStringPtrA(lpszString, -1))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }

    len = MultiByteToWideChar(CP_ACP, 0, lpszString, -1, NULL, 0) * sizeof(WCHAR);
    stringW = Alloc(len);
    if (!stringW)
        return -1;

    MultiByteToWideChar(CP_ACP, 0, lpszString, -1, stringW, len/sizeof(WCHAR));
    ret = AddMRUData(hList, stringW, len);
    Free(stringW);
    return ret;
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
BOOL WINAPI DelMRUString(HANDLE hList, INT nItemPos)
{
    FIXME("(%p, %d): stub\n", hList, nItemPos);
    return TRUE;
}

/**************************************************************************
 *                  FindMRUStringW [COMCTL32.402]
 *
 * See FindMRUStringA.
 */
INT WINAPI FindMRUStringW (HANDLE hList, LPCWSTR lpszString, LPINT lpRegNum)
{
  return FindMRUData(hList, lpszString,
                     (lstrlenW(lpszString) + 1) * sizeof(WCHAR), lpRegNum);
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
INT WINAPI FindMRUStringA (HANDLE hList, LPCSTR lpszString, LPINT lpRegNum)
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
 *                 create_mru_list (internal)
 */
static HANDLE create_mru_list(LPWINEMRULIST mp)
{
    UINT i, err;
    HKEY newkey;
    DWORD datasize, dwdisp;
    WCHAR realname[2];
    LPWINEMRUITEM witem;
    DWORD type;

    /* get space to save indices that will turn into names
     * but in order of most to least recently used
     */
    mp->realMRU = Alloc((mp->extview.uMax + 2) * sizeof(WCHAR));

    /* get space to save pointers to actual data in order of
     * 'a' to 'z' (0 to n).
     */
    mp->array = Alloc(mp->extview.uMax * sizeof(LPVOID));

    /* open the sub key */
    if ((err = RegCreateKeyExW( mp->extview.hKey, mp->extview.lpszSubKey,
			        0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				KEY_READ | KEY_WRITE,
                                0,
				&newkey,
				&dwdisp))) {
	/* error - what to do ??? */
	ERR("(%u %u %x %p %s %p): Could not open key, error=%d\n",
	    mp->extview.cbSize, mp->extview.uMax, mp->extview.fFlags,
	    mp->extview.hKey, debugstr_w(mp->extview.lpszSubKey),
            mp->extview.u.string_cmpfn, err);
	return 0;
    }

    /* get values from key 'MRUList' */
    if (newkey) {
	datasize = (mp->extview.uMax + 1) * sizeof(WCHAR);
	if (RegQueryValueExW( newkey, strMRUList, 0, &type,
				  (LPBYTE)mp->realMRU, &datasize)) {
	    /* not present - set size to 1 (will become 0 later) */
	    datasize = 1;
	    *mp->realMRU = 0;
	}
        else
            datasize /= sizeof(WCHAR);

	TRACE("MRU list = %s, datasize = %d\n", debugstr_w(mp->realMRU), datasize);

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

    TRACE("(%u %u %x %p %s %p): Current Size = %d\n",
	  mp->extview.cbSize, mp->extview.uMax, mp->extview.fFlags,
	  mp->extview.hKey, debugstr_w(mp->extview.lpszSubKey),
	  mp->extview.u.string_cmpfn, mp->cursize);
    return mp;
}

/**************************************************************************
 *                  CreateMRUListLazyW [COMCTL32.404]
 *
 * See CreateMRUListLazyA.
 */
HANDLE WINAPI CreateMRUListLazyW (const MRUINFOW *infoW, DWORD dwParam2,
                                  DWORD dwParam3, DWORD dwParam4)
{
    LPWINEMRULIST mp;

    /* Native does not check for a NULL lpcml */
    if (!infoW->hKey || IsBadStringPtrW(infoW->lpszSubKey, -1))
	return NULL;

    mp = Alloc(sizeof(WINEMRULIST));
    memcpy(&mp->extview, infoW, sizeof(MRUINFOW));
    mp->extview.lpszSubKey = Alloc((lstrlenW(infoW->lpszSubKey) + 1) * sizeof(WCHAR));
    lstrcpyW(mp->extview.lpszSubKey, infoW->lpszSubKey);
    mp->isUnicode = TRUE;

    return create_mru_list(mp);
}

/**************************************************************************
 *                  CreateMRUListLazyA [COMCTL32.157]
 *
 * Creates a most-recently-used list.
 *
 * PARAMS
 *     lpcml    [I] ptr to CREATEMRULIST structure.
 *     dwParam2 [I] Unknown
 *     dwParam3 [I] Unknown
 *     dwParam4 [I] Unknown
 *
 * RETURNS
 *     Handle to MRU list.
 */
HANDLE WINAPI CreateMRUListLazyA (const MRUINFOA *lpcml, DWORD dwParam2,
                                  DWORD dwParam3, DWORD dwParam4)
{
    LPWINEMRULIST mp;
    DWORD len;

    /* Native does not check for a NULL lpcml */

    if (!lpcml->hKey || IsBadStringPtrA(lpcml->lpszSubKey, -1))
	return 0;

    mp = Alloc(sizeof(WINEMRULIST));
    memcpy(&mp->extview, lpcml, sizeof(MRUINFOA));
    len = MultiByteToWideChar(CP_ACP, 0, lpcml->lpszSubKey, -1, NULL, 0);
    mp->extview.lpszSubKey = Alloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, lpcml->lpszSubKey, -1,
			mp->extview.lpszSubKey, len);
    mp->isUnicode = FALSE;
    return create_mru_list(mp);
}

/**************************************************************************
 *              CreateMRUListW [COMCTL32.400]
 *
 * See CreateMRUListA.
 */
HANDLE WINAPI CreateMRUListW (const MRUINFOW *infoW)
{
    return CreateMRUListLazyW(infoW, 0, 0, 0);
}

/**************************************************************************
 *              CreateMRUListA [COMCTL32.151]
 *
 * Creates a most-recently-used list.
 *
 * PARAMS
 *     lpcml [I] ptr to CREATEMRULIST structure.
 *
 * RETURNS
 *     Handle to MRU list.
 */
HANDLE WINAPI CreateMRUListA (const MRUINFOA *lpcml)
{
     return CreateMRUListLazyA (lpcml, 0, 0, 0);
}


/**************************************************************************
 *                EnumMRUListW [COMCTL32.403]
 *
 * Enumerate item in a most-recently-used list
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
INT WINAPI EnumMRUListW (HANDLE hList, INT nItemPos, LPVOID lpBuffer,
                         DWORD nBufferSize)
{
    const WINEMRULIST *mp = hList;
    const WINEMRUITEM *witem;
    INT desired, datasize;

    if (!mp) return -1;
    if ((nItemPos < 0) || !lpBuffer) return mp->cursize;
    if (nItemPos >= mp->cursize) return -1;
    desired = mp->realMRU[nItemPos];
    desired -= 'a';
    TRACE("nItemPos=%d, desired=%d\n", nItemPos, desired);
    witem = mp->array[desired];
    datasize = min( witem->size, nBufferSize );
    memcpy( lpBuffer, &witem->datastart, datasize);
    TRACE("(%p, %d, %p, %d): returning len=%d\n",
	  hList, nItemPos, lpBuffer, nBufferSize, datasize);
    return datasize;
}

/**************************************************************************
 *                EnumMRUListA [COMCTL32.154]
 *
 * See EnumMRUListW.
 */
INT WINAPI EnumMRUListA (HANDLE hList, INT nItemPos, LPVOID lpBuffer,
                         DWORD nBufferSize)
{
    const WINEMRULIST *mp = hList;
    LPWINEMRUITEM witem;
    INT desired, datasize;
    DWORD lenA;

    if (!mp) return -1;
    if ((nItemPos < 0) || !lpBuffer) return mp->cursize;
    if (nItemPos >= mp->cursize) return -1;
    desired = mp->realMRU[nItemPos];
    desired -= 'a';
    TRACE("nItemPos=%d, desired=%d\n", nItemPos, desired);
    witem = mp->array[desired];
    if(mp->extview.fFlags & MRU_BINARY) {
        datasize = min( witem->size, nBufferSize );
	memcpy( lpBuffer, &witem->datastart, datasize);
    } else {
        lenA = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)&witem->datastart, -1,
				   NULL, 0, NULL, NULL);
	datasize = min( lenA, nBufferSize );
	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)&witem->datastart, -1,
			    lpBuffer, datasize, NULL, NULL);
        ((char *)lpBuffer)[ datasize - 1 ] = '\0';
        datasize = lenA - 1;
    }
    TRACE("(%p, %d, %p, %d): returning len=%d\n",
	  hList, nItemPos, lpBuffer, nBufferSize, datasize);
    return datasize;
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

INT Str_GetPtrWtoA (LPCWSTR lpSrc, LPSTR lpDest, INT nMaxLen)
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
 * Str_GetPtrAtoW [internal]
 *
 * Converts a multibyte string into a unicode string
 *
 * PARAMS
 *     lpSrc   [I] Pointer to the multibyte source string
 *     lpDest  [O] Pointer to caller supplied storage for the unicode string
 *     nMaxLen [I] Size, in characters, of the destination buffer
 *
 * RETURNS
 *     Length, in characters, of the converted string.
 */

INT Str_GetPtrAtoW (LPCSTR lpSrc, LPWSTR lpDest, INT nMaxLen)
{
    INT len;

    TRACE("(%s %p %d)\n", debugstr_a(lpSrc), lpDest, nMaxLen);

    if (!lpDest && lpSrc)
	return MultiByteToWideChar(CP_ACP, 0, lpSrc, -1, 0, 0);

    if (nMaxLen == 0)
	return 0;

    if (lpSrc == NULL) {
	lpDest[0] = '\0';
	return 0;
    }

    len = MultiByteToWideChar(CP_ACP, 0, lpSrc, -1, 0, 0);
    if (len >= nMaxLen)
	len = nMaxLen - 1;

    MultiByteToWideChar(CP_ACP, 0, lpSrc, -1, lpDest, len);
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
BOOL Str_SetPtrAtoW (LPWSTR *lppDest, LPCSTR lpSrc)
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
        Free (*lppDest);
        *lppDest = NULL;
    }

    return TRUE;
}

/**************************************************************************
 * Str_SetPtrWtoA [internal]
 *
 * Converts a unicode string to a multi byte string.
 * If the pointer to the destination buffer is NULL a buffer is allocated.
 * If the destination buffer is too small to keep the converted wide
 * string the destination buffer is reallocated. If the source pointer is
 * NULL, the destination buffer is freed.
 *
 * PARAMS
 *     lppDest [I/O] pointer to a pointer to the destination buffer
 *     lpSrc   [I] pointer to a wide string
 *
 * RETURNS
 *     TRUE: conversion successful
 *     FALSE: error
 */
BOOL Str_SetPtrWtoA (LPSTR *lppDest, LPCWSTR lpSrc)
{
    TRACE("(%p %s)\n", lppDest, debugstr_w(lpSrc));

    if (lpSrc) {
        INT len = WideCharToMultiByte(CP_ACP,0,lpSrc,-1,NULL,0,NULL,FALSE);
        LPSTR ptr = ReAlloc (*lppDest, len*sizeof(CHAR));

        if (!ptr)
            return FALSE;
        WideCharToMultiByte(CP_ACP,0,lpSrc,-1,ptr,len,NULL,FALSE);
        *lppDest = ptr;
    }
    else {
        Free (*lppDest);
        *lppDest = NULL;
    }

    return TRUE;
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

static LRESULT DoNotify (const NOTIFYDATA *lpNotify, UINT uCode, LPNMHDR lpHdr)
{
    NMHDR nmhdr;
    LPNMHDR lpNmh = NULL;
    UINT idFrom = 0;

    TRACE("(%p %p %d %p 0x%08x)\n",
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

    return SendMessageW (lpNotify->hwndTo, WM_NOTIFY, idFrom, (LPARAM)lpNmh);
}


/**************************************************************************
 * SendNotify [COMCTL32.341]
 *
 * Sends a WM_NOTIFY message to the specified window.
 *
 * PARAMS
 *     hwndTo   [I] Window to receive the message
 *     hwndFrom [I] Window that the message is from (see notes)
 *     uCode    [I] Notification code
 *     lpHdr    [I] The NMHDR and any additional information to send or NULL
 *
 * RETURNS
 *     Success: return value from notification
 *     Failure: 0
 *
 * NOTES
 *     If hwndFrom is -1 then the identifier of the control sending the
 *     message is taken from the NMHDR structure.
 *     If hwndFrom is not -1 then lpHdr can be NULL.
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
 * Sends a WM_NOTIFY message to the specified window.
 *
 * PARAMS
 *     hwndFrom [I] Window to receive the message
 *     hwndTo   [I] Window that the message is from
 *     uCode    [I] Notification code
 *     lpHdr    [I] The NMHDR and any additional information to send or NULL
 *     dwParam5 [I] Unknown
 *
 * RETURNS
 *     Success: return value from notification
 *     Failure: 0
 *
 * NOTES
 *     If hwndFrom is -1 then the identifier of the control sending the
 *     message is taken from the NMHDR structure.
 *     If hwndFrom is not -1 then lpHdr can be NULL.
 */
LRESULT WINAPI SendNotifyEx (HWND hwndTo, HWND hwndFrom, UINT uCode,
                             LPNMHDR lpHdr, DWORD dwParam5)
{
    NOTIFYDATA notify;
    HWND hwndNotify;

    TRACE("(%p %p %d %p 0x%08x)\n",
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
