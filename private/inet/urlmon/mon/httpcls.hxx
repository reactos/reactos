//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       HTTPCLS.HXX
//
//  Contents:   Implements the HTTPMoniker class, which is derived from
//              the CAsyncMoniker class.
//
//  Classes:    HTTPMoniker
//
//  Functions:
//
//  History:    11-02-95   JoeS (Joe Souza)     Created
//
//----------------------------------------------------------------------------

class FAR HTTPMoniker : public CAsyncMoniker
{
public:
        // *** IUnknown methods
        STDMETHOD_(ULONG, Release)(void);

        // *** IPersist methods
        STDMETHOD(GetClassID)(CLSID *pClassID);

        // *** IMoniker methods ***
        STDMETHOD(BindToStorage) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
                REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
                LPWSTR lpszDisplayName, ULONG FAR* pchEaten,
                LPMONIKER FAR* ppmkOut);

private:
        ULONG   m_refs;
        LPWSTR  m_pwzURL;   // the url string

protected:
};

