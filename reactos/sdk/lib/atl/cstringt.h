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
        if (pszSource == NULL) return -1;
        return static_cast<int>(wcslen(pszSource));
    }

    static int __cdecl GetBaseTypeLength(_In_z_ LPCSTR pszSource) throw()
    {
        if (pszSource == NULL) return 0;
        return ::MultiByteToWideChar(_AtlGetConversionACP(), 0, pszSource, -1, NULL, 0) - 1;
    }

    static int __cdecl GetBaseTypeLength(
        _In_reads_(nLength) LPCWSTR pszSource,
        _In_ int nLength) throw()
    {
        return nLength;
    }

    static int __cdecl GetBaseTypeLength(
        _In_reads_(nLength) LPCSTR pszSource,
        _In_ int nLength) throw()
    {
        return ::MultiByteToWideChar(_AtlGetConversionACP(), 0, pszSource, nLength, NULL, 0);
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

    static void __cdecl ConvertToBaseType(
        _Out_writes_(nDestLength) LPWSTR pszDest,
        _In_ int nDestLength,
        _In_ LPCSTR pszSrc,
        _In_ int nSrcLength = -1)
    {
        if (nSrcLength == -1)
            nSrcLength = 1 + GetBaseTypeLength(pszSrc);

        ::MultiByteToWideChar(_AtlGetConversionACP(), 0, pszSrc, nSrcLength, pszDest, nDestLength);
    }

    static void __cdecl MakeLower(
        _Out_writes_(nSrcLength) LPWSTR pszSource,
        _In_ int nSrcLength)
    {
        ::CharLowerBuffW(pszSource, nSrcLength);
    }

    static DWORD GetEnvironmentVariable(
        _In_z_ LPCWSTR pszVar,
        _Out_writes_opt_(nBufLength) LPWSTR pszBuf,
        _In_opt_ int nBufLength)
    {
        return ::GetEnvironmentVariableW(pszVar, pszBuf, nBufLength);
    }

    static void __cdecl MakeUpper(
        _Out_writes_(nSrcLength) LPWSTR pszSource,
        _In_ int nSrcLength)
    {
        ::CharUpperBuffW(pszSource, nSrcLength);
    }

    static LPWSTR __cdecl FindString(
        _In_z_ LPCWSTR pszSource,
        _In_z_ LPCWSTR pszSub)
    {
        return ::wcsstr(pszSource, pszSub);
    }

    static LPWSTR __cdecl FindChar(
        _In_z_ LPCWSTR pszSource,
        _In_ WCHAR ch)
    {
        return ::wcschr(pszSource, ch);
    }

    static LPWSTR __cdecl FindCharReverse(
        _In_z_ LPCWSTR pszSource,
        _In_ WCHAR ch)
    {
        return ::wcsrchr(pszSource, ch);
    }

    static LPWSTR __cdecl FindOneOf(
        _In_z_ LPCWSTR pszSource,
        _In_z_ LPCWSTR pszCharSet)
    {
        return ::wcspbrk(pszSource, pszCharSet);
    }

    static int __cdecl Compare(
        _In_z_ LPCWSTR psz1,
        _In_z_ LPCWSTR psz2)
    {
        return ::wcscmp(psz1, psz2);
    }

    static int __cdecl FormatV(
        _In_opt_z_ LPWSTR pszDest,
        _In_z_ LPCWSTR pszFormat,
        _In_ va_list args)
    {
        if (pszDest == NULL)
            return ::_vscwprintf(pszFormat, args);
        return ::vswprintf(pszDest, pszFormat, args);
    }

};


// Template specialization

template<>
class ChTraitsCRT<char> : public ChTraitsBase<char>
{
public:

    static int __cdecl GetBaseTypeLength(_In_z_ LPCWSTR pszSource) throw()
    {
        return ::WideCharToMultiByte(_AtlGetConversionACP(), 0, pszSource, -1, NULL, 0, NULL, NULL) - 1;
    }

    static int __cdecl GetBaseTypeLength(_In_z_ LPCSTR pszSource) throw()
    {
        if (pszSource == NULL) return 0;
        return static_cast<int>(strlen(pszSource));
    }

    static int __cdecl GetBaseTypeLength(
        _In_reads_(nLength) LPCWSTR pszSource,
        _In_ int nLength) throw()
    {
        return ::WideCharToMultiByte(_AtlGetConversionACP(), 0, pszSource, nLength, NULL, 0, NULL, NULL);
    }

    static int __cdecl GetBaseTypeLength(
        _In_reads_(nLength) LPCSTR pszSource,
        _In_ int nLength) throw()
    {
        return nLength;
    }

    static void __cdecl ConvertToBaseType(
        _Out_writes_(nDestLength) LPSTR pszDest,
        _In_ int nDestLength,
        _In_ LPCWSTR pszSrc,
        _In_ int nSrcLength = -1)
    {
        if (nSrcLength == -1)
            nSrcLength = 1 + GetBaseTypeLength(pszSrc);

        ::WideCharToMultiByte(_AtlGetConversionACP(), 0, pszSrc, nSrcLength, pszDest, nDestLength, NULL, NULL);
    }

    static void __cdecl ConvertToBaseType(
        _Out_writes_(nDestLength) LPSTR pszDest,
        _In_ int nDestLength,
        _In_ LPCSTR pszSrc,
        _In_ int nSrcLength = -1)
    {
        if (nSrcLength == -1)
            nSrcLength = 1 + GetBaseTypeLength(pszSrc);

        memcpy(pszDest, pszSrc, nSrcLength);
    }

    static void __cdecl MakeLower(
        _Out_writes_(nSrcLength) LPSTR pszSource,
        _In_ int nSrcLength)
    {
        ::CharLowerBuffA(pszSource, nSrcLength);
    }

    static DWORD GetEnvironmentVariable(
        _In_z_ LPCSTR pszVar,
        _Out_writes_opt_(nBufLength) LPSTR pszBuf,
        _In_opt_ int nBufLength)
    {
        return ::GetEnvironmentVariableA(pszVar, pszBuf, nBufLength);
    }

    static void __cdecl MakeUpper(
        _Out_writes_(nSrcLength) LPSTR pszSource,
        _In_ int nSrcLength)
    {
        ::CharUpperBuffA(pszSource, nSrcLength);
    }

    static LPSTR __cdecl FindString(
        _In_z_ LPCSTR pszSource,
        _In_z_ LPCSTR pszSub)
    {
        return ::strstr(pszSource, pszSub);
    }

    static LPSTR __cdecl FindChar(
        _In_z_ LPCSTR pszSource,
        _In_ CHAR ch)
    {
        return ::strchr(pszSource, ch);
    }

    static LPSTR __cdecl FindCharReverse(
        _In_z_ LPCSTR pszSource,
        _In_ CHAR ch)
    {
        return ::strrchr(pszSource, ch);
    }

    static LPSTR __cdecl FindOneOf(
        _In_z_ LPCSTR pszSource,
        _In_z_ LPCSTR pszCharSet)
    {
        return ::strpbrk(pszSource, pszCharSet);
    }

    static int __cdecl Compare(
        _In_z_ LPCSTR psz1,
        _In_z_ LPCSTR psz2)
    {
        return ::strcmp(psz1, psz2);
    }

    static int __cdecl FormatV(
        _In_opt_z_ LPSTR pszDest,
        _In_z_ LPCSTR pszFormat,
        _In_ va_list args)
    {
        if (pszDest == NULL)
            return ::_vscprintf(pszFormat, args);
        return ::vsprintf(pszDest, pszFormat, args);
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


// TODO: disable conversion functions when _CSTRING_DISABLE_NARROW_WIDE_CONVERSION is defined.

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
        new(pString) CStringT;
    }

    CStringT(_In_ const CStringT& strSrc) :
        CThisSimpleString(strSrc)
    {
    }

    template<class StringTraits_>
    CStringT(_In_ const CStringT<YCHAR, StringTraits_> & strSrc) :
        CThisSimpleString(StringTraits::GetDefaultManager())
    {
        *this = static_cast<const YCHAR*>(strSrc);
    }

protected:
    /* helper function */
    template <typename T_CHAR>
    void LoadFromPtr_(_In_opt_z_ const T_CHAR* pszSrc)
    {
        if (pszSrc == NULL)
            return;
        if (IS_INTRESOURCE(pszSrc))
            LoadString(LOWORD(pszSrc));
        else
            *this = pszSrc;
    }

public:
    CStringT(_In_opt_z_ const XCHAR* pszSrc) :
        CThisSimpleString(StringTraits::GetDefaultManager())
    {
        LoadFromPtr_(pszSrc);
    }

    CStringT(_In_opt_z_ const XCHAR* pszSrc,
             _In_ IAtlStringMgr* pStringMgr) : CThisSimpleString(pStringMgr)
    {
        LoadFromPtr_(pszSrc);
    }

    CStringT(_In_opt_z_ const YCHAR* pszSrc) :
        CThisSimpleString(StringTraits::GetDefaultManager())
    {
        LoadFromPtr_(pszSrc);
    }

    CStringT(_In_opt_z_ const YCHAR* pszSrc,
             _In_ IAtlStringMgr* pStringMgr) : CThisSimpleString(pStringMgr)
    {
        LoadFromPtr_(pszSrc);
    }

    CStringT(_In_reads_z_(nLength) const XCHAR* pch,
             _In_ int nLength) : 
        CThisSimpleString(pch, nLength, StringTraits::GetDefaultManager())
    {
    }

    CStringT(_In_reads_z_(nLength) const YCHAR* pch,
             _In_ int nLength) : 
        CThisSimpleString(pch, nLength, StringTraits::GetDefaultManager())
    {
    }

    CStringT& operator=(_In_ const CStringT& strSrc)
    {
        CThisSimpleString::operator=(strSrc);
        return *this;
    }

    CStringT& operator=(_In_opt_z_ PCXSTR pszSrc)
    {
        CThisSimpleString::operator=(pszSrc);
        return *this;
    }

    CStringT& operator=(_In_opt_z_ PCYSTR pszSrc)
    {
        int length = pszSrc ? StringTraits::GetBaseTypeLength(pszSrc) : 0;
        if (length > 0)
        {
            PXSTR result = CThisSimpleString::GetBuffer(length);
            StringTraits::ConvertToBaseType(result, length, pszSrc);
            CThisSimpleString::ReleaseBufferSetLength(length);
        }
        else
        {
            CThisSimpleString::Empty();
        }
        return *this;
    }

    friend bool operator==(const CStringT& str1, const CStringT& str2) throw()
    {
        return str1.Compare(str2) == 0;
    }

    friend bool operator==(const CStringT& str1, PCXSTR psz2) throw()
    {
        return str1.Compare(psz2) == 0;
    }

    friend bool operator==(const CStringT& str1, PCYSTR psz2) throw()
    {
        CStringT tmp(psz2, str1.GetManager());
        return tmp == str1;
    }

    friend bool operator==(const CStringT& str1, XCHAR ch2) throw()
    {
        return str1.GetLength() == 1 && str1[0] == ch2;
    }

    friend bool operator==(PCXSTR psz1, const CStringT& str2) throw()
    {
        return str2.Compare(psz1) == 0;
    }

    friend bool operator==(PCYSTR psz1, const CStringT& str2) throw()
    {
        CStringT tmp(psz1, str2.GetManager());
        return tmp.Compare(str2) == 0;
    }

    friend bool operator==(XCHAR ch1, const CStringT& str2) throw()
    {
        return str2.GetLength() == 1 && str2[0] == ch1;
    }

    CStringT& operator+=(_In_ const CThisSimpleString& str)
    {
        CThisSimpleString::operator+=(str);
        return *this;
    }

    CStringT& operator+=(_In_z_ PCXSTR pszSrc)
    {
        CThisSimpleString::operator+=(pszSrc);
        return *this;
    }

    BOOL LoadString(_In_ UINT nID)
    {
        return LoadString(_AtlBaseModule.GetResourceInstance(), nID);
    }

    _Check_return_ BOOL LoadString(_In_ HINSTANCE hInstance,
                                   _In_ UINT nID)
    {
        const ATLSTRINGRESOURCEIMAGE* pImage = AtlGetStringResourceImage(hInstance, nID);
        if (pImage == NULL) return FALSE;

        int nLength = StringTraits::GetBaseTypeLength(pImage->achString, pImage->nLength);
        PXSTR pszBuffer = CThisSimpleString::GetBuffer(nLength);
        StringTraits::ConvertToBaseType(pszBuffer, nLength, pImage->achString, pImage->nLength);
        CThisSimpleString::ReleaseBufferSetLength(nLength);

        return TRUE;
    }

    BOOL GetEnvironmentVariable(_In_z_ PCXSTR pszVar)
    {
        int nLength = StringTraits::GetEnvironmentVariable(pszVar, NULL, 0);

        if (nLength > 0)
        {
            PXSTR pszBuffer = CThisSimpleString::GetBuffer(nLength);
            StringTraits::GetEnvironmentVariable(pszVar, pszBuffer, nLength);
            CThisSimpleString::ReleaseBuffer();
            return TRUE;
        }

        CThisSimpleString::Empty();
        return FALSE;
    }

    CStringT& MakeLower()
    {
        int nLength = CThisSimpleString::GetLength();
        PXSTR pszBuffer = CThisSimpleString::GetBuffer(nLength);

        StringTraits::MakeLower(pszBuffer, nLength);
        CThisSimpleString::ReleaseBufferSetLength(nLength);

        return *this;
    }

    CStringT& MakeUpper()
    {
        int nLength = CThisSimpleString::GetLength();
        PXSTR pszBuffer = CThisSimpleString::GetBuffer(nLength);

        StringTraits::MakeUpper(pszBuffer, nLength);
        CThisSimpleString::ReleaseBufferSetLength(nLength);

        return *this;
    }

    int Find(_In_ PCXSTR pszSub, _In_opt_ int iStart = 0) const throw()
    {
        int nLength = CThisSimpleString::GetLength();

        if (iStart >= nLength || iStart < 0)
            return -1;

        PCXSTR pszString = CThisSimpleString::GetString();
        PCXSTR pszResult = StringTraits::FindString(pszString + iStart, pszSub);

        return pszResult ? ((int)(pszResult - pszString)) : -1;
    }

    int Find(_In_ XCHAR ch, _In_opt_ int iStart = 0) const throw()
    {
        int nLength = CThisSimpleString::GetLength();

        if (iStart >= nLength || iStart < 0)
            return -1;

        PCXSTR pszString = CThisSimpleString::GetString();
        PCXSTR pszResult = StringTraits::FindChar(pszString + iStart, ch);

        return pszResult ? ((int)(pszResult - pszString)) : -1;
    }

    int FindOneOf(_In_ PCXSTR pszCharSet) const throw()
    {
        PCXSTR pszString = CThisSimpleString::GetString();
        PCXSTR pszResult = StringTraits::FindOneOf(pszString, pszCharSet);

        return pszResult ? ((int)(pszResult - pszString)) : -1;
    }

    int ReverseFind(_In_ XCHAR ch) const throw()
    {
        PCXSTR pszString = CThisSimpleString::GetString();
        PCXSTR pszResult = StringTraits::FindCharReverse(pszString, ch);

        return pszResult ? ((int)(pszResult - pszString)) : -1;
    }

    int Compare(_In_z_ PCXSTR psz) const
    {
        return StringTraits::Compare(CThisSimpleString::GetString(), psz);
    }


    CStringT Mid(int iFirst, int nCount) const
    {
        int nLength = CThisSimpleString::GetLength();

        if (iFirst < 0)
            iFirst = 0;
        if (nCount < 0)
            nCount = 0;
        if (iFirst > nLength)
            iFirst = nLength;
        if (iFirst + nCount > nLength)
            nCount = nLength - iFirst;

        return CStringT(CThisSimpleString::GetString() + iFirst, nCount);
    }

    CStringT Mid(int iFirst) const
    {
        int nLength = CThisSimpleString::GetLength();

        if (iFirst < 0)
            iFirst = 0;
        if (iFirst > nLength)
            iFirst = nLength;

        return CStringT(CThisSimpleString::GetString() + iFirst, nLength - iFirst);
    }

    CStringT Left(int nCount) const
    {
        int nLength = CThisSimpleString::GetLength();

        if (nCount < 0)
            nCount = 0;
        if (nCount > nLength)
            nCount = nLength;

        return CStringT(CThisSimpleString::GetString(), nCount);
    }

    CStringT Right(int nCount) const
    {
        int nLength = CThisSimpleString::GetLength();

        if (nCount < 0)
            nCount = 0;
        if (nCount > nLength)
            nCount = nLength;

        return CStringT(CThisSimpleString::GetString() + nLength - nCount, nCount);
    }


    void __cdecl Format(UINT nFormatID, ...)
    {
        va_list args;
        va_start(args, nFormatID);
        CStringT formatString;
        if (formatString.LoadString(nFormatID))
            FormatV(formatString, args);
        va_end(args);
    }

    void __cdecl Format(PCXSTR pszFormat, ...)
    {
        va_list args;
        va_start(args, pszFormat);
        FormatV(pszFormat, args);
        va_end(args);
    }

    void FormatV(PCXSTR pszFormat, va_list args)
    {
        int nLength = StringTraits::FormatV(NULL, pszFormat, args);

        PXSTR pszBuffer = CThisSimpleString::GetBuffer(nLength);
        StringTraits::FormatV(pszBuffer, pszFormat, args);
        CThisSimpleString::ReleaseBufferSetLength(nLength);
    }


    int Replace(PCXSTR pszOld, PCXSTR pszNew)
    {
        PCXSTR pszString = CThisSimpleString::GetString();

        const int nLength = CThisSimpleString::GetLength();
        const int nOldLen = StringTraits::GetBaseTypeLength(pszOld);
        const int nNewLen = StringTraits::GetBaseTypeLength(pszNew);
        const int nDiff = nNewLen - nOldLen;
        int nResultLength = nLength;

        PCXSTR pszFound;
        while ((pszFound = StringTraits::FindString(pszString, pszOld)))
        {
            nResultLength += nDiff;
            pszString = pszFound + nOldLen;
        }

        if (pszString == CThisSimpleString::GetString())
            return 0;

        PXSTR pszResult = CThisSimpleString::GetBuffer(nResultLength);
        PXSTR pszNext;
        int nCount = 0, nRemaining = nLength;
        while (nRemaining && (pszNext = StringTraits::FindString(pszResult, pszOld)))
        {
            nRemaining -= (pszNext - pszResult);
            nRemaining -= nOldLen;
            if (nRemaining > 0)
                CThisSimpleString::CopyCharsOverlapped(pszNext + nNewLen, nRemaining + 1, pszNext + nOldLen, nRemaining + 1);
            CThisSimpleString::CopyCharsOverlapped(pszNext, nNewLen, pszNew, nNewLen);
            pszResult = pszNext + nNewLen;
            nCount++;
        }

        CThisSimpleString::ReleaseBufferSetLength(nResultLength);

        return nCount;
    }

    int Replace(XCHAR chOld, XCHAR chNew)
    {
        PCXSTR pszString = CThisSimpleString::GetString();
        PXSTR pszFirst = StringTraits::FindChar(pszString, chOld);
        if (!pszFirst)
            return 0;

        int nLength = CThisSimpleString::GetLength();
        int nCount = 0;

        PXSTR pszBuffer = CThisSimpleString::GetBuffer(nLength);
        pszFirst = pszBuffer + (pszFirst - pszString);
        do {
            *pszFirst = chNew;
            ++nCount;
        } while ((pszFirst = StringTraits::FindChar(pszFirst + 1, chOld)));

        CThisSimpleString::ReleaseBufferSetLength(nLength);
        return nCount;
    }


    static PCXSTR DefaultTrimChars()
    {
        static XCHAR str[] = { ' ', '\t', '\r', '\n', 0 };
        return str;
    }


    CStringT& TrimLeft()
    {
        return TrimLeft(DefaultTrimChars());
    }

    CStringT& TrimLeft(XCHAR chTarget)
    {
        XCHAR str[2] = { chTarget, 0 };
        return TrimLeft(str);
    }

    CStringT& TrimLeft(PCXSTR pszTargets)
    {
        int nLength = CThisSimpleString::GetLength();
        PXSTR pszBuffer = CThisSimpleString::GetBuffer(nLength);
        int nCount = 0;

        while (nCount < nLength && StringTraits::FindChar(pszTargets, pszBuffer[nCount]))
            nCount++;

        if (nCount > 0)
        {
            CThisSimpleString::CopyCharsOverlapped(pszBuffer, nLength - nCount, pszBuffer + nCount, nLength - nCount);
            nLength -= nCount;
        }
        CThisSimpleString::ReleaseBufferSetLength(nLength);

        return *this;
    }


    CStringT& TrimRight()
    {
        return TrimRight(DefaultTrimChars());
    }

    CStringT& TrimRight(XCHAR chTarget)
    {
        XCHAR str[2] = { chTarget, 0 };
        return TrimRight(str);
    }

    CStringT& TrimRight(PCXSTR pszTargets)
    {
        int nLength = CThisSimpleString::GetLength();
        PXSTR pszBuffer = CThisSimpleString::GetBuffer(nLength);

        while (nLength > 0 && StringTraits::FindChar(pszTargets, pszBuffer[nLength-1]))
            nLength--;

        CThisSimpleString::ReleaseBufferSetLength(nLength);

        return *this;
    }


    CStringT& Trim()
    {
        return Trim(DefaultTrimChars());
    }

    CStringT& Trim(XCHAR chTarget)
    {
        XCHAR str[2] = { chTarget, 0 };
        return Trim(str);
    }

    CStringT& Trim(PCXSTR pszTargets)
    {
        return TrimRight(pszTargets).TrimLeft(pszTargets);
    }


};

} //namespace ATL

#endif