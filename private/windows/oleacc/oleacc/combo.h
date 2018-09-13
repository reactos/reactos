// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  COMBO.H
//
//  Combobox object
//
// --------------------------------------------------------------------------

//
// NOTE:
// Since a combobox has a known # of children (elements and objects), we
// can simplify our life a lot by not deferring to CClient for things like
// the child count property.  We accept IDs for all relevant properties
// even if the ID is of a child object.
//
class   CCombo: public CClient
{
    public:
        // IAccessible
        virtual STDMETHODIMP        get_accChildCount(long* pcCount);
        virtual STDMETHODIMP        get_accChild(VARIANT, IDispatch**);

        virtual STDMETHODIMP        get_accName(VARIANT, BSTR*);
        virtual STDMETHODIMP        get_accValue(VARIANT, BSTR*);
        virtual STDMETHODIMP        get_accRole(VARIANT, VARIANT*);
        virtual STDMETHODIMP        get_accState(VARIANT, VARIANT*);
        virtual STDMETHODIMP        get_accKeyboardShortcut(VARIANT, BSTR*);
        virtual STDMETHODIMP        get_accDefaultAction(VARIANT, BSTR*);

        virtual STDMETHODIMP        accLocation(long*, long*, long*, long*, VARIANT);
        virtual STDMETHODIMP        accNavigate(long, VARIANT, VARIANT*);
        virtual STDMETHODIMP        accHitTest(long, long, VARIANT*);
        virtual STDMETHODIMP        accDoDefaultAction(VARIANT);

        virtual STDMETHODIMP        put_accValue(VARIANT, BSTR);

        // IEnumVARIANT
        STDMETHODIMP        Next(ULONG, VARIANT*, ULONG*);
        STDMETHODIMP        Skip(ULONG);

        CCombo(HWND, long);

    private:
        BOOL    m_fHasButton:1;
        BOOL    m_fHasEdit:1;
};
