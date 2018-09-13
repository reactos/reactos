// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  OUTLINE.H
//
//  Knows how to talk to COMCTL32's TreeView control.
//
//  NOTE:  The VALUE is the indent level.  This lets us treat the children
//  like peers (otherwise, the elements of a node would be children but not
//  contained, really weird).
//
//  NOTE:  The child id is the HTREEITEM.  There is no index support for
//  treeview.  Hence we must do our own validation and IEnumVARIANT handling.
//
// --------------------------------------------------------------------------


class COutlineView32 :  public CClient
{
    public:
        // IAccessible
        STDMETHODIMP        get_accName(VARIANT, BSTR*);
        STDMETHODIMP        get_accValue(VARIANT, BSTR*);
        STDMETHODIMP        get_accRole(VARIANT, VARIANT*);
        STDMETHODIMP        get_accState(VARIANT, VARIANT*);
        STDMETHODIMP        get_accFocus(VARIANT*);
        STDMETHODIMP        get_accSelection(VARIANT*);
        STDMETHODIMP        get_accDefaultAction(VARIANT, BSTR*);

        STDMETHODIMP        accSelect(long, VARIANT);
        STDMETHODIMP        accLocation(long*, long*, long*, long*, VARIANT);
        STDMETHODIMP        accNavigate(long, VARIANT, VARIANT*);
        STDMETHODIMP        accHitTest(long, long, VARIANT*);
        STDMETHODIMP        accDoDefaultAction(VARIANT varChild);

        // IEnumVARIANT
        STDMETHODIMP        Next(ULONG, VARIANT*, ULONG*);
        STDMETHODIMP        Skip(ULONG);
        STDMETHODIMP        Reset(void);

        HTREEITEM   NextLogicalItem(HTREEITEM, BOOL);
        
        BOOL        ValidateChild(VARIANT*);
        void        SetupChildren(void);
        COutlineView32(HWND, long);
};


