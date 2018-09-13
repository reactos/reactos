//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       imgelem.hxx
//
//  Contents:   CImgElement, etc...
//
//----------------------------------------------------------------------------

#ifndef I_IMGELEM_HXX_
#define I_IMGELEM_HXX_
#pragma INCMSG("--- Beg 'imgelem.hxx'")

#ifndef X_IMGHLPER_HXX_
#define X_IMGHLPER_HXX_
#include "imghlper.hxx"
#endif

#ifndef X_BTNHLPER_HXX_
#define X_BTNHLPER_HXX_
#include "btnhlper.hxx"
#endif

#define _hxx_
#include "img.hdl"

class CAnchorElement;
class CMapElement;
class CAreaElement;
class CShape;

class CImgElementLayout;
class CInputImageLayout;

MtExtern(CImgElement)
MtExtern(CInputImage)
MtExtern(CImageElementFactory)

class CImgElementLayout;

//+---------------------------------------------------------------------------
//
// CImgElement
//
//----------------------------------------------------------------------------

class CImgElement : public CSite
{
    friend class CImgElementLayout;

    DECLARE_CLASS_TYPES(CImgElement, CSite)

    friend class CDBindMethodsImg;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgElement))

    CImgElement (ELEMENT_TAG etag, CDoc *pDoc);

    virtual void    Notify(CNotification *pNF);
    virtual HRESULT Init();
    HRESULT EnterTree();

    void EnsureMap();
    void EnsureTabStop();

    DECLARE_LAYOUT_FNS(CImgElementLayout)

    STDMETHOD(PrivateQueryInterface)(REFIID iid, void ** ppv);

    STDMETHOD(QueryStatus) (
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext);

    STDMETHOD(Exec) (
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual void Passivate();
    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);
    NV_DECLARE_MOUSECAPTURE_METHOD(HandleCaptureMessageForImage, handlecapturemessageforimage,
                                          (CMessage * pMessage));
    NV_DECLARE_MOUSECAPTURE_METHOD(HandleCaptureMessageForArea, handlecapturemessageforarea,
                                          (CMessage * pMessage));

    virtual HRESULT ClickAction(CMessage * pMessage);
    HRESULT ClickOnServerMap(POINT pt, BOOL fOpenInNewWindow);

    CAnchorElement *FindEnclosingAnchorScope();
    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);
    HRESULT Save(CStreamWriteBuff * pStmWrBuff, BOOL fEnd);

    virtual HRESULT DoSubDivisionEvents(long lSubDivision, DISPID dispidMethod, DISPID dispidProp, VARIANT *pvb, BYTE * pbTypes, ...);
    virtual HRESULT GetSubDivisionCount(long *pc);
    virtual HRESULT GetSubDivisionTabs(long *pTabs, long c);
    virtual HRESULT SubDivisionFromPt(POINT pt, long *piSub);
    virtual HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);
    virtual HRESULT ShowTooltip(CMessage *pmsg, POINT pt);
    
    COLORREF GetAnchorColor(CAnchorElement * pAnchorElement);
    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    void DocPtToImgPt(POINT *ppt);

    // helper functions
    CAnchorElement * GetContainingAnchor() 
            { return _fBelowAnchor ? FindEnclosingAnchorScope() : NULL; }
    CMapElement * GetMap() { return _pMap; }
    CMapElement * EnsureAndGetMap() { EnsureMap(); return _pMap; }
    CAreaElement * GetCurrentArea();

    // InvokeEx to validate ready state.
    NV_DECLARE_TEAROFF_METHOD(ContextThunk_InvokeExReady, invokeexready, (
            DISPID id,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pdp,
            VARIANT *pvarRes,
            EXCEPINFO *pei,
            IServiceProvider *pSrvProvider));

    //base implementation prototypes
    NV_DECLARE_TEAROFF_METHOD(put_src, PUT_src, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_src, GET_src, (BSTR * p));
    HRESULT STDMETHODCALLTYPE put_height(long v);
    HRESULT STDMETHODCALLTYPE get_height(long*p);
    HRESULT STDMETHODCALLTYPE put_width(long v);
    HRESULT STDMETHODCALLTYPE get_width(long*p);

    STDMETHOD(put_hspace)(long v);
    STDMETHOD(get_hspace)(long * p);

    BOOL    IsHSpaceDefined() const ;

    HRESULT putHeight(long l);
    HRESULT putWidth(long l);
    HRESULT GetHeight(long *pl);
    HRESULT GetWidth(long *pl);

    NV_DECLARE_TEAROFF_METHOD(get_onload, GET_onload, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD(put_onload, PUT_onload, (VARIANT));

    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (BSTR *pBSTR));
    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (VARIANT *pReadyState));
    NV_DECLARE_TEAROFF_METHOD(get_readyStateValue, get_readyStateValue, (long *plRetValue));

#ifndef NO_DATABINDING
    // databinding
    virtual const CDBindMethods *GetDBindMethods();
#endif

#ifdef WIN16
    static HRESULT STDMETHODCALLTYPE PUT_hspace(CImgElement *pObj, long v)
    { return pObj->put_hspace(v); }
    static HRESULT STDMETHODCALLTYPE GET_hspace(CImgElement *pObj, long * p)
    { return pObj->get_hspace(p); }
    static HRESULT STDMETHODCALLTYPE PUT_width(CImgElement *pObj, long v)
    { return pObj->put_width(v); }
    static HRESULT STDMETHODCALLTYPE GET_width(CImgElement *pObj, long * p)
    { return pObj->get_width(p); }
    static HRESULT STDMETHODCALLTYPE PUT_height(CImgElement *pObj, long v)
    { return pObj->put_height(v); }
    static HRESULT STDMETHODCALLTYPE GET_height(CImgElement *pObj, long * p)
    { return pObj->get_height(p); }
#endif

    #define _CImgElement_
    #include "img.hdl"

    CImgHelper *_pImage;


protected:
    DECLARE_CLASSDESC_MEMBERS;
    static const CLSID *            s_apclsidPages[];

private:
    CMapElement *       _pMap;              // associated client side map

    RECT                _rcWobbleZone;
    BOOL                _fCanClickImage;
    BOOL                _fBelowAnchor;

    NO_COPY(CImgElement);
};

#ifdef  NEVER

//+---------------------------------------------------------------------------
//
// CInputImage
//
//----------------------------------------------------------------------------

class CInputImage: public CSite, public CBtnHelper
{
    DECLARE_CLASS_TYPES(CInputImage, CSite)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CInputImage))

    CInputImage (ELEMENT_TAG eTag, CDoc *pDoc) :
        CSite(eTag, pDoc)
        {
            _fActsLikeButton=TRUE;
#ifdef WIN16
            m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
            m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
        }

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult,
                              htmlInput type);

    DECLARE_LAYOUT_FNS(CInputImageLayout)

    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage, CElement * pChild, CTreeNode * pNodeContext);
    virtual CElement * GetElement ( void ) { return this; }
    virtual HRESULT ClickAction(CMessage * pMessage);
    virtual HRESULT GetSubmitInfo(CPostData * pSubmitData);
    virtual HRESULT GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);
    virtual void    Notify(CNotification *pNF);

#ifdef WIN16
    static HRESULT STDMETHODCALLTYPE PUT_hspace(CInputImage *pObj, long v)
    { return pObj->put_hspace(v); }
    static HRESULT STDMETHODCALLTYPE GET_hspace(CInputImage *pObj, long * p)
    { return pObj->get_hspace(p); }
    static HRESULT STDMETHODCALLTYPE PUT_width(CInputImage *pObj, long v)
    { return pObj->put_width(v); }
    static HRESULT STDMETHODCALLTYPE GET_width(CInputImage *pObj, long * p)
    { return pObj->get_width(p); }
    static HRESULT STDMETHODCALLTYPE PUT_height(CInputImage *pObj, long v)
    { return pObj->put_height(v); }
    static HRESULT STDMETHODCALLTYPE GET_height(CInputImage *pObj, long * p)
    { return pObj->get_height(p); }
#endif

    #define _CInputImage_
    #include "img.hdl"

    POINT   _pt;

protected:
    DECLARE_CLASSDESC_MEMBERS;
};

#endif

class CImageElementFactory : public CElementFactory
{
    DECLARE_CLASS_TYPES(CImageElementFactory, CElementFactory)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImageElementFactory))

    CImageElementFactory(){};
    ~CImageElementFactory(){};

    #define _CImageElementFactory_
    #include "img.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;
};

#pragma INCMSG("--- End 'imgelem.hxx'")
#else
#pragma INCMSG("*** Dup 'imgelem.hxx'")
#endif
