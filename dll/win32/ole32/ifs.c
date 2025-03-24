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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "winerror.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(olemalloc);

/******************************************************************************
 *	IMalloc32 implementation
 *
 * NOTES
 *  For supporting CoRegisterMallocSpy the IMalloc implementation must know if
 *  a given memory block was allocated with a spy active.
 *
 *****************************************************************************/
/* set the vtable later */
static const IMallocVtbl VT_IMalloc32;

struct allocator
{
        IMalloc IMalloc_iface;
	IMallocSpy * pSpy;          /* the spy when active */
	DWORD SpyedAllocationsLeft; /* number of spyed allocations left */
	BOOL SpyReleasePending;     /* CoRevokeMallocSpy called with spyed allocations left*/
        LPVOID * SpyedBlocks;       /* root of the table */
        DWORD SpyedBlockTableLength;/* size of the table*/
};

static struct allocator Malloc32 = { .IMalloc_iface.lpVtbl = &VT_IMalloc32 };

/* with a spy active all calls from pre to post methods are threadsafe */
static CRITICAL_SECTION IMalloc32_SpyCS;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &IMalloc32_SpyCS,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": IMalloc32_SpyCS") }
};
static CRITICAL_SECTION IMalloc32_SpyCS = { &critsect_debug, -1, 0, 0, 0, 0 };

/* resize the old table */
static BOOL SetSpyedBlockTableLength ( DWORD NewLength )
{
	LPVOID *NewSpyedBlocks;

	if (!Malloc32.SpyedBlocks) NewSpyedBlocks = LocalAlloc(LMEM_ZEROINIT, NewLength * sizeof(PVOID));
	else NewSpyedBlocks = LocalReAlloc(Malloc32.SpyedBlocks, NewLength * sizeof(PVOID), LMEM_ZEROINIT | LMEM_MOVEABLE);
	if (NewSpyedBlocks) {
		Malloc32.SpyedBlocks = NewSpyedBlocks;
		Malloc32.SpyedBlockTableLength = NewLength;
	}

	return NewSpyedBlocks != NULL;
}

/* add a location to the table */
static BOOL AddMemoryLocation(LPVOID * pMem)
{
        LPVOID * Current;

	/* allocate the table if not already allocated */
        if (!Malloc32.SpyedBlockTableLength && !SetSpyedBlockTableLength(0x1000))
            return FALSE;

	/* find a free location */
	Current = Malloc32.SpyedBlocks;
	while (*Current) {
            Current++;
	    if (Current >= Malloc32.SpyedBlocks + Malloc32.SpyedBlockTableLength) {
	        /* no more space in table, grow it */
                DWORD old_length = Malloc32.SpyedBlockTableLength;
                if (!SetSpyedBlockTableLength( Malloc32.SpyedBlockTableLength + 0x1000))
                    return FALSE;
                Current = Malloc32.SpyedBlocks + old_length;
	    }
	};

	/* put the location in our table */
	*Current = pMem;
        Malloc32.SpyedAllocationsLeft++;
	/*TRACE("%lu\n",Malloc32.SpyedAllocationsLeft);*/
        return TRUE;
}

static void** mallocspy_is_allocation_spyed(const void *mem)
{
    void **current = Malloc32.SpyedBlocks;

    while (*current != mem)
    {
        current++;
        if (current >= Malloc32.SpyedBlocks + Malloc32.SpyedBlockTableLength)
            return NULL;
    }

    return current;
}

static BOOL mallocspy_remove_spyed_memory(const void *mem)
{
    void **current;

    if (!Malloc32.SpyedBlockTableLength)
        return FALSE;

    if (!(current = mallocspy_is_allocation_spyed(mem)))
        return FALSE;

    Malloc32.SpyedAllocationsLeft--;
    *current = NULL;
    return TRUE;
 }
/******************************************************************************
 *	IMalloc32_QueryInterface	[VTABLE]
 */
static HRESULT WINAPI IMalloc_fnQueryInterface(IMalloc *iface, REFIID refiid, void **obj)
{
	TRACE("(%s,%p)\n",debugstr_guid(refiid),obj);

	if (IsEqualIID(&IID_IUnknown,refiid) || IsEqualIID(&IID_IMalloc,refiid)) {
		*obj = &Malloc32;
		return S_OK;
	}
	return E_NOINTERFACE;
}

/******************************************************************************
 *	IMalloc32_AddRefRelease		[VTABLE]
 */
static ULONG WINAPI IMalloc_fnAddRefRelease(IMalloc *iface)
{
	return 1;
}

/******************************************************************************
 *	IMalloc32_Alloc 		[VTABLE]
 */
static void * WINAPI IMalloc_fnAlloc(IMalloc *iface, SIZE_T cb)
{
	void *addr;

	TRACE("(%ld)\n",cb);

	if(Malloc32.pSpy) {
	    SIZE_T preAllocResult;
	    
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
static void * WINAPI IMalloc_fnRealloc(IMalloc *iface, void *pv, SIZE_T cb)
{
	void *pNewMemory;

	TRACE("(%p,%ld)\n",pv,cb);

	if(Malloc32.pSpy) {
	    void *pRealMemory;
	    BOOL fSpyed;

	    EnterCriticalSection(&IMalloc32_SpyCS);
            fSpyed = mallocspy_remove_spyed_memory(pv);
            cb = IMallocSpy_PreRealloc(Malloc32.pSpy, pv, cb, &pRealMemory, fSpyed);

	    /* check if can release the spy */
	    if(Malloc32.SpyReleasePending && !Malloc32.SpyedAllocationsLeft) {
	        IMallocSpy_Release(Malloc32.pSpy);
		Malloc32.SpyReleasePending = FALSE;
		Malloc32.pSpy = NULL;
		LeaveCriticalSection(&IMalloc32_SpyCS);
	    }

	    if (0==cb) {
		/* PreRealloc can force Realloc to fail */
		if (Malloc32.pSpy)
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
static void WINAPI IMalloc_fnFree(IMalloc *iface, void *mem)
{
    BOOL spyed_block = FALSE, spy_active = FALSE;

    TRACE("(%p)\n", mem);

    if (!mem)
        return;

    if (Malloc32.pSpy)
    {
        EnterCriticalSection(&IMalloc32_SpyCS);
        spyed_block = mallocspy_remove_spyed_memory(mem);
        spy_active = TRUE;
        mem = IMallocSpy_PreFree(Malloc32.pSpy, mem, spyed_block);
    }

    HeapFree(GetProcessHeap(), 0, mem);

    if (spy_active)
    {
        IMallocSpy_PostFree(Malloc32.pSpy, spyed_block);

        /* check if can release the spy */
        if (Malloc32.SpyReleasePending && !Malloc32.SpyedAllocationsLeft)
        {
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
static SIZE_T WINAPI IMalloc_fnGetSize(IMalloc *iface, void *mem)
{
    BOOL spyed_block = FALSE, spy_active = FALSE;
    SIZE_T size;

    TRACE("(%p)\n", mem);

    if (!mem)
        return (SIZE_T)-1;

    if (Malloc32.pSpy)
    {
        EnterCriticalSection(&IMalloc32_SpyCS);
        spyed_block = !!mallocspy_is_allocation_spyed(mem);
        spy_active = TRUE;
        mem = IMallocSpy_PreGetSize(Malloc32.pSpy, mem, spyed_block);
    }

    size = HeapSize(GetProcessHeap(), 0, mem);

    if (spy_active)
    {
        size = IMallocSpy_PostGetSize(Malloc32.pSpy, size, spyed_block);
        LeaveCriticalSection(&IMalloc32_SpyCS);
    }
    return size;
}

/******************************************************************************
 * IMalloc32_DidAlloc [VTABLE]
 */
static INT WINAPI IMalloc_fnDidAlloc(IMalloc *iface, void *mem)
{
    BOOL spyed_block = FALSE, spy_active = FALSE;
    int did_alloc;

    TRACE("(%p)\n", mem);

    if (!mem)
        return -1;

    if (Malloc32.pSpy)
    {
        EnterCriticalSection(&IMalloc32_SpyCS);
        spyed_block = !!mallocspy_is_allocation_spyed(mem);
        spy_active = TRUE;
        mem = IMallocSpy_PreDidAlloc(Malloc32.pSpy, mem, spyed_block);
    }

    did_alloc = HeapValidate(GetProcessHeap(), 0, mem);

    if (spy_active)
    {
        did_alloc = IMallocSpy_PostDidAlloc(Malloc32.pSpy, mem, spyed_block, did_alloc);
        LeaveCriticalSection(&IMalloc32_SpyCS);
    }

    return did_alloc;
}

/******************************************************************************
 * IMalloc32_HeapMinimize [VTABLE]
 */
static void WINAPI IMalloc_fnHeapMinimize(IMalloc *iface)
{
    BOOL spy_active = FALSE;

    TRACE("()\n");

    if (Malloc32.pSpy)
    {
        EnterCriticalSection(&IMalloc32_SpyCS);
        spy_active = TRUE;
        IMallocSpy_PreHeapMinimize(Malloc32.pSpy);
    }

    if (spy_active)
    {
        IMallocSpy_PostHeapMinimize(Malloc32.pSpy);
        LeaveCriticalSection(&IMalloc32_SpyCS);
    }
}

static const IMallocVtbl VT_IMalloc32 =
{
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
 *		CoGetMalloc	[OLE32.@]
 *
 * Retrieves the current IMalloc interface for the process.
 *
 * PARAMS
 *  context [I] Should always be MEMCTX_TASK.
 *  imalloc [O] Address where memory allocator object will be stored.
 *
 * RETURNS
 *	Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI CoGetMalloc(DWORD context, IMalloc **imalloc)
{
    if (context != MEMCTX_TASK) {
        *imalloc = NULL;
        return E_INVALIDARG;
    }

    *imalloc = &Malloc32.IMalloc_iface;
    return S_OK;
}

/***********************************************************************
 *           CoTaskMemAlloc     [OLE32.@]
 *
 * Allocates memory using the current process memory allocator.
 *
 * PARAMS
 *  size [I] Size of the memory block to allocate.
 *
 * RETURNS
 * 	Success: Pointer to newly allocated memory block.
 *  Failure: NULL.
 */
LPVOID WINAPI CoTaskMemAlloc(SIZE_T size)
{
        return IMalloc_Alloc(&Malloc32.IMalloc_iface,size);
}

/***********************************************************************
 *           CoTaskMemFree      [OLE32.@]
 *
 * Frees memory allocated from the current process memory allocator.
 *
 * PARAMS
 *  ptr [I] Memory block to free.
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI CoTaskMemFree(LPVOID ptr)
{
        IMalloc_Free(&Malloc32.IMalloc_iface, ptr);
}

/***********************************************************************
 *           CoTaskMemRealloc   [OLE32.@]
 *
 * Allocates memory using the current process memory allocator.
 *
 * PARAMS
 *  pvOld [I] Pointer to old memory block.
 *  size  [I] Size of the new memory block.
 *
 * RETURNS
 * 	Success: Pointer to newly allocated memory block.
 *  Failure: NULL.
 */
LPVOID WINAPI CoTaskMemRealloc(LPVOID pvOld, SIZE_T size)
{
        return IMalloc_Realloc(&Malloc32.IMalloc_iface, pvOld, size);
}

/***********************************************************************
 *           CoRegisterMallocSpy        [OLE32.@]
 *
 * Registers an object that receives notifications on memory allocations and
 * frees.
 *
 * PARAMS
 *  pMallocSpy [I] New spy object.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * NOTES
 *  if a mallocspy is already registered, we can't do it again since
 *  only the spy knows, how to free a memory block
 */
HRESULT WINAPI CoRegisterMallocSpy(LPMALLOCSPY pMallocSpy)
{
	IMallocSpy* pSpy;
        HRESULT hres = E_INVALIDARG;

	TRACE("%p\n", pMallocSpy);

	if(!pMallocSpy) return E_INVALIDARG;

        EnterCriticalSection(&IMalloc32_SpyCS);

	if (Malloc32.pSpy)
	    hres = CO_E_OBJISREG;
	else if (SUCCEEDED(IMallocSpy_QueryInterface(pMallocSpy, &IID_IMallocSpy, (void**)&pSpy))) {
	    Malloc32.pSpy = pSpy;
	    hres = S_OK;
	}

	LeaveCriticalSection(&IMalloc32_SpyCS);

	return hres;
}

/***********************************************************************
 *           CoRevokeMallocSpy  [OLE32.@]
 *
 * Revokes a previously registered object that receives notifications on memory
 * allocations and frees.
 *
 * PARAMS
 *  pMallocSpy [I] New spy object.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * NOTES
 *  we can't revoke a malloc spy as long as memory blocks allocated with
 *  the spy are active since only the spy knows how to free them
 */
HRESULT WINAPI CoRevokeMallocSpy(void)
{
	HRESULT hres = S_OK;
	TRACE("\n");

        EnterCriticalSection(&IMalloc32_SpyCS);

	if (!Malloc32.pSpy)
	    hres = CO_E_OBJNOTREG;
	else if (Malloc32.SpyedAllocationsLeft) {
            TRACE("SpyReleasePending with %u allocations left\n", Malloc32.SpyedAllocationsLeft);
	    Malloc32.SpyReleasePending = TRUE;
	    hres = E_ACCESSDENIED;
	} else {
	    IMallocSpy_Release(Malloc32.pSpy);
	    Malloc32.pSpy = NULL;
        }
	LeaveCriticalSection(&IMalloc32_SpyCS);

	return hres;
}

/******************************************************************************
 *		IsValidInterface	[OLE32.@]
 *
 * Determines whether a pointer is a valid interface.
 *
 * PARAMS
 *  punk [I] Interface to be tested.
 *
 * RETURNS
 *  TRUE, if the passed pointer is a valid interface, or FALSE otherwise.
 */
BOOL WINAPI IsValidInterface(LPUNKNOWN punk)
{
	return !(
		IsBadReadPtr(punk,4)					||
		IsBadReadPtr(punk->lpVtbl,4)				||
		IsBadReadPtr(punk->lpVtbl->QueryInterface,9)	||
		IsBadCodePtr((FARPROC)punk->lpVtbl->QueryInterface)
	);
}
