//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       pad.hxx
//
//  Contents:   Definitions for the Trident application
//
//-------------------------------------------------------------------------

#ifndef _PAD_HXX_
#define _PAD_HXX_

#ifndef X_PADRC_H_
#define X_PADRC_H_
#include "padrc.h"
#endif

#ifndef X_F3PAD_H_
#define X_F3PAD_H_
#include "f3pad.h"
#endif

#ifndef X_PADDISP_H_
#define X_PADDISP_H_
#include "paddisp.h"
#endif

#ifndef X_TESTEVNT_HXX_
#define X_TESTEVNT_HXX_
#include "testevnt.hxx"
#endif

#ifndef X_URLHIST_H_
#define X_URLHIST_H_
#include "urlhist.h"
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include <shlguid.h>        // for UrlHistory guid defs
#endif

#ifndef X_DWNNOT_H_
#define X_DWNNOT_H_
#include <dwnnot.h>
#endif

#ifndef X_SHLOBJ_H_
#define X_SHLOBJ_H_
#include <shlobj.h>
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include <shellapi.h>
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

extern "C" const CLSID CLSID_HTMLDocument;

typedef enum tagSAVEOPTS
{
	SAVEOPTS_SAVEIFDIRTY = 0,
	SAVEOPTS_NOSAVE = 1,
	SAVEOPTS_PROMPTSAVE = 2
} SAVEOPTS;

struct ComboItem {
        INT     iIdm;
        LONG    lData;
};

#undef ASSERT

MtExtern(CPadDoc)
MtExtern(CPadScriptSite)
PerfExtern(tagPerfWatchPad)

class CPadDoc;
class CDebugWindow;

#if defined(PRODUCT_PROF) && !defined(_MAC)
extern "C" void _stdcall StartCAPAll(void);
extern "C" void _stdcall StopCAPAll(void);
extern "C" void _stdcall SuspendCAPAll(void);
extern "C" void _stdcall ResumeCAPAll(void);
extern "C" void _stdcall StartCAP(void);
extern "C" void _stdcall StopCAP(void);
extern "C" void _stdcall SuspendCAP(void);
extern "C" void _stdcall ResumeCAP(void);
#else
inline void StartCAPAll() { }
inline void StopCAPAll() { }
inline void SuspendCAPAll() { }
inline void ResumeCAPAll() { }
inline void StartCAP() { }
inline void StopCAP() { }
inline void SuspendCAP() { }
inline void ResumeCAP() { }
#endif

//+------------------------------------------------------------------------
//
//  Class:      CPadFactory
//
//  Purpose:    Class factory for pad documents.
//
//-------------------------------------------------------------------------

class CPadFactory : public IClassFactory
{
public:
    CPadFactory(REFCLSID clsid,
                HRESULT (*pfnCreate)(IUnknown **),
                void (*pfnRevoke)() = NULL);

    STDMETHOD(QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG,AddRef)  ();
    STDMETHOD_(ULONG,Release) ();
    STDMETHOD (CreateInstance) (
            IUnknown * pUnkOuter,
            REFIID iid,
            void ** ppv);
    STDMETHOD (LockServer) (BOOL fLock);

    HRESULT       (*_pfnCreate)(IUnknown **);
    void          (*_pfnRevoke)();
    CLSID         _clsid;
    DWORD         _dwRegister;
    CPadFactory * _pFactoryNext;

    static CPadFactory *s_pFactoryFirst;    // Used only by the main thread.
    static HRESULT Register();
    static HRESULT Revoke();
};

//+------------------------------------------------------------------------
//
//  Class:      CPadBSC
//
//  Purpose:    Bind status callback
//
//-------------------------------------------------------------------------

class CPadBSC :
    public IBindStatusCallback
{
public:

    DECLARE_SUBOBJECT_IUNKNOWN(CPadDoc, PadDoc)

    // IBindStatusCallback methods

    STDMETHOD(OnStartBinding)(DWORD grfBSCOption, IBinding *pib);
    STDMETHOD(GetPriority)(LONG *pnPriority);
    STDMETHOD(OnLowResource)(DWORD reserved);
    STDMETHOD(OnProgress)( ULONG ulProgress, ULONG ulProgressMax,  ULONG ulStatusCode,  LPCWSTR szStatusText);
    STDMETHOD(OnStopBinding)(HRESULT hresult, LPCWSTR szError);
    STDMETHOD(GetBindInfo)(DWORD *grfBINDF, BINDINFO *pbindinfo);
    STDMETHOD(OnDataAvailable)(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM  *pstgmed);
    STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown *punk);
};

//+------------------------------------------------------------------------
//
//  Class:      CPadSite
//
//  Purpose:    OLE Site for embedded object
//
//-------------------------------------------------------------------------

class CPadSite :
        public IOleClientSite,
        public IOleInPlaceSite,
        public IDispatch,
        public IServiceProvider,
        public IOleDocumentSite,
        public IOleCommandTarget,
        public IOleControlSite,
        public IAdviseSink,
        public IDocHostUIHandler,
        public IElementBehaviorFactory,
        public IVersionHost        
{
public:
    DECLARE_SUBOBJECT_IUNKNOWN(CPadDoc, PadDoc)

    STDMETHOD(GetExternal) (IDispatch **ppDisp)
    {
        HRESULT     hr;

        if (!ppDisp)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            *ppDisp = NULL;
            hr = S_OK;
        }
   
        return hr;
    }

    STDMETHOD(TranslateUrl) (DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
    {
        HRESULT     hr;

        if (!ppchURLOut)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            *ppchURLOut = NULL;
            hr = S_OK;
        }
   
        return hr;
    }

    STDMETHOD(FilterDataObject) (IDataObject *pDO, IDataObject **ppDORet)
    {
        HRESULT     hr;

        if (!ppDORet)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            *ppDORet = NULL;
            hr = S_OK;
        }
       
        return hr;
    }

    // IOleClientSite methods
    STDMETHOD(SaveObject) (void);
    STDMETHOD(GetMoniker) (DWORD dwAssign, DWORD dwWhichMoniker,
                LPMONIKER * ppmk);
    STDMETHOD(GetContainer) (LPOLECONTAINER * ppContainer);
    STDMETHOD(ShowObject) (void);
    STDMETHOD(OnShowWindow) (BOOL fShow);
    STDMETHOD(RequestNewObjectLayout) (void);

    // IOleWindow methods
    STDMETHOD(GetWindow) (HWND * lphwnd);
    STDMETHOD(ContextSensitiveHelp) (BOOL fEnterMode);

    // IOleInPlaceSite methods
    STDMETHOD(CanInPlaceActivate)   (void);
    STDMETHOD(OnInPlaceActivate)    (void);
    STDMETHOD(OnUIActivate)         (void);
    STDMETHOD(GetWindowContext) (
        LPOLEINPLACEFRAME FAR *     lplpFrame,
        LPOLEINPLACEUIWINDOW FAR *  lplpDoc,
        LPOLERECT                   lprcPosRect,
        LPOLERECT                   lprcClipRect,
        LPOLEINPLACEFRAMEINFO       lpFrameInfo);
    STDMETHOD(Scroll)               (OLESIZE scrollExtent);
    STDMETHOD(OnUIDeactivate)       (BOOL fUndoable);
    STDMETHOD(OnInPlaceDeactivate)  (void);
    STDMETHOD(DiscardUndoState)     (void);
    STDMETHOD(DeactivateAndUndo)    (void);
    STDMETHOD(OnPosRectChange)      (LPCOLERECT lprcPosRect);

    // IDispatch methods

    STDMETHOD(GetTypeInfoCount) ( UINT * pctinfo );
    STDMETHOD(GetTypeInfo)  (UINT itinfo, LCID lcid, ITypeInfo ** pptinfo);
    STDMETHOD(GetIDsOfNames)(
            REFIID                riid,
            LPOLESTR *            rgszNames,
            UINT                  cNames,
            LCID                  lcid,
            DISPID *           rgdispid);
    STDMETHOD(Invoke)           (
        DISPID          dispidMember,
        REFIID          riid,
        LCID            lcid,
        WORD            wFlags,
        DISPPARAMS *    pdispparams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pexcepinfo,
        UINT *          puArgErr);

    // IServiceProvider methods

    STDMETHOD(QueryService) (REFGUID guid, REFIID iid, LPVOID * ppv);

    // IOleDocumentSite methods

    STDMETHOD(ActivateMe)(IOleDocumentView *pviewToActivate);

    // IOleCommandTarget methods

    STDMETHOD(QueryStatus)(
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext);

    STDMETHOD(Exec)(
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);

    // IOleControlSite methods

    STDMETHOD(OnControlInfoChanged)  (void);
    STDMETHOD(LockInPlaceActive)     (BOOL fLock);
    STDMETHOD(GetExtendedControl)    (IDispatch ** ppDisp);
    STDMETHOD(TransformCoords)(
        POINTL * pPtlHiMetric,
        POINTF * pPtfContainer,
        DWORD dwFlags);
    STDMETHOD(TranslateAccelerator)  (MSG * lpmsg, DWORD grfModifiers);
    STDMETHOD(OnFocus)               (BOOL fGotFocus);
    STDMETHOD(ShowPropertyFrame)     (void);

    // IAdviseSink methods
    void STDMETHODCALLTYPE OnViewChange(DWORD dwAspect, LONG lindex);
    void STDMETHODCALLTYPE OnDataChange(FORMATETC *petc, STGMEDIUM *pstgmed);
    void STDMETHODCALLTYPE OnRename(IMoniker *pmk);
    void STDMETHODCALLTYPE OnSave();
    void STDMETHODCALLTYPE OnClose();

    // IDocHostUIHandler methods
    STDMETHOD(GetHostInfo) (DOCHOSTUIINFO * pInfo);
    STDMETHOD(ShowUI) (
            DWORD dwID,
            IOleInPlaceActiveObject * pActiveObject,
            IOleCommandTarget * pCommandTarget,
            IOleInPlaceFrame * pFrame,
            IOleInPlaceUIWindow * pDoc);
    STDMETHOD(HideUI) (void);
    STDMETHOD(UpdateUI) (void);
    STDMETHOD(EnableModeless)(BOOL fEnable);
    STDMETHOD(OnDocWindowActivate) (BOOL fActivate);
    STDMETHOD(OnFrameWindowActivate) (BOOL fActivate);
    STDMETHOD(ResizeBorder) (
            LPCRECT prcBorder,
            IOleInPlaceUIWindow * pUIWindow,
            BOOL fRameWindow);
    STDMETHOD(ShowContextMenu) (
            DWORD dwID,
            POINT * pptPosition,
            IUnknown * pcmdtReserved,
            IDispatch * pDispatchObjectHit);
    STDMETHOD(TranslateAccelerator) (
            LPMSG lpMsg,
            const GUID * pguidCmdGroup, 
            DWORD nCmdID);
    STDMETHOD(GetOptionKeyPath) (BSTR *pbstrKey, DWORD dw);
    STDMETHOD(GetDropTarget) (
            IDropTarget * pDropTarget, 
            IDropTarget ** ppDropTarget);
    
    STDMETHOD(FindBehavior)
        (LPOLESTR pchName, LPOLESTR pchUrl, IElementBehaviorSite * pSite, IElementBehavior **ppPeer);

    // IVersionHost methods
    STDMETHOD(QueryUseLocalVersionVector) (BOOL *fUseLocal);
    STDMETHOD(QueryVersionVector) (IVersionVector *pVersionVector);

    //
    // Create the Toolbar UI.
    //
    VOID CreateToolbarUI();

};


//+------------------------------------------------------------------------
//
//  Class:      CPadFrame
//
//  Purpose:    OLE Frame for active object
//
//-------------------------------------------------------------------------

class CPadFrame : public IOleInPlaceFrame
{
public:

    DECLARE_SUBOBJECT_IUNKNOWN(CPadDoc, PadDoc)

    // IOleWindow methods
    STDMETHOD(GetWindow)            (HWND * lphwnd);
    STDMETHOD(ContextSensitiveHelp) (BOOL fEnterMode);

    // IOleInPlaceUIWindow methods
    STDMETHOD(GetBorder)            (LPOLERECT lprectBorder);
    STDMETHOD(RequestBorderSpace)   (LPCBORDERWIDTHS lpborderwidths);
    STDMETHOD(SetBorderSpace)       (LPCBORDERWIDTHS lpborderwidths);
    STDMETHOD(SetActiveObject)      (
        LPOLEINPLACEACTIVEOBJECT    lpActiveObject,
        LPCTSTR                   lpszObjName);

    // IOleInPlaceFrame methods
    STDMETHOD(InsertMenus)          (HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHOD(SetMenu)              (HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHOD(RemoveMenus)          (HMENU hmenuShared);
    STDMETHOD(SetStatusText)        (LPCTSTR lpszStatusText);
    STDMETHOD(EnableModeless)       (BOOL fEnable);
    STDMETHOD(TranslateAccelerator) (LPMSG lpmsg, WORD wID);
};

//+------------------------------------------------------------------------
//
//  Class:      CPadScriptSite
//
//  Purpose:    Active scripting site
//
//-------------------------------------------------------------------------

class CPadScriptSite :
    public IActiveScriptSite,
    public IActiveScriptSiteWindow,
    public IProvideMultipleClassInfo,
    public IConnectionPointContainer,
    public IPad,
    public IOleCommandTarget
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPadScriptSite))

    CPadScriptSite(CPadDoc * pDoc);
    ~CPadScriptSite();

    HRESULT Init(TCHAR *pchType);
    void    Close();

    // IUnknown methods

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID, void **);

    // IActiveScriptSite methods

    STDMETHOD(GetLCID)(LCID *plcid);
    STDMETHOD(GetItemInfo)(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti);
    STDMETHOD(GetDocVersionString)(BSTR *pszVersion);
    STDMETHOD(RequestItems)(void);
    STDMETHOD(RequestTypeLibs)(void);
    STDMETHOD(OnScriptTerminate)(const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo);
    STDMETHOD(OnStateChange)(SCRIPTSTATE ssScriptState);
    STDMETHOD(OnScriptError)(IActiveScriptError *pscripterror);
    STDMETHOD(OnEnterScript)(void);
    STDMETHOD(OnLeaveScript)(void);

    // IActiveScriptSiteWindow methods

    STDMETHOD(GetWindow)(HWND *phwnd);
    STDMETHOD(EnableModeless)(BOOL fEnable);

    // IProvideClassInfo methods

    STDMETHOD(GetClassInfo)(ITypeInfo **);
    STDMETHOD(GetGUID)(DWORD dwGuidKind, GUID * pGUID);

    // IProvideMultipleClassInfo methods

    STDMETHOD(GetMultiTypeInfoCount)(ULONG *pcti);
    STDMETHOD(GetInfoOfIndex)(ULONG iti, DWORD dwFlags, ITypeInfo** pptiCoClass, DWORD* pdwTIFlags, ULONG* pcdispidReserved, IID* piidPrimary, IID* piidSource);

    // IConnectionPointContainer methods

    STDMETHOD(EnumConnectionPoints)(LPENUMCONNECTIONPOINTS*);
    STDMETHOD(FindConnectionPoint)(REFIID, LPCONNECTIONPOINT*);

    // IPad methods
    // We need to implement these on a separate identity from
    // the main pad object in order to prevent ref count loops
    // with the script engine.

    STDMETHOD(GetTypeInfoCount)(UINT FAR* pctinfo);

    STDMETHOD(GetTypeInfo)(
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo);

    STDMETHOD(GetIDsOfNames)(
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid);

    STDMETHOD(Invoke)(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr);

    STDMETHOD(QueryStatus)(
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext);

    STDMETHOD(Exec)(
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);

    STDMETHOD(DoEvents)(VARIANT_BOOL Wait) ;
    STDMETHOD(EndEvents)();
    STDMETHOD(WaitForRecalc)();
    STDMETHOD(SetPerfCtl)(DWORD dwFlags);
    STDMETHOD(ClearDownloadCache)();
    STDMETHOD(LockWindowUpdate)(VARIANT_BOOL Lock);
    STDMETHOD(LockKeyState)(VARIANT_BOOL Shift, VARIANT_BOOL Control,VARIANT_BOOL Alt);
    STDMETHOD(UnlockKeyState)();
    STDMETHOD(SendKeys)(BSTR Keys, VARIANT_BOOL Wait) ;
    STDMETHOD(OpenFile)(BSTR Path, BSTR ProgID);
    STDMETHOD(SaveFile)(BSTR Path);
    STDMETHOD(CloseFile)();
    STDMETHOD(ExecuteCommand)(long CmdID, VARIANT * Data);
    STDMETHOD(QueryCommandStatus)(long CmdID, VARIANT * Status);
    STDMETHOD(ExecuteScript)(BSTR Path, VARIANT *ScriptParam, VARIANT_BOOL Async);
    STDMETHOD(RegisterControl)(BSTR Path);
    STDMETHOD(IncludeScript)(BSTR Path);
    STDMETHOD(SetProperty)(IDispatch * Object, BSTR Property, VARIANT * Value);
    STDMETHOD(GetProperty)(IDispatch * Object, BSTR Property, VARIANT * Value);
    STDMETHOD(get_ScriptPath)(long Level, BSTR * Path);
    STDMETHOD(get_ProcessorArchitecture)(BSTR * Path);
    STDMETHOD(get_ScriptParam)(VARIANT *ScriptParam);
    STDMETHOD(get_ScriptObject)(IDispatch **ScriptObject);
    STDMETHOD(get_CurrentTime)(long * Time);
    STDMETHOD(get_Document)(IDispatch * * Document);
    STDMETHOD(get_TempPath)(BSTR * Path);
    STDMETHOD(GetTempFileName)(BSTR * Name);
    STDMETHOD(PrintStatus)(BSTR Message);
    STDMETHOD(PrintLog)(BSTR Line);
    STDMETHOD(PrintDebug)(BSTR Line);
    STDMETHOD(CreateObject)(BSTR ProgID, IDispatch **ppDisp);
    STDMETHOD(GetObject)(BSTR FileName, BSTR ProgID, IDispatch **ppDisp);
    STDMETHOD(CompareFiles)(BSTR File1, BSTR File2, VARIANT_BOOL * FilesMatch);
    STDMETHOD(CopyThisFile)(BSTR File1, BSTR File2, VARIANT_BOOL * Success);
    STDMETHOD(DRTPrint)(long Flags, VARIANT_BOOL * Success);
    STDMETHOD(SetDefaultPrinter)(BSTR bstrNewDefaultPrinter, VARIANT_BOOL * Success);
    STDMETHOD(FileExists)(BSTR File, VARIANT_BOOL * pfFileExists);
    STDMETHOD(StartCAP)();
    STDMETHOD(StopCAP)();
    STDMETHOD(SuspendCAP)();
    STDMETHOD(ResumeCAP)();
    STDMETHOD(TicStartAll)();
    STDMETHOD(TicStopAll)();
    STDMETHOD(get_TimerInterval)(long * Interval);
    STDMETHOD(put_TimerInterval)(long Interval);
    STDMETHOD(DisableDialogs)();
    STDMETHOD(ShowWindow)(long lCmdShow);
    STDMETHOD(MoveWindow)(long x,long y, long cx, long cy);
    STDMETHOD(get_WindowLeft)(long *x);
    STDMETHOD(get_WindowTop)(long *y);
    STDMETHOD(get_WindowWidth)(long *cx);
    STDMETHOD(get_WindowHeight)(long *);
    STDMETHOD(get_DialogsEnabled)(VARIANT_BOOL *Enabled);
    STDMETHOD(ASSERT)(VARIANT_BOOL Assertion, BSTR LogMsg);
    STDMETHOD(get_Lines)(IDispatch * pObject, long *pl);
    STDMETHOD(get_Line)(IDispatch * pObject, long l, IDispatch **ppLine);
    STDMETHOD(get_Cascaded)(IDispatch * pObject, IDispatch **ppCascaded);
    STDMETHOD(EnableTraceTag)(BSTR bstrTag, BOOL fEnable);
    STDMETHOD(get_Dbg)(long * plDbg);
    STDMETHOD(CleanupTempFiles)();
    STDMETHOD(WsClear)();
    STDMETHOD(WsTakeSnapshot)();
    STDMETHOD(get_WsModule)(long row, BSTR *pbstrModule);
    STDMETHOD(get_WsSection)(long row, BSTR *pbstrSection);
    STDMETHOD(get_WsSize)(long row, long *plWsSize);
    STDMETHOD(get_WsCount)(long *plCount);
    STDMETHOD(get_WsTotal)(long *plTotal);
    STDMETHOD(WsStartDelta)();
    STDMETHOD(WsEndDelta)(long *pnPageFaults);
    STDMETHOD(SetRegValue)(long hkey, BSTR bstrSubKey, BSTR bstrValueName, VARIANT value);
    STDMETHOD(CoMemoryTrackDisable)(VARIANT_BOOL fDisable);
    STDMETHOD(get_UseShdocvw)(VARIANT_BOOL *pfHosted);
    STDMETHOD(put_UseShdocvw)(VARIANT_BOOL fHosted);
    STDMETHOD(GoBack)(VARIANT_BOOL *pfWentBack);
    STDMETHOD(GoForward)(VARIANT_BOOL *pfWentForward);
    STDMETHOD(TestExternal)(BSTR bstrDLLName, BSTR bstrFunctionName, VARIANT *pParam, long *plRetVal);
    STDMETHOD(UnLoadDLL)();
    STDMETHOD(DeinitDynamicLibrary)(BSTR bstrDLLName);
    STDMETHOD(IsDynamicLibraryLoaded)(BSTR bstrDLLName, VARIANT_BOOL * pfLoaded);
    STDMETHOD(GetRegValue)(long hkey, BSTR bstrSubKey, BSTR bstrValueName, VARIANT *pValue);
    STDMETHOD(DeleteRegValue)(long hkey, BSTR bstrSubKey, BSTR bstrValueName);
    STDMETHOD(TrustProvider)(BSTR bstrKey, BSTR bstrProvider, VARIANT *poldValue);
    STDMETHOD(RevertTrustProvider)(BSTR bstrKey);
    STDMETHOD(DoReloadHistory)();
    STDMETHOD(ComputeCRC)(BSTR bstrText, VARIANT *pCRC);
    STDMETHOD(OpenFileStream)(BSTR Path);
    STDMETHOD(get_ViewChangesFired)(long *plCount);
    STDMETHOD(get_DataChangesFired)(long *plCount);
    STDMETHOD(get_DownloadNotifyMask)(ULONG *pulMask);
    STDMETHOD(put_DownloadNotifyMask)(ULONG ulMask);
    STDMETHOD(DumpMeterLog)(BSTR bstrFileName);
    STDMETHOD(GetSwitchTimers)(VARIANT * pValue);
    STDMETHOD(MoveMouseTo)(int X, int Y, VARIANT_BOOL fLeftButton, int keyState);
    STDMETHOD(DoMouseButton)(VARIANT_BOOL fLeftButton, BSTR action, int keyState);
    STDMETHOD(DoMouseButtonAt)(int X, int Y, VARIANT_BOOL fLeftButton, BSTR action, int keyState);
    STDMETHOD(TimeSaveDocToDummyStream)(long *plTimeMicros);
    STDMETHOD(Sleep)(int nMilliseconds);
    STDMETHOD(IsWin95)(long * pfWin95);
    STDMETHOD(GetAccWindow)( IDispatch **ppAccWindow );
    STDMETHOD(GetAccObjAtPoint)( long x, long y, IDispatch **ppAccObject );
    STDMETHOD(SetKeyboard)(BSTR bstrKeyboard);
    STDMETHOD(GetKeyboard)(VARIANT *pKeyboard);
    STDMETHOD(ToggleIMEMode)(BSTR bstrIME);
    STDMETHOD(SendIMEKeys)(BSTR bstrKeys);
    STDMETHOD(Markup)(VARIANT *, VARIANT *, VARIANT *, VARIANT *, VARIANT *, VARIANT *, VARIANT *);
    STDMETHOD(Random)(long range,long *plRetVal);
    STDMETHOD(RandomSeed)(long);
    STDMETHOD(GetHeapCounter)(long iCounter, long *plRetVal);
    STDMETHOD(CreateProcess)(BSTR bstrCommandLine, VARIANT_BOOL fWait);
    STDMETHOD(GetCurrentProcessId)(long * plRetVal);
    
    // Other methods

    HRESULT ExecuteScriptStr(TCHAR * pchScript);
    HRESULT ExecuteScriptFile(TCHAR *pchPath);
    HRESULT SetScriptState(SCRIPTSTATE ss);

    CPadDoc * PadDoc() {return _pDoc;}

    // Member variables

    ULONG                       _ulRefs;
    CPadScriptSite *            _pScriptSitePrev;
    IActiveScript *             _pScript;
    CPadDoc*                    _pDoc;
    TCHAR                       _achPath[MAX_PATH];
    VARIANT                     _varParam;
    IDispatch *                 _pDispSink;
};

//+------------------------------------------------------------------------
//
//  Class:      CPadDocPNS
//
//  Purpose:    Events sink for IPropertyNotifySink.
//
//-------------------------------------------------------------------------

class CPadDocPNS : public IPropertyNotifySink
{
public:
    DECLARE_FORMS_SUBOBJECT_IUNKNOWN(CPadDoc)

    // IPropertyNotifySink methods

    STDMETHOD(OnChanged)  (DISPID dispid);
    STDMETHOD(OnRequestEdit) (DISPID dispid) { return(S_OK); }

    // Data members

    DWORD _dwCookie;

};

class CPadDocAS : public IAdviseSink
{
public:
    DECLARE_SUBOBJECT_IUNKNOWN(CPadDoc, PadDoc)

};

// HtmlForm in-place toolbar and comboboxes.
#define IDR_HTMLFORM_TBSTANDARD  950
#define IDR_HTMLFORM_TBFORMAT    951

#define MAX_COMBO_VISUAL_ITEMS 20

//+------------------------------------------------------------------------
//
//  Class:      CPadDoc
//
//  Purpose:    The application
//
//-------------------------------------------------------------------------

class CPadDoc :
    public IPad,
    public IOleObject,
    public IOleContainer   
{
public:
    friend class CPadDocPNS;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPadDoc))

    CPadDoc(BOOL fUseShdocvw = FALSE);
    virtual ~CPadDoc();

    // IUnknown methods

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID, void **);

    // Sub-object management

    virtual void Passivate();
    ULONG SubAddRef();
    ULONG SubRelease();
    ULONG GetRefs()       { return _ulAllRefs; }
    ULONG GetObjectRefs() { return _ulRefs; }

    // Initialization

    void    SetWindowPosition();
    void    PersistWindowPosition();
    virtual HRESULT Init(int nCmdShow, CEventCallBack * pEvent = NULL);
    void    Welcome();

    // IOleObject interface

    STDMETHOD(SetClientSite)(IOleClientSite *);
    STDMETHOD(GetClientSite)(IOleClientSite **);
    STDMETHOD(SetHostNames)(LPCOLESTR, LPCOLESTR);
    STDMETHOD(Close)(DWORD);
    STDMETHOD(SetMoniker)(DWORD , IMoniker *);
    STDMETHOD(GetMoniker)(DWORD, DWORD, IMoniker **);
    STDMETHOD(InitFromData)(IDataObject *, BOOL, DWORD);
    STDMETHOD(GetClipboardData)(DWORD, IDataObject **);
    STDMETHOD(DoVerb)(LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCOLERECT);
    STDMETHOD(EnumVerbs)(IEnumOLEVERB **);
    STDMETHOD(Update)();
    STDMETHOD(IsUpToDate)();
    STDMETHOD(GetUserClassID)(CLSID *);
    STDMETHOD(GetUserType)(DWORD, LPOLESTR *);
    STDMETHOD(SetExtent)(DWORD, SIZEL *);
    STDMETHOD(GetExtent)(DWORD, SIZEL *);
    STDMETHOD(Advise)(IAdviseSink *, DWORD *);
    STDMETHOD(Unadvise)(DWORD);
    STDMETHOD(EnumAdvise)(IEnumSTATDATA **);
    STDMETHOD(GetMiscStatus)(DWORD, DWORD *);
    STDMETHOD(SetColorScheme)(LOGPALETTE  *pLogpal);

    // IOleContainer methods

    STDMETHOD(ParseDisplayName)(IBindCtx *, LPOLESTR, ULONG *, IMoniker **);
    STDMETHOD(EnumObjects)(DWORD, IEnumUnknown **);
    STDMETHOD(LockContainer)(BOOL);

    // IDispatch interface

    STDMETHOD(GetTypeInfoCount)(UINT FAR* pctinfo);

    STDMETHOD(GetTypeInfo)(
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo);

    STDMETHOD(GetIDsOfNames)(
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid);

    STDMETHOD(Invoke)(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr);

    HRESULT RegisterPadWndClass();

    // IPropertyNotifySink callback

    void OnReadyStateChange();

    // IPad interface

    STDMETHOD(DoEvents)(VARIANT_BOOL Wait);
    STDMETHOD(EndEvents)();
    STDMETHOD(WaitForRecalc)();
    STDMETHOD(SetPerfCtl)(DWORD dwFlags);
    STDMETHOD(ClearDownloadCache)();
    STDMETHOD(LockWindowUpdate)(VARIANT_BOOL Lock);
    STDMETHOD(LockKeyState)(VARIANT_BOOL Shift, VARIANT_BOOL Control,VARIANT_BOOL Alt);
    STDMETHOD(UnlockKeyState)();
    STDMETHOD(SendKeys)(BSTR Keys, VARIANT_BOOL Wait);
    STDMETHOD(OpenFile)(BSTR Path, BSTR ProgID);
    STDMETHOD(SaveFile)(BSTR Path);
    STDMETHOD(CloseFile)();
    STDMETHOD(ExecuteCommand)(long CmdID, VARIANT * Data);
    STDMETHOD(QueryCommandStatus)(long CmdID, VARIANT * Status);
    STDMETHOD(ExecuteScript)(BSTR Path, VARIANT *ScriptParam, VARIANT_BOOL Async);
    STDMETHOD(RegisterControl)(BSTR Path);
    STDMETHOD(IncludeScript)(BSTR Path);
    STDMETHOD(SetProperty)(IDispatch * Object, BSTR Property, VARIANT * Value);
    STDMETHOD(GetProperty)(IDispatch * Object, BSTR Property, VARIANT * Value);
    STDMETHOD(get_ScriptPath)(long Level, BSTR * Path);
    STDMETHOD(get_ProcessorArchitecture)(BSTR * Path);
    STDMETHOD(get_ScriptParam)(VARIANT *ScriptParam);
    STDMETHOD(get_ScriptObject)(IDispatch **ScriptObject);
    STDMETHOD(get_CurrentTime)(long * Time);
    STDMETHOD(get_Document)(IDispatch * * Document);
    STDMETHOD(get_TempPath)(BSTR * Path);
    STDMETHOD(GetTempFileName)(BSTR * Name);
    STDMETHOD(PrintStatus)(BSTR Message);
    STDMETHOD(PrintLog)(BSTR Line);
    STDMETHOD(PrintDebug)(BSTR Line);
    STDMETHOD(CreateObject)(BSTR ProgID, IDispatch **ppDisp);
    STDMETHOD(GetObject)(BSTR FileName, BSTR ProgID, IDispatch **ppDisp);
    STDMETHOD(CompareFiles)(BSTR File1, BSTR File2, VARIANT_BOOL * FilesMatch);
    STDMETHOD(CopyThisFile)(BSTR File1, BSTR File2, VARIANT_BOOL * Success);
    STDMETHOD(DRTPrint)(long Flags, VARIANT_BOOL * Success);
    STDMETHOD(SetDefaultPrinter)(BSTR bstrNewDefaultPrinter, VARIANT_BOOL * Success);
    STDMETHOD(FileExists)(BSTR File, VARIANT_BOOL * pfFileExists);
    STDMETHOD(StartCAP)();
    STDMETHOD(StopCAP)();
    STDMETHOD(SuspendCAP)();
    STDMETHOD(ResumeCAP)();
    STDMETHOD(TicStartAll)();
    STDMETHOD(TicStopAll)();
    STDMETHOD(get_TimerInterval)(long * Interval);
    STDMETHOD(put_TimerInterval)(long Interval);
    STDMETHOD(DisableDialogs)();
    STDMETHOD(ShowWindow)(long);
    STDMETHOD(MoveWindow)(long,long,long,long);
    STDMETHOD(get_WindowLeft)(long *);
    STDMETHOD(get_WindowTop)(long *);
    STDMETHOD(get_WindowWidth)(long *);
    STDMETHOD(get_WindowHeight)(long *);
    STDMETHOD(get_DialogsEnabled)(VARIANT_BOOL *Enabled);
    STDMETHOD(ASSERT)(VARIANT_BOOL Assertion, BSTR LogMsg);
    STDMETHOD(get_Lines)(IDispatch * pObject, long *pl);
    STDMETHOD(get_Line)(IDispatch * pObject, long l, IDispatch **ppLine);
    STDMETHOD(get_Cascaded)(IDispatch * pObject, IDispatch **ppCascaded);
    STDMETHOD(EnableTraceTag)(BSTR bstrTag, BOOL fEnable);
    STDMETHOD(get_Dbg)(long * plDbg);
    STDMETHOD(CleanupTempFiles)();
    STDMETHOD(WsClear)();
    STDMETHOD(WsTakeSnapshot)();
    STDMETHOD(get_WsModule)(long row, BSTR *pbstrModule);
    STDMETHOD(get_WsSection)(long row, BSTR *pbstrSection);
    STDMETHOD(get_WsSize)(long row, long *plWsSize);
    STDMETHOD(get_WsCount)(long *plCount);
    STDMETHOD(get_WsTotal)(long *plTotal);
    STDMETHOD(WsStartDelta)();
    STDMETHOD(WsEndDelta)(long *pnPageFaults);
    STDMETHOD(SetRegValue)(long hkey, BSTR bstrSubKey, BSTR bstrValueName, VARIANT value);
    STDMETHOD(CoMemoryTrackDisable)(VARIANT_BOOL fDisable);
    STDMETHOD(get_UseShdocvw)(VARIANT_BOOL *pfHosted);
    STDMETHOD(put_UseShdocvw)(VARIANT_BOOL fHosted);
    STDMETHOD(GoBack)(VARIANT_BOOL *pfWentBack);
    STDMETHOD(GoForward)(VARIANT_BOOL *pfWentForward);
    STDMETHOD(TestExternal)(BSTR bstrDLLName, BSTR bstrFunctionName, VARIANT *pParam, long *plRetVal);
    STDMETHOD(UnLoadDLL)();
    STDMETHOD(DeinitDynamicLibrary)(BSTR bstrDLLName);
    STDMETHOD(IsDynamicLibraryLoaded)(BSTR bstrDLLName, VARIANT_BOOL * pfLoaded);
    STDMETHOD(GetRegValue)(long hkey, BSTR bstrSubKey, BSTR bstrValueName, VARIANT *pValue);
    STDMETHOD(DeleteRegValue)(long hkey, BSTR bstrSubKey, BSTR bstrValueName);
    STDMETHOD(TrustProvider)(BSTR bstrKey, BSTR bstrProvider, VARIANT *poldValue);
    STDMETHOD(RevertTrustProvider)(BSTR bstrKey);
    STDMETHOD(DoReloadHistory)();
    STDMETHOD(ComputeCRC)(BSTR bstrText, VARIANT *pCRC);
    STDMETHOD(OpenFileStream)(BSTR Path);
    STDMETHOD(get_ViewChangesFired)(long *plCount);
    STDMETHOD(get_DataChangesFired)(long *plCount);
    STDMETHOD(get_DownloadNotifyMask)(ULONG *pulMask);
    STDMETHOD(put_DownloadNotifyMask)(ULONG ulMask);
    STDMETHOD(DumpMeterLog)(BSTR bstrFileName);
    STDMETHOD(GetSwitchTimers)(VARIANT * pValue);
    STDMETHOD(MoveMouseTo)(int X, int Y, VARIANT_BOOL fLeftButton, int keyState);
    STDMETHOD(DoMouseButton)(VARIANT_BOOL fLeftButton, BSTR action, int keyState);
    STDMETHOD(DoMouseButtonAt)(int X, int Y,VARIANT_BOOL fLeftButton, BSTR action, int keyState);    
    STDMETHOD(TimeSaveDocToDummyStream)(long *plTimeMicros);
    STDMETHOD(Sleep)(int nMilliseconds);
    STDMETHOD(IsWin95)(long * pfWin95);
    STDMETHOD(GetAccWindow)( IDispatch **ppAccWindow );
    STDMETHOD(GetAccObjAtPoint)( long x, long y, IDispatch ** ppAccObject );
    STDMETHOD(SetKeyboard)(BSTR bstrKeyboard);
    STDMETHOD(GetKeyboard)(VARIANT *pKeyboard);
    STDMETHOD(ToggleIMEMode)(BSTR bstrIME);
    STDMETHOD(SendIMEKeys)(BSTR bstrKeys);
    STDMETHOD(Markup)(VARIANT *, VARIANT *, VARIANT *, VARIANT *, VARIANT *, VARIANT *, VARIANT *);
    STDMETHOD(Random)(long range, long *plRetVal);
    STDMETHOD(RandomSeed)(long);
    STDMETHOD(GetHeapCounter)(long iCounter, long *plRetVal);
    STDMETHOD(CreateProcess)(BSTR bstrCommandLine, VARIANT_BOOL fWait);
    STDMETHOD(GetCurrentProcessId)(long * plRetVal);

    //  IServiceProvider methods
    virtual HRESULT QueryService(REFGUID sid, REFIID iid, void ** ppv) { return E_FAIL; };

    
    // Document mangement

    HRESULT QuerySave(DWORD dwSaveOptions);
    HRESULT Open(TCHAR *szPath);
    HRESULT Open(REFCLSID clsid, TCHAR *szPath = NULL);
    HRESULT Open(IStream *pStream);
    HRESULT OpenHistory(IStream * pStream);
    HRESULT Save(LPCTSTR szName);
    HRESULT Save(IStream *pStream);
    HRESULT SaveHistory(IStream * pStream);
    HRESULT Activate(IOleObject *pObject);
    HRESULT Deactivate();

    HRESULT PromptOpenURL(HWND hwnd = NULL, const CLSID * pclsid = NULL);
    HRESULT PromptOpenFile(HWND hwnd = NULL, const CLSID * pclsid = NULL);

    virtual HRESULT DoSave(BOOL fPrompt);
    virtual BOOL    GetDirtyState();
    virtual void    GetViewRect(RECT *prc, BOOL fIncludeObjectAdornments);
    virtual void    Resize();

    // Script management

    HRESULT LoadTypeLibrary();
    HRESULT PushScript(TCHAR *pchType);
    HRESULT PopScript();
    HRESULT CloseScripts();
    HRESULT ExecuteTopLevelScript(TCHAR *pchPath);
    HRESULT ExecuteTopLevelScriptlet(TCHAR *pchScript);
    HRESULT PromptExecuteScript(BOOL fDRT);
    HRESULT ExecuteDRT();
    void    FireEvent(DISPID, UINT cArg, VARIANTARG *pvararg);
    void    FireEvent(DISPID, LPCTSTR);
    void    FireEvent(DISPID, BOOL);

    // Window procedure

    LRESULT OnClose();
    LRESULT OnDestroy();
    LRESULT OnActivate(WORD);
    virtual LRESULT OnCommand(WORD wNotifyCode, WORD idm, HWND hwndCtl);
    virtual LRESULT OnInitMenuPopup(HMENU hmenuPopup, UINT uPos, BOOL fSystemMenu);
    LRESULT OnMenuSelect(UINT uItem, UINT fuFlags, HMENU hmenu);
    HRESULT EnableMenuItems( UINT * menuItem, HMENU hmenu ) ;
    LRESULT OnInitMenuEdit(UINT uItem,  HMENU hmenu);
    LRESULT OnInitMenuView(UINT uItem, HMENU hmenu);
    LRESULT OnInitMenuInsert(UINT uItem, HMENU hmenu);
    LRESULT OnInitMenuFormat(UINT uItem, HMENU hmenu);
    
    virtual LRESULT OnSize(WORD fwSizeType, WORD nWidth, WORD nHeight);
    static  LRESULT CALLBACK WndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
    virtual LRESULT PadWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
    virtual BOOL OnTranslateAccelerator(MSG * pMsg);
    void InstallComboboxTooltip(HWND hwndCombo, UINT IDMmessage);
    LRESULT OnQueryNewPalette();
    LRESULT OnPaletteChanged(HWND hwnd);
    void    DirtyColors();

    // UI management

    HRESULT UIActivateDoc(LPMSG pMsg);
    HRESULT UIDeactivateDoc();
    
    void    UpdateFontSizeBtns(IOleCommandTarget * pCommandTarget);
    virtual HRESULT InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS pmgw);
    virtual UINT GetMenuID()  { return IDR_PADMENU; }
    virtual void SetDocTitle(TCHAR * pchTitle);

    HRESULT CreateToolBarUI();
    HRESULT CreateStandardToolbar();
    HRESULT CreateFormatToolbar();

    LRESULT UpdateToolbarUI();
    LRESULT UpdateDirtyUI();
    LRESULT UpdateStandardToolbar();
    LRESULT UpdateFormatToolbar();
    VOID UpdateFormatButtonStatus();
    HRESULT MoveFormatToolbar();
    void InitStandardToolbar();
    void InitFormatToolbar();
    HRESULT OnStandardCombobox(WORD wNotifyCode, WORD idm, HWND hwndCtl );
    HRESULT OnFormatCombobox(WORD wNotifyCode, WORD idm, HWND hwndCtl );
    BOOL IsEditCommand(WORD idm);
    
    void ShowFormatToolbar();
    void HideFormatToolbar();
    BOOL  IsEdit();
    
    //HRESULT InstallToolbarUI();
    //void RemoveToolbarUI();
    void DestroyToolbars();

    void ResetZoomPercent();
    void ApplyZoomPercent();
    
    // Helper functions

    HRESULT InitializeBindContext();
    void    ReleaseBindContext();
    void    SendAmbientPropertyChange(DISPID);
    void    SetStatusText(LPCTSTR lpszStatusText);
    VOID    ToggleDebugWindowVisibility ();
    BOOL    IsDebugWindowVisible ();
    void    ForgetPalette();
    HRESULT AttachDownloadNotify(IOleObject *pObject);
    HRESULT DetachDownloadNotify(IOleObject *pObject);
    
    void    EnqueueKeyAction(DWORD type, UINT msg, WPARAM wParam, LPARAM lParam);
    void    DoKeyAction();
    TCHAR * SendKey(TCHAR * pch, DWORD dwFlags);
    TCHAR * SendIMEKey(TCHAR * pch, DWORD dwFlags);
    TCHAR * SendSpecial(TCHAR * pch, DWORD dwFlags);
    TCHAR * SendIMESpecial(TCHAR * pch, DWORD dwFlags);

    HRESULT GetMoniker (LPMONIKER * ppmk);
    HRESULT GetBrowser();
    void    SetupComposeFont();
    void    SetupDefaultBlock();

    void    PerfCtlCallback(DWORD dwArg1, void * pvArg2);
             
    void ConvColorrefToString(COLORREF crColor, LPTSTR szName, int cchName );

    DWORD AddComboboxItems(HWND hwndCombo,
                           BOOL fItemData,
                           const ComboItem * pComboItems);

  
#if DBG==1
    HRESULT TestNile(void);
#endif

    // Static functions

    static void RunOneMessage(MSG *);
    static int Run(BOOL fStopWhenEmpty = FALSE);

    // Embedded members

    CPadSite                    _Site;
    CPadFrame                   _Frame;
    CPadBSC                     _BSC;
    CPadDocPNS                  _PNS;

    // Member variables

    ULONG                       _ulRefs;
    ULONG                       _ulAllRefs;


    CPadDoc *                   _pDocNext;
    IOleObject *                _pObject;
    IOleInPlaceObject *         _pInPlaceObject;
    IOleInPlaceActiveObject *   _pInPlaceActiveObject;
    IBinding *                  _pBinding;
    IBindCtx *                  _pBCtx;
    IOleDocumentView *          _pView;
    ITypeLib *                  _pTypeLibPad;
    ITypeInfo *                 _pTypeInfoCPad;
    ITypeInfo *                 _pTypeInfoIPad;
    ITypeInfo *                 _pTypeInfoILine;
    ITypeInfo *                 _pTypeInfoICascaded;
    ITypeLib *                  _pTypeLibDLL;
    ITypeComp *                 _apTypeComp[2];

    CPadScriptSite *            _pScriptSite;

    HWND                        _hwnd;
    HWND                        _hwndStatus;
    HWND                        _hwndWelcome;
    HWND                        _hwndToolbar;   // pad toolbar.
    HWND                        _hwndFormatToolbar; // pad format toolbar

    // Format TB Combobox Controls

    HWND                    _hwndComboTag;
    HWND                    _hwndComboFont;
    HWND                    _hwndComboSize;
    HWND                    _hwndComboColor;
    HWND                    _hwndComboZoom;
    unsigned                _fToolbarDestroy:1;
    unsigned                _fComboLoaded:1;
    unsigned                _fToolbarhidden:1;
    unsigned                _fFormatInit:1;
    unsigned                _fStandardInit:1;
    
    HMENU                       _hmenuMain;
    HMENU                       _hmenuEdit;
    
    HMENU                       _hmenuHelp;
    HMENU                       _hmenuObjectHelp;
    int                         _cMenuHelpItems;
    BOOL                        _fObjectHelp;

    BORDERWIDTHS                _bwToolbarSpace; // space for in-place toolbar

    long                        _lTimerInterval;
    
    long                        _lViewChangesFired;
    long                        _lDataChangesFired;
    DWORD                       _dwCookie;

    int                         _iZoom;
    int                         _iZoomMax;
    int                         _iZoomMin;

    WNDPROC                     _pfnOrigWndProc;
    HWND                        _hwndHooked;
    CEventCallBack *            _pEvent;

    CDebugWindow*               _pDebugWindow;

    IMoniker *                  _pMk;

    IWebBrowser *               _pBrowser;

    LONG                        _lReadyState;

    HPALETTE                    _hpal;
    enum { palNormal, palChanged, palUnknown } _palState;

    IDownloadNotify *           _pDownloadNotify;

    ULONG                       _ulDownloadNotifyMask;

    //
    // Support for pointer automation in the pad
    //

    struct PadPointerData
    {
        IMarkupPointer * _pPointer;
        long             _id;
    };

    CDataAry < PadPointerData > _aryPadPointers;
    
    IMarkupPointer * FindPadPointer ( long id );
    IMarkupPointer * FindPadPointer ( VARIANT * );
    
    //
    // Support for markup container automation in the pad
    //

    struct PadContainerData
    {
        IMarkupContainer * _pContainer;
        long               _id;
    };

    long _idPadIDNext;

    CDataAry < PadContainerData > _aryPadContainers;

    IMarkupContainer * FindPadContainer ( long id );
    IMarkupContainer * FindPadContainer ( VARIANT * );

    //
    // Markup Helpers
    //
    
    IHTMLElement *   GetElement ( VARIANT * );
    HRESULT          RemoveElement ( IDispatch * );

    // Flags

    BOOL                        _fVisible;
    BOOL                        _fInitNew;
    BOOL                        _fUserMode;
    BOOL                        _fMSHTML;
    BOOL                        _fNetscape;
    BOOL                        _fActive;
    BOOL                        _fDecrementedObjectCount;
    BOOL                        _fOpenURL;
    BOOL                        _fPaintLocked;
    BOOL                        _fPaintOnUnlock;
    BOOL                        _fUseShdocvw;
    BOOL                        _fDisablePadEvents;
    BOOL                        _fKeyStateLocked;

    // Static members

    static BOOL                 s_fPadWndClassRegistered;   // Protected by LOCK_GLOBALS
    static BOOL                 s_fPaletteDevice;



};

enum PAD_ACTION {
    ACTION_NONE,
    ACTION_WELCOME,
    ACTION_REGISTER_LOCAL_TRIDENT,
    ACTION_REGISTER_SYSTEM_TRIDENT,
    ACTION_REGISTER_PAD,
    ACTION_REGISTER_MAIL,
    ACTION_UNREGISTER_MAIL,
    ACTION_SERVER,
    ACTION_NEW,
    ACTION_SCRIPT,
    ACTION_OPEN,
    ACTION_HELP,
    ACTION_NUKE_KNOWNDLLS
};


class CThreadProcParam
{
public:
    CThreadProcParam(
            BOOL fUseShdocvw = FALSE,
            PAD_ACTION action = ACTION_WELCOME,
            BOOL fKeepRunning = FALSE,
            TCHAR * pchParam = NULL) :
        _action(action),
        _fKeepRunning(fKeepRunning),
        _pchParam(pchParam),
        _hEvent(NULL),
        _ppStm(NULL),
        _uShow(SW_SHOWNORMAL),
        _fUseShdocvw(fUseShdocvw)
        {}

    PAD_ACTION  _action;
    BOOL        _fKeepRunning;
    TCHAR *     _pchParam;
    EVENT_HANDLE      _hEvent;
    IStream **  _ppStm;
    UINT        _uShow;
    BOOL        _fUseShdocvw;
};


#ifdef WHEN_CONTROL_PALETTE_IS_SUPPORTED
// ControlPalette

void    DeinitControlPalette();
BOOL    IsControlPaletteVisible();
void    ToggleControlPaletteVisibility();
void    SetControlPaletteOwner(HWND);
HRESULT GetControlPaletteService(REFIID iid, void **ppv);
#endif // WHEN_CONTROL_PALETTE_IS_SUPPORTED


// Misc
HRESULT CreatePadDoc(CThreadProcParam * ptpp, IUnknown ** ppUnk);
HRESULT CheckError(HWND hwnd, HRESULT hr);
BOOL    GetURL(HWND hwnd, TCHAR *pchURL, int cch);
void    GetPadDLLName(TCHAR * pszDLL, TCHAR * achBuf, int cchBuf);
HRESULT RegisterPad();
HRESULT RegisterMsg(BOOL fDialog);
HRESULT UnregisterMsg();
HRESULT RegisterTrident(HWND, BOOL, BOOL);
HRESULT RegisterLocalCLSIDs();
void    UnregisterLocalCLSIDs();
HRESULT RegisterScripts(BOOL fSystem = FALSE);
HRESULT RegisterDLL(TCHAR *Path);
HRESULT NukeKnownDLLStuff();
void    CheckObjCount();
HRESULT PadCreateInstance(REFCLSID rclsid, IUnknown * pUnkOuter,
                          DWORD dwClsContext, REFIID riid, void ** ppv);

#define SZ_APPLICATION_NAME TEXT("Forms3 Trident")
#define SZ_PAD_WNDCLASS SZ_APPLICATION_NAME TEXT(" Doc")

EXTERN_C const GUID CLSID_CPadMessage;
EXTERN_C const GUID CLSID_NSCP;
EXTERN_C const GUID CGID_MSHTML;


#define WM_RUNSCRIPT    WM_APP + 1
#define WM_DOKEYACION   WM_APP + 2


//+------------------------------------------------------------------------
//
//  Globals in Pad
//
//-------------------------------------------------------------------------

enum REGISTER_ENUM
{
    REGISTER_NONE,
    REGISTER_TRIDENT,
    REGISTER_CLASSIC
};

extern HINSTANCE     g_hInstCore;         // Instance of dll
extern HINSTANCE     g_hInstResource;     // Instance of the resource dll
extern HWND          g_hwndMain;          // Main window handle
extern BOOL          g_fLoadSystemMSHTML; // Option to load MSHTML from the system directory

#define PUNKFACTORY_ARRAY_SIZE      256
#define NUM_LOCAL_DLLS              8

struct SENDKEY_ACTION
{
    UINT        msg;
    WPARAM      wParam;
    LPARAM      lParam;
    DWORD       dwFlags;
};

struct PADTHREADSTATE
{
    long                lObjCount;
    CPadDoc *           pDocFirst;
    BOOL                fEndEvents;
    UINT                cDoEvents;
    HWND                hwndFrame;
    HWND                hwndBar;
    HWND                hwndOwner;
    BOOL                fInitialized;
    int                 iciChecked;
    POINTS              ptsDrag;
    int                 iciDrag;
    CLIPFORMAT          cfCLSID;
    BOOL                fLocalReg;
    IUnknown *          pUnkFactory[PUNKFACTORY_ARRAY_SIZE];
    DWORD               dwCookie[PUNKFACTORY_ARRAY_SIZE];   
    WNDPROC             pfnBarWndProc;
    HINSTANCE           hinstDllThread[NUM_LOCAL_DLLS];
    int                 iaction;
    int                 caction;
    SENDKEY_ACTION      aaction[1024];
    BOOL                fDebugWindowInFront;
    TCHAR               achURL[INTERNET_MAX_URL_LENGTH+1];
};

PADTHREADSTATE *        GetThreadState();

inline HINSTANCE
GetResourceHInst()
{
    Assert(g_hInstResource && "Resource DLL is not loaded!");
    return g_hInstResource;
}

HRESULT GetOmDocumentFromDoc (IUnknown * pUnkDoc, IHTMLDocument2 ** ppOmDoc);

#endif // _PAD_HXX_
