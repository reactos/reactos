// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  MDICLI.H
//
//  MDI Client support
//
// --------------------------------------------------------------------------


class CMdiClient : public CClient
{
    public:
        // IAccessible
        virtual STDMETHODIMP        get_accName(VARIANT, BSTR*);
        virtual STDMETHODIMP        get_accFocus(VARIANT*);
        virtual STDMETHODIMP        get_accSelection(VARIANT*);

        CMdiClient(HWND, long);
};

