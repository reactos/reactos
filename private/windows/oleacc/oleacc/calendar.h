// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  CALENDAR.H
//
//  Knows how to talk to COMCTL32's calendar and date-picker controls.
//
// --------------------------------------------------------------------------


class CCalendar : public CClient
{
    public:
        // IAccessible
        STDMETHODIMP        get_accName(VARIANT, BSTR*);
        STDMETHODIMP        get_accRole(VARIANT, VARIANT*);
        STDMETHODIMP        get_accState(VARIANT, VARIANT*);
        STDMETHODIMP        get_accFocus(VARIANT*);
        STDMETHODIMP        get_accSelection(VARIANT*);

        STDMETHODIMP        accSelect(long, VARIANT);
        STDMETHODIMP        accLocation(long*, long*, long*, long*, VARIANT);
        STDMETHODIMP        accNavigate(long, VARIANT, VARIANT*);
        STDMETHODIMP        accHitTest(long, long, VARIANT*);

        void SetupChildren(void);
        CCalendar(HWND, long);
};

