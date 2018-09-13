//=--------------------------------------------------------------------------=
// CtrlObj.H
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// class declaration for the COleControl object
//
#ifndef _CTRLOBJ_H_

// we need the automation object and ctlole.h
//
#include "AutoObj.H"
#include <olectl.h>

// forward declaration
//
class COleControl;

//=--------------------------------------------------------------------------=
// Misc Helper Functions
//=--------------------------------------------------------------------------=
//
// given an Unknown pointer, get the COleControl * for it.  used typically
// in property page code.
//
COleControl *ControlFromUnknown(IUnknown *);


//=--------------------------------------------------------------------------=
// Misc Constants
//=--------------------------------------------------------------------------=
// maximum number of arguments that can be sent to FireEvent()
//
#define MAX_ARGS    32

// for the types of sinks that the COleControl class has.  you shouldn't ever
// need to use these
//
#define SINK_TYPE_EVENT      0
#define SINK_TYPE_PROPNOTIFY 1

// superclass window support.  you can pass this in to DoSuperClassPaint
//
#define DRAW_SENDERASEBACKGROUND        1

//=--------------------------------------------------------------------------=
// Various Hosts don't handle OLEIVERB_PROPERTIES correctly, so we can't use
// that as our Properties verb number.  Instead, we're going to define
// CTLIVERB_PROPERTIES as 1, and return that one in IOleObject::EnumVerbs,
// but we'll still handle OLEIVERB_PROPERTIES correctly in DoVerb.
//
#define CTLIVERB_PROPERTIES     1


//=--------------------------------------------------------------------------=
// this structure is like the OLEVERB structure, except that it has a resource ID
// instead of a string for the verb's name.  better support for localization.
//
typedef struct tagVERBINFO {

    LONG    lVerb;                // verb id
    ULONG   idVerbName;           // resource ID of verb name
    DWORD   fuFlags;              // verb flags
    DWORD   grfAttribs;           // Specifies some combination of the verb attributes in the OLEVERBATTRIB enumeration.

} VERBINFO;

// describes an event
//
typedef struct tagEVENTINFO {

    DISPID    dispid;                    // dispid of the event
    int       cParameters;               // number of arguments to the event
    VARTYPE  *rgTypes;                   // type of each argument

} EVENTINFO;

//=--------------------------------------------------------------------------=
// CONTROLOBJECTINFO
//=--------------------------------------------------------------------------=
// for each control you wish to expose to the programmer/user, you need to
// declare and define one of the following structures.  the first part should
// follow the rules of the AUTOMATIONOBJECTINFO structure.  it's pretty hard,
// however, to imagine a scenario where the control isn't CoCreatable ...
// once this structre is declared/defined, an entry should be put in the
// global g_ObjectInfo table.
//
typedef struct {

    AUTOMATIONOBJECTINFO AutomationInfo;           // automation and creation information
    const IID      *piidEvents;                    // IID of primary event interface
    DWORD           dwOleMiscFlags;                // control flags
    DWORD           dwActivationPolicy;            // IPointerInactive support
    VARIANT_BOOL    fOpaque;                       // is your control 100% opaque?
    VARIANT_BOOL    fWindowless;                   // do we do windowless if we can?
    WORD            wToolboxId;                    // resource ID of Toolbox Bitmap
    LPCSTR          szWndClass;                    // name of window control class
    VARIANT_BOOL    fWindowClassRegistered;        // has the window class been registered yet?
    WORD            cPropPages;                    // number of property pages
    const GUID    **rgPropPageGuids;               // array of the property page GUIDs
    WORD            cCustomVerbs;                  // number of custom verbs
    const VERBINFO *rgCustomVerbs;                 // description of custom verbs
    WNDPROC         pfnSubClass;                   // for subclassed controls.

} CONTROLOBJECTINFO;


#ifndef INITOBJECTS

#define DEFINE_CONTROLOBJECT(name, clsid, progid, fn, ver, riid, pszh, piide, dwcf, dwap, w, szwc, cpp, rgppg, ccv, rgcv) \
extern CONTROLOBJECTINFO name##Control \

#define DEFINE_WINDOWLESSCONTROLOBJECT(name, clsid, progid, fn, ver, riid, pszh, piide, dwcf, dwap, fo,  w, szwc, cpp, rgppg, ccv, rgcv) \
extern CONTROLOBJECTINFO name##Control \


#else
#define DEFINE_CONTROLOBJECT(name, clsid, progid, fn, ver, riid, pszh, piide, dwcf, dwap, w, szwc, cpp, rgppg, ccv, rgcv) \
CONTROLOBJECTINFO name##Control = { { {clsid, progid, fn}, ver, riid, pszh, NULL, 0}, piide, dwcf, dwap, TRUE, FALSE, w, szwc, FALSE, cpp, rgppg, ccv, rgcv, NULL } \

#define DEFINE_WINDOWLESSCONTROLOBJECT(name, clsid, progid, fn, ver, riid, pszh, piide, dwcf, dwap, fo, w, szwc, cpp, rgppg, ccv, rgcv) \
CONTROLOBJECTINFO name##Control = { { {clsid, progid, fn}, ver, riid, pszh, NULL, 0}, piide, dwcf, dwap, fo, TRUE, w, szwc, FALSE, cpp, rgppg, ccv, rgcv, NULL } \

#endif // !INITOBJECTS

#define OLEMISCFLAGSOFCONTROL(index)     ((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->dwOleMiscFlags
#define FCONTROLISWINDOWLESS(index)      ((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->fWindowless
#define FCONTROLISOPAQUE(index)          ((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->fOpaque
#define ACTIVATIONPOLICYOFCONTROL(index) ((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->dwActivationPolicy
#define EVENTIIDOFCONTROL(index)         (*(((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->piidEvents))
#define WNDCLASSNAMEOFCONTROL(index)     ((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->szWndClass
#define CPROPPAGESOFCONTROL(index)       ((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->cPropPages
#define PPROPPAGESOFCONTROL(index)       ((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->rgPropPageGuids
#define CCUSTOMVERBSOFCONTROL(index)     ((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->cCustomVerbs
#define CUSTOMVERBSOFCONTROL(index)      ((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->rgCustomVerbs
#define BITMAPIDOFCONTROL(index)         ((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->wToolboxId
#define CTLWNDCLASSREGISTERED(index)     ((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->fWindowClassRegistered
#define SUBCLASSWNDPROCOFCONTROL(index)  ((CONTROLOBJECTINFO *)(g_ObjectInfo[index].pInfo))->pfnSubClass


//=--------------------------------------------------------------------------=
// COleControl
//=--------------------------------------------------------------------------=
// the mother of all C++ objects
//
class COleControl : public CAutomationObject,
                    public IOleObject, public IOleControl,
                    public IOleInPlaceObjectWindowless, public IOleInPlaceActiveObject,
                    public IViewObjectEx, public IPersistPropertyBag,
                    public IPersistStreamInit, public IPersistStorage,
                    public IConnectionPointContainer, public ISpecifyPropertyPages,
                    public IProvideClassInfo, public IPointerInactive,
                    public IQuickActivate
{
  public:
    // IUnknown methods -- there are required since we inherit from variuos
    // people who themselves inherit from IUnknown.  just delegate to controlling
    // unknown
    //
    DECLARE_STANDARD_UNKNOWN();

    //=--------------------------------------------------------------------------=
    // IPersist methods.  used by IPersistStream and IPersistStorage
    //
    STDMETHOD(GetClassID)(THIS_ LPCLSID lpClassID);

    // IPersistStreamInit methods
    //
    STDMETHOD(IsDirty)(THIS);
    STDMETHOD(Load)(LPSTREAM pStm);
    STDMETHOD(Save)(LPSTREAM pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER FAR* pcbSize);
    STDMETHOD(InitNew)();

    // IPersistStorage
    //
    STDMETHOD(InitNew)(IStorage  *pStg);
    STDMETHOD(Load)(IStorage  *pStg);
    STDMETHOD(Save)(IStorage  *pStgSave, BOOL fSameAsLoad);
    STDMETHOD(SaveCompleted)(IStorage  *pStgNew);
    STDMETHOD(HandsOffStorage)(void);

    // IPersistPropertyBag
    //
    STDMETHOD(Load)(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog);
    STDMETHOD(Save)(LPPROPERTYBAG pPropBag, BOOL fClearDirty,
                    BOOL fSaveAllProperties);

    // IOleControl methods
    //
    STDMETHOD(GetControlInfo)(LPCONTROLINFO pCI);
    STDMETHOD(OnMnemonic)(LPMSG pMsg);
    STDMETHOD(OnAmbientPropertyChange)(DISPID dispid);
    STDMETHOD(FreezeEvents)(BOOL bFreeze);

    // IOleObject methods
    //
    STDMETHOD(SetClientSite)(IOleClientSite  *pClientSite);
    STDMETHOD(GetClientSite)(IOleClientSite  * *ppClientSite);
    STDMETHOD(SetHostNames)(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj);
    STDMETHOD(Close)(DWORD dwSaveOption);
    STDMETHOD(SetMoniker)(DWORD dwWhichMoniker, IMoniker  *pmk);
    STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker  * *ppmk);
    STDMETHOD(InitFromData)(IDataObject  *pDataObject, BOOL fCreation, DWORD dwReserved);
    STDMETHOD(GetClipboardData)(DWORD dwReserved, IDataObject  * *ppDataObject);
    STDMETHOD(DoVerb)(LONG iVerb, LPMSG lpmsg, IOleClientSite  *pActiveSite, LONG lindex,
                                     HWND hwndParent, LPCRECT lprcPosRect);
    STDMETHOD(EnumVerbs)(IEnumOLEVERB  * *ppEnumOleVerb);
    STDMETHOD(Update)(void);
    STDMETHOD(IsUpToDate)(void);
    STDMETHOD(GetUserClassID)(CLSID  *pClsid);
    STDMETHOD(GetUserType)(DWORD dwFormOfType, LPOLESTR  *pszUserType);
    STDMETHOD(SetExtent)(DWORD dwDrawAspect,SIZEL  *psizel);
    STDMETHOD(GetExtent)(DWORD dwDrawAspect, SIZEL  *psizel);
    STDMETHOD(Advise)(IAdviseSink  *pAdvSink, DWORD  *pdwConnection);
    STDMETHOD(Unadvise)(DWORD dwConnection);
    STDMETHOD(EnumAdvise)(IEnumSTATDATA  * *ppenumAdvise);
    STDMETHOD(GetMiscStatus)(DWORD dwAspect, DWORD  *pdwStatus);
    STDMETHOD(SetColorScheme)(LOGPALETTE  *pLogpal);

    // IOleWindow.  required for IOleInPlaceObject and IOleInPlaceActiveObject
    //
    STDMETHOD(GetWindow)(HWND *phwnd);
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);

    // IOleInPlaceObject/IOleInPlaceObjectWindowless
    //
    STDMETHOD(InPlaceDeactivate)(void);
    STDMETHOD(UIDeactivate)(void);
    STDMETHOD(SetObjectRects)(LPCRECT lprcPosRect,LPCRECT lprcClipRect) ;
    STDMETHOD(ReactivateAndUndo)(void);
    STDMETHOD(OnWindowMessage)(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
    STDMETHOD(GetDropTarget)(IDropTarget **ppDropTarget);

    // IOleInPlaceActiveObject
    //
    STDMETHOD(TranslateAccelerator)(LPMSG lpmsg);
    STDMETHOD(OnFrameWindowActivate)(BOOL fActivate);
    STDMETHOD(OnDocWindowActivate)(BOOL fActivate);
    STDMETHOD(ResizeBorder)(LPCRECT prcBorder,
                            IOleInPlaceUIWindow  *pUIWindow,
                            BOOL fFrameWindow);
    STDMETHOD(EnableModeless)(BOOL fEnable);

    // IViewObject2/IViewObjectEx
    //
    STDMETHOD(Draw)(DWORD dwDrawAspect, LONG lindex, void  *pvAspect,
                    DVTARGETDEVICE  *ptd, HDC hdcTargetDev, HDC hdcDraw,
                    LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
                    BOOL ( __stdcall  *pfnContinue )(ULONG_PTR dwContinue),
                    ULONG_PTR dwContinue);
    STDMETHOD(GetColorSet)(DWORD dwDrawAspect,LONG lindex, void  *pvAspect,
                           DVTARGETDEVICE  *ptd, HDC hicTargetDev,
                           LOGPALETTE  * *ppColorSet);
    STDMETHOD(Freeze)(DWORD dwDrawAspect, LONG lindex,
                      void  *pvAspect,DWORD  *pdwFreeze);
    STDMETHOD(Unfreeze)(DWORD dwFreeze);
    STDMETHOD(SetAdvise)(DWORD aspects, DWORD advf, IAdviseSink  *pAdvSink);
    STDMETHOD(GetAdvise)(DWORD *pAspects, DWORD  *pAdvf, IAdviseSink  * *ppAdvSink);
    STDMETHOD(GetExtent)(DWORD dwDrawAspect, LONG lindex, DVTARGETDEVICE __RPC_FAR *ptd, LPSIZEL lpsizel);
    STDMETHOD(GetRect)(DWORD dwAspect, LPRECTL pRect);
    STDMETHOD(GetViewStatus)(DWORD *pdwStatus);
    STDMETHOD(QueryHitPoint)(DWORD dwAspect, LPCRECT pRectBounds, POINT ptlLoc, LONG lCloseHint, DWORD *pHitResult);
    STDMETHOD(QueryHitRect)(DWORD dwAspect, LPCRECT pRectBounds, LPCRECT prcLoc, LONG lCloseHint, DWORD *pHitResult);
    STDMETHOD(GetNaturalExtent)(DWORD dwAspect, LONG lindex, DVTARGETDEVICE *ptd, HDC hicTargetDev, DVEXTENTINFO *pExtentInfo, LPSIZEL psizel);

    // IConnectionPointContainer methods
    //
    STDMETHOD(EnumConnectionPoints)(LPENUMCONNECTIONPOINTS FAR* ppEnum);
    STDMETHOD(FindConnectionPoint)(REFIID iid, LPCONNECTIONPOINT FAR* ppCP);

    // ISpecifyPropertyPages
    //
    STDMETHOD(GetPages)(CAUUID * pPages);

    // IProvideClassInfo methods
    //
    STDMETHOD(GetClassInfo)(LPTYPEINFO * ppTI);

    // IPointerInactive methods
    //
    STDMETHOD(GetActivationPolicy)(DWORD *pdwPolicy);
    STDMETHOD(OnInactiveMouseMove)(LPCRECT pRectBounds, long x, long y, DWORD dwMouseMsg);
    STDMETHOD(OnInactiveSetCursor)(LPCRECT pRectBounds, long x, long y, DWORD dwMouseMsg, BOOL fSetAlways);

    // IQuickActivate methods
    //
    STDMETHOD(QuickActivate)(QACONTAINER *pqacontainer, QACONTROL *pqacontrol);
    STDMETHOD(SetContentExtent)(LPSIZEL);
    STDMETHOD(GetContentExtent)(LPSIZEL);

    // constructor and destructor
    //
    COleControl(IUnknown *pUnkOuter, int iPrimaryDispatch, void *pMainInterface,
		BOOL fExpandoEnabled=FALSE);
    virtual ~COleControl();

    //=--------------------------------------------------------------------------=
    // callable by anybody
    //
    static LRESULT CALLBACK ControlWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK ReflectWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static COleControl * ControlFromHwnd(HWND hwnd) {
        return (COleControl *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    void __cdecl FireEvent(EVENTINFO * pEventInfo, ...);
    HINSTANCE    GetResourceHandle(void);


    //=--------------------------------------------------------------------------=
    // ole controls that want to support both windowed and windowless operations
    // should use these wrappers instead of the appropriate win32 api routine.
    // controls that don't care and just want to be windowed all the time can
    // just go ahead and use the api routines.
    //
    BOOL    SetFocus(BOOL fGrab);                       // SetFocus API
    BOOL    OcxGetFocus(void);                          // GetFocus() == m_hwnd
    BOOL    OcxGetWindowRect(LPRECT);                   // gets your current window rect
    LRESULT OcxDefWindowProc(UINT, WPARAM, LPARAM);     // DefWindowProc
    HDC     OcxGetDC(void);                             // GetDC(m_hwnd);
    void    OcxReleaseDC(HDC hdc);                      // ReleaseDC(m_hwnd, hdc);
    BOOL    OcxSetCapture(BOOL fGrab);                  // SetCapture(fGrab ? m_hwnd : NULL);
    BOOL    OcxGetCapture(void);                        // GetCapture() == m_hwnd
    BOOL    OcxInvalidateRect(LPCRECT, BOOL);           // InvalidateRect(m_hwnd, prc, f);
    BOOL    OcxScrollRect(LPCRECT, LPCRECT, int, int);  // ScrollWindowEx(...);

  protected:

    //=--------------------------------------------------------------------------=
    // member variables that derived controls can get at.
    //
    // derived controls Should NOT modify the following.
    //
    IOleClientSite     *m_pClientSite;             // client site
    IOleControlSite    *m_pControlSite;            // IOleControlSite ptr on client site
    IOleInPlaceSite    *m_pInPlaceSite;            // IOleInPlaceSite for managing activation
    IOleInPlaceFrame   *m_pInPlaceFrame;           // IOleInPlaceFrame ptr on client site
    IOleInPlaceUIWindow *m_pInPlaceUIWindow;       // for negotiating border space with client
    ISimpleFrameSite   *m_pSimpleFrameSite;        // simple frame site
    IDispatch          *m_pDispAmbient;            // ambient dispatch pointer
    SIZEL               m_Size;                    // the size of this control
    RECT                m_rcLocation;              // where we at
    HWND                m_hwnd;                    // our window
    HWND                m_hwndParent;              // our parent window
    HRGN                m_hRgn;

    // Windowless OLE controls support
    //
    IOleInPlaceSiteWindowless *m_pInPlaceSiteWndless; // IOleInPlaceSiteWindowless pointer

    // flags indicating internal state.  do not modify.
    //
    unsigned m_fDirty:1;                           // does the control need to be resaved?
    unsigned m_fInPlaceActive:1;                   // are we in place active or not?
    unsigned m_fInPlaceVisible:1;                  // we are in place visible or not?
    unsigned m_fUIActive:1;                        // are we UI active or not.
    unsigned m_fCreatingWindow:1;                  // indicates if we're in CreateWindowEx or not

    //=--------------------------------------------------------------------------=
    // methods that derived controls can override, but may need to be called
    // from their versions.
    //
    virtual void      ViewChanged(void);
    virtual HRESULT   InternalQueryInterface(REFIID riid, void **ppvObjOut);
    virtual BOOL      SetGUIFocus(HWND hwndSet);

    //=--------------------------------------------------------------------------=
    // member functions that provide for derived controls, or that we use, but
    // derived controls might still find useful.
    //
    HRESULT      DoSuperClassPaint(HDC, LPCRECTL);
    HRESULT      RecreateControlWindow(void);
    BOOL         DesignMode(void);
    BOOL         GetAmbientProperty(DISPID, VARTYPE, void *);
    BOOL         GetAmbientFont(IFont **ppFontOut);
    void         ModalDialog(BOOL fShow);
    void         InvalidateControl(LPCRECT prc);
    BOOL         SetControlSize(SIZEL *pSizel);

    HWND         CreateInPlaceWindow(int x, int y, BOOL fNoRedraw);
    HRESULT      InPlaceActivate(LONG lVerb);
    void         SetInPlaceVisible(BOOL);
    void         SetInPlaceParent(HWND);

    // IPropertyNotifySink stuff.
    //
    inline void  PropertyChanged(DISPID dispid) {
        m_cpPropNotify.DoOnChanged(dispid);
    }
    inline BOOL  RequestPropertyEdit(DISPID dispid) {
        return m_cpPropNotify.DoOnRequestEdit(dispid);
    }

    // subclassed windows controls support ...
    //
    inline HWND  GetOuterWindow(void) {
        return (m_hwndReflect) ? m_hwndReflect : m_hwnd;
    }

    // little routine for people to tell if they are windowless or not
    //
    inline BOOL  Windowless(void) {
        return !m_fInPlaceActive || m_pInPlaceSiteWndless;
    }

    // some people don't care if they're windowed or not -- they just need
    // a site pointer.  this makes it a little easier.
    //
    inline IOleInPlaceSite    *GetInPlaceSite(void) {
        return (IOleInPlaceSite *)(m_pInPlaceSiteWndless ? m_pInPlaceSiteWndless : m_pInPlaceSite);
    }

  private:
    //=--------------------------------------------------------------------------=
    // the following are methods that ALL control writers must override and implement
    //
    STDMETHOD(LoadBinaryState)(IStream *pStream) PURE;
    STDMETHOD(SaveBinaryState)(IStream *pStream) PURE;
    STDMETHOD(LoadTextState)(IPropertyBag *pPropertyBag, IErrorLog *pErrorLog) PURE;
    STDMETHOD(SaveTextState)(IPropertyBag *pPropertyBag, BOOL fWriteDefault) PURE;
    STDMETHOD(OnDraw)(DWORD dvAspect, HDC hdcDraw, LPCRECTL prcBounds, LPCRECTL prcWBounds, HDC hicTargetDev, BOOL fOptimize) PURE;
    virtual LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam) PURE;
    virtual BOOL    RegisterClassData(void) PURE;

    //=--------------------------------------------------------------------------=
    // OVERRIDABLES -- methods controls can implement for customized functionality
    //
    virtual void    AmbientPropertyChanged(DISPID dispid);
    virtual BOOL    BeforeCreateWindow(DWORD *, DWORD *, LPSTR);
    virtual void    BeforeDestroyWindow(void);
    virtual HRESULT DoCustomVerb(LONG lVerb);
    virtual BOOL    OnSetExtent(const SIZEL *pSizeL);
    virtual BOOL    OnSpecialKey(LPMSG);
    virtual BOOL    OnGetPalette(HDC, LOGPALETTE **);
    virtual HRESULT OnQuickActivate(QACONTAINER *, DWORD *);
    virtual BOOL    InitializeNewState();
    virtual BOOL    AfterCreateWindow(void);
    virtual BOOL    OnGetRect(DWORD dvAspect, LPRECTL prcRect);
    virtual void    OnSetObjectRectsChangingWindowPos(DWORD *dwFlag);
    virtual void    OnVerb(LONG lVerb);

    //=--------------------------------------------------------------------------=
    // methods that various people internally will share.  not needed, however, by
    // any inherting classes.
    //
    HRESULT         m_SaveToStream(IStream *pStream);
    HRESULT         LoadStandardState(IPropertyBag *pPropertyBag, IErrorLog *pErrorLog);
    HRESULT         LoadStandardState(IStream *pStream);
    HRESULT         SaveStandardState(IPropertyBag *pPropertyBag);
    HRESULT         SaveStandardState(IStream *pStream);

    //=--------------------------------------------------------------------------=
    // member variables we don't want anybody to get their hands on, including
    // inheriting classes
    //
    HWND              m_hwndReflect;               // for subclassed windows
    IOleAdviseHolder *m_pOleAdviseHolder;          // IOleObject::Advise holder object
    IAdviseSink      *m_pViewAdviseSink;           // IViewAdvise sink for IViewObject2
    unsigned short    m_nFreezeEvents;             // count of freezes versus thaws
    unsigned          m_fHostReflects:1;           // does the host reflect messages?
    unsigned          m_fCheckedReflecting:1;      // have we checked above yet?

    // internal flags.  various other flags are visible to the end control class.
    //
    unsigned m_fModeFlagValid:1;                   // we stash the mode as much as possible
    unsigned m_fSaveSucceeded:1;                   // did an IStorage save work correctly?
    unsigned m_fViewAdvisePrimeFirst: 1;           // for IViewobject2::setadvise
    unsigned m_fViewAdviseOnlyOnce: 1;             // for iviewobject2::setadvise
    unsigned m_fUsingWindowRgn:1;                  // for SetObjectRects and clipping
    unsigned m_fRunMode:1;                         // are we in run mode or not?

  protected:
    class CConnectionPoint : public IConnectionPoint {
      public:
        IUnknown **m_rgSinks;

        // IUnknown methods
        //
        STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
        STDMETHOD_(ULONG,AddRef)(THIS) ;
        STDMETHOD_(ULONG,Release)(THIS) ;

        // IConnectionPoint methods
        //
        STDMETHOD(GetConnectionInterface)(IID FAR* pIID);
        STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer FAR* FAR* ppCPC);
        STDMETHOD(Advise)(LPUNKNOWN pUnkSink, DWORD FAR* pdwCookie);
        STDMETHOD(Unadvise)(DWORD dwCookie);
        STDMETHOD(EnumConnections)(LPENUMCONNECTIONS FAR* ppEnum);

        void    DoInvoke(DISPID dispid, DISPPARAMS * pdispparam);
        void    DoOnChanged(DISPID dispid);
        BOOL    DoOnRequestEdit(DISPID dispid);
        HRESULT AddSink(void *, DWORD *);

        COleControl *m_pOleControl();
        CConnectionPoint(BYTE b){
            m_bType = b;
            m_rgSinks = NULL;
            m_cSinks = 0;
            m_cAllocatedSinks = 0;
            m_SingleSink = NULL;
        }
        ~CConnectionPoint();

        BYTE   m_bType;
        unsigned short m_cSinks;
        IUnknown *m_SingleSink;
        unsigned short m_cAllocatedSinks;

    } m_cpEvents, m_cpPropNotify;

    // so they can get at some of our protected things, like AddRef, QI, etc.
    //
    friend CConnectionPoint;
};

#define _CTRLOBJ_H_
#endif // _CTRLOBJ_H_
