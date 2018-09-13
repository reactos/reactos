//===========================================================================
//
//   File : Persist.hxx
//
//   contents : the implementation of the persistData xtag 
//              and the various persistence behaviours derived from that.
//
//===========================================================================

#ifndef __PERSIST_HXX_
#define __PERSIST_HXX_

#include "resource.h"     // main symbols

#ifndef __ACTIVSCP_H_
#define __ACTIVSCP_H_
#include "activscp.h"     // for IActiveScriptSite
#endif

#ifndef __MSHTMHST_H_
#define __MSHTMHST_H_
#include <mshtmhst.h>    // for IElementBehavior
#endif

#ifndef __UTILS_H_
#define __UTILS_H_
#include "utils.hxx"
#endif

#ifndef __IEXTAG_H_
#define __IEXTAG_H_
#include "iextag.h"
#endif

enum ENUM_SAVE_CATEGORY {
    ESC_UNKNOWN,
    ESC_PASSWORD,
    ESC_INTRINSIC,
    ESC_CONTROL,
    ESC_SCRIPT,
};

#define SCRIPT_ENGINE_JSCRIPT       0x0001
#define SCRIPT_ENGINE_VBSCRIPT      SCRIPT_ENGINE_JSCRIPT  <<1
#define SCRIPT_ENGINE_OTHER         SCRIPT_ENGINE_VBSCRIPT <<1
   
#define INIT_USE_CACHED             0x0001

//=====================================================
//
// Class : CPersistDataPeer
//
// Purpose : base class for many of the persistence peers
//   its job is to do the consistent processing
//
//=====================================================

class ATL_NO_VTABLE CPersistDataPeer : 
        public CComObjectRootEx<CComSingleThreadModel>,
        public CComCoClass<CPersistDataPeer, &CLSID_CPersistDataPeer>,
        public IDispatchImpl<IHTMLPersistDataOM, &IID_IHTMLPersistDataOM, &LIBID_IEXTagLib>,
        public IElementBehavior,
        public IHTMLPersistData,
        public IActiveScriptSite
{
public:
    CPersistDataPeer() 
    {
        _pPeerSite = NULL;
        _pPeerSiteOM = NULL;
        _pInnerXMLDoc = NULL;
        _pRoot = NULL;
        _eState = htmlPersistStateNormal;
    };
    ~CPersistDataPeer()
    {
        ClearInterface(&_pPeerSite);
        ClearInterface(&_pPeerSiteOM);
        ClearOMInterfaces();
    }

    BEGIN_COM_MAP(CPersistDataPeer)
        COM_INTERFACE_ENTRY(IElementBehavior)
        COM_INTERFACE_ENTRY(IActiveScriptSite)
        COM_INTERFACE_ENTRY(IHTMLPersistDataOM)
        COM_INTERFACE_ENTRY(IHTMLPersistData)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    // 
    // IUnknown
    //----------------------------------------------
    STDMETHOD_(ULONG, AddRef)() 
            { return InternalAddRef();};
    STDMETHOD_(ULONG, Release)()
            {
                ULONG l = InternalRelease();
                if (l == 0) 
                    delete this; 
                return l;
            };
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
            { return _InternalQueryInterface(iid, ppvObject); };

    //
    // IElementBehavior
    //---------------------------------------------
    STDMETHOD(Init)(IElementBehaviorSite *pPeerSite);
    STDMETHOD(Notify)(LONG, VARIANT *);
    STDMETHOD(Detach)() { return S_OK; };


    // IActiveScriptSite methods
    //----------------------------------------------
    STDMETHOD(GetLCID)(LCID *plcid)  
            { return S_OK; };
    STDMETHOD(GetItemInfo)(LPCOLESTR pstrName, 
                           DWORD dwReturnMask, 
                           IUnknown **ppiunkItem, 
                           ITypeInfo **ppti) 
            { return S_OK; };
    STDMETHOD(GetDocVersionString)(BSTR *pszVersion)
            { return S_OK; };
    STDMETHOD(RequestItems)(void)
            { return S_OK; };
    STDMETHOD(RequestTypeLibs)(void)
            { return S_OK; };
    STDMETHOD(OnScriptTerminate)(const VARIANT *pvarResult, 
                                 const EXCEPINFO *pexcepinfo)
            { return S_OK; };
    STDMETHOD(OnStateChange)(SCRIPTSTATE ssScriptState)
            { return S_OK; };
    STDMETHOD(OnScriptError)(IActiveScriptError *pscripterror)
            { return S_OK; };
    STDMETHOD(OnEnterScript)(void)
            { return S_OK; };
    STDMETHOD(OnLeaveScript)(void)
            { return S_OK; };

    
    //
    // IHTMLPersistDataOM interfaces
    //----------------------------------------------
    STDMETHOD(get_XMLDocument)(IDispatch **ppDisp);
    STDMETHOD(getAttribute)(BSTR strAttr, VARIANT * pvar);
    STDMETHOD(setAttribute)(BSTR strAttr, VARIANT var);
    STDMETHOD(removeAttribute)(BSTR strAttr);


    //
    // IHTMLPersistData
    //-----------------------------------------------
    virtual STDMETHODIMP save(IUnknown *pUnk, long lType, VARIANT_BOOL* pfContinue);
    virtual STDMETHODIMP load(IUnknown *pUnk, long lType, VARIANT_BOOL* pfContinue);
            STDMETHODIMP queryType(long pl, VARIANT_BOOL *pvBool);

protected:
    //
    // Helper functions
    //----------------------------------------------
    HRESULT FireEvent(BSTR bstrEvent, VARIANT_BOOL *pfc, BOOL fIsSaveEvent);
    HRESULT InitOM() { return (!_pInnerXMLDoc || !_pRoot) ? E_PENDING : S_OK; };
    HRESULT InitOM(IUnknown *pUnk, long lType, DWORD dwFlags=0 );
    void    ClearOMInterfaces();

    //
    // Handlers for saving tag types :
    //      generic  - outerHTML save/load operations
    //      script   - BuildNewScriptBlock save/load
    //----------------------------------------------------
    HRESULT SaveHandler_GenericTag();
    HRESULT LoadHandler_GenericTag();
    HRESULT SaveHandler_ScriptTag();
    HRESULT LoadHandler_ScriptTag();

    //
    // script save helpers
    //----------------------------------------------
    ENUM_SAVE_CATEGORY GetSaveCategory    ();
    IActiveScript     *GetScriptEngine    (IHTMLDocument2 * pBrowseDoc, ULONG *puFlag);
    HRESULT            GetEngineClsidForLanguage(CLSID * pcslid,
                                           IHTMLDocument2 * pBrowseDoc);
    HRESULT            BuildNewScriptBlock(CBufferedStr * pstrBuffer, ULONG *puFlags);


    //
    // Member data
    //----------------------------------------------
    IElementBehaviorSite   * _pPeerSite;
    IElementBehaviorSiteOM * _pPeerSiteOM;

    IXMLDOMDocument        * _pInnerXMLDoc;  // XML object
    IXMLDOMElement         * _pRoot;         // root of xml documnt
    htmlPersistState         _eState;        // what persist event is this?

};

//====================================================
//
// Class : CPersistShortcut
//
//====================================================

class CPersistShortcut: 
        public CPersistDataPeer
{
    typedef CPersistDataPeer super;

public:
    CPersistShortcut() 
        {   _eState = htmlPersistStateFavorite; };

    DECLARE_REGISTRY_RESOURCEID(IDR_CPERSISTFAVORITE)

    // IHTMLPersistData overrides
    //----------------------------------------------------
    virtual STDMETHODIMP save(IUnknown *pUnk, long lType, VARIANT_BOOL* pfContinue);
    virtual STDMETHODIMP load(IUnknown *pUnk, long lType, VARIANT_BOOL* pfContinue);
};

//==============================
//
// Class : CPersistHistory
//
//==============================
class CPersistHistory: 
        public CPersistDataPeer
{
    typedef CPersistDataPeer super;

public:
    CPersistHistory()
        {    _eState = htmlPersistStateHistory; }

    DECLARE_REGISTRY_RESOURCEID(IDR_CPERSISTHISTORY)

    // IHTMLPersistData overrides
    //----------------------------------------------------
    virtual STDMETHODIMP save(IUnknown *pUnk, long lType, VARIANT_BOOL* pfContinue);
    virtual STDMETHODIMP load(IUnknown *pUnk, long lType, VARIANT_BOOL* pfContinue);
};

//==============================
//
// Class : CPersistSnapshot
//
//==============================

class CPersistSnapshot: 
        public CPersistDataPeer
{
    typedef CPersistDataPeer super;

public:
    CPersistSnapshot()
        {    _eState = htmlPersistStateSnapshot; }

    DECLARE_REGISTRY_RESOURCEID(IDR_CPERSISTSNAPSHOT)

    // IHTMLPersistData overrides
    //----------------------------------------------------
    virtual STDMETHODIMP save(IUnknown *pUnk, long lType, VARIANT_BOOL* pfContinue);
    virtual STDMETHODIMP load(IUnknown *pUnk, long lType, VARIANT_BOOL* pfContinue);


protected:
    // Helper methods
    //----------------------------------------------
    HRESULT       TransferScriptValues   (IHTMLDocument2 *pIDoc);
    HRESULT       TransferIntrinsicValues(IHTMLDocument2 *pIDoc);
    HRESULT       TransferControlValues  (IHTMLDocument2 *pIDoc);
    IHTMLElement *GetDesignElem          (IHTMLDocument2 *pDesignDoc);
};
#endif  //__PERSIST_HXX_




