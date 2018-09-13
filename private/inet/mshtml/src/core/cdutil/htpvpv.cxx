//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       htpvpv.cxx
//
//  Contents:   Hash table mapping PVOID to PVOID
//
//              CHtPvPv
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTPVPV_HXX_
#define X_HTPVPV_HXX_
#include "htpvpv.hxx"
#endif

DeclareTag(tagHtPvPvGrow, "Utils", "Trace CHtPvPv Grow/Shrink")
MtDefine(CHtPvPv, Elements, "CHtPvPv")
MtDefine(CHtPvPv_pEnt, CHtPvPv, "CHtPvPv::_pEnt")

// Definitions ----------------------------------------------------------------

#define HtKeyEqual(pvKey1, pvKey2)  (((void *)((DWORD_PTR)pvKey1 & ~1L)) == (pvKey2))
#define HtKeyInUse(pvKey)           ((DWORD_PTR)pvKey > 1L)
#define HtKeyTstFree(pvKey)         (pvKey == NULL)
#define HtKeyTstBridged(pvKey)      ((DWORD_PTR)pvKey & 1L)
#define HtKeySetBridged(pvKey)      ((void *)((DWORD_PTR)pvKey | 1L))
#define HtKeyClrBridged(pvKey)      ((void *)((DWORD_PTR)pvKey & ~1L))
#define HtKeyTstRehash(pvKey)       ((DWORD_PTR)pvKey & 2L)
#define HtKeySetRehash(pvKey)       ((void *)((DWORD_PTR)pvKey | 2L))
#define HtKeyClrRehash(pvKey)       ((void *)((DWORD_PTR)pvKey & ~2L))
#define HtKeyTstFlags(pvKey)        ((DWORD_PTR)pvKey & 3L)
#define HtKeyClrFlags(pvKey)        ((void *)((DWORD_PTR)pvKey & ~3L))

// Constructor / Destructor ---------------------------------------------------

CHtPvPv::CHtPvPv()
{
    _pEnt        = &_EntEmpty;
    _pEntLast    = &_EntEmpty;
    _cEntMax     = 1;
    _cStrideMask = 1;
}

CHtPvPv::~CHtPvPv()
{
    if (_pEnt != &_EntEmpty)
    {
        MemFree(_pEnt);
    }
}

// Utilities ------------------------------------------------------------------

UINT
CHtPvPv::ComputeStrideMask(UINT cEntMax)
{
    UINT iMask;
    for (iMask = 1; iMask < cEntMax; iMask <<= 1);
    return((iMask >> 1) - 1);
}

// Private Methods ------------------------------------------------------------

HRESULT
CHtPvPv::Grow()
{
    HRESULT hr;
    DWORD * pdw;
    UINT    cEntMax;
    UINT    cEntGrow;
    UINT    cEntShrink;
    HTENT * pEnt;

    extern DWORD s_asizeAssoc[];

    for (pdw = s_asizeAssoc; *pdw <= _cEntMax; pdw++) ;

    cEntMax    = *pdw;
    cEntGrow   = cEntMax * 8L / 10L;
    cEntShrink = (pdw > s_asizeAssoc) ? *(pdw - 1) * 4L / 10L : 0;
    pEnt       = (_pEnt == &_EntEmpty) ? NULL : _pEnt;

    hr = MemRealloc(Mt(CHtPvPv_pEnt), (void **)&pEnt, cEntMax * sizeof(HTENT));

    if (hr == S_OK)
    {
        _pEnt           = pEnt;
        _pEntLast       = &_EntEmpty;
        _cEntGrow       = cEntGrow;
        _cEntShrink     = cEntShrink;
        _cStrideMask    = ComputeStrideMask(cEntMax);

        memset(&_pEnt[_cEntMax], 0, (cEntMax - _cEntMax) * sizeof(HTENT));

        if (_cEntMax == 1)
        {
            memset(_pEnt, 0, sizeof(HTENT));
        }

        Rehash(cEntMax);
    }

    TraceTag((tagHtPvPvGrow, "Growing to cEntMax=%ld (cEntShrink=%ld,cEnt=%ld,cEntGrow=%ld)",
        _cEntMax, _cEntShrink, _cEnt, _cEntGrow));

    RRETURN(hr);
}

void
CHtPvPv::Shrink()
{
    DWORD * pdw;
    UINT    cEntMax;
    UINT    cEntGrow;
    UINT    cEntShrink;

    extern DWORD s_asizeAssoc[];

    for (pdw = s_asizeAssoc; *pdw < _cEntMax; pdw++) ;

    cEntMax    = *--pdw;
    cEntGrow   = cEntMax * 8L / 10L;
    cEntShrink = (pdw > s_asizeAssoc) ? *(pdw - 1) * 4L / 10L : 0;

    Assert(_cEnt < cEntGrow);
    Assert(_cEnt > cEntShrink);

    _pEntLast       = &_EntEmpty;
    _cEntGrow       = cEntGrow;
    _cEntShrink     = cEntShrink;
    _cStrideMask    = ComputeStrideMask(cEntMax);

    Rehash(cEntMax);

    Verify(MemRealloc(Mt(CHtPvPv_pEnt), (void **)&_pEnt, cEntMax * sizeof(HTENT)) == S_OK);

    TraceTag((tagHtPvPvGrow, "Shrinking to cEntMax=%ld (cEntShrink=%ld,cEnt=%ld,cEntGrow=%ld)",
        _cEntMax, _cEntShrink, _cEnt, _cEntGrow));
}

void
CHtPvPv::Rehash(UINT cEntMax)
{
    UINT    iEntScan    = 0;
    UINT    cEntScan    = _cEntMax;
    HTENT * pEntScan    = _pEnt;
    UINT    iEnt;
    UINT    cEnt;
    HTENT * pEnt;

    _cEntDel = 0;
    _cEntMax = cEntMax;

    for (; iEntScan < cEntScan; ++iEntScan, ++pEntScan)
    {
        if (HtKeyInUse(pEntScan->pvKey))
            pEntScan->pvKey = HtKeyClrBridged(HtKeySetRehash(pEntScan->pvKey));
        else
            pEntScan->pvKey = NULL;
        Assert(!HtKeyTstBridged(pEntScan->pvKey));
    }

    iEntScan = 0;
    pEntScan = _pEnt;

    for (; iEntScan < cEntScan; ++iEntScan, ++pEntScan)
    {

    repeat:

        if (HtKeyTstRehash(pEntScan->pvKey))
        {
            pEntScan->pvKey = HtKeyClrRehash(pEntScan->pvKey);

            iEnt = ComputeProbe(pEntScan->pvKey);
            cEnt = ComputeStride(pEntScan->pvKey);

            for (;;)
            {
                pEnt = &_pEnt[iEnt];

                if (pEnt == pEntScan)
                    break;

                if (pEnt->pvKey == NULL)
                {
                    *pEnt = *pEntScan;
                    pEntScan->pvKey = NULL;
                    break;
                }

                if (HtKeyTstRehash(pEnt->pvKey))
                {
                    void * pvKey1 = HtKeyClrBridged(pEnt->pvKey);
                    void * pvKey2 = HtKeyClrBridged(pEntScan->pvKey);
                    void * pvVal1 = pEnt->pvVal;
                    void * pvVal2 = pEntScan->pvVal;

                    if (HtKeyTstBridged(pEntScan->pvKey))
                    {
                        pvKey1 = HtKeySetBridged(pvKey1);
                    }

                    if (HtKeyTstBridged(pEnt->pvKey))
                    {
                        pvKey2 = HtKeySetBridged(pvKey2);
                    }

                    pEntScan->pvKey = pvKey1;
                    pEntScan->pvVal = pvVal1;
                    pEnt->pvKey = pvKey2;
                    pEnt->pvVal = pvVal2;

                    goto repeat;
                }
            
                pEnt->pvKey = HtKeySetBridged(pEnt->pvKey);

                iEnt += cEnt;

                if (iEnt >= _cEntMax)
                    iEnt -= _cEntMax;
            }
        }
    }
}

// Public Methods -------------------------------------------------------------

void *
CHtPvPv::Lookup(void * pvKey)
{
    HTENT * pEnt;
    UINT        iEnt;
    UINT        cEnt;

    Assert(!HtKeyTstFree(pvKey) && !HtKeyTstFlags(pvKey));

    if (HtKeyEqual(_pEntLast->pvKey, pvKey))
    {
        return(_pEntLast->pvVal);
    }

    iEnt = ComputeProbe(pvKey);
    pEnt = &_pEnt[iEnt];

    if (HtKeyEqual(pEnt->pvKey, pvKey))
    {
        _pEntLast = pEnt;
        return(pEnt->pvVal);
    }

    if (!HtKeyTstBridged(pEnt->pvKey))
    {
        return(NULL);
    }

    cEnt = ComputeStride(pvKey);

    for (;;)
    {
        iEnt += cEnt;

        if (iEnt >= _cEntMax)
            iEnt -= _cEntMax;

        pEnt = &_pEnt[iEnt];

        if (HtKeyEqual(pEnt->pvKey, pvKey))
        {
            _pEntLast = pEnt;
            return(pEnt->pvVal);
        }

        if (!HtKeyTstBridged(pEnt->pvKey))
        {
            return(NULL);
        }
    }
}

#if DBG==1
BOOL
CHtPvPv::IsPresent(void * pvKey)
{
    HTENT * pEnt;
    UINT        iEnt;
    UINT        cEnt;

    Assert(!HtKeyTstFree(pvKey) && !HtKeyTstFlags(pvKey));

    iEnt = ComputeProbe(pvKey);
    pEnt = &_pEnt[iEnt];

    if (HtKeyEqual(pEnt->pvKey, pvKey))
        return TRUE;

    if (!HtKeyTstBridged(pEnt->pvKey))
        return FALSE;

    cEnt = ComputeStride(pvKey);

    for (;;)
    {
        iEnt += cEnt;

        if (iEnt >= _cEntMax)
            iEnt -= _cEntMax;

        pEnt = &_pEnt[iEnt];

        if (HtKeyEqual(pEnt->pvKey, pvKey))
            return TRUE;

        if (!HtKeyTstBridged(pEnt->pvKey))
            return FALSE;
    }
}
#endif

HRESULT
CHtPvPv::Insert(void * pvKey, void * pvVal)
{
    HTENT * pEnt;
    UINT    iEnt;
    UINT    cEnt;

    Assert(!HtKeyTstFree(pvKey) && !HtKeyTstFlags(pvKey));
    Assert(!IsPresent(pvKey));

    if (_cEnt + _cEntDel >= _cEntGrow)
    {
        if (_cEntDel > (_cEnt >> 2))
            Rehash(_cEntMax);
        else
        {
            HRESULT hr = Grow();

            if (hr)
            {
                RRETURN(hr);
            }
        }
    }

    iEnt = ComputeProbe(pvKey);
    pEnt = &_pEnt[iEnt];

    if (!HtKeyInUse(pEnt->pvKey))
    {
        goto insert;
    }

    pEnt->pvKey = HtKeySetBridged(pEnt->pvKey);

    cEnt = ComputeStride(pvKey);

    for (;;)
    {
        iEnt += cEnt;

        if (iEnt >= _cEntMax)
            iEnt -= _cEntMax;

        pEnt = &_pEnt[iEnt];

        if (!HtKeyInUse(pEnt->pvKey))
        {
            goto insert;
        }

        pEnt->pvKey = HtKeySetBridged(pEnt->pvKey);
    }

insert:

    if (HtKeyTstBridged(pEnt->pvKey))
    {
        _cEntDel -= 1;
        pEnt->pvKey = HtKeySetBridged(pvKey);
    }
    else
    {
        pEnt->pvKey = pvKey;
    }

    pEnt->pvVal = pvVal;

    _pEntLast = pEnt;

    _cEnt += 1;

    return(S_OK);
}

void *
CHtPvPv::Remove(void * pvKey)
{
    HTENT * pEnt;
    UINT    iEnt;
    UINT    cEnt;

    Assert(!HtKeyTstFree(pvKey) && !HtKeyTstFlags(pvKey));

    iEnt = ComputeProbe(pvKey);
    pEnt = &_pEnt[iEnt];

    if (HtKeyEqual(pEnt->pvKey, pvKey))
    {
        goto remove;
    }

    if (!HtKeyTstBridged(pEnt->pvKey))
    {
        return(NULL);
    }

    cEnt = ComputeStride(pvKey);

    for (;;)
    {
        iEnt += cEnt;

        if (iEnt >= _cEntMax)
            iEnt -= _cEntMax;

        pEnt = &_pEnt[iEnt];

        if (HtKeyEqual(pEnt->pvKey, pvKey))
        {
            goto remove;
        }

        if (!HtKeyTstBridged(pEnt->pvKey))
        {
            return(NULL);
        }
    }

remove:

    if (HtKeyTstBridged(pEnt->pvKey))
    {
        pEnt->pvKey = HtKeySetBridged(NULL);
        _cEntDel += 1;
    }
    else
    {
        pEnt->pvKey = NULL;
    }

    pvKey = pEnt->pvVal;

    _cEnt -= 1;

    if (_cEnt < _cEntShrink)
    {
        Shrink();
    }

    return(pvKey);
}

// Testing --------------------------------------------------------------------

#if 0

#define MAKE_HTKEY(i)   ((void *)((((DWORD)(i) * 4567) << 2) | 4))
#define MAKE_HTVAL(k)   ((void *)(~(DWORD)MAKE_HTKEY(k)))

CHtPvPv * phtable = NULL;

BOOL TestHTInsert(int i)
{
    void * pvKey = MAKE_HTKEY(i);
    void * pvVal = MAKE_HTVAL(i);
    Verify(phtable->Insert(pvKey, pvVal) == S_OK);
    Verify(phtable->Lookup(pvKey) == pvVal);
    return(TRUE);
}

BOOL TestHTRemove(int i)
{
    void * pvKey = MAKE_HTKEY(i);
    void * pvVal = MAKE_HTVAL(i);
    Verify(phtable->Remove(pvKey) == pvVal);
    Verify(phtable->Remove(pvKey) == NULL);
    return(TRUE);
}

BOOL TestHTVerify(int i, int n)
{
    void * pvKey;
    void * pvVal; 
    int    j;

    Verify((int)phtable->GetCount() == (n - i));

    for (j = i; j < n; ++j)
    {
        pvKey = MAKE_HTKEY(j);
        pvVal = MAKE_HTVAL(j);
        Verify(phtable->Lookup(pvKey) == pvVal);
    }

    return(TRUE);
}

HRESULT TestHtPvPv()
{
    CHtPvPv *   pht = new CHtPvPv;
    int         cLim, cEntMax;
    int         i, j;

    // Insufficient memory, don't crash.
    if (!pht)
        return S_FALSE;

//  printf("---- Begin testing CHashTable implementation\n\n");

    cLim    = 256;
    cEntMax = 383;
    phtable = pht;

    // Insert elements from 0 to cLim
//  printf("  Inserting from %3d to %3d\n", 0, cLim); fflush(stdout);
    for (i = 0; i < cLim; ++i) {
        if (!TestHTInsert(i)) return(S_FALSE);
        if (!TestHTVerify(0, i + 1)) return(S_FALSE);
    }

    // Remove elements from 0 to cLim
//  printf("  Removing  from %3d to %3d\n", 0, cLim); fflush(stdout);
    for (i = 0; i < cLim; ++i) {
        if (!TestHTRemove(i)) return(S_FALSE);
        if (!TestHTVerify(i + 1, cLim)) return(S_FALSE);
    }

    // Rehash and make sure number of deleted entries is now zero
    phtable->Rehash(phtable->GetMaxCount());
    Verify(phtable->GetDelCount() == 0);
    if (!TestHTVerify(0, 0)) return(S_FALSE);

    // Insert elements from 0 to cLim/2
//  printf("  Inserting from %3d to %3d\n", 0, cLim / 2); fflush(stdout);
    for (i = 0; i < cLim/2; ++i) {
        if (!TestHTInsert(i)) return(S_FALSE);
        if (!TestHTVerify(0, i + 1)) return(S_FALSE);
    }

    // Insert elements from cLim/2 to cLim and remove elements from 0 to cLim/2
//  printf("  Inserting from %3d to %3d, removing from %3d to %3d\n", cLim / 2, cLim, 0, cLim / 2); fflush(stdout);
    for (i = cLim/2, j = 0; i < cLim; ++i, ++j) {
        if (!TestHTInsert(i)) return(S_FALSE);
        if (!TestHTVerify(j, i + 1)) return(S_FALSE);
        if (!TestHTRemove(j)) return(S_FALSE);
        if (!TestHTVerify(j + 1, i + 1)) return(S_FALSE);
    }

    // Insert two elements from cLim to cLim*2, remove one element from cLim/2 to cLim
//  printf("  Inserting from %3d to %3d, removing from %3d to %3d\n", cLim, cLim*2, cLim / 2, cLim);
    for (i = cLim, j = cLim / 2; i < cLim*2; i += 2, ++j) {
        if (!TestHTRemove(j)) return(S_FALSE);
        if (!TestHTVerify(j + 1, i)) return(S_FALSE);
        if (!TestHTInsert(i)) return(S_FALSE);
        if (!TestHTVerify(j + 1, i + 1)) return(S_FALSE);
        if (!TestHTInsert(i + 1)) return(S_FALSE);
        if (!TestHTVerify(j + 1, i + 2)) return(S_FALSE);
    }

    // Rehash and make sure number of deleted entries is now zero
    phtable->Rehash(phtable->GetMaxCount());
    Verify(phtable->GetDelCount() == 0);

    if (!TestHTVerify(cLim, cLim*2)) return(S_FALSE);

    // Insert elements from cLim*2, remove two elements from cLim
//  printf("  Inserting from %3d to %3d, removing from %3d to %3d\n", cLim*2, cLim*3, cLim, cLim*3);
    for (i = cLim*2, j = cLim; i < cLim*3; ++i, j += 2) {
        if (!TestHTInsert(i)) return(S_FALSE);
        if (!TestHTVerify(j, i + 1)) return(S_FALSE);
        if (!TestHTRemove(j)) return(S_FALSE);
        if (!TestHTVerify(j + 1, i + 1)) return(S_FALSE);
        if (!TestHTRemove(j + 1)) return(S_FALSE);
        if (!TestHTVerify(j + 2, i + 1)) return(S_FALSE);
    }

    // Rehash and make sure number of deleted entries is now zero
    phtable->Rehash(phtable->GetMaxCount());
    Verify(phtable->GetDelCount() == 0);
    if (!TestHTVerify(0, 0)) return(S_FALSE);

    delete pht;

    return(S_OK);
}

#endif
