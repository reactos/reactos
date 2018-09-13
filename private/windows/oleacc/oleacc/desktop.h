// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  DESKTOP.H
//
//  (Real not shell) Desktop client support
//
// --------------------------------------------------------------------------


class CDesktop : public CClient
{
    public:
        // IAccessible
        virtual STDMETHODIMP        get_accName(VARIANT, BSTR*);
        virtual STDMETHODIMP        get_accFocus(VARIANT*);
        virtual STDMETHODIMP        get_accSelection(VARIANT*);

        CDesktop(HWND, long);
};


