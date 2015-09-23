#ifndef __CSTRINGT_H__
#define __CSTRINGT_H__

#pragma once
#include <atlsimpstr.h>
#include <stddef.h>
#include <stdio.h>
#include <wchar.h>
#include <atlmem.h>

namespace ATL
{

inline UINT WINAPI _AtlGetConversionACP() throw()
{
#ifdef _CONVERSION_DONT_USE_THREAD_LOCALE
    return CP_ACP;
#else
    return CP_THREAD_ACP;
#endif
}


template<typename _CharType = wchar_t>
class ChTraitsCRT : public ChTraitsBase<_CharType>
{
public:

    static int __cdecl GetBaseTypeLength(_In_z_ LPCWSTR pszSource) throw()
    {
        return ::WideCharToMultiByte(_AtlGetConversionACP(), 0, pszSource, -1, NULL, 0, NULL, NULL) - 1;
    }

    static int __cdecl GetBaseTypeLength(
        _In_reads_(nLength) LPCWSTR pszSource,
        _In_ int nLength) throw()
    {
        return ::WideCharToMultiByte(CP_THREAD_ACP, 0, pszSource, nLength, NULL, 0, NULL, NULL);
    }

    static void __cdecl ConvertToBaseType(
        _Out_writes_(nDestLength) LPWSTR pszDest,
        _In_ int nDestLength,
        _In_ LPCWSTR pszSrc,
        _In_ int nSrcLength = -1)
    {
        if (nSrcLength == -1)
            nSrcLength = 1 + GetBaseTypeLength(pszSrc);

        wmemcpy(pszDest, pszSrc, nSrcLength);
    }
};



namespace _CSTRING_IMPL_
{
    template <typename _CharType, class StringTraits>
    struct _MFCDLLTraitsCheck
    {
        const static bool c_bIsMFCDLLTraits = false;
    };
}


template <typename BaseType, class StringTraits>
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

    static void __cdecl Construct(_In_ CStringT* pString)
    {
        new pString (CStringT);
    }

    CStringT(_In_ const CStringT& strSrc) :
        CThisSimpleString(strSrc)
    {
    }

    CStringT& operator=(_In_opt_z_ PCXSTR pszSrc)
    {
        CThisSimpleString::operator=(pszSrc);
        return *this;
    }

    _Check_return_ BOOL LoadString(_In_ HINSTANCE hInstance,
                                   _In_ UINT nID)
    {
        const ATLSTRINGRESOURCEIMAGE* pImage = AtlGetStringResourceImage(hInstance, nID);
        if (pImage == NULL) return FALSE;

        int nLength = StringTraits::GetBaseTypeLength(pImage->achString, pImage->nLength);
        PXSTR pszBuffer = GetBuffer(nLength);
        StringTraits::ConvertToBaseType(pszBuffer, nLength, pImage->achString, pImage->nLength);
        ReleaseBufferSetLength(nLength);

        return TRUE;
    }
};

} //namespace ATL

#endif