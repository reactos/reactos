#ifndef __ATLSIMPSTR_H__
#define __ATLSIMPSTR_H__

#pragma once

#include "atlcore.h"


namespace ATL
{
struct CStringData;

interface IAtlStringMgr;
// #undef INTERFACE
// #define INTERFACE IAtlStringMgr
DECLARE_INTERFACE(IAtlStringMgr)
{
public:

    _Ret_maybenull_ _Post_writable_byte_size_(sizeof(CStringData) + nAllocLength*nCharSize)
        CStringData* Allocate(
        _In_ int nAllocLength,
        _In_ int nCharSize
        );

    void Free(
        _Inout_ CStringData* pData
        );

    virtual _Ret_maybenull_ _Post_writable_byte_size_(sizeof(CStringData) + nAllocLength*nCharSize)
        CStringData* Reallocate(
        _Inout_ CStringData* pData,
        _In_ int nAllocLength,
        _In_ int nCharSize
        );

    CStringData* GetNilString(void);
    IAtlStringMgr* Clone(void);
};


struct CStringData
{
    IAtlStringMgr* pStringMgr;
    int nAllocLength;
    int nDataLength;
    long nRefs;

    void* data() throw()
    {
        return (this + 1);
    }

    void AddRef() throw()
    {
        ATLASSERT(nRefs > 0);
        _InterlockedIncrement(&nRefs);
    }

    void Release() throw()
    {
        ATLASSERT(nRefs != 0);

        if (_InterlockedDecrement(&nRefs) <= 0)
        {
            pStringMgr->Free(this);
        }
    }

    bool IsLocked() const throw()
    {
        return (nRefs < 0);
    }

    bool IsShared() const throw()
    {
        return (nRefs > 1);
    }
};

class CNilStringData :
    public CStringData
{
public:
    CNilStringData() throw()
    {
        pStringMgr = NULL;
        nRefs = 2;
        nDataLength = 0;
        nAllocLength = 0;
        achNil[0] = 0;
        achNil[1] = 0;
    }

    void SetManager(_In_ IAtlStringMgr* pMgr) throw()
    {
        ATLASSERT(pStringMgr == NULL);
        pStringMgr = pMgr;
    }

public:
    wchar_t achNil[2];
};

template< typename BaseType = char >
class ChTraitsBase
{
public:
    typedef char XCHAR;
    typedef LPSTR PXSTR;
    typedef LPCSTR PCXSTR;
    typedef wchar_t YCHAR;
    typedef LPWSTR PYSTR;
    typedef LPCWSTR PCYSTR;
};

template<>
class ChTraitsBase< wchar_t >
{
public:
    typedef wchar_t XCHAR;
    typedef LPWSTR PXSTR;
    typedef LPCWSTR PCXSTR;
    typedef char YCHAR;
    typedef LPSTR PYSTR;
    typedef LPCSTR PCYSTR;
};



template< typename BaseType, bool t_bMFCDLL = false>
class CSimpleStringT
{
private:
    LPWSTR m_pszData;

public:
    typedef typename ChTraitsBase<BaseType>::XCHAR XCHAR;
    typedef typename ChTraitsBase<BaseType>::PXSTR PXSTR;
    typedef typename ChTraitsBase<BaseType>::PCXSTR PCXSTR;
    typedef typename ChTraitsBase<BaseType>::YCHAR YCHAR;
    typedef typename ChTraitsBase<BaseType>::PYSTR PYSTR;
    typedef typename ChTraitsBase<BaseType>::PCYSTR PCYSTR;

public:
    explicit CSimpleStringT(_Inout_ IAtlStringMgr* pStringMgr)
    {
        CStringData* pData = pStringMgr->GetNilString();
        Attach(pData);
    }

    CSimpleStringT(_In_ const CSimpleStringT& strSrc)
    {
        CStringData* pSrcData = strSrc.GetData();
        CStringData* pNewData = CloneData(pSrcData);
        Attach(pNewData);
    }

    CSimpleStringT(_In_ const CSimpleStringT<BaseType, !t_bMFCDLL>& strSrc)
    {
        CStringData* pSrcData = strSrc.GetData();
        CStringData* pNewData = CloneData(pSrcData);
        Attach(pNewData);
    }

    CSimpleStringT& operator=(_In_opt_z_ PCXSTR pszSrc)
    {
        SetString(pszSrc);
        return *this;
    }


    operator PCXSTR() const throw()
    {
        return m_pszData;
    }


    void Empty() throw()
    {
        CStringData* pOldData = GetData();
        IAtlStringMgr* pStringMgr = pOldData->pStringMgr;
        if (pOldData->nDataLength == 0) return;

        if (pOldData->IsLocked())
        {
            SetLength(0);
        }
        else
        {
            pOldData->Release();
            CStringData* pNewData = pStringMgr->GetNilString();
            Attach(pNewData);
        }
    }

    void SetString(_In_opt_z_ PCXSTR pszSrc)
    {
        SetString(pszSrc, StringLength(pszSrc));
    }

    void SetString(_In_reads_opt_(nLength) PCXSTR pszSrc,
                   _In_ int nLength)
    {
        if (nLength == 0)
        {
            Empty();
        }
        else
        {
            UINT nOldLength = GetLength();
            UINT_PTR nOffset = pszSrc - GetString();

            PXSTR pszBuffer = GetBuffer(nLength);
            if (nOffset <= nOldLength)
            {
                CopyCharsOverlapped(pszBuffer, GetAllocLength(),
                                    pszBuffer + nOffset, nLength);
            }
            else
            {
                CopyChars(pszBuffer, GetAllocLength(), pszSrc, nLength);
            }
            ReleaseBufferSetLength(nLength);
        }
    }

    static int __cdecl StringLength(_In_opt_z_ const wchar_t* psz) throw()
    {
        if (psz == NULL) return 0;
        return (int)wcslen(psz);
    }

    PXSTR GetBuffer()
    {
        CStringData* pData = GetData();
        if (pData->IsShared())
        {
            // We should fork here
            Fork(pData->nDataLength);
        }

        return m_pszData;
    }

    int GetAllocLength() const throw()
    {
        return GetData()->nAllocLength;
    }

    int GetLength() const throw()
    {
        return GetData()->nDataLength;
    }

    PCXSTR GetString() const throw()
    {
        return m_pszData;
    }

    void ReleaseBufferSetLength(_In_ int nNewLength)
    {
        ATLASSERT(nNewLength >= 0);
        SetLength(nNewLength);
    }

    bool IsEmpty() const throw()
    {
        return (GetLength() == 0);
    }

    _Ret_notnull_ _Post_writable_size_(nMinBufferLength + 1) PXSTR GetBuffer(_In_ int nMinBufferLength)
    {
        return PrepareWrite(nMinBufferLength);
    }

    CStringData* GetData() const throw()
    {
        return reinterpret_cast<CStringData*>(m_pszData) - 1;
    }

    static void __cdecl CopyChars(
        _Out_writes_to_(nDestLen, nChars) XCHAR* pchDest,
        _In_ size_t nDestLen,
        _In_reads_opt_(nChars) const XCHAR* pchSrc,
        _In_ int nChars) throw()
    {
        memcpy(pchDest, pchSrc, nChars * sizeof(XCHAR));
    }

    static void __cdecl CopyCharsOverlapped(
        _Out_writes_to_(nDestLen, nDestLen) XCHAR* pchDest,
        _In_ size_t nDestLen,
        _In_reads_(nChars) const XCHAR* pchSrc,
        _In_ int nChars) throw()
    {
        memmove(pchDest, pchSrc, nChars * sizeof(XCHAR));
    }


private:

    void Attach(_Inout_ CStringData* pData) throw()
    {
        m_pszData = static_cast<PXSTR>(pData->data());
    }
    __declspec(noinline) void Fork(_In_ int nLength)
    {
        CStringData* pOldData = GetData();
        int nOldLength = pOldData->nDataLength;
        CStringData* pNewData = pOldData->pStringMgr->Clone()->Allocate(nLength, sizeof(XCHAR));
        if (pNewData == NULL)
        {
            throw; // ThrowMemoryException();
        }
        int nCharsToCopy = ((nOldLength < nLength) ? nOldLength : nLength) + 1;
        CopyChars(PXSTR(pNewData->data()), nCharsToCopy,
                  PCXSTR(pOldData->data()), nCharsToCopy);
        pNewData->nDataLength = nOldLength;
        pOldData->Release();
        Attach(pNewData);
    }


    PXSTR PrepareWrite(_In_ int nLength)
    {
        CStringData* pOldData = GetData();
        int nShared = 1 - pOldData->nRefs;
        int nTooShort = pOldData->nAllocLength - nLength;
        if ((nShared | nTooShort) < 0)
        {
            PrepareWrite2(nLength);
        }

        return m_pszData;
    }
    void PrepareWrite2(_In_ int nLength)
    {
        CStringData* pOldData = GetData();
        if (pOldData->nDataLength > nLength)
        {
            nLength = pOldData->nDataLength;
        }
        if (pOldData->IsShared())
        {
            Fork(nLength);
            ATLASSERT(FALSE);
        }
        else if (pOldData->nAllocLength < nLength)
        {
            int nNewLength = pOldData->nAllocLength;
            if (nNewLength > 1024 * 1024 * 1024)
            {
                nNewLength += 1024 * 1024;
            }
            else
            {
                nNewLength = nNewLength + nNewLength / 2;
            }
            if (nNewLength < nLength)
            {
                nNewLength = nLength;
            }
            Reallocate(nNewLength);
        }
    }

    void Reallocate(_In_ int nLength)
    {
        CStringData* pOldData = GetData();
        ATLASSERT(pOldData->nAllocLength < nLength);
        IAtlStringMgr* pStringMgr = pOldData->pStringMgr;
        if (pOldData->nAllocLength >= nLength || nLength <= 0)
        {
            return;
        }
        CStringData* pNewData = pStringMgr->Reallocate(pOldData, nLength, sizeof(XCHAR));
        if (pNewData == NULL) throw;

        Attach(pNewData);
    }

    void SetLength(_In_ int nLength)
    {
        ATLASSERT(nLength >= 0);
        ATLASSERT(nLength <= GetData()->nAllocLength);

        if (nLength < 0 || nLength > GetData()->nAllocLength)
            throw;

        GetData()->nDataLength = nLength;
        m_pszData[nLength] = 0;
    }

    static CStringData* __cdecl CloneData(_Inout_ CStringData* pData)
    {
        CStringData* pNewData = NULL;

        IAtlStringMgr* pNewStringMgr = pData->pStringMgr->Clone();
        if (!pData->IsLocked() && (pNewStringMgr == pData->pStringMgr))
        {
            pNewData = pData;
            pNewData->AddRef();
        }
        else
        {
            pNewData = pNewStringMgr->Allocate(pData->nDataLength, sizeof(XCHAR));
            if (pNewData == NULL)
            {
                throw; // ThrowMemoryException();
            }
            pNewData->nDataLength = pData->nDataLength;
            CopyChars(PXSTR(pNewData->data()), pData->nDataLength + 1,
                      PCXSTR(pData->data()), pData->nDataLength + 1);
        }

        return( pNewData );
    }

};
}

#endif