//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       paint.cxx
//
//  Contents:   Painting and invalidation
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_COLOR3D_HXX_
#define X_COLOR3D_HXX_
#include "color3d.hxx"
#endif

#ifndef X_HEADFOOT_HXX_
#define X_HEADFOOT_HXX_
#include "headfoot.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_OCMM_H_
#define X_OCMM_H_
#include "ocmm.h"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_PRINTWRP_HXX_
#define X_PRINTWRP_HXX_
#include "printwrp.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_TIMER_HXX_
#define X_TIMER_HXX_
#include "timer.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_UPDSINK_HXX_
#define X_UPDSINK_HXX_
#include "updsink.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifdef WIN16
#ifndef X_PRINT_H_
#define X_PRINT_H_
#include <print.h>
#endif
#ifndef UNICODE
#define iswspace isspace
#endif
#endif // WIN16

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

DeclareTagOther(tagPaintShow,    "DocPaintShow",    "erase bkgnd before paint")
DeclareTagOther(tagPaintPause,   "DocPaintPause",   "pause briefly before paint")
DeclareTagOther(tagPaintWait,    "DocPaintWait",    "wait for shift key before paint")
DeclareTagOther(tagInvalShow,    "DocInvalShow",    "paint hatched brush over invalidated areas")
DeclareTagOther(tagInvalWait,    "DocInvalWait",    "wait after invalidating")
DeclareTagOther(tagHackGDICoords,"DocHackGDICoords","simulate Win95 GDI coordinate limitation")
PerfDbgTag(tagDocPaint,          "DocPaint",        "painting")
DeclareTag(tagFormInval,         "DocInval",        "invalidation")
DeclareTag(tagFormInvalT,        "DocInvalStack",   "invalidation stack trace")
DeclareTag(tagPrintClearBkgrnd,  "Print",           "No parent site background clearing")
PerfDbgTag(tagNoGdiBatch,        "DocBatch",        "disable GDI batching")
PerfDbgTag(tagNoOffScr,          "DocOffScreen",    "disable off-screen rendering")
PerfDbgTag(tagNoTile,            "DocTile",         "disable tiling")
DeclareTagOther(tagForceClip,    "DocForceClip",    "force physical clipping always");
DeclareTagOther(tagUpdateInt,    "DocUpdateInt",    "trace UpdateInterval stuff");
DeclareTag(tagTile,              "DocTile",         "tiling information");
DeclareTag(tagDisEraseBkgnd,     "Erase Background: disable", "Disable erase background")
MtDefine(CSiteDrawList, Locals, "CSiteDrawList")
MtDefine(CSiteDrawList_pv, CSiteDrawList, "CSiteDrawList::_pv")
MtDefine(CSiteDrawSiteList_aryElements_pv, Locals, "CSite::DrawSiteList aryElements::_pv")
MtDefine(CSiteGetSiteDrawList_aryElements_pv, Locals, "CSite::GetSiteDrawList aryElements::_pv")

ExternTag(tagPalette);
ExternTag(tagTimePaint);
ExternTag(tagView);

#ifdef PRODUCT_PROF_FERG
// for PROFILING perposes only
extern "C" void _stdcall ResumeCAP(void);
extern "C" void _stdcall SuspendCAP(void);
#endif

//+------------------------------------------------------------------------
//
//  Function:   DumpRgn
//
//  Synopsis:   Write region to debug output
//
//-------------------------------------------------------------------------

#if DBG==1
void
DumpRgn(HRGN hrgn)
{
    struct
    {
        RGNDATAHEADER rdh;
        RECT arc[128];
    } data;

    if (GetRegionData(hrgn, ARRAY_SIZE(data.arc), (RGNDATA *)&data) != 1)
    {
        TraceTag((0, "HRGN=%08x: buffer too small", hrgn));
    }
    else
    {
        TraceTag((0, "HRGN=%08x, iType=%d, nCount=%d, nRgnSize=%d, t=%d b=%d l=%d r=%d",
                hrgn,
                data.rdh.iType,
                data.rdh.nCount,
                data.rdh.nRgnSize,
                data.rdh.rcBound.top,
                data.rdh.rcBound.bottom,
                data.rdh.rcBound.left,
                data.rdh.rcBound.right));
        for (DWORD i = 0; i < data.rdh.nCount; i++)
        {
            TraceTag((0, "    t=%d, b=%d, l=%d, r=%d",
                data.arc[i].top,
                data.arc[i].left,
                data.arc[i].bottom,
                data.arc[i].right));
        }
    }
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     CDoc::UpdateForm
//
//  Synopsis:   Update the form's window
//
//-------------------------------------------------------------------------

void
CDoc::UpdateForm()
{
    if (_pInPlace)
    {
        // CHROME
        // If we are Chrome hosted and windowless then we can't
        // UpdateChildTree to update the form. Instead we must
        // ask the windowless site to invalidate us.
        if (!IsChromeHosted())
        {
            UpdateChildTree(_pInPlace->_hwnd);
        }
        else
        {
            Assert(_pInPlace->_fWindowlessInplace);
            ((IOleInPlaceSiteWindowless *)(_pInPlace->_pInPlaceSite))->InvalidateRect(NULL, FALSE);
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::Invalidate
//
//  Synopsis:   Invalidate an area in the form.
//
//              We keep track of whether the background should be
//              erased on paint privately.  If we let Windows keep
//              track of this, then we can get flashing because
//              the WM_ERASEBKGND message can be delivered far in
//              advance of the WM_PAINT message.
//
//              Invalidation flags:
//
//              INVAL_CHILDWINDOWS
//                  Invaildate child windows. Causes the RDW_ALLCHILDREN
//                  flag to be passed to RedrawWindow.
//
//  Arguments:  prc         The physical rectangle to invalidate.
//              prcClip     Clip invalidation against this rectangle.
//              hrgn        ...or, the region to invalidate.
//              dwFlags     See description above.
//
//-------------------------------------------------------------------------

void
CDoc::Invalidate(const RECT *prc, const RECT *prcClip, HRGN hrgn, DWORD dwFlags)
{
    UINT uFlags;
    RECT rc;
    
    if (_state >= OS_INPLACE)
    {
        if (prcClip)
        {
            Assert(prc);
            if (!IntersectRect(&rc, prc, prcClip))
                return;

            prc = &rc;
        }

        // Do not invalidate when not yet INTERACTIVE.
        
        // INTERACTIVE is when we want to start doing our own drawing. It means we've had
        // to process an externally triggered WM_PAINT, or we've loaded enough of the
        // document to draw the initial scroll position correctly, or five seconds
        // have passed since we've tried to do the initial scroll position.

        if (LoadStatus() < LOADSTATUS_INTERACTIVE)
        {
            _fInvalNoninteractive = TRUE; // When we become interactive, we must inval
            return;
        }
        
#if DBG==1 && !defined(WINCE)
        if (prc)
        {
            TraceTag((tagFormInval,
                    "%08lX Inval%s l=%ld, t=%ld, r=%ld, b=%ld",
                    this,
                    dwFlags & INVAL_CHILDWINDOWS ? " CHILD" : "",
                    prc->left, prc->top, prc->right, prc->bottom));
        }
        else
        {
            TraceTag((tagFormInval, "%08lX Inval%s",
                    this,
                    dwFlags & INVAL_CHILDWINDOWS ? " CHILD" : ""));
        }
        TraceCallers(tagFormInvalT, 2, 4);

        if (IsTagEnabled(tagInvalShow))
        {
            static int s_iclr;
            static COLORREF s_aclr[] =
            {
                    RGB(255, 0, 0), RGB(0, 255, 0),
                    RGB(255, 255, 0), RGB(0, 255, 255),
            };

            s_iclr = (s_iclr + 1) % ARRAY_SIZE(s_aclr);
            HDC hdc = ::GetDC(_pInPlace->_hwnd);
            HBRUSH hbrush = CreateHatchBrush(HS_DIAGCROSS, s_aclr[s_iclr]);
            int bkMode = SetBkMode(hdc, TRANSPARENT);
            if (prc)
            {
                FillRect(hdc, prc, hbrush);
            }
            else if (hrgn)
            {
                FillRgn(hdc, hrgn, hbrush);
            }
            DeleteObject((HGDIOBJ)hbrush);
            SetBkMode(hdc, bkMode);
            ::ReleaseDC(_pInPlace->_hwnd, hdc);
        }
        if (IsTagEnabled(tagInvalWait))
            Sleep(120);
#endif // DBG==1 && !defined(WINCE)

        uFlags = RDW_INVALIDATE | RDW_NOERASE;

        if (dwFlags & INVAL_CHILDWINDOWS)
        {
            uFlags |= RDW_ALLCHILDREN;
        }

        Assert(_pInPlace && " about to crash");
        // CHROME
        _cInval++;

        if ( !_pUpdateIntSink )
        {
            // CHROME
            if ( !IsChromeHosted() )
            {
                RedrawWindow(_pInPlace->_hwnd, prc, hrgn, uFlags);
            }
            else
            {
                // If Chrome hosted and windowless then RedrawWindow
                // cannot be used as there is no valid HWND. If this
                // is the case then use the windowless site's
                // InvalidateRect member.
                Assert(_pInPlace->_fWindowlessInplace);
                ((IOleInPlaceSiteWindowless *)(_pInPlace->_pInPlaceSite))->InvalidateRect( prc, FALSE );
            }
        }
        else if ( _pUpdateIntSink->_state < UPDATEINTERVAL_ALL )
        {
            // accumulate invalid rgns until the updateInterval timer fires
            HRGN    hrgnSrc;
            TraceTag((tagUpdateInt, "Accumulating invalid Rect/Rgn"));
            if ( prc )
            {
                hrgnSrc = CreateRectRgnIndirect( prc );
                if ( !hrgnSrc )
                {
                    // CHROME
                    if ( !IsChromeHosted() )
                        RedrawWindow(_pInPlace->_hwnd, prc, hrgn, uFlags);
                    else
                        InvalidateRect( prc, FALSE );
                }
                else if ( UPDATEINTERVAL_EMPTY == _pUpdateIntSink->_state )
                {
                    _pUpdateIntSink->_hrgn = hrgnSrc;
                    _pUpdateIntSink->_state = UPDATEINTERVAL_REGION;
                    _pUpdateIntSink->_dwFlags |= uFlags;
                }
                else
                {
                    Assert( UPDATEINTERVAL_REGION == _pUpdateIntSink->_state );
                    if ( ERROR == CombineRgn(_pUpdateIntSink->_hrgn,
                                             _pUpdateIntSink->_hrgn,
                                             hrgnSrc, RGN_OR) )
                    {
                        TraceTag((tagUpdateInt, "Error in accumulating invalid Rect"));
                        // CHROME
                        if ( !IsChromeHosted() )
                            RedrawWindow(_pInPlace->_hwnd, prc, hrgn, uFlags);
                        else
                            InvalidateRect( prc, FALSE );
                    }
                    else
                    {
                        _pUpdateIntSink->_dwFlags |= uFlags;
                    }
                    DeleteObject( hrgnSrc );
                }
            }
            else if ( hrgn )
            {
                if ( UPDATEINTERVAL_EMPTY == _pUpdateIntSink->_state )
                {
                    _pUpdateIntSink->_hrgn = hrgn;
                    _pUpdateIntSink->_state = UPDATEINTERVAL_REGION;
                    _pUpdateIntSink->_dwFlags |= uFlags;
                }
                else
                {
                    Assert( UPDATEINTERVAL_REGION == _pUpdateIntSink->_state );
                    if ( ERROR == CombineRgn(_pUpdateIntSink->_hrgn,
                                             _pUpdateIntSink->_hrgn,
                                             hrgn, RGN_OR) )
                    {
                        TraceTag((tagUpdateInt, "Error in accumulating invalid Rgn"));
                        // CHROME
                        if ( !IsChromeHosted() )
                            RedrawWindow(_pInPlace->_hwnd, prc, hrgn, uFlags);
                        else
                            InvalidateRgn( hrgn, FALSE );
                    }
                    else
                    {
                        _pUpdateIntSink->_dwFlags |= uFlags;
                    }
                }
            }
            else
            {
                // update entire client area, no need to accumulate anymore
                _pUpdateIntSink->_state = UPDATEINTERVAL_ALL;
                DeleteObject( _pUpdateIntSink->_hrgn );
                _pUpdateIntSink->_hrgn = NULL;
                _pUpdateIntSink->_dwFlags |= uFlags;
            }
        }

        OnViewChange(DVASPECT_CONTENT);
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::Invalidate
//
//  Synopsis:   Invalidate the form's entire area.
//
//-------------------------------------------------------------------------

void
CDoc::Invalidate()
{
    Invalidate(NULL, NULL, NULL, 0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DisabledTilePaint()
//
//  Synopsis:   return true if tiled painting disabled
//
//----------------------------------------------------------------------------

inline BOOL
CDoc::TiledPaintDisabled()
{
#if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagNoTile))
        return TRUE;
#endif

    return _fDisableTiledPaint;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnPaint
//
//  Synopsis:   Handle WM_PAINT
//
//----------------------------------------------------------------------------

#ifndef NO_RANDOMRGN
typedef int WINAPI GETRANDOMRGN(HDC, HRGN, int);
static GETRANDOMRGN * s_pfnGetRandomRgn;
static BOOL s_fGetRandomRgnFetched = FALSE;
#endif

void
CDoc::OnPaint()
{
    IF_WIN16(RECT rc;)
    PAINTSTRUCT         ps;
    CFormDrawInfo       DI;
    BOOL                fViewIsReady;
    BOOL                fHtPalette;
#ifndef NO_RANDOMRGN
    HRGN                hrgn = NULL;
    POINT               ptBefore;
    POINT               ptAfter;
#endif

    TraceTagEx((tagView, TAG_NONAME,
           "View : CDoc::OnPaint"));

    _fInvalInScript = FALSE;

    DI._hdc = NULL;

    // Don't allow OnPaint to recurse.  This can occur as a result of the
    // call to RedrawWindow in a random control.

    // BUGBUG: StylesheetDownload should be linked to the LOADSTATUS interactive?
    if (TestLock(SERVERLOCK_BLOCKPAINT) || IsStylesheetDownload())
    {
        // We get endless paint messages when we try to popup a messagebox...
        // and prevented the messagebox from getting paint !
        BeginPaint(_pInPlace->_hwnd, &ps);
        EndPaint(_pInPlace->_hwnd, &ps);

        // Post a delayed paint to make up for what was missed
        // Post a delayed paint to make up for what was missed
        _view.Invalidate(&ps.rcPaint, FALSE, FALSE, FALSE);

        TraceTagEx((tagView, TAG_NONAME,
               "View : CDoc::OnPaint - Exit"));

        return;
    }

    // If we're not interactive by the time we get the first paint, we should be
    if (PrimaryMarkup()->LoadStatus() < LOADSTATUS_INTERACTIVE)
    {
        PrimaryMarkup()->OnLoadStatus(LOADSTATUS_INTERACTIVE);
    }

    PerfDbgLog(tagDocPaint, this, "+CDoc::OnPaint");
    CDebugPaint debugPaint;
    debugPaint.StartTimer();

    fViewIsReady = _view.EnsureView(LAYOUT_SYNCHRONOUS | LAYOUT_INPAINT | LAYOUT_DEFERPAINT);
    Assert( !fViewIsReady
        ||  !_view.IsDisplayTreeOpen());

    CLock Lock(this, SERVERLOCK_BLOCKPAINT);

    SwitchesBegTimer(SWITCHES_TIMER_PAINT);

    // since we're painting, paint any accumulated regions if any
    if ( _pUpdateIntSink && _pUpdateIntSink->_state != UPDATEINTERVAL_EMPTY )
    {
        ::InvalidateRgn( _pInPlace->_hwnd, _pUpdateIntSink->_hrgn, FALSE );
        if ( _pUpdateIntSink->_hrgn )
        {
            DeleteObject( _pUpdateIntSink->_hrgn );
            _pUpdateIntSink->_hrgn = NULL;
            _pUpdateIntSink->_state = UPDATEINTERVAL_EMPTY;
            _pUpdateIntSink->_dwFlags = 0;
        }
    }

#ifndef NO_RANDOMRGN
    ptBefore.x = ptBefore.y = 0;
    MapWindowPoints(_pInPlace->_hwnd, NULL, &ptBefore, 1);
#endif

    // Setup DC for painting.

    BeginPaint(_pInPlace->_hwnd, &ps);

    if (IsRectEmpty(&ps.rcPaint))
    {
        // It appears there are cases when our window is obscured yet
        // internal invalidations still trigger WM_PAINT with a valid
        // update region but the PAINTSTRUCT RECT is empty.
        goto Cleanup;
    }

    // If the view could not be properly prepared, accumulate the invalid rectangle and return
    // (The view will issue the invalidation/paint once it is safe to do so)
    if (!fViewIsReady)
    {
        _view.Invalidate(&ps.rcPaint);
        _view.SetFlag(CView::VF_FORCEPAINT);

        TraceTagEx((tagView, TAG_NONAME,
               "View : CDoc::OnPaint - !fViewIsReady, Setting VF_FORCEPAINT for rc(%d, %d, %d, %d)",
               ps.rcPaint.left,
               ps.rcPaint.top,
               ps.rcPaint.right,
               ps.rcPaint.bottom));

        goto Cleanup;
    }

#ifndef NO_RANDOMRGN
#ifdef WIN32
    if (!s_fGetRandomRgnFetched)
    {
        s_pfnGetRandomRgn = (GETRANDOMRGN *)GetProcAddress(GetModuleHandleA("GDI32.DLL"), "GetRandomRgn");
        s_fGetRandomRgnFetched = TRUE;
    }

    if (s_pfnGetRandomRgn)
    {
        Verify((hrgn = CreateRectRgnIndirect(&g_Zero.rc)) != NULL);
        Verify(s_pfnGetRandomRgn(ps.hdc, hrgn, 4) != ERROR);
        if (g_dwPlatformID == VER_PLATFORM_WIN32_NT)
        {
            Verify(OffsetRgn(hrgn, -ptBefore.x, -ptBefore.y) != ERROR);
        }

        // Don't trust the region if the window moved in the meantime

        ptAfter.x = ptAfter.y = 0;
        MapWindowPoints(_pInPlace->_hwnd, NULL, &ptAfter, 1);

        if (ptBefore.x != ptAfter.x || ptBefore.y != ptAfter.y)
        {
            Verify(DeleteObject(hrgn));
            hrgn = NULL;
            goto Cleanup;
        }
    }
#endif
#endif

    GetPalette(ps.hdc, &fHtPalette);

#ifndef NO_PERFDBG
#if DBG==1
    // If it looks like we are the foreground application, check to see
    // if we have an identify palette.  Turn on "warn if not identity palette"
    // trace tag to see the output.

    if (_pElemUIActive == PrimaryRoot() &&
        _pInPlace->_fFrameActive)
    {
        extern BOOL IsSameAsPhysicalPalette(HPALETTE);
        if (!IsSameAsPhysicalPalette(GetPalette()))
            TraceTag((tagError, "Logical palette does not match physical palette"));
    }
#endif
#endif

#if DBG==1
    if (!CDebugPaint::UseDisplayTree() &&
        (IsTagEnabled(tagPaintShow) || IsTagEnabled(tagPaintPause)))
    {
        // Flash the background.

        HBRUSH hbr;
        static int s_iclr;
        static COLORREF s_aclr[] =
        {
                RGB(  0,   0, 255),
                RGB(  0, 255,   0),
                RGB(  0, 255, 255),
                RGB(255,   0,   0),
                RGB(255,   0, 255),
                RGB(255, 255,   0)
        };

        GetAsyncKeyState(VK_SHIFT);

        do
        {
            // Fill the rect and pause.

            if (IsTagEnabled(tagPaintShow))
            {
                s_iclr = (s_iclr + 1) % ARRAY_SIZE(s_aclr);
                hbr = GetCachedBrush(s_aclr[s_iclr]);
                FillRect(ps.hdc, &ps.rcPaint, hbr);
                ReleaseCachedBrush(hbr);
                GdiFlush();
            }

            if (IsTagEnabled(tagPaintPause))
            {
                DWORD dwTick = GetTickCount();
                while (GetTickCount() - dwTick < 100) ;
            }
        }
        while (GetAsyncKeyState(VK_SHIFT) & 0x8000);
    }
#endif // DBG==1

#if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagNoGdiBatch))
        GdiSetBatchLimit(1);
#endif // DBG==1 || defined(PERFTAGS)

    if (_pTimerDraw)
        _pTimerDraw->Freeze(TRUE);      // time stops so controls can synchronize
#if !defined(WIN16) && !defined(NO_RANDOMRGN)
    if (!TiledPaintDisabled())
    {
#endif
        // Invalidation was not on behalf of an ActiveX control.  Paint the
        // document in one pass here and tile if required in CSite::DrawOffscreen.

        DI.Init(GetPrimaryElementTop(), ps.hdc);
        DI._rcClip = ps.rcPaint;
        DI._rcClipSet = ps.rcPaint;
#ifndef NO_RANDOMRGN
        DI._hrgnPaint = hrgn;
#else
        DI._hrgnPaint = NULL;
#endif
        DI._fHtPalette = fHtPalette;

        Assert(!_view.IsDisplayTreeOpen());

#ifndef NO_RANDOMRGN
        _view.RenderView(&DI, DI._hrgnPaint);
#else
        _view.RenderView(&DI, &ps.rcPaint);
#endif
        WHEN_DBG(debugPaint.StopTimer(tagTimePaint, "Display Tree Paint", TRUE));

#if !defined(WIN16) && !defined(NO_RANDOMRGN)
    }
    else
    {
        RECT *   prc;
        int      c;
        struct REGION_DATA
        {
            RGNDATAHEADER rdh;
            RECT          arc[MAX_INVAL_RECTS];
        } rd;

        // Invalidation was on behalf of an ActiveX Control.  We chunk things
        // up here based on the inval region with the hope that the ActiveX
        // Control will be painted in a single pass.  We do this because
        // some ActiveX Controls (controls using Direct Animation are an example)
        // have very bad performance when painted in tiles.

        // if we have more than one invalid rectangle, see if we can combine
        // them so drawing will be more efficient. Windows chops up invalid
        // regions in a funny, but predicable, way to maintain their ordered
        // listing of rectangles. Also, some times it is more efficient to
        // paint a little extra area than to traverse the hierarchy multiple times.

        if (hrgn &&
            GetRegionData(hrgn, sizeof(rd), (RGNDATA *)&rd) &&
            rd.rdh.iType == RDH_RECTANGLES &&
            rd.rdh.nCount <= MAX_INVAL_RECTS)
        {
            c = rd.rdh.nCount;
            prc = rd.arc;

            CombineRectsAggressive(&c, prc);
        }
        else
        {
            c = 1;
            prc = &ps.rcPaint;
        }

        // Paint each rectangle.

        for (; --c >= 0; prc++)
        {
            DI.Init(GetPrimaryElementTop());
            DI._hdc = ps.hdc;
            DI._hic = ps.hdc;
            DI._rcClip = *prc;
            DI._rcClipSet = *prc;
            DI._fHtPalette = fHtPalette;

            if (prc != &ps.rcPaint)
            {
                // If painting the update region in more than one
                // pass and painting directly to the screen, then
                // we explicitly set the clip rectangle to insure correct
                // painting.  If we don't do this, the FillRect for the
                // background of a later pass will clobber the foreground
                // for an earlier pass.

                Assert(DI._hdc == ps.hdc);
                IntersectClipRect(DI._hdc,
                        DI._rcClip.left, DI._rcClip.top,
                        DI._rcClip.right, DI._rcClip.bottom);

            }

            PerfDbgLog6(tagDocPaint, this,
                    "CDoc::OnPaint Draw(i=%d %s l=%ld, t=%ld, r=%ld, b=%ld)",
                    c,
                    DI._hdc != ps.hdc ? "OFF" : "ON",
                    DI._rcClip.left, DI._rcClip.top,
                    DI._rcClip.right, DI._rcClip.bottom);

            _view.RenderView(&DI, &DI._rcClip);

            PerfDbgLog(tagDocPaint, this, "-CDoc::OnPaint Draw");

            if (c != 0)
            {
                // Restore the clip region set above.
                SelectClipRgn(DI._hdc, NULL);
            }
        }
    }
#endif

    if (_pTimerDraw)
        _pTimerDraw->Freeze(FALSE);

Cleanup:
    if (DI._hdc)
        SelectPalette(DI._hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);
    EndPaint(_pInPlace->_hwnd, &ps);
    _fDisableTiledPaint = FALSE;

#ifndef NO_RANDOMRGN
    // Find out if the window moved during paint

    ptAfter.x = ptAfter.y = 0;
    MapWindowPoints(_pInPlace->_hwnd, NULL, &ptAfter, 1);

    if (ptBefore.x != ptAfter.x || ptBefore.y != ptAfter.y)
    {
        TraceTag((tagDocPaint, "CDoc::OnPaint (Window moved during paint!)"));
        Invalidate(hrgn ? NULL : &ps.rcPaint, NULL, hrgn, 0);
    }

    if (hrgn)
    {
        Verify(DeleteObject(hrgn));
    }
#endif

    SwitchesEndTimer(SWITCHES_TIMER_PAINT);

    PerfDbgLog(tagDocPaint, this, "-CDoc::OnPaint");

    TraceTagEx((tagView, TAG_NONAME,
           "View : CDoc::OnPaint - Exit"));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnEraseBkgnd
//
//  Synopsis:   Handle WM_ERASEBKGND
//
//----------------------------------------------------------------------------

BOOL
CDoc::OnEraseBkgnd(HDC hdc)
{
    CFormDrawInfo         DI;
    BOOL                  fHtPalette;

    if (TestLock(SERVERLOCK_BLOCKPAINT))
        return FALSE;

#if DBG==1
    if(IsTagEnabled(tagDisEraseBkgnd))
        return TRUE;
#endif

#if !defined(WIN16) && !defined(WINCE)
    // Ignore unnecessary erase requests
    // (e.g., those generated by SetMenu, EndDeferWindowPos)
    if (TestLock(SERVERLOCK_IGNOREERASEBKGND) &&
        WindowFromDC(hdc) == InPlace()->_hwnd)
        return TRUE;
#else
    if (TestLock(SERVERLOCK_IGNOREERASEBKGND))
        return TRUE;
#endif

    // Framesets leave the background to the embedded frames.

    if (_fFrameSet)
        return TRUE;

    SwitchesBegTimer(SWITCHES_TIMER_PAINT);

    PerfDbgLog(tagDocPaint, this, "+CDoc::OnEraseBkgnd");

    CLock Lock(this, SERVERLOCK_BLOCKPAINT);

    GetPalette(hdc, &fHtPalette);
    DI.Init(GetPrimaryElementTop());
    GetClipBox(hdc, &DI._rcClip);
    DI._rcClipSet = DI._rcClip;
    DI._hdc = hdc;
    DI._hic = hdc;
    DI._fInplacePaint = TRUE;
    DI._fHtPalette = fHtPalette;

    PerfDbgLog1(tagDocPaint, this, "CDoc::OnEraseBkgnd l=%ld, t=%ld, r=%ld, b=%ld", *DI.ClipRect());

#if DBG==1

    if (IsTagEnabled(tagPaintShow) || IsTagEnabled(tagPaintPause))
    {
        HBRUSH hbr;

        // Fill the rect with blue and pause.
        hbr = GetCachedBrush(RGB(0,0,255));
        FillRect(hdc, DI.ClipRect(), hbr);
        ReleaseCachedBrush(hbr);
        GdiFlush();
        Sleep(IsTagEnabled(tagPaintPause) ? 160 : 20);
    }
#endif

#ifdef WIN16
        // when OE 16 uses us the option settings are not read immediately,
        // so we check here.
        if ( _pOptionSettings == NULL )
        {
                if ( UpdateFromRegistry(0) )
                        return TRUE;
        }
#endif

    _view.EraseBackground(&DI, &DI._rcClip);
    
    SwitchesEndTimer(SWITCHES_TIMER_PAINT);

    PerfDbgLog(tagDocPaint, this, "-CDoc::OnEraseBkgnd");

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Draw, CServer
//
//  Synopsis:   Draw the form.
//              Called from CServer implementation if IViewObject::Draw
//
//----------------------------------------------------------------------------

HRESULT
CDoc::Draw(CDrawInfo * pDI, RECT *prc)
{
    CServer::CLock Lock(this, SERVERLOCK_IGNOREERASEBKGND);

    CSaveTransform  st(_dci);
    CFormDrawInfo   DI;
    int             r;
    CSize           sizeOld;
    CSize           sizeNew;
    CPoint          ptOrgOld;
    CPoint          ptOrg;
    HRESULT         hr = S_OK;
    BOOL            fHadView = !!_view.IsActive();

    // CLayout *       pLayoutRoot = PrimaryRoot()->GetLayout();

    if (DVASPECT_CONTENT  != pDI->_dwDrawAspect &&
        DVASPECT_DOCPRINT != pDI->_dwDrawAspect)
    {
        RRETURN(DV_E_DVASPECT);
    }

    //
    // Do not display if we haven't received all the stylesheets
    //

    if (IsStylesheetDownload())
        return hr;

    //
    // Setup drawing info.
    //

    DI.Init(GetPrimaryElementTop());

    //
    // Copy the CDrawInfo information into CFormDrawInfo
    //

    *(CDrawInfo*)&DI = *pDI;

    ::SetViewportOrgEx(DI._hdc, 0, 0, &ptOrg);

    ((CRect *)prc)->OffsetRect(ptOrg.AsSize());

    DI._pDoc     = this;
    DI._hrgnClip = CreateRectRgnIndirect(prc);

    r = GetClipRgn(DI._hdc, DI._hrgnClip);
    if (r == -1)
    {
        DeleteObject(DI._hrgnClip);
        DI._hrgnClip = NULL;
    }

#ifdef _MAC
    DI._rcClipSet = (*prc);
#else
    r = GetClipBox(DI._hdc, &DI._rcClip);
    if (r == 0)
    {
        // No clip box, assume very large clip box to start.
        DI._rcClip.left   =
        DI._rcClip.top    = SHRT_MIN;
        DI._rcClip.right  =
        DI._rcClip.bottom = SHRT_MAX;
    }
    DI._rcClipSet = DI._rcClip;
#endif

    DI.CTransform::Init(prc, _sizel);

    //
    // We are in print preview mode if DI._hic is
    // null and DI._ptd is not null.  Setup a second draw info
    // with information about the printer.
    //

    if (!DI._hic)
    {
        if (!DI.Printing())
        {
            DI._hic = DI.IsMetafile() ? TLS(hdcDesktop) : DI._hdc;
        }
        else
        {
            DI._hic =  CreateIC(
                (LPCTSTR) ((char *)DI._ptd + DI._ptd->tdDriverNameOffset),
                (LPCTSTR) ((char *)DI._ptd + DI._ptd->tdDeviceNameOffset),
                (LPCTSTR) ((char *)DI._ptd + DI._ptd->tdPortNameOffset),
                NULL );

            if (NULL == DI._hic)
            {
                // Couldn't create it
                hr = E_FAIL;
                goto Cleanup;
            }

            DI._sizeInch.cx = GetDeviceCaps(DI._hic, LOGPIXELSX);
            DI._sizeInch.cy = GetDeviceCaps(DI._hic, LOGPIXELSY);

            // BUGBUG (garybu) Need to set these sizes up to match the printer.
            // BUGBUG (cthrash) this isn't correct.

            DI._sizeSrc.cx = 2540;
            DI._sizeSrc.cy = 2540;
            DI._sizeDst.cx = DI._sizeInch.cx;
            DI._sizeDst.cy = DI._sizeInch.cy;

#if DBG==1
            RECT rc;

            rc.left = 0;
            rc.top = 0;
#ifndef WIN16
            rc.right = GetDeviceCaps( DI._hic, PHYSICALWIDTH );
            rc.bottom = GetDeviceCaps( DI._hic, PHYSICALHEIGHT );
#else
            rc.right = GetDeviceCaps( DI._hic, HORZRES );
            rc.bottom = GetDeviceCaps( DI._hic, VERTRES );
#endif // !WIN16

#endif  // DBG==1
        }
    }

    DI._sizeInch.cx = GetDeviceCaps(DI._hic, LOGPIXELSX);
    DI._sizeInch.cy = GetDeviceCaps(DI._hic, LOGPIXELSY);

#ifdef  IE5_ZOOM

    // REVIEW sidda:    hack to force scaling from display pixels to printer pixels when printing
    DI.SetScaleFraction(1, 1, 1);
    DI.SetResolutions(&g_sizePixelsPerInch, &DI._sizeInch);

#endif  // IE5_ZOOM

    _dci = *((CDocInfo *)&DI);

    //
    //  *********************************** HACK FOR HOME PUBLISHER ***********************************
    //
    //  Home Publisher subclasses our HWND, taking control of WM_PAINT processing. Instead of allowing us to
    //  see the WM_PAINT, they catch the message and use IVO::Draw instead. Our asynch collect/send techniques
    //  causes them both performance problems and never-ending loops, as well as scrolling difficulties.
    //  So, the following special cases Home Publisher to make them work better (even though what they're doing
    //  is very, very bad!).
    //
    //  Essentially, the hack assumes that the IVO::Draw call is a replacement for WM_PAINT and behaves just
    //  as if WM_PAINT had been received instead. This only works when the IVO::Draw call is used by our
    //  host to supercede our WM_PAINT handling.
    //
    //  This hack can be removed once we no longer share the layout/display information maintained for our
    //  primary HWND with that used to service an IVO::Draw request.
    //
    //  (brendand)
    //
    //  *********************************** HACK FOR HOME PUBLISHER ***********************************
    //

    if (g_fInHomePublisher98)
    {
        BOOL    fViewIsReady;

        Assert(fHadView);

        fViewIsReady = _view.EnsureView(LAYOUT_SYNCHRONOUS | LAYOUT_INPAINT | LAYOUT_DEFERPAINT);
        Assert( !fViewIsReady
            ||  !_view.IsDisplayTreeOpen());

        if (fViewIsReady)
        {
            _view.RenderView(&DI, DI._hrgnClip, &DI._rcClip);
        }
        else
        {
            _view.Invalidate(&DI._rcClip);
        }
    }

    //
    //  Otherwise, proceed as normal
    //

    else
    {
        CRegion rgnInvalid;

        //
        // Update layout if needed
        // NOTE: Normally, this code should allow the paint to take place (in fact, it should force one), but doing so
        //       can cause problems for clients that use IVO::Draw at odd times.
        //       So, instead of pushing the paint through, the simple collects the invalid region and holds on to it
        //       until it's safe (see below). (brendand)
        //

        if (!fHadView)
        {
            _view.Activate();
        }
        else
        {
            _view.EnsureView(   LAYOUT_NOBACKGROUND
                            |   LAYOUT_SYNCHRONOUS
                            |   LAYOUT_DEFEREVENTS
                            |   LAYOUT_DEFERINVAL
                            |   LAYOUT_DEFERPAINT);

            rgnInvalid = _view._rgnInvalid;
            _view.ClearInvalid();
        }

        _view.GetViewSize(&sizeOld);
        sizeNew = ((CRect *)prc)->Size();

        _view.SetViewSize(sizeNew);

        _view.GetViewPosition(&ptOrgOld);
        _view.SetViewPosition(ptOrg);

        //
        //  In all cases, ensure the view is up-to-date with the passed dimensions
        //  (Do not invalidate the in-place HWND (if any) or do anything else significant as result (e.g., fire events)
        //   since the information backing it all is transient and only relevant to this request.)
        //

        _view.EnsureView(   LAYOUT_FORCE
                        |   LAYOUT_NOBACKGROUND
                        |   LAYOUT_SYNCHRONOUS
                        |   LAYOUT_DEFEREVENTS
                        |   LAYOUT_DEFERENDDEFER
                        |   LAYOUT_DEFERINVAL
                        |   LAYOUT_DEFERPAINT);
        _view.ClearInvalid();

        //
        //  Render the sites.
        //

        _view.RenderView(&DI, DI._hrgnClip, &DI._rcClip);

        //
        // Restore layout if inplace.
        // (Now, bring the view et. al. back in-sync with the document. Again, do not force a paint or any other
        //  significant work during this layout pass. Once it completes, however, re-establish any held invalid
        //  region and post a call such that layout/paint does eventually occur.)
        //

        if (fHadView)
        {
            _view.SetViewSize(sizeOld);
            _view.SetViewPosition(ptOrgOld);
            _view.EnsureView(   LAYOUT_FORCE
                            |   LAYOUT_NOBACKGROUND
                            |   LAYOUT_SYNCHRONOUS
                            |   LAYOUT_DEFEREVENTS
                            |   LAYOUT_DEFERENDDEFER
                            |   LAYOUT_DEFERINVAL
                            |   LAYOUT_DEFERPAINT);
            _view.ClearInvalid();

            if (!rgnInvalid.IsEmpty())
            {
                _view.OpenView();
                _view._rgnInvalid = rgnInvalid;
            }
        }
        else
        {
            _view.Deactivate();
        }
    }

Cleanup:
    if (DI._hrgnClip)
        DeleteObject(DI._hrgnClip);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormDrawInfo::DrawImageFlags
//
//  Synopsis:   Return DRAWIMAGE flags to be used when drawing with the DI
//
//----------------------------------------------------------------------------
DWORD
CFormDrawInfo::DrawImageFlags()
{
    return _fHtPalette ? 0 : DRAWIMAGE_NHPALETTE;
}

//-----------------------------------------------------
//
//  Member:     GetPalette
//
//  Synopsis:   Returns the current document palette and optionally
//              selects it into the destation DC.
//
//              The palette returned depends on several factors.
//              If there is an ambient palette, we always use that.
//              If the buffer depth or the screen depth is 8 (actually
//              if the screen is palettized) then we use whatever we
//              use _pColors
//      The final choice of palette is affected by several things:
//      _hpalAmbient (modified by OnAmbientPropertyChange)
//      _pColors (modified by InvalidateColors)
//
//

HPALETTE
CDoc::GetPalette(HDC hdc, BOOL *pfHtPal)
{
    HPALETTE hpal;
    BOOL fHtPal;
    
    // IE5 bug 65066 (dbau)
    // A buggy host can delete our ambient palette without warning us; 
    // let's detect and partially protect against that situation;
    // With a bit of paranoia, we also give _hpalDocument the same treatment.
    
    if (_hpalAmbient && GetObjectType((HGDIOBJ)_hpalAmbient) != OBJ_PAL)
    {
        TraceTag((tagError, "Error! Ambient palette was deleted from underneath mshtml.dll. Clearing value."));
        _hpalAmbient = NULL;
        _fHtAmbientPalette = FALSE;
    }
    
    if (_hpalDocument && GetObjectType((HGDIOBJ)_hpalDocument) != OBJ_PAL)
    {
        TraceTag((tagError, "Error! Document palette was deleted from underneath mshtml.dll. Clearing value."));
        _hpalDocument = NULL;
        _fHtDocumentPalette = FALSE;
    }
    
    hpal = _hpalAmbient;
    fHtPal = _fHtAmbientPalette;
    
    if (!hpal && (_bufferDepth == 8 || (GetDeviceCaps(TLS(hdcDesktop), RASTERCAPS) & RC_PALETTE)))
    {
        if (_hpalDocument)
        {
            hpal = _hpalDocument;
            fHtPal = _fHtDocumentPalette;
        }
        else
        {
            if (!_pColors)
            {
                CColorInfo CI;
                UpdateColors(&CI);
            }

            if (_pColors)
            {
                hpal = _hpalDocument = CreatePalette(_pColors);
                _fHtDocumentPalette = fHtPal = IsHalftonePalette(hpal);
            }
            else
            {
                hpal = GetDefaultPalette();
                fHtPal = TRUE;
            }
        }
    }

    if (pfHtPal)
    {
        *pfHtPal = fHtPal;
    }

    if (hpal && hdc)
    {
        SelectPalette(hdc, hpal, TRUE);
        RealizePalette(hdc);
    }
    
    return hpal;
}

//---------------------------------------
//
//  Member:     GetColors
//
//  Synopsis:   Computes the document color set (always for DVASPECT_CONTENT)
//              Unlike IE 3, we don't care about the UIActive control, instead
//              we allow the author to define colors.
//
//
HRESULT
CDoc::GetColors(CColorInfo *pCI)
{
    HRESULT hr = S_OK;

    if (_fGotAuthorPalette)
    {
        // REVIEW - michaelw (Can I count on having a _pDwnDoc)
        Assert(_pDwnDoc);
        hr = _pDwnDoc->GetColors(pCI);
    }

    if (SUCCEEDED(hr) && !pCI->IsFull())
    {
        CElement *pElementClient = GetPrimaryElementClient();
        
        hr = pElementClient ? pElementClient->GetColors(pCI) : S_FALSE;
    }

    //
    // Add the halftone colors in, only if we are not
    // hosted as a frame.  This isn't the place to
    // add system colors, that will get done by GetColorSet
    //
    if (SUCCEEDED(hr) && !pCI->IsFull() && !_pDocParent)
        hr = pCI->AddColors(236, &g_lpHalftone.ape[10]);

    RRETURN1(hr, S_FALSE);
}

HRESULT
CDoc::UpdateColors(CColorInfo *pCI)
{
    HRESULT hr = GetColors(pCI);

    if (SUCCEEDED(hr))
    {
        hr = pCI->GetColorSet(&_pColors);
    }

    RRETURN1(hr, S_FALSE);
}

STDMETHODIMP
CDoc::GetColorSet(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DVTARGETDEVICE * ptd, HDC hicTargetDev, LPLOGPALETTE * ppColorSet)
{
    if (ppColorSet == NULL)
        RRETURN(E_POINTER);

    HRESULT hr = S_OK;

    // We only cache the colors for DVASPECT_CONTENT

    if (dwDrawAspect != DVASPECT_CONTENT)
    {
        CColorInfo CI(dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev);

        hr = GetColors(&CI);

        if (SUCCEEDED(hr))
            hr = CI.GetColorSet(ppColorSet);
    }
    else
    {
        if (!_pColors)
        {
            CColorInfo CI(dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev);

            hr = UpdateColors(&CI);
        }

        if (hr == S_OK && _pColors)
        {
            unsigned cbColors = GetPaletteSize(_pColors);
            *ppColorSet = (LPLOGPALETTE) CoTaskMemAlloc(cbColors);
            if (*ppColorSet)
            {
                memcpy(*ppColorSet, _pColors, cbColors);
                hr = S_OK;
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
        {
            *ppColorSet = NULL;
            hr = S_FALSE;
        }
    }

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::InvalidateColors()
//
//  Synopsis:   Invalidates the color set of the document.
//
//----------------------------------------------------------------------------
void
CDoc::InvalidateColors()
{
    TraceTag((tagPalette, "InvalidateColors"));
    if (!_fColorsInvalid)
    {
        _fColorsInvalid = TRUE;

        GWPostMethodCallEx(GetThreadState(), (void *)this,
                           ONCALL_METHOD(CDoc, OnRecalcColors, onrecalccolors),
                           0, FALSE, "CDoc::OnRecalcColors");
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::RecalcColors()
//
//  Synopsis:   Recalcs the colors of the document and if necessary, notify's
//              our container.
//
//----------------------------------------------------------------------------

void
CDoc::OnRecalcColors(DWORD_PTR dwContext)
{
    TraceTag((tagPalette, "OnRecalcColors"));
    LOGPALETTE *pColors = NULL;

    CColorInfo CI;

    if (SUCCEEDED(GetColors(&CI)))
    {
        CI.GetColorSet(&pColors);
    }


    //
    // If the palettes are different, we need to broadcast the change
    //

    if ((pColors == 0) && (_pColors == 0))
        goto Cleanup;

    if (!pColors || !_pColors || ComparePalettes(pColors, _pColors))
    {
        TraceTag((tagPalette, "Document palette has changed!"));
        CoTaskMemFree(_pColors);
        _pColors = pColors;

#if 0
        _fNonhalftonePalette = ComparePalettes((LOGPALETTE *)&g_lpHalftone, pColors);
#endif

        //
        // Force the document palette (if any) to be recreated
        //
        if (_hpalDocument)
        {
            DeleteObject(_hpalDocument);
            _hpalDocument = 0;
            _fHtDocumentPalette = FALSE;
        }

        // If our container doesn't implement the ambient palette then we must
        // be faking it.  Now is the time to broadcast the change.
        if (GetAmbientPalette() == NULL)
        {
            CNotification   nf;

            nf.AmbientPropChange(PrimaryRoot(), (void *)DISPID_AMBIENT_PALETTE);
            BroadcastNotify(&nf);
        }

        //
        // This is the new, efficient way to say that our colors have changed.
        // The shell can avoid setting and advise sink (since it really doesn't
        // need it for any other reason) and just watch for this.  This prevents
        // palette recalcs everytime we do OnViewChange.
        //
        if (_pClientSite)
        {
            IOleCommandTarget *pCT;
            HRESULT hr = THR_NOTRACE(_pClientSite->QueryInterface(IID_IOleCommandTarget, (void **) &pCT));
            if (!hr)
            {
                VARIANT v;

                VariantInit(&v);

                V_VT(&v) = VT_BYREF;
                V_BYREF(&v) = pColors;

                THR_NOTRACE(pCT->Exec(
                        &CGID_ShellDocView,
                        SHDVID_ONCOLORSCHANGE,
                        OLECMDEXECOPT_DONTPROMPTUSER,
                        &v,
                        NULL));
                pCT->Release();
            }
        }
        OnViewChange(DVASPECT_CONTENT);
    }
    else
        CoTaskMemFree(pColors);

Cleanup:
    // Clear the way for the next InvalidateColors to force a RecalcColors.  If
    // for some bizarre reason RecalcColors (and by implication GetColors) causes
    // someone to call InvalidateColors after we've checked them, tough luck.
    _fColorsInvalid = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::updateInterval
//
//  Synopsis:   Set the paint update interval. Throttles multiple controls
//              that are randomly invalidating into a periodic painting
//              interval.
//
//----------------------------------------------------------------------------
void
CDoc::UpdateInterval(long interval)
{
    ITimerService  *pTM = NULL;
    VARIANT         vtimeMin, vtimeMax, vtimeInt;

    interval = interval < 0 ? 0 : interval; // no negative values
    VariantInit( &vtimeMin );    V_VT(&vtimeMin) = VT_UI4;
    VariantInit( &vtimeMax );    V_VT(&vtimeMax) = VT_UI4;
    VariantInit( &vtimeInt );    V_VT(&vtimeInt) = VT_UI4;

    if ( !_pUpdateIntSink )
    {
        if ( 0 == interval )
            return;

        // allocate timer and sink
        TraceTag((tagUpdateInt, "creating updateInterval sink"));
        Assert( !_pUpdateIntSink );
        _pUpdateIntSink = new CDocUpdateIntSink( this );
        if ( !_pUpdateIntSink )
            goto error;

        if ( FAILED(QueryService( SID_STimerService, IID_ITimerService, (void **)&pTM )) )
            goto error;

        if ( FAILED(pTM->GetNamedTimer( NAMEDTIMER_DRAW,
                                        &_pUpdateIntSink->_pTimer )) )
            goto error;

        pTM->Release();
        pTM = NULL;
    }

    if ( 0 == interval )
    {
        // disabling updateInterval, invalidate what we have
        HRGN    hrgn = _pUpdateIntSink->_hrgn;
        DWORD   hrgnFlags = _pUpdateIntSink->_dwFlags;

        TraceTag((tagUpdateInt, "deleting updateInterval sink. Cookie = %d", _pUpdateIntSink->_cookie));
        _pUpdateIntSink->_pDoc = NULL;      // let sink know not to respond.
        _pUpdateIntSink->_pTimer->Unadvise( _pUpdateIntSink->_cookie );
        _pUpdateIntSink->_pTimer->Release();
        _pUpdateIntSink->Release();
        _pUpdateIntSink = NULL;
        Invalidate( NULL, NULL, hrgn, hrgnFlags );
        DeleteObject( hrgn );

    }
    else if ( interval != _pUpdateIntSink->_interval )
    {
        // reset timer interval
        _pUpdateIntSink->_pTimer->GetTime(&vtimeMin);
        V_UI4(&vtimeMax) = 0;
        V_UI4(&vtimeInt) = interval;
        _pUpdateIntSink->_pTimer->Unadvise( _pUpdateIntSink->_cookie );     // ok if 0
        if ( FAILED(_pUpdateIntSink->_pTimer->Advise(vtimeMin, vtimeMax,
                                                     vtimeInt, 0,
                                                     (ITimerSink *)_pUpdateIntSink,
                                                     &_pUpdateIntSink->_cookie )) )
            goto error;

        _pUpdateIntSink->_interval = interval;
        TraceTag((tagUpdateInt, "setting updateInterval = %d", _pUpdateIntSink->_interval));
    }

cleanup:
    return;

error:
    if ( _pUpdateIntSink )
    {
        ReleaseInterface( _pUpdateIntSink->_pTimer );
        _pUpdateIntSink->Release();
        _pUpdateIntSink = NULL;
    }
    ReleaseInterface( pTM );
    goto cleanup;
}

LONG CDoc::GetUpdateInterval()
{
    return _pUpdateIntSink ? _pUpdateIntSink->_interval : 0;
}

BOOL IntersectRgnRect(HRGN hrgn, RECT *prc, RECT *prcIntersect)
{
    BOOL fIntersects;
    HRGN hrgnScratch;

    if (!hrgn)
    {
        Assert(prc && prcIntersect);

        *prcIntersect = *prc;
        fIntersects = TRUE;

        return fIntersects;
    }

    hrgnScratch = CreateRectRgnIndirect(prc);

    switch (CombineRgn(hrgnScratch, hrgnScratch, hrgn, RGN_AND))
    {
    case NULLREGION:
        memset(prcIntersect, 0, sizeof(*prcIntersect));
        fIntersects = FALSE;
        break;

    default:
        if (GetRgnBox(hrgnScratch, prcIntersect) != ERROR)
        {
            fIntersects = TRUE;
            break;
        }
        // fall through

    case ERROR:
        *prcIntersect = *prc;
        fIntersects = TRUE;
        break;
    }

    DeleteObject(hrgnScratch);

    return fIntersects;
}


/******************************************************************************
                CDocUpdateIntSink
******************************************************************************/
CDocUpdateIntSink::CDocUpdateIntSink( CDoc *pDoc )
{
    _pDoc = pDoc;
    _hrgn = 0;
    _dwFlags = 0;
    _state = UPDATEINTERVAL_EMPTY;
    _interval = 0;
    _pTimer = NULL;
    _cookie = 0;
    _ulRefs = 1;
}

CDocUpdateIntSink::~CDocUpdateIntSink( )
{
    Assert( !_pDoc );     // Makes sure CDoc knows we are on our way out.
}

ULONG
CDocUpdateIntSink::AddRef()
{
    return ++_ulRefs;
}

ULONG
CDocUpdateIntSink::Release()
{
    if ( 0 == --_ulRefs )
    {
        delete this;
        return 0;
    }
    return _ulRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     QueryInterface
//
//  Synopsis:   IUnknown implementation.
//
//  Arguments:  the usual
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CDocUpdateIntSink::QueryInterface(REFIID iid, void **ppv)
{
    if ( !ppv )
        RRETURN(E_POINTER);

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((ITimerSink *)this, IUnknown)
        QI_INHERITS(this, ITimerSink)
        default:
            break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}

//+----------------------------------------------------------------------------
//
//  Method:     OnTimer             [ITimerSink]
//
//  Synopsis:   Takes the accumulated region and invalidates the window
//
//  Arguments:  timeAdvie - the time that the advise was set.
//
//  Returns:    S_OK
//
//-----------------------------------------------------------------------------
HRESULT
CDocUpdateIntSink::OnTimer( VARIANT vtimeAdvise )
{
    TraceTag((tagUpdateInt, "CDocUpdateIntSink::OnTimer "));
    if ( _state != UPDATEINTERVAL_EMPTY &&
         _pDoc && _pDoc->_state >= OS_INPLACE &&
         _pDoc->LoadStatus() >= LOADSTATUS_INTERACTIVE )
    {
        TraceTag((tagUpdateInt, "CDocUpdateIntSink::OnTimer redrawing window at %d", V_UI4(&vtimeAdvise) ));
        Assert(_pDoc->_pInPlace);

        // CHROME
        // If Chrome hosted and windowless then can't RedrawWindow as no hwnd is present
        // so use the windowless site's InvalidateRgn call instead
        if( !_pDoc->IsChromeHosted() )
        {
            Assert(_pDoc->_pInPlace->_hwnd);
            RedrawWindow( _pDoc->_pInPlace->_hwnd, (GDIRECT *)NULL, _hrgn, _dwFlags );
        }
        else
        {
            Assert(_pDoc->_pInPlace->_fWindowlessInplace);
            ((IOleInPlaceSiteWindowless *)(_pDoc->_pInPlace->_pInPlaceSite))->InvalidateRgn( _hrgn, FALSE );
        }

        if ( _hrgn )
        {
            DeleteObject( _hrgn );
            _hrgn = NULL;
        }
        _state = UPDATEINTERVAL_EMPTY;
        _dwFlags = 0;

        // CHROME
        // If we are Chrome hosted and windowless then don't update window
        // as we have no hwnd. In any other case just UpdateWindow.
        if ( !_pDoc->IsChromeHosted() )
            UpdateWindow( _pDoc->_pInPlace->_hwnd );
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormDrawInfo::Init
//
//  Synopsis:   Initialize paint info for painting to form's hwnd
//
//----------------------------------------------------------------------------

void
CFormDrawInfo::Init(
    HDC     hdc,
    RECT *  prcClip)
{
    _hdc =
    _hic = (hdc
                ? hdc
                : TLS(hdcDesktop));

    _fInplacePaint = TRUE;
    _dwDrawAspect  = DVASPECT_CONTENT;
    _lindex        = -1;

    _dvai.cb       = sizeof(_dvai);
    _dvai.dwFlags  = DVASPECTINFOFLAG_CANOPTIMIZE;
    _pvAspect      = (void *)&_dvai;

    _rcClip.top    =
    _rcClip.left   = LONG_MIN;
    _rcClip.bottom =
    _rcClip.right  = LONG_MAX;

    if (_pDoc->IsPrintDoc())
    {
        FixupForPrint(DYNCAST(CPrintDoc, _pDoc->GetRootDoc()));
    }
    else if (_pDoc->State()     >= OS_INPLACE            &&
             _pDoc->LoadStatus() >= LOADSTATUS_INTERACTIVE )
    {
        IntersectRect(&_rcClipSet,
                      &_pDoc->_pInPlace->_rcClip,
                      &_pDoc->_pInPlace->_rcPos);
    }
    else
    {
        _rcClipSet = g_Zero.rc;
    }
    
    _fDeviceCoords = FALSE;
    _sizeDeviceOffset = g_Zero.size;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormDrawInfo::Init
//
//  Synopsis:   Initialize paint info for painting to form's hwnd
//
//----------------------------------------------------------------------------

void
CFormDrawInfo::Init(
    CElement * pElement,
    HDC     hdc,
    RECT *  prcClip)
{
    memset(this, 0, sizeof(*this));

    Assert(pElement);

    CDocInfo::Init(pElement);

    Init(hdc, prcClip);

    InitToSite(pElement->GetUpdatedLayout(), prcClip);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormDrawInfo::Init
//
//  Synopsis:   Initialize paint info for painting to form's hwnd
//
//----------------------------------------------------------------------------

void
CFormDrawInfo::Init(
    CLayout * pLayout,
    HDC     hdc,
    RECT *  prcClip)
{
    Assert(pLayout);

    Init(pLayout->ElementOwner(), hdc, prcClip);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormDrawInfo::InitToSite
//
//  Synopsis:   Reduce/set the clipping RECT to the visible portion of
//              the passed CSite
//
//----------------------------------------------------------------------------

void
CFormDrawInfo::InitToSite(CLayout * pLayout, RECT * prcClip)
{
    RECT    rcClip;
    RECT    rcUserClip;

    //
    // For CRootElement (no layout) on normal documents,
    // initialize the clip RECT to that of the entire document
    //

    if (!pLayout && !_pDoc->IsPrintDoc())
    {
        if (_pDoc->State()     >= OS_INPLACE            &&
            _pDoc->LoadStatus() >= LOADSTATUS_INTERACTIVE )
        {
            IntersectRect(&rcClip,
                          &_pDoc->_pInPlace->_rcClip,
                          &_pDoc->_pInPlace->_rcPos);
        }
        else
        {
            rcClip = g_Zero.rc;
        }

        rcUserClip = rcClip;
    }

    //
    // For all sites other than CRootSite or CRootSite on print documents,
    // set _rcClip to the visible client RECT
    // NOTE: Do not pass _rcClip to prevent its modification during initialization
    //

    else
    {
// BUGBUG: This needs to be fixed! (brendand)
        rcClip     =
        rcUserClip = g_Zero.rc;
    }

    //
    // Reduce the current clipping RECT
    //

    IntersectRect(&_rcClip, &_rcClip, &rcClip);

    //
    // If the clip RECT is empty, canonicalize it to all zeros
    //

    if (IsRectEmpty(&_rcClip))
    {
        _rcClip = g_Zero.rc;
    }

    //
    // If passed a clipping RECT, use it to reduce the clip RECT
    //

    if (prcClip)
    {
        IntersectRect (&_rcClip, &_rcClip, prcClip);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CFormDrawInfo::FixupForPrint
//
//  Synopsis:   Initialize for printing
//
//----------------------------------------------------------------------------

HRESULT
CFormDrawInfo::FixupForPrint(CPrintDoc * pPrintDoc)
{
    // If we're not ready, just leave.

    if (!pPrintDoc->_hic)
        goto Cleanup;

    Assert(pPrintDoc->_ptd);
    Assert(pPrintDoc->_hdc);

    // Copy over fields that were erroneously filled in CFormDrawInfo::Init

    _ptd          = pPrintDoc->_ptd;
    _hdc          = pPrintDoc->_hdc;
    _hic          = pPrintDoc->_hic;
    _fIsMetafile  = ( GetDeviceCaps( _hic, TECHNOLOGY ) == DT_METAFILE );

    // BUGBUG = we leave this as content, because too many servers
    // including Trident do not support this
    //_dwDrawAspect = DVASPECT_DOCPRINT;
    _dwDrawAspect = DVASPECT_CONTENT;
    _lindex       = -1;
    _fInplacePaint = FALSE;

    // Set the clipping rectangle.  If _nPage is -1, we are still laying
    // out the page.  Maximum clip.

    // Copy over the CTransform.  Set our src/dst rectangles correctly.

    CDocInfo::Init( &pPrintDoc->_dci );

Cleanup:
    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     GetDC, GetGlobalDC, GetDirectDrawSurface, GetSurface
//
//  Synopsis:   Each of these is a simple layer over the true CDispSurface call
//
//----------------------------------------------------------------------------

HDC
CFormDrawInfo::GetDC(
    BOOL    fPhysicallyClip)
{
    if (    !_hdc
        &&  _pSurface)
    {
        if (!_fDeviceCoords)
        {
            _pSurface->GetDC(&_hdc, _rc, _rcClip, fPhysicallyClip);
        }
        
        else
        {
            // we have to do tricky stuff here because the client wants to deal
            // with device coordinates in order to circumvent GDI's 16-bit
            // coordinate limitations on Win 9x.  We can't use SetViewportOrgEx
            // as we can on Win NT because of potential overflow, especially
            // when printing large documents.
            _rc.OffsetRect(-_sizeDeviceOffset);
            _rcClip.OffsetRect(-_sizeDeviceOffset);
            _pSurface->GetDC(&_hdc, _rc, _rcClip, fPhysicallyClip);
            _rc.OffsetRect(_sizeDeviceOffset);
            _rcClip.OffsetRect(_sizeDeviceOffset);
            ::SetViewportOrgEx(_hdc, 0, 0, NULL);
        }
    }

    return _hdc;
}


HDC
CFormDrawInfo::GetGlobalDC(
    BOOL    fPhysicallyClip)
{
    // we don't anticipate using the device coordinate mode in this case
    Assert(!_fDeviceCoords);
    
    HDC hdc = NULL;

    if (_pSurface)
    {
        _pSurface->GetGlobalDC(&hdc, &_rc, &_rcClip, fPhysicallyClip);
    }

    return hdc;
}


HRESULT
CFormDrawInfo::GetDirectDrawSurface(
    IDirectDrawSurface **   ppSurface,
    SIZE *                  pOffset)
{
    return _pSurface
                ? _pSurface->GetDirectDrawSurface(ppSurface, pOffset)
                : E_FAIL;
}


HRESULT
CFormDrawInfo::GetSurface(
    const IID & iid,
    void **     ppv,
    SIZE *      pOffset)
{
    return _pSurface
                ? _pSurface->GetSurface(iid, ppv, pOffset)
                : E_FAIL;
}


void
CFormDrawInfo::SetDeviceCoordinateMode()
{
    // on Win9x, we jump through hoops to avoid the 16-bit coordinate
    // limitations in GDI
    BOOL fHackForGDI = g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS;

#if DBG==1
    if (IsTagEnabled(tagHackGDICoords))
        fHackForGDI = TRUE;
#endif

    if (fHackForGDI && _pSurface)
    {
        if (!_fDeviceCoords)
        {
            _pSurface->GetOffset(&_sizeDeviceOffset);
            _rc.OffsetRect(_sizeDeviceOffset);
            _rcClip.OffsetRect(_sizeDeviceOffset);
            _fDeviceCoords = TRUE;
        }
    }
    else
    {
        _sizeDeviceOffset = g_Zero.size;
        _fDeviceCoords = FALSE;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CSetDrawInfo::CSetDrawInfo
//
//  Synopsis:   Initialize a CSetDrawInfo
//
//----------------------------------------------------------------------------

CSetDrawSurface::CSetDrawSurface(
    CFormDrawInfo * pDI,
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pSurface)
{
    Assert(pDI);
    Assert(pSurface);

    _pDI      = pDI;
    _hdc      = pDI->_hdc;
    _pSurface = pDI->_pSurface;

    _pDI->_pSurface = pSurface;
    _pDI->_rc       = *prcBounds;
    _pDI->_rcClip   = *prcRedraw;
    _pDI->_fIsMemoryDC = _pDI->_pSurface->IsMemory();
    _pDI->_fIsMetafile = _pDI->_pSurface->IsMetafile();

    _pDI->_fDeviceCoords = FALSE;
    _pDI->_sizeDeviceOffset = g_Zero.size;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDocInfo::Init
//
//  Synopsis:   Initialize a CDocInfo.
//
//----------------------------------------------------------------------------

void
CDocInfo::Init(CElement * pElement)
{
    Assert(pElement);

    _pDoc = pElement->Doc();

    Assert(_pDoc);

    Init(&_pDoc->_dci);

}

//+---------------------------------------------------------------------------
//
//  Member:     CParentInfo::Init
//
//  Synopsis:   Initialize a CParentInfo.
//
//----------------------------------------------------------------------------

void
CParentInfo::Init()
{
    _sizeParent = g_Zero.size;
}

void
CParentInfo::Init(const CDocInfo * pdci)
{
    Assert(pdci);

    ::memcpy(this, pdci, sizeof(CDocInfo));

    Init();
}

void
CParentInfo::Init(const CCalcInfo * pci)
{
    ::memcpy(this, pci, sizeof(CParentInfo));
}

void
CParentInfo::Init(SIZE * psizeParent)
{
    SizeToParent(psizeParent
                    ? psizeParent
                    : &g_Zero.size);
}

void
CParentInfo::Init(CLayout * pLayout)
{
    Assert(pLayout);

    CDocInfo::Init(pLayout->ElementOwner());

    SizeToParent(pLayout);
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentInfo::SizeToParent
//
//  Synopsis:   Set the parent size to the client RECT of the passed CLayout
//
//----------------------------------------------------------------------------

void
CParentInfo::SizeToParent(CLayout * pLayout)
{
    RECT    rc;

    Assert(pLayout);

    pLayout->GetClientRect(&rc);
    SizeToParent(&rc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCalcInfo::Init
//
//  Synopsis:   Initialize a CCalcInfo.
//
//----------------------------------------------------------------------------

void
CCalcInfo::Init()
{
    _smMode     = SIZEMODE_NATURAL;
    _grfLayout  = 0L;
    _hdc        = ( _pDoc ? _pDoc->GetHDC() : TLS(hdcDesktop) );
    _yBaseLine  = 0;
    _fUseOffset     = FALSE;
    _fTableCalcInfo = FALSE;
}

void
CCalcInfo::Init(const CDocInfo * pdci, CLayout * pLayout)
{
    CView *     pView;
    DWORD       grfState;

    Assert(pdci);
    Assert(pLayout);

    ::memcpy(this, pdci, sizeof(CDocInfo));

    CParentInfo::Init();

    Init();

    pView = pLayout->GetView();
    Assert(pView);

    grfState = pView->GetState();

    if (!(grfState & (CView::VS_OPEN | CView::VS_INLAYOUT | CView::VS_INRENDER)))
    {
        Verify(pView->OpenView());
    }
}

void
CCalcInfo::Init(CLayout * pLayout, SIZE * psizeParent, HDC hdc)
{
    CView *     pView;
    DWORD       grfState;

    CParentInfo::Init(pLayout);

    _smMode    = SIZEMODE_NATURAL;
    _grfLayout = 0L;

    // If a DC was passed in, use that one - else ask the doc which DC to use.
    // Only if there is no doc yet, use the desktop DC.
    if (hdc)
        _hdc = hdc;
    else if (_pDoc)
        _hdc = _pDoc->GetHDC();
    else
        _hdc = TLS(hdcDesktop);

    _yBaseLine = 0;
    _fUseOffset     = FALSE;
    _fTableCalcInfo = FALSE;

    pView = pLayout->GetView();
    Assert(pView);

    grfState = pView->GetState();

    if (!(grfState & (CView::VS_OPEN | CView::VS_INLAYOUT | CView::VS_INRENDER)))
    {
        Verify(pView->OpenView());
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetLabel
//
//  Synopsis:   Return any label element associated with this element. If the
//              element has no label, return NULL. If the element has more than
//              one label, return the first one encountered in the collection.
//
//----------------------------------------------------------------------------

CLabelElement *
CElement::GetLabel() const
{
    HRESULT         hr = S_OK;
    CElement *      pElem;
    CLabelElement * pLabel = NULL;
    int             iCount, iIndex;
    LPCTSTR         pszIdFor, pszId;
    CCollectionCache *pCollectionCache;
    CMarkup *       pMarkup;

    // check if there is label associated with this site

    pszId = GetAAid();
    if (!pszId || lstrlen(pszId) == 0)
        goto Cleanup;

    pMarkup = GetMarkupPtr();
    hr = THR(pMarkup->EnsureCollectionCache(CMarkup::LABEL_COLLECTION));
    if (hr)
        goto Cleanup;

    pCollectionCache = pMarkup->CollectionCache();
    iCount = pCollectionCache->SizeAry(CMarkup::LABEL_COLLECTION);
    if (iCount <= 0)
        goto Cleanup;

    for (iIndex = iCount - 1; iIndex >= 0; iIndex--)
    {
        hr = THR(pCollectionCache->GetIntoAry(
                CMarkup::LABEL_COLLECTION,
                iIndex,
                &pElem));
        if (hr)
            goto Cleanup;

        pLabel = DYNCAST(CLabelElement, pElem);
        pszIdFor = pLabel->GetAAhtmlFor();
        if (!pszIdFor || lstrlen(pszIdFor) == 0)
            continue;

        if (!FormsStringICmp(pszIdFor, pszId))
            break; // found the associated label
    }

    if (iIndex < 0)
        pLabel = NULL;

Cleanup:
    // ignore hr
    return pLabel;
}
