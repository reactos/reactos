// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  STATIC.H
//
//  Static object
//
// --------------------------------------------------------------------------

class   CStatic :   public CClient
{
    public:
        // IAccessible
        STDMETHODIMP    get_accRole(VARIANT varChild, VARIANT *pvarRole);
        STDMETHODIMP    get_accKeyboardShortcut(VARIANT, BSTR*);
		STDMETHODIMP	get_accState(VARIANT varChild, VARIANT *pvarState);

        CStatic(HWND, long);

    protected:
        BOOL    m_fGraphic;
};


