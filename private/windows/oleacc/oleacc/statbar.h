// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  STATBAR.H
//
//  This communicates with COMCTL32's status bar control.
//
// --------------------------------------------------------------------------


class CStatusBar32 : public CClient
{
    public:
        // IAccessible
        virtual STDMETHODIMP    get_accName(VARIANT, BSTR*);
        virtual STDMETHODIMP    get_accRole(VARIANT, VARIANT*);
        virtual STDMETHODIMP    get_accState(VARIANT, VARIANT*);

        virtual STDMETHODIMP    accLocation(long*, long*, long*, long*, VARIANT);
        virtual STDMETHODIMP    accNavigate(long, VARIANT, VARIANT*);
        virtual STDMETHODIMP    accHitTest(long, long, VARIANT*);

        CStatusBar32(HWND, long);
        void                SetupChildren(void);
        long                ConvertHwndToID (long HwndID);
        long                FindChildWindowFromID (long ID);

};


