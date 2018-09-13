// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  CARET.H
//
//  Caret OLE Accessibility implementation
//
// --------------------------------------------------------------------------


class   CCaret :    public  CAccessible
{
    public:
        // IAccessible
        STDMETHODIMP    get_accName(VARIANT varChild, BSTR * pszName);
        STDMETHODIMP    get_accRole(VARIANT varChild, VARIANT * lpRole);
        STDMETHODIMP    get_accState(VARIANT varChild, VARIANT * lpState);
        STDMETHODIMP    accLocation(long* pxLeft, long* pyTop,
            long* pcxWidth, long* pcyHeight, VARIANT varChild);
        STDMETHODIMP    accHitTest(long xLeft, long yTop, VARIANT* pvarChild);

        // IEnumVARIANT
        STDMETHODIMP    Clone(IEnumVARIANT** ppenum);

        BOOL            FInitialize(HWND hwnd);

    private:
        DWORD           m_dwThreadId;
};


HRESULT     CreateCaretThing(HWND, REFIID, void**);
