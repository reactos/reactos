#ifndef __ATLSIMPSTR_H__
#define __ATLSIMPSTR_H__

#pragma once

#include <atlcore.h>


namespace ATL
{
    struct CStringData;

    __interface IAtlStringMgr
    {
    public:

        _Ret_maybenull_ _Post_writable_byte_size_(sizeof(CStringData) + nAllocLength*nCharSize)
            CStringData* Allocate(
            _In_ int nAllocLength,
            _In_ int nCharSize) throw();

        void Free(_Inout_ CStringData* pData) throw();

        virtual _Ret_maybenull_ _Post_writable_byte_size_(sizeof(CStringData) + nAllocLength*nCharSize)
            CStringData* Reallocate(
            _Inout_ CStringData* pData,
            _In_ int nAllocLength,
            _In_ int nCharSize) throw();

        CStringData* GetNilString() throw();

        IAtlStringMgr* Clone() throw();
    };

    struct CStringData
    {
        IAtlStringMgr* pStringMgr;
        int nDataLength;
        int nAllocLength;
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
            ATLENSURE(pStringMgr != NULL);
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

    };
}

#endif