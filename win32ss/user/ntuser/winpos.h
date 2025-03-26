#pragma once

typedef struct _CVR // Tag Ussw
{
  WINDOWPOS   pos;
  LONG        xClientNew;
  LONG        yClientNew;
  LONG        cxClientNew;
  LONG        cyClientNew;
  RECT        rcBlt;
  LONG        dxBlt;
  LONG        dyBlt;
  UINT        fsRE;
  HRGN        hrgnVisOld;
  PTHREADINFO pti;
  HRGN        hrgnClip;
  HRGN        hrgnInterMonitor;
} CVR, *PCVR;

typedef struct _SMWP
{
  HEAD head;
  UINT bShellNotify:1;
  UINT bHandle:1;
  INT  ccvr;
  INT  ccvrAlloc;
  PCVR acvr;
} SMWP, *PSMWP;

FORCEINLINE BOOL IntPtInWindow(PWND pwnd, INT x, INT y)
{
    if(!RECTL_bPointInRect(&pwnd->rcWindow, x, y))
    {
        return FALSE;
    }

    if(!pwnd->hrgnClip || pwnd->style & WS_MINIMIZE)
    {
        return TRUE;
    }

    return NtGdiPtInRegion(pwnd->hrgnClip, 
                           x - pwnd->rcWindow.left,
                           y - pwnd->rcWindow.top);
}

FORCEINLINE BOOL
IntEqualRect(RECTL *lprc1, RECTL *lprc2)
{
   if (lprc1 == NULL || lprc2 == NULL)
       return FALSE;

   return (lprc1->left  == lprc2->left)  && (lprc1->top    == lprc2->top) &&
          (lprc1->right == lprc2->right) && (lprc1->bottom == lprc2->bottom);
}

BOOL FASTCALL ActivateOtherWindowMin(PWND);
UINT FASTCALL co_WinPosArrangeIconicWindows(PWND parent);
BOOL FASTCALL IntGetClientOrigin(PWND Window, LPPOINT Point);
LRESULT FASTCALL co_WinPosGetNonClientSize(PWND Window, RECTL* WindowRect, RECTL* ClientRect);
UINT FASTCALL co_WinPosGetMinMaxInfo(PWND Window, POINT* MaxSize, POINT* MaxPos, POINT* MinTrack, POINT* MaxTrack);
UINT FASTCALL co_WinPosMinMaximize(PWND WindowObject, UINT ShowFlag, RECTL* NewPos);
BOOLEAN FASTCALL co_WinPosSetWindowPos(PWND Wnd, HWND WndInsertAfter, INT x, INT y, INT cx, INT cy, UINT flags);
BOOLEAN FASTCALL co_WinPosShowWindow(PWND Window, INT Cmd);
void FASTCALL co_WinPosSendSizeMove(PWND Window);
PWND APIENTRY co_WinPosWindowFromPoint(IN PWND ScopeWin, IN POINT *WinPoint, IN OUT USHORT* HitTest, IN BOOL Ignore);
VOID FASTCALL co_WinPosActivateOtherWindow(PWND);
PWND FASTCALL IntRealChildWindowFromPoint(PWND,LONG,LONG);
BOOL FASTCALL IntScreenToClient(PWND,LPPOINT);
BOOL FASTCALL IntClientToScreen(PWND,LPPOINT);
BOOL FASTCALL IntGetWindowRect(PWND,RECTL*);
BOOL UserHasWindowEdge(DWORD,DWORD);
VOID UserGetWindowBorders(DWORD,DWORD,SIZE*,BOOL);

UINT FASTCALL IntGetWindowSnapEdge(PWND Wnd);
VOID FASTCALL co_IntCalculateSnapPosition(PWND Wnd, UINT Edge, OUT RECT *Pos);
VOID FASTCALL co_IntSnapWindow(PWND Wnd, UINT Edge);
VOID FASTCALL IntSetSnapEdge(PWND Wnd, UINT Edge);
VOID FASTCALL IntSetSnapInfo(PWND Wnd, UINT Edge, IN const RECT *Pos OPTIONAL);

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
