// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  UPDOWN.H
//
//  This knows how to talk to COMCTL32's updown control
//
// --------------------------------------------------------------------------


class CUpDown32 : public CClient
{
    public:
        // IAccessible
        // BOGUS!  No way to do default action support!
        //         Or button state.  Need COMCTL32 help.
        STDMETHODIMP        get_accName(VARIANT, BSTR*);
        STDMETHODIMP        get_accValue(VARIANT, BSTR*);
        STDMETHODIMP        get_accRole(VARIANT, VARIANT*);

        STDMETHODIMP        accLocation(long*, long*, long*, long*, VARIANT);
        STDMETHODIMP        accNavigate(long, VARIANT, VARIANT*);
        STDMETHODIMP        accHitTest(long, long, VARIANT*);

        STDMETHODIMP        put_accValue(VARIANT, BSTR);

        CUpDown32(HWND, long);

    protected:
        BOOL    m_fVertical;
};


#define INDEX_UPDOWN_SELF       0
#define INDEX_UPDOWN_UPLEFT     1
#define INDEX_UPDOWN_DNRIGHT    2
#define CCHILDREN_UPDOWN        2


