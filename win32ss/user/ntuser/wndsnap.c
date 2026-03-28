/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Window snap preview animation
 * FILE:             win32ss/user/ntuser/wndsnap.c
 * PROGRAMER:
 */

#include <win32k.h>

DBG_DEFAULT_CHANNEL(UserWinpos);

/* Snap preview animation constants */
#define SNAP_ANIM_DURATION_MS 180
#define SNAP_PREVIEW_FILL_ALPHA 72
#define SNAP_PREVIEW_BORDER_ALPHA 160
#define SNAP_PREVIEW_BORDER_WIDTH 2

/* --- Internal helpers --- */

static VOID
SnapPreviewDestroyBuffers(SNAP_PREVIEW_STATE *pState)
{
    if (pState->hdcBackground)
    {
        if (pState->hbmBackground && pState->hbmBackgroundOld)
            NtGdiSelectBitmap(pState->hdcBackground, pState->hbmBackgroundOld);
        if (pState->hbmBackground)
            GreDeleteObject(pState->hbmBackground);
        IntGdiDeleteDC(pState->hdcBackground, FALSE);
    }

    if (pState->hdcOverlay)
    {
        if (pState->hbmOverlay && pState->hbmOverlayOld)
            NtGdiSelectBitmap(pState->hdcOverlay, pState->hbmOverlayOld);
        if (pState->hbmOverlay)
            GreDeleteObject(pState->hbmOverlay);
        IntGdiDeleteDC(pState->hdcOverlay, FALSE);
    }

    pState->hdcBackground = NULL;
    pState->hbmBackground = NULL;
    pState->hbmBackgroundOld = NULL;
    pState->hdcOverlay = NULL;
    pState->hbmOverlay = NULL;
    pState->hbmOverlayOld = NULL;
}

static BOOL
SnapPreviewCreateBuffers(HDC hdc, SNAP_PREVIEW_STATE *pState, LONG cx, LONG cy)
{
    if (cx <= 0 || cy <= 0 || !pState->hbrFill || !pState->hbrBorder)
        return FALSE;

    SnapPreviewDestroyBuffers(pState);

    pState->hdcBackground = NtGdiCreateCompatibleDC(hdc);
    if (!pState->hdcBackground)
        return FALSE;

    pState->hdcOverlay = NtGdiCreateCompatibleDC(hdc);
    if (!pState->hdcOverlay)
    {
        SnapPreviewDestroyBuffers(pState);
        return FALSE;
    }

    pState->hbmBackground = NtGdiCreateCompatibleBitmap(hdc, cx, cy);
    pState->hbmOverlay = NtGdiCreateCompatibleBitmap(hdc, cx, cy);
    if (!pState->hbmBackground || !pState->hbmOverlay)
    {
        SnapPreviewDestroyBuffers(pState);
        return FALSE;
    }

    pState->hbmBackgroundOld = NtGdiSelectBitmap(pState->hdcBackground, pState->hbmBackground);
    pState->hbmOverlayOld = NtGdiSelectBitmap(pState->hdcOverlay, pState->hbmOverlay);
    if (!pState->hbmBackgroundOld || !pState->hbmOverlayOld)
    {
        SnapPreviewDestroyBuffers(pState);
        return FALSE;
    }

    return TRUE;
}

static BOOL
SnapPreviewBlendSolid(HDC hdcDst,
                      HDC hdcSrc,
                      HBRUSH hbr,
                      INT xDst,
                      INT yDst,
                      INT cx,
                      INT cy,
                      BYTE Alpha,
                      const RECT *prcExclude)
{
    BLENDFUNCTION Blend = { AC_SRC_OVER, 0, Alpha, 0 };
    HBRUSH hbrOld;
    BOOL bResult;
    INT iSaveLevel = 0;

    if (cx <= 0 || cy <= 0)
        return TRUE;

    hbrOld = NtGdiSelectBrush(hdcSrc, hbr);
    NtGdiPatBlt(hdcSrc, 0, 0, cx, cy, PATCOPY);
    NtGdiSelectBrush(hdcSrc, hbrOld);

    if (prcExclude && !RECTL_bIsEmptyRect(prcExclude))
    {
        iSaveLevel = NtGdiSaveDC(hdcDst);
        if (iSaveLevel > 0)
        {
            NtGdiExcludeClipRect(hdcDst,
                                 prcExclude->left,
                                 prcExclude->top,
                                 prcExclude->right,
                                 prcExclude->bottom);
        }
    }

    bResult = NtGdiAlphaBlend(hdcDst,
                              xDst,
                              yDst,
                              cx,
                              cy,
                              hdcSrc,
                              0,
                              0,
                              cx,
                              cy,
                              Blend,
                              0);

    if (iSaveLevel > 0)
        NtGdiRestoreDC(hdcDst, -1);

    return bResult;
}

static BOOL
SnapPreviewCaptureBackground(HDC hdc, SNAP_PREVIEW_STATE *pState, const RECT *prc)
{
    LONG cx = prc->right - prc->left;
    LONG cy = prc->bottom - prc->top;

    return NtGdiBitBlt(pState->hdcBackground,
                       0,
                       0,
                       cx,
                       cy,
                       hdc,
                       prc->left,
                       prc->top,
                       SRCCOPY,
                       CLR_INVALID,
                       0);
}

static VOID
SnapPreviewRestore(HDC hdc, SNAP_PREVIEW_STATE *pState)
{
    LONG cx = pState->rcCurrent.right - pState->rcCurrent.left;
    LONG cy = pState->rcCurrent.bottom - pState->rcCurrent.top;

    NtGdiBitBlt(hdc,
                pState->rcCurrent.left,
                pState->rcCurrent.top,
                cx,
                cy,
                pState->hdcBackground,
                0,
                0,
                SRCCOPY,
                CLR_INVALID,
                0);
}

static VOID
SnapPreviewInterpolateRect(const RECT *pOrigin, const RECT *pTarget,
                           ULONG dwElapsed, RECT *pResult)
{
    LONG t;

    if (dwElapsed >= SNAP_ANIM_DURATION_MS)
    {
        *pResult = *pTarget;
        return;
    }

    /* Fixed-point progress 0..256 */
    t = (LONG)(dwElapsed * 256 / SNAP_ANIM_DURATION_MS);

    /* Cubic ease-out for a snappier Win7-like expansion */
    {
        LONG inv = 256 - t;
        LONG inv2 = inv * inv;
        t = 256 - (inv2 * inv / 65536);
    }

    pResult->left   = pOrigin->left   + (pTarget->left   - pOrigin->left)   * t / 256;
    pResult->top    = pOrigin->top    + (pTarget->top    - pOrigin->top)    * t / 256;
    pResult->right  = pOrigin->right  + (pTarget->right  - pOrigin->right)  * t / 256;
    pResult->bottom = pOrigin->bottom + (pTarget->bottom - pOrigin->bottom) * t / 256;
}

static VOID
SnapPreviewInterpolateCurrentRect(SNAP_PREVIEW_STATE *pState, RECT *pResult)
{
    ULONG dwElapsed = EngGetTickCount32() - pState->dwStartTime;

    SnapPreviewInterpolateRect(&pState->rcOrigin, &pState->rcTarget,
                               dwElapsed, pResult);
}

static BOOL
SnapPreviewHasReachedTarget(const SNAP_PREVIEW_STATE *pState)
{
    return pState->rcCurrent.left   == pState->rcTarget.left  &&
           pState->rcCurrent.top    == pState->rcTarget.top   &&
           pState->rcCurrent.right  == pState->rcTarget.right &&
           pState->rcCurrent.bottom == pState->rcTarget.bottom;
}

/* --- Public interface --- */

VOID
SnapPreviewInit(SNAP_PREVIEW_STATE *pState)
{
    RtlZeroMemory(pState, sizeof(*pState));
    pState->hbrFill = IntGdiCreateSolidBrush(RGB(176, 214, 244));
    pState->hbrBorder = IntGdiCreateSolidBrush(RGB(94, 150, 214));
}

BOOL
SnapPreviewAdvance(HDC hdc, SNAP_PREVIEW_STATE *pState, const RECT *prcExclude)
{
    RECT rcNew;

    if (!pState->bVisible || SnapPreviewHasReachedTarget(pState))
        return FALSE;

    SnapPreviewInterpolateCurrentRect(pState, &rcNew);

    if (rcNew.left   != pState->rcCurrent.left  ||
        rcNew.top    != pState->rcCurrent.top    ||
        rcNew.right  != pState->rcCurrent.right  ||
        rcNew.bottom != pState->rcCurrent.bottom)
    {
        LONG cx = rcNew.right - rcNew.left;
        LONG cy = rcNew.bottom - rcNew.top;
        LONG bw = min(SNAP_PREVIEW_BORDER_WIDTH, cx / 2);
        LONG bh = min(SNAP_PREVIEW_BORDER_WIDTH, cy / 2);
        LONG cyMiddle = max(0, cy - 2 * bh);

        SnapPreviewRestore(hdc, pState);
        if (!SnapPreviewCaptureBackground(hdc, pState, &rcNew))
        {
            pState->bVisible = FALSE;
            return FALSE;
        }

        if (!SnapPreviewBlendSolid(hdc, pState->hdcOverlay, pState->hbrFill,
                                   rcNew.left, rcNew.top, cx, cy,
                                   SNAP_PREVIEW_FILL_ALPHA, prcExclude))
        {
            pState->bVisible = FALSE;
            return FALSE;
        }

        SnapPreviewBlendSolid(hdc, pState->hdcOverlay, pState->hbrBorder,
                              rcNew.left, rcNew.top, cx, bh,
                              SNAP_PREVIEW_BORDER_ALPHA, prcExclude);
        SnapPreviewBlendSolid(hdc, pState->hdcOverlay, pState->hbrBorder,
                              rcNew.left, rcNew.bottom - bh, cx, bh,
                              SNAP_PREVIEW_BORDER_ALPHA, prcExclude);
        SnapPreviewBlendSolid(hdc, pState->hdcOverlay, pState->hbrBorder,
                              rcNew.left, rcNew.top + bh, bw, cyMiddle,
                              SNAP_PREVIEW_BORDER_ALPHA, prcExclude);
        SnapPreviewBlendSolid(hdc, pState->hdcOverlay, pState->hbrBorder,
                              rcNew.right - bw, rcNew.top + bh, bw, cyMiddle,
                              SNAP_PREVIEW_BORDER_ALPHA, prcExclude);

        pState->rcCurrent = rcNew;
    }

    return !SnapPreviewHasReachedTarget(pState);
}

VOID
SnapPreviewHide(HDC hdc, SNAP_PREVIEW_STATE *pState)
{
    if (pState->bVisible)
    {
        SnapPreviewRestore(hdc, pState);
        pState->bVisible = FALSE;
    }
    pState->nSnapEdge = HTNOWHERE;
}

VOID
SnapPreviewShow(HDC hdc, SNAP_PREVIEW_STATE *pState,
                UINT nEdge, const RECT *pTargetRect, POINT ptCursor,
                const RECT *prcExclude)
{
    if (!pState->hbrFill || !pState->hbrBorder)
        return;

    /* Same edge and same target: advance the animation */
    if (pState->nSnapEdge == nEdge &&
        pState->bVisible &&
        RtlEqualMemory(&pState->rcTarget, pTargetRect, sizeof(*pTargetRect)))
    {
        SnapPreviewAdvance(hdc, pState, prcExclude);
        return;
    }

    /* New edge: erase any existing preview, start fresh animation */
    if (pState->bVisible)
    {
        SnapPreviewRestore(hdc, pState);
        pState->bVisible = FALSE;
    }

    pState->nSnapEdge = nEdge;
    pState->rcTarget = *pTargetRect;
    pState->dwStartTime = EngGetTickCount32();
    if (!SnapPreviewCreateBuffers(hdc,
                                  pState,
                                  pTargetRect->right - pTargetRect->left,
                                  pTargetRect->bottom - pTargetRect->top))
    {
        pState->nSnapEdge = HTNOWHERE;
        return;
    }

    /* Origin: small rect centered at cursor */
    pState->rcOrigin.left   = ptCursor.x - 10;
    pState->rcOrigin.top    = ptCursor.y - 10;
    pState->rcOrigin.right  = ptCursor.x + 10;
    pState->rcOrigin.bottom = ptCursor.y + 10;

    pState->rcCurrent = pState->rcOrigin;
    if (!SnapPreviewCaptureBackground(hdc, pState, &pState->rcCurrent))
        return;

    if (!SnapPreviewBlendSolid(hdc,
                               pState->hdcOverlay,
                               pState->hbrFill,
                               pState->rcCurrent.left,
                               pState->rcCurrent.top,
                               pState->rcCurrent.right - pState->rcCurrent.left,
                               pState->rcCurrent.bottom - pState->rcCurrent.top,
                               SNAP_PREVIEW_FILL_ALPHA,
                               prcExclude))
        return;

    pState->bVisible = TRUE;
    SnapPreviewAdvance(hdc, pState, prcExclude);
}

VOID
SnapPreviewCleanup(HDC hdc, SNAP_PREVIEW_STATE *pState)
{
    SnapPreviewHide(hdc, pState);
    SnapPreviewDestroyBuffers(pState);
    if (pState->hbrFill)
        GreDeleteObject(pState->hbrFill);
    if (pState->hbrBorder)
        GreDeleteObject(pState->hbrBorder);
    pState->hbrFill = NULL;
    pState->hbrBorder = NULL;
}

/* --- Window snap logic --- */

UINT
GetSnapActivationPoint(PWND Wnd, POINT pt)
{
    // TODO: SPI_GETMOUSEDOCKTHRESHOLD
    RECT wa;
    if (!GetSnapSetting(bDockMoving))
        return HTNOWHERE;
    UserSystemParametersInfo(SPI_GETWORKAREA, 0, &wa, 0); /* FIXME: MultiMon of PWND */

    if (pt.x <= wa.left) return HTLEFT;
    if (pt.x >= wa.right-1) return HTRIGHT;
    if (pt.y <= wa.top) return HTTOP; /* Maximize */
    return HTNOWHERE;
}

/* Windows 10 (1903?)
BOOL APIENTRY
NtUserIsWindowArranged(HWND hWnd)
{
    PWND pwnd = UserGetWindowObject(hWnd);
    return pwnd && IntIsWindowSnapped(pwnd);
}
*/

UINT FASTCALL
IntGetWindowSnapEdge(PWND Wnd)
{
    if (Wnd->ExStyle2 & WS_EX2_VERTICALLYMAXIMIZEDLEFT) return HTLEFT;
    if (Wnd->ExStyle2 & WS_EX2_VERTICALLYMAXIMIZEDRIGHT) return HTRIGHT;
    return HTNOWHERE;
}

VOID FASTCALL
co_IntCalculateSnapPosition(PWND Wnd, UINT Edge, OUT RECT *Pos)
{
    POINT maxs, mint, maxt;
    UINT width, height;
    UserSystemParametersInfo(SPI_GETWORKAREA, 0, Pos, 0); /* FIXME: MultiMon of PWND */

    co_WinPosGetMinMaxInfo(Wnd, &maxs, NULL, &mint, &maxt);
    width = Pos->right - Pos->left;
    width = min(min(max(width / 2, mint.x), maxt.x), width);
    height = Pos->bottom - Pos->top;
    height = min(max(height, mint.y), maxt.y);

    switch (Edge)
    {
    case HTTOP: /* Maximized (Calculate RECT snap preview for SC_MOVE) */
        height = min(Pos->bottom - Pos->top, maxs.y);
        break;
    case HTLEFT:
        Pos->right = width;
        break;
    case HTRIGHT:
        Pos->left = Pos->right - width;
        break;
    default:
        ERR("Unexpected snap edge %#x\n", Edge);
    }
    Pos->bottom = Pos->top + height;
}

VOID FASTCALL
co_IntSnapWindow(PWND Wnd, UINT Edge)
{
    RECT newPos;
    BOOLEAN wasSnapped = IntIsWindowSnapped(Wnd);
    UINT normal = !(Wnd->style & (WS_MAXIMIZE | WS_MINIMIZE));
    USER_REFERENCE_ENTRY ref;
    BOOLEAN hasRef = FALSE;

    if (Edge == HTTOP)
    {
        co_IntSendMessage(UserHMGetHandle(Wnd), WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        return;
    }
    else if (Edge != HTNOWHERE)
    {
        UserRefObjectCo(Wnd, &ref);
        hasRef = TRUE;
        co_IntCalculateSnapPosition(Wnd, Edge, &newPos);
        IntSetSnapInfo(Wnd, Edge, (wasSnapped || !normal) ? NULL : &Wnd->rcWindow);
    }
    else if (wasSnapped)
    {
        if (!normal)
        {
            IntSetSnapEdge(Wnd, HTNOWHERE);
            return;
        }
        newPos = Wnd->InternalPos.NormalRect; /* Copy RECT now before it is lost */
        IntSetSnapInfo(Wnd, HTNOWHERE, NULL);
    }
    else
    {
        return; /* Already unsnapped, do nothing */
    }

    TRACE("WindowSnap: %d->%d\n", IntGetWindowSnapEdge(Wnd), Edge);
    co_WinPosSetWindowPos(Wnd, HWND_TOP,
                          newPos.left,
                          newPos.top,
                          newPos.right - newPos.left,
                          newPos.bottom - newPos.top,
                          0);
    if (hasRef)
        UserDerefObjectCo(Wnd);
}

VOID FASTCALL
IntSetSnapEdge(PWND Wnd, UINT Edge)
{
    UINT styleMask = WS_EX2_VERTICALLYMAXIMIZEDLEFT | WS_EX2_VERTICALLYMAXIMIZEDRIGHT;
    UINT style = 0;
    switch (Edge)
    {
    case HTNOWHERE:
        style = 0;
        break;
    case HTTOP: /* Maximize throws away the snap */
        style = 0;
        break;
    case HTLEFT:
        style = WS_EX2_VERTICALLYMAXIMIZEDLEFT;
        break;
    case HTRIGHT:
        style = WS_EX2_VERTICALLYMAXIMIZEDRIGHT;
        break;
    default:
        ERR("Unexpected snap edge %#x\n", Edge);
    }
    Wnd->ExStyle2 = (Wnd->ExStyle2 & ~styleMask) | style;
}

VOID FASTCALL
IntSetSnapInfo(PWND Wnd, UINT Edge, IN const RECT *Pos OPTIONAL)
{
    RECT r;
    IntSetSnapEdge(Wnd, Edge);
    if (Edge == HTNOWHERE)
    {
        RECTL_vSetEmptyRect(&r);
        Pos = (Wnd->style & WS_MINIMIZE) ? NULL : &r;
    }
    if (Pos)
    {
        Wnd->InternalPos.NormalRect = *Pos;
    }
}
