// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  TITLEBAR.H
//
//  Titlebar ole accessibility implementation
//
// --------------------------------------------------------------------------

//
// BOGUS!  Do we implement QueryInterface() and respond to ITextDocument etc.
// if OSM is around?
//

class   CTitleBar :   public  CAccessible
{
    public:
        // IAccessible
        STDMETHODIMP        get_accName(VARIANT varChild, BSTR * pszName);
        STDMETHODIMP        get_accValue(VARIANT, BSTR*);
        STDMETHODIMP        get_accDescription(VARIANT varChild, BSTR * pszDesc);
        STDMETHODIMP        get_accRole(VARIANT varChild, VARIANT * lpRole);
        STDMETHODIMP        get_accState(VARIANT varChild, VARIANT * lpRole);
        STDMETHODIMP        get_accDefaultAction(VARIANT varChild, BSTR * pszDefAction);

        STDMETHODIMP        accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
            long* pcyHeight, VARIANT varChild);
        STDMETHODIMP        accNavigate(long narDir, VARIANT varStart,
            VARIANT * pvarEndUpAt);
        STDMETHODIMP        accHitTest(long xLeft, long yTop, VARIANT * pvarChild);
        STDMETHODIMP        accDoDefaultAction(VARIANT varChild);
		STDMETHODIMP		accSelect(long flagsSel, VARIANT varChild);

        // IEnumVARIANT
        STDMETHODIMP        Clone(IEnumVARIANT** ppenum);

        BOOL                FInitialize(HWND hwnd, LONG iChildCur);
};


//
// Helper functions
//
HRESULT     CreateTitleBarThing(HWND hwnd, long idObject, REFIID riid, void** ppvObject);
long        GetRealChild(DWORD dwStyle, LONG lChild);
