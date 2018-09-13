//+----------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994, 1995, 1996, 1997, 1998
//
//  File:       view.hxx
//
//  Contents:   CView and related classes, constants, etc.
//
//-----------------------------------------------------------------------------

#ifndef I_VIEW_HXX_
#define I_VIEW_HXX_
#pragma INCMSG("--- Beg 'view.hxx'")

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif


class CAdorner;
class CDispRoot;
class CDispScroller;
class CDoc;
class CElement;
class CFocusAdorner;
class CLayout;
class CLock;
class CScope;
class CView;
class CViewDispClient;
class HITTESTRESULTS;

MtExtern(CView)
MtExtern(CView_aryTaskMisc_pv)
MtExtern(CView_aryTaskLayout_pv)
MtExtern(CView_aryTaskEvent_pv)
MtExtern(CView_aryTaskRecalc_pv)
MtExtern(CView_arySor_pv)
MtExtern(CView_aryTransition_pv)
MtExtern(CView_aryWndPos_pv)
MtExtern(CView_aryWndRgn_pv)
MtExtern(CView__aryClippingOuterHwnd_pv)
MtExtern(CView_aryAdorners_pv)
MtExtern(CViewDispClient)


//+----------------------------------------------------------------------------
//
//  Class:  CViewTask
//
//-----------------------------------------------------------------------------

class CViewTask
{
    friend  CView;

private:
    enum VIEWTASKTYPE
    {
        VTT_MISC    = 1,
        VTT_LAYOUT  = 2,
        VTT_RECALC  = 3,
        VTT_EVENT   = 4,

        VTT_MAX
    };

    VIEWTASKTYPE    _vtt;
    DWORD           _grfLayout;

    union
    {
        void *      _pv;
        CElement *  _pElement;
        CLayout *   _pLayout;
    };

    union
    {
        LONG        _lData;
        DISPID      _dispidEvent;
    };

    CViewTask() { }
    CViewTask(void * pv, VIEWTASKTYPE vtt, LONG lData = 0, DWORD grfLayout = 0)
                {
                    Assert(!(grfLayout & LAYOUT_NONTASKFLAGS));

                    _pv        = pv;
                    _grfLayout = grfLayout;
                    _vtt       = vtt;
                    _lData     = lData;
                }

    void *      GetObject() const
                {
                    return _pv;
                }

    CElement *  GetElement() const;
    CLayout *   GetLayout() const;

    DISPID      GetEventDispID() const;

    VIEWTASKTYPE    GetType() const
                {
                    return _vtt;
                }

    long        GetSourceIndex() const;

    BOOL        operator==(const CViewTask & vt) const
                {
                    return (    vt._pv  == _pv
                            &&  vt._vtt == _vtt);
                }

    BOOL        operator!=(const CViewTask & vt) const
                {
                    return (    vt._pv  != _pv
                            ||  vt._vtt != _vtt);
                }

    void        AddFlags(DWORD grfLayout)
                {
                    Assert(!(grfLayout & LAYOUT_NONTASKFLAGS));
                    _grfLayout |= grfLayout;
                }

    DWORD       GetFlags() const
                {
                    return _grfLayout;
                }

    BOOL        IsFlagSet(LAYOUT_FLAGS f) const
                {
                    return (_grfLayout & f);
                }

    BOOL        IsFlagsEqual(DWORD grfLayout) const
                {
                    return (grfLayout == _grfLayout);
                }

    BOOL        IsFlagsSet(DWORD grfLayout) const
                {
                    Assert(!(grfLayout & LAYOUT_NONTASKFLAGS));
                    return (_grfLayout & grfLayout);
                }

    BOOL        IsType(VIEWTASKTYPE vtt) const
                {
                    return (_vtt == vtt);
                }

    BOOL        IsType(const CViewTask & vt) const
                {
                    return IsType(vt._vtt);
                }
};

typedef CDataAry<CViewTask> CAryVTasks;


//+----------------------------------------------------------------------------
//
//  Class:  CViewDispClient
//
//-----------------------------------------------------------------------------

class CViewDispClient : public IDispObserver,
                        public CDispClient
{
    friend class CView;

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CViewDispClient))

    //
    //  IUnknown methods
    //

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG,   AddRef)(void) {return ++_cRefs;}
    STDMETHOD_(ULONG,   Release)(void);

    //
    //  IDispObserver methods
    //

    STDMETHOD_(void, Invalidate)(
                const RECT * prcInvalid,
                HRGN         rgnInvalid,
                DWORD        flags);
    STDMETHOD_(HDC, GetClientDC)(const RECT * prc);
    STDMETHOD_(void, ReleaseClientDC)(HDC hdc);
    STDMETHOD_(void, DrawSynchronous)(
                HRGN         hrgn,
                HDC          hdc,
                const RECT * prcClip);

    //
    //  CDispClient methods
    //

    void GetOwner(
                CDispNode *    pDispNode,
                void **        ppv);

    void DrawClient(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         cookie,
                void *         pClientData,
                DWORD          dwFlags);

    void DrawClientBackground(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);

    void DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);

    void DrawClientScrollbar(
                int            iDirection,
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                long           contentSize,
                long           containerSize,
                long           scrollAmount,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);

    void DrawClientScrollbarFiller(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags);

    BOOL HitTestScrollbar(
                int            iDirection,
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestScrollbarFiller(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestContent(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestFuzzy(
                const POINT *  pptHitInParentCoords,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestBorder(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL ProcessDisplayTreeTraversal(
                void *         pClientData);
    
    LONG GetZOrderForSelf();
    LONG GetZOrderForChild(
                void *         cookie);

    LONG CompareZOrder(
                CDispNode *    pDispNode1,
                CDispNode *    pDispNode2);

    CDispFilter* GetFilter();

    void HandleViewChange(
                DWORD          flags,
                const RECT *   prcClient,
                const RECT *   prcClip,
                CDispNode *    pDispNode);

    void NotifyScrollEvent(
                RECT *  prcScroll,
                SIZE *  psizeScrollDelta);

    virtual BOOL ScrollRectIntoParentView(
                                const CRect&        rc,
                                CRect::SCROLLPIN    spVert,
                                CRect::SCROLLPIN    spHorz,
                                BOOL                fScrollBits);

    DWORD GetClientLayersInfo(CDispNode *pDispNodeFor);

    void DrawClientLayers(
                const RECT *    prcBounds,
                const RECT *    prcRedraw,
                CDispSurface *  pSurface,
                CDispNode *     pDispNode,
                void *          cookie,
                void *          pClientData,
                DWORD           dwFlags);

#if DBG==1
    void DumpDebugInfo(
                HANDLE         hFile,
                long           level,
                long           childNumber,
                CDispNode *    pDispNode,
                void *         cookie);
#endif

    //
    //  Internal methods
    //

    CDoc *      Doc() const;
    CView *     View() const;

    ULONG   _cRefs;

private:
    CViewDispClient()  { };
    ~CViewDispClient() { };
};


//+----------------------------------------------------------------------------
//
//  Class:  CView
//
//-----------------------------------------------------------------------------

class CView : public CVoid
{
    friend class CDoc;
    friend class CFocusAdorner;
    friend class CViewDispClient;

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CView))

    //
    //  Public data
    //

    enum VIEWFLAGS      // Public view flags - These occupy the lower 16 bits of _grfFlags
    {
        VF_FULLSIZE     = 0x00000001,   // Use SIZEMODE_FULLSIZE when sizing the view
        VF_ZEROBORDER   = 0x00000002,   // Draw single-pixel border when borders are missing
        VF_DIRTYZORDER  = 0x00000004,   // Child windows must be reordered in Z order
        
        VF_PUBLICMAX
    };

    enum VIEWSTATE      // Public view states - These are derived from flags in _grfFlags
    {
        VS_OPEN         = 0x00000001,   // View is open
        VS_INLAYOUT     = 0x00000002,   // View is performing layout (aka EnsureView is executing)
        VS_INRENDER     = 0x00000004,   // View is rendering
    };

    //
    //  View management methods
    //

    CView();
    ~CView();

    void        Initialize(CDoc * pDoc);
    BOOL        IsInitialized() const;

    HRESULT     Activate();
    void        Deactivate();
    void        Unload();
    BOOL        IsActive() const;


    BOOL        OpenView(BOOL fBackground = FALSE, BOOL fResetTree = FALSE);
    BOOL        EnsureView(DWORD grfLayout = 0L);

    DWORD       GetState() const;
    BOOL        IsInState(VIEWSTATE vs) const;
    
    BOOL        SetViewOffset(const SIZE & sizeOffset);

    void        GetViewPosition(CPoint * ppt);
    void        GetViewPosition(POINT * ppt)
                {
                    GetViewPosition((CPoint *)ppt);
                }
    void        SetViewPosition(const POINT & pt);

    void        GetViewSize(CSize * psize) const;
    void        SetViewSize(const CSize & size);
    void        SetViewSize(const SIZE & size);

    void        GetViewRect(CRect * prc);
    void        GetViewRect(RECT * prc)
                {
                    GetViewRect((CRect *)prc);
                }

    void        EraseBackground(CFormDrawInfo * pDI, HRGN hrgnDraw, const RECT * prcDraw);
    void        EraseBackground(CFormDrawInfo * pDI, HRGN hrgnDraw)
                {
                    EraseBackground(pDI, hrgnDraw, NULL);
                }
    void        EraseBackground(CFormDrawInfo * pDI, const RECT * prcDraw)
                {
                    EraseBackground(pDI, NULL, prcDraw);
                }
    void        RenderBackground(CFormDrawInfo * pDI);
    void        RenderElement(CElement *pElement, HDC hdc);
    void        RenderView(CFormDrawInfo * pDI, HRGN hrgnDraw, const RECT * prcDraw);
    void        RenderView(CFormDrawInfo * pDI, HRGN hrgnDraw)
                {
                    RenderView(pDI, hrgnDraw, NULL);
                }
    void        RenderView(CFormDrawInfo * pDI, const RECT * prcDraw)
                {
                    RenderView(pDI, NULL, prcDraw);
                }

    void        RefreshSettings();

#if DBG==1
    void        DumpDisplayTree() const;
    BOOL        IsDisplayTreeOpen() const;
#endif

    void        ForceRelayout();

    void        SetFocus(CElement * pElement, long  iSection);
    void        InvalidateFocus() const;

    CDispScroller * HitScrollInset(CPoint *pptHit, DWORD *pdwScrollDir);
    HTC         HitTestPoint(CMessage * pMessage, CTreeNode ** ppTreeNode, DWORD grfFlags);
    HTC         HitTestPoint(
                        const POINT &       pt,
                        COORDINATE_SYSTEM   cs,
                        CElement *          pElement,
                        DWORD               grfFlags,
                        HITTESTRESULTS *    phtr,
                        CTreeNode **        ppTreeNode,
                        POINT &             ptContent,
                        CDispNode **        ppDispNode);

    void        Invalidate(const CRect * prc = NULL, BOOL fSynchronousRedraw = FALSE, BOOL fInvalChildWindows = FALSE, BOOL fPostRender = TRUE);
    void        Invalidate(const RECT * prc, BOOL fSynchronousRedraw = FALSE, BOOL fInvalChildWindows = FALSE, BOOL fPostRender = TRUE)
                {
                    Invalidate((const CRect *)prc, fSynchronousRedraw, fInvalChildWindows, fPostRender);
                }
    void        Invalidate(const CRegion& rgn, BOOL fSynchronousRedraw = FALSE, BOOL fInvalChildWindows = FALSE);
    void        Invalidate(HRGN hrgn, BOOL fSynchronousRedraw = FALSE, BOOL fInvalChildWindows = FALSE)
                {
                    Invalidate(CRegion(hrgn), fSynchronousRedraw, fInvalChildWindows);
                }
    void        InvalidateBorder(long cBorder);

    void        Notify(CNotification * pnf);

    void        SetFlag(VIEWFLAGS f);
    void        ClearFlag(VIEWFLAGS f);
    BOOL        IsFlagSet(VIEWFLAGS f) const;
    
    //
    //  Task methods
    //

    HRESULT     AddLayoutTask(CLayout * pLayout, DWORD grfLayout = 0);
    void        ExecuteLayoutTask(CLayout * pLayout);
    void        RemoveLayoutTask(const CLayout * pLayout);
    BOOL        HasLayoutTask(const CLayout * pLayout = NULL) const;
    DWORD       GetLayoutFlags() const;

    HRESULT     AddEventTask(CElement * pElement, DISPID dispEvent, DWORD grfLayout = 0);
    void        RemoveEventTask(const CElement * pElement, const DISPID dispEvent);
    void        RemoveEventTasks(CElement * pElement);
    BOOL        HasEventTask(const CElement * pElement = NULL) const;

    HRESULT     AddRecalcTask(CElement *pElement);
    void        RemoveRecalcTask(CElement *pElement);
    BOOL        HasRecalcTask(const CElement *pElement = NULL) const;

    void        RequestRecalc();

    // 
    //
    //  Adorner methods
    //

    CAdorner *  CreateAdorner(CElement * pElement);
    CAdorner *  CreateAdorner(long cpStart, long cpEnd);
    void        RemoveAdorner(CAdorner * pAdorner);
    void        RemoveAdorners(CElement * pElement);
    BOOL        HasAdorners() const;
    BOOL        HasAdorners(CElement * pElement) const;
#if DBG==1
    BOOL        IsValidAdorner(CAdorner * pAdorner);
#endif

    //
    //  Deferred calls methods
    //

    void        EndDeferred();
    void        FlushDeferSetWindowPos() { EndDeferSetWindowPos(); }

    void        DeferSetObjectRects(IOleInPlaceObject * pInPlaceObject,
                                    RECT * prcObj, RECT * prcClip,
                                    HWND hwnd, BOOL fInvalidate);
    void        DeferSetWindowPos(HWND hwnd, const RECT * prc, UINT uFlags, const RECT * prcInvalid);
    void        DeferSetWindowRgn(HWND hwnd, const RECT * prc, BOOL fRedraw);
    void        DeferTransition(COleSite *pOleSite);

    //
    //  HWND managers
    //
    
    void        GetHWNDRect(HWND hwnd, CRect *prcWndActual) const;
    int         IndexOfClippingOuterWindow(HWND hwnd);
    BOOL        IsClippingOuterWindow(HWND hwnd)
                    { return IndexOfClippingOuterWindow(hwnd) >= 0; }
    void        AddClippingOuterWindow(HWND hwnd);
    void        RemoveClippingOuterWindow(HWND hwnd);

    //
    //  Helper classes
    //
    //      CWindowOrderInfo   - FixWindowZOrder helper
    //      CEnsureDisplayTree - Stack based object to ensure display tree is recalculated
    // 
    
    class CWindowOrderInfo
    {
    public:
        CWindowOrderInfo();

        void AddWindow(HWND hwnd);
        void SetWindowOrder();

    private:    
        HDWP    _hdwp;
        HWND    _hwndAfter;
    };

    class CEnsureDisplayTree
    {
    public:
        CEnsureDisplayTree(CView * pView);
        ~CEnsureDisplayTree();

    private:
        CView * _pView;
        BOOL    _fTreeOpen;
    };
    friend class CEnsureDisplayTree;
    
protected:
    enum VIEWLOCKS              // View lock flags
    {
        VL_ENSUREINPROGRESS     = 0x00000001,   // EnsureView is executing
        VL_UPDATEINPROGRESS     = 0x00000002,   // EnsureView is updating the view (may generate WM_PAINT messages)
        VL_ACCUMULATINGINVALID  = 0x00000004,   // EnsureView is accumulating invalid rectangles/regions
        VL_TASKSINPROGRESS      = 0x00000008,   // EnsureView is processing tasks

        VL_RENDERINPROGRESS     = 0x00000010,   // RenderView is processing
        VL_TASKCANRUNSCRIPT     = 0x00000020,   // A task that can run script is processing

        VL_MAX
    };

    enum PRIVATEVIEWFLAGS       // Private view flags - These occupy the upper 16 bits of _grfFlags
    {
        VF_ACTIVE               = 0x00010000,   // TRUE if view is activated, FALSE otherwise
        VF_ATTACHED             = 0x00020000,   // TRUE if the top layout dispnode is attached to CDispRoot
        VF_SIZED                = 0x00040000,   // TRUE if the top layout is sized, FALSE otherwise
        VF_TREEOPEN             = 0x00080000,   // TRUE if the display tree is open, FALSE otherwise

        VF_PENDINGENSUREVIEW    = 0x00100000,   // TRUE if a call to EnsureView is pending, FALSE otherwise
        VF_PENDINGTRANSITION    = 0x00200000,   // TRUE if a call to EndDeferTransition is pending, FALSE otherwise
        VF_NEEDSRECALC          = 0x00400000,   // TRUE if the recalc engine has requested a chance to run
        VF_FORCEPAINT           = 0x00800000,   // TRUE if a WM_PAINT should be forced (to make up for one missed earlier)

        VF_INVALCHILDWINDOWS    = 0x01000000,   // TRUE if child windows should be invalidated
        VF_HASRENDERED          = 0x02000000,   // TRUE if view has rendered at least one time
        
        VF_PRIVATEMAX
    };

    struct SOR                  // Description of a deferred SetObjectsRect call
    {
        IOleInPlaceObject * pInPlaceObject;
        RECT                rc;
        RECT                rcClip;
        HWND                hwnd;
        BOOL                fInvalidate;
    };

    struct TRANSITIONTO_INFO    // Description of a deferred TransitionTo call
    {
        COleSite *          pOleSite;
    };

    struct WND_RGN              // Deferred SetWindowRgn call
    {
        HWND                hwnd;
        CRect               rc;
        BOOL                fRedraw;
    };

    struct WND_POS              // Deferred window positions
    {
        HWND                hwnd;
        CRect               rc;
        UINT                uFlags;
    };

    DECLARE_CDataAry(CAryVTaskMisc,   CViewTask,         Mt(Mem), Mt(CView_aryTaskMisc_pv))
    DECLARE_CDataAry(CAryVTaskLayout, CViewTask,         Mt(Mem), Mt(CView_aryTaskLayout_pv))
    DECLARE_CDataAry(CAryVTaskEvent,  CViewTask,         Mt(Mem), Mt(CView_aryTaskEvent_pv))
    DECLARE_CDataAry(CAryVTaskRecalc, CViewTask,         Mt(Mem), Mt(CView_aryTaskRecalc_pv))
    DECLARE_CDataAry(CAryAdorners,    CAdorner *,        Mt(Mem), Mt(CView_aryAdorners_pv))
    DECLARE_CDataAry(CSorAry,         SOR,               Mt(Mem), Mt(CView_arySor_pv))
    DECLARE_CDataAry(CTransToAry,     TRANSITIONTO_INFO, Mt(Mem), Mt(CView_aryTransition_pv))
    DECLARE_CDataAry(CWndPosAry,      WND_POS,           Mt(Mem), Mt(CView_aryWndPos_pv))
    DECLARE_CDataAry(CWndRgnAry,      WND_RGN,           Mt(Mem), Mt(CView_aryWndRgn_pv))
    DECLARE_CDataAry(CHWNDAry,        HWND,              Mt(Mem), Mt(CView__aryClippingOuterHwnd_pv))

    CDoc *          _pDoc;                  // Associated CDoc
    DWORD           _grfLocks;              // Lock flags (collection of VL_xxxx flags)

    CDispRoot *     _pDispRoot;             // Root of the display tree
    long            _cEnsureDisplayTree;    // Depth of open CEnsureDisplayTree objects

    enum            {MAX_INVALID=8};        // Maximum number of invalid rectangles
    long            _cInvalidRects;         // Current number of invalid rectangles
    CRect           _aryInvalidRects[MAX_INVALID];  // Array of invalid rectangles

    CRegion         _rgnInvalid;            // Current invalid region (may be NULL)

    CViewDispClient _client;                // IDispClient/IDispObserver object

    CLayout *       _pLayout;               // Last sized CLayout (resize required when differs from GetRootLayout)

    CAryVTaskMisc   _aryTaskMisc;           // Array of misc tasks
    CAryVTaskLayout _aryTaskLayout;         // Array of layout tasks
    CAryVTaskEvent  _aryTaskEvent;          // Array of event tasks
    CAryVTaskRecalc _aryTaskRecalc;         // Array of recalc tasks
    DWORD           _grfLayout;             // Collection of LAYOUT_xxxx flags (only used with layout tasks)

    CSorAry         _arySor;                // Deferred SetObjectRect calls
    CWndPosAry      _aryWndPos;             // Deferred SetWindowPos calls
    CWndRgnAry      _aryWndRgn;             // Deferred SetWindowRgn calls
    CTransToAry     _aryTransition;         // Deferred Transition calls
    CHWNDAry        _aryClippingOuterHwnd;  // HWNDs that behave like MFC outer windows

    CAryAdorners    _aryAdorners;           // Array of CAdorners

    CFocusAdorner * _pFocusAdorner;         // Focus adorner

    long            _cpStartMeasured;       // Accumulated measured range start cp (-1 if clean)
    long            _cpEndMeasured;         // Accumulated measured range end cp

    long            _cpStartTranslated;     // Accumulated translated range start cp (-1 if clean)
    long            _cpEndTranslated;       // Accumulated translated range end cp
    CSize           _sizeTranslated;        // Translation amount (zero if unknown)

    DWORD           _grfFlags;              // Public and private flags (see VIEWFLAGS and PRIVATEVIEWFLAGS)

#if DBG==1
    BOOL            _fDEBUGRecursion;       // TRUE if illegal EnsureView recursion occurred
#endif

    //
    //  Core view methods
    //

    CDoc *      Doc() const;
    BOOL        OpenView(BOOL fBackground, BOOL fPostClose, BOOL fResetTree);
    void        CloseView(DWORD grfLayout = 0L);
    void        PostCloseView(BOOL fBackground = FALSE);
    void        PostRenderView(BOOL fSynchronousRedraw = FALSE);
    void        PostEndDeferTransition();

    void        DrawSynchronous();

    void        EndDeferSetObjectRects(DWORD grfLayout = 0, BOOL fIgnore = FALSE);
    void        EndDeferSetWindowPos(DWORD grfLayout = 0, BOOL fIgnore = FALSE);
    void        EndDeferSetWindowRgn(DWORD grfLayout = 0, BOOL fIgnore = FALSE);
    void        EndDeferTransition(DWORD grfLayout = 0, BOOL fIgnore = FALSE);

    void        EnsureFocus();
    void        EnsureSize(DWORD grfLayout);

    NV_DECLARE_ONCALL_METHOD(EndDeferTransitionCallback, enddefertransitioncallback, (DWORD_PTR grfLayout));
    NV_DECLARE_ONCALL_METHOD(EnsureViewCallback, ensureviewcallback, (DWORD_PTR grfLayout));

    void        FixWindowZOrder();

    CLayout *   GetRootLayout() const;

    BOOL        IsAttached(CLayout * pLayout) const;
    BOOL        IsSized(CLayout * pLayout) const;

    BOOL        HasInvalid() const;
    void        ScrollInvalid(const CRect& rcScroll, const CSize& sizeScrollDelta);
    void        ClearInvalid();
    void        PublishInvalid(DWORD grfLayout);
    void        MergeInvalid();

    void        SetFlag(PRIVATEVIEWFLAGS f);
    void        ClearFlag(PRIVATEVIEWFLAGS f);
    BOOL        IsFlagSet(PRIVATEVIEWFLAGS f) const;

    //
    //  Helpers
    //
    
    void        SetObjectRectsHelper(
                            IOleInPlaceObject * pInPlaceObject,
                            RECT *              prcObj,
                            RECT *              prcClip,
                            HWND                hwnd,
                            BOOL                fInvalidate);

    //
    //  Display tree methods
    //

    void        OpenDisplayTree();
    void        CloseDisplayTree();
    void        EnsureDisplayTreeIsOpen();
    CDispRoot * GetDisplayTree() const;
    BOOL        WantOffscreenBuffer() const;
    BOOL        AllowOffscreenBuffer() const;
    BOOL        AllowScrolling() const;
    BOOL        AllowSmoothScrolling() const;


    //
    //  Task methods
    //

    CAryVTasks * GetTaskList(CViewTask::VIEWTASKTYPE vtt) const;

    HRESULT     AddTask(void * pv, CViewTask::VIEWTASKTYPE vtt, DWORD grfLayout, LONG lData = 0);

    void        ExecuteTask(void * pv, CViewTask::VIEWTASKTYPE vtt);

    BOOL        ExecuteLayoutTasks(DWORD grfLayout);
    BOOL        ExecuteEventTasks(DWORD  grfLayout);
public:
    // These methods need to be called from some OM calls
    BOOL        ExecuteRecalcTasks(DWORD grfLayout);
protected:

    int         FindTask(const CAryVTasks * pTaskList, const void * pv, CViewTask::VIEWTASKTYPE vtt, const LONG lData = 0) const;
    int         FindTask(const CAryVTasks * pTaskList, const CViewTask & vt) const;

    CViewTask * GetTask(const void * pv, CViewTask::VIEWTASKTYPE vtt) const;
    CViewTask * GetTask(const CViewTask & vt) const;

    CViewTask * GetNextTask(CViewTask::VIEWTASKTYPE vtt, DWORD grfLayout) const;
    CViewTask * GetNextTaskInSourceOrder(CViewTask::VIEWTASKTYPE vtt, DWORD grfLayout) const;

    BOOL        HasTask(const void * pv, CViewTask::VIEWTASKTYPE vtt, const LONG lData = 0) const;
    BOOL        HasTask(const CViewTask & vt) const;
    BOOL        HasTasks() const;

    void        RemoveTask(const void * pv, CViewTask::VIEWTASKTYPE vtt, const LONG lData = 0);
    void        RemoveTask(const CViewTask & vt);
    void        RemoveTask(CAryVTasks * TaskList, int iTask);

    //
    //  Accumulated range methods
    //
    
    void        ClearRanges();
    BOOL        HasMeasuredRange() const;
    BOOL        HasTranslatedRange() const;
    
    //
    //  Adorner methods
    //

    HRESULT     AddAdorner(CAdorner * pAdorner);
    void        DeleteAdorners();
    long        GetAdorner(CElement * pElement, long  iAdorner = 0) const;
    void        RemoveAdorner(CAdorner * pAdorner, BOOL fCheckForLast);

    void        AccumulateMeasuredRange(long cp, long cch);
    void        AccumulateTranslatedRange(const CSize & size, long cp, long cch);
    BOOL        HasDirtyRange() const;
    void        UpdateAdorners(DWORD grfLayout);

    //
    //  Lock methods
    //

    void        SetLocks(DWORD grfLocks);
    void        ClearLocks(DWORD grfLocks);
    BOOL        IsLockSet(DWORD grfLocks) const;

    class CLock
    {
    public:
        CLock(CView * pView, DWORD grfLocks);
        ~CLock();

    private:
        CView * _pView;
        DWORD   _grfLocks;
    };
    friend class CLock;

private:
    NO_COPY(CView);
};


//+----------------------------------------------------------------------------
//
//  Inlines
//
//-----------------------------------------------------------------------------

inline CDoc *
CViewDispClient::Doc() const
{
    return View()->Doc();
}

inline CView *
CViewDispClient::View() const
{
    Assert(&(CONTAINING_RECORD(this, CView, _client)->_client) == this);
    return CONTAINING_RECORD(this, CView, _client);
}

inline BOOL 
CView::IsInitialized() const
{
    return (_pDoc != NULL);
}

inline BOOL
CView::IsActive() const
{
    Assert(!IsFlagSet(VF_ACTIVE) || _pDispRoot);
    return IsFlagSet(VF_ACTIVE);
}

inline DWORD
CView::GetState() const
{
    DWORD   grfState = 0;

    if (IsFlagSet(VF_TREEOPEN))
    {
        grfState = VS_OPEN;
    }

    if (IsLockSet(VL_ENSUREINPROGRESS))
    {
        grfState |= VS_INLAYOUT;
    }
    else if (IsLockSet(VL_RENDERINPROGRESS))
    {
        grfState |= VS_INRENDER;
    }

    return grfState;
}

inline BOOL
CView::IsInState(
    VIEWSTATE   vs) const
{
    switch (vs)
    {
    case VS_OPEN:       return IsFlagSet(VF_TREEOPEN);
    case VS_INLAYOUT:   return IsLockSet(VL_ENSUREINPROGRESS);
    case VS_INRENDER:   return IsLockSet(VL_RENDERINPROGRESS);
    default:            return FALSE;
    }
}

inline void
CView::ForceRelayout()
{
    if (IsActive())
    {
        OpenView();
        _grfLayout |= LAYOUT_FORCE;
    }
}

inline void
CView::GetViewRect(
    CRect * prc)
{
    Assert(prc);

    CPoint  pt;
    CSize   size;

    GetViewPosition(&pt);
    GetViewSize(&size);

    prc->SetRect(pt, size);
}

inline BOOL
CView::OpenView(
    BOOL    fBackground,
    BOOL    fResetTree)
{
    return OpenView(fBackground, TRUE, fResetTree);
}

inline void
CView::SetViewSize(const SIZE & size)
{
    SetViewSize((const CSize &)size);
}

inline void
CView::SetFlag(
    VIEWFLAGS   f)
{
    _grfFlags |= f;
}

inline void
CView::ClearFlag(
    VIEWFLAGS   f)
{
    _grfFlags &= ~f;
}

inline BOOL
CView::IsFlagSet(
    VIEWFLAGS   f) const
{
    return !!(_grfFlags & f);
}

inline void
CView::ExecuteLayoutTask(
    CLayout * pLayout)
{
    ExecuteTask(pLayout, CViewTask::VTT_LAYOUT);
}

inline void
CView::RemoveLayoutTask(
    const CLayout * pLayout)
{
    RemoveTask(pLayout, CViewTask::VTT_LAYOUT);
}

inline BOOL
CView::HasLayoutTask(
    const CLayout * pLayout) const
{
    return HasTask(pLayout, CViewTask::VTT_LAYOUT);
}

inline DWORD
CView::GetLayoutFlags() const
{
    return _grfLayout;
}

inline HRESULT
CView::AddEventTask(CElement * pElement, DISPID dispEvent, DWORD grfLayout)
{
    Assert(pElement);
    HRESULT hr = AddTask(pElement, CViewTask::VTT_EVENT, grfLayout, dispEvent);
    if (SUCCEEDED(hr))
    {
        pElement->_fHasPendingEvent = TRUE;
    }
    return hr;
}

inline void
CView::RemoveEventTask(const CElement * pElement, const DISPID dispEvent)
{
    RemoveTask(pElement, CViewTask::VTT_EVENT, dispEvent);
}

inline BOOL
CView::HasEventTask(
    const CElement * pElement) const
{
    return HasTask(pElement, CViewTask::VTT_EVENT);
}

inline HRESULT
CView::AddRecalcTask(CElement *pElement)
{
    Assert(pElement);
    HRESULT hr = S_OK;

    if (!pElement->_fHasPendingRecalcTask)
    {
        hr = AddTask(pElement, CViewTask::VTT_RECALC, 0, 0);
        if (!hr)
            pElement->_fHasPendingRecalcTask = TRUE;
    }

    RRETURN(hr);
}

inline void
CView::RemoveRecalcTask(CElement *pElement)
{
    if (pElement->_fHasPendingRecalcTask)
    {
        RemoveTask(pElement, CViewTask::VTT_RECALC, 0);
        pElement->_fHasPendingRecalcTask = FALSE;
    }
}

inline BOOL
CView::HasRecalcTask(const CElement *pElement) const
{
    Assert(HasTask(pElement, CViewTask::VTT_RECALC) == !!pElement->_fHasPendingRecalcTask);
    return pElement->_fHasPendingRecalcTask;
}

inline void
CView::RemoveAdorner(
    CAdorner * pAdorner)
{
    RemoveAdorner(pAdorner, TRUE);
}

inline BOOL
CView::HasAdorners() const
{
    return _aryAdorners.Size() != 0;
}

inline BOOL
CView::HasAdorners(
    CElement *  pElement) const
{
    return (GetAdorner(pElement) >= 0);
}

#if DBG==1
inline BOOL
CView::IsValidAdorner(
    CAdorner *  pAdorner)
{
    return (_aryAdorners.FindIndirect(&pAdorner) >= 0);
}
#endif

inline CDispRoot *
CView::GetDisplayTree() const
{
    return _pDispRoot;
}

inline CDoc *
CView::Doc() const
{
    return _pDoc;
}

inline BOOL
CView::IsAttached(
    CLayout *   pLayout) const
{
    return  IsFlagSet(VF_ATTACHED)
        &&  _pLayout == pLayout;
}

inline BOOL
CView::IsSized(
    CLayout *   pLayout) const
{
    return  IsFlagSet(VF_SIZED)
        &&  pLayout == _pLayout;
}

inline BOOL
CView::HasInvalid() const
{
    Assert( !IsFlagSet(VF_INVALCHILDWINDOWS)
        ||  _cInvalidRects
        ||  !_rgnInvalid.IsEmpty());
    return  _cInvalidRects
        ||  !_rgnInvalid.IsEmpty();
}

inline void
CView::SetFlag(
    PRIVATEVIEWFLAGS    f)
{
    _grfFlags |= f;
}

inline void
CView::ClearFlag(
    PRIVATEVIEWFLAGS    f)
{
    _grfFlags &= ~f;
}

inline BOOL
CView::IsFlagSet(
    PRIVATEVIEWFLAGS    f) const
{
    return !!(_grfFlags & f);
}

inline void
CView::EnsureDisplayTreeIsOpen()
{
    if (!IsFlagSet(VF_TREEOPEN))
    {
        OpenDisplayTree();
    }
}

inline CAryVTasks *
CView::GetTaskList(
    CViewTask::VIEWTASKTYPE vtt) const
{
    switch (vtt)
    {
    case CViewTask::VTT_EVENT:
        return (CAryVTasks *)(&_aryTaskEvent);
    case CViewTask::VTT_LAYOUT:
        return (CAryVTasks *)(&_aryTaskLayout);
    case CViewTask::VTT_RECALC:
        return (CAryVTasks *)(&_aryTaskRecalc);
    }
    return (CAryVTasks *)(&_aryTaskMisc);
}

inline BOOL
CView::HasTask(
    const void *            pv,
    CViewTask::VIEWTASKTYPE vtt,
    const LONG              lData) const
{
    return (FindTask(GetTaskList(vtt), pv, vtt, lData) >= 0);
}

inline BOOL
CView::HasTask(
    const CViewTask &   vt) const
{
    return (FindTask(GetTaskList(vt.GetType()), vt) >= 0);
}

inline void
CView::RemoveTask(
    const void *            pv,
    CViewTask::VIEWTASKTYPE vtt,
    const LONG              lData)
{
    Assert(pv);
    Assert(vtt);
    RemoveTask(GetTaskList(vtt), FindTask(GetTaskList(vtt), pv, vtt, lData));
}

inline void
CView::RemoveTask(
    const CViewTask &   vt)
{
    Assert(vt.GetObject());
    Assert(vt.GetType());

    RemoveTask(GetTaskList(vt.GetType()), FindTask(GetTaskList(vt.GetType()), vt));
}

inline int
CView::FindTask(
    const CAryVTasks *      pTaskList,
    const void *            pv,
    CViewTask::VIEWTASKTYPE vtt,
    const LONG      lData) const
{
    return FindTask(pTaskList, CViewTask((void *)pv, vtt, (LONG) lData));
}

inline CViewTask *
CView::GetTask(
    const void *            pv,
    CViewTask::VIEWTASKTYPE vtt) const
{
    return GetTask(CViewTask((void *)pv, vtt));
}

inline BOOL
CView::HasTasks() const
{
    return (   _aryTaskMisc.Size()   != 0
            || _aryTaskRecalc.Size() != 0
            || _aryTaskLayout.Size() != 0
            || _aryTaskEvent.Size()  != 0);
}

inline void
CView::ClearRanges()
{
    _cpStartMeasured   =
    _cpEndMeasured     =
    _cpStartTranslated =
    _cpEndTranslated   = -1;
}

inline BOOL
CView::HasDirtyRange() const
{
    return  HasMeasuredRange()
        ||  HasTranslatedRange();
}

inline BOOL
CView::HasMeasuredRange() const
{
    return (_cpStartMeasured >= 0);
}

inline BOOL
CView::HasTranslatedRange() const
{
    return (_cpStartTranslated >= 0);
}

inline void
CView::SetLocks(
    DWORD   grfLocks)
{
    _grfLocks |= grfLocks;
}

inline void
CView::ClearLocks(
    DWORD   grfLocks)
{
    _grfLocks &= ~grfLocks;
}

inline BOOL
CView::IsLockSet(
    DWORD   grfLocks) const
{
    return (_grfLocks & grfLocks);
}

inline
CView::CEnsureDisplayTree::CEnsureDisplayTree(
    CView * pView)
{
    Assert(pView);
    _pView     = pView;
    _fTreeOpen = pView->IsFlagSet(VF_TREEOPEN);

    _pView->_cEnsureDisplayTree++;
    _pView->EnsureDisplayTreeIsOpen();
}


inline
CView::CEnsureDisplayTree::~CEnsureDisplayTree()
{
    Assert(_pView);

    _pView->_cEnsureDisplayTree--;
    if (!_pView->_cEnsureDisplayTree)
    {
        _pView->CloseDisplayTree();

        if (_fTreeOpen)
        {
            _pView->EnsureDisplayTreeIsOpen();
        }
    }
}

inline
CView::CLock::CLock(
    CView * pView,
    DWORD   grfLocks)
{
    Assert(pView);

    _pView    = pView;
    _grfLocks = pView->_grfLocks;

    _pView->_grfLocks |= grfLocks;
}

inline
CView::CLock::~CLock()
{
    Assert(_pView);
    _pView->_grfLocks = _grfLocks;
}

#pragma INCMSG("--- End 'view.hxx'")
#else
#pragma INCMSG("*** Dup 'view.hxx'")
#endif
