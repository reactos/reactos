/****************************** Module Header ******************************\
* Module Name: sprite.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Windows Layering (Sprite) support.
*
* History:
* 12/05/97      vadimg      created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* IncrementRedirectedCount
*
\***************************************************************************/

VOID IncrementRedirectedCount(VOID)
{
    gnRedirectedCount++;

    if (gnRedirectedCount == 1) {
        InternalSetTimer(gTermIO.spwndDesktopOwner, IDSYS_LAYER, 100,
                xxxSystemTimerProc, TMRF_SYSTEM | TMRF_PTIWINDOW);
    }

    UserAssert(gnRedirectedCount >= 0);
}

/***************************************************************************\
* DecrementRedirectedCount
*
\***************************************************************************/

VOID DecrementRedirectedCount(VOID)
{
    gnRedirectedCount--;

    if (gnRedirectedCount == 0) {
        _KillSystemTimer(gTermIO.spwndDesktopOwner, IDSYS_LAYER);
    }

    UserAssert(gnRedirectedCount >= 0);
}

/***************************************************************************\
* CreateRedirectionBitmap
*
* 10/1/1998        vadimg      created
\***************************************************************************/

HBITMAP CreateRedirectionBitmap(PWND pwnd)
{
    HBITMAP hbm;

    UserAssert(pwnd->rcWindow.right >= pwnd->rcWindow.left);
    UserAssert(pwnd->rcWindow.bottom >= pwnd->rcWindow.top);

    /*
     * Make sure the (0,0) case doesn't fail, since the window really
     * can be sized this way.
     */
    if ((hbm = GreCreateCompatibleBitmap(gpDispInfo->hdcScreen,
            max(pwnd->rcWindow.right - pwnd->rcWindow.left, 1),
            max(pwnd->rcWindow.bottom - pwnd->rcWindow.top, 1) |
            CCB_NOVIDEOMEMORY)) == NULL) {
        RIPMSG0(RIP_WARNING, "CreateRedirectionBitmap: bitmap create failed");
        return NULL;
    }

    if (!GreSetBitmapOwner(hbm, OBJECT_OWNER_PUBLIC) ||
            !GreMarkUndeletableBitmap(hbm) ||
            !InternalSetProp(pwnd, PROP_LAYER, hbm, PROPF_INTERNAL |
            PROPF_NOPOOL)) {
        RIPMSG0(RIP_WARNING, "CreateRedirectionBitmap: bitmap set failed");
        GreMarkDeletableBitmap(hbm);
        GreDeleteObject(hbm);
        return NULL;
    }

    /*
     * Force the window to redraw if we could recreate the bitmap since
     * the redirection bitmap we just allocated doesn't contain anything
     * yet.
     */
    BEGINATOMICCHECK();
    xxxInternalInvalidate(pwnd, HRGN_FULL, RDW_INVALIDATE | RDW_ERASE |
            RDW_FRAME | RDW_ALLCHILDREN);
    ENDATOMICCHECK();    

    IncrementRedirectedCount();

    return hbm;
}

/***************************************************************************\
* ConvertRedirectionDCs
*
* 11/19/1998        vadimg      created
\***************************************************************************/

VOID ConvertRedirectionDCs(PWND pwnd, HBITMAP hbm)
{
    PDCE pdce;

    GreLockDisplay(gpDispInfo->hDev);

    for (pdce = gpDispInfo->pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {

        if (!(pdce->DCX_flags & DCX_INUSE))
            continue;

        if (GetTopLevelWindow(pdce->pwndOrg) != pwnd)
            continue;

        /*
         * Only normal DCs can be redirected. Redirection on monitor
         * specific DCs is not supported.
         */
        if (pdce->pMonitor != NULL)
            continue;

        SET_OR_CLEAR_FLAG(pdce->DCX_flags, DCX_LAYERED, (hbm != NULL));

        UserVerify(GreSelectRedirectionBitmap(pdce->hdc, hbm));

        InvalidateDce(pdce);
    }

    GreUnlockDisplay(gpDispInfo->hDev);
}

/***************************************************************************\
* UpdateLayeredSprite
*
* 11/19/1998        vadimg      created
\***************************************************************************/

VOID UpdateLayeredSprite(PDCE pdce)
{
    RECT rcBounds;
    PWND pwnd;
    SIZE size;
    POINT pt;
    HBITMAP hbm, hbmOld;

    /*
     * Check to see if any drawing has been done into this DC
     * that should be transferred to the sprite.
     */
    if (!GreGetBounds(pdce->hdc, &rcBounds, 0))
        return;

    pwnd = GetLayeredWindow(pdce->pwndOrg);

    UserAssert(FLayeredOrRedirected(pwnd));

    if (TestWF(pwnd, WEFLAYERED)) {
        hbm = (HBITMAP)_GetProp(pwnd, PROP_LAYER, TRUE);
    
        UserAssert(hbm != NULL);
    
        hbmOld = GreSelectBitmap(ghdcMem, hbm);
    
        size.cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
        size.cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;
    
        pt.x = pt.y = 0;
        GreUpdateSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL, NULL, NULL,
                &size, ghdcMem, &pt, 0, NULL, ULW_DEFAULT_ATTRIBUTES, &rcBounds);
    
        GreSelectBitmap(ghdcMem, hbmOld);
    }

#ifdef REDIRECTION
    if (FWINABLE()) {
        BEGINATOMICCHECK();
        xxxWindowEvent(EVENT_SYSTEM_REDIRECTEDPAINT, pwnd,
                MAKELONG(rcBounds.left, rcBounds.top),
                MAKELONG(rcBounds.right, rcBounds.bottom),
                WEF_ASYNC);
        ENDATOMICCHECK();
    }
#endif // REDIRECTION
}

/***************************************************************************\
* DeleteRedirectionBitmap
*
\***************************************************************************/

VOID DeleteRedirectionBitmap(HBITMAP hbm)
{
    GreMarkDeletableBitmap(hbm);
    GreDeleteObject(hbm);
    DecrementRedirectedCount();
}

/***************************************************************************\
* RemoveRedirectionBitmap
*
* 9/23/1998        vadimg      created
\***************************************************************************/

VOID RemoveRedirectionBitmap(PWND pwnd)
{
    HBITMAP hbm;

    UserAssert(FLayeredOrRedirected(pwnd));

    /*
     * Delete the backing bitmap for this layered window.
     */
    if ((hbm = (HBITMAP)_GetProp(pwnd, PROP_LAYER, TRUE)) == NULL)
        return;

    ConvertRedirectionDCs(pwnd, NULL);
    UserVerify(InternalRemoveProp(pwnd, PROP_LAYER, TRUE));
    DeleteRedirectionBitmap(hbm);
}

/***************************************************************************\
* _SetLayeredWindowAttributes
*
* 9/24/1998        vadimg      created
\***************************************************************************/

BOOL _SetLayeredWindowAttributes(PWND pwnd, COLORREF crKey, BYTE bAlpha,
        DWORD dwFlags)
{
    BOOL bRet;
    BLENDFUNCTION blend;
    HBITMAP hbm;

    if (!TestWF(pwnd, WEFLAYERED)) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING,
                "SetLayeredWindowAttributes: not a sprite %X", pwnd);
        return FALSE;
    }

    if ((hbm = _GetProp(pwnd, PROP_LAYER, TRUE)) == NULL) {
        HBITMAP hbmNew;

        if ((hbmNew = CreateRedirectionBitmap(pwnd)) == NULL) {
            return FALSE;
        }

        ConvertRedirectionDCs(pwnd, hbmNew);
    }

    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.AlphaFormat = 0;
    blend.SourceConstantAlpha = bAlpha;

    dwFlags |= ULW_NEW_ATTRIBUTES; // Notify gdi that these are new attributes

    if (hbm != NULL) {
        HBITMAP hbmOld;
        SIZE size;
        POINT ptSrc = {0,0};
        
        hbmOld = GreSelectBitmap(ghdcMem, hbm);
    
        size.cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
        size.cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;
    
        bRet =  GreUpdateSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL, NULL,
            NULL, &size, ghdcMem, &ptSrc, crKey, &blend, dwFlags, NULL);
    
        GreSelectBitmap(ghdcMem, hbmOld);
    } else {
        bRet =  GreUpdateSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL, NULL,
            NULL, NULL, NULL, NULL, crKey, &blend, dwFlags, NULL);
    }

    return bRet;
}

/***************************************************************************\
* UserRecreateRedirectionBitmap
*
* Called by GDI during mode changes.
*
* 10/6/1998        vadimg      created
\***************************************************************************/

BOOL UserRecreateRedirectionBitmap(HWND hwnd)
{
    PWND pwnd;

    if ((pwnd = RevalidateHwnd(hwnd)) == NULL) {
        RIPMSG0(RIP_WARNING, "UserRecreateRedirectionBitmap: invalid hwnd");
        return FALSE;
    }

    return RecreateRedirectionBitmap(pwnd);
}

/***************************************************************************\
* UserRemoveRedirectionBitmap
*
* Called by GDI during mode changes.
*
* 2/15/1998        OriG        created
\***************************************************************************/

VOID UserRemoveRedirectionBitmap(HWND hwnd)
{
    PWND pwnd;

    if ((pwnd = RevalidateHwnd(hwnd)) == NULL) {
        RIPMSG0(RIP_WARNING, "UserRemoveRedirectionBitmap: invalid hwnd");
        return;
    }

    RemoveRedirectionBitmap(pwnd);

    /*
     * Clear the layered window flag.  This is because GDI is unable to
     * transfer the sprite to the new PDEV, and so if we keep this flag
     * we will have a layered window without a corresponding sprite which
     * can lead to all sort of nasty problems (such as redirection bitmap
     * not getting recreated in subsequent mode changes).
     */
    ClrWF(pwnd, WEFLAYERED);
}

/***************************************************************************\
* RecreateRedirectionBitmap
*
* 10/1/1998        vadimg      created
\***************************************************************************/

BOOL RecreateRedirectionBitmap(PWND pwnd)
{
    HBITMAP hbm, hbmNew, hbmMem, hbmMem2;
    BITMAP bm, bmNew;
    int cx, cy;
    PDCE pdce;

    UserAssert(FLayeredOrRedirected(pwnd));

    /*
     * No need to do anything if this layered window doesn't have
     * a redirection bitmap.
     */
    if ((hbm = (HBITMAP)_GetProp(pwnd, PROP_LAYER, TRUE)) == NULL)
        return FALSE;

    /*
     * Try to create a new redirection bitmap with the new size. If failed,
     * delete the old one and remove it from the window property list.
     */
    if ((hbmNew = CreateRedirectionBitmap(pwnd)) == NULL) {
        RemoveRedirectionBitmap(pwnd);
        return FALSE;
    }

    /*
     * Make sure that the display is locked, so that nobody can be drawing
     * into the redirection DCs while we're switching bitmaps under them.
     */
    UserAssert(GreIsDisplayLocked(gpDispInfo->hDev));

    /*
     * Get the size of the old bitmap to know how much to copy.
     */
    GreExtGetObjectW(hbm, sizeof(bm), (LPSTR)&bm);
    GreExtGetObjectW(hbmNew, sizeof(bmNew), (LPSTR)&bmNew);

    /*
     * Copy the bitmap from the old bitmap into the new one.
     */
    hbmMem = GreSelectBitmap(ghdcMem, hbm);
    hbmMem2 = GreSelectBitmap(ghdcMem2, hbmNew);

    cx = min(bm.bmWidth, bmNew.bmWidth);
    cy = min(bm.bmHeight, bmNew.bmHeight);

    GreBitBlt(ghdcMem2, 0, 0, cx, cy, ghdcMem, 0, 0, SRCCOPY | NOMIRRORBITMAP, 0);

    /*
     * Find layered DCs that are in use corresponding to this window and
     * replace the old redirection bitmap by the new one.
     */
    for (pdce = gpDispInfo->pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {

        if (!(pdce->DCX_flags & DCX_LAYERED) || !(pdce->DCX_flags & DCX_INUSE))
            continue;

        if (GetTopLevelWindow(pdce->pwndOrg) != pwnd)
            continue;

        UserVerify(GreSelectRedirectionBitmap(pdce->hdc, hbmNew));
    }

    GreSelectBitmap(ghdcMem, hbmMem);
    GreSelectBitmap(ghdcMem2, hbmMem2);

    /*
     * Finally, delete the old redirection bitmap.
     */
    DeleteRedirectionBitmap(hbm);

    return TRUE;
}

/***************************************************************************\
* UnsetLayeredWindow
*
* 1/30/1998   vadimg          created
\***************************************************************************/

BOOL UnsetLayeredWindow(PWND pwnd)
{
    HWND hwnd = PtoHq(pwnd);

    /*
     * Remove the layered redirection bitmap.
     */
    RemoveRedirectionBitmap(pwnd);

    /*
     * If the window is still visible, leave the sprite bits on the screen.
     */
    if (TestWF(pwnd, WFVISIBLE)) {
        GreUpdateSprite(gpDispInfo->hDev, hwnd, NULL, NULL, NULL, NULL,
                NULL, NULL, 0, NULL, ULW_NOREPAINT, NULL);
    }

    /*
     * Delete the sprite object.
     */
    if (!GreDeleteSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL)) {
        RIPMSG1(RIP_WARNING, "xxxSetLayeredWindow failed %X", pwnd);
        return FALSE;
    }
    ClrWF(pwnd, WEFLAYERED);

    /*
     * No need to jiggle the mouse when the sprite is removed. As an
     * added bonus that means that zzzInvalidateDCCache won't leave
     * the critical section.
     *
     * Make sure the window gets painted if visible.
     *
     * BUGBUG: should jiggle the mouse. Remove IDC_NOMOUSE when
     * SetFMouseMoved and thus InvalidateDCCache don't leave crit.
     */
    if (TestWF(pwnd, WFVISIBLE)) {
        BEGINATOMICCHECK();
        zzzInvalidateDCCache(pwnd, IDC_DEFAULT | IDC_NOMOUSE);
        ENDATOMICCHECK();
    }
    return TRUE;
}

/***************************************************************************\
* xxxSetLayeredWindow
*
* 12/05/97      vadimg      wrote
\***************************************************************************/

HANDLE xxxSetLayeredWindow(PWND pwnd, BOOL fRepaintBehind)
{
    HANDLE hsprite;
    SIZE size;

    CheckLock(pwnd);

#ifndef CHILD_LAYERING
    if (!FTopLevel(pwnd)) {
        RIPMSG1(RIP_WARNING, "xxxSetLayeredWindow: not top-level %X", pwnd);
        return NULL;
    }
#endif // CHILD_LAYERING

#ifdef REDIRECTION
    /*
     * For now disallow making a layered window redirected and vice versa.
     * This is because we need to come up with a clear way to expose
     * layered attributes for redirected windows and also because we don't
     * know if the redirected bitmap was created for layered redirection
     * or redirection in itself. We would need to store a struct with
     * flags and the bitmap in the window property to keep track of that.
     */
    if (TestWF(pwnd, WEFREDIRECTED))
        return NULL;
#endif // REDIRECTION

#if DBG
    if (TestWF(pwnd, WEFLAYERED)) {
        RIPMSG1(RIP_ERROR, "xxxSetLayeredWindow: already layered %X", pwnd);
    }
#endif
    
    size.cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
    size.cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;

    hsprite = GreCreateSprite(gpDispInfo->hDev, PtoHq(pwnd), &pwnd->rcWindow);
    if (hsprite == NULL) {
        RIPMSG1(RIP_WARNING, "xxxSetLayeredWindow failed %X", pwnd);
        return NULL;
    }

    SetWF(pwnd, WEFLAYERED);
    TrackLayeredZorder(pwnd);

    /*
     * Invalidate the DC cache because changing the sprite status
     * may change the visrgn for some windows.
     *
     * BUGBUG: should jiggle the mouse. Remove IDC_NOMOUSE when
     * SetFMouseMoved and thus InvalidateDCCache don't leave crit.
     */
    BEGINATOMICCHECK();
    zzzInvalidateDCCache(pwnd, IDC_DEFAULT | IDC_NOMOUSE);
    ENDATOMICCHECK();

    /*
     * For the dynamic promotion to a sprite, put the proper bits into
     * the sprite itself by doing ULW with the current screen content
     * and into the background by invalidating windows behind.  There
     * might be some dirty bits if the window is partially obscured, but
     * they will be refreshed as soon as the app calls ULW on its own.
     */
    if (TestWF(pwnd, WFVISIBLE)) {
        if (fRepaintBehind) {
            POINT pt;
    
            pt.x = pwnd->rcWindow.left;
            pt.y = pwnd->rcWindow.top;
    
            _UpdateLayeredWindow(pwnd, gpDispInfo->hdcScreen, &pt, &size,
                    gpDispInfo->hdcScreen, &pt, 0, NULL, ULW_OPAQUE);
        }
    } else {
        /*
         * No need to repaint behind if the window is still invisible.
         */
        fRepaintBehind = FALSE;
    }

    /*
     * This must be done after the DC cache is invalidated, because
     * the xxxUpdateWindows call will redraw some stuff.
     */
    if (fRepaintBehind) {
        HRGN hrgn = GreCreateRectRgnIndirect(&pwnd->rcWindow);
        xxxRedrawWindow(NULL, NULL, hrgn, RDW_INVALIDATE | RDW_FRAME |
                RDW_ERASE | RDW_ALLCHILDREN);
        xxxUpdateWindows(pwnd, hrgn);
        GreDeleteObject(hrgn);
    }
    return hsprite;
}

/***************************************************************************\
* UserVisrgnFromHwnd
*
* Calculate a non-clipchildren visrgn for sprites. This function must be
* called while inside the USER critical section.
*
* 12/05/97      vadimg      wrote
\***************************************************************************/

BOOL UserVisrgnFromHwnd(HRGN *phrgn, HWND hwnd)
{
    PWND pwnd;
    DWORD dwFlags;
    RECT rcWindow;
    BOOL fRet;

    CheckCritIn();

    if ((pwnd = RevalidateHwnd(hwnd)) == NULL) {
        RIPMSG0(RIP_WARNING, "VisrgnFromHwnd: invalid hwnd");
        return FALSE;
    }

    /*
     * So that we don't have to recompute the layered window's visrgn
     * every time the layered window is moved, we compute the visrgn once 
     * as if the layered window covered the entire screen.  GDI will 
     * automatically intersect with this region whenever the sprite moves.
     */
    rcWindow = pwnd->rcWindow;
    pwnd->rcWindow = gpDispInfo->rcScreen;

    /*
     * Since we use DCX_WINDOW, only rcWindow needs to be faked and saved.
     */
    dwFlags = DCX_WINDOW;
    if (TestWF(pwnd, WFCLIPSIBLINGS))
        dwFlags |= DCX_CLIPSIBLINGS;

    fRet = CalcVisRgn(phrgn, pwnd, pwnd, dwFlags);

    pwnd->rcWindow = rcWindow;

    return fRet;
}

/***************************************************************************\
* SetRectRelative
\***************************************************************************/

void SetRectRelative(PRECT prc, int dx, int dy, int dcx, int dcy)
{
    prc->left += dx;
    prc->top += dy;
    prc->right += (dx + dcx);
    prc->bottom += (dy + dcy);
}

/***************************************************************************\
* xxxUpdateLayeredWindow
*
* 1/20/1998   vadimg          created
\***************************************************************************/

BOOL _UpdateLayeredWindow(
    PWND pwnd, 
    HDC hdcDst,
    POINT *pptDst,
    SIZE *psize,
    HDC hdcSrc,
    POINT *pptSrc,
    COLORREF crKey,
    BLENDFUNCTION *pblend,
    DWORD dwFlags)
{
    int dx, dy, dcx, dcy;
    BOOL fMove = FALSE, fSize = FALSE;

    /*
     * Verify that we're called with a real layered window.
     */
    if (!TestWF(pwnd, WEFLAYERED) ||
            _GetProp(pwnd, PROP_LAYER, TRUE) != NULL) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING,
                "_UpdateLayeredWindow: can't call on window %X", pwnd);
        return FALSE;
    }

    if (!GreUpdateSprite(gpDispInfo->hDev, PtoHq(pwnd), NULL, hdcDst, pptDst,
            psize, hdcSrc, pptSrc, crKey, pblend, dwFlags, NULL)) {
        RIPMSG1(RIP_WARNING, "_UpdateLayeredWindow: !UpdateSprite %X", pwnd);
        return FALSE;
    }

    /*
     * Figure out relative adjustments in position and size.
     */
    if (pptDst != NULL) {
        dx = pptDst->x - pwnd->rcWindow.left;
        dy = pptDst->y - pwnd->rcWindow.top;
        if (dx != 0 || dy != 0) {
            fMove = TRUE;
        }
    } else {
        dx = 0;
        dy = 0;
    }
    if (psize != NULL) {
        dcx = psize->cx - (pwnd->rcWindow.right - pwnd->rcWindow.left);
        dcy = psize->cy - (pwnd->rcWindow.bottom - pwnd->rcWindow.top);
        if (dcx != 0 || dcy != 0) {
            fSize = TRUE;
        }
    } else {
        dcx = 0;
        dcy = 0;
    }

    if (fMove || fSize) {
        /*
         * Adjust the client rect position and size relative to 
         * the window rect.
         */
        SetRectRelative(&pwnd->rcWindow, dx, dy, dcx, dcy);
        SetRectRelative(&pwnd->rcClient, dx, dy, dcx, dcy);

        /*
         * Since the client rect could be smaller than the window
         * rect make sure the client rect doesn't underflow!
         */
        if ((dcx < 0) && (pwnd->rcClient.left < pwnd->rcWindow.left)) {
            pwnd->rcClient.left = pwnd->rcWindow.left;
            pwnd->rcClient.right = pwnd->rcWindow.left;
        }
        if ((dcy < 0) && (pwnd->rcClient.top < pwnd->rcWindow.top)) {
            pwnd->rcClient.top = pwnd->rcWindow.top;
            pwnd->rcClient.bottom = pwnd->rcWindow.top;
        }
       /*
        * BUGBUG: should jiggle the mouse, do this when SetFMouseMoved 
        * doesn't leave crit.
        * 
        * SetFMouseMoved();
        */
    }

    return TRUE;
}

/***************************************************************************\
* DeleteFadeSprite
\***************************************************************************/

PWND DeleteFadeSprite(void)
{
    PWND pwnd = NULL;

    if (gfade.dwFlags & FADE_WINDOW) {
        if ((pwnd = RevalidateHwnd(gfade.hsprite)) != NULL) {
            if (TestWF(pwnd, WEFLAYERED)) {
                UnsetLayeredWindow(pwnd);
            }
        } else {
            RIPMSG0(RIP_WARNING, "DeleteFadeSprite: hwnd no longer valid");
        }
    } else {
        GreDeleteSprite(gpDispInfo->hDev, NULL, gfade.hsprite);
    }
    gfade.hsprite = NULL;
    return pwnd;
}

/***************************************************************************\
* UpdateFade
*
* 2/16/1998   vadimg          created
\***************************************************************************/

void UpdateFade(POINT *pptDst, SIZE *psize, HDC hdcSrc, POINT *pptSrc, 
        BLENDFUNCTION *pblend)
{
    PWND pwnd;

    if (gfade.dwFlags & FADE_WINDOW) {
        if ((pwnd = RevalidateHwnd(gfade.hsprite)) != NULL) {
            _UpdateLayeredWindow(pwnd, NULL, pptDst, psize, hdcSrc, 
                     pptSrc, 0, pblend, ULW_ALPHA);
        }
    } else {
        GreUpdateSprite(gpDispInfo->hDev, NULL, gfade.hsprite, NULL, 
                pptDst, psize, hdcSrc, pptSrc, 0, pblend, ULW_ALPHA, NULL);
    }
}

/***************************************************************************\
* CreateFade
*
* 2/5/1998   vadimg          created
\***************************************************************************/

HDC CreateFade(PWND pwnd, RECT *prc, DWORD dwTime, DWORD dwFlags)
{
    SIZE size;

    /*
     * Bail if there is a fade animation going on already.
     */
    if (gfade.hbm != NULL) {
        RIPMSG0(RIP_WARNING, "CreateFade: failed, fade not available");
        return NULL;
    }

    /*
     * Create a cached compatible DC.
     */
    if (gfade.hdc == NULL) {
        gfade.hdc = GreCreateCompatibleDC(gpDispInfo->hdcScreen);
        if (gfade.hdc == NULL) {
            return NULL;
        }
    }

    /*
     * A windowed fade must have window position and size, so 
     * prc passed in is disregarded.
     */
    UserAssert((pwnd == NULL) || (prc == NULL));

    if (pwnd != NULL) {
        prc = &pwnd->rcWindow;
    }

    size.cx = prc->right - prc->left;
    size.cy = prc->bottom - prc->top;

    if (pwnd == NULL) {
        gfade.hsprite = GreCreateSprite(gpDispInfo->hDev, NULL, prc);
    } else {
        gfade.dwFlags |= FADE_WINDOW;
        gfade.hsprite = HWq(pwnd);

        BEGINATOMICCHECK();
        xxxSetLayeredWindow(pwnd, FALSE);
        ENDATOMICCHECK();
    }

    if (gfade.hsprite == NULL)
        return FALSE;

    /*
     * Create a compatible bitmap for this size animation.
     */
    gfade.hbm = GreCreateCompatibleBitmap(gpDispInfo->hdcScreen, size.cx, size.cy);
    if (gfade.hbm == NULL) {
        DeleteFadeSprite();
        return NULL;
    }

    GreSelectBitmap(gfade.hdc, gfade.hbm);

    /*
     * Since this isn't necessarily the first animation and the hdc could
     * be set to public, make sure the owner is the current process. This
     * way this process will be able to draw into it.
     */
    GreSetDCOwner(gfade.hdc, OBJECT_OWNER_CURRENT);

    /*
     * Initialize all other fade animation data.
     */
    gfade.ptDst.x = prc->left;
    gfade.ptDst.y = prc->top;
    gfade.size.cx = size.cx;
    gfade.size.cy = size.cy;
    gfade.dwTime = dwTime;
    gfade.dwFlags |= dwFlags;

    return gfade.hdc;
}

/***************************************************************************\
* ShowFade
*
* GDI says that for alpha fade-out it's more efficient to do the first 
* show as opaque alpha instead of using ULW_OPAQUE.
\***************************************************************************/

#define ALPHASTART 40

void ShowFade(void)
{
    BLENDFUNCTION blend;
    POINT ptSrc;
    BOOL fShow;

    UserAssert(gfade.hdc != NULL);
    UserAssert(gfade.hbm != NULL);

    if (gfade.dwFlags & FADE_SHOWN)
        return;
    
    fShow = (gfade.dwFlags & FADE_SHOW);
    ptSrc.x = ptSrc.y = 0;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.AlphaFormat = 0;
    blend.SourceConstantAlpha = fShow ? ALPHASTART : (255 - ALPHASTART);
    UpdateFade(&gfade.ptDst, &gfade.size, gfade.hdc, &ptSrc, &blend);

    gfade.dwFlags |= FADE_SHOWN;
}

/***************************************************************************\
* StartFade
*
* 2/5/1998   vadimg          created
\***************************************************************************/

void StartFade(void)
{
    DWORD dwElapsed;

    UserAssert(gfade.hdc != NULL);
    UserAssert(gfade.hbm != NULL);

    /*
     * Set dc and bitmap to public so the desktop thread can use them.
     */
    GreSetDCOwner(gfade.hdc, OBJECT_OWNER_PUBLIC);
    GreSetBitmapOwner(gfade.hbm, OBJECT_OWNER_PUBLIC);

    /*
     * If it's not already shown, do the initial update that makes copy of
     * the source. All other updates will only need to change the alpha value.
     */
    ShowFade();

    /*
     * Get the start time for the fade animation.
     */
    dwElapsed = (gfade.dwTime * ALPHASTART + 255) / 255;
    gfade.dwStart = NtGetTickCount() - dwElapsed;

    /*
     * Set the timer in the desktop thread. This will insure that the
     * animation is smooth and won't get stuck if the current thread hangs.
     */
    InternalSetTimer(gTermIO.spwndDesktopOwner, IDSYS_FADE, 10,
            xxxSystemTimerProc, TMRF_SYSTEM | TMRF_PTIWINDOW);
}

/***************************************************************************\
* StopFade
*
* 2/5/1998   vadimg          created
\***************************************************************************/

void StopFade(void)
{
    DWORD dwRop=SRCCOPY;
    PWND pwnd;

    UserAssert(gfade.hdc != NULL);
    UserAssert(gfade.hbm != NULL);

    /*
     * Stop the fade animation timer.
     */
    _KillSystemTimer(gTermIO.spwndDesktopOwner, IDSYS_FADE);

    pwnd = DeleteFadeSprite();

    /*
     * If showing and the animation isn't completed, blt the last frame.
     */
    if (!(gfade.dwFlags & FADE_COMPLETED) && (gfade.dwFlags & FADE_SHOW)) {
        int x, y;
        HDC hdc;

        /*
         * For a windowed fade, make sure we observe the current visrgn.
         */
        if (pwnd != NULL) {
            hdc = _GetDCEx(pwnd, NULL, DCX_WINDOW | DCX_CACHE);
            x = 0;
            y = 0;
        } else {
            hdc = gpDispInfo->hdcScreen;
            x = gfade.ptDst.x;
            y = gfade.ptDst.y;
        }
        
#ifdef USE_MIRRORING
        /*
         * If the destination DC is RTL mirrored, then BitBlt call should mirror the
         * content, since we want the menu to preserve it text (i.e. not to
         * be flipped). [samera]
         */
        if (GreGetLayout(hdc) & LAYOUT_RTL) {
            dwRop |= NOMIRRORBITMAP;
        }
#endif

        GreBitBlt(hdc, x, y, gfade.size.cx, gfade.size.cy, gfade.hdc, 0, 0, dwRop, 0);
        _ReleaseDC(hdc);
    }

    /*
     * Clean up the animation data.
     */
    GreSelectBitmap(gfade.hdc, GreGetStockObject(PRIV_STOCK_BITMAP));
    GreDeleteObject(gfade.hbm);

    gfade.hbm = NULL;
    gfade.dwFlags = 0;
}

/***************************************************************************\
* AnimateFade
*
* 2/5/1998   vadimg          created
\***************************************************************************/

void AnimateFade(void)
{
    DWORD dwTimeElapsed;
    BLENDFUNCTION blend;
    BYTE bAlpha;
    BOOL fShow;

    UserAssert(gfade.hdc != NULL);
    UserAssert(gfade.hbm != NULL);

    dwTimeElapsed = NtGetTickCount() - gfade.dwStart;

    /*
     * If exceeding the allowed time, stop the animation now.
     */
    if (dwTimeElapsed > gfade.dwTime) {
        StopFade();
        return;
    }

    fShow = (gfade.dwFlags & FADE_SHOW);

    /*
     * Calculate new alpha value based on time elapsed.
     */
    if (fShow) {
        bAlpha = (BYTE)((255 * dwTimeElapsed) / gfade.dwTime);
    } else {
        bAlpha = (BYTE)(255 * (gfade.dwTime - dwTimeElapsed) / gfade.dwTime);
    }

    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.AlphaFormat = 0;
    blend.SourceConstantAlpha = bAlpha;
    UpdateFade(NULL, NULL, NULL, NULL, &blend);

    /*
     * Check if finished animating the fade.
     */
    if ((fShow && bAlpha == 255) || (!fShow && bAlpha == 0)) {
        gfade.dwFlags |= FADE_COMPLETED;
        StopFade();
    }
}

#ifdef REDIRECTION
#ifndef CHILD_LAYERING
#error You must enable CHILD_LAYERING to use REDIRECTION
#endif // CHILD_LAYERING

/***************************************************************************\
* SetRedirectedWindow
*
* 1/27/99      vadimg      wrote
\***************************************************************************/

BOOL SetRedirectedWindow(PWND pwnd)
{
    HBITMAP hbmNew = NULL;

    /*
     * For now disallow making a layered window redirected and vice versa.
     * This is because we need to come up with a clear way to expose
     * layered attributes for redirected windows and also because we don't
     * know if the redirected bitmap was created for layered redirection
     * or redirection in itself. We would need to store a struct with
     * flags and the bitmap in the window property to keep track of that.
     */
    if (TestWF(pwnd, WEFLAYERED))
        return FALSE;

    if (_GetProp(pwnd, PROP_LAYER, TRUE) == NULL) {
        if ((hbmNew = CreateRedirectionBitmap(pwnd)) == NULL) {
            return FALSE;
        }
    }

    SetWF(pwnd, WEFREDIRECTED);

    if (hbmNew != NULL) {
        ConvertRedirectionDCs(pwnd, hbmNew);
    }

    /*
     * Invalidate the DC cache because changing visual state may
     * change the visrgn for some windows.
     */
    BEGINATOMICCHECK();
    zzzInvalidateDCCache(pwnd, IDC_DEFAULT | IDC_NOMOUSE);
    ENDATOMICCHECK();

    return TRUE;
}

/***************************************************************************\
* UnsetRedirectedWindow
*
* 1/27/1999        vadimg      created
\***************************************************************************/

BOOL UnsetRedirectedWindow(PWND pwnd)
{
    RemoveRedirectionBitmap(pwnd);

    ClrWF(pwnd, WEFREDIRECTED);

    /*
     * Invalidate the DC cache because changing visual state may
     * change the visrgn for some windows.
     */
    BEGINATOMICCHECK();
    zzzInvalidateDCCache(pwnd, IDC_DEFAULT | IDC_NOMOUSE);
    ENDATOMICCHECK();

    return TRUE;
}

#endif // REDIRECTION

#ifdef CHILD_LAYERING

/***************************************************************************\
* GetNextLayeredWindow
*
* Preorder traversal of the window tree to find the next layering window
* below in zorder than pwnd. We need this because sprites are stored in a
* linked list. Note that this algorithm is iterative which is cool!
\***************************************************************************/

PWND GetNextLayeredWindow(PWND pwnd)
{
    PWND pwndStop = PWNDDESKTOP(pwnd);

    while (TRUE) {
        if (pwnd->spwndChild != NULL) {
            pwnd = pwnd->spwndChild;
        } else if (pwnd->spwndNext != NULL) {
            pwnd = pwnd->spwndNext;
        } else {
    
            do {
                pwnd = pwnd->spwndParent;
    
                if (pwnd == pwndStop) {
                    return NULL;
                }
    
            } while (pwnd->spwndNext == NULL);
    
            pwnd = pwnd->spwndNext;
        }
    
        if (TestWF(pwnd, WEFLAYERED)) {
            return pwnd;
        }
    }
}

/***************************************************************************\
* GetLayeredWindow
*
\***************************************************************************/

PWND GetLayeredWindow(PWND pwnd)
{
    while (pwnd != NULL) {
        if (FLayeredOrRedirected(pwnd))
            break;

        pwnd = pwnd->spwndParent;
    }
    return pwnd;
}

#else // CHILD_LAYERING

/***************************************************************************\
* GetLayeredWindow
*
\***************************************************************************/

PWND GetLayeredWindow(PWND pwnd)
{
    pwnd = GetTopLevelWindow(pwnd);

    if (TestWF(pwnd, WEFLAYERED))
        return pwnd;

    return NULL;
}

#endif // CHILD_LAYERING

/***************************************************************************\
* TrackLayeredZorder
*
* Unlike USER, GDI stores sprites from bottom to top.
\***************************************************************************/

void TrackLayeredZorder(PWND pwnd)
{
#ifdef CHILD_LAYERING

    PWND pwndT = GetNextLayeredWindow(pwnd);

#else // CHILD_LAYERING

    PWND pwndT = pwnd->spwndNext;

    while (pwndT != NULL) {

        if (TestWF(pwndT, WEFLAYERED))
            break;

        pwndT = pwndT->spwndNext;
    }

#endif // CHILD_LAYERING

    GreZorderSprite(gpDispInfo->hDev, PtoHq(pwnd), PtoH(pwndT));
}

