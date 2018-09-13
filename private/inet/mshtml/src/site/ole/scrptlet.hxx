//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       scriptlet.hxx
//
//  History:    19-Jan-1998     sramani     Created
//
//  Contents:   CScriptlet definition
//
//----------------------------------------------------------------------------

#ifndef I_SCRPTLET_HXX_
#define I_SCRPTLET_HXX_
#pragma INCMSG("--- Beg 'scrptlet.hxx'")

#ifndef X_SCRSBOBJ_HXX_
#define X_SCRSBOBJ_HXX_
#include "scrsbobj.hxx"
#endif

// Forward declarations

class CScriptControl;

HRESULT Property_get(IDispatch * pDisp, DISPID dispid, VARIANT * pvar);
inline HRESULT IdFromName(IDispatch * pdisp, LPOLESTR pstr, DISPID * pdispid)
{ 
    return pdisp->GetIDsOfNames(IID_NULL, &pstr, 1, LOCALE_USER_DEFAULT, pdispid);
}

// Name of the magic object in script that defines the control's interface

#define EXTERNAL_DESCRIPTION    _T("public_description")
#define DISPID_VECTOR_BASE	    10

#define _hxx_
#include "scrptlet.hdl"
   
MtExtern(CSortedAtomTable)
MtExtern(CSortedAtomTable_pv)
MtExtern(CSortedAtomTable_aryIndex_pv)

class CSortedAtomTable : protected CDataAry<CStr>
{
public:
    CSortedAtomTable() : CDataAry<CStr>(Mt(CSortedAtomTable_pv)), _aryIndex(Mt(CSortedAtomTable_aryIndex_pv)) {}
    HRESULT Insert(LPCTSTR pch, LONG lInsertAt, LONG *plIndex);
    BOOL    Find(LPCTSTR pch, LONG *plIndex, BOOL fCaseSensitive);
    void    Free();

private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
    CDataAry<WORD> _aryIndex;
};

/////////////////////////////////////////////////////////////////////////////
// CScriptlet

MtExtern(CScriptlet)

class CScriptlet : public CBase
{
    DECLARE_CLASS_TYPES(CScriptlet, CBase)

    friend class CScriptControl;
    friend class CScriptletSubObjects;

public: 
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CScriptlet))

    DECLARE_TEAROFF_TABLE(IOleObject)
    DECLARE_TEAROFF_TABLE(IOleControl)
    DECLARE_TEAROFF_TABLE(IOleInPlaceObject)
    DECLARE_TEAROFF_TABLE(IPersistStreamInit)
    DECLARE_TEAROFF_TABLE(IPersistPropertyBag)

    CScriptlet(IUnknown *pUnkOuter);
    ~CScriptlet();

    DECLARE_AGGREGATED_IUNKNOWN(CScriptlet)
    DECLARE_PRIVATE_QI_FUNCS(CScriptlet)
    DECLARE_DERIVED_DISPATCH(CBase)
    
    // IDispatchEx method over-rides on CDoc

    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));
    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (DISPID dispid,
                           LCID lcid,
                           WORD wFlags,
                           DISPPARAMS *pdispparams,
                           VARIANT *pvarResult,
                           EXCEPINFO *pexcepinfo,
                           IServiceProvider *pSrvProvider));

    // IOleObject
    NV_DECLARE_TEAROFF_METHOD(SetClientSite, setclientsite, (IOleClientSite *pClientSite));
    NV_DECLARE_TEAROFF_METHOD(DoVerb, doverb, (LONG iVerb, LPMSG lpmsg, IOleClientSite* pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect));
    NV_DECLARE_TEAROFF_METHOD(GetUserClassID, getuserclassid, (CLSID * pClsid));
    NV_DECLARE_TEAROFF_METHOD(GetUserType, getusertype, (DWORD dwFormOfType, LPTSTR * pszUserType));
    NV_DECLARE_TEAROFF_METHOD(SetExtent, setextent, (DWORD dwDrawAspect, LPSIZEL lpsizel));
    NV_DECLARE_TEAROFF_METHOD(GetExtent, getextent, (DWORD dwDrawAspect, LPSIZEL lpsizel));

    NV_DECLARE_TEAROFF_METHOD(GetClientSite, getclientsite, (IOleClientSite **ppClientSite))
        { return _pDoc->GetClientSite(ppClientSite); }
    NV_DECLARE_TEAROFF_METHOD(SetHostNames, sethostnames, (LPCOLESTR szContainerApp, LPCOLESTR szContainerObj))
        { return _pDoc->SetHostNames(szContainerApp, szContainerObj); }
    NV_DECLARE_TEAROFF_METHOD(Close, close, (DWORD dwSaveOption))
        { return _pDoc->Close(dwSaveOption); }
    NV_DECLARE_TEAROFF_METHOD(SetMoniker, setmoniker, (DWORD dwWhichMoniker, IMoniker* pmk))
        { return _pDoc->SetMoniker(dwWhichMoniker, pmk); }
    NV_DECLARE_TEAROFF_METHOD(GetMoniker, getmoniker, (DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk))
        { return _pDoc->GetMoniker(dwAssign, dwWhichMoniker, ppmk); }
    NV_DECLARE_TEAROFF_METHOD(InitFromData, initfromdata, (IDataObject* pDataObject, BOOL fCreation, DWORD dwReserved))
        { return _pDoc->InitFromData(pDataObject, fCreation, dwReserved); }
    NV_DECLARE_TEAROFF_METHOD(GetClipboardData, getclipboarddata, (DWORD dwReserved, IDataObject** ppDataObject))
        { return _pDoc->GetClipboardData(dwReserved, ppDataObject); }

    NV_DECLARE_TEAROFF_METHOD(EnumVerbs, enumverbs, (IEnumOLEVERB **ppEnumOleVerb))
        { return _pDoc->EnumVerbs(ppEnumOleVerb); }
    NV_DECLARE_TEAROFF_METHOD(Update, update, ())
        { return _pDoc->Update(); }
    NV_DECLARE_TEAROFF_METHOD(IsUpToDate, isuptodate, ())
        { return _pDoc->IsUpToDate(); }

    NV_DECLARE_TEAROFF_METHOD(Advise, advise, (IAdviseSink * pAdvSink, DWORD * pdwConnection))
        { return _pDoc->Advise(pAdvSink, pdwConnection); }
    NV_DECLARE_TEAROFF_METHOD(Unadvise, unadvise, (DWORD dwConnection))
        { return _pDoc->Unadvise(dwConnection); }
    NV_DECLARE_TEAROFF_METHOD(EnumAdvise, enumadvise, (LPENUMSTATDATA * ppenumAdvise))
        { return _pDoc->EnumAdvise(ppenumAdvise); }
    NV_DECLARE_TEAROFF_METHOD(GetMiscStatus, getmiscstatus, (DWORD dwAspect, DWORD * pdwStatus))
        { return _pDoc->GetMiscStatus(dwAspect, pdwStatus); }
    NV_DECLARE_TEAROFF_METHOD(SetColorScheme, setcolorscheme, (LOGPALETTE *pLogpal))
        { return _pDoc->SetColorScheme(pLogpal); }

    // IOleControl
    
    NV_DECLARE_TEAROFF_METHOD(GetControlInfo, getcontrolinfo, (CONTROLINFO *ctrlInfo))
        { return _pDoc->GetControlInfo(ctrlInfo); }
    NV_DECLARE_TEAROFF_METHOD(OnMnemonic, onmnemonic, (MSG *msg))
        { return _pDoc->OnMnemonic(msg); }
    NV_DECLARE_TEAROFF_METHOD(OnAmbientPropertyChange, onambientpropertychange, (DISPID dispid))
        { return _pDoc->OnAmbientPropertyChange(dispid); }
    NV_DECLARE_TEAROFF_METHOD(FreezeEvents, freezeevents, (BOOL fFreeze));

    // IOleInPlaceObject

    NV_DECLARE_TEAROFF_METHOD(GetWindow, getwindow, (HWND * phwnd))
        { return _pDoc->GetWindow(phwnd); }
    NV_DECLARE_TEAROFF_METHOD(ContextSensitiveHelp, contextsensitivehelp, (BOOL fHelp))
        { return _pDoc->ContextSensitiveHelp(fHelp); }
    NV_DECLARE_TEAROFF_METHOD(InPlaceDeactivate, inplacedeactivate, ());
    NV_DECLARE_TEAROFF_METHOD(UIDeactivate, uideactivate, ())
        { return _pDoc->UIDeactivate(); }
    NV_DECLARE_TEAROFF_METHOD(ReactivateAndUndo, reactivateandundo, ())
        { return _pDoc->ReactivateAndUndo(); }
    NV_DECLARE_TEAROFF_METHOD(SetObjectRects, setobjectrects, (LPCRECT prcPos, LPCRECT prcClip));

    // IPersistPropertyBag
    NV_DECLARE_TEAROFF_METHOD(GetClassID, getclassid, (CLSID *pClassID));
    NV_DECLARE_TEAROFF_METHOD(InitNew, initnew, ());
    NV_DECLARE_TEAROFF_METHOD(Load, LOAD, (LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog));
    NV_DECLARE_TEAROFF_METHOD(Save, SAVE, (LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties));

    // IPersistStreamInit
    NV_DECLARE_TEAROFF_METHOD(IsDirty, isdirty, ());
    NV_DECLARE_TEAROFF_METHOD(Load, LOAD, (LPSTREAM pStm));
    NV_DECLARE_TEAROFF_METHOD(Save, SAVE, (LPSTREAM pStm, BOOL fClearDirty));
    NV_DECLARE_TEAROFF_METHOD(GetSizeMax, getsizemax, (ULARGE_INTEGER FAR* pcbSize))
        { return E_NOTIMPL; }

    // pdl hook up
    #define _CScriptlet_
    #include "scrptlet.hdl"

    static const CONNECTION_POINT_INFO  s_acpi[];
    static const CBase::CLASSDESC s_classdesc;
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    virtual HRESULT Init();
    virtual void Passivate();
    IUnknown * PunkOuter() { return _pUnkOuter; }

private: 
    enum 
    {
        WEBBRIDGE_DEFAULT_WIDTH = 200, 
        WEBBRIDGE_DEFAULT_HEIGHT = 123      // Golden Ratio = (sqrt(5) - 1) / 2
    };

    HRESULT LoadScriptletURL(TCHAR *pchUrl = NULL);
    void    OnVisibilityChange();
    BOOL    PassThruDISPID(DISPID dispid);
    BOOL    InDesignMode();
    HRESULT FireEvent(DISPID dispidEvent, DISPID dispidProp, BYTE * pbTypes, ...);
    HRESULT GetStyleProperty(IHTMLStyle **ppHTMLStyle);
    HRESULT Resize();
    void    SetWidth(DISPID dispid);
    void    SetHeight(DISPID dispid);
    void    OnReadyStateChange();

    // "ScriptControl" is a parallel object that we maintain.  It is inserted
    // into the script namespace so script components can fire events and talk to
    // the WebBridge.
    CScriptControl             *_pScriptCtrl;
    IUnknown                   *_pTrident;
    CDoc                       *_pDoc;
    IOleClientSite             *_pOCS;
    IUnknown                   *_pUnkOuter;         //  Outer unknown
    IDispatch                  *_pDescription;
    IHTMLElement               *_pHTMLElement;
    CScriptletSubObjects        _ScriptletSubObjects;

    // Property data members.

    CStr                        _cstrUrl;
    VARIANT_BOOL                _vbScrollbar;
    VARIANT_BOOL                _vbEmbedded;
    VARIANT_BOOL                _vbSelectable;
    VARIANT                     _varOnVisChange;
    SIZEL                       _sizePixExtent;
    int                         _cFreezes;
    
    unsigned                    _fValidCx:1;
    unsigned                    _fValidCy:1;
    unsigned                    _fIsVisible:1;
    unsigned                    _fHardWiredURL:1;
    unsigned                    _fRequiresSave:1;
    unsigned                    _fDelayOnReadyStateFiring:1;
    unsigned                    _fExtentSet:1; // Has SetExtent() been called on scriptlet's Doc?

    struct DispidRecord
    {
        DISPID  dispid_get;
        DISPID  dispid_put;
        DISPID  dispidBare;     // no get_ or put_ functions found. So we are just a property.
    };

    CDataAry<DispidRecord>  _aryDR;
    CSortedAtomTable        _aryDispid;
    DISPID                  _dispidCur;
};

#pragma INCMSG("--- End 'scrptlet.hxx'")
#else
#pragma INCMSG("*** Dup 'scrptlet.hxx'")
#endif
