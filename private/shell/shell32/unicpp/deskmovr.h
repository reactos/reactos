// DeskMovr.h : Declaration of the CDeskMovr

#ifndef __DESKMOVR_H_
#define __DESKMOVR_H_

#include "resource.h"       // main symbols

#include "admovr2.h"

#include "stdafx.h"

#define IDR_BOGUS_MOVR_REG  23

// Function prototypes
typedef HRESULT (CALLBACK FAR * LPFNCOMPENUM)(COMPONENT * pComp, LPVOID lpvData, DWORD dwData);
typedef HRESULT (CALLBACK FAR * LPFNELEMENUM)(IHTMLElement * pielem, LPVOID lpvData, DWORD dwData);

/////////////////////////////////////////////////////////////////////////////
// CDeskMovr
class ATL_NO_VTABLE CDeskMovr :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDeskMovr,&CLSID_DeskMovr>,
    public CComControl<CDeskMovr>,
    public IDeskMovr,
    public IOleObjectImpl<CDeskMovr>,
    public IPersistPropertyBag,
    public IOleControlImpl<CDeskMovr>,
    public IOleInPlaceActiveObjectImpl<CDeskMovr>,
    public IViewObjectExImpl<CDeskMovr>,
    public IOleInPlaceObjectWindowlessImpl<CDeskMovr>,
    public IQuickActivateImpl<CDeskMovr>
{
public:
    
    CDeskMovr(void);
    ~CDeskMovr(void);

DECLARE_REGISTRY_RESOURCEID(IDR_BOGUS_MOVR_REG)

DECLARE_WND_CLASS(TEXT("DeskMover"));

BEGIN_COM_MAP(CDeskMovr)
    COM_INTERFACE_ENTRY(IDeskMovr)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY(IPersistPropertyBag)
    COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CDeskMovr)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CDeskMovr)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouseDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnMouseUp)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
    MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
ALT_MSG_MAP(1)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
END_MSG_MAP()

    // IOleInPlaceObject
    virtual STDMETHODIMP InPlaceDeactivate(void);

    // IOleObject
    STDMETHODIMP GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus);
    virtual STDMETHODIMP SetClientSite(IOleClientSite *pClientSite);

    // IOleControl method we override to identify a safe time hook up with our partner and
    // party on the Trident OM
    virtual STDMETHODIMP FreezeEvents(BOOL bFreeze);


    // IViewObjectEx
    virtual STDMETHODIMP GetViewStatus(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }


    // IQuickActivate
    virtual STDMETHODIMP QuickActivate(QACONTAINER *pQACont, QACONTROL *pQACtrl);

    void OnKeyboardHook(WPARAM wParam, LPARAM lParam);

public:

    HRESULT OnDraw(ATL_DRAWINFO& di);

    // IPersistPropertyBag
    // IPersist
    virtual STDMETHODIMP GetClassID(CLSID *pClassID)
    {
        *pClassID = CComCoClass<CDeskMovr,&CLSID_DeskMovr>::GetObjectCLSID();
        return S_OK;
    }

    // IPersistPropertyBag
    //
    virtual STDMETHODIMP InitNew()
    {
        ATLTRACE(_T("CDeskMovr::InitNew\n"));
        return S_OK;
    }
    virtual STDMETHODIMP Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog);
    virtual STDMETHODIMP Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    HWND Create(HWND hWndParent, RECT& rcPos, LPCTSTR szWindowName = NULL,
            DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, DWORD dwExStyle = 0,
            UINT nID = 0)
    {
        // We override this method inorder to add the WS_CLIPSIBLINGS bit
        // to dwStyle, which is needed to prevent the IFrames from flashing
        // when the windowed control is moved over them.
        ATOM atom = GetWndClassInfo().Register(&m_pfnSuperWindowProc);
        return CWindowImplBase::Create(hWndParent, rcPos, szWindowName, dwStyle, dwExStyle,
            nID, atom);
    }
    HRESULT SmartActivateMovr(HRESULT hrPropagate);

protected:
    void DeactivateMovr(BOOL fDestroy);           // stop timer, release interfaces
    HRESULT ActivateMovr();          // start timer, secure interfaces

    STDMETHODIMP GetOurStyle(void); // get our control extender's style obj

    void _ChangeCapture(BOOL fSet);
    BOOL FFindTargetElement( IHTMLElement *pielem, IHTMLElement **ppielem );

    HRESULT MoveSelfToTarget( IHTMLElement  *pielem, POINT * pptDoc );
    void    TrackTarget(POINT * pptDoc);

    LRESULT OnPaint( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnMouseDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnMouseUp( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnCaptureChanged( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnTimer( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnSetCursor( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    HRESULT MoveSelfAndTarget( LONG x, LONG y );

    BOOL TrackCaption( POINT * pptDoc );
    void DrawCaptionButton(HDC hdc, LPRECT lprc, UINT uType, UINT uState, BOOL fErase);
    void DrawCaption(HDC hdc, UINT uDrawFlags, int x, int y);
    void UpdateCaption(UINT uDrawFlags);
    void CheckCaptionState(int x, int y);
    HRESULT _DisplayContextMenu(void);
    HRESULT _GetHTMLDoc(IOleClientSite * pocs, IHTMLDocument2 ** pphd2);
    HRESULT _IsInElement(HWND hwndParent, POINT * ppt, IHTMLElement ** pphe);
    HRESULT _GetZOrderSlot(LONG * plZOrderSlot, BOOL fTop);
    HRESULT _HandleZoom(LONG lCommand);
    HRESULT _EnumComponents(LPFNCOMPENUM lpfn, LPVOID lpvData, DWORD dwData);
    HRESULT _EnumElements(LPFNELEMENUM lpfn, LPVOID lpvData, DWORD dwData);
    HRESULT _TrackElement(POINT * ppt, IHTMLElement * pielem, BOOL * fDidWork);
    void _MapPoints(int * px, int * py);
    int CountActiveCaptions();

    // private state information.
    //
    BOOL m_fEnabled;
    long m_lInterval;
 
    int     m_cxBorder;
    int     m_cyBorder;
    int     m_cyCaption;
    int     m_cySysCaption;
    int     m_cyCaptionShow;
    int     m_cySMBorder;
    int     m_cxSMBorder;

    CContainedWindow m_TimerWnd;    // if we're timer-driven, we need this in case we're windowless

    enum DragMode {
        dmNull = 0,         // no dragable part
        dmMenu,             // caption drop down button for menu
        dmClose,            // caption button for close
        dmRestore,          // caption button for restore
        dmFullScreen,       // caption button for full screen
        dmSplit,            // caption button for split
                            // all gadgets on the caption bar should appear before dmMove!
        dmMove,             // move the component
        dmSizeWHBR,         // resize width and height from bottom right corner
        dmSizeWHTL,         // resize width and height from top left corner
        dmSizeWHTR,         // resize width and height from top right corner
        dmSizeWHBL,         // resize width and height from bottom left corner
        dmSizeTop,          // resize from the top edge
        dmSizeBottom,       // resize from the bottom edge
        dmSizeLeft,         // resize from the left edge
        dmSizeRight,        // resize from the right edge
        cDragModes          // count of drag modes, including dmNull
    };

    BITBOOL  m_fCanResizeX; // Whether this component can be resized in X Direction?
    BITBOOL  m_fCanResizeY; // Whether this component can be resized in Y Direction?

    HRESULT  InitAttributes(IHTMLElement *pielem);
    HRESULT GetParentWindow(void);

    DragMode m_dmCur;         // current drag mode, or dmNull if none.
    DragMode m_dmTrack;       // last drag mode seen by TrackCaption
    RECT     m_rectInner;     // area inside frame, in local coords
    RECT     m_rectOuter;     // outer bounds of mover, in local coords
    RECT     m_rectCaption;   // rect of our pseudo-caption, in local coords
    SIZE     m_sizeCorner;    // size of the corner areas of the frame


    BOOL GetCaptionButtonRect(DragMode dm, LPRECT lprc);
    void  SyncRectsToTarget(void);

    DragMode DragModeFromPoint( POINT pt );

    HCURSOR  CursorFromDragMode( DragMode dm );

    HRESULT SizeSelfAndTarget(POINT ptDoc);
    void DismissSelfNow(void);

    BOOL HandleNonMoveSize(DragMode dm);

    HCURSOR m_hcursor;

    LONG    m_top;
    LONG    m_left;
    LONG    m_width;
    LONG    m_height;

    BOOL    m_fTimer;         // do we have a timer running?
    UINT    m_uiTimerID;
    POINT   m_ptMouseCursor;  // Mouse cursor at timer
    

    BSTR    m_bstrTargetName;   // name attribute on targetted html elements

    IHTMLStyle         *m_pistyle;          // our control's style object
    IHTMLStyle         *m_pistyleTarget;    // The style object of our current target, also how we ID it
    IHTMLElement       *m_pielemTarget;     // This interface on the target is how we move and resize it
    LONG                m_iSrcTarget;       // get_sourceIndex value for the current target

    BOOL    m_fCaptured;    // true if mouse-capture/ move operation in progress
    LONG    m_dx;           // delta from mouse down to corner of active gadget
    LONG    m_dy;           // delta from mouse down to corner of active gadget};
    DWORD   m_CaptionState;
    HWND    m_hwndParent;
    LONG    m_zIndexTop;
    LONG    m_zIndexBottom;
    LONG    m_cSkipTimer;   // Used to allow the dismissal of the mover to take two timer periods.
    DWORD   m_ItemState;
};

// Defines for DrawCaption
#define DMDC_CAPTION    0x0001
#define DMDC_MENU       0x0002
#define DMDC_CLOSE      0x0004
#define DMDC_RESTORE    0x0008
#define DMDC_FULLSCREEN 0x0010
#define DMDC_SPLIT      0x0020
#define DMDC_ALL      (DMDC_CAPTION | DMDC_MENU | DMDC_CLOSE | DMDC_SPLIT | DMDC_FULLSCREEN | DMDC_RESTORE)

// Defines for CaptionState
#define CS_MENUTRACKED          0x00000001
#define CS_MENUPUSHED           0x00000002
#define CS_CLOSETRACKED         0x00000004
#define CS_CLOSEPUSHED          0x00000008
#define CS_RESTORETRACKED       0x00000010
#define CS_RESTOREPUSHED        0x00000020
#define CS_FULLSCREENTRACKED    0x00000040
#define CS_FULLSCREENPUSHED     0x00000080
#define CS_SPLITTRACKED         0x00000100
#define CS_SPLITPUSHED          0x00000200

#endif //__DESKMOVR_H_
