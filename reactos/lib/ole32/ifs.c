/*
 *	basic interfaces
 *
 *	Copyright 1997	Marcus Meissner
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

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "winerror.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/******************************************************************************
 *	IMalloc32 implementation
 *
 * NOTES
 *  For supporting CoRegisterMallocSpy the IMalloc implementation must know if
 *  a given memory block was allocated with a spy active.
 *
 *****************************************************************************/
/* set the vtable later */
extern ICOM_VTABLE(IMalloc) VT_IMalloc32;

typedef struct {
        ICOM_VFIELD(IMalloc);
        DWORD dummy;                /* nothing, we are static */
	IMallocSpy * pSpy;          /* the spy when active */
	DWORD SpyedAllocationsLeft; /* number of spyed allocations left */
	BOOL SpyReleasePending;     /* CoRevokeMallocSpy called with spyed allocations left*/
        LPVOID * SpyedBlocks;       /* root of the table */
        int SpyedBlockTableLength;  /* size of the table*/
} _Malloc32;

/* this is the static object instance */
_Malloc32 Malloc32 = {&VT_IMalloc32, 0, NULL, 0, 0, NULL, 0};

/* with a spy active all calls from pre to post methods are threadsave */
static CRITICAL_SECTION IMalloc32_SpyCS;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &IMalloc32_SpyCS,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": IMalloc32_SpyCS") }
};
static CRITICAL_SECTION IMalloc32_SpyCS = { &critsect_debug, -1, 0, 0, 0, 0 };

/* resize the old table */
static int SetSpyedBlockTableLength ( int NewLength )
{
	LPVOID *NewSpyedBlocks;

	if (!Malloc32.SpyedBlocks) NewSpyedBlocks = LocalAlloc(GMEM_ZEROINIT, NewLength);
	else NewSpyedBlocks = LocalReAlloc(Malloc32.SpyedBlocks, NewLength, GMEM_ZEROINIT);
	if (NewSpyedBlocks) {
		Malloc32.SpyedBlocks = NewSpyedBlocks;
		Malloc32.SpyedBlockTableLength = NewLength;
	}

	return NewSpyedBlocks != NULL;
}

/* add a location to the table */
static int AddMemoryLocation(LPVOID * pMem)
{
        LPVOID * Current;

	/* allocate the table if not already allocated */
	if (!Malloc32.SpyedBlockTableLength) {
            if (!SetSpyedBlockTableLength(0x1000)) return 0;
	}

	/* find a free location */
	Current = Malloc32.SpyedBlocks;
	while (*Current) {
            Current++;
	    if (Current >= Malloc32.SpyedBlocks + Malloc32.SpyedBlockTableLength) {
	        /* no more space in table, grow it */
	        if (!SetSpyedBlockTableLength( Malloc32.SpyedBlockTableLength + 0x1000 )) return 0;
	    }
	};

	/* put the location in our table */
	*Current = pMem;
        Malloc32.SpyedAllocationsLeft++;
	/*TRACE("%lu\n",Malloc32.SpyedAllocationsLeft);*/
        return 1;
}

static int RemoveMemoryLocation(LPVOID * pMem)
{
        LPVOID * Current;

	/* allocate the table if not already allocated */
	if (!Malloc32.SpyedBlockTableLength) {
            if (!SetSpyedBlockTableLength(0x1000)) return 0;
	}

	Current = Malloc32.SpyedBlocks;

	/* find the location */
	while (*Current != pMem) {
            Current++;
	    if (Current >= Malloc32.SpyedBlocks + Malloc32.SpyedBlockTableLength)  return 0;      /* not found  */
	}

	/* location found */
        Malloc32.SpyedAllocationsLeft--;
	/*TRACE("%lu\n",Malloc32.SpyedAllocationsLeft);*/
	*Current = NULL;
	return 1;
}

/******************************************************************************
 *	IMalloc32_QueryInterface	[VTABLE]
 */
static HRESULT WINAPI IMalloc_fnQueryInterface(LPMALLOC iface,REFIID refiid,LPVOID *obj) {

	TRACE("(%s,%p)\n",debugstr_guid(refiid),obj);

	if (IsEqualIID(&IID_IUnknown,refiid) || IsEqualIID(&IID_IMalloc,refiid)) {
		*obj = (LPMALLOC)&Malloc32;
		return S_OK;
	}
	return E_NOINTERFACE;
}

/******************************************************************************
 *	IMalloc32_AddRefRelease		[VTABLE]
 */
static ULONG WINAPI IMalloc_fnAddRefRelease (LPMALLOC iface) {
	return 1;
}

/******************************************************************************
 *	IMalloc32_Alloc 		[VTABLE]
 */
static LPVOID WINAPI IMalloc_fnAlloc(LPMALLOC iface, DWORD cb) {

	LPVOID addr;

	TRACE("(%ld)\n",cb);

	if(Malloc32.pSpy) {
	    DWORD preAllocResult;
	    
	    EnterCriticalSection(&IMalloc32_SpyCS);
	    preAllocResult = IMallocSpy_PreAlloc(Malloc32.pSpy, cb);
	    if ((cb != 0) && (preAllocResult == 0)) {
		/* PreAlloc can force Alloc to fail, but not if cb == 0 */
		TRACE("returning null\n");
		LeaveCriticalSection(&IMalloc32_SpyCS);
		return NULL;
	    }
	}
 	
	addr = HeapAlloc(GetProcessHeap(),0,cb);

	if(Malloc32.pSpy) {
	    addr = IMallocSpy_PostAlloc(Malloc32.pSpy, addr);
	    if (addr) AddMemoryLocation(addr);
	    LeaveCriticalSection(&IMalloc32_SpyCS);
	}

	TRACE("--(%p)\n",addr);
	return addr;
}

/******************************************************************************
 * IMalloc32_Realloc [VTABLE]
 */
static LPVOID WINAPI IMalloc_fnRealloc(LPMALLOC iface,LPVOID pv,DWORD cb) {

	LPVOID pNewMemory;

	TRACE("(%p,%ld)\n",pv,cb);

	if(Malloc32.pSpy) {
	    LPVOID pRealMemory;
	    BOOL fSpyed;

	    EnterCriticalSection(&IMalloc32_SpyCS);
            fSpyed = RemoveMemoryLocation(pv);
            cb = IMallocSpy_PreRealloc(Malloc32.pSpy, pv, cb, &pRealMemory, fSpyed);

	    /* check if can release the spy */
	    if(Malloc32.SpyReleasePending && !Malloc32.SpyedAllocationsLeft) {
	        IMallocSpy_Release(Malloc32.pSpy);
		Malloc32.SpyReleasePending = FALSE;
		Malloc32.pSpy = NULL;
	    }

	    if (0==cb) {
	        /* PreRealloc can force Realloc to fail */
                LeaveCriticalSection(&IMalloc32_SpyCS);
		return NULL;
	    }
	    pv = pRealMemory;
	}

        if (!pv) pNewMemory = HeapAlloc(GetProcessHeap(),0,cb);
	else if (cb) pNewMemory = HeapReAlloc(GetProcessHeap(),0,pv,cb);
	else {
	    HeapFree(GetProcessHeap(),0,pv);
	    pNewMemory = NULL;
	}

	if(Malloc32.pSpy) {
	    pNewMemory = IMallocSpy_PostRealloc(Malloc32.pSpy, pNewMemory, TRUE);
	    if (pNewMemory) AddMemoryLocation(pNewMemory);
            LeaveCriticalSection(&IMalloc32_SpyCS);
	}

	TRACE("--(%p)\n",pNewMemory);
	return pNewMemory;
}

/******************************************************************************
 * IMalloc32_Free [VTABLE]
 */
static VOID WINAPI IMalloc_fnFree(LPMALLOC iface,LPVOID pv) {

	BOOL fSpyed = 0;

	TRACE("(%p)\n",pv);

	if(Malloc32.pSpy) {
            EnterCriticalSection(&IMalloc32_SpyCS);
            fSpyed = RemoveMemoryLocation(pv);
	    pv = IMallocSpy_PreFree(Malloc32.pSpy, pv, fSpyed);
	}

	HeapFree(GetProcessHeap(),0,pv);

	if(Malloc32.pSpy) {
	    IMallocSpy_PostFree(Malloc32.pSpy, fSpyed);

	    /* check if can release the spy */
	    if(Malloc32.SpyReleasePending && !Malloc32.SpyedAllocationsLeft) {
	        IMallocSpy_Release(Malloc32.pSpy);
		Malloc32.SpyReleasePending = FALSE;
		Malloc32.pSpy = NULL;
	    }

	    LeaveCriticalSection(&IMalloc32_SpyCS);
        }
}

/******************************************************************************
 * IMalloc32_GetSize [VTABLE]
 *
 * NOTES
 *  FIXME returns:
 *      win95:  size allocated (4 byte boundarys)
 *      win2k:  size originally requested !!! (allocated on 8 byte boundarys)
 */
static DWORD WINAPI IMalloc_fnGetSize(LPMALLOC iface,LPVOID pv) {

	DWORD cb;
	BOOL fSpyed = 0;

	TRACE("(%p)\n",pv);

	if(Malloc32.pSpy) {
            EnterCriticalSection(&IMalloc32_SpyCS);
	    pv = IMallocSpy_PreGetSize(Malloc32.pSpy, pv, fSpyed);
	}

	cb = HeapSize(GetProcessHeap(),0,pv);

	if(Malloc32.pSpy) {
	    cb = IMallocSpy_PostGetSize(Malloc32.pSpy, cb, fSpyed);
	    LeaveCriticalSection(&IMalloc32_SpyCS);
	}

	return cb;
}

/******************************************************************************
 * IMalloc32_DidAlloc [VTABLE]
 */
static INT WINAPI IMalloc_fnDidAlloc(LPMALLOC iface,LPVOID pv) {

	BOOL fSpyed = 0;
	int didAlloc;

	TRACE("(%p)\n",pv);

	if(Malloc32.pSpy) {
            EnterCriticalSection(&IMalloc32_SpyCS);
	    pv = IMallocSpy_PreDidAlloc(Malloc32.pSpy, pv, fSpyed);
	}

	didAlloc = -1;

	if(Malloc32.pSpy) {
	    didAlloc = IMallocSpy_PostDidAlloc(Malloc32.pSpy, pv, fSpyed, didAlloc);
            LeaveCriticalSection(&IMalloc32_SpyCS);
	}
	return didAlloc;
}

/******************************************************************************
 * IMalloc32_HeapMinimize [VTABLE]
 */
static VOID WINAPI IMalloc_fnHeapMinimize(LPMALLOC iface) {
	TRACE("()\n");

	if(Malloc32.pSpy) {
            EnterCriticalSection(&IMalloc32_SpyCS);
	    IMallocSpy_PreHeapMinimize(Malloc32.pSpy);
	}

	if(Malloc32.pSpy) {
	    IMallocSpy_PostHeapMinimize(Malloc32.pSpy);
            LeaveCriticalSection(&IMalloc32_SpyCS);
	}
}

static ICOM_VTABLE(IMalloc) VT_IMalloc32 =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IMalloc_fnQueryInterface,
	IMalloc_fnAddRefRelease,
	IMalloc_fnAddRefRelease,
	IMalloc_fnAlloc,
	IMalloc_fnRealloc,
	IMalloc_fnFree,
	IMalloc_fnGetSize,
	IMalloc_fnDidAlloc,
	IMalloc_fnHeapMinimize
};

/******************************************************************************
 *	IMallocSpy implementation
 *****************************************************************************/

/* set the vtable later */
extern ICOM_VTABLE(IMallocSpy) VT_IMallocSpy;

typedef struct {
        ICOM_VFIELD(IMallocSpy);
        DWORD ref;
} _MallocSpy;

/* this is the static object instance */
_MallocSpy MallocSpy = {&VT_IMallocSpy, 0};

/******************************************************************************
 *	IMalloc32_QueryInterface	[VTABLE]
 */
static HRESULT WINAPI IMallocSpy_fnQueryInterface(LPMALLOCSPY iface,REFIID refiid,LPVOID *obj)
{

	TRACE("(%s,%p)\n",debugstr_guid(refiid),obj);

	if (IsEqualIID(&IID_IUnknown,refiid) || IsEqualIID(&IID_IMallocSpy,refiid)) {
		*obj = (LPMALLOC)&MallocSpy;
		return S_OK;
	}
	return E_NOINTERFACE;
}

/******************************************************************************
 *	IMalloc32_AddRef		[VTABLE]
 */
static ULONG WINAPI IMallocSpy_fnAddRef (LPMALLOCSPY iface)
{

    ICOM_THIS (_MallocSpy, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return ++(This->ref);
}

/******************************************************************************
 *	IMalloc32_AddRelease		[VTABLE]
 *
 * NOTES
 *   Our MallocSpy is static. If the count reaches 0 we dump the leaks
 */
static ULONG WINAPI IMallocSpy_fnRelease (LPMALLOCSPY iface)
{

    ICOM_THIS (_MallocSpy, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    if (!--(This->ref)) {
        /* our allocation list MUST be empty here */
    }
    return This->ref;
}

static ULONG WINAPI IMallocSpy_fnPreAlloc(LPMALLOCSPY iface, ULONG cbRequest)
{
    ICOM_THIS (_MallocSpy, iface);
    TRACE ("(%p)->(%lu)\n", This, cbRequest);
    return cbRequest;
}
static PVOID WINAPI IMallocSpy_fnPostAlloc(LPMALLOCSPY iface, void* pActual)
{
    ICOM_THIS (_MallocSpy, iface);
    TRACE ("(%p)->(%p)\n", This, pActual);
    return pActual;
}

static PVOID WINAPI IMallocSpy_fnPreFree(LPMALLOCSPY iface, void* pRequest, BOOL fSpyed)
{
    ICOM_THIS (_MallocSpy, iface);
    TRACE ("(%p)->(%p %u)\n", This, pRequest, fSpyed);
    return pRequest;
}
static void  WINAPI IMallocSpy_fnPostFree(LPMALLOCSPY iface, BOOL fSpyed)
{
    ICOM_THIS (_MallocSpy, iface);
    TRACE ("(%p)->(%u)\n", This, fSpyed);
}

static ULONG WINAPI IMallocSpy_fnPreRealloc(LPMALLOCSPY iface, void* pRequest, ULONG cbRequest, void** ppNewRequest, BOOL fSpyed)
{
    ICOM_THIS (_MallocSpy, iface);
    TRACE ("(%p)->(%p %lu %u)\n", This, pRequest, cbRequest, fSpyed);
    *ppNewRequest = pRequest;
    return cbRequest;
}

static PVOID WINAPI IMallocSpy_fnPostRealloc(LPMALLOCSPY iface, void* pActual, BOOL fSpyed)
{
    ICOM_THIS (_MallocSpy, iface);
    TRACE ("(%p)->(%p %u)\n", This, pActual, fSpyed);
    return pActual;
}

static PVOID WINAPI IMallocSpy_fnPreGetSize(LPMALLOCSPY iface, void* pRequest, BOOL fSpyed)
{
    ICOM_THIS (_MallocSpy, iface);
    TRACE ("(%p)->(%p %u)\n", This,  pRequest, fSpyed);
    return pRequest;
}

static ULONG WINAPI IMallocSpy_fnPostGetSize(LPMALLOCSPY iface, ULONG cbActual, BOOL fSpyed)
{
    ICOM_THIS (_MallocSpy, iface);
    TRACE ("(%p)->(%lu %u)\n", This, cbActual, fSpyed);
    return cbActual;
}

static PVOID WINAPI IMallocSpy_fnPreDidAlloc(LPMALLOCSPY iface, void* pRequest, BOOL fSpyed)
{
    ICOM_THIS (_MallocSpy, iface);
    TRACE ("(%p)->(%p %u)\n", This, pRequest, fSpyed);
    return pRequest;
}

static int WINAPI IMallocSpy_fnPostDidAlloc(LPMALLOCSPY iface, void* pRequest, BOOL fSpyed, int fActual)
{
    ICOM_THIS (_MallocSpy, iface);
    TRACE ("(%p)->(%p %u %u)\n", This, pRequest, fSpyed, fActual);
    return fActual;
}

static void WINAPI IMallocSpy_fnPreHeapMinimize(LPMALLOCSPY iface)
{
    ICOM_THIS (_MallocSpy, iface);
    TRACE ("(%p)->()\n", This);
}

static void WINAPI IMallocSpy_fnPostHeapMinimize(LPMALLOCSPY iface)
{
    ICOM_THIS (_MallocSpy, iface);
    TRACE ("(%p)->()\n", This);
}

static void MallocSpyDumpLeaks() {
        TRACE("leaks: %lu\n", Malloc32.SpyedAllocationsLeft);
}

static ICOM_VTABLE(IMallocSpy) VT_IMallocSpy =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IMallocSpy_fnQueryInterface,
	IMallocSpy_fnAddRef,
	IMallocSpy_fnRelease,
	IMallocSpy_fnPreAlloc,
	IMallocSpy_fnPostAlloc,
	IMallocSpy_fnPreFree,
	IMallocSpy_fnPostFree,
	IMallocSpy_fnPreRealloc,
	IMallocSpy_fnPostRealloc,
	IMallocSpy_fnPreGetSize,
	IMallocSpy_fnPostGetSize,
	IMallocSpy_fnPreDidAlloc,
	IMallocSpy_fnPostDidAlloc,
	IMallocSpy_fnPreHeapMinimize,
	IMallocSpy_fnPostHeapMinimize
};

/******************************************************************************
 *		CoGetMalloc	[OLE32.@]
 *
 * RETURNS
 *	The win32 IMalloc
 */
HRESULT WINAPI CoGetMalloc(DWORD dwMemContext, LPMALLOC *lpMalloc)
{
        *lpMalloc = (LPMALLOC)&Malloc32;
        return S_OK;
}

/***********************************************************************
 *           CoTaskMemAlloc     [OLE32.@]
 * RETURNS
 * 	pointer to newly allocated block
 */
LPVOID WINAPI CoTaskMemAlloc(ULONG size)
{
        return IMalloc_Alloc((LPMALLOC)&Malloc32,size);
}
/***********************************************************************
 *           CoTaskMemFree      [OLE32.@]
 */
VOID WINAPI CoTaskMemFree(LPVOID ptr)
{
        IMalloc_Free((LPMALLOC)&Malloc32, ptr);
}

/***********************************************************************
 *           CoTaskMemRealloc   [OLE32.@]
 * RETURNS
 * 	pointer to newly allocated block
 */
LPVOID WINAPI CoTaskMemRealloc(LPVOID pvOld, ULONG size)
{
        return IMalloc_Realloc((LPMALLOC)&Malloc32, pvOld, size);
}

/***********************************************************************
 *           CoRegisterMallocSpy        [OLE32.@]
 *
 * NOTES
 *  if a mallocspy is already registered, we cant do it again since
 *  only the spy knows, how to free a memory block
 */
HRESULT WINAPI CoRegisterMallocSpy(LPMALLOCSPY pMallocSpy)
{
	IMallocSpy* pSpy;
        HRESULT hres = E_INVALIDARG;

	TRACE("\n");

	/* HACK TO ACTIVATE OUT SPY */
	if (pMallocSpy == (LPVOID)-1) pMallocSpy =(IMallocSpy*)&MallocSpy;

	if(Malloc32.pSpy) return CO_E_OBJISREG;

        EnterCriticalSection(&IMalloc32_SpyCS);

	if (SUCCEEDED(IUnknown_QueryInterface(pMallocSpy, &IID_IMallocSpy, (LPVOID*)&pSpy))) {
	    Malloc32.pSpy = pSpy;
	    hres = S_OK;
	}

	LeaveCriticalSection(&IMalloc32_SpyCS);

	return hres;
}

/***********************************************************************
 *           CoRevokeMallocSpy  [OLE32.@]
 *
 * NOTES
 *  we can't rewoke a malloc spy as long as memory blocks allocated with
 *  the spy are active since only the spy knows how to free them
 */
HRESULT WINAPI CoRevokeMallocSpy(void)
{
	HRESULT hres = S_OK;
	TRACE("\n");

        EnterCriticalSection(&IMalloc32_SpyCS);

	/* if it's our spy it's time to dump the leaks */
	if (Malloc32.pSpy == (IMallocSpy*)&MallocSpy) {
	    MallocSpyDumpLeaks();
	}

	if (Malloc32.SpyedAllocationsLeft) {
	    TRACE("SpyReleasePending with %lu allocations left\n", Malloc32.SpyedAllocationsLeft);
	    Malloc32.SpyReleasePending = TRUE;
	    hres = E_ACCESSDENIED;
	} else {
	    IMallocSpy_Release(Malloc32.pSpy);
	    Malloc32.pSpy = NULL;
        }
	LeaveCriticalSection(&IMalloc32_SpyCS);

	return S_OK;
}

/******************************************************************************
 *		IsValidInterface	[OLE32.@]
 *
 * RETURNS
 *  True, if the passed pointer is a valid interface
 */
BOOL WINAPI IsValidInterface(
	LPUNKNOWN punk	/* [in] interface to be tested */
) {
	return !(
		IsBadReadPtr(punk,4)					||
		IsBadReadPtr(punk->lpVtbl,4)				||
		IsBadReadPtr(punk->lpVtbl->QueryInterface,9)	||
		IsBadCodePtr((FARPROC)punk->lpVtbl->QueryInterface)
	);
}
