//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       scrsbobj.cxx
//
//  History:    19-Jan-1998     sramani     Created
//
//  Contents:   CScriptletSubObjects: CScriptlet subobject implementation for 
//              IDocHostUIHandler and IPropertyNotifySink Support
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_SCRSBOBJ_HXX_
#define X_SCRSBOBJ_HXX_
#include "scrsbobj.hxx"
#endif

#ifndef X_SCRPTLET_HXX_
#define X_SCRPTLET_HXX_
#include "scrptlet.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

MtDefine(CScriptletSubObjects, Scriptlet, "CScriptletSubObjects")
MtDefine(CScriptletSubObjects_aryDispid_pv, CScriptletSubObjects, "CScriptletSubObjects::_aryDispid::_pv")
MtDefine(CScriptletSubObjectsSetContextMenu_aryDispid_pv, Locals, "CScriptletSubObjects::SetContextMenu aryDispid::_pv")

enum { MENUID_OFFSET = 1 };

IMPLEMENT_SUBOBJECT_IUNKNOWN(CScriptletSubObjects, CScriptlet, Scriptlet, _ScriptletSubObjects)

CScriptletSubObjects::~CScriptletSubObjects()
{
    if (_hmenuCtx)
        DestroyMenu(_hmenuCtx);
    
    _aryDispid.DeleteAll();
}

// IUnknown methods

STDMETHODIMP CScriptletSubObjects::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid==IID_IUnknown || riid==IID_IDocHostUIHandler)
    {
        *ppv = (IDocHostUIHandler *)this;
        AddRef();
        return S_OK;
    }
    else if (riid==IID_IPropertyNotifySink)
    {
        *ppv = (IPropertyNotifySink *)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

// IDocHostUIHandler interface ////////////////////////////////////

STDMETHODIMP
CScriptletSubObjects::ShowContextMenu(DWORD dwID, POINT * ppt, IUnknown *, IDispatch *)
{
    HWND            hwnd = NULL;
    MSG             msg;
    DISPPARAMS      dispparams = { NULL, 0, NULL, 0 };

    // If we have no ctx menu, we're out of a job!

    if (!_hmenuCtx)
        goto Cleanup;

    // Create a small window to show what we've got. 

    hwnd = CreateWindowA(
                "STATIC",
                "",
                WS_POPUP,
                CW_USEDEFAULT, CW_USEDEFAULT,
                CW_USEDEFAULT, CW_USEDEFAULT,
                NULL,
                NULL,
                g_hInstCore,
                NULL);
    if (hwnd == NULL)
        goto Cleanup;

    // Show the context menu, funneling its command into the queue of hwnd.

    if (TrackPopupMenu(
            _hmenuCtx, TPM_LEFTALIGN, 
            ppt->x,
            ppt->y,
            0,
            hwnd,
            NULL) == 0)
    {
        goto Cleanup;
    }

    // The WM_COMMAND is in the message queue for the HWND. Get it.

    if (PeekMessage(&msg, hwnd, WM_COMMAND, WM_COMMAND, PM_REMOVE) == 0)
        goto Cleanup;

    // We have a dispid, now call the script object with it.

    Assert(Scriptlet()->_pDoc->_pOmWindow);
    Scriptlet()->_pDoc->_pOmWindow->InvokeEx(
                _aryDispid[LOWORD(msg.wParam) - MENUID_OFFSET],
                LOCALE_USER_DEFAULT,
                DISPATCH_METHOD,
                &dispparams, 
                NULL, NULL, NULL);

Cleanup:
    if (hwnd)
        DestroyWindow(hwnd);

    return S_OK;
}

STDMETHODIMP
CScriptletSubObjects::GetHostInfo(DOCHOSTUIINFO * pDHUI)
{
    pDHUI->cbSize = sizeof(*pDHUI);
    pDHUI->dwFlags =
            DOCHOSTUIFLAG_NO3DBORDER | 
            DOCHOSTUIFLAG_DISABLE_HELP_MENU;
    if (!Scriptlet()->_vbSelectable)
        pDHUI->dwFlags |= DOCHOSTUIFLAG_DIALOG;
    if (!Scriptlet()->_vbScrollbar) 
        pDHUI->dwFlags |= DOCHOSTUIFLAG_SCROLL_NO;
    pDHUI->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

    return S_OK;
}

STDMETHODIMP
CScriptletSubObjects::ShowUI(DWORD dwID, IOleInPlaceActiveObject *, IOleCommandTarget *, IOleInPlaceFrame *, IOleInPlaceUIWindow *)
{
    // We have no UI to show, Trident will do that for itself.
    return S_FALSE;
}

STDMETHODIMP
CScriptletSubObjects::HideUI()
{
    return S_OK;
}

STDMETHODIMP
CScriptletSubObjects::UpdateUI()
{
    return S_OK;
}

STDMETHODIMP
CScriptletSubObjects::EnableModeless(BOOL fEnable)
{
    return S_FALSE;
}

STDMETHODIMP
CScriptletSubObjects::OnDocWindowActivate(BOOL fActive)
{
    return S_FALSE;
}

STDMETHODIMP
CScriptletSubObjects::OnFrameWindowActivate(BOOL fActive)
{
    return S_FALSE;
}

STDMETHODIMP
CScriptletSubObjects::ResizeBorder(LPCRECT, IOleInPlaceUIWindow *, BOOL fFrameWindow)
{
    return S_FALSE;
}

STDMETHODIMP
CScriptletSubObjects::TranslateAccelerator(MSG * pmsg,const GUID * pguidCmdGroup, DWORD nCmdID)
{
    // disable F5 as a refresh tool.

    if (pmsg->message == WM_KEYDOWN || pmsg->message == WM_KEYUP)
    {
        if (pmsg->wParam == VK_F5)
            return S_OK;
    }

    return S_FALSE;
}

STDMETHODIMP
CScriptletSubObjects::GetOptionKeyPath(LPOLESTR * pchKey,  DWORD dwReserved)
{
    // User preferences are the same as those of Default Trident
    return S_FALSE;
}

STDMETHODIMP
CScriptletSubObjects::GetDropTarget(IDropTarget *, IDropTarget ** )
{
    // We don't interfere with Trident's drag/drop stuff.
    return S_FALSE;
}

/*
 *  IDocHostUIHandler::GetExternal
 *
 *  The IDispatch asked for will be used to resolve any references to 
 *  window.external from script. In our case, we want the object
 *  to be the CScriptControl.
 */

STDMETHODIMP
CScriptletSubObjects::GetExternal(IDispatch ** ppDisp)
{
    Assert(Scriptlet()->_pScriptCtrl);

    *ppDisp = (IDispatch *)(Scriptlet()->_pScriptCtrl);
    (*ppDisp)->AddRef();

    return S_OK;
}

STDMETHODIMP
CScriptletSubObjects::TranslateUrl(DWORD dwTranslate, OLECHAR *, OLECHAR ** )
{
    return S_FALSE;
}

STDMETHODIMP
CScriptletSubObjects::FilterDataObject(IDataObject *, IDataObject ** )
{
    return S_FALSE;
}


// IPropertyNotifySink

STDMETHODIMP CScriptletSubObjects::OnRequestEdit(DISPID dispid)
{
    if (dispid < 0)     // no Trident properties make it out.
        return S_OK;

    return Scriptlet()->FireRequestEdit(dispid);
}

STDMETHODIMP CScriptletSubObjects::OnChanged(DISPID dispid)
{
    switch (dispid)
    {
    case DISPID_READYSTATE:
        Scriptlet()->FireOnChanged(DISPID_READYSTATE);
        Scriptlet()->OnReadyStateChange();
        break;

    case STDPROPID_XOBJ_WIDTH:
        Scriptlet()->SetWidth(dispid);
        break;

    case STDPROPID_XOBJ_HEIGHT:
        Scriptlet()->SetHeight(dispid);
        break;

    default:
        if (dispid >= 0)     // no Trident properties make it out.
            Scriptlet()->FireOnChanged(dispid);
        break;
    }

    return S_OK;
}

// CScriptletSubObjects helper methods /////////////////////////////////////////

HRESULT
CScriptletSubObjects::SetContextMenu(VARIANT varArray)
{
    HRESULT                  hr;
    int                      i;
    DISPID                   dispid;
    char                     achCaption[256];
    HMENU                    hmenu = NULL;
    VARIANT                  varCaption;
    VARIANT                  varFunction;
    CDataAry<DISPID>         aryDispid(Mt(CScriptletSubObjectsSetContextMenu_aryDispid_pv));
    HRESULT                (*pfnGetItem)(const VARIANT & varArray, long i, VARIANT * pvar);

    // Determine the way to unpack the array.
    
    hmenu = CreatePopupMenu();
    if (!hmenu)
        goto Cleanup;

    switch (varArray.vt)
    {
    case VT_VARIANT | VT_ARRAY | VT_BYREF:
        // Access the data in a safearray (thank *you*, VBScript!)
        pfnGetItem = CScriptletSubObjects::GetVBScriptItem;
        break;
    case VT_DISPATCH:
        pfnGetItem = CScriptletSubObjects::GetJavaScriptItem;
        break;
    default:
        // Can't safely handle your data, mister.
        hr = E_FAIL;
        goto Cleanup;
    }

    // Unpack the array and store items in the menu and dynarray

    for (i = 0; ; i += 2)
    {
        hr = pfnGetItem(varArray, i, &varCaption);
        if (hr)
            break;
        hr = VariantChangeType(&varCaption, &varCaption, 0, VT_BSTR);
        if (hr)
            break;
        if (varCaption.bstrVal && varCaption.bstrVal[0])
        {
            WideCharToMultiByte(CP_ACP, 0, 
                                varCaption.bstrVal, -1, 
                                achCaption, sizeof(achCaption), 
                                NULL, NULL);

            hr = pfnGetItem(varArray, i+1, &varFunction);
            if (hr)
                break;
            hr = VariantChangeType(&varFunction, &varFunction, 0, VT_BSTR);
            if (hr)
                break;
            
            Assert(Scriptlet()->_pDoc->_pOmWindow);
            hr = Scriptlet()->_pDoc->_pOmWindow->GetDispID(varFunction.bstrVal, fdexFromGetIdsOfNames, &dispid);
            if (hr)
                break;
        }
        else
        {
            achCaption[0] = '\0';
            dispid = DISPID_UNKNOWN;
        }

        // The menu ID's are going to be 1+the offset into the arydispid array.
        // A menu ID of 0 is hard to distinguish from Windows error codes.

        Assert(aryDispid.Size() == i/2);

        hr = aryDispid.AppendIndirect(&dispid);
        if (hr)
            break;

        if (!AppendMenuA(
                hmenu,
                (achCaption[0]) ? MF_STRING : MF_SEPARATOR,
                i/2 + MENUID_OFFSET, 
                achCaption))
            break;

        VariantClear(&varCaption);
        VariantClear(&varFunction);
    }

    if (i == 0)
        goto Cleanup;

    hr = _aryDispid.EnsureSize(aryDispid.Size());
    if (hr)
        goto Cleanup;

    // All went well! Now destroy the old context menu and keep 
    // the new one.

    if (_hmenuCtx)
        DestroyMenu(_hmenuCtx);

    _hmenuCtx = hmenu;
    hmenu = NULL;

    memcpy((char *)(DISPID *)_aryDispid,
           (char *)(DISPID *)aryDispid,
           aryDispid.Size() * sizeof(DISPID));

    _aryDispid.SetSize(aryDispid.Size());

Cleanup:
    if (hmenu)
        DestroyMenu(hmenu);

    return S_OK;
}

HRESULT
CScriptletSubObjects::GetJavaScriptItem(const VARIANT & varArray, long i, VARIANT * pvar)
{
    HRESULT hr;
    DISPID  dispid;
    TCHAR   ach[30];

    // JScript returns a dispatch object with items named "0", "1", ....

    hr = Format(0, ach, ARRAY_SIZE(ach), _T("<0d>"), i);
    if (hr)
        goto Cleanup;

    hr = IdFromName(varArray.pdispVal, ach, &dispid);
    if (hr)
        goto Cleanup;
    
    hr = Property_get(varArray.pdispVal, dispid, pvar);

Cleanup:
    return hr;
}

HRESULT
CScriptletSubObjects::GetVBScriptItem(const VARIANT & varArray, long i, VARIANT * pvar)
{
    long cElements;

    // Convert to closest even number.
    cElements = ((*varArray.pparray)->rgsabound[0].cElements) & ~1;
    if (i >= cElements)
        return DISP_E_BADINDEX;
    return SafeArrayGetElement(*varArray.pparray, &i, pvar);
}
