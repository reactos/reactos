// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  LISTBOX.H
//
//  Default listbox OLE ACC Client
//
// --------------------------------------------------------------------------


class CListBox : public CClient
{
    public:
        // IAccessible
        STDMETHODIMP        get_accName(VARIANT varChild, BSTR *pszName);
        STDMETHODIMP        get_accRole(VARIANT varChild, VARIANT* pvarRole);
        STDMETHODIMP        get_accState(VARIANT varChild, VARIANT* pvarState);
        STDMETHODIMP        get_accKeyboardShortcut(VARIANT, BSTR*);
        STDMETHODIMP        get_accFocus(VARIANT * pvarFocus);
        STDMETHODIMP        get_accSelection(VARIANT *pvarSelection);
        STDMETHODIMP        get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction);

        STDMETHODIMP        accDoDefaultAction(VARIANT varChild);
        STDMETHODIMP        accSelect(long flagsSel, VARIANT varChild);
        STDMETHODIMP        accLocation(long* pxLeft, long *pyTop, long *pcxWidth,
            long *pcyHeight, VARIANT varChild);
        STDMETHODIMP        accNavigate(long dwNavDir, VARIANT varStart, VARIANT *pvarEnd);
        STDMETHODIMP        accHitTest(long xLeft, long yTop, VARIANT *pvarHit);


        void SetupChildren(void);
        CListBox(HWND, long);

    protected:
        BOOL    m_fComboBox;
        BOOL    m_fDropDown;
};


class CListBoxFrame : public CWindow
{
    public:
        // IAccessible
        STDMETHODIMP        get_accParent(IDispatch **ppdispParent);
        STDMETHODIMP        get_accState(VARIANT varStart, VARIANT* pvarState);
        STDMETHODIMP        accNavigate(VARIANT varStart, long dwNavDir, VARIANT* pvarEnd);

        CListBoxFrame(HWND, long);

    protected:
        BOOL    m_fComboBox;
        BOOL    m_fDropDown;
};



// --------------------------------------------------------------------------
//
//  Although CListBoxSelection() is based off of CAccessibleObject, it only
//  supports IDispatch and IEnumVARIANT.  It will hand back the proper IDs
//  so you can pass them to the real listbox parent object.
//
// --------------------------------------------------------------------------
class CListBoxSelection : public IEnumVARIANT
{
    public:
        // IUnknown
        virtual STDMETHODIMP            QueryInterface(REFIID, void**);
        virtual STDMETHODIMP_(ULONG)    AddRef(void);
        virtual STDMETHODIMP_(ULONG)    Release(void);

        // IEnumVARIANT
        virtual STDMETHODIMP            Next(ULONG celt, VARIANT* rgvar, ULONG * pceltFetched);
        virtual STDMETHODIMP            Skip(ULONG celt);
        virtual STDMETHODIMP            Reset(void);
        virtual STDMETHODIMP            Clone(IEnumVARIANT ** ppenum);

        CListBoxSelection(int, int, LPINT);
        ~CListBoxSelection();

    protected:
        int     m_cRef;
        int     m_idChildCur;
        int     m_cSelected;
        LPINT   m_lpSelected;
};


extern HRESULT GetListBoxSelection(HWND hwnd, VARIANT * pvarSelection);

