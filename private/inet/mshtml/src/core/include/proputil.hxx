#ifndef I_PROPUTIL_HXX_
#define I_PROPUTIL_HXX_
#pragma INCMSG("--- Beg 'proputil.hxx'")

//
//  Property helper functions in prophelp.cxx
//

HRESULT
GetCommonPropertyValue(
        DISPID      dispid,
        UINT        cDisp,
        IDispatch ** apDisp,
        VARIANT *   pVar);

HRESULT
GetCommonSubObjectPropertyValue(
        DISPID      dispidMainObject,
        DISPID      dispidSubObject,
        UINT        cDisp,
        IDispatch ** apDisp,
        VARIANT *   pVar);

HRESULT
SetCommonSubObjectPropertyValue(
        DISPID      dispidMainObject,
        DISPID      dispidSubObject,
        UINT        cDisp,
        IDispatch ** apDisp,
        VARIANT *   pVar);

HRESULT
SetCommonPropertyValue(
        DISPID      dispid,
        UINT        cDisp,
        IDispatch ** apDisp,
        VARIANT *   pVar);

BOOL IsSameFontValue(VARIANT * pvar1, VARIANT * pvar2);

// Font Dialog Helper
HRESULT 
OpenFontDialog(
        CBase *     pBase,
        HWND        hWnd,
        UINT        cUnk,
        IUnknown ** apUnk,
        BOOL *      pfRet);


HRESULT
FindFontObject(
        UINT        cUnk,
        IUnknown ** apUnk,
        IFont **    ppFont);

#pragma INCMSG("--- End 'proputil.hxx'")
#else
#pragma INCMSG("*** Dup 'proputil.hxx'")
#endif

