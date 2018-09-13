//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       xicon.hxx
//
//  Contents:   Declaration of CSharesEI & CSharesEIA, implementations
//              of IExtractIcon
//
//  History:    14-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __XICON_HXX__
#define __XICON_HXX__

//////////////////////////////////////////////////////////////////////////////

class CSharesEI : public IExtractIcon
{
public:

    CSharesEI(IN BYTE bFlags, IN DWORD type) : m_ulRefs(0), m_bFlags(bFlags), m_dwType(type) { AddRef(); }
    ~CSharesEI() {}

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    //
    // IExtractIcon methods
    //

    STDMETHOD(GetIconLocation)(
        UINT   uFlags,
        LPWSTR szIconFile,
        UINT   cchMax,
        int*   piIndex,
        UINT*  pwFlags
        );

    STDMETHOD(Extract)(
        LPCWSTR pszFile,
        UINT    nIconIndex,
        HICON*  phiconLarge,
        HICON*  phiconSmall,
        UINT    nIconSize
        );

private:

    ULONG m_ulRefs;
    BYTE  m_bFlags;
	DWORD m_dwType;
};


#ifdef UNICODE

class CSharesEIA : public IExtractIconA
{
public:

    CSharesEIA(IN BYTE bFlags, IN DWORD type) : m_ulRefs(0), m_bFlags(bFlags), m_dwType(type) { AddRef(); }
    ~CSharesEIA() {}

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    //
    // IExtractIconA methods
    //

    STDMETHOD(GetIconLocation)(
        UINT   uFlags,
        LPSTR  szIconFile,
        UINT   cchMax,
        int*   piIndex,
        UINT*  pwFlags
        );

    STDMETHOD(Extract)(
        LPCSTR  pszFile,
        UINT    nIconIndex,
        HICON*  phiconLarge,
        HICON*  phiconSmall,
        UINT    nIconSize
        );

private:

    ULONG m_ulRefs;
    BYTE  m_bFlags;
	DWORD m_dwType;
};

#endif // UNICODE

#endif // __XICON_HXX__
