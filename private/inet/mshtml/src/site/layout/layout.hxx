//+----------------------------------------------------------------------------
//
// File:        LAYOUT.HXX
//
// Contents:    CLayout class
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_LAYOUT_HXX_
#define I_LAYOUT_HXX_
#pragma INCMSG("--- Beg 'layout.hxx'")

#ifndef X_ILAYOUT_HXX_
#define X_ILAYOUT_HXX_
#include "ilayout.hxx"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif


MtExtern(CLayout)
MtExtern(CRequest)
MtExtern(CBgRecalcInfo)
MtExtern(CLayout_aryRequests_pv)
MtExtern(CLayoutScopeFlag)

class CPostData;
class CFilter;
class CLabelElement;
class CDispNode;
class CDispItemPlus;
class CDispInteriorNode;
class CDispContainer;
class CRecalcTask;
class CStackPageBreaks;

//+---------------------------------------------------------------------------
//
//  Enums
//
//----------------------------------------------------------------------------

#ifndef NAVIGATE_ENUM
#   define NAVIGATE_ENUM
    enum NAVIGATE_DIRECTION
    {
        NAVIGATE_UP     = 0x0001,
        NAVIGATE_DOWN   = 0x0002,
        NAVIGATE_LEFT   = 0x0004,
        NAVIGATE_RIGHT  = 0x0008
    };
#endif

enum LAYOUTDESC_FLAG
{
    LAYOUTDESC_FLOWLAYOUT       = 0x00000001,
    LAYOUTDESC_TABLELAYOUT      = 0x00000002,
    LAYOUTDESC_TABLECELL        = 0x00000004,
    LAYOUTDESC_NOSCROLLBARS     = 0x00000008,
    LAYOUTDESC_HASINSETS        = 0x00000010,
    LAYOUTDESC_NOTALTERINSET    = 0x00000020,
    LAYOUTDESC_NEVEROPAQUE      = 0x00000040,           // always transparent
};

//+---------------------------------------------------------------------------
//
// Bit fields describing how to draw selection feedback.
//
//----------------------------------------------------------------------------

enum DSI_FLAGS
{
    DSI_PASSIVE         = 0x00000000,   // Site is passive in form
    DSI_DOMINANT        = 0x00000001,   // Site is dominant in form
    DSI_LOCKED          = 0x00000002,   // Site is locked
    DSI_UIACTIVE        = 0x00000004,   // Site is UI active
    DSI_NOLEFTHANDLES   = 0x00000008,   // Don't draw grab handles on the left
    DSI_NOTOPHANDLES    = 0x00000010,   // ... on the top
    DSI_NORIGHTHANDLES  = 0x00000020,   // ... on the right
    DSI_NOBOTTOMHANDLES = 0x00000040,   // ... on the bottom
    DSI_FLAGS_Last_Enum
};

enum HT_FLAG
{
    HT_IGNOREINDRAG          = 0x00000001,  // ignore this site if it's being dragged.
    HT_SCROLLTESTONLY        = 0x00000002,  // prevents CScrollSite from delegating to CSite
    HT_VIRTUALHITTEST        = 0x00000004,  // do a virtual hit-test.
    HT_IGNORESCROLL          = 0x00000008,  // ignore scroll bars from hit-testing.
    HT_SKIPSITES             = 0x00000010,  // Only do element hit testing

    // The next four flags are specific to CFlowLayout, passed down to element from point
    HT_ALLOWEOL              = 0x00000020,  // Allow hitting end of line
    HT_DONTIGNOREBEFOREAFTER = 0x00000040,  // Ignore white spaces?
    HT_NOEXACTFIT            = 0x00000080,  // Exact fit?
    HT_NOGRABTEST            = 0x00000100,  // Test for grab handles?

    HT_ELEMENTSWITHLAYOUT    = 0x00000200,  // Return only elements that have their own layout engine
};


enum IR_FLAG
{
    IR_ALL        = 0x00000001,     // Invalidate all of both RECTs
    IR_RIGHTEDGE  = 0x00000002,     // Invalidate the difference between the right-hand edges
    IR_BOTTOMEDGE = 0x00000004,     // Invalidate the difference between the bottom edges
    IR_USECLIENT  = 0x00000008,     // Use the top/right edges of client RECT when invalidating

    IR_MOST       = LONG_MAX
};

//+---------------------------------------------------------------------------
//
//  Enumeration: for Move
//
//--------------------------------------------------------------------------

enum SITEMOVE_FLAGS
{
    SITEMOVE_NOINVALIDATE   = 0x0001, // don't invalidate the rect
    SITEMOVE_NOSETEXTENT    = 0x0002, // don't call SetExtent on the control inside
    SITEMOVE_POPULATEDIR    = 0x0004, // 0 - UP, 1 - DOWN BUGBUG revisit later (istvanc)
    SITEMOVE_NOUNDO         = 0x0008, // don't save undo information
    SITEMOVE_NOMOVECHILDREN = 0x0010, // BUGBUG temp until relative coordinates (istvanc)
    SITEMOVE_NONOTIFYPARENT = 0x0020, // don't call OnMove on parent
    SITEMOVE_NODIRTY        = 0x0040, // don't set the dirty flag
    SITEMOVE_NOFIREEVENT    = 0x0080, // don't fire a move event
    SITEMOVE_INVALIDATENEW  = 0x0100, // invalidate newly exposed areas
    SITEMOVE_NORESIZE       = 0x0200, // don't set width or height values
    SITEMOVE_RESIZEONLY     = 0x0400, // don't set top or left values
    SITEMOVE_MAX            = LONG_MAX    // needed to force enum to be dword
};

enum SS_FLAGS
{
    SS_ADDTOSELECTION       = 0x01,
    SS_REMOVEFROMSELECTION  = 0x02,
    SS_KEEPOLDSELECTION     = 0x04,
    SS_NOSELECTCHANGE       = 0x08,
    SS_DONTEXPANDGROUPS     = 0x10,
    SS_SETSELECTIONSTATE    = 0x20,
    SS_MERGESELECTION       = 0x40,
    SS_CLEARSELECTION       = 0x80
};

// default smooth scrolling interval
#define MAX_SCROLLTIME      125

//+------------------------------------------------------------------------
//
//  Enumeration: SITE_INTERSECT flags for CheckSiteIntersect
//
//-------------------------------------------------------------------------
enum SITE_INTERSECT
{
    SI_BELOW            = 0x00000001,
    SI_BELOWWINDOWED    = 0x00000010,   // Requires SI_BELOW
    SI_ABOVE            = 0x00000100,
    SI_ABOVEWINDOWED    = 0x00001000,   // Requires SI_ABOVE
    SI_OPAQUE           = 0x00010000,   // Mutually exclusive with SI_TRANSPARENT
    SI_TRANSPARENT      = 0x00100000,   // Mutually exclusive with SI_OPAQUE
    SI_CHILDREN         = 0x01000000,

    SI_MAX       = LONG_MAX
};

//+------------------------------------------------------------------------
//
//  BACKGROUNDINFO - Describes background color/image
//
//-------------------------------------------------------------------------
struct BACKGROUNDINFO
{
    CImgCtx *           pImgCtx;           // Fill background with image
    LONG                lImgCtxCookie;     // Image context cookie
    RECT                rcImg;             // The tile rect for the image
    BOOL                fFixed;            // TRUE if watermark
    LONG                xScroll;           // xScroll of site
    LONG                yScroll;           // yScroll of site
    POINT               ptBackOrg;         // Logical background origin
    COLORREF            crBack;            // Background color
    COLORREF            crTrans;           // Transparent color (tiling)
};

#define DECLARE_LAYOUTDESC_MEMBERS                              \
    static const LAYOUTDESC   s_layoutdesc;                     \
    virtual const CLayout::LAYOUTDESC *GetLayoutDesc() const    \
        { return (CLayout::LAYOUTDESC *)&s_layoutdesc;}


//+----------------------------------------------------------------------------
//
//  CScrollbarProperties - Collection of requested scrollbar properties
//
//-----------------------------------------------------------------------------

class CScrollbarProperties
{
public:
    BOOL    _fHSBAllowed;
    BOOL    _fVSBAllowed;
    BOOL    _fHSBForced;
    BOOL    _fVSBForced;
};


//+----------------------------------------------------------------------------
//
//  CDispNodeInfo - A property bag useful in determining the type of
//                  display node needed
//
//-----------------------------------------------------------------------------

class CDispNodeInfo
{
public:
    ELEMENT_TAG             _etag;
    DISPNODELAYER           _layer;
    DISPNODEBORDER          _dnbBorders;
    styleOverflow           _overflowX;
    styleOverflow           _overflowY;
    VISIBILITYMODE          _visibility;
    CBorderInfo             _bi;
    CScrollbarProperties    _sp;

    union
    {
        DWORD       _grfFlags;

        struct
        {
            DWORD   _fHasInset              :1;
            DWORD   _fHasBackground         :1;
            DWORD   _fHasFixedBackground    :1;
            DWORD   _fIsOpaque              :1;

            DWORD   _fIsScroller            :1;
            DWORD   _fHasUserClip           :1;
            DWORD   _fRTL                   :1;

            DWORD   _fUnused                :25;
        };
    };

    ELEMENT_TAG     Tag() const
                    {
                        return _etag;
                    }

    BOOL            IsTag(ELEMENT_TAG etag) const
                    {
                        return _etag == etag;
                    }

    DISPNODELAYER   GetLayer() const
                    {
                        return _layer;
                    }

    DISPNODEBORDER  GetBorderType() const
                    {
                        return _dnbBorders;
                    }

    long            GetSimpleBorderWidth() const
                    {
                        return _bi.aiWidths[0];
                    }

    void            GetComplexBorderWidths(CRect & rc) const
                    {
                        rc.top    = _bi.aiWidths[BORDER_TOP];
                        rc.left   = _bi.aiWidths[BORDER_LEFT];
                        rc.bottom = _bi.aiWidths[BORDER_BOTTOM];
                        rc.right  = _bi.aiWidths[BORDER_RIGHT];
                    }

    void            GetBorderWidths(CRect * prc) const
                    {
                        prc->top    = _bi.aiWidths[BORDER_TOP];
                        prc->left   = _bi.aiWidths[BORDER_LEFT];
                        prc->bottom = _bi.aiWidths[BORDER_BOTTOM];
                        prc->right  = _bi.aiWidths[BORDER_RIGHT];
                    }

    BOOL            HasInset() const
                    {
                        return _fHasInset;
                    }

    BOOL            IsScroller() const
                    {
                        return _fIsScroller;
                    }

    BOOL            IsHScrollbarAllowed() const
                    {
                        return _sp._fHSBAllowed;
                    }

    BOOL            IsVScrollbarAllowed() const
                    {
                        return _sp._fVSBAllowed;
                    }

    BOOL            IsHScrollbarForced() const
                    {
                        return _sp._fHSBForced;
                    }

    BOOL            IsVScrollbarForced() const
                    {
                        return _sp._fVSBForced;
                    }

    BOOL            HasScrollbars() const
                    {
                        return  _sp._fHSBAllowed
                            ||  _sp._fVSBAllowed;
                    }

    BOOL            HasUserClip() const
                    {
                        return _fHasUserClip;
                    }

    BOOL            HasBackground() const
                    {
                        return _fHasBackground;
                    }

    BOOL            HasFixedBackground() const
                    {
                        return _fHasFixedBackground;
                    }

    BOOL            IsOpaque() const
                    {
                        return _fIsOpaque;
                    }

    styleOverflow   GetOverflowX() const
                    {
                        return _overflowX;
                    }

    styleOverflow   GetOverflowY() const
                    {
                        return _overflowY;
                    }

    styleOverflow   GetOverflow() const
                    {
                        return (_overflowX > _overflowY
                                    ? _overflowX
                                    : _overflowY);
                    }

    VISIBILITYMODE  GetVisibility() const
                    {
                        return _visibility;
                    }

    BOOL            IsRTL() const
                    {
                        return _fRTL;
                    }

};


//+----------------------------------------------------------------------------
//
//  CHitTestInfo - Client-side data passed to display tree during hit-testing
//
//-----------------------------------------------------------------------------

class CHitTestInfo
{
public:
    HTC                 _htc;
    HITTESTRESULTS *    _phtr;
    CTreeNode *         _pNodeElement;
    CDispNode *         _pDispNode;
    CPoint              _ptContent;
    DWORD               _grfFlags;
};


//+----------------------------------------------------------------------------
//
//  CRequest - Description of a re-size, z-change, or positioning request
//             for a positioned element
//
//-----------------------------------------------------------------------------

class CRequest
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRequest))

    enum REQUESTFLAGS
    {
        RF_MEASURE      = 0x00000001,
        RF_POSITION     = 0x00000002,
        RF_ADDADORNER   = 0x00000004,

        RF_AUTOVALID    = 0x00010000,
    };

    CRequest(REQUESTFLAGS rf, CElement * pElement);
    ~CRequest();

    DWORD       AddRef();
    DWORD       Release();

    void        SetFlag(REQUESTFLAGS rf);
    void        ClearFlag(REQUESTFLAGS rf);
    BOOL        IsFlagSet(REQUESTFLAGS rf) const;

    CElement *  GetElement() const;

    BOOL        QueuedOnLayout(CLayout * pLayout) const;
    void        DequeueFromLayout(CLayout * pLayout);

    CLayout *   GetLayout(REQUESTFLAGS rf) const;
    void        SetLayout(REQUESTFLAGS rf, CLayout * pLayout);

    CAdorner *  GetAdorner() const;
    void        SetAdorner(CAdorner * pAdorner);

    CPoint &    GetAuto();
    void        SetAuto(const CPoint & ptAuto, BOOL fAutoValid);

protected:
    DWORD       _cRefs;
    CElement *  _pElement;
    CAdorner *  _pAdorner;
    CPoint      _ptAuto;
    DWORD       _grfFlags;

    CLayout *   _pLayoutMeasure;
    CLayout *   _pLayoutPosition;
    CLayout *   _pLayoutAdorner;
};


//+----------------------------------------------------------------------------
//
//  CBgRecalcInfo - Background recalc object
//
//-----------------------------------------------------------------------------

class CBgRecalcInfo
{
public:
    CBgRecalcInfo()     { memset( this, 0, sizeof(CBgRecalcInfo) ); }
    ~CBgRecalcInfo()    {}

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBgRecalcInfo))

    LONG _cpWait;                       // cp WaitForRecalc() is waiting for (or < 0)
    LONG _yWait;                        // y WaitForRecalc() is waiting for (or < 0)

    CRecalcTask *       _pRecalcTask;   // The recalc task
    DWORD               _dwBgndTickMax; // time till we can execute recalclines
};

//+---------------------------------------------------------------------------
//
//  Flag values for CLayout::RegionFromElement
//
//----------------------------------------------------------------------------

// If no flags are specified, RFE returns a rect/region suitable for drawing
// backgrounds and borders.
enum RFE_FLAGS
{
    RFE_SCREENCOORD         = 1,
    RFE_IGNORE_ALIGNED      = 2,        // don't look at aligned elements
    RFE_IGNORE_BLOCKNESS    = 4,        // handle as an in-lined element
    RFE_IGNORE_CLIP_RC_BOUNDS = 8,      // ignore adjustment for clipping CPs to smaller than element start/finish
    RFE_IGNORE_RELATIVE     = 16,       // don't look at relatively positioned elements
    RFE_TIGHT_RECTS         = 32,       // the rect of an element correctly adjusted for its height
    RFE_NESTED_REL_RECTS    = 64,       // Do we get rects for nested rel elements
    RFE_SCROLL_INTO_VIEW    = 128,      // are we called by scrollintoview
};

// These are the most commonly used flag combinations.
#define RFE_HITTEST         0                       // includes aligned elements. 
                                                    // if block element returns block, else returns non-tight text rects 
                                                    // (uses line height)
#define RFE_SELECTION       (RFE_IGNORE_BLOCKNESS | RFE_NESTED_REL_RECTS)  // includes aligned elements. always returns non-tight text rects
#define RFE_ELEMENT_RECT    (RFE_TIGHT_RECTS)       // used to request a bounding rect that includes aligned/floated elements.
#define RFE_BACKGROUND      (RFE_IGNORE_ALIGNED | RFE_TIGHT_RECTS | RFE_IGNORE_CLIP_RC_BOUNDS)
                                                    // used to request the background rect for backgrounds and
                                                    // borders. does not include aligned elements and does not adjust
                                                    // CPs to smaller than element start/finish

//+----------------------------------------------------------------------------
//
//  CLayout
//
//-----------------------------------------------------------------------------

class CLayout : public CDispClient
{
    friend class CElement;
    friend class CFilter;
public:

    DECLARE_FORMS_STANDARD_IUNKNOWN(CLayout)

    // Construct / Destruct
    CLayout(CElement *pElementLayout);  // Normal constructor.
    virtual ~CLayout();

    virtual HRESULT Init();
    virtual HRESULT OnExitTree();

    virtual void    Detach();
    virtual void    Reset(BOOL fForce);

            CElement *  ElementContent() const;
            CElement *  ElementOwner() const
                        {
                            return _pElementOwner;
                        }
            ELEMENT_TAG Tag() const
                        {
                            return ElementOwner()->Tag();
                        }
#if DBG == 1 || defined(DUMPTREE)
            int         SN () const
                        {
                            return ElementOwner()->SN();
                        }
#endif

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLayout))

    virtual void        DoLayout(DWORD grfLayout);
    virtual void        Notify(CNotification * pnf);
    virtual HRESULT     OnFormatsChange(DWORD dwFlags) { return S_OK;}

            void        HandleVisibleChange(BOOL fVisibility);

            void        HandleElementMeasureRequest(CCalcInfo * pci, CElement *  pElement, BOOL fEditable);

            BOOL        HandlePositionNotification(CNotification * pnf);
            void        HandlePositionRequest(CElement * pElement, const CPoint & ptAuto, BOOL fAutoValid);

            BOOL        HandleAddAdornerNotification(CNotification * pnf);
            void        HandleAddAdornerRequest(CAdorner * pAdorner);

            void        HandleZeroGrayChange(CNotification* pnf);

    // helper function to calculate absolutely positioned child layout
    virtual void        CalcAbsolutePosChild(CCalcInfo *pci, CLayout *pChildLayout);
            
    CDoc *          Doc() const;

    // These functions let the layout access the content (that needs to be laid out and
    // rendered) in an abstract manner, so that the layout does not have worry about
    // whether the content lives in the master markup or the slave maekup.
    CMarkup *   GetContentMarkup() const;
    void        GetContentTreeExtent(CTreePos ** pptpStart, CTreePos ** pptpEnd);
    long        GetContentFirstCp();
    long        GetContentLastCp();
    long        GetContentTextLength();

    //
    // View helpers
    //

    CView * GetView() const
            {
                return Doc()->GetView();
            }
    BOOL    OpenView(BOOL fBackground = FALSE, BOOL fResetTree = FALSE)
            {
                return Doc()->GetView()->OpenView(fBackground, fResetTree);
            }

    void    EndDeferred()
            {
                GetView()->EndDeferred();
            }
    void    DeferSetObjectRects(IOleInPlaceObject * pInPlaceObject,
                                RECT * prcObj, RECT * prcClip,
                                HWND hwnd, BOOL fInvalidate)
            {
                GetView()->DeferSetObjectRects(pInPlaceObject, prcObj, prcClip,
                                                hwnd, fInvalidate);
            }
    void    DeferSetWindowPos(HWND hwnd, const RECT * prc, UINT uFlags, const RECT* prcInvalid)
            {
                GetView()->DeferSetWindowPos(hwnd, prc, uFlags, prcInvalid);
            }
    void    DeferSetWindowRgn(HWND hwnd, const RECT * prc, BOOL fRedraw)
            {
                GetView()->DeferSetWindowRgn(hwnd, prc, fRedraw);
            }
    void    DeferTransition(COleSite *pOleSite)
            {
                GetView()->DeferTransition(pOleSite);
            }

    //
    // Useful to determine if a site is percent sized or not.
    //
    virtual BOOL PercentSize();
    virtual BOOL PercentWidth();
    virtual BOOL PercentHeight();


    virtual CFlowLayout *   IsFlowLayout()   { return NULL; }
    virtual CTableLayout *  IsTableLayout()  { return NULL; }
    virtual CLayout *       IsFlowOrSelectLayout() {return NULL; }

    // element delegates OnPropertyChange to layouts
    virtual HRESULT OnPropertyChange ( DISPID dispid, DWORD dwFlags );

    //
    // Helper methods
    //
    virtual void RegionFromElement( CElement       * pElement,
                                    CDataAry<RECT> * paryRects,
                                    RECT           * prcBound,
                                    DWORD            dwFlags);


    void    PostLayoutRequest(DWORD grfLayout = 0)
            {
                Doc()->GetView()->AddLayoutTask(this, grfLayout);
            }
    void    RemoveLayoutRequest()
            {
                Doc()->GetView()->RemoveLayoutTask(this);
            }

    // Enumeration method to loop thru children (start)
    virtual CLayout *   GetFirstLayout (
                                DWORD_PTR * pdw,
                                BOOL    fBack = FALSE,
                                BOOL    fRaw = FALSE)
                        {
                            return NULL;
                        }

    // Enumeration method to loop thru children (continue)
    virtual CLayout *   GetNextLayout (
                                DWORD_PTR * pdw,
                                BOOL    fBack = FALSE,
                                BOOL    fRaw = FALSE )
                        {
                            return NULL;
                        }

    virtual BOOL        ContainsChildLayout(BOOL fRaw = FALSE)
                        {
                            return FALSE;
                        }

    virtual void        ClearLayoutIterator(DWORD_PTR dw, BOOL fRaw = FALSE) {}

    virtual CFlowLayout *GetFlowLayoutAtPoint(POINT pt) { return NULL; }
    virtual CFlowLayout *GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp, BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL)
                        {
                            return GetUpdatedParentLayout()->GetNextFlowLayout(iDir, ptPosition, ElementOwner(), pcp, pfCaretNotAtBOL, pfAtLogicalBOL);
                        }

    // Returns complete list of elements, including positioned elements that
    // are not direct children of us, in the correct ZOrder (bottom-most =
    // index 0)
    virtual HRESULT     GetElementsInZOrder(
                                CPtrAry<CElement *> *parySites,
                                CElement            *pElementThis,
                                RECT *               prc = NULL,
                                HRGN                 hrgn = NULL,
                                BOOL                 fIncludeNotVisible = FALSE);

            BOOL        IsDisplayNone()
                        {
                            return ElementOwner()->IsDisplayNone();
                        }

    virtual BOOL        CanHaveChildren()
                        {
                            return FALSE;
                        }

    //+-------------------------------------------------------------------------
    //
    //  Enumeration: LAYOUT_ZORDER zorder manipulation
    //
    //--------------------------------------------------------------------------

    enum LAYOUT_ZORDER
    {
        SZ_FRONT    = 0,
        SZ_BACK     = 1,
        SZ_FORWARD  = 2,
        SZ_BACKWARD = 3,

        SZ_MAX      = LONG_MAX    // Force enum to 32 bits
    };

    // set z order for layout
    //
    virtual HRESULT     SetZOrder(CLayout * pLayout, LAYOUT_ZORDER zorder, BOOL fUpdate)
                        {
                            return E_FAIL;
                        }

    // Override CElement's handlemessage
    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);
            void    PrepareMessage(CMessage * pMessage, CDispNode * pDispNode = NULL);

    // Helper for handling keydowns on the site
    HRESULT HandleKeyDown(CMessage *pMessage, CElement *pElemChild);

    virtual BOOL    IsDirty()   { return FALSE; }

    //
    //  Size and position manipulations
    //

    virtual DWORD   CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);

        CRequest *  QueueRequest(CRequest::REQUESTFLAGS rf, CElement * pElement);
            void    FlushRequests();
            void    ProcessRequests(CCalcInfo * pci, const CSize & size);
            BOOL    ProcessRequest(CCalcInfo * pci, CRequest  * pRequest);
            void    ProcessRequest(CElement * pElement);

            void    NotifyMeasuredRange(long cpStart, long cpEnd);
            void    NotifyTranslatedRange(const CSize & size, long cpStart = -1, long cpFinish = -1);

    virtual void    TranslateRelDispNodes(const CSize & size, long lStart = 0)  { }
    virtual void    ZChangeRelDispNodes() { }

    //
    //  Lookaside routines for tracking requests from positioned children
    //

    enum
    {
        LOOKASIDE_REQUESTQUEUE      = 0,
        LOOKASIDE_BGRECALCINFO      = 1,
        LOOKASIDE_LAYOUT_NUMBER     = 2

        // *** There are only 2 bits reserved in the layout.
        // *** if you add more lookasides you have to make sure 
        // *** that you make room for those bits.
    };

    inline  BOOL    HasLookasidePtr(int iPtr);
    inline  void *  GetLookasidePtr(int iPtr);
    inline  HRESULT SetLookasidePtr(int iPtr, void * pvVal);
    inline  void *  DelLookasidePtr(int iPtr);

    DECLARE_CPtrAry(CRequests, CRequest *, Mt(Mem), Mt(CLayout_aryRequests_pv))

    inline  BOOL        HasRequestQueue()                       { return HasLookasidePtr(LOOKASIDE_REQUESTQUEUE); }
    inline  CRequests * RequestQueue()                          { return (CRequests *)GetLookasidePtr(LOOKASIDE_REQUESTQUEUE); }
    inline  HRESULT     AddRequestQueue(CRequests * pRequests)  { return SetLookasidePtr(LOOKASIDE_REQUESTQUEUE, pRequests); }
    inline  CRequests * DeleteRequestQueue()                    { return (CRequests *)DelLookasidePtr(LOOKASIDE_REQUESTQUEUE); }

    inline  BOOL        HasBgRecalcInfo()                   { return HasLookasidePtr(LOOKASIDE_BGRECALCINFO); }
    inline  CBgRecalcInfo * BgRecalcInfo();
    inline  HRESULT     EnsureBgRecalcInfo();
    inline  void        DeleteBgRecalcInfo();
    inline  LONG        YWait()                             { return HasBgRecalcInfo() ? BgRecalcInfo()->_yWait         : -1;   }
    inline  LONG        CPWait()                            { return HasBgRecalcInfo() ? BgRecalcInfo()->_cpWait        : -1;   }
    inline  CRecalcTask * RecalcTask()                      { return HasBgRecalcInfo() ? BgRecalcInfo()->_pRecalcTask   : NULL; }
    inline  DWORD       BgndTickMax()                       { return HasBgRecalcInfo() ? BgRecalcInfo()->_dwBgndTickMax :  0;   }
    inline  BOOL        CanHaveBgRecalcInfo();

#if DBG==1
            union
            {
                void *  _apLookAside[LOOKASIDE_LAYOUT_NUMBER];
                struct
                {
                    CRequests *     _pRequests;
                    CBgRecalcInfo * _pBgRecalcInfo;
                };
            };
#endif

            void    TransformPoint(
                        CPoint* ppt,
                        COORDINATE_SYSTEM source,
                        COORDINATE_SYSTEM destination,
                        CDispNode * pDispNode = NULL) const;

            void    TransformRect(
                        RECT* prc,
                        COORDINATE_SYSTEM source,
                        COORDINATE_SYSTEM destination,
                        BOOL fClip = FALSE,
                        CDispNode * pDispNode = NULL) const;

            void    GetRect(CRect * prc, COORDINATE_SYSTEM cs = COORDSYS_PARENT) const;
            void    GetRect(RECT * prc, COORDINATE_SYSTEM cs = COORDSYS_PARENT) const
                    {
                        GetRect((CRect *)prc, cs);
                    }

            void    GetClippedRect(CRect * prc, COORDINATE_SYSTEM cs = COORDSYS_PARENT) const;

            void    GetSize(CSize * psize) const;
            void    GetSize(SIZE * psize) const
                    {
                        GetSize((CSize *)psize);
                    }
            long    GetHeight() const
                    {
                        CSize   size;

                        GetSize(&size);
                        return size.cy;
                    }
            long    GetWidth() const
                    {
                        CSize   size;

                        GetSize(&size);
                        return size.cx;
                    }

    virtual void    GetContentSize(CSize * psize, BOOL fActualSize = TRUE) const;
            void    GetContentSize(SIZE * psize, BOOL fActualSize = TRUE) const
                    {
                        GetContentSize((CSize *)psize, fActualSize);
                    }
            void    GetContentRect(CRect *prc, COORDINATE_SYSTEM cs) const;

            long    GetContentHeight(BOOL fActualSize = TRUE) const
                    {
                        CSize   size;

                        GetContentSize(&size, fActualSize);
                        return size.cy;
                    }
            long    GetContentWidth(BOOL fActualSize = TRUE) const
                    {
                        CSize   size;

                        GetContentSize(&size, fActualSize);
                        return size.cx;
                    }

            void    GetPosition(CPoint * ppt, COORDINATE_SYSTEM cs = COORDSYS_PARENT) const;
            void    GetPosition(POINT * ppt, COORDINATE_SYSTEM cs = COORDSYS_PARENT) const
                    {
                        GetPosition((CPoint *)ppt, cs);
                    }
            long    GetPositionTop(COORDINATE_SYSTEM cs = COORDSYS_PARENT) const
                    {
                        CPoint  pt;

                        GetPosition(&pt, cs);
                        return pt.y;
                    }
            long    GetPositionLeft(COORDINATE_SYSTEM cs = COORDSYS_PARENT) const
                    {
                        CPoint  pt;

                        GetPosition(&pt, cs);
                        return pt.x;
                    }

            void    SetPosition(const CPoint & pt, BOOL fNotifyAuto = FALSE);
            void    SetPosition(const POINT & pt, BOOL fNotifyAuto = FALSE)
                    {
                        SetPosition((const CPoint &)pt, fNotifyAuto);
                    }
            void    SetPosition(long x, long y, BOOL fNotifyAuto = FALSE)
                    {
                        SetPosition(CPoint(x, y), fNotifyAuto);
                    }


    //  Returns client rectangle (available for children)
            void    GetClientRect(
                            CRect *             prc,
                            COORDINATE_SYSTEM   cs   = COORDSYS_CONTENT,
                            CLIENTRECT          crt  = CLIENTRECT_CONTENT,
                            CDocInfo *          pdci = NULL) const;
            void    GetClientRect(
                            RECT *              prc,
                            COORDINATE_SYSTEM   cs   = COORDSYS_CONTENT,
                            CLIENTRECT          crt  = CLIENTRECT_CONTENT,
                            CDocInfo *          pdci = NULL) const
                    {
                        GetClientRect((CRect *)prc, cs, crt, pdci);
                    }
            void    GetClientRect(
                            CRect *     prc,
                            CLIENTRECT  crt,
                            CDocInfo *  pdci = NULL) const
                    {
                        GetClientRect(prc, COORDSYS_CONTENT, crt, pdci);
                    }
            void    GetClientRect(
                            RECT *      prc,
                            CLIENTRECT  crt,
                            CDocInfo *  pdci = NULL) const
                    {
                        GetClientRect((CRect *)prc, COORDSYS_CONTENT, crt, pdci);
                    }

            long    GetClientWidth() const
                    {
                        CRect   rc;

                        GetClientRect(&rc);
                        return rc.Width();
                    }
            long    GetClientHeight() const
                    {
                        CRect   rc;

                        GetClientRect(&rc);
                        return rc.Height();
                    }

            long    GetClientTop(COORDINATE_SYSTEM cs = COORDSYS_CONTENT) const
                    {
                        CRect   rc;

                        GetClientRect(&rc, cs);
                        return rc.top;
                    }
            long    GetClientBottom(COORDINATE_SYSTEM cs = COORDSYS_CONTENT) const
                    {
                        CRect   rc;

                        GetClientRect(&rc, cs);
                        return rc.bottom;
                    }
            long    GetClientLeft(COORDINATE_SYSTEM cs = COORDSYS_CONTENT) const
                    {
                        CRect   rc;

                        GetClientRect(&rc, cs);
                        return rc.left;
                    }
            long    GetClientRight(COORDINATE_SYSTEM cs = COORDSYS_CONTENT) const
                    {
                        CRect   rc;

                        GetClientRect(&rc, cs);
                        return rc.right;
                    }

            void    GetClippedClientRect(
                            CRect *             prc,
                            COORDINATE_SYSTEM   cs   = COORDSYS_CONTENT,
                            CLIENTRECT          crt  = CLIENTRECT_CONTENT) const;
            void    GetClippedClientRect(
                            RECT *              prc,
                            COORDINATE_SYSTEM   cs   = COORDSYS_CONTENT,
                            CLIENTRECT          crt  = CLIENTRECT_CONTENT) const
                    {
                        GetClippedClientRect((CRect *)prc, cs, crt);
                    }
            void    GetClippedClientRect(
                            CRect *     prc,
                            CLIENTRECT  crt) const
                    {
                        GetClippedClientRect(prc, COORDSYS_CONTENT, crt);
                    }
            void    GetClippedClientRect(
                            RECT *      prc,
                            CLIENTRECT  crt) const
                    {
                        GetClippedClientRect((CRect *)prc, COORDSYS_CONTENT, crt);
                    }

            void    GetAutoPosition(
                            CElement  *  pElement,
                            CElement  *  pElementZParent,
                            CDispNode ** ppDNZParent,
                            CLayout   *  pLayoutParent,
                            CPoint    *  ppt,
                            BOOL         fAutoValid);

            void    GetParentTopLeft(CPoint * ppt);

    virtual void    GetPositionInFlow(
                            CElement *  pElement,
                            CPoint   *  ppt)
    {
        AssertSz(FALSE, "Should be handled by a parent layout");
        *ppt = g_Zero.pt;
    }

    virtual HRESULT GetChildElementTopLeft(POINT & pt, CElement * pChild);
    virtual void    GetMarginInfo(CParentInfo *ppri,
                                LONG * plLeftMargin, LONG * plTopMargin,
                                LONG * plRightMargin, LONG *plBottomMargin);

    // For object model to return proper position regardless if object is parked

            void    RestrictPointToClientRect(POINT *ppt);

    //
    // Scrolling
    //
            void    AttachScrollbarController(CDispNode * pDispNode, CMessage * pMessage);
            void    DetachScrollbarController(CDispNode * pDispNode);

            HRESULT ScrollElementIntoView(
                                CElement *  pElement = NULL,
                                SCROLLPIN   spVert = SP_MINIMAL,
                                SCROLLPIN   spHorz = SP_MINIMAL,
                                BOOL        fScrollBits = TRUE);
            virtual HRESULT ScrollRangeIntoView(
                                long        cpMin,
                                long        cpMost,
                                SCROLLPIN   spVert = SP_MINIMAL,
                                SCROLLPIN   spHorz = SP_MINIMAL,
                                BOOL        fScrollBits = TRUE);
            BOOL    ScrollRectIntoView(const CRect & rc, SCROLLPIN spVert, SCROLLPIN spHorz, BOOL fScrollBits);

            void    ScrollH(long lCode)
                    {
                        OnScroll(0, lCode);
                    }
            void    ScrollV(long lCode)
                    {
                        OnScroll(1, lCode);
                    }

            BOOL    ScrollBy(const CSize & sizeDelta, LONG lScrollTime = 0);
            BOOL    ScrollByPercent(const CSize & sizePercent, LONG lScrollTime = 0);
            BOOL    ScrollByPercent(long dx, long dy, LONG lScrollTime = 0)
                    {
                        return ScrollByPercent(CSize(dx, dy), lScrollTime);
                    }

            BOOL    ScrollTo(const CSize & sizeOffset, LONG lScrollTime = 0);
            BOOL    ScrollTo(long x, long y, LONG lScrollTime = 0)
                    {
                        return ScrollTo(CSize(x, y), lScrollTime);
                    }

            void    ScrollToX(long x, LONG lScrollTime = 0);
            void    ScrollToY(long y, LONG lScrollTime = 0);

            long    GetXScroll() const;
            long    GetYScroll() const;

            HRESULT BUGCALL OnScroll(int iDirection, UINT uCode, long lPosition = 0, BOOL fRepeat = FALSE, LONG lScrollTime = 0);

            _htmlComponent  ComponentFromPoint(long x, long y);

    //
    // Compute which direction to scroll in based on position of point in
    // scrolling inset.   Return TRUE iff scrolling is possible.
    virtual BOOL    GetInsetScrollDir(POINT pt, UINT * pDir)
                    {
                        return FALSE;
                    }

    virtual void    AdjustSizeForBorder(SIZE * pSize, CDocInfo * pdci, BOOL fInflate);

    virtual BOOL    GetBackgroundInfo(CFormDrawInfo * pDI, BACKGROUNDINFO * pbginfo, BOOL fAll = TRUE, BOOL fRightToLeft = FALSE);

    virtual void    Draw(CFormDrawInfo *pDI, CDispNode * pDispNode = NULL);

            void    DrawTextSelectionForSite(CFormDrawInfo *pDI);

            void    DrawZeroBorder(CFormDrawInfo *pDI);

            virtual VOID    ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected, BOOL fLayoutCompletelyEnclosed, BOOL fFireOM );

            virtual HRESULT OnSelectionChange(void) { return S_OK; }
            
            void SetSelected(BOOL fSelected, BOOL fInvalidate = FALSE );

            HRESULT GetDC(RECT *prc, DWORD dwFlags, HDC *phDC);
            HRESULT ReleaseDC(HDC hdc);

            void    DrawBackground(CFormDrawInfo * pDI, BACKGROUNDINFO * pbginfo, RECT *prc = NULL);

            BOOL    NeedsSurface() {return _fSurface;}
            void    SetSurfaceFlags(BOOL fSurface, BOOL f3DSurface, BOOL fIgnoreFilter = FALSE);

            void    DrawSelectInfo(HDC hdc, RECT *prc, DWORD dwInfo);
            void    DrawGrabBorders(HDC hdc, RECT *prc, BOOL fHatch);
            void    DrawGrabHandles(HDC hdc, RECT *prc, DWORD dwInfo);
            HTC     HitTestSelect(POINT pt, RECT *prc);
            HTC     HitTestPoint(
                            const CPoint &  pt,
                            CTreeNode **    ppNodeElement,
                            DWORD           grfFlags);
// BUGBUG: Can this routine be removed? (brendand)
            BOOL    HitTestRect(RECT *  prc);

            void    Invalidate(const RECT& rc, COORDINATE_SYSTEM cs);
            void    Invalidate(const RECT * prc = NULL, int nRects = 1, const RECT * prcClip = NULL);
            void    Invalidate(HRGN hrgn);


    void SetIsAdorned( BOOL fAdorned );
    BOOL IsAdorned();
    BOOL IsCalcingSize ();
    void SetCalcing( BOOL fState );



    virtual BOOL    OnSetCursor(CMessage *pMessage);

    // Drag/drop methods
    //
            HRESULT DoDrag(
                        DWORD                       dwKeyState,
                        IUniformResourceLocator *   pURLToDrag = NULL,
                        BOOL                        fCreateDataObjOnly = FALSE);
    virtual HRESULT PreDrag(
                        DWORD dwKeyState,
                        IDataObject **ppDO,
                        IDropSource **ppDS);
    virtual HRESULT PostDrag(HRESULT hrDrop, DWORD dwEffect);
    virtual HRESULT DragEnter(
                        IDataObject *pDataObj,
                        DWORD grfKeyState,
                        POINTL ptl,
                        DWORD *pdwEffect);
    virtual HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    virtual HRESULT Drop(
                        IDataObject *pDataObj,
                        DWORD grfKeyState,
                        POINTL ptl,
                        DWORD *pdwEffect)
                    {
                        return E_FAIL;
                    }
    virtual HRESULT DragLeave();
            void    DragHide();
    virtual HRESULT ParseDragData(IDataObject *pDataObj);
    virtual void    DrawDragFeedback()
                    { }
    virtual HRESULT InitDragInfo(IDataObject *pDataObj, POINTL ptlScreen)
                    {
                        return S_FALSE;
                    }
    virtual HRESULT UpdateDragFeedback(POINTL ptlScreen)
                    {
                        return E_FAIL;
                    }
            HRESULT DropHelper(POINTL ptlScreen, DWORD dwAllowed, DWORD *pdwEffect, LPTSTR lptszFileType);

    virtual HRESULT PageBreak(LONG yStart, LONG yIdeal, CStackPageBreaks * paryValues);
    virtual HRESULT ContainsPageBreak(long yTop, long yBottom, long * pyBreak, BOOL * pfPageBreakAfterAtTopLevel = NULL);
            BOOL    HasPageBreakBefore();
            BOOL    HasPageBreakAfter();



    void    SetSiteTextSelection (BOOL fSelected, BOOL fSwap) ; // Set the Text Selected Bit


            BOOL    TestClassFlag(CElement::ELEMENTDESC_FLAG dwFlag) const
                    {
                        return ElementOwner()->TestClassFlag(dwFlag);
                    }
            BOOL    IsEditable(BOOL fCheckContainerOnly = FALSE)
                    {
                        return ElementOwner()->IsEditable(fCheckContainerOnly);
                    }
            BOOL    IsAligned()
                    {
                        return ElementOwner()->IsAligned();
                    }
            void    SetXProposed(LONG xPos)
                    {
                        _ptProposed.x = xPos;
                    }

            void    SetYProposed(LONG yPos)
                    {
                        _ptProposed.y = yPos;
                    }

            LONG    GetXProposed()
                    {
                        return _ptProposed.x;
                    }

            LONG    GetYProposed()
                    {
                        return _ptProposed.y;
                    }

            BOOL    TestLock(CElement::ELEMENTLOCK_FLAG enumLockFlags)
                    {
                        return ElementOwner()->TestLock(enumLockFlags);
                    }

            CTreeNode * GetFirstBranch()  const
                    {
                        return ElementOwner()->GetFirstBranch();
                    }
            CLayout *   GetCurParentLayout()
                    {
                        return GetFirstBranch()
                                    ? GetFirstBranch()->GetCurParentLayout()
                                    : NULL;
                    }
            CLayout *   GetUpdatedParentLayout()
                    {
                        return GetFirstBranch()
                                    ? GetFirstBranch()->GetUpdatedParentLayout()
                                    : NULL;
                    }

    // Adjust the unit value coords of the site
            HRESULT Move (RECT *prcpixels, DWORD dwFlags = 0);

    // Class Descriptor
    struct LAYOUTDESC
    {
        DWORD                   _dwFlags;           // any combination of the above LAYOUTDESC_FLAG

        BOOL TestFlag(LAYOUTDESC_FLAG dw) const { return (_dwFlags & dw) != 0; }
    };

    BOOL TestLayoutDescFlag(LAYOUTDESC_FLAG dw) const
    {
        Assert(GetLayoutDesc());
        return GetLayoutDesc()->TestFlag(dw);
    }

    virtual const CLayout::LAYOUTDESC *GetLayoutDesc() const = 0;

    class CScopeFlag
    {
    public:
        DECLARE_MEMALLOC_NEW_DELETE(Mt(CLayoutScopeFlag))
        CScopeFlag (CLayout * pLayoutParent )
        {
            _pLayout = pLayoutParent;
            if (pLayoutParent)
            {
                /// we need to cached the old value, so that 
                // nested CScopeFlags work properly
                _fCalcing = _pLayout->_fCalcingSize;
                _pLayout->SetCalcing( TRUE );
            }
        };
        ~CScopeFlag()
        {
            if (_pLayout)
                _pLayout->SetCalcing( _fCalcing );
        };

    private:
        CLayout * _pLayout;
        unsigned _fCalcing:1; 
    };

    //+-----------------------------------------------------------------------
    //
    //  Member Data
    //
    //------------------------------------------------------------------------
#if DBG==1
    DWORD       _snLast;            // Last notification received (DEBUG builds only)
#endif

    CElement  * _pElementOwner;     // element that owns this layout

    //
    // This pointer points to either a ped if the
    // element is in a tree or otherwise points to the
    // document.
    //

    void *              __pvChain;

#if DBG == 1
    CDoc *              _pDocDbg;
    CMarkup *           _pMarkupDbg;
#endif

    union
    {
        DWORD   _dwFlags;

        struct
        {
            unsigned    _fSurface                : 1;   // 0
            unsigned    _f3DSurface              : 1;   // 1
            unsigned    _fSizeThis               : 1;   // 2
            unsigned    _fForceLayout            : 1;   // 3
            unsigned    _fEnsureDispNode         : 1;   // 4
            unsigned    _fHasLookasidePtr        : 2;   // 5-6
            unsigned    _fContentsAffectSize     : 1;   // 7
            unsigned    _fAdorned                : 1;   // 8
            unsigned    _fCalcingSize            : 1;   // 9
            unsigned    _fVisible                : 1;   // 10
            unsigned    _fInvisibleAtRuntime     : 1;   // 11
            unsigned    _fGrabInside             : 1;   // 12
            unsigned    _fWantGrabInside         : 1;   // 13
            unsigned    _fIsDragDropSrc          : 1;   // 14
            unsigned    _fTextSelected           : 1;   // 15
            unsigned    _fSwapColor              : 1;   // 16
            unsigned    _fCanHaveChildren        : 1;   // 17
            unsigned    _fNoUIActivate           : 1;   // 18
            unsigned    _fNoUIActivateInDesign   : 1;   // 19
            unsigned    _fOwnTabOrder            : 1;   // 20
            unsigned    _fAllowSelectionInDialog : 1;   // 21
            unsigned    _fClearedSiteMark        : 1;   // 22
            unsigned    _fDetached               : 1;   // 23
            unsigned    _fMinMaxValid            : 1;   // 24
            unsigned    _fHasMarkupPtr           : 1;   // 25 - TRUE if this layout has a pointer to a markup
            unsigned    _fVertical               : 1;   // 26 - Vertical display property
            unsigned    _fAutoBelow              : 1;   // 27 - Is itself or contains auto/relative positioned elements
            unsigned    _fContainsRelative       : 1;   // 28 - contains relatively positioned elements
            unsigned    _fPositionSet            : 1;   // 29 - set to true when the layout is positioned for the first time.
            unsigned    _fIsEditable             : 1;   // 30 - is editable?
            unsigned    _fEditableDirty          : 1;   // 31 - isEditable cache void
        };
    };

protected:
    POINT       _ptProposed;        // Layout position (set/owned by the object's container)

public:
    //
    //  CDispClient overrides
    //

    void GetOwner(
                CDispNode *     pDispNode,
                void **         ppv);

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
                int            whichScrollbar,
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

    BOOL HitTestContent(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestFuzzy(
                const POINT *  pptHitInParentCoords,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestScrollbar(
                int            whichScrollbar,
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestScrollbarFiller(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    BOOL HitTestBorder(
                const POINT *pptHit,
                CDispNode *pDispNode,
                void *pClientData);

    BOOL ProcessDisplayTreeTraversal(
                void *pClientData);

    LONG GetZOrderForSelf();
    LONG GetZOrderForChild(
                void *         cookie);

    LONG CompareZOrder(
                CDispNode *    pDispNode1,
                CDispNode *    pDispNode2);

    // BUGBUG (donmarsh) - need implementation for GetFilter()
    CDispFilter* GetFilter();

    void HandleViewChange(
                DWORD          flags,
                const RECT *   prcClient,
                const RECT *   prcClip,
                CDispNode *    pDispNode);

    virtual void NotifyScrollEvent(
                RECT *  prcScroll,
                SIZE *  psizeScrollDelta);

    virtual DWORD GetClientLayersInfo(CDispNode *pDispNodeFor);

    DWORD GetPeerLayersInfo();

    void DrawClientLayers(
                const RECT* prcBounds,
                const RECT* prcRedraw,
                CDispSurface *pSurface,
                CDispNode *pDispNode,
                void *cookie,
                void *pClientData,
                DWORD dwFlags);

#if DBG==1
    void DumpDebugInfo(
                HANDLE         hFile,
                long           level,
                long           childNumber,
                CDispNode *    pDispNode,
                void *         cookie);
#endif

protected:
    CDispNode *     _pDispNode;

public:
    virtual void    GetFlowPosition(CDispNode * pDispNode, CPoint * ppt)
                    {
                        *ppt = g_Zero.pt;
                        Assert("This should only be called on a flow layout");
                    }

    virtual CDispNode * GetElementDispNode(CElement * pElement = NULL) const;
    virtual void        SetElementDispNode(CElement * pElement, CDispNode * pDispNode);
    CDispItemPlus * GetFirstContentDispNode(CDispNode * pDispNode = NULL) const;
    CDispNode *     GetLayerParentDispNode(DISPNODELAYER layer) const;

    void            GetDispNodeInfo(CDispNodeInfo * pdni, CDocInfo * pdci = NULL, BOOL fBorders = FALSE) const;
    void            GetDispNodeScrollbarProperties(CDispNodeInfo * pdni) const;

    HRESULT         EnsureDispNode(CDocInfo * pdci, BOOL fForce = FALSE);

    void            EnsureDispNodeLayer(DISPNODELAYER layer, CDispNode * pDispNode = NULL);
    void            EnsureDispNodeLayer(const CDispNodeInfo & dni, CDispNode * pDispNode = NULL)
                    {
                        EnsureDispNodeLayer(dni.GetLayer(), pDispNode);
                    }
    void            EnsureDispNodeLayer(CDispNode *pDispNode = NULL)
                    {
                        CDispNodeInfo   dni;

                        GetDispNodeInfo(&dni);
                        EnsureDispNodeLayer(dni.GetLayer(), pDispNode);
                    }
    void            EnsureDispNodeLayer(DISPNODELAYER layer, CElement * pElement)
                    {
                        Assert(GetElementDispNode(pElement));
                        EnsureDispNodeLayer(layer, GetElementDispNode(pElement));
                    }

    void            EnsureDispNodeBackground(const CDispNodeInfo & dni, CDispNode * pDispNode = NULL);
    void            EnsureDispNodeBackground(CDispNode * pDispNode = NULL)
                    {
                        CDispNodeInfo   dni;

                        GetDispNodeInfo(&dni);
                        EnsureDispNodeBackground(dni, pDispNode);
                    }

    CDispContainer* EnsureDispNodeIsContainer(CElement * pElement = NULL);

    void            EnsureDispNodeScrollbars(CDocInfo * pdci, const CScrollbarProperties & sp, CDispNode * pDispNode = NULL);
    void            EnsureDispNodeScrollbars(CDocInfo * pdci, const CDispNodeInfo & dni, CDispNode * pDispNode = NULL)
                    {
                        EnsureDispNodeScrollbars(pdci, dni._sp, pDispNode);
                    }

    VISIBILITYMODE  VisibilityModeFromStyle(styleVisibility visibility) const
                    {
                        VISIBILITYMODE  visibilityMode;

                        switch (visibility)
                        {
                        case styleVisibilityVisible:
                            visibilityMode = VISIBILITYMODE_VISIBLE;
                            break;
                        case styleVisibilityHidden:
                            visibilityMode = VISIBILITYMODE_INVISIBLE;
                            break;
                        default:
                            visibilityMode = VISIBILITYMODE_INHERIT;
                            break;
                        }

                        return visibilityMode;
                    }

    void            EnsureDispNodeVisibility(VISIBILITYMODE visibility, CElement * pElement, CDispNode * pDispNode = NULL);
    void            EnsureDispNodeVisibility(CElement *pElement = NULL, CDispNode * pDispNode = NULL);

    virtual void    EnsureContentVisibility(CDispNode * pDispNode, BOOL fVisible);

    void            ExtractDispNodes(
                            CDispNode * pDispNodeStart = NULL,
                            CDispNode * pDispNodeEnd = NULL,
                            BOOL        fRestrictToLayer = TRUE);

    void            HandleTranslatedRange(const CSize & offset);

    void            SetPositionAware(BOOL fPositionAware = TRUE, CDispNode * pDispNode = NULL);
    void            SetInsertionAware(BOOL fInsertionAware = TRUE, CDispNode * pDispNode = NULL);

    void            SizeDispNode(CCalcInfo * pci, long cx, long cy, BOOL fInvalidateAll = FALSE)
                    {
                        SizeDispNode(pci, CSize(cx, cy), fInvalidateAll);
                    }
    void            SizeDispNode(CCalcInfo * pci, const SIZE & size, BOOL fInvalidateAll = FALSE);
    void            SizeDispNodeInsets(
                            styleVerticalAlign  va,
                            long                cy,
                            CDispNode *         pDispNode = NULL);

    void            SizeDispNodeUserClip(CDocInfo * pdci, const CSize & size, CDispNode * pDispNode = NULL);
    void            SizeContentDispNode(long cx, long cy, BOOL fInvalidateAll = FALSE)
                    {
                        SizeContentDispNode(CSize(cx, cy), fInvalidateAll);
                    }
    virtual void    SizeContentDispNode(const SIZE & size, BOOL fInvalidateAll = FALSE);

    void            TranslateDispNodes(
                            const SIZE &    size,
                            CDispNode *     pDispNodeStart = NULL,
                            CDispNode *     pDispNodeEnd = NULL,
                            BOOL            fRestrictToLayer = TRUE,
                            BOOL            fExtractHidden = FALSE);

    void            DestroyDispNode();

    HRESULT         HandleScrollbarMessage(CMessage * pMessage, CElement * pElement);

    BOOL            IsPositionValid();

protected:
    BOOL            DoFuzzyHitTest();
};


//+----------------------------------------------------------------------------
//
//  Global routines
//
//-----------------------------------------------------------------------------
CElement *  GetDispNodeElement(CDispNode * pDispNode);


//+----------------------------------------------------------------------------
//
//  Inlines
//
//-----------------------------------------------------------------------------
inline BOOL
CLayout::IsPositionValid()
{
    return _fPositionSet;
}

//
//  Inlines for managing the request queue
//
inline BOOL
CLayout::HasLookasidePtr(int iPtr)
{
    return(_fHasLookasidePtr & (1 << iPtr));
}

inline void *
CLayout::GetLookasidePtr(int iPtr)
{
#if DBG == 1
    if(HasLookasidePtr(iPtr))
    {
        void * pLookasidePtr =  Doc()->GetLookasidePtr((DWORD *)this + iPtr);

        Assert(pLookasidePtr == _apLookAside[iPtr]);

        return pLookasidePtr;
    }
    else
        return NULL;
#else
    return(HasLookasidePtr(iPtr) ? Doc()->GetLookasidePtr((DWORD *)this + iPtr) : NULL);
#endif
}

inline HRESULT
CLayout::SetLookasidePtr(int iPtr, void * pvVal)
{
    HRESULT hr = THR(Doc()->SetLookasidePtr((DWORD *)this + iPtr, pvVal));

    if (hr == S_OK)
    {
        _fHasLookasidePtr |= 1 << iPtr;
#if DBG == 1
        _apLookAside[iPtr] = pvVal;
#endif
    }

    RRETURN(hr);
}

inline void *
CLayout::DelLookasidePtr(int iPtr)
{
    if (HasLookasidePtr(iPtr))
    {
        void * pvVal = Doc()->DelLookasidePtr((DWORD *)this + iPtr);
        _fHasLookasidePtr &= ~(1 << iPtr);
#if DBG == 1
        _apLookAside[iPtr] = NULL;
#endif
        return(pvVal);
    }

    return(NULL);
}

inline BOOL
CLayout::CanHaveBgRecalcInfo()
{
    BOOL    fCanHaveBgRecalcInfo = !TestClassFlag(CElement::ELEMENTDESC_NOBKGRDRECALC);
    Assert(!fCanHaveBgRecalcInfo || Tag()!=ETAG_TD || !"Table Cells don't recalc in the background");
    return fCanHaveBgRecalcInfo;
}

inline CBgRecalcInfo *
CLayout::BgRecalcInfo()
{
    return HasBgRecalcInfo() ? (CBgRecalcInfo *)GetLookasidePtr(LOOKASIDE_BGRECALCINFO) : NULL;
}

inline HRESULT
CLayout::EnsureBgRecalcInfo()
{
    if (HasBgRecalcInfo())
        return S_OK;

    // Table cells don't get a CBgRecalcInfo and that's ok (S_FALSE).
    if (!CanHaveBgRecalcInfo())
        return S_FALSE;

    CBgRecalcInfo * pBgRecalcInfo = new CBgRecalcInfo;
    Assert(pBgRecalcInfo && "Failure to allocate CLayout::CBgRecalcInfo");

    if (pBgRecalcInfo)
    {
        IGNORE_HR( SetLookasidePtr(LOOKASIDE_BGRECALCINFO, pBgRecalcInfo) );
    }

    return pBgRecalcInfo ? S_OK : E_OUTOFMEMORY;
}

inline void
CLayout::DeleteBgRecalcInfo()
{
    Assert(HasBgRecalcInfo());

    CBgRecalcInfo * pBgRecalcInfo = (CBgRecalcInfo *)DelLookasidePtr(LOOKASIDE_BGRECALCINFO);

    Assert(pBgRecalcInfo);
    if (pBgRecalcInfo)
    {
        delete pBgRecalcInfo;
    }
}

inline BOOL
CLayout::IsAdorned()
{
    return _fAdorned;
}

inline BOOL
CLayout::IsCalcingSize ()
{
    return _fCalcingSize;
}

inline void
CLayout::SetCalcing( BOOL fState )
{
    _fCalcingSize = fState;
}
inline BOOL
CLayout::DoFuzzyHitTest()
{
    // BUGBUG (donmarsh) - we should also do fuzzy hit testing whenever the parent
    // layout is editable, but we don't have a cheap way to do that yet, and
    // this method is called during hit testing and cannot be too expensive.
    return Doc()->_fDesignMode;
}

#pragma INCMSG("--- End 'layout.hxx'")
#else
#pragma INCMSG("*** Dup 'layout.hxx'")
#endif
