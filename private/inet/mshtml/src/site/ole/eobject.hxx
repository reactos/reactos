//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       eobject.hxx
//
//  Contents:   CObjectElement, etc...
//
//----------------------------------------------------------------------------

#ifndef I_EOBJECT_HXX_
#define I_EOBJECT_HXX_
#pragma INCMSG("--- Beg 'eobject.hxx'")

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#define _hxx_
#include "object.hdl"

#define OBJECTPARAM_NAME    _T("NAME")
#define OBJECTPARAM_VALUE   _T("VALUE")
#define OBJECTPARAM_DATASRC   _T("DATASRC")
#define OBJECTPARAM_DATAFLD   _T("DATAFLD")
#define OBJECTPARAM_DATAFORMATAS    _T("DATAFORMATAS")

#define MAGIC_DATA_OBJECT_COOKIE   _T("MAGIC_DATA_OBJECT_COOKIE")

MtExtern(CObjectElement)

//+---------------------------------------------------------------------------
//
// CObjectElement
//
//----------------------------------------------------------------------------

class CObjectElement : public COleSite
{
    NO_COPY(CObjectElement);
    DECLARE_CLASS_TYPES(CObjectElement, COleSite)

    friend class CDBindMethodsObject;
    friend class CHtmObjectParseCtx;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CObjectElement))

    CObjectElement(ELEMENT_TAG etag, CDoc *pDoc);
    virtual void Passivate();

    // IUnknown methods
    DECLARE_PRIVATE_QI_FUNCS(CBase);

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    // InvokeEx to validate ready state.
    NV_DECLARE_TEAROFF_METHOD(ContextThunk_InvokeExReady, invokeexready, (
            DISPID id,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pdp,
            VARIANT *pvarRes,
            EXCEPINFO *pei,
            IServiceProvider *pSrvProvider));

    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (VARIANT *pReadyState));
    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (long *pReadyState));
    NV_DECLARE_TEAROFF_METHOD(put_classid, put_classid, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_classid, get_classid, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(put_codeType, put_codeType, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_codeType, get_codeType, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(put_type, put_type, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_type, get_type, (BSTR *p));

    //
    // CElement/CSite/COleSite overrides
    //
    
    virtual HRESULT Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);
    
#ifndef NO_DATABINDING
    virtual HRESULT SaveDataIfChanged(LONG id, BOOL fLoud = FALSE, BOOL fForceIsCurrent=FALSE);
    
    virtual const CDBindMethods *GetDBindMethods();
#endif
    virtual HRESULT OnControlRequestEdit(DISPID dispid);
    virtual HRESULT OnControlChanged(DISPID dispid);

    virtual HRESULT OnFailToCreate();

    #define _CObjectElement_
    #include "object.hdl"

    //
    // Databinding
    //
    
    struct PARAMBINDING
    {
        TCHAR *_strParamName;   // name of bound property
        TCHAR *_strDataSrc;
        TCHAR *_strDataFld;
        TCHAR *_strDataFormatAs;
        DISPID _dispidGet;      // dispid to get property
        DISPID _dispidPut;      // dispid to set property
        DWORD  _dwInvokeFlags;  // should we use METHOD, or PROPERTYPUT/GET?
        VARTYPE _vt;           // preferred type of property
        
        PARAMBINDING(): _strParamName(NULL), _strDataSrc(NULL),
                        _strDataFld(NULL), _strDataFormatAs(FALSE),
                        _dispidGet(DISPID_UNKNOWN), _dispidPut(DISPID_UNKNOWN),
                        _dwInvokeFlags(0), _vt(VT_EMPTY) {}
    };
    
    HRESULT GetIDForParamBinding(PARAMBINDING *pParamBinding, BOOL fPut);
    HRESULT RemoveBoundParams(CPropertyBag *pParamBag,
                                    BOOL fPreserve,
                                    CPropertyBag **ppParamBagReturn);
    HRESULT SaveParamBindings(CStreamWriteBuff * pStreamWrBuff);
    void    EnsureParamType(PARAMBINDING *pParamBinding);  
     
#ifdef WIN16  
    HRESULT SaveWin16AppletProps(IPropertyBag* _pParamBag);
#endif
    NV_DECLARE_ONCALL_METHOD(DeferredSaveData, deferredsavedata, (DWORD_PTR));

    //
    // Misc. helpers
    //

    HRESULT CreateObject();
    HRESULT StreamFromInlineData(TCHAR *pchData, IStream **ppStm);
    HRESULT SaveToDataStream();
    HRESULT SaveAltHtml (CStreamWriteBuff *pStreamWrBuff);
    void    RetrieveClassidAndData(
        CLSID *pclsid, 
        IStream **pStm, 
        TCHAR **ppchData,
        TCHAR **ppchClassid);

    BOOL Fire_onerror();
    
    //
    // Form submission
    //
    
    virtual HRESULT GetSubmitInfo(CPostData *pSubmitData);

    
    DECLARE_CLASSDESC_MEMBERS;

    //
    // additional databinding stuff
    //
    CDataAry<PARAMBINDING> _aryParamBinding;
    
    static const CONNECTION_POINT_INFO              s_acpi[];

};

inline BOOL 
CObjectElement::Fire_onerror()
{
    return FireCancelableEvent(DISPID_XOBJ_BASE+19, DISPID_EVPROP_ONERROR, _T("error"), (BYTE *) VTS_NONE);
}

#pragma INCMSG("--- End 'eobject.hxx'")
#else
#pragma INCMSG("*** Dup 'eobject.hxx'")
#endif
