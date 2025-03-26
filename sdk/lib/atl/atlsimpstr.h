#ifndef __ATLSIMPSTR_H__
#define __ATLSIMPSTR_H__

#pragma once

#include <atlcore.h>
#include <atlexcept.h>

#ifdef __RATL__
    #ifndef _In_count_
        #define _In_count_(nLength)
    #endif
#endif

namespace ATL
{
struct CStringData;

// Pure virtual interface
class IAtlStringMgr
{
public:

    virtual ~IAtlStringMgr() {}

    virtual _Ret_maybenull_ _Post_writable_byte_size_(sizeof(CStringData) + nAllocLength*nCharSize)
        CStringData* Allocate(
        _In_ int nAllocLength,
        _In_ int nCharSize
        ) = 0;

    virtual void Free(
        _Inout_ CStringData* pData
        ) = 0;

    virtual _Ret_maybenull_ _Post_writable_byte_size_(sizeof(CStringData) + nAllocLength*nCharSize)
        CStringData* Reallocate(
        _Inout_ CStringData* pData,
        _In_ int nAllocLength,
        _In_ int nCharSize
        ) = 0;

    virtual CStringData* GetNilString(void) = 0;
    virtual IAtlStringMgr* Clone(void) = 0;
};


struct CStringData
{
    IAtlStringMgr* pStringMgr;
    int nAllocLength;
    int nDataLength;
    long nRefs;

    void* data() noexcept
    {
        return (this + 1);
    }

    void AddRef() noexcept
    {
        ATLASSERT(nRefs > 0);
        _InterlockedIncrement(&nRefs);
    }

    void Release() noexcept
    {
        ATLASSERT(nRefs != 0);

        if (_InterlockedDecrement(&nRefs) <= 0)
        {
            pStringMgr->Free(this);
        }
    }

    bool IsLocked() const noexcept
    {
        return (nRefs < 0);
    }

    bool IsShared() const noexcept
    {
        return (nRefs > 1);
    }
};

class CNilStringData :
    public CStringData
{
public:
    CNilStringData() noexcept
    {
        pStringMgr = NULL;
        nRefs = 2;
        nDataLength = 0;
        nAllocLength = 0;
        achNil[0] = 0;
        achNil[1] = 0;
    }

    void SetManager(_In_ IAtlStringMgr* pMgr) noexcept
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
class ChTraitsBase<wchar_t>
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
public:
    typedef typename ChTraitsBase<BaseType>::XCHAR XCHAR;
    typedef typename ChTraitsBase<BaseType>::PXSTR PXSTR;
    typedef typename ChTraitsBase<BaseType>::PCXSTR PCXSTR;
    typedef typename ChTraitsBase<BaseType>::YCHAR YCHAR;
    typedef typename ChTraitsBase<BaseType>::PYSTR PYSTR;
    typedef typename ChTraitsBase<BaseType>::PCYSTR PCYSTR;

private:
    PXSTR m_pszData;

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

    CSimpleStringT(
        _In_z_ PCXSTR pszSrc,
        _Inout_ IAtlStringMgr* pStringMgr)
    {
        int nLength = StringLength(pszSrc);
        CStringData* pData = pStringMgr->Allocate(nLength, sizeof(XCHAR));
        if (pData == NULL)
            ThrowMemoryException();

        Attach(pData);
        SetLength(nLength);
        CopyChars(m_pszData, nLength, pszSrc, nLength);
    }

    CSimpleStringT(
        _In_count_(nLength) const XCHAR* pchSrc,
        _In_ int nLength,
        _Inout_ IAtlStringMgr* pStringMgr)
    {
        if (pchSrc == NULL && nLength != 0)
            ThrowInvalidArgException();

        CStringData* pData = pStringMgr->Allocate(nLength, sizeof(XCHAR));
        if (pData == NULL)
        {
            ThrowMemoryException();
        }
        Attach(pData);
        SetLength(nLength);
        CopyChars(m_pszData, nLength, pchSrc, nLength);
    }

    ~CSimpleStringT() noexcept
    {
        CStringData* pData = GetData();
        pData->Release();
    }

    CSimpleStringT& operator=(_In_opt_z_ PCXSTR pszSrc)
    {
        SetString(pszSrc);
        return *this;
    }

    CSimpleStringT& operator=(_In_ const CSimpleStringT& strSrc)
    {
        CStringData* pData = GetData();
        CStringData* pNewData = strSrc.GetData();

        if (pNewData != pData)
        {
            if (!pData->IsLocked() && (pNewData->pStringMgr == pData->pStringMgr))
            {
                pNewData = CloneData(pNewData);
                pData->Release();
                Attach(pNewData);
            }
            else
            {
                SetString(strSrc.GetString(), strSrc.GetLength());
            }
        }

        return *this;
    }

    CSimpleStringT& operator+=(_In_ const CSimpleStringT& strSrc)
    {
        Append(strSrc);
        return *this;
    }

    CSimpleStringT& operator+=(_In_z_ PCXSTR pszSrc)
    {
        Append(pszSrc);
        return *this;
    }

    CSimpleStringT& operator+=(XCHAR ch)
    {
        Append(&ch, 1);
        return *this;
    }

    operator PCXSTR() const noexcept
    {
        return m_pszData;
    }

    void Empty() noexcept
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

    void Append(
        _In_count_(nLength) PCXSTR pszSrc,
        _In_ int nLength)
    {
        UINT_PTR nOffset = pszSrc - GetString();

        int nOldLength = GetLength();
        if (nOldLength < 0)
            nOldLength = 0;

        ATLASSERT(nLength >= 0);

#if 0 // FIXME: See comment for StringLengthN below.
        nLength = StringLengthN(pszSrc, nLength);
        if (!(INT_MAX - nLength >= nOldLength))
            throw;
#endif

        int nNewLength = nOldLength + nLength;
        PXSTR pszBuffer = GetBuffer(nNewLength);
        if (nOffset <= (UINT_PTR)nOldLength)
        {
            pszSrc = pszBuffer + nOffset;
        }
        CopyChars(pszBuffer + nOldLength, nLength, pszSrc, nLength);
        ReleaseBufferSetLength(nNewLength);
    }

    void Append(_In_z_ PCXSTR pszSrc)
    {
        Append(pszSrc, StringLength(pszSrc));
    }

    void Append(_In_ const CSimpleStringT& strSrc)
    {
        Append(strSrc.GetString(), strSrc.GetLength());
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

    _Ret_notnull_ _Post_writable_size_(nMinBufferLength + 1) PXSTR GetBuffer(_In_ int nMinBufferLength)
    {
        return PrepareWrite(nMinBufferLength);
    }

    int GetAllocLength() const noexcept
    {
        return GetData()->nAllocLength;
    }

    int GetLength() const noexcept
    {
        return GetData()->nDataLength;
    }

    PXSTR GetString() noexcept
    {
        return m_pszData;
    }
    PCXSTR GetString() const noexcept
    {
        return m_pszData;
    }

    void Preallocate(_In_ int nLength)
    {
        PrepareWrite(nLength);
    }

    void ReleaseBufferSetLength(_In_ int nNewLength)
    {
        ATLASSERT(nNewLength >= 0);
        SetLength(nNewLength);
    }

    void ReleaseBuffer(_In_ int nNewLength = -1)
    {
        if (nNewLength < 0)
            nNewLength = StringLength(m_pszData);
        ReleaseBufferSetLength(nNewLength);
    }

    bool IsEmpty() const noexcept
    {
        return (GetLength() == 0);
    }

    CStringData* GetData() const noexcept
    {
        return (reinterpret_cast<CStringData*>(m_pszData) - 1);
    }

    IAtlStringMgr* GetManager() const noexcept
    {
        IAtlStringMgr* pStringMgr = GetData()->pStringMgr;
        return (pStringMgr ? pStringMgr->Clone() : NULL);
    }

public:
    friend CSimpleStringT operator+(
        _In_ const CSimpleStringT& str1,
        _In_ const CSimpleStringT& str2)
    {
        CSimpleStringT s(str1.GetManager());
        Concatenate(s, str1, str1.GetLength(), str2, str2.GetLength());
        return s;
    }

    friend CSimpleStringT operator+(
        _In_ const CSimpleStringT& str1,
        _In_z_ PCXSTR psz2)
    {
        CSimpleStringT s(str1.GetManager());
        Concatenate(s, str1, str1.GetLength(), psz2, StringLength(psz2));
        return s;
    }

    friend CSimpleStringT operator+(
        _In_z_ PCXSTR psz1,
        _In_ const CSimpleStringT& str2)
    {
        CSimpleStringT s(str2.GetManager());
        Concatenate(s, psz1, StringLength(psz1), str2, str2.GetLength());
        return s;
    }

    static void __cdecl CopyChars(
        _Out_writes_to_(nDestLen, nChars) XCHAR* pchDest,
        _In_ size_t nDestLen,
        _In_reads_opt_(nChars) const XCHAR* pchSrc,
        _In_ int nChars) noexcept
    {
        memcpy(pchDest, pchSrc, nChars * sizeof(XCHAR));
    }

    static void __cdecl CopyCharsOverlapped(
        _Out_writes_to_(nDestLen, nDestLen) XCHAR* pchDest,
        _In_ size_t nDestLen,
        _In_reads_(nChars) const XCHAR* pchSrc,
        _In_ int nChars) noexcept
    {
        memmove(pchDest, pchSrc, nChars * sizeof(XCHAR));
    }

    static int __cdecl StringLength(_In_opt_z_ const char* psz) noexcept
    {
        if (psz == NULL) return 0;
        return (int)strlen(psz);
    }

    static int __cdecl StringLength(_In_opt_z_ const wchar_t* psz) noexcept
    {
        if (psz == NULL) return 0;
        return (int)wcslen(psz);
    }

#if 0 // For whatever reason we do not link with strnlen / wcsnlen. Please investigate!
      // strnlen / wcsnlen are available in MSVCRT starting Vista+.
    static int __cdecl StringLengthN(
        _In_opt_z_count_(sizeInXChar) const char* psz,
        _In_ size_t sizeInXChar) noexcept
    {
        if (psz == NULL) return 0;
        return (int)strnlen(psz, sizeInXChar);
    }

    static int __cdecl StringLengthN(
        _In_opt_z_count_(sizeInXChar) const wchar_t* psz,
        _In_ size_t sizeInXChar) noexcept
    {
        if (psz == NULL) return 0;
        return (int)wcsnlen(psz, sizeInXChar);
    }
#endif

protected:
    static void __cdecl Concatenate(
        _Inout_ CSimpleStringT& strResult,
        _In_count_(nLength1) PCXSTR psz1,
        _In_ int nLength1,
        _In_count_(nLength2) PCXSTR psz2,
        _In_ int nLength2)
    {
        int nNewLength = nLength1 + nLength2;
        PXSTR pszBuffer = strResult.GetBuffer(nNewLength);
        CopyChars(pszBuffer, nLength1, psz1, nLength1);
        CopyChars(pszBuffer + nLength1, nLength2, psz2, nLength2);
        strResult.ReleaseBufferSetLength(nNewLength);
    }

private:
    void Attach(_Inout_ CStringData* pData) noexcept
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
            ThrowMemoryException();
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
            //ATLASSERT(FALSE);
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
        if (pNewData == NULL)
        {
            ThrowMemoryException();
        }

        Attach(pNewData);
    }

    void SetLength(_In_ int nLength)
    {
        ATLASSERT(nLength >= 0);
        ATLASSERT(nLength <= GetData()->nAllocLength);

        if (nLength < 0 || nLength > GetData()->nAllocLength)
        {
            AtlThrow(E_INVALIDARG);
        }

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
                ThrowMemoryException();
            }

            pNewData->nDataLength = pData->nDataLength;
            CopyChars(PXSTR(pNewData->data()), pData->nDataLength + 1,
                      PCXSTR(pData->data()), pData->nDataLength + 1);
        }

        return pNewData;
    }

protected:
    static void ThrowMemoryException()
    {
        AtlThrow(E_OUTOFMEMORY);
    }

    static void ThrowInvalidArgException()
    {
        AtlThrow(E_INVALIDARG);
    }
};

#ifdef UNICODE
typedef CSimpleStringT<WCHAR>   CSimpleString;
#else
typedef CSimpleStringT<CHAR>    CSimpleString;
#endif
}

#endif
