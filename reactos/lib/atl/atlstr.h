#ifndef __ATLSTR_H__
#define __ATLSTR_H__

#pragma once

#ifndef __cplusplus
    #error ATL requires C++ compilation (use a .cpp suffix)
#endif

#include <atlbase.h>
#include <cstringt.h>

namespace ATL
{
    template< typename _BaseType = char, class StringIterator = ChTraitsOS<_BaseType>>
    class StrTraitATL :
        public StringIterator
    {
    public:
        static HINSTANCE FindStringResourceInstance(_In_ UINT nID) throw()
        {
            return(AtlFindStringResourceInstance(nID));
        }

        static IAtlStringMgr* GetDefaultManager() throw()
        {
            return CAtlStringMgr::GetInstance();
        }
    };


    template< typename _CharType = wchar_t>
    class ChTraitsOS :
        public ChTraitsBase<_CharType>
    {
    protected:

    public:

    };

#ifndef _ATL_CSTRING_NO_CRT
    typedef CStringT<wchar_t, StrTraitATL<wchar_t, ChTraitsCRT<wchar_t>>> CAtlStringW;
#else
    typedef CStringT<wchar_t, StrTraitATL<wchar_t>> CAtlStringW;
#endif

#ifndef _AFX
    typedef CAtlStringW CStringW;
#endif


} //namespace ATL

#endif