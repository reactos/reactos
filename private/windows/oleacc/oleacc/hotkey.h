// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  HOTKEY.H
//
//  This knows how to talk to COMCTL32's hotkey control.
//
// --------------------------------------------------------------------------


class CHotKey32 : public CClient
{
    public:
        // IAccessible
        virtual STDMETHODIMP    get_accRole(VARIANT, VARIANT*);
        virtual STDMETHODIMP    get_accValue(VARIANT, BSTR*);

        CHotKey32(HWND, long);
};


