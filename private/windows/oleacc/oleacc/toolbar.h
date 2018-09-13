// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  TOOLBAR.H
//
//  This communicates with COMCTL32's tool bar control.
//
// --------------------------------------------------------------------------


class CToolBar32 : public CClient
{
    public:
        // IAccessible
        STDMETHODIMP    get_accName(VARIANT, BSTR*);
        STDMETHODIMP    get_accRole(VARIANT, VARIANT*);
        STDMETHODIMP    get_accState(VARIANT, VARIANT*);
        STDMETHODIMP    get_accKeyboardShortcut(VARIANT, BSTR*);
        STDMETHODIMP    get_accDefaultAction(VARIANT, BSTR*);

        STDMETHODIMP    accLocation(long*, long*, long*, long*, VARIANT);
        STDMETHODIMP    accNavigate(long, VARIANT, VARIANT*);
        STDMETHODIMP    accHitTest(long, long, VARIANT*);
        STDMETHODIMP    accDoDefaultAction(VARIANT);

        // IEnumVARIANT
        STDMETHODIMP    Next(ULONG celt, VARIANT *rgvar, ULONG* pceltFetched);

        // constructor
        CToolBar32(HWND, long);

        // misc. methods
        BOOL                GetItemData(int, LPTBBUTTON);
        void                SetupChildren();

    private:
        STDMETHODIMP    GetToolbarString(int ChildId, LPTSTR* ppszName);
};

