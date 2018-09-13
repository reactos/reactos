// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  TABCTRL.H
//
//  Knows how to talk to COMCTL32's tab control
//
// --------------------------------------------------------------------------

class CTabControl32 : public CClient
{
    public:
        // IAccessible
        virtual STDMETHODIMP    get_accName(VARIANT, BSTR*);
        virtual STDMETHODIMP    get_accRole(VARIANT, VARIANT*);
        virtual STDMETHODIMP    get_accState(VARIANT, VARIANT*);
        virtual STDMETHODIMP    get_accKeyboardShortcut(VARIANT, BSTR*);
        virtual STDMETHODIMP    get_accFocus(VARIANT*);
        virtual STDMETHODIMP    get_accSelection(VARIANT*);
        virtual STDMETHODIMP    get_accDefaultAction(VARIANT, BSTR*);

        virtual STDMETHODIMP    accSelect(long, VARIANT);
        virtual STDMETHODIMP    accLocation(long*, long*, long*, long*, VARIANT);
        virtual STDMETHODIMP    accNavigate(long, VARIANT, VARIANT*);
        virtual STDMETHODIMP    accHitTest(long, long, VARIANT*);
        virtual STDMETHODIMP    accDoDefaultAction(VARIANT);

        // constructor
        CTabControl32(HWND, long);

        // other methods
        void                    SetupChildren(void);

    private:
        STDMETHODIMP            GetTabControlString(int ChildIndex,LPTSTR* ppszName);
};



//
// cbExtra for the tray is 8, so pick something even bigger, 16
//
#define CBEXTRA_TRAYTAB     16
