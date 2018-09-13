// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  HEADER.H
//
//  Knows how to talk to COMCTL32's header control
//
// --------------------------------------------------------------------------

class CHeader32 : public CClient
{
    public:
        // IAccessible
        STDMETHODIMP    get_accName(VARIANT, BSTR*);
        STDMETHODIMP    get_accRole(VARIANT, VARIANT*);
		STDMETHODIMP	get_accState(VARIANT, VARIANT*);
        STDMETHODIMP    get_accDefaultAction(VARIANT, BSTR*);

        STDMETHODIMP    accDoDefaultAction(VARIANT);
        STDMETHODIMP    accLocation(long*, long*, long*, long*, VARIANT);
        STDMETHODIMP    accNavigate(long, VARIANT, VARIANT*);
        STDMETHODIMP    accHitTest(long, long, VARIANT*);

        CHeader32(HWND, long);
        void        SetupChildren(void);
};
