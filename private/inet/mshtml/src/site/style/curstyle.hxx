//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       curStyle.cxx
//
//  Contents:   Support for CElement::CurrentStyle property
//
//----------------------------------------------------------------------------

#ifndef I_CURSTYLE_HXX_
#define I_CURSTYLE_HXX_
#pragma INCMSG("--- Beg 'curstyle.hxx'")

#define _hxx_
#include "curstyle.hdl"

class CTreeNode;

//+===========================================================================
//
//  Class:      CCurrentStyle
//
//  purpose : this class implements the object returned to OM by the 
//      currentSytle property of IHTMLElement. This class's exposes a subset of
//      the current style settings of the element.
//----------------------------------------------------------------------------

MtExtern(CCurrentStyle)

class CCurrentStyle : public CBase
{
    DECLARE_CLASS_TYPES(CCurrentStyle, CBase)

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCurrentStyle))

    // CTOR & Passivate
    CCurrentStyle(CTreeNode *pElem);
    void    Passivate();

    STDMETHOD(PrivateQueryInterface)(REFIID iid, void ** ppv);

    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));
    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
            VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pSrvProvider));
    
    DECLARE_TEAROFF_TABLE(IRecalcProperty)
    // IRecalcProperty methods
    NV_DECLARE_TEAROFF_METHOD(GetCanonicalProperty, getcanonicalproperty, (DISPID dispid, IUnknown **ppUnk, DISPID *pdispid));

    // helper methods
    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { Assert(_pNode); Assert(_pNode->Element()); return _pNode->Element()->GetAtomTable(pfExpando); }        

    // private data access helpers
    CTreeNode *GetNodePtr ( void ) { return _pNode; }
    HRESULT ComputeLongExtraFormat(DISPID dispID, BOOL fInherits, long defaultVal, long *plReturn);

    // interface defnitions
    #define _CCurrentStyle_
    #include "curstyle.hdl"


protected:
    DECLARE_CLASSDESC_MEMBERS;
    HRESULT GetColorHelper(VARIANT * p, const CColorValue &cvCol);
    HRESULT GetUnitValueHelper(VARIANT *p, CUnitValue uvVal, const PROPERTYDESC *pDesc, 
                    const ENUMDESC *pEnDesc = NULL, int nDefEnumValue = 0);
    HRESULT GetImageNameFromCookie(long lCookie, BSTR *p);

    CAttrArray *GetCachedExpandoAA();

    // This is used by properties like borderStyle, borderColor
    HRESULT GetCompositBSTR(LPCTSTR szTop, LPCTSTR szRight, 
        LPCTSTR szBottom, LPCTSTR szLeft, BSTR *bstrStr);
    HRESULT GetCompositBSTR(CVariant *pvarTop, CVariant *pvarRight, 
        CVariant *varpBottom, CVariant *pvarLeft, BSTR *bstrStr);

    styleBorderStyle GetBorderStyle(int nBorder);

private:
    WHEN_DBG(enum {eCookie=0x13000031};)
    WHEN_DBG(DWORD _dwCookie;)

    CTreeNode       *_pNode;
};

#pragma INCMSG("--- End 'curstyle.hxx'")
#else
#pragma INCMSG("*** Dup 'curstyle.hxx'")
#endif

