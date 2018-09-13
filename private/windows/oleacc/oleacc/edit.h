// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  EDIT.H
//
//  Edit field
//
// --------------------------------------------------------------------------

class   CEdit : public CClient
{
    public:
        // IAccessible
        virtual STDMETHODIMP        get_accName(VARIANT, BSTR*);
        virtual STDMETHODIMP        get_accValue(VARIANT, BSTR*);
        virtual STDMETHODIMP        get_accRole(VARIANT, VARIANT*);
        virtual STDMETHODIMP        get_accState(VARIANT, VARIANT*);
        virtual STDMETHODIMP        get_accKeyboardShortcut(VARIANT, BSTR*);
        virtual STDMETHODIMP        put_accValue(VARIANT, BSTR);

        CEdit(HWND, long);
};
