#ifndef __ATLSTR_H__
#define __ATLSTR_H__

#pragma once
#include "atlbase.h"
#include "cstringt.h"

namespace ATL
{

class CAtlStringMgr : public IAtlStringMgr
{
protected:
    IAtlMemMgr* m_MemMgr;
    CNilStringData m_NilStrData;

public:
    CAtlStringMgr(_In_opt_ IAtlMemMgr* MemMgr = NULL):
        m_MemMgr(MemMgr)
    {
        m_NilStrData.SetManager(this);
    }

    virtual ~CAtlStringMgr(void)
    {
    }

    static IAtlStringMgr* GetInstance(void)
    {
        static CWin32Heap Win32Heap(::GetProcessHeap());
        static CAtlStringMgr StringMgr(&Win32Heap);
        return &StringMgr;
    }

    virtual _Ret_maybenull_ _Post_writable_byte_size_(sizeof(CStringData) + NumChars * CharSize) CStringData* Allocate(
        _In_ int NumChars,
        _In_ int CharSize)
    {
        size_t SizeBytes;
        CStringData* StrData;

        SizeBytes = sizeof(CStringData) + ((NumChars + 1) * CharSize);

        StrData = static_cast<CStringData*>(m_MemMgr->Allocate(SizeBytes));
        if (StrData == NULL) return NULL;

        StrData->pStringMgr = this;
        StrData->nRefs = 1;
        StrData->nAllocLength = NumChars;
        StrData->nDataLength = 0;

        return StrData;
    }

    virtual void Free(_In_ CStringData* StrData)
    {
        ATLASSERT(StrData->pStringMgr == this);
        m_MemMgr->Free(StrData);
    }

    virtual _Ret_maybenull_ _Post_writable_byte_size_(sizeof(CStringData) + nChars*nCharSize) CStringData* Reallocate(
        _Inout_ _Post_readable_byte_size_(sizeof(CStringData)) CStringData* StrData,
        _In_ int nChars,
        _In_ int nCharSize) noexcept
    {
        ATLASSERT(StrData->pStringMgr == this);

        CStringData* pNewData;
        ULONG SizeBytes;
        ULONG nDataBytes;

        nChars++;
        nDataBytes = nChars * nCharSize;
        SizeBytes = sizeof(CStringData) + nDataBytes;

        pNewData = static_cast<CStringData*>(m_MemMgr->Reallocate(StrData, SizeBytes));
        if (pNewData == NULL) return NULL;

        pNewData->nAllocLength = nChars - 1;
        return pNewData;
    }
    virtual CStringData* GetNilString() noexcept
    {
        m_NilStrData.AddRef();
        return &m_NilStrData;
    }
    virtual IAtlStringMgr* Clone() noexcept
    {
        return this;
    }

private:
    static bool StaticInitialize()
    {
        GetInstance();
        return true;
    }
};


template< typename _CharType = wchar_t >
class ChTraitsOS :
    public ChTraitsBase < _CharType >
{

};


template<typename _BaseType = wchar_t, class StringIterator = ChTraitsOS<_BaseType> >
class StrTraitATL :
    public StringIterator
{
public:
    static HINSTANCE FindStringResourceInstance(_In_ UINT nID) noexcept
    {
        return AtlFindStringResourceInstance(nID);
    }

    static IAtlStringMgr* GetDefaultManager() noexcept
    {
        return CAtlStringMgr::GetInstance();
    }
};


typedef CStringT< wchar_t, StrTraitATL< wchar_t, ChTraitsCRT<wchar_t> > > CAtlStringW;
typedef CStringT< char, StrTraitATL< char, ChTraitsCRT<char> > > CAtlStringA;


typedef CAtlStringW CStringW;
typedef CAtlStringA CStringA;


#ifdef UNICODE
typedef CAtlStringW CAtlString;
typedef CStringW CString;
#else
typedef CAtlStringA CAtlString;
typedef CStringA CString;
#endif


}

#endif
