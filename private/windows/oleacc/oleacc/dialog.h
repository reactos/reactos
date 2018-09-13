// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  DIALOG.H
//
//  Dialog object
//
// --------------------------------------------------------------------------

class   CDialog :   public CClient
{
    public:
        // IAccessible
        STDMETHODIMP    get_accRole(VARIANT varChild, VARIANT* pvarRole);
        STDMETHODIMP    get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction);
        STDMETHODIMP    accDoDefaultAction(VARIANT varChild);
        
        CDialog(HWND, long);
};


