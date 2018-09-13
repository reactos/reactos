// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  ANIMATED.H
//
// --------------------------------------------------------------------------

class   CAnimation : public CClient
{        
    public:
        // IAccessible
        STDMETHODIMP    get_accRole(VARIANT varChild, VARIANT* pvarRole);
        STDMETHODIMP    get_accState(VARIANT varChild, VARIANT* pvarState);

        CAnimation(HWND, long);
};

