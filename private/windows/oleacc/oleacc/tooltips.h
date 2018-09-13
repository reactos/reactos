// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  TOOLTIPS.H
//
//  Knows how to talk to COMCTL32's tooltips.
//
// --------------------------------------------------------------------------


class CToolTips32 : public CClient
{
    public:
        // IAccessible
        STDMETHODIMP    get_accRole(VARIANT varChild, VARIANT* pvarRole);

        CToolTips32(HWND, long);
};
