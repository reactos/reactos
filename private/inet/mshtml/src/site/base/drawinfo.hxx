//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       drawinfo.hxx
//
//  Contents:   CFormDrawInfo
//
//----------------------------------------------------------------------------

#ifndef I_DRAWINFO_HXX_
#define I_DRAWINFO_HXX_
#pragma INCMSG("--- Beg 'drawinfo.hxx'")

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

class CDoc;
class CPrintDoc;
class CSite;
class CElement;
class CParentInfo;
class CCalcInfo;
class CSaveCalcInfo;
class CFormDrawInfo;
class CSetDrawSurface;
class COleSite;
class CLayout;
class CLSMeasurer;
class COleLayout;
class CView;
class CHeaderFooterInfo;
class CDispSurface;
struct IDirectDrawSurface;

//+---------------------------------------------------------------------------
//
//  Enumerations
//
//----------------------------------------------------------------------------

enum LAYOUT_FLAGS
{
    // Layout input flags

    LAYOUT_FORCE            = 0x00000001,   // Force layout
    LAYOUT_USEDEFAULT       = 0x00000002,   // Use default size (ignore user settings)
    LAYOUT_FORCEFIRSTCALC   = 0x00000004,   // Force first calc of the layout (used for delayed calc table cells)
    LAYOUT_NOBACKGROUND     = 0x00000008,   // Disallow initiating background layout

    // Layout output flags

    LAYOUT_THIS             = 0x00000010,   // Site has changed size
    LAYOUT_HRESIZE          = 0x00000020,   // Site has changed size horizontally
    LAYOUT_VRESIZE          = 0x00000040,   // Site has changed size vertically

    // Layout control flags (all used by CView::EnsureView)

    LAYOUT_SYNCHRONOUS      = 0x00000100,   // Synchronous call that overrides pending asynchronous call
    LAYOUT_SYNCHRONOUSPAINT = 0x00000200,   // Synchronously paint
    LAYOUT_INPAINT          = 0x00000400,   // Called from within WM_PAINT handling

    LAYOUT_DEFEREVENTS      = 0x00001000,   // Prevent deferred events
    LAYOUT_DEFERENDDEFER    = 0x00002000,   // Prevent EndDeferxxxx processing
    LAYOUT_DEFERINVAL       = 0x00004000,   // Prevent publication of pending invalidations
    LAYOUT_DEFERPAINT       = 0x00008000,   // Prevent synchronous updates (instead wait for the next WM_PAINT)

    // Task type flags

    LAYOUT_BACKGROUND       = 0x00010000,   // Task runs at background priority
    LAYOUT_PRIORITY         = 0x00020000,   // Task runs at high priority

    LAYOUT_MEASURE          = 0x00100000,   // Task needs to handle measuring
    LAYOUT_POSITION         = 0x00200000,   // Task needs to handle positioning
    LAYOUT_ADORNERS         = 0x00400000,   // Task needs to handle adorners
    LAYOUT_TASKDELETED      = 0x00800000,   // Task is deleted (transient state only)

    // Masks
    LAYOUT_NONTASKFLAGS     = 0x0000FFFF,   // Non-task flags mask
    LAYOUT_TASKFLAGS        = 0xFFFF0000,   // Task flags mask

    LAYOUT_MAX              = LONG_MAX
};

// NOTE: Only SIZEMODE_NATURAL requests are propagated through the tree, all others
//       handled directly by the receiving site
enum SIZEMODE
{
    SIZEMODE_NATURAL,       // Natural object size (based on user settings and parent/available size)
    SIZEMODE_SET,           // Set object size (override returned natural size)
    SIZEMODE_FULLSIZE,      // Return full object size (regardless of user settings/may differ from max)

    SIZEMODE_MMWIDTH,       // Minimum/Maximum object size
    SIZEMODE_MINWIDTH,      // Minimum object size

    SIZEMODE_PAGE,          // Object size for the current page

    SIZEMODE_MAX
};


//+---------------------------------------------------------------------------
//
//  Class:      CParentInfo
//
//  Purpose:    CTransform which contains a "parent" size
//
//----------------------------------------------------------------------------

class CParentInfo : public CDocInfo
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    SIZE        _sizeParent;        // Parent size

    CParentInfo()                              { Init(); };
    CParentInfo(const CDocInfo * pdci)         { Init(pdci); }
    CParentInfo(const CParentInfo * ppri)      { Init(ppri); }
    CParentInfo(const CCalcInfo * pci)         { Init(pci); }
    CParentInfo(const CTransform * ptransform) { CTransform::Init(ptransform); }
    CParentInfo(const CTransform & transform)  { CTransform::Init(&transform); }
    CParentInfo(CLayout * pLayout)             { Init(pLayout); }
    CParentInfo(SIZE * psizeParent)            { Init(psizeParent); }
    CParentInfo(RECT * prcParent)              { Init(prcParent); }

    void Init(const CDocInfo * pdci);
    void Init(const CParentInfo * ppri)
    {
        ::memcpy(this, ppri, sizeof(CParentInfo));
    }
    void Init(const CCalcInfo * pci);
    void Init(CLayout * pLayout);
    void Init(SIZE * psizeParent);
    void Init(RECT * prcParent)
    {
        SIZE    sizeParent = { prcParent->right - prcParent->left,
                               prcParent->bottom - prcParent->top };

        Init(&sizeParent);
    }

    void SizeToParent(long cx, long cy)
    {
        _sizeParent.cx = cx;
        _sizeParent.cy = cy;
    }
    void SizeToParent(SIZE * psizeParent)
    {
        Assert(psizeParent);
        _sizeParent = *psizeParent;
    }
    void SizeToParent(RECT * prcParent)
    {
        Assert(prcParent);
        _sizeParent.cx = prcParent->right - prcParent->left;
        _sizeParent.cy = prcParent->bottom - prcParent->top;
    }
    void SizeToParent(CLayout * pLayout);

protected:
    void Init();
};


//+---------------------------------------------------------------------------
//
//  Class:      CCalcInfo, CSaveCalcInfo
//
//  Purpose:    CTransform used for site size calculations
//
//----------------------------------------------------------------------------

class CCalcInfo : public CParentInfo
{
    friend  class CSaveCalcInfo;
    friend  class CTableCalcInfo;

private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    SIZEMODE    _smMode;            // Sizing mode
    DWORD       _grfLayout;         // Collection of LAYOUT_xxxx flags
    HDC         _hdc;               // Measuring HDC (may not be used for rendering)

    //
    // Values below this point are not saved/restored by CSaveCalcInfo
    //

    int         _yBaseLine;         // Baseline of measured site with SIZEMODE_NATURAL (returned)
    long        _xOffsetInLine;     // xoffset of an absolutely positioned site with width and left auto

    union
    {
        DWORD   _grfFlags;

        struct
        {
            DWORD   _fTableCalcInfo : 1;    // Is this a CTableCalcInfo?
            DWORD   _fUseOffset     : 1;    // Use _xOffsetInLine, when set
        };
    };

    CCalcInfo()                                         { Init(); };
    CCalcInfo(const CDocInfo * pdci, CLayout * pLayout) { Init(pdci, pLayout); }
    CCalcInfo(const CCalcInfo * pci)                    { Init(pci); }
    CCalcInfo(CLayout * pLayout, SIZE * psizeParent = NULL, HDC hdc = NULL)
                                                        { Init(pLayout, psizeParent, hdc); }
    CCalcInfo(CLayout * pLayout, RECT * prcParent, HDC hdc = NULL)
                                                        { Init(pLayout, prcParent, hdc); }

    void Init(const CDocInfo * pdci, CLayout * pLayout);
    void Init(const CCalcInfo * pci)
    {
        ::memcpy(this, pci, sizeof(CCalcInfo));
    }
    void Init(CLayout * pLayout, SIZE * psizeParent = NULL, HDC hdc = NULL);
    void Init(CLayout * pLayout, RECT * prcParent, HDC hdc = NULL)
    {
        SIZE    sizeParent = { prcParent->right - prcParent->left,
                               prcParent->bottom - prcParent->top };

        Init(pLayout, &sizeParent, hdc);
    }

    BOOL IsNaturalMode() const
    {
        return (    _smMode == SIZEMODE_NATURAL
                ||  _smMode == SIZEMODE_SET
                ||  _smMode == SIZEMODE_FULLSIZE);
    }

protected:
    void Init();
};

class CSaveCalcInfo : public CCalcInfo
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    CSaveCalcInfo(CCalcInfo * pci, CLayout * pLayout = NULL) { Save(pci, pLayout); }
    CSaveCalcInfo(CCalcInfo & ci, CLayout * pLayout = NULL)  { Save(&ci, pLayout); }

    ~CSaveCalcInfo()    { Restore(); }

private:
    CCalcInfo * _pci;
    CLayout *   _pLayout;

    void Save(CCalcInfo * pci, CLayout * pLayout)
    {
        _pci     = pci;
        _pLayout = pLayout;

        ::memcpy(this, pci, offsetof(CCalcInfo, _yBaseLine));
        _yBaseLine = 0;
    }
    void Restore()
    {
        ::memcpy(_pci, this, offsetof(CCalcInfo, _yBaseLine));
    }
};


//+---------------------------------------------------------------------------
//
//  Class:      CFormDrawInfo, CSetDrawSurface
//
//  Purpose:    CTransform used for rendering
//
//----------------------------------------------------------------------------

class CFormDrawInfo : public CDrawInfo
{
    friend class CDoc;
    friend class CView;
    friend class CSetDrawSurface;

private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:

    void    Init(CLayout *pLayout, HDC hdc = NULL, RECT *prcClip = NULL);
    void    Init(CElement *pElement, HDC hdc = NULL, RECT *prcClip = NULL);
    void    Init(HDC hdc = NULL, RECT *prcClip = NULL);
    void    InitToSite(CLayout * CLayout, RECT * prcClip = NULL);

    HRESULT FixupForPrint( class CPrintDoc * pPrintDoc );
    BOOL    IsMetafile() { return _fIsMetafile; }
    BOOL    IsMemory()   { return _fIsMemoryDC; }
    BOOL    Printing()          { return _ptd != NULL; }
    DWORD   DrawImageFlags();

    HDC     GetDC(BOOL fPhysicallyClip = FALSE);
    HDC     GetGlobalDC(BOOL fPhysicallyClip = FALSE);
    HRESULT GetDirectDrawSurface(IDirectDrawSurface ** ppSurface, SIZE * pOffset);
    HRESULT GetSurface(const IID & iid, void ** ppv, SIZE * pOffset);

    void    SetDeviceCoordinateMode();
    void    TransformToDeviceCoordinates(POINT* ppt) const
                    {((CPoint&)*ppt) += _sizeDeviceOffset;}
    void    TransformToDeviceCoordinates(RECT* prc) const
                    {((CRect*)prc)->OffsetRect(_sizeDeviceOffset);}
    
    RECT *  Rect()              { return &_rc; }
    RECT *  ClipRect()          { return &_rcClip; }

    CDispSurface *  _pSurface;          // Display tree "surface" object (valid only if non-NULL)
    CRect           _rc;                // Current bounding rectangle
    CRect           _rcClip;            // Current logical clipping rectangle
    CRect           _rcClipSet;         // Current physical clipping rectangle
    DVASPECTINFO    _dvai;              // Aspect info set to optimize HDC handling
    HRGN            _hrgnClip;          // Initial clip region.  Can be null.
    HRGN            _hrgnPaint;         // Region for paint test.  Can be null.
    unsigned        _fDeviceCoords:1;   // True if _rc & _rcClip are in device coordinates.
    CSize           _sizeDeviceOffset;  // Offset to device coordinates.
};

class CSetDrawSurface
{
public:
    CSetDrawSurface(CFormDrawInfo * pDI, const RECT * prcBounds, const RECT * prcRedraw, CDispSurface * pSurface);
    ~CSetDrawSurface()
    {
        _pDI->_hdc      = _hdc;
        _pDI->_pSurface = _pSurface;
    }

private:
    CFormDrawInfo * _pDI;
    HDC             _hdc;
    CDispSurface *  _pSurface;
};

#pragma INCMSG("--- End 'drawinfo.hxx'")
#else
#pragma INCMSG("*** Dup 'drawinfo.hxx'")
#endif
