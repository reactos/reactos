//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       window.hxx
//
//  Contents:   The window (object model)
//
//-------------------------------------------------------------------------

#ifndef I_WINDOW_HXX_
#define I_WINDOW_HXX_
#pragma INCMSG("--- Beg 'window.hxx'")

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#pragma INCMSG("--- Beg <dispex.h>")
#include <dispex.h>
#pragma INCMSG("--- End <dispex.h>")
#endif

MtExtern(CScreen)
MtExtern(COmWindow2)
MtExtern(COmLocation)
MtExtern(COmHistory)
MtExtern(CMimeTypes)
MtExtern(CPlugins)
MtExtern(COpsProfile)
MtExtern(COmNavigator)
MtExtern(COmWindowProxy)
MtExtern(CAryWindowTbl)
MtExtern(CAryWindowTbl_pv)

#define SID_SHTMLWindow2 IID_IHTMLWindow2
#define SID_SOmLocation  IID_IHTMLLocation
#define DISPID_OMWINDOWMETHODS    (DISPID_WINDOW + 10000)


//
// Forward decls.
//

class COmWindow2;
class COmWindowProxy;
class CEventObj;
class CElement;
class COmLocationProxy;
class COmHistory;
class COmLocation;
class COmNavigator;
interface IPersistDataFactory;
interface IHTMLFramesCollection2;
interface IHTMLWindow2;

enum URLCOMP_ID;

#define _hxx_
#include "window.hdl"

#define _hxx_
#include "history.hdl"

//+------------------------------------------------------------------------
//
//  Class:      CScreen
//
//  Purpose:    The screen object.  Referenced off the window object but
//              implemented as a subobject of CDoc.
//
//-------------------------------------------------------------------------

class CScreen :
        public CBase
{
    DECLARE_CLASS_TYPES(CScreen, CBase)

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CScreen))
    DECLARE_FORMS_SUBOBJECT_IUNKNOWN(COmWindow2)

    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}

    DECLARE_PRIVATE_QI_FUNCS(CBase)

    #define _CScreen_
    #include "window.hdl"
    
    // Static members
    static const CLASSDESC                    s_classdesc;
};


//+------------------------------------------------------------------------
//
//  Class:      COmWindow2
//
//  Purpose:    The automatable window (one per doc).
//
//-------------------------------------------------------------------------

class COmWindow2 :   public CBase,
                     public IHTMLWindow2,
                     public IDispatchEx,
                     public IHTMLWindow3
{
    DECLARE_CLASS_TYPES(COmWindow2, CBase)

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COmWindow2))
    DECLARE_TEAROFF_TABLE(IProvideMultipleClassInfo)
    DECLARE_TEAROFF_TABLE(IServiceProvider)

    // CBase methods
    DECLARE_PLAIN_IUNKNOWN(COmWindow2)

    // ctor/dtor
    COmWindow2(CDoc *pDoc);
    ~COmWindow2();
    
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}
    virtual void                Passivate();
    virtual CAtomTable * GetAtomTable (BOOL *pfExpando = NULL);

    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // IDispatch methods:
    STDMETHOD(GetTypeInfoCount)         (UINT FAR* pctinfo);
    STDMETHOD(GetTypeInfo)              (
                UINT itinfo, 
                LCID lcid, 
                ITypeInfo ** pptinfo);
    STDMETHOD(GetIDsOfNames)            (
                REFIID                riid,
                LPOLESTR *            rgszNames,
                UINT                  cNames,
                LCID                  lcid,
                DISPID FAR*           rgdispid);
    STDMETHOD(Invoke)                   (
                DISPID          dispidMember,
                REFIID          riid,
                LCID            lcid,
                WORD            wFlags,
                DISPPARAMS *    pdispparams,
                VARIANT *       pvarResult,
                EXCEPINFO *     pexcepinfo,
                UINT *          puArgErr);

    // IDispatchEx methods
    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID *pid);

    STDMETHOD (InvokeEx)(
            DISPID dispidMember,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            IServiceProvider *pSrvProvider);

    HRESULT STDMETHODCALLTYPE DeleteMemberByName(BSTR bstr,DWORD grfdex);
    HRESULT STDMETHODCALLTYPE DeleteMemberByDispID(DISPID id);

    STDMETHOD(GetMemberProperties)(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex);

    STDMETHOD(GetMemberName) (DISPID id,
                              BSTR *pbstrName);
    STDMETHOD(GetNextDispID)(
                DWORD grfdex,
                DISPID id,
                DISPID *prgid);
    STDMETHOD(GetNameSpaceParent)(IUnknown **ppunk);

    // IProvideMultiClassInfo methods
    DECLARE_TEAROFF_METHOD(GetClassInfo, getclassinfo,                    \
            (ITypeInfo ** ppTI))              \
        { return CBase::GetClassInfo(ppTI); }            

    DECLARE_TEAROFF_METHOD(GetGUID, getguid,                    \
            (DWORD dwGuidKind, GUID * pGUID))              \
        { return CBase::GetGUID(dwGuidKind, pGUID); }            

    NV_DECLARE_TEAROFF_METHOD(GetMultiTypeInfoCount, getmultitypeinfocount, (ULONG *pcti));
    NV_DECLARE_TEAROFF_METHOD(GetInfoOfIndex, getinfoofindex, (
            ULONG iti,
            DWORD dwFlags,
            ITypeInfo** pptiCoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource));

    // IServiceProvider methods
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));

    // so that we can get an eventParam
    HRESULT FireEvent(DISPID dispidMethod, DISPID dispidProp, 
                  LPCTSTR pchEventType, 
                  BYTE * pbTypes, ...);

    HRESULT ShowHTMLDialogHelper(BSTR      bstrUrl, 
                                 VARIANT * pvarArgIn, 
                                 VARIANT * pvarOptions, 
                                 BOOL      fModeless,                                 
                                 IHTMLWindow2** ppDialog,
                                 VARIANT * pvarArgOut);

    // pdl hook up
    #define _COmWindow2_
    #include "window.hdl"

    // Method exposed via nopropdesc:nameonly (not spit out because no tearoff exist).
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, setTimeout, settimeout,   (BSTR expression,long msec,VARIANT* language,long* timerID));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, setInterval, setinterval, (BSTR expression,long msec,VARIANT* language,long* timerID));

    // Event fire method declarations for eventset
    void Fire_onload();
    void Fire_onunload();
    BOOL Fire_onbeforeunload();
    void Fire_onfocus();
    void Fire_onblur();
    void Fire_onerror(BSTR, BSTR, long);
    void Fire_onscroll();
    BOOL Fire_onhelp(BYTE * pbTypes = (BYTE *) VTS_NONE, ... );
    void Fire_onresize();
    void Fire_onbeforeprint();
    void Fire_onafterprint();

    // Helper function to search other script engines name space.
    HRESULT FindNamesFromOtherScripts(REFIID riid,
                                      LPOLESTR *rgszNames,
                                      UINT cNames,
                                      LCID lcid,
                                      DISPID *rgdispid,
                                      DWORD grfdex = 0);

    // Data Members
    CDoc *                              _pDoc;
    
    CScreen                             _Screen;    //  Embedded screen subobject

    // The following 3 subobjects provide expando prop support for their corresponding 
    // browser provided implementations as well as full support for cases when we are
    // not hosted by shdocvw.
    COmHistory *                        _pHistory;
    COmLocation *                       _pLocation;
    COmNavigator *                      _pNavigator;

    // Static members
    static const CONNECTION_POINT_INFO  s_acpi[];
    static const CLASSDESC              s_classdesc;

    CStr                                _cstrName;  // holds name for non-shdocvw implementation
    VARIANT                             _varOpener; // holds the opener prop value for non-shdocvw implementation
#if 0
//BUGBUG: ***TLL*** quick event hookup for VBS
    IDispatch                          *_pSinkupObject;
#endif

protected:
    // Wrapper that dynamically loads, translates to ASCII and calls HtmlHelpA
    HRESULT CallHtmlHelp(HWND hwnd, BSTR pszFile, UINT uCommand, DWORD_PTR dwData, HWND *pRetHWND = NULL);

    HRESULT FilterOutFeaturesString(BSTR bstrFeatures, BSTR *pbstrOut);
};

//+------------------------------------------------------------------------
//
//  Class:      COmLocation
//
//  Purpose:    The location object.  Referenced off the window object but
//              implemented as a subobject of CDoc.
//
//-------------------------------------------------------------------------

class COmLocation : public CBase
{
    DECLARE_CLASS_TYPES(COmLocation, CBase)

private:
    COmWindow2 *_pWindow;

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COmLocation))

    COmLocation(COmWindow2 *pWindow);
    ~COmLocation() { super::Passivate(); }

    //CBase methods
    STDMETHOD(PrivateQueryInterface)(REFIID iid, void ** ppv);
    HRESULT QueryInterface(REFIID iid, void **ppv){return PrivateQueryInterface(iid, ppv);}

    STDMETHODIMP_(ULONG) PrivateAddRef();
    STDMETHODIMP_(ULONG) PrivateRelease();

    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { return _pWindow->GetAtomTable(pfExpando); }        

    HRESULT GetUrlComponent(BSTR *pstrComp, URLCOMP_ID ucid, TCHAR **ppchUrl, DWORD dwFlags);
    HRESULT SetUrlComponent(const BSTR bstrComp, URLCOMP_ID ucid);

    #define _COmLocation_
    #include "history.hdl"
    
    // IObjectIdentity methods
    NV_DECLARE_TEAROFF_METHOD(IsEqualObject, isequalobject, (IUnknown *ppunk));

protected:
    DECLARE_CLASSDESC_MEMBERS;
};
        
//+------------------------------------------------------------------------
//
//  Class:      COmHistory
//
//  Purpose:    The history object.  Referenced off the window object but
//              implemented as a subobject of CDoc.
//
//-------------------------------------------------------------------------

class COmHistory : public CBase
{
    DECLARE_CLASS_TYPES(COmHistory, CBase)

private:
    COmWindow2 *_pWindow;

// CHROME
    HRESULT GetChromeSiteHistory(IOmHistory **ppiChromeSiteHistory);

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COmHistory))

    COmHistory(COmWindow2 *pWindow);
    ~COmHistory() { super::Passivate(); }

    //CBase methods
    STDMETHOD(PrivateQueryInterface)(REFIID iid, void ** ppv);
    HRESULT QueryInterface(REFIID iid, void **ppv){return PrivateQueryInterface(iid, ppv);}

    STDMETHODIMP_(ULONG) PrivateAddRef();
    STDMETHODIMP_(ULONG) PrivateRelease();

    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { return _pWindow->GetAtomTable(pfExpando); }        

    #define _COmHistory_
    #include "history.hdl"
    
        // IObjectIdentity methods
    NV_DECLARE_TEAROFF_METHOD(IsEqualObject, isequalobject, (IUnknown *ppunk));


protected:
    DECLARE_CLASSDESC_MEMBERS;
};


//+------------------------------------------------------------------------
//
//  Class:      CMimeTypes
//
//  Purpose:    Mime types collection
//
//-------------------------------------------------------------------------

class CMimeTypes : public CBase
{
     DECLARE_CLASS_TYPES(CMimeTypes, CBase)

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CMimeTypes))

    CMimeTypes() {};
    ~CMimeTypes() {super::Passivate();}

    // IUnknown
    DECLARE_PLAIN_IUNKNOWN(CMimeTypes );

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    // IHTMLMimeTypes methods
    #define _CMimeTypes_
    #include "history.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

};


//+------------------------------------------------------------------------
//
//  Class:      CPlugins
//
//  Purpose:    Plugins collection for navigator object
//
//-------------------------------------------------------------------------

class CPlugins : public CBase
{
     DECLARE_CLASS_TYPES(CPlugins, CBase)

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CPlugins))

    CPlugins() {};
    ~CPlugins() {super::Passivate();}

    // IUnknown
    DECLARE_PLAIN_IUNKNOWN(CPlugins);

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    // IHTMLPlugins methods
    #define _CPlugins_
    #include "history.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

};

//+------------------------------------------------------------------------
//
//  Class:      COpsProfile
//
//  Purpose:    COpsProfile for navigator object
//
//-------------------------------------------------------------------------

class COpsProfile : public CBase
{
     DECLARE_CLASS_TYPES(COpsProfile, CBase)

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COpsProfile))

    COpsProfile() {};
    ~COpsProfile() {super::Passivate();}

    // IUnknown
    DECLARE_PLAIN_IUNKNOWN(COpsProfile);

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    // IHTMLOpsProfile methods
    #define _COpsProfile_
    #include "history.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

};


//+------------------------------------------------------------------------
//
//  Class:      COmNavigator
//
//  Purpose:    The navigator object.  Referenced off the window object but
//              implemented as a subobject of CDoc.
//
//-------------------------------------------------------------------------

class COmNavigator : public CBase
{
    DECLARE_CLASS_TYPES(COmNavigator, CBase)

private:
    COmWindow2 *_pWindow;

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COmNavigator))

    COmNavigator(COmWindow2 *pWindow);
    ~COmNavigator();

    //CBase methods
    STDMETHOD(PrivateQueryInterface)(REFIID iid, void ** ppv);
    HRESULT QueryInterface(REFIID iid, void **ppv){return PrivateQueryInterface(iid, ppv);}

    STDMETHODIMP_(ULONG) PrivateAddRef();
    STDMETHODIMP_(ULONG) PrivateRelease();

    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { return _pWindow->GetAtomTable(pfExpando); }        

    #define _COmNavigator_
    #include "history.hdl"
    
protected:
    DECLARE_CLASSDESC_MEMBERS;
private:
    CPlugins    * _pPluginsCollection;
    CMimeTypes  * _pMimeTypesCollection;
    COpsProfile * _pOpsProfile;
};

//+------------------------------------------------------------------------
//
//  Class:      CLocationProxy
//
//  Purpose:    The automatable location obj.
//
//-------------------------------------------------------------------------

class COmLocationProxy : public CVoid, 
                         public IHTMLLocation
{
    DECLARE_CLASS_TYPES(COmLocationProxy, CVoid)
    
public:
    DECLARE_TEAROFF_TABLE(IDispatchEx)
	DECLARE_TEAROFF_TABLE(IObjectIdentity)
	DECLARE_TEAROFF_TABLE(IServiceProvider)
    DECLARE_TEAROFF_TABLE_NAMED(s_apfnLocationVTable)

    // IUnknown methods
    NV_DECLARE_TEAROFF_METHOD_(ULONG, AddRef, addref, ());
    NV_DECLARE_TEAROFF_METHOD_(ULONG, Release, release, ());
    NV_DECLARE_TEAROFF_METHOD(QueryInterface, queryinterface, (REFIID iid, void **ppvObj));
    
    // IDispatch methods:
    NV_DECLARE_TEAROFF_METHOD(GetTypeInfoCount , gettypeinfocount , ( UINT * pctinfo ));
    NV_DECLARE_TEAROFF_METHOD(GetIDsOfNames, getidsofnames, (
            REFIID                riid,
            LPTSTR *              rgszNames,
            UINT                  cNames,
            LCID                  lcid,
            DISPID *              rgdispid));
    NV_DECLARE_TEAROFF_METHOD(GetTypeInfo, gettypeinfo, (UINT,ULONG, ITypeInfo**));
    NV_DECLARE_TEAROFF_METHOD(Invoke, invoke, (
            DISPID dispidMember,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS FAR* pdispparams,
            VARIANT FAR* pvarResult,
            EXCEPINFO FAR* pexcepinfo,
            UINT FAR* puArgErr));

    // IHTMLLocation methods
    NV_STDMETHOD(LocationGetTypeInfoCount)(UINT FAR* pctinfo);
    NV_STDMETHOD(LocationGetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo);
    NV_STDMETHOD(LocationGetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID FAR *rgdispid);
    NV_STDMETHOD(LocationInvoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr);
    NV_STDMETHOD(put_href)(BSTR bstr);
    NV_STDMETHOD(get_href)(BSTR *pbstr);
    NV_STDMETHOD(put_protocol)(BSTR bstr);
    NV_STDMETHOD(get_protocol)(BSTR *pbstr);
    NV_STDMETHOD(put_host)(BSTR bstr);
    NV_STDMETHOD(get_host)(BSTR *pbstr);
    NV_STDMETHOD(put_hostname)(BSTR bstr);
    NV_STDMETHOD(get_hostname)(BSTR *pbstr);
    NV_STDMETHOD(put_port)(BSTR bstr);
    NV_STDMETHOD(get_port)(BSTR *pbstr);
    NV_STDMETHOD(put_pathname)(BSTR bstr);
    NV_STDMETHOD(get_pathname)(BSTR *pbstr);
    NV_STDMETHOD(put_search)(BSTR bstr);
    NV_STDMETHOD(get_search)(BSTR *pbstr);
    NV_STDMETHOD(put_hash)(BSTR bstr);
    NV_STDMETHOD(get_hash)(BSTR *pbstr);
    NV_STDMETHOD(reload)(VARIANT_BOOL fFlag);
    NV_STDMETHOD(replace)(BSTR bstr);
    NV_STDMETHOD(assign)(BSTR bstr);
    NV_STDMETHOD(toString)(BSTR *pbstr);

    // IDispatchEx methods
    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));
    DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (
            DISPID dispidMember,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            IServiceProvider *pSrvProvider ));
    NV_DECLARE_TEAROFF_METHOD(DeleteMemberByName, deletememberbyname, (BSTR bstr,DWORD grfdex));
    NV_DECLARE_TEAROFF_METHOD(DeleteMemberByDispID, deletememberbydispid, (DISPID id));

    NV_DECLARE_TEAROFF_METHOD(GetMemberProperties, getmemberproperties, (
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex));
    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (DISPID id, BSTR *pbstrName));
    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (
                DWORD grfdex,
                DISPID id,
                DISPID *prgid));
    NV_DECLARE_TEAROFF_METHOD(GetNameSpaceParent, getnamespaceparent, (IUnknown **ppunk));

    // IMarshal methods
    NV_DECLARE_TEAROFF_METHOD(MarshalInterface, marshalinterface, (IStream *pistm,REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags));

    // IObjectIdentity methods
    NV_DECLARE_TEAROFF_METHOD(IsEqualObject, isequalobject, (IUnknown *ppunk));

    // IServiceProvider methods
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));

    // Helper
    COmWindowProxy *    MyWindowProxy();
};


//+------------------------------------------------------------------------
//
//  Class:      COmWindowProxy
//
//  Purpose:    The automatable window (one per doc).
//
//-------------------------------------------------------------------------

class COmWindowProxy :  public CBase,
                        public IHTMLWindow2,
                        public IDispatchEx,
                        public IHTMLWindow3
{
    DECLARE_CLASS_TYPES(COmWindowProxy, CBase)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(COmWindowProxy))
    DECLARE_TEAROFF_TABLE(IMarshal)
	DECLARE_TEAROFF_TABLE(IServiceProvider)

    // CBase methods
    DECLARE_PLAIN_IUNKNOWN(COmWindowProxy)

    // ctor/dtor
    COmWindowProxy();
    
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}
    virtual void                Passivate();

    // Misc. helpers
    HRESULT SecureObject(VARIANT *pvarIn, VARIANT *pvarOut, IServiceProvider *pSrvProvider, BOOL fSecurityCheck);
    HRESULT SecureObject(IHTMLWindow2 *pWindowIn, IHTMLWindow2 **ppWindowOut);
    HRESULT Init(IHTMLWindow2 *pWindow, BYTE *pbSID, DWORD cbSID);
    BOOL    AccessAllowed();
    BOOL    AccessAllowed(IDispatch *pDisp);
    BOOL    AccessAllowedToNamedFrame(LPCTSTR pchTarget);

    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // so that we can fire with and without an eventParam
    HRESULT FireEvent(DISPID dispidMethod, DISPID dispidProp, 
                  LPCTSTR pchEventType, 
                  BYTE * pbTypes, ...);

    // pdl hook up
    #define _COmWindowProxy_
    #include "window.hdl"

    // IDispatch methods:
    STDMETHOD(GetTypeInfoCount)         (UINT FAR* pctinfo);
    STDMETHOD(GetTypeInfo)              (
                UINT itinfo, 
                LCID lcid, 
                ITypeInfo ** pptinfo);
    STDMETHOD(GetIDsOfNames)            (
                REFIID                riid,
                LPOLESTR *            rgszNames,
                UINT                  cNames,
                LCID                  lcid,
                DISPID FAR*           rgdispid);
    STDMETHOD(Invoke)                   (
                DISPID          dispidMember,
                REFIID          riid,
                LCID            lcid,
                WORD            wFlags,
                DISPPARAMS *    pdispparams,
                VARIANT *       pvarResult,
                EXCEPINFO *     pexcepinfo,
                UINT *          puArgErr);

    // IDispatchEx methods
    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID *pid);

    STDMETHOD (InvokeEx)(
            DISPID dispidMember,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            IServiceProvider *pSrvProvider);

    HRESULT STDMETHODCALLTYPE DeleteMemberByName(BSTR bstr,DWORD grfdex);
    HRESULT STDMETHODCALLTYPE DeleteMemberByDispID(DISPID id);

    STDMETHOD(GetMemberProperties)(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex);

    STDMETHOD(GetMemberName) (DISPID id, BSTR *pbstrName);

    STDMETHOD(GetNextDispID)(
                DWORD grfdex,
                DISPID id,
                DISPID *prgid);

    STDMETHOD(GetNameSpaceParent)(IUnknown **ppunk);
   
    //  IHTMLWindow2
    STDMETHOD(item)(VARIANT* pvarIndex,VARIANT* pvarResult);
    STDMETHOD(get_length)(long*p);
    STDMETHOD(get_frames)(IHTMLFramesCollection2**p);
    STDMETHOD(put_defaultStatus)(BSTR v);
    STDMETHOD(get_defaultStatus)(BSTR*p);
    STDMETHOD(put_status)(BSTR v);
    STDMETHOD(get_status)(BSTR*p);
    STDMETHOD(setTimeout)(BSTR expression,long msec,VARIANT* language,long* timerID);
    STDMETHOD(setTimeout)(VARIANT *pCode, long msec,VARIANT* language,long* timerID);
    STDMETHOD(clearTimeout)(long timerID);
    STDMETHOD(alert)(BSTR message);
    STDMETHOD(confirm)(BSTR message, VARIANT_BOOL* confirmed);
    STDMETHOD(prompt)(BSTR message, BSTR defstr, VARIANT* textdata);
    STDMETHOD(navigate)(BSTR url);
    STDMETHOD(get_Image)(IHTMLImageElementFactory **p);
    STDMETHOD(get_location)(IHTMLLocation**p);
    STDMETHOD(get_history)(IOmHistory**p);
    STDMETHOD(close)();
    STDMETHOD(put_opener)(VARIANT v);
    STDMETHOD(get_opener)(VARIANT*p);
    STDMETHOD(get_navigator)(IOmNavigator**p);
    STDMETHOD(put_name)(BSTR v);
    STDMETHOD(get_name)(BSTR*p);
    STDMETHOD(get_parent)(IHTMLWindow2**p);
    STDMETHOD(open)(BSTR url, BSTR name, BSTR features, VARIANT_BOOL replace, IHTMLWindow2** pomWindowResult);
    STDMETHOD(get_self)(IHTMLWindow2**p);
    STDMETHOD(get_top)(IHTMLWindow2**p);
    STDMETHOD(get_window)(IHTMLWindow2**p);
    STDMETHOD(get_document)(IHTMLDocument2**p);
    STDMETHOD(get_event)(IHTMLEventObj**p);
    STDMETHOD(get__newEnum)(IUnknown**p);
    STDMETHOD(showModalDialog)(BSTR dialog,VARIANT* varArgIn,VARIANT* varOptions,VARIANT* varArgOut);
    STDMETHOD(showHelp)(BSTR helpURL,VARIANT helpArg,BSTR features);
    STDMETHOD(focus)();
    STDMETHOD(blur)();
    STDMETHOD(scroll)(long x, long y);
    STDMETHOD(get_screen)(IHTMLScreen**p);
    STDMETHOD(get_Option)(IHTMLOptionElementFactory **p);
    STDMETHOD(get_closed)(VARIANT_BOOL *p);
    STDMETHOD(get_clientInformation)(IOmNavigator**p);
    STDMETHOD(setInterval)(BSTR expression,long msec,VARIANT* language,long* timerID);
    STDMETHOD(setInterval)(VARIANT *pCode,long msec,VARIANT* language,long* timerID);
    STDMETHOD(clearInterval)(long timerID);
    STDMETHOD(put_offscreenBuffering)(VARIANT var);
    STDMETHOD(get_offscreenBuffering)(VARIANT *pvar);
    STDMETHOD(execScript)(BSTR strCode, BSTR strLanguage, VARIANT * pvarRet);
    STDMETHOD(toString)(BSTR *);
    STDMETHOD(scrollTo)(long x, long y);
    STDMETHOD(scrollBy)(long x, long y);
    STDMETHOD(moveTo)(long x, long y);
    STDMETHOD(moveBy)(long x, long y);
    STDMETHOD(resizeTo)(long x, long y);
    STDMETHOD(resizeBy)(long x, long y);
    STDMETHOD(get_external)(IDispatch **ppDisp);

    // IHTMLWindow3
    STDMETHOD(get_screenLeft)(long *pVal);
    STDMETHOD(get_screenTop)(long *pVal);
    STDMETHOD(attachEvent)(BSTR event, IDispatch* pDisp, VARIANT_BOOL *pResult);
    STDMETHOD(detachEvent)(BSTR event, IDispatch *pDisp);
    STDMETHOD(print)();
    STDMETHOD(get_clipboardData)(IHTMLDataTransfer **p);
    STDMETHOD(showModelessDialog)(BSTR strUrl, 
                                    VARIANT * pvarArgIn, 
                                    VARIANT * pvarOptions, 
                                    IHTMLWindow2 ** ppDialog);

    // IMarshal methods
    NV_DECLARE_TEAROFF_METHOD(GetUnmarshalClass, getunmarshalclass, (REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,CLSID *pCid));
    NV_DECLARE_TEAROFF_METHOD(GetMarshalSizeMax, getmarshalsizemax, (REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,DWORD *pSize));
    NV_DECLARE_TEAROFF_METHOD(MarshalInterface, marshalinterface, (IStream *pistm,REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags));
    NV_DECLARE_TEAROFF_METHOD(UnmarshalInterface, unmarshalinterface, (IStream *pistm,REFIID riid,void ** ppvObj));
    NV_DECLARE_TEAROFF_METHOD(ReleaseMarshalData, releasemarshaldata, (IStream *pStm));
    NV_DECLARE_TEAROFF_METHOD(DisconnectObject, disconnectobject, (DWORD dwReserved));

    // IObjectIdentity methods
    NV_DECLARE_TEAROFF_METHOD(IsEqualObject, isequalobject, (IUnknown *ppunk));

    // IServiceProvider methods
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));

    // misc
    COmWindow2 * Window();
    HRESULT ValidateMarshalParams(REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags);
    BOOL    CanMarshalIID(REFIID riid);
    HRESULT MarshalInterface(BOOL fWindow, IStream *pistm,REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags);
    BOOL IsOleProxy();

    // Event fire method declarations for eventset
    void Fire_onload();
    void Fire_onunload();
    BOOL Fire_onbeforeunload();
    void BUGCALL Fire_onfocus(DWORD_PTR dwContext);
    void BUGCALL Fire_onblur(DWORD_PTR dwContext);
    BOOL Fire_onerror(BSTR bstrMessage, BSTR bstrUrl,
                      long lLine, long lCharacter, long lCode,
                      BOOL fWindow);
    void Fire_onscroll();
    BOOL Fire_onhelp(BYTE * pbTypes = (BYTE *) VTS_NONE, ... );
    void Fire_onresize();
    void Fire_onbeforeprint();
    void Fire_onafterprint();


    // Data members
    IHTMLWindow2 *              _pWindow;       // The real html window
    BYTE *                      _pbSID;         // The security identifier
    DWORD                       _cbSID;         // Number of bytes in SID
    COmLocationProxy            _Location;      // Proxy location obj.
    BOOL                        _fTrustedDoc;   // TRUE if proxy for trusted doc.

    unsigned                    _fTrusted : 1;  // Optimization of Invoke.
    
#ifdef OBJCNTCHK
    DWORD                       _dwObjCnt;
#endif

    // Static members
    static const CONNECTION_POINT_INFO  s_acpi[];
    static const CLASSDESC              s_classdesc;
};


struct WINDOWTBL
{
    void    Free();
    
    IHTMLWindow2 *  pWindow;        // IHTMLWindow2 of the underlying 
                                    //   window 
    BYTE *          pbSID;          // The security id 
    DWORD           cbSID;          // Length of security id
    BOOL            fTrust;         // Is this entry for a trusted proxy?
    IHTMLWindow2 *  pProxy;         // IHTMLWindow2 of the proxy for
                                    //   above combination.
};


class CAryWindowTbl : public CDataAry<struct WINDOWTBL> 
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAryWindowTbl))
    CAryWindowTbl() : CDataAry<struct WINDOWTBL>(Mt(CAryWindowTbl_pv)) {}
    HRESULT FindProxy(
        IHTMLWindow2 *pWindowIn, 
        BYTE *pbSID, 
        DWORD cbSID, 
        BOOL fTrust,
        IHTMLWindow2 **ppProxyOut);
    HRESULT AddTuple(
        IHTMLWindow2 *pWindow, 
        BYTE *pbSID, 
        DWORD cbSID,
        BOOL fTrust,
        IHTMLWindow2 *pProxy);
    void DeleteProxyEntry(IHTMLWindow2 *pProxy);
};


inline COmWindowProxy *
COmLocationProxy::MyWindowProxy()
{ 
    return CONTAINING_RECORD(this, COmWindowProxy, _Location);
}


inline ULONG
COmLocationProxy::AddRef()
{ 
    return MyWindowProxy()->AddRef();
}


inline ULONG
COmLocationProxy::Release()
{ 
    return MyWindowProxy()->Release();
}

#pragma INCMSG("--- End 'window.hxx'")
#else
#pragma INCMSG("*** Dup 'window.hxx'")
#endif
