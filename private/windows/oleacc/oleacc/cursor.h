// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  CURSOR.H
//
//  Cursor OLE Accessibility implementation
//
// --------------------------------------------------------------------------


class   CCursor :   public  CAccessible
{
    public:
        // IAccessible
        STDMETHODIMP        get_accName(VARIANT varChild, BSTR * pszName);
        STDMETHODIMP        get_accRole(VARIANT varChild, VARIANT * lpRole);
        STDMETHODIMP        get_accState(VARIANT varChild, VARIANT * lpRole);
        STDMETHODIMP        accLocation(long* pxLeft, long* pyTop,
            long* pcxWidth, long* pcyHeight, VARIANT varChild);
        STDMETHODIMP        accHitTest(long xLeft, long yTop, VARIANT * pvarChild);

        // IEnumVARIANT
        STDMETHODIMP        Clone(IEnumVARIANT * * ppenum);
};


long    MapCursorIndex(HCURSOR hCur);
HRESULT CreateCursorThing(REFIID, void**);
