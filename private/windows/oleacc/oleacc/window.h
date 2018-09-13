// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  WINDOW.H
//
//  Default window OLE accessible object class
//
// --------------------------------------------------------------------------

class CWindow : public CAccessible
{
    public:
        // IAccessible
        virtual STDMETHODIMP    get_accParent(IDispatch ** ppdispParent);
        virtual STDMETHODIMP    get_accChild(VARIANT varChildIndex, IDispatch ** ppdispChild);

        virtual STDMETHODIMP    get_accName(VARIANT varChild, BSTR* pszName);
        virtual STDMETHODIMP    get_accDescription(VARIANT varChild, BSTR* pszDescription);
        virtual STDMETHODIMP    get_accRole(VARIANT varChild, VARIANT * pvarRole);
        virtual STDMETHODIMP    get_accState(VARIANT varChild, VARIANT *pvarState);
        virtual STDMETHODIMP    get_accHelp(VARIANT varChild, BSTR* pszHelp);
        virtual STDMETHODIMP    get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut);
        virtual STDMETHODIMP    get_accFocus(VARIANT * pvarFocusChild);

        virtual STDMETHODIMP    accSelect(long flags, VARIANT varChild);
        virtual STDMETHODIMP    accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild);
        virtual STDMETHODIMP    accNavigate(long navDir, VARIANT varStart, VARIANT* pvarEndUpAt);
        virtual STDMETHODIMP    accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint);

        // IEnumVARIANT
        virtual STDMETHODIMP    Next(ULONG celt, VARIANT* rgvar, ULONG * pceltFetched);
        virtual STDMETHODIMP    Clone(IEnumVARIANT * *);

        void    Initialize(HWND, long);

        //
        // NOTE:  We override the default implementation of ValidateChild()!
        //
        virtual BOOL ValidateChild(VARIANT*);
};


//
// Version defines
//
#define VER30   0x0300
#define VER31   0x030A
#define VER40   0x0400
#define VER41   0x040A

#define ObjidFromIndex(index)       (DWORD)(0 - (LONG)(index))
#define IndexFromObjid(objid)       (-(long)(objid))

extern HRESULT  CreateWindowThing(HWND hwnd, long iChild, REFIID riid, void** ppvObjct);
extern HRESULT  FrameNavigate(HWND, long, long, VARIANT *);
