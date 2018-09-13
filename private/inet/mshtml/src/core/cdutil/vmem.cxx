//+------------------------------------------------------------------------
//
//  File:       vmem.cxx
//
//  Contents:   Strict memory allocation utilities
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_VMEM_HXX_
#define X_VMEM_HXX_
#include "vmem.hxx"
#endif

//
// VMem allocates memory using the operating system's low-level virtual allocator. It arranges for an
// allocation to start at the very beginning of a page, with a non-accessable page just before it,
// or for it to end at the very end of a page, with a non-accessible page just after it.  The idea is
// to catch memory overwrites quickly.
//
// The layout of an allocation is as follows:
//
//  +--- page VMEMINFO structure
//  |
//  |                 +--- pv if using front-side-strict memory allocations
//  |                 |                   
//  |                 |                   +--- filled with pattern to detect back-side overwrite
//  v                 v                   v
// +--------+--------+--------+--------+--------+--------+
// |VMEMINFO|   NO   |[User's memory area]XXXXXX|   NO   |
// |        | ACCESS |XXXXXX[User's memory area]| ACCESS |
// +--------+--------+--------+--------+--------+--------+
//                    ^     ^
//                    |     +--- pv if using back-side-strict memory allocations
//                    |
//                    +--- filled with pattern to detect front-side overwrite
//
//

#if defined(_ALPHA_) || defined(SPARC)
#define PAGE_SIZE       8192
#else
#define PAGE_SIZE       4096
#endif

DWORD
VMemQueryProtect(void * pv, DWORD cb)
{
    MEMORY_BASIC_INFORMATION mbi = { 0 };
    VirtualQuery(pv, &mbi, sizeof(mbi));
    return (mbi.Protect ? mbi.Protect : mbi.AllocationProtect);
}

VMEMINFO *
VMemIsValid(void * pv)
{
    VMEMINFO * pvmi;
    BYTE * pb;
    UINT cb;

    if (pv == NULL)
    {
        return NULL;
    }

    pvmi = (VMEMINFO *)(((DWORD_PTR)pv & ~(PAGE_SIZE - 1)) - PAGE_SIZE * 2);

    if (VMemQueryProtect(pvmi, PAGE_SIZE) != PAGE_READONLY)
    {
        AssertSz(FALSE, "VMemIsValid - VMEMINFO page is not marked READONLY");
        return NULL;
    }

    if (pv != pvmi->pv)
    {
        AssertSz(FALSE, "VMemIsValid - VMEMINFO doesn't point back to pv");
        return NULL;
    }

    if (VMemQueryProtect((BYTE *)pvmi + PAGE_SIZE, PAGE_SIZE) != PAGE_NOACCESS)
    {
        AssertSz(FALSE, "VMemIsValid - can't detect first no-access page");
        return NULL;
    }

    if (VMemQueryProtect((BYTE *)pvmi + PAGE_SIZE * 2, pvmi->cbFill1 + pvmi->cb + pvmi->cbFill2) != PAGE_READWRITE)
    {
        AssertSz(FALSE, "VMemIsValid - user memory block is not all writable");
        return NULL;
    }

    if (pvmi->cbFill1)
    {
        pb = (BYTE *)pvmi + PAGE_SIZE * 2;
        cb = pvmi->cbFill1;

        for (; cb > 0; --cb, ++pb)
        {
            if (*pb != 0x1A)
            {
                AssertSz(FALSE, "VMemIsValid - detected user memory pre-data overwrite");
                return NULL;
            }
        }
    }

    if (pvmi->cbFill2)
    {
        pb = (BYTE *)pvmi + PAGE_SIZE * 2 + pvmi->cbFill1 + pvmi->cb;
        cb = pvmi->cbFill2;

        for (; cb > 0; --cb, ++pb)
        {
            if (*pb != 0x3A)
            {
                AssertSz(FALSE, "VMemIsValid - detected user memory post-data overwrite");
                return NULL;
            }
        }
    }

    if (VMemQueryProtect((BYTE *)pvmi + PAGE_SIZE * 2 + pvmi->cbFill1 + pvmi->cb + pvmi->cbFill2, PAGE_SIZE) != PAGE_NOACCESS)
    {
        AssertSz(FALSE, "VMemIsValid - can't detect second no-access page");
        return NULL;
    }

    return(pvmi);
}

void *
VMemAlloc(size_t cb, DWORD dwFlags, void * pvUser)
{
    void * pv1, * pv2, * pv3;
    size_t cbUser, cbPage;
    DWORD dwOldProtect;
    VMEMINFO * pvmi;

    if (cb == 0)
    {
        cb = 1;
    }

    if (    (dwFlags & VMEM_BACKSIDESTRICT)
        &&  (dwFlags & VMEM_BACKSIDEALIGN8))
        cbUser = (cb + 7) & ~7;
    else
        cbUser = cb;

    cbPage = (cbUser + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	pv1 = VirtualAlloc(0, cbPage + PAGE_SIZE * 3, MEM_RESERVE, PAGE_NOACCESS);

    if (pv1 == NULL)
    {
        return(NULL);
    }

	pv2 = VirtualAlloc(pv1, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);

    if (pv2 == NULL)
    {
        return(NULL);
    }

    pvmi          = (VMEMINFO *)pv2;
    pvmi->cb      = cb;
    pvmi->dwFlags = dwFlags;
    pvmi->pvUser  = pvUser;

    pv3 = VirtualAlloc((BYTE *)pv1 + PAGE_SIZE * 2, cbPage, MEM_COMMIT, PAGE_READWRITE);

    if (pv3 == NULL)
    {
        return(NULL);
    }

    if (dwFlags & VMEM_BACKSIDESTRICT)
    {
        pvmi->cbFill1 = cbPage - cbUser;
        pvmi->cbFill2 = cbUser - cb;
    }
    else
    {
        pvmi->cbFill1 = 0;
        pvmi->cbFill2 = cbPage - cbUser;
    }

    Assert(pvmi->cbFill1 + cb + pvmi->cbFill2 == cbPage);

    if (pvmi->cbFill1)
    {
        memset((BYTE *)pv3, 0x1A, pvmi->cbFill1);
    }

    memset((BYTE *)pv3 + pvmi->cbFill1, 0x2A, cb);

    if (pvmi->cbFill2)
    {
        memset((BYTE *)pv3 + pvmi->cbFill1 + cb, 0x3A, pvmi->cbFill2);
    }

    pvmi->pv = (BYTE *)pv3 + pvmi->cbFill1;

    VirtualProtect(pv1, PAGE_SIZE, PAGE_READONLY, &dwOldProtect);

    Assert(VMemIsValid(pvmi->pv));

    return(pvmi->pv);
}

void *
VMemAllocClear(size_t cb, DWORD dwFlags, void * pvUser)
{
    void * pv = VMemAlloc(cb, dwFlags, pvUser);

    if (pv)
    {
        memset(pv, 0, cb);
    }

    return(pv);
}

HRESULT
VMemRealloc(void ** ppv, size_t cb, DWORD dwFlags, void * pvUser)
{
    if (cb == 0)
    {
        VMemFree(*ppv);
        return S_OK;
    }
    else if (*ppv == NULL)
    {
        *ppv = VMemAlloc(cb, dwFlags, pvUser);
        return(*ppv ? S_OK : E_OUTOFMEMORY);
    }
    else
    {
        void *     pvOld    = *ppv;
        VMEMINFO * pvmiOld  = VMemIsValid(pvOld);
        void *     pvNew;

        if (pvmiOld == NULL)
        {
            return E_OUTOFMEMORY;
        }

        pvNew = VMemAlloc(cb, dwFlags, pvUser);

        if (pvNew == NULL)
        {
            return E_OUTOFMEMORY;
        }

        memcpy(pvNew, pvOld, min(cb, pvmiOld->cb));

        VMemFree(pvOld);

        *ppv = pvNew;
        return S_OK;
    }
}

void
VMemFree(void * pv)
{
    VMEMINFO * pvmi = VMemIsValid(pv);

    if (pvmi)
    {
        Verify(VirtualFree(pvmi, 0, MEM_RELEASE));
    }
}

size_t
VMemGetSize(void * pv)
{
    VMEMINFO * pvmi = VMemIsValid(pv);
    return(pvmi ? pvmi->cb : 0);
}
