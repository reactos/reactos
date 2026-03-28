#pragma once

extern BOOL g_bWindowSnapEnabled;

/* Snap preview animation timing */
#define SNAP_ANIM_STEP_DELAY_MS 10

typedef struct _SNAP_PREVIEW_STATE
{
    BOOL     bVisible;      /* Is the preview currently drawn on screen? */
    RECT     rcCurrent;     /* Currently displayed rect */
    RECT     rcTarget;      /* Final snap target */
    RECT     rcOrigin;      /* Small rect at cursor where animation starts */
    ULONG    dwStartTime;   /* EngGetTickCount32() when snap zone entered */
    UINT     nSnapEdge;     /* HTLEFT/HTRIGHT/HTTOP or HTNOWHERE */
    HBRUSH   hbrFill;       /* Fill brush for translucent preview */
    HBRUSH   hbrBorder;     /* Border brush for the snap outline */
    HDC      hdcBackground; /* Saved screen contents under rcCurrent */
    HBITMAP  hbmBackground;
    HBITMAP  hbmBackgroundOld;
    HDC      hdcOverlay;    /* Scratch surface for alpha-blended fills */
    HBITMAP  hbmOverlay;
    HBITMAP  hbmOverlayOld;
} SNAP_PREVIEW_STATE;

/* Snap preview animation */
VOID SnapPreviewInit(SNAP_PREVIEW_STATE *pState);
BOOL SnapPreviewAdvance(HDC hdc, SNAP_PREVIEW_STATE *pState, const RECT *prcExclude);
VOID SnapPreviewShow(HDC hdc, SNAP_PREVIEW_STATE *pState, UINT nEdge, const RECT *pTargetRect, POINT ptCursor, const RECT *prcExclude);
VOID SnapPreviewHide(HDC hdc, SNAP_PREVIEW_STATE *pState);
VOID SnapPreviewCleanup(HDC hdc, SNAP_PREVIEW_STATE *pState);

/* Snap logic */
UINT FASTCALL IntGetWindowSnapEdge(PWND Wnd);
VOID FASTCALL co_IntCalculateSnapPosition(PWND Wnd, UINT Edge, OUT RECT *Pos);
VOID FASTCALL co_IntSnapWindow(PWND Wnd, UINT Edge);
VOID FASTCALL IntSetSnapEdge(PWND Wnd, UINT Edge);
VOID FASTCALL IntSetSnapInfo(PWND Wnd, UINT Edge, IN const RECT *Pos OPTIONAL);
UINT GetSnapActivationPoint(PWND Wnd, POINT pt);

#define GetSnapSetting(gspvmember) (IsSnapEnabled() ? (gspv.gspvmember) : 0)

FORCEINLINE BOOL
IsSnapEnabled(VOID)
{
    return g_bWindowSnapEnabled;
}

FORCEINLINE VOID
co_IntUnsnapWindow(PWND Wnd)
{
    co_IntSnapWindow(Wnd, HTNOWHERE);
}

FORCEINLINE BOOLEAN
IntIsWindowSnapped(PWND Wnd)
{
    return (Wnd->ExStyle2 & (WS_EX2_VERTICALLYMAXIMIZEDLEFT | WS_EX2_VERTICALLYMAXIMIZEDRIGHT)) != 0;
}

FORCEINLINE BOOLEAN
IntIsSnapAllowedForWindow(PWND Wnd)
{
    /* We want to forbid snapping operations on the TaskBar and on child windows.
     * We use a heuristic for detecting the TaskBar by its typical Style & ExStyle. */
    const UINT style = Wnd->style;
    const UINT tbws = WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    const UINT tbes = WS_EX_TOOLWINDOW;
    BOOLEAN istb = (style & tbws) == tbws && (Wnd->ExStyle & (tbes | WS_EX_APPWINDOW)) == tbes;
    BOOLEAN thickframe = (style & WS_THICKFRAME) && (style & (WS_DLGFRAME | WS_BORDER)) != WS_DLGFRAME;
    return thickframe && !(style & WS_CHILD) && !istb;
}
