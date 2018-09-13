// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  SCROLLBAR.H
//
//  Scrollbar ole accessibility implementation
//
// --------------------------------------------------------------------------


//
// Scrollbar
//
class   CScrollBar : public CAccessible
{
    public:
        // IAccessible
        STDMETHODIMP            get_accName(VARIANT varChild, BSTR* pszName);
        STDMETHODIMP            get_accValue(VARIANT varChild, BSTR* pszValue);
        STDMETHODIMP            get_accDescription(VARIANT varChild, BSTR * pszDescription);
        STDMETHODIMP            get_accRole(VARIANT varChild, VARIANT *pvarRole);
        STDMETHODIMP            get_accState(VARIANT varChild, VARIANT *pvarState);
        STDMETHODIMP			get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction);

        STDMETHODIMP			accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild);
        STDMETHODIMP			accNavigate(long navDir, VARIANT varChild, VARIANT* pvarEndUpAt);
        STDMETHODIMP			accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint);
        STDMETHODIMP            accDoDefaultAction(VARIANT varChild);

        STDMETHODIMP			put_accValue(VARIANT varChild, BSTR pszValue);

        // IEnumVARIANT
        STDMETHODIMP            Clone(IEnumVARIANT** ppenum);

        // This is virtual, since each type of sys object implements this.
        BOOL                    FInitialize(HWND hwnd, LONG idObject, LONG iChildCur);

    protected:
        BOOL    m_fVertical;      // Vertical or horizontal
};



//
// Scrollbar control
//
class   CScrollCtl : public CClient
{
    public:
        // IAccessible
        STDMETHODIMP            get_accName(VARIANT varChild, BSTR* pszName);
        STDMETHODIMP            get_accValue(VARIANT varChild, BSTR* pszValue);
        STDMETHODIMP            get_accDescription(VARIANT varChild, BSTR * pszDescription);
        STDMETHODIMP            get_accRole(VARIANT varChild, VARIANT *pvarRole);
        STDMETHODIMP            get_accState(VARIANT varChild, VARIANT *pvarState);
        STDMETHODIMP			get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction);

        STDMETHODIMP			accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild);
        STDMETHODIMP			accNavigate(long navDir, VARIANT varChild, VARIANT* pvarEndUpAt);
        STDMETHODIMP			accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint);
        STDMETHODIMP            accDoDefaultAction(VARIANT varChild);

        STDMETHODIMP			put_accValue(VARIANT varChild, BSTR pszValue);

        // This is virtual, since each type of sys object implements this.
        CScrollCtl(HWND, long);

    protected:
        BOOL    m_fGrip;            // Sizebox instead of bar
        BOOL    m_fVertical;        // Vertical or horizontal
};



//
// Size grip
//
class   CSizeGrip : public CAccessible
{
    public:
        // IAccessible
        STDMETHODIMP            get_accName(VARIANT varChild, BSTR * pszNaem);
        STDMETHODIMP            get_accDescription(VARIANT varChild, BSTR * pszDesc);
        STDMETHODIMP            get_accRole(VARIANT varChild, VARIANT * pvarRole);
        STDMETHODIMP            get_accState(VARIANT varChild, VARIANT * pvarState);
        
        STDMETHODIMP            accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild);
        STDMETHODIMP            accNavigate(long dwNavFlags, VARIANT varStart, VARIANT * pvarEnd);
        STDMETHODIMP            accHitTest(long xLeft, long yTop, VARIANT * pvarHit);

        // IEnumVARIANT
        STDMETHODIMP            Clone(IEnumVARIANT * * ppenum);

        BOOL                    FInitialize(HWND hwnd);
};


HRESULT CreateScrollBarThing(HWND hwnd, LONG idObj, LONG iItem, REFIID riid, void** ppvScroll);
void            FixUpScrollBarInfo(LPSCROLLBARINFO);

HRESULT CreateSizeGripThing(HWND hwnd, LONG idObj, REFIID riid, void** ppvObject);
