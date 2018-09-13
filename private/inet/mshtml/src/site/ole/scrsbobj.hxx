//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       scrsbobj.hxx
//
//  History:    19-Jan-1998     sramani     Created
//
//  Contents:   CScriptletSubObjects definition
//
//----------------------------------------------------------------------------

#ifndef I_SCRSBOBJ_HXX_
#define I_SCRSBOBJ_HXX_
#pragma INCMSG("--- Beg 'scrsbobj.hxx'")

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#pragma INCMSG("--- Beg <mshtmhst.h>")
#include <mshtmhst.h>
#pragma INCMSG("--- End <mshtmhst.h>")
#endif

MtExtern(CScriptletSubObjects)
MtExtern(CScriptletSubObjects_aryDispid_pv)

class CScriptlet;

class CScriptletSubObjects : public IDocHostUIHandler,
                             public IPropertyNotifySink
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CScriptletSubObjects))
    CScriptletSubObjects() : _aryDispid(Mt(CScriptletSubObjects_aryDispid_pv)) {}
    ~CScriptletSubObjects();

    DECLARE_SUBOBJECT_IUNKNOWN(CScriptlet, Scriptlet)

    // IDocHostUIHandler
    STDMETHOD(ShowContextMenu)(DWORD dwID, POINT * ppt, IUnknown *, IDispatch *);
    STDMETHOD(GetHostInfo)(DOCHOSTUIINFO *);
    STDMETHOD(ShowUI)(DWORD dwID, IOleInPlaceActiveObject *, IOleCommandTarget *, IOleInPlaceFrame *, IOleInPlaceUIWindow *);
    STDMETHOD(HideUI)();
    STDMETHOD(UpdateUI)();
    STDMETHOD(EnableModeless)(BOOL fEnable);
    STDMETHOD(OnDocWindowActivate)(BOOL fActive);
    STDMETHOD(OnFrameWindowActivate)(BOOL fActive);
    STDMETHOD(ResizeBorder)(LPCRECT, IOleInPlaceUIWindow *, BOOL fFrameWindow);
    STDMETHOD(TranslateAccelerator)(MSG *,const GUID * pguidCmdGroup, DWORD nCmdID);
    STDMETHOD(GetOptionKeyPath)(LPOLESTR * pchKey,  DWORD dwReserved);
    STDMETHOD(GetDropTarget)(IDropTarget *, IDropTarget **);
    STDMETHOD(GetExternal)(IDispatch **);
    STDMETHOD(TranslateUrl)(DWORD dwTranslate, OLECHAR *, OLECHAR **);
    STDMETHOD(FilterDataObject)(IDataObject *, IDataObject **);

    // IPropertyNotifySink methods
    STDMETHOD(OnChanged)  (DISPID dispid);
    STDMETHOD(OnRequestEdit) (DISPID dispid);

    HRESULT SetContextMenu(VARIANT var);

private:
    static HRESULT  GetJavaScriptItem(const VARIANT & varArray, long i, VARIANT * pvar);
    static HRESULT  GetVBScriptItem(const VARIANT & varArray, long i, VARIANT * pvar);

    HMENU            _hmenuCtx;
    CDataAry<DISPID> _aryDispid;
};

#pragma INCMSG("--- End 'scrsbobj.hxx'")
#else
#pragma INCMSG("*** Dup 'scrsbobj.hxx'")
#endif
