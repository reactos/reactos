//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       rulestyl.hxx
//
//  Contents:   Support for Cascading Style Sheets Object Model.
//
//              CRuleStyle
//
//----------------------------------------------------------------------------

#ifndef I_RULESTYL_HXX_
#define I_RULESTYL_HXX_
#pragma INCMSG("--- Beg 'rulestyl.hxx'")

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

class CStyleSheetRule;

MtExtern(CRuleStyle)

//+---------------------------------------------------------------------------
//
//  Class:      CRuleStyle
//
//----------------------------------------------------------------------------
class CRuleStyle : public CStyle
{
    DECLARE_CLASS_TYPES(CRuleStyle, CStyle)
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRuleStyle))
    CRuleStyle( CStyleSheetRule *pRule );
    ~CRuleStyle();

    inline void ClearRule() { _pRule = NULL; _pAA = NULL; };

    //Data access
    HRESULT PrivatizeProperties(CVoid **ppv) {return S_OK;}

    //Parsing methods
    HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    //CBase methods
    STDMETHOD(PrivateQueryInterface)(REFIID iid, void ** ppv);
    HRESULT QueryInterface(REFIID iid, void **ppv){return PrivateQueryInterface(iid, ppv);}

    STDMETHODIMP_(ULONG) PrivateAddRef();
    STDMETHODIMP_(ULONG) PrivateRelease();

    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL );

    void Passivate();
    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void*_apfnTearOff;
    };

    // Helper for HDL file
    virtual CAttrArray **GetAttrArray ( void ) const;

    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));
    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pSrvProvider));

    // Override all property gets/puts so that the invtablepropdesc points to
    // our local functions to give us a chance to pass the pAA of CStyleRule and
    // not CRuleStyle.
    NV_DECLARE_TEAROFF_METHOD(put_StyleComponent, PUT_StyleComponent, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(put_Url, PUT_Url, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(put_String, PUT_String, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(put_Short, PUT_Short, (short v));
    NV_DECLARE_TEAROFF_METHOD(put_Long, PUT_Long, (long v));
    NV_DECLARE_TEAROFF_METHOD(put_Bool, PUT_Bool, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(put_Variant, PUT_Variant, (VARIANT v));
    NV_DECLARE_TEAROFF_METHOD(put_DataEvent, PUT_DataEvent, (VARIANT v));
    NV_DECLARE_TEAROFF_METHOD(get_Url, GET_Url, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(get_StyleComponent, GET_StyleComponent, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(get_Property, GET_Property, (void *p));

#ifndef NO_EDIT
    virtual IOleUndoManager * UndoManager(void);
    virtual BOOL QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange = TRUE);
#endif // NO_EDIT

    #define _CRuleStyle_
    #include "style.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

    CStyleSheetRule  *_pRule;   // May be NULL if disconnected.
};

#pragma INCMSG("--- End 'rulestyl.hxx'")
#else
#pragma INCMSG("*** Dup 'rulestyl.hxx'")
#endif
