// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  PROGRESS.H
//
// --------------------------------------------------------------------------

class   CProgressBar :     public CClient
{
    public:
        // IAccessible
        STDMETHODIMP    get_accRole(VARIANT varChild, VARIANT* pvarRole);
        STDMETHODIMP    get_accValue(VARIANT varChild, BSTR* pszValue);

        CProgressBar(HWND, long);
};


