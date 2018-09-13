/*++
 *
 *  Windows NT v5.0 WOW
 *
 *  Copyright (c) 1997, Microsoft Corporation
 *
 *  WIN95.C
 *
 *  WOW32 Hand-coded (as opposed to interpreted) thunks for new-for-Win95
 *        exports.
 *
 *  History:
 *  16 Feb 97 Created davehart
--*/

#include "precomp.h"
#pragma hdrstop

MODNAME(win95.c);


ULONG FASTCALL WU32TileWindows(PVDMFRAME pFrame)
{
    return W32TileOrCascadeWindows(pFrame, TileWindows);
}


ULONG FASTCALL WU32CascadeWindows(PVDMFRAME pFrame)
{
    return W32TileOrCascadeWindows(pFrame, CascadeWindows);
}


ULONG FASTCALL W32TileOrCascadeWindows(PVDMFRAME pFrame, PFNTILECASCADEWINDOWS pfnWin32)
{
    register PCASCADEWINDOWS16 parg16;
    ULONG ul;
    RECT rc;
    PRECT prc;
    HWND ahwnd[8];
    DWORD chwnd;
    HWND16 UNALIGNED *phwnd16;
    HWND *phwnd;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    chwnd = parg16->chwnd;

    if (parg16->lpRect) {
        GETRECT16(parg16->lpRect, &rc);
        prc = &rc;
    } else {
        prc = NULL;
    }

    if (parg16->ahwnd) {
        phwnd = STACKORHEAPALLOC( chwnd * sizeof(HWND), sizeof(ahwnd), ahwnd);
        phwnd16 = VDMPTR(parg16->ahwnd, chwnd * sizeof(HWND16));

        for (ul = 0; ul < chwnd; ul++) {
            phwnd[ul] = HWND32(phwnd16[ul]);
        }

        FREEVDMPTR(phwnd16);
    } else {
        phwnd = NULL;
    }

    ul = (*pfnWin32)(
             HWND32(parg16->hwndParent),
             parg16->wFlags,
             prc,
             chwnd,
             phwnd
             );

    //
    // Memory movement may have occurred due to message activity,
    // so throw away flat pointers to 16-bit memory.
    //

    FREEARGPTR(parg16);

    if (phwnd) {
        STACKORHEAPFREE(phwnd, ahwnd);
    }

    return ul;
}


ULONG FASTCALL WU32DrawAnimatedRects(PVDMFRAME pFrame)
{
    register PDRAWANIMATEDRECTS16 parg16;
    ULONG ul;
    RECT rcFrom, rcTo;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETRECT16(parg16->lprcStart, &rcFrom);
    GETRECT16(parg16->lprcEnd, &rcTo);

    ul = DrawAnimatedRects(
             HWND32(parg16->hwndClip),
             parg16->idAnimation,
             &rcFrom,
             &rcTo
             );

    FREEARGPTR(parg16);

    return ul;
}


ULONG FASTCALL WU32DrawCaption(PVDMFRAME pFrame)
{
    register PDRAWCAPTION16 parg16;
    ULONG ul;
    RECT rc;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETRECT16(parg16->lprc, &rc);

    ul = DrawCaption(
             HWND32(parg16->hwnd),
             HDC32(parg16->hdc),
             &rc,
             parg16->wFlags
             );

    FREEARGPTR(parg16);

    return ul;
}


ULONG FASTCALL WU32DrawEdge(PVDMFRAME pFrame)
{
    return W32DrawEdgeOrFrameControl(pFrame, DrawEdge);
}


ULONG FASTCALL WU32DrawFrameControl(PVDMFRAME pFrame)
{
    return W32DrawEdgeOrFrameControl(pFrame, DrawFrameControl);
}


ULONG FASTCALL W32DrawEdgeOrFrameControl(PVDMFRAME pFrame, PFNDRAWEDGEFRAMECONTROL pfnWin32)
{
    register PDRAWEDGE16 parg16;
    ULONG ul;
    RECT rc;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETRECT16(parg16->lprc, &rc);

    ul = (*pfnWin32)(
             HDC32(parg16->hdc),
             &rc,
             parg16->wEdge,
             parg16->wFlags
             );

    PUTRECT16(parg16->lprc, &rc);

    FREEARGPTR(parg16);

    return ul;
}


ULONG FASTCALL WU32DrawTextEx(PVDMFRAME pFrame)
{
    register PDRAWTEXTEX16 parg16;
    ULONG ul;
    PSZ psz;
    RECT rc;
    DRAWTEXTPARAMS dtp, *pdtp;
    PDRAWTEXTPARAMS16 pdtp16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETVARSTRPTR(parg16->lpchText, parg16->cchText, psz);
    GETRECT16(parg16->lprc, &rc);

    if ( (parg16->lpDTparams)  &&
         (pdtp16 = VDMPTR(parg16->lpDTparams, sizeof(DRAWTEXTPARAMS16))) ) {

        pdtp = &dtp;
        dtp.cbSize = sizeof(dtp);
        dtp.iTabLength = pdtp16->iTabLength;
        dtp.iLeftMargin = pdtp16->iLeftMargin;
        dtp.iRightMargin = pdtp16->iRightMargin;
        dtp.uiLengthDrawn = 0;
    } else {
        pdtp = NULL;
    }

    ul = DrawTextEx(
             HDC32(parg16->hdc),
             psz,
             parg16->cchText,
             &rc,
             parg16->dwDTformat,
             pdtp
             );

    if (pdtp) {
        pdtp16->uiLengthDrawn = (WORD)dtp.uiLengthDrawn;
    }

    FREEVDMPTR(pdtp16);
    FREEVDMPTR(psz);
    FREEARGPTR(parg16);

    return ul;
}


ULONG FASTCALL WU32GetIconInfo(PVDMFRAME pFrame)
{
    register PGETICONINFO16 parg16;
    ULONG ul;
    ICONINFO ii;
    PICONINFO16 pii16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    ul = GetIconInfo(
             HICON32(parg16->hicon),
             &ii
             );

    pii16 = VDMPTR(parg16->lpiconinfo, sizeof(*pii16));
    pii16->fIcon = (BOOL16)ii.fIcon;
    pii16->xHotspot = (INT16)ii.xHotspot;
    pii16->yHotspot = (INT16)ii.yHotspot;
    pii16->hbmMask  = GETHBITMAP16(ii.hbmMask);
    pii16->hbmColor = GETHBITMAP16(ii.hbmColor);
    FREEVDMPTR(pii16);

    FREEARGPTR(pFrame);

    return ul;
}


ULONG FASTCALL WU32GetMenuItemInfo(PVDMFRAME pFrame)
{
    register PGETMENUITEMINFO16 parg16;
    ULONG ul;
    MENUITEMINFO mii;
    PMENUITEMINFO16 pmii16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETVDMPTR(parg16->lpmii, sizeof(*pmii16), pmii16);

    mii.cbSize = sizeof(mii);
    mii.fMask = pmii16->fMask;

    FREEVDMPTR(pmii16);

    ul = GetMenuItemInfo(
             HMENU32(parg16->hmenu),
             parg16->wIndex,
             parg16->fByPosition,
             &mii
             );

    putmenuiteminfo16(parg16->lpmii, &mii);

    FREEARGPTR(pFrame);

    return ul;
}


ULONG FASTCALL WU32InsertMenuItem(PVDMFRAME pFrame)
{
    register PINSERTMENUITEM16 parg16;
    ULONG ul;
    MENUITEMINFO mii;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    getmenuiteminfo16(parg16->lpmii, &mii);

    ul = InsertMenuItem(
             HMENU32(parg16->hmenu),
             parg16->wIndex,
             parg16->fByPosition,
             &mii
             );


    FREEARGPTR(pFrame);

    return ul;
}


ULONG FASTCALL WU32SetMenuItemInfo(PVDMFRAME pFrame)
{
    register PSETMENUITEMINFO16 parg16;
    ULONG ul;
    MENUITEMINFO mii;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    getmenuiteminfo16(parg16->lpmii, &mii);

    ul = SetMenuItemInfo(
             HMENU32(parg16->hmenu),
             parg16->wIndex,
             parg16->fByPosition,
             &mii
             );

    FREEARGPTR(pFrame);

    return ul;
}


ULONG FASTCALL WU32GetMenuItemRect(PVDMFRAME pFrame)
{
    register PGETMENUITEMRECT16 parg16;
    ULONG ul;
    RECT rc;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    ul = GetMenuItemRect(
             HWND32(parg16->hwnd),
             HMENU32(parg16->hmenu),
             parg16->wIndex,
             &rc
             );

    PUTRECT16(parg16->lprcScreen, &rc);

    FREEARGPTR(pFrame);

    return ul;
}


ULONG FASTCALL WU32TrackPopupMenuEx(PVDMFRAME pFrame)
{
    register PTRACKPOPUPMENUEX16 parg16;
    ULONG ul;
    TPMPARAMS tpmp;
    LPTPMPARAMS lptpmp;
    VPRECT16 vprcExclude;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    if (parg16->lpTpm) {
        lptpmp = &tpmp;
        tpmp.cbSize = sizeof(tpmp);
        vprcExclude = parg16->lpTpm + offsetof(TPMPARAMS16, rcExclude);
        GETRECT16(vprcExclude, &tpmp.rcExclude);
    } else {
        lptpmp = NULL;
    }

    ul = TrackPopupMenuEx(
             HMENU32(parg16->hmenu),
             parg16->wFlags,
             parg16->x,
             parg16->y,
             HWND32(parg16->hwndOwner),
             lptpmp
             );

    FREEARGPTR(pFrame);

    return ul;
}


ULONG FASTCALL WG32GetCharacterPlacement(PVDMFRAME pFrame)
{
    register PGETCHARACTERPLACEMENT16 parg16;
    ULONG ul;
    PSZ pszText;
    PGCP_RESULTS16 pgcp16;
    GCP_RESULTS gcp;

    //
    // Thankfully on Win95 the 16-bit GCP_RESULTS structure
    // points to 32-bit ints, so the structure thunking
    // is trivial.
    //

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETPSZPTR(parg16->lpszText, pszText);
    GETVDMPTR(parg16->lpResults, sizeof(*pgcp16), pgcp16);

    gcp.lStructSize = sizeof gcp;
    gcp.nGlyphs = pgcp16->nGlyphs;
    gcp.nMaxFit = pgcp16->nMaxFit;
    GETOPTPTR(pgcp16->lpOutString, 1, gcp.lpOutString);
    GETOPTPTR(pgcp16->lpOrder, 4, gcp.lpOrder);
    GETOPTPTR(pgcp16->lpDx, 4, gcp.lpDx);
    GETOPTPTR(pgcp16->lpCaretPos, 4, gcp.lpCaretPos);
    GETOPTPTR(pgcp16->lpClass, 1, gcp.lpClass);
    GETOPTPTR(pgcp16->lpGlyphs, 4, gcp.lpGlyphs);

    ul = GetCharacterPlacement(
             HDC32(parg16->hdc),
             pszText,
             parg16->wCount,
             parg16->wMaxExtent,
             &gcp,
             parg16->dwFlags
             );

    pgcp16->nGlyphs = (SHORT)gcp.nGlyphs;
    pgcp16->nMaxFit = (SHORT)gcp.nMaxFit;

    FREEARGPTR(pFrame);

    return ul;
}

//
// On Win95, GetProductName returns "Windows 95".
// We'll return "Windows NT" unless something forces us
// to be identical.
//
// Two flavors:  call with cbBuffer == 0 and it returns
// the length required minus 1 (a bug I think).  Call with
// cbBuffer > 0 and it copies as much as possible and returns
// lpBuffer.
//

ULONG FASTCALL WK32GetProductName(PVDMFRAME pFrame)
{
    register PGETPRODUCTNAME16 parg16;
    ULONG ul;
    PSZ pszBuffer;
    static char szProductName[] = "Windows NT";

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    if (0 == parg16->cbBuffer) {
        ul = (sizeof szProductName) - 1;
    } else {
        GETVDMPTR(parg16->lpBuffer, parg16->cbBuffer, pszBuffer);
        WOW32VERIFY(pszBuffer == lstrcpyn(pszBuffer, szProductName, parg16->cbBuffer));
        FREEVDMPTR(pszBuffer);
        ul = parg16->lpBuffer;
    }

    FREEARGPTR(pFrame);

    return ul;
}


typedef struct _tagWOWDRAWSTATECALLBACK {
    VPVOID vpfnCallback;
    LPARAM lparamUser;
} WOWDRAWSTATECALLBACK, *PWOWDRAWSTATECALLBACK;


BOOL CALLBACK WOWDrawStateCallback(HDC hdc, LPARAM lData, WPARAM wData, int cx, int cy)
{
    PWOWDRAWSTATECALLBACK pwds = (PWOWDRAWSTATECALLBACK) lData;
    ULONG ul;
    WORD awCallbackArgs[6];

    awCallbackArgs[0] = (WORD)(SHORT)cy;
    awCallbackArgs[1] = (WORD)(SHORT)cx;
    awCallbackArgs[2] = (WORD)wData;
    awCallbackArgs[3] = LOWORD(pwds->lparamUser);
    awCallbackArgs[4] = HIWORD(pwds->lparamUser);
    awCallbackArgs[5] = GETHDC16(hdc);

    WOWCallback16Ex(
        pwds->vpfnCallback,
        WCB16_PASCAL,
        sizeof awCallbackArgs,
        awCallbackArgs,
        &ul                      // retcode filled into ul
        );

    return LOWORD(ul);
}


ULONG FASTCALL WU32DrawState(PVDMFRAME pFrame)
{
    register PDRAWSTATE16 parg16;
    ULONG ul;
    WOWDRAWSTATECALLBACK wds;
    DRAWSTATEPROC pDrawStateCallback = NULL;
    LPARAM lData;
    HBRUSH hbr;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    switch (parg16->uFlags & DST_TYPEMASK) {

        case DST_COMPLEX:
            if (parg16->pfnCallBack) {
                wds.vpfnCallback = parg16->pfnCallBack;
                wds.lparamUser = parg16->lData;
                lData = (LPARAM) &wds;
                pDrawStateCallback = (DRAWSTATEPROC) WOWDrawStateCallback;
            }
            break;

        case DST_TEXT:
        case DST_PREFIXTEXT:
            lData = (LPARAM) VDMPTR(parg16->lData, parg16->wData);
            break;

        case DST_ICON:
            lData = (LPARAM) HICON32( (WORD) parg16->lData );
            break;

        case DST_BITMAP:
            lData = (LPARAM) HBITMAP32(parg16->lData);
            break;

        default:
            WOW32WARNMSGF(FALSE, ("WOW32: Unknown DST_ code to DrawState %x.\n",
                                  parg16->uFlags & DST_TYPEMASK));
    }

    hbr = (parg16->uFlags & DSS_MONO)
              ? HBRUSH32(parg16->hbrFore)
              : NULL;

    ul = GETBOOL16(DrawState(
                       HDC32(parg16->hdcDraw),
                       hbr,
                       pDrawStateCallback,
                       lData,
                       parg16->wData,
                       parg16->x,
                       parg16->y,
                       parg16->cx,
                       parg16->cy,
                       parg16->uFlags
                       ));

    FREEARGPTR(pFrame);

    return ul;
}


ULONG FASTCALL WU32GetAppVer(PVDMFRAME pFrame)
{
    return ((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_ExpWinVer;
}


ULONG FASTCALL WU32CopyImage(PVDMFRAME pFrame)
{
    register PCOPYIMAGE16 parg16;
    ULONG ul;
    BOOL fIconCursor;   // as opposed to bitmap

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    //
    // NOTE first parameter to Win16 CopyImage is hinstOwner,
    // which isn't a parameter to Win32 CopyImage.  It may
    // be that we'll need to special-case LR_COPYFROMRESOURCE
    // to work correctly.
    //

    fIconCursor = (parg16->wType != IMAGE_BITMAP);

    ul = (ULONG) CopyImage(
                     (fIconCursor)
                         ? HICON32(parg16->hImage)
                         : HBITMAP32(parg16->hImage),
                     parg16->wType,
                     parg16->cxNew,
                     parg16->cyNew,
                     parg16->wFlags
                     );

    ul = (fIconCursor)
             ? GETHICON16(ul)
             : GETHBITMAP16(ul);

    return ul;
}


//
// WowMsgBoxIndirectCallback is called by User32 when a 16-bit app
// calls MessageBoxIndirect and specifies a help callback proc.
// User32 passes the 16:16 callback address to us along with a
// flat pointer to the HELPINFO structure to pass to the callback.
//

VOID FASTCALL WowMsgBoxIndirectCallback(DWORD vpfnCallback, LPHELPINFO lpHelpInfo)
{
    VPVOID vpHelpInfo16;
    LPHELPINFO lpHelpInfo16;

    //
    // As best as I can tell Win95 passes the WIN32 HELPINFO struct back to the
    // 16-bit callback proc (i.e. there is no HELPINFO16).
    //

    // be sure allocation size matches stackfree16() size below
    vpHelpInfo16 = stackalloc16( sizeof(*lpHelpInfo16) );

    GETVDMPTR(vpHelpInfo16, sizeof(*lpHelpInfo16), lpHelpInfo16);
    RtlCopyMemory(lpHelpInfo16, lpHelpInfo, sizeof(*lpHelpInfo16));
    FREEVDMPTR(lpHelpInfo16);

    WOWCallback16(
        vpfnCallback,
        vpHelpInfo16
        );

    if(vpHelpInfo16) {
        stackfree16(vpHelpInfo16, sizeof(*lpHelpInfo16));
    }
}


ULONG FASTCALL WU32MessageBoxIndirect(PVDMFRAME pFrame)
{
    register PMESSAGEBOXINDIRECT16 parg16;
    ULONG ul;
    PMSGBOXPARAMS16 pmbp16;
    MSGBOXPARAMS mbp;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETVDMPTR(parg16->lpmbp, sizeof *pmbp16, pmbp16);

    mbp.cbSize = sizeof mbp;
    mbp.hwndOwner = HWND32(pmbp16->hwndOwner);
    mbp.hInstance = HINSTRES32(pmbp16->hInstance);
    GETPSZIDPTR(pmbp16->lpszText, mbp.lpszText);
    GETPSZIDPTR(pmbp16->lpszCaption, mbp.lpszCaption);
    mbp.dwStyle = pmbp16->dwStyle;
    GETPSZIDPTR(pmbp16->lpszIcon, mbp.lpszIcon);
    mbp.dwContextHelpId = pmbp16->dwContextHelpId;
    if (pmbp16->vpfnMsgBoxCallback) {
        MarkWOWProc(pmbp16->vpfnMsgBoxCallback, mbp.lpfnMsgBoxCallback)
    } else {
        mbp.lpfnMsgBoxCallback = 0;
    }
    mbp.dwLanguageId = pmbp16->dwLanguageId;

    ul = GETINT16( MessageBoxIndirect(&mbp) );

    FREEARGPTR(pFrame);

    return ul;
}


//
// was in wow.it: HGDI  CreateEnhMetaFile(HGDI, PTR, PTR, PTR);
// Using real thunk to ensure Win32 curdir matches Win16.
//
ULONG FASTCALL WG32CreateEnhMetaFile(PVDMFRAME pFrame)
{
    register PCREATEENHMETAFILE16 parg16;
    ULONG ul;
    LPCSTR lpszFile, lpszDescription;
    CONST RECT *prclFrame;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETVDMPTR(parg16->lpszFile, 1, lpszFile);
    // note lpszDescription is really two SZs with extra terminator
    GETVDMPTR(parg16->lpszDescription, 3, lpszDescription);
    // note lprclFrame is a LPRECTL, a Win32 RECT
    GETVDMPTR(parg16->lprclFrame, sizeof(*prclFrame), prclFrame);

    //
    // Make sure the Win32 current directory matches this task's.
    //

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    ul = GETHDC16(CreateEnhMetaFile(
             HDC32(parg16->hdcRef),
             lpszFile,
             prclFrame,
             lpszDescription
             ));

    FREEVDMPTR(prclFrame);
    FREEVDMPTR(lpszDescription);
    FREEVDMPTR(lpszFile);

    return ul;
}
