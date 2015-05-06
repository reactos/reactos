#ifndef __CSTRINGT_H__
#define __CSTRINGT_H__

#pragma once

#include <atlsimpstr.h>
#include <stddef.h>

#include <stdio.h>
#include <wchar.h>


namespace ATL
{

    template< typename _CharType = wchar_t>
    class ChTraitsCRT :
        public ChTraitsBase<_CharType>
    {
    public:

    };



    namespace _CSTRING_IMPL_
    {
        template <typename _CharType, class StringTraits>
        struct _MFCDLLTraitsCheck
        {
            const static bool c_bIsMFCDLLTraits = false;
        };
    }

    template< typename BaseType, class StringTraits>
    class CStringT :
        public CSimpleStringT <BaseType, _CSTRING_IMPL_::_MFCDLLTraitsCheck<BaseType, StringTraits>::c_bIsMFCDLLTraits>
    {
    public:
        typedef CSimpleStringT<BaseType, _CSTRING_IMPL_::_MFCDLLTraitsCheck<BaseType, StringTraits>::c_bIsMFCDLLTraits> CThisSimpleString;
        typedef StringTraits StrTraits;
        typedef typename CThisSimpleString::XCHAR XCHAR;
        typedef typename CThisSimpleString::PXSTR PXSTR;
        typedef typename CThisSimpleString::PCXSTR PCXSTR;
        typedef typename CThisSimpleString::YCHAR YCHAR;
        typedef typename CThisSimpleString::PYSTR PYSTR;
        typedef typename CThisSimpleString::PCYSTR PCYSTR;

    public:
        CStringT() throw() :
            CThisSimpleString(StringTraits::GetDefaultManager())
        {
        }
        explicit CStringT( _In_ IAtlStringMgr* pStringMgr) throw() :
            CThisSimpleString(pStringMgr)
        {
        }

        CStringT(_In_ const VARIANT& varSrc);
        CStringT(
            _In_ const VARIANT& varSrc,
            _In_ IAtlStringMgr* pStringMgr);

        static void __cdecl Construct(_In_ CStringT* pString)
        {
            new(pString)CStringT;
        }

        CStringT(_In_ const CStringT& strSrc) :
            CThisSimpleString(strSrc)
        {
        }
    };

} //namespace ATL

#endif