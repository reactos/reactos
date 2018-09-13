//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       padframe.cxx
//
//  Contents:   CPadFrame class.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

IMPLEMENT_SUBOBJECT_IUNKNOWN(CPadFrame, CPadDoc, PadDoc, _Frame);

STDMETHODIMP
CPadFrame::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (iid == IID_IOleInPlaceFrame ||
        iid == IID_IOleWindow ||
        iid == IID_IOleInPlaceUIWindow ||
        iid == IID_IUnknown)
    {
        *ppv = (IOleInPlaceFrame *) this;
        AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}


STDMETHODIMP
CPadFrame::GetWindow(HWND * phWnd)
{
    *phWnd = PadDoc()->_hwnd;
    return S_OK;
}


STDMETHODIMP
CPadFrame::ContextSensitiveHelp(BOOL fEnterMode)
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CPadFrame::GetBorder(LPOLERECT prcBorder)
{
    RECT rc;
    PadDoc()->GetViewRect(&rc, FALSE);
    CopyRect(prcBorder, &rc);
    return S_OK;
}


STDMETHODIMP
CPadFrame::RequestBorderSpace(LPCBORDERWIDTHS pbw)
{
    // Always try to grant the space, delay the actual BorderSpace checks to
    // SetBorderSpace.

    RRETURN(S_OK);
}


STDMETHODIMP
CPadFrame::SetBorderSpace(LPCBORDERWIDTHS pbw)
{
    RECT         rcCurrentSize;
    BORDERWIDTHS bw;

    if (pbw == NULL) // in-place object is not interested in tools.
    {
        memset(&bw, 0, sizeof(bw));
        pbw = &bw;
    }

    PadDoc()->GetViewRect(&rcCurrentSize, TRUE);

    // If the requested border space is larger than the current window size, no space can be given.

    if (((rcCurrentSize.bottom - rcCurrentSize.top) <= (pbw->top + pbw->bottom)) ||
        ((rcCurrentSize.right - rcCurrentSize.left) <= (pbw->left + pbw->right)))
    {
        RRETURN(INPLACE_E_NOTOOLSPACE);
    }

    // Record the requested in-place toolbar border space and re-set the client window area size.

    PadDoc()->_bwToolbarSpace = *pbw;
    PadDoc()->Resize(); // We must call this here for the doc-obj to resize itself.

    RRETURN(S_OK);
}


STDMETHODIMP
CPadFrame::SetActiveObject(
        LPOLEINPLACEACTIVEOBJECT    pActiveObj,
        LPCTSTR                     pstrObjName)
{
    ReplaceInterface(&PadDoc()->_pInPlaceActiveObject, pActiveObj);
    return S_OK;
}


STDMETHODIMP
CPadFrame::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS pmgw)
{
    return PadDoc()->InsertMenus(hmenuShared, pmgw);
}


STDMETHODIMP
CPadFrame::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    return S_OK;
}


STDMETHODIMP
CPadFrame::RemoveMenus(HMENU hmenuShared)
{
    while (GetMenuItemCount(hmenuShared) > 0)
    {
        RemoveMenu(hmenuShared, 0, MF_BYPOSITION);
    }

    return S_OK;
}


STDMETHODIMP
CPadFrame::SetStatusText(LPCTSTR szStatusText)
{
    PadDoc()->SetStatusText(szStatusText);
    return S_OK;
}


STDMETHODIMP
CPadFrame::EnableModeless(BOOL fEnable)
{
    //  BUGBUG should probably disable ourselves?
    return S_OK;
}


STDMETHODIMP
CPadFrame::TranslateAccelerator(LPMSG pmsg, WORD wID)
{
    return S_FALSE;
}

