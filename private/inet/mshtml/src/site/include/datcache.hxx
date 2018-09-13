//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       datcache.hxx
//
//  Contents:   CObjCache - Object cache class
//
//----------------------------------------------------------------------------

#ifndef I_DATCACHE_HXX_
#define I_DATCACHE_HXX_
#pragma INCMSG("--- Beg 'datcache.hxx'")

// the WATCOM compiler complains about operations
// on undefined DATA type, delete, clone etc.
#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"
#endif

// ========================  CDataCacheElem  =================================

// Each element of the data cache is described by the following:

struct CDataCacheElem
{
    // Pinter to the data, NULL if this element is free

    void *  _pvData;

    // Index of the next free element for a free element
    // ref count, Crc and optional Id for a used element

    union
    {
        LONG _ielNextFree;
        struct
        {
            // BUGBUG? davidd: potential ENDIAN problems. For now ignored as
            // Free&Used elements are mutually exclusive I suppose...
            WORD    _wPad;
            WORD    _wCrc;
        } ielRef;
    };
    DWORD _cRef;
};

#if _MSC_VER >= 1100
template <class Data> class CDataCache;       // forward ref
#endif


// ========================  CCacheCookie  =================================

// User reference to any cacheable item.
// This is not an integrated solution, meaning the correct way to do this
// is to modify the cache format, but prevents need for seperate
// bit which tells whether an item is the pointer, a cookie, or NULL.

template <class DATA>
class CCacheCookie
{
private:
    DWORD _dwVal;

public:

    BOOL  IsEmpty() const            {return (0==_dwVal);}
    void  Clear()                    {_dwVal = 0;}

    BOOL  IsRealCookie() const       {return ((_dwVal&0x3)==0x2);}
    LONG  GetRealCookie() const      {Assert(IsRealCookie());return (LONG) (_dwVal>>2);}
    void  SetRealCookie(LONG iCookie){Assert(!(iCookie&0xC0000000));(_dwVal=(DWORD) (iCookie<<2)|0x2);}

    BOOL  IsPointer() const          {return (!IsRealCookie());}
    DATA* GetPointer() const         {Assert(IsPointer());return (DATA*) (void*) _dwVal;}
    void  SetPointer(DATA *pv)       {Assert(pv);_dwVal = (LONG) (void*) pv;}

    void SetRawValue(DWORD dwVal)    {_dwVal = (DWORD) dwVal;}
    DWORD GetRawValue() const        {return _dwVal;}

    CCacheCookie()                   {_dwVal = 0;}

    HRESULT ReleaseData(CDataCache<DATA> *pDC)
        {
            HRESULT hr = S_OK;
            Assert(pDC);
            if (IsRealCookie())
            {
                hr = THR(pDC->ReleaseData(GetRealCookie()));
            }
            else if (!IsEmpty())
            {
                delete GetPointer();
            }
            Clear();
            RRETURN(hr);
        }

    HRESULT AddRefData(CDataCache<DATA> *pDC)
    {
        if (IsRealCookie())
        {
            RRETURN(THR(pDC->AddRefData(GetRealCookie())));
        }
        else if (!IsEmpty())
        {
            CacheData(pDC);
        }
        return S_OK;
    }

    const DATA* GetRO(CDataCache<DATA> *pDC) const
        {
            static DATA dataNull;

            if (IsRealCookie())
            {
                return &(*pDC)[GetRealCookie()];
            }
            else if (IsEmpty())
            {
                return &dataNull;
            }
            else
            {
                return GetPointer();
            }
        }

    HRESULT CacheData(CDataCache<DATA> *pDC)
    {
        LONG iCookie;
        HRESULT hr;

        Assert(!IsEmpty()&&IsPointer());
        void *pv = (void*) GetPointer();
        hr = THR( pDC->CacheDataPointer((void**)&pv, &iCookie) );
        if (!hr)
        {
            SetRealCookie(iCookie);
        }
        RRETURN(hr);
    }
};


// ===========================  CDataCache  ==================================

// This array class ensures stability of the indexes. Elements are freed by
// inserting them in a free list, and the array is never shrunk.
// The first UINT of DATA is used to store the index of next element in the
// free list.

class CDataCacheBase
{
private:
    CDataCacheElem*  _pael;         // array of elements
    LONG             _cel;          // total count of elements (including free ones)
    LONG             _ielFirstFree; // first free element | FLBIT

#if DBG == 1
public:
    LONG             _celsInCache;  // total count of elements (excluding free ones)
    LONG             _cMaxEls;      // maximum no. of cells in the cache                
#endif

    HRESULT CacheData(const void *pvData, LONG *piel, BOOL *pfDelete, BOOL fClone);
public:
    CDataCacheBase();
    virtual ~CDataCacheBase() {}

    //Cache this data w/o cloning.  Takes care of further memory management of data
    HRESULT CacheDataPointer(void **ppvData, LONG *piel);
    HRESULT CacheData(const void *pvData, LONG *piel) {return CacheData(pvData, piel, NULL, TRUE);}
    void AddRefData(LONG iel);
    void ReleaseData(LONG iel);

    long Size() const
    {
        return _cel;
    }

protected:
    CDataCacheElem*   Elem(LONG iel) const
    {
        Assert(iel >= 0 && iel < _cel);
        return (_pael + iel);
    }

    HRESULT Add(const void * pvData, LONG *piel, BOOL fClone);
    void    Free(LONG ielFirst);
    void    Free();
    LONG    Find(const void *pvData) const;

    virtual void    DeletePtr(void *pvData) = 0;
    virtual HRESULT InitData(CDataCacheElem *pel, const void *pvData, BOOL fClone) = 0;
    virtual void    PassivateData(CDataCacheElem *pel) = 0;
    virtual WORD    ComputeDataCrc(const void *pvData) const = 0;
    virtual BOOL    CompareData(const void *pvData1, const void *pvData2) const = 0;

#if DBG==1
    VOID CheckFreeChainFn() const;
#endif
};

template <class DATA>
class CDataCache : public CDataCacheBase
{
public:
    virtual ~CDataCache()
        { Free(); }

    const DATA& operator[](LONG iel) const
    {
          Assert(Elem(iel)->_pvData != NULL);
          return *(DATA*)(Elem(iel)->_pvData);
    }

    const DATA * ElemData(LONG iel) const
    {
        return Elem(iel)->_pvData ? (DATA*)(Elem(iel)->_pvData) : NULL;
    }

protected:
    virtual void DeletePtr(void *pvData)
    {
        Assert(pvData);
        delete (DATA*) pvData;
    }
    virtual HRESULT InitData(CDataCacheElem *pel, const void *pvData, BOOL fClone)
    {
        Assert(pvData);
        if (fClone)
        {
            return ((DATA*)pvData)->Clone((DATA**)&pel->_pvData);
        }
        else
        {
            //BUGBUG, evil typecast
            pel->_pvData = (void*) pvData;
            return S_OK;
        }
    }

    virtual void PassivateData(CDataCacheElem *pel)
    {
        delete (DATA*)pel->_pvData;
    }

    virtual WORD ComputeDataCrc(const void *pvData) const
    {
        Assert(pvData);
        return ((DATA*)pvData)->ComputeCrc();
    }

    virtual BOOL CompareData(const void *pvData1, const void *pvData2) const
    {
        Assert(pvData1 && pvData2);
        return ((DATA*)pvData1)->Compare((DATA*)pvData2);
    }
};

#pragma INCMSG("--- End 'datcache.hxx'")
#else
#pragma INCMSG("*** Dup 'datcache.hxx'")
#endif
