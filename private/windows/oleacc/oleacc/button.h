// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  BUTTON.H
//
//  Button object
//
// --------------------------------------------------------------------------

class   CButton :   public CClient
{
    public:
        // IAccessible
        STDMETHODIMP    get_accName(VARIANT varChild, BSTR* pszName);
        STDMETHODIMP    get_accRole(VARIANT varChild, VARIANT *pvarRole);
        STDMETHODIMP    get_accState(VARIANT varChild, VARIANT *pvarState);
        STDMETHODIMP    get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction);
		STDMETHODIMP	get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut);
		STDMETHODIMP	get_accChildCount(long *pcCount);
		STDMETHODIMP	get_accChild(VARIANT varChild, IDispatch ** ppdispChild);
		STDMETHODIMP	accNavigate(long dwNavDir, VARIANT varStart, VARIANT * pvarEnd);
        STDMETHODIMP    accDoDefaultAction(VARIANT varChild);
		// IEnumVariant
		STDMETHODIMP	Next(ULONG celt, VARIANT *rgvar, ULONG* pceltFetched);
		STDMETHODIMP	Skip(ULONG celt);

		//Helpers
		void SetupChildren(void);

        CButton(HWND, long);
};

