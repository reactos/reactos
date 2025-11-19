/*
 * Copyright 1997 Marcus Meissner
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

#define COBJMACROS

#include "oleauto.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(olemalloc);

static const IMallocVtbl allocator_vtbl;

struct allocator
{
    IMalloc IMalloc_iface;
    IMallocSpy *spy;
    DWORD spyed_allocations;
    BOOL spy_release_pending; /* CoRevokeMallocSpy called with spyed allocations left */
    void **blocks;
    DWORD blocks_length;
};

static struct allocator allocator = { .IMalloc_iface.lpVtbl = &allocator_vtbl };

static CRITICAL_SECTION allocspy_cs;
static CRITICAL_SECTION_DEBUG allocspy_cs_debug =
{
    0, 0, &allocspy_cs,
    { &allocspy_cs_debug.ProcessLocksList, &allocspy_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": allocspy_cs") }
};
static CRITICAL_SECTION allocspy_cs = { &allocspy_cs_debug, -1, 0, 0, 0, 0 };

static BOOL mallocspy_grow(DWORD length)
{
    void **blocks;

    if (!allocator.blocks) blocks = LocalAlloc(LMEM_ZEROINIT, length * sizeof(void *));
    else blocks = LocalReAlloc(allocator.blocks, length * sizeof(void *), LMEM_ZEROINIT | LMEM_MOVEABLE);
    if (blocks)
    {
        allocator.blocks = blocks;
        allocator.blocks_length = length;
    }

    return blocks != NULL;
}

static void mallocspy_add_mem(void *mem)
{
    void **current;

    if (!mem || (!allocator.blocks_length && !mallocspy_grow(0x1000)))
        return;

    /* Find a free location */
    current = allocator.blocks;
    while (*current)
    {
        current++;
        if (current >= allocator.blocks + allocator.blocks_length)
        {
            DWORD old_length = allocator.blocks_length;
            if (!mallocspy_grow(allocator.blocks_length + 0x1000))
                return;
            current = allocator.blocks + old_length;
        }
    }

    *current = mem;
    allocator.spyed_allocations++;
}

static void** mallocspy_is_allocation_spyed(const void *mem)
{
    void **current = allocator.blocks;

    while (*current != mem)
    {
        current++;
        if (current >= allocator.blocks + allocator.blocks_length)
            return NULL;
    }

    return current;
}

static BOOL mallocspy_remove_spyed_memory(const void *mem)
{
    void **current;

    if (!allocator.blocks_length)
        return FALSE;

    if (!(current = mallocspy_is_allocation_spyed(mem)))
        return FALSE;

    allocator.spyed_allocations--;
    *current = NULL;
    return TRUE;
}

static HRESULT WINAPI allocator_QueryInterface(IMalloc *iface, REFIID riid, void **obj)
{
    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IMalloc, riid))
    {
        *obj = &allocator.IMalloc_iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI allocator_AddRef(IMalloc *iface)
{
    return 2;
}

static ULONG WINAPI allocator_Release(IMalloc *iface)
{
    return 1;
}

static void * WINAPI allocator_Alloc(IMalloc *iface, SIZE_T cb)
{
    void *addr;

    TRACE("%Id.\n", cb);

    if (allocator.spy)
    {
        SIZE_T preAllocResult;

        EnterCriticalSection(&allocspy_cs);
        preAllocResult = IMallocSpy_PreAlloc(allocator.spy, cb);
        if (cb && !preAllocResult)
        {
            /* PreAlloc can force Alloc to fail, but not if cb == 0 */
            TRACE("returning null\n");
            LeaveCriticalSection(&allocspy_cs);
            return NULL;
        }
    }

    addr = HeapAlloc(GetProcessHeap(), 0, cb);

    if (allocator.spy)
    {
        addr = IMallocSpy_PostAlloc(allocator.spy, addr);
        mallocspy_add_mem(addr);
        LeaveCriticalSection(&allocspy_cs);
    }

    TRACE("%p.\n",addr);
    return addr;
}

static void * WINAPI allocator_Realloc(IMalloc *iface, void *pv, SIZE_T cb)
{
    void *addr;

    TRACE("%p, %Id.\n", pv, cb);

    if (allocator.spy)
    {
        void *real_mem;
        BOOL spyed;

        EnterCriticalSection(&allocspy_cs);
        spyed = mallocspy_remove_spyed_memory(pv);
        cb = IMallocSpy_PreRealloc(allocator.spy, pv, cb, &real_mem, spyed);

        /* check if can release the spy */
        if (allocator.spy_release_pending && !allocator.spyed_allocations)
        {
            IMallocSpy_Release(allocator.spy);
            allocator.spy_release_pending = FALSE;
            allocator.spy = NULL;
            LeaveCriticalSection(&allocspy_cs);
        }

        if (!cb)
        {
            /* PreRealloc can force Realloc to fail */
            if (allocator.spy)
                LeaveCriticalSection(&allocspy_cs);
            return NULL;
        }

        pv = real_mem;
    }

    if (!pv) addr = HeapAlloc(GetProcessHeap(), 0, cb);
    else if (cb) addr = HeapReAlloc(GetProcessHeap(), 0, pv, cb);
    else
    {
        HeapFree(GetProcessHeap(), 0, pv);
        addr = NULL;
    }

    if (allocator.spy)
    {
        addr = IMallocSpy_PostRealloc(allocator.spy, addr, TRUE);
        mallocspy_add_mem(addr);
        LeaveCriticalSection(&allocspy_cs);
    }

    TRACE("%p.\n", addr);
    return addr;
}

static void WINAPI allocator_Free(IMalloc *iface, void *mem)
{
    BOOL spyed_block = FALSE, spy_active = FALSE;

    TRACE("%p.\n", mem);

    if (!mem)
        return;

    if (allocator.spy)
    {
        EnterCriticalSection(&allocspy_cs);
        spyed_block = mallocspy_remove_spyed_memory(mem);
        spy_active = TRUE;
        mem = IMallocSpy_PreFree(allocator.spy, mem, spyed_block);
    }

    HeapFree(GetProcessHeap(), 0, mem);

    if (spy_active)
    {
        IMallocSpy_PostFree(allocator.spy, spyed_block);

        /* check if can release the spy */
        if (allocator.spy_release_pending && !allocator.spyed_allocations)
        {
            IMallocSpy_Release(allocator.spy);
            allocator.spy_release_pending = FALSE;
            allocator.spy = NULL;
        }

        LeaveCriticalSection(&allocspy_cs);
    }
}

/******************************************************************************
 * NOTES
 *  FIXME returns:
 *      win95:  size allocated (4 byte boundaries)
 *      win2k:  size originally requested !!! (allocated on 8 byte boundaries)
 */
static SIZE_T WINAPI allocator_GetSize(IMalloc *iface, void *mem)
{
    BOOL spyed_block = FALSE, spy_active = FALSE;
    SIZE_T size;

    TRACE("%p.\n", mem);

    if (!mem)
        return (SIZE_T)-1;

    if (allocator.spy)
    {
        EnterCriticalSection(&allocspy_cs);
        spyed_block = !!mallocspy_is_allocation_spyed(mem);
        spy_active = TRUE;
        mem = IMallocSpy_PreGetSize(allocator.spy, mem, spyed_block);
    }

    size = HeapSize(GetProcessHeap(), 0, mem);

    if (spy_active)
    {
        size = IMallocSpy_PostGetSize(allocator.spy, size, spyed_block);
        LeaveCriticalSection(&allocspy_cs);
    }

    return size;
}

static INT WINAPI allocator_DidAlloc(IMalloc *iface, void *mem)
{
    BOOL spyed_block = FALSE, spy_active = FALSE;
    int did_alloc;

    TRACE("%p.\n", mem);

    if (!mem)
        return -1;

    if (allocator.spy)
    {
        EnterCriticalSection(&allocspy_cs);
        spyed_block = !!mallocspy_is_allocation_spyed(mem);
        spy_active = TRUE;
        mem = IMallocSpy_PreDidAlloc(allocator.spy, mem, spyed_block);
    }

    did_alloc = HeapValidate(GetProcessHeap(), 0, mem);

    if (spy_active)
    {
        did_alloc = IMallocSpy_PostDidAlloc(allocator.spy, mem, spyed_block, did_alloc);
        LeaveCriticalSection(&allocspy_cs);
    }

    return did_alloc;
}

static void WINAPI allocator_HeapMinimize(IMalloc *iface)
{
    BOOL spy_active = FALSE;

    TRACE("\n");

    if (allocator.spy)
    {
        EnterCriticalSection(&allocspy_cs);
        spy_active = TRUE;
        IMallocSpy_PreHeapMinimize(allocator.spy);
    }

    if (spy_active)
    {
        IMallocSpy_PostHeapMinimize(allocator.spy);
        LeaveCriticalSection(&allocspy_cs);
    }
}

static const IMallocVtbl allocator_vtbl =
{
    allocator_QueryInterface,
    allocator_AddRef,
    allocator_Release,
    allocator_Alloc,
    allocator_Realloc,
    allocator_Free,
    allocator_GetSize,
    allocator_DidAlloc,
    allocator_HeapMinimize
};

/******************************************************************************
 *                CoGetMalloc        (combase.@)
 */
HRESULT WINAPI CoGetMalloc(DWORD context, IMalloc **imalloc)
{
    if (context != MEMCTX_TASK)
    {
        *imalloc = NULL;
        return E_INVALIDARG;
    }

    *imalloc = &allocator.IMalloc_iface;

    return S_OK;
}

/***********************************************************************
 *           CoTaskMemAlloc         (combase.@)
 */
void * WINAPI CoTaskMemAlloc(SIZE_T size)
{
    return IMalloc_Alloc(&allocator.IMalloc_iface, size);
}

/***********************************************************************
 *           CoTaskMemFree          (combase.@)
 */
void WINAPI CoTaskMemFree(void *ptr)
{
    IMalloc_Free(&allocator.IMalloc_iface, ptr);
}

/***********************************************************************
 *           CoTaskMemRealloc        (combase.@)
 */
void * WINAPI CoTaskMemRealloc(void *ptr, SIZE_T size)
{
    return IMalloc_Realloc(&allocator.IMalloc_iface, ptr, size);
}

/***********************************************************************
 *           CoRegisterMallocSpy        (combase.@)
 */
HRESULT WINAPI CoRegisterMallocSpy(IMallocSpy *spy)
{
    HRESULT hr = E_INVALIDARG;

    TRACE("%p.\n", spy);

    if (!spy) return E_INVALIDARG;

    EnterCriticalSection(&allocspy_cs);

    if (allocator.spy)
        hr = CO_E_OBJISREG;
    else if (SUCCEEDED(IMallocSpy_QueryInterface(spy, &IID_IMallocSpy, (void **)&spy)))
    {
        allocator.spy = spy;
        hr = S_OK;
    }

    LeaveCriticalSection(&allocspy_cs);

    return hr;
}

/***********************************************************************
 *           CoRevokeMallocSpy (combase.@)
 */
HRESULT WINAPI CoRevokeMallocSpy(void)
{
    HRESULT hr = S_OK;

    TRACE("\n");

    EnterCriticalSection(&allocspy_cs);

    if (!allocator.spy)
        hr = CO_E_OBJNOTREG;
    else if (allocator.spyed_allocations)
    {
        allocator.spy_release_pending = TRUE;
        hr = E_ACCESSDENIED;
    }
    else
    {
        IMallocSpy_Release(allocator.spy);
        allocator.spy = NULL;
    }

    LeaveCriticalSection(&allocspy_cs);

    return hr;
}
