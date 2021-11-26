#include <precomp.h>

static const TCHAR szToolDockWndClass[] = TEXT("ImageSoftToolDockWndClass");

typedef struct _TOOLDOCKWND_INIT
{
    PTOOLBAR_DOCKS TbDocks;
    PDOCKBAR_ITEM Item;
} TOOLDOCKWND_INIT, *PTOOLDOCKWND_INIT;

static UINT
TbdCalculateInsertIndex(PTOOLBAR_DOCKS TbDocks,
                        DOCK_POSITION Position,
                        POINT pt)
{
    RECT rcRebar;
    UINT Ret = 0;

    GetWindowRect(TbDocks->hRebar[Position],
                  &rcRebar);

    switch (Position)
    {
        case TOP_DOCK:
        case BOTTOM_DOCK:
            if (pt.y > rcRebar.top + ((rcRebar.bottom - rcRebar.top) / 2))
                Ret = (UINT)-1;
            break;

        case LEFT_DOCK:
        case RIGHT_DOCK:
            if (pt.x > rcRebar.left + ((rcRebar.right - rcRebar.left) / 2))
                Ret = (UINT)-1;
            break;

        default:
            break;
    }

    return Ret;
}

INT
TbdAdjustUpdateClientRect(PTOOLBAR_DOCKS TbDocks,
                          PRECT rcClient)
{
    INT i, DocksVisible = 0;

    for (i = 0; i < DOCKS_COUNT; i++)
    {
        if (TbDocks->hRebar[i] != NULL)
        {
            DocksVisible++;
        }
    }

    if (DocksVisible != 0)
    {
        rcClient->top += TbDocks->rcRebar[TOP_DOCK].bottom;
        rcClient->left += TbDocks->rcRebar[LEFT_DOCK].right;
        rcClient->right -= TbDocks->rcRebar[RIGHT_DOCK].right;
        rcClient->bottom -= TbDocks->rcRebar[BOTTOM_DOCK].bottom;
    }

    TbDocks->rcClient = *rcClient;

    return DocksVisible;
}

HDWP
TbdDeferDocks(HDWP hWinPosInfo,
              PTOOLBAR_DOCKS TbDocks)
{
    LONG cx, cy;
    HDWP hRet = hWinPosInfo;

    cx = TbDocks->rcClient.right - TbDocks->rcClient.left;
    cy = TbDocks->rcClient.bottom - TbDocks->rcClient.top;

    /* Top dock */
    if (TbDocks->hRebar[TOP_DOCK] != NULL)
    {
        hRet = DeferWindowPos(hRet,
                              TbDocks->hRebar[TOP_DOCK],
                              NULL,
                              TbDocks->rcClient.left - TbDocks->rcRebar[LEFT_DOCK].right,
                              TbDocks->rcClient.top - TbDocks->rcRebar[TOP_DOCK].bottom,
                              cx + TbDocks->rcRebar[LEFT_DOCK].right + TbDocks->rcRebar[RIGHT_DOCK].right,
                              TbDocks->rcRebar[TOP_DOCK].bottom,
                              SWP_NOZORDER);
        if (hRet == NULL)
            return NULL;
    }

    /* Left dock */
    if (TbDocks->hRebar[LEFT_DOCK] != NULL)
    {
        hRet = DeferWindowPos(hRet,
                              TbDocks->hRebar[LEFT_DOCK],
                              NULL,
                              TbDocks->rcClient.left - TbDocks->rcRebar[LEFT_DOCK].right,
                              TbDocks->rcClient.top,
                              TbDocks->rcRebar[LEFT_DOCK].right,
                              cy,
                              SWP_NOZORDER);
        if (hRet == NULL)
            return NULL;
    }

    /* Right dock */
    if (TbDocks->hRebar[RIGHT_DOCK] != NULL)
    {
        hRet = DeferWindowPos(hRet,
                              TbDocks->hRebar[RIGHT_DOCK],
                              NULL,
                              TbDocks->rcClient.right,
                              TbDocks->rcClient.top,
                              TbDocks->rcRebar[RIGHT_DOCK].right,
                              cy,
                              SWP_NOZORDER);
        if (hRet == NULL)
            return NULL;
    }

    /* Bottom dock */
    if (TbDocks->hRebar[BOTTOM_DOCK] != NULL)
    {
        hRet = DeferWindowPos(hRet,
                              TbDocks->hRebar[BOTTOM_DOCK],
                              NULL,
                              TbDocks->rcClient.left - TbDocks->rcRebar[LEFT_DOCK].right,
                              TbDocks->rcClient.bottom,
                              cx + TbDocks->rcRebar[LEFT_DOCK].right + TbDocks->rcRebar[RIGHT_DOCK].right,
                              TbDocks->rcRebar[BOTTOM_DOCK].bottom,
                              SWP_NOZORDER);
        if (hRet == NULL)
            return NULL;
    }

    return hRet;
}

static PDOCKBAR_ITEM
TbnDockbarItemFromBandId(PTOOLBAR_DOCKS TbDocks,
                         DOCK_POSITION Position,
                         UINT uBand)
{
    REBARBANDINFO rbi = {0};

    rbi.cbSize = sizeof(rbi);
    rbi.fMask = RBBIM_LPARAM;

    if (SendMessage(TbDocks->hRebar[Position],
                    RB_GETBANDINFO,
                    (WPARAM)uBand,
                    (LPARAM)&rbi))
    {
        return (PDOCKBAR_ITEM)rbi.lParam;
    }

    return NULL;
}

static VOID
TbnRebarChangeSize(PTOOLBAR_DOCKS TbDocks,
                   DOCK_POSITION Position)
{
    LONG cRebar;

    TbDocks->rcRebar[Position].left = 0;
    TbDocks->rcRebar[Position].top = 0;

    cRebar = (LONG)SendMessage(TbDocks->hRebar[Position],
                               RB_GETBARHEIGHT,
                               0,
                               0);

    switch (Position)
    {
        case TOP_DOCK:
        case BOTTOM_DOCK:
            TbDocks->rcRebar[Position].bottom = cRebar;
            break;

        case LEFT_DOCK:
        case RIGHT_DOCK:
            TbDocks->rcRebar[Position].right = cRebar;
            break;

        default:
            break;
    }

    if (TbDocks->ParentResize != NULL)
    {
        RECT rcClient = {0};

        GetClientRect(TbDocks->hParent,
                      &rcClient);

        TbDocks->ParentResize(TbDocks->Context,
                              rcClient.right - rcClient.left,
                              rcClient.bottom - rcClient.top);
    }
}

static VOID
TbnRebarChevronPushed(PTOOLBAR_DOCKS TbDocks,
                      DOCK_POSITION Position,
                      LPNMREBARCHEVRON lpnm)
{
    PDOCKBAR_ITEM Item;

    Item = TbnDockbarItemFromBandId(TbDocks,
                                    Position,
                                    lpnm->uBand);

    if (Item != NULL && Item->Callbacks->ChevronPushed)
    {
        Item->Callbacks->ChevronPushed(TbDocks,
                                       &Item->DockBar,
                                       Item->Context,
                                       Item->hWndClient,
                                       lpnm);
    }
}

static LRESULT
TbnRebarBeginDrag(PTOOLBAR_DOCKS TbDocks,
                  DOCK_POSITION Position,
                  LPNMREBAR lpnmrb)
{
    PDOCKBAR_ITEM Item;

    Item = TbnDockbarItemFromBandId(TbDocks,
                                    Position,
                                    lpnmrb->uBand);

    if (Item != NULL)
    {
        TbDocks->Dragging = Item;
        TbDocks->DraggingBandId = lpnmrb->wID;
        return FALSE;
    }

    return TRUE;
}

static VOID
TbnRebarEndDrag(PTOOLBAR_DOCKS TbDocks,
                DOCK_POSITION Position,
                LPNMREBAR lpnmrb)
{
    PDOCKBAR_ITEM Item;

    Item = TbnDockbarItemFromBandId(TbDocks,
                                    Position,
                                    lpnmrb->uBand);

    if (Item != NULL)
    {
        /* Nothing to do */
    }
}

BOOL
TbdDockBarIdFromClientWindow(PTOOLBAR_DOCKS TbDocks,
                             HWND hWndClient,
                             UINT *Id)
{
    PDOCKBAR_ITEM Item;
    BOOL Ret = FALSE;

    Item = TbDocks->Items;
    while (Item != NULL)
    {
        if (Item->hWndClient == hWndClient)
        {
            *Id = Item->DockBar.BarId;
            Ret = TRUE;
            break;
        }

        Item = Item->Next;
    }

    return Ret;
}

BOOL
TbdHandleNotifications(PTOOLBAR_DOCKS TbDocks,
                       LPNMHDR pnmh,
                       LRESULT *Result)
{
    BOOL Handled = FALSE;

    if (pnmh->hwndFrom != NULL)
    {
        DOCK_POSITION Position;

        for (Position = TOP_DOCK; Position < NO_DOCK; Position++)
        {
            if (pnmh->hwndFrom == TbDocks->hRebar[Position])
            {
                switch (pnmh->code)
                {
                    case RBN_HEIGHTCHANGE:
                    {
                        TbnRebarChangeSize(TbDocks,
                                           Position);
                        break;
                    }

                    case RBN_BEGINDRAG:
                    {
                        *Result = TbnRebarBeginDrag(TbDocks,
                                                    Position,
                                                    (LPNMREBAR)pnmh);
                        break;
                    }

                    case RBN_ENDDRAG:
                    {
                        TbnRebarEndDrag(TbDocks,
                                        Position,
                                        (LPNMREBAR)pnmh);
                        break;
                    }

                    case RBN_CHEVRONPUSHED:
                    {
                        TbnRebarChevronPushed(TbDocks,
                                              Position,
                                              (LPNMREBARCHEVRON)pnmh);
                        break;
                    }
                }

                Handled = TRUE;
                break;
            }
        }
    }

    return Handled;
}

static BOOL
TbdCreateToolbarWnd(PTOOLBAR_DOCKS TbDocks,
                    PDOCKBAR_ITEM Item,
                    DOCK_POSITION PrevPosition,
                    UINT PrevBandIndex,
                    LPREBARBANDINFO rbi,
                    POINT pt,
                    HWND hRebar,
                    UINT uBand,
                    BOOL Drag)
{
    LPCTSTR lpCaption = NULL;
    TOOLDOCKWND_INIT Init;
    HWND hToolbar;

    Init.TbDocks = TbDocks;
    Init.Item = Item;

    if (rbi->fMask & RBBIM_TEXT)
    {
        lpCaption = rbi->lpText;
    }

    Item->Callbacks->DockBand(TbDocks,
                              &Item->DockBar,
                              Item->Context,
                              PrevPosition,
                              NO_DOCK,
                              rbi);

    if (rbi->fMask & RBBIM_CHILD)
        Item->hWndClient = rbi->hwndChild;
    else
        Item->hWndClient = NULL;


    if ((rbi->fMask & (RBBIM_CHILDSIZE | RBBIM_SIZE)) == (RBBIM_CHILDSIZE | RBBIM_SIZE))
    {
        RECT rcWnd;
        static const DWORD dwStyle = WS_POPUPWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_DLGFRAME;
        static const DWORD dwExStyle = WS_EX_TOOLWINDOW;

        rcWnd.left = pt.x - GetSystemMetrics(SM_CXFIXEDFRAME) - (GetSystemMetrics(SM_CYSMCAPTION) / 2);
        rcWnd.top = pt.y + GetSystemMetrics(SM_CYFIXEDFRAME) + (GetSystemMetrics(SM_CYSMCAPTION) / 2);
        rcWnd.right = rcWnd.left + rbi->cx;
        rcWnd.bottom = rcWnd.top + rbi->cyMinChild;

        if (AdjustWindowRectEx(&rcWnd,
                               dwStyle,
                               FALSE,
                               dwExStyle))
        {
            hToolbar = CreateWindowEx(dwExStyle,
                                      szToolDockWndClass,
                                      lpCaption,
                                      dwStyle,
                                      rcWnd.left,
                                      rcWnd.top,
                                      rcWnd.right - rcWnd.left,
                                      rcWnd.bottom - rcWnd.top,
                                      TbDocks->hParent,
                                      NULL,
                                      hInstance,
                                      &Init);
            if (hToolbar != NULL)
            {
                RECT rcClient;

                if (uBand != (UINT)-1)
                {
                    /* delete the band before showing the client window,
                       otherwise deleting the band will cause the client
                       window to be hidden, regardless of whether the band
                       was hidden before being deleted or not */
                    SendMessage(hRebar,
                                RB_DELETEBAND,
                                (WPARAM)uBand,
                                0);
                }

                if (Item->hWndClient != NULL)
                {
                    GetClientRect(hToolbar,
                                  &rcClient);

                    SetParent(Item->hWndClient,
                              hToolbar);

                    SetWindowPos(Item->hWndClient,
                                 NULL,
                                 0,
                                 0,
                                 rcClient.right,
                                 rcClient.bottom,
                                 SWP_NOZORDER);

                    SetWindowPos(Item->hWndClient,
                                 HWND_TOP,
                                 0,
                                 0,
                                 0,
                                 0,
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                }

                SetWindowPos(hToolbar,
                             HWND_TOP,
                             0,
                             0,
                             0,
                             0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

                if (Drag)
                {
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                    SendMessage(hToolbar,
                                WM_NCLBUTTONDOWN,
                                HTCAPTION,
                                MAKELPARAM(pt.x,
                                           pt.y));
                }

                return TRUE;
            }
        }
    }

    return FALSE;
}

static BOOL
TdbInsertToolbar(PTOOLBAR_DOCKS TbDocks,
                 PDOCKBAR_ITEM Item,
                 DOCK_POSITION Position)
{
    LPTSTR lpCaption = NULL;
    REBARBANDINFO rbi = {0};
    BOOL Ret = FALSE;

    rbi.cbSize = sizeof(rbi);
    rbi.fMask = RBBIM_ID | RBBIM_STYLE | RBBIM_LPARAM;
    rbi.wID = Item->DockBar.BarId;
    rbi.fStyle = RBBS_GRIPPERALWAYS;
    rbi.lParam = (LPARAM)Item;

    if (Item->DockBar.DisplayTextId != 0)
    {
        if (AllocAndLoadString(&lpCaption,
                               hInstance,
                               Item->DockBar.DisplayTextId))
        {
            rbi.fMask |= RBBIM_TEXT;
            rbi.lpText = lpCaption;
        }
    }

    if (Item->hWndClient != NULL)
    {
        rbi.fMask |= RBBIM_CHILD;
        rbi.hwndChild = Item->hWndClient;
    }

    switch (Item->DockBar.Position)
    {
        case NO_DOCK:
        {
            POINT pt = {0};

            /* FIXME - calculate size */
            Ret = TbdCreateToolbarWnd(TbDocks,
                                      Item,
                                      Item->DockBar.Position,
                                      (UINT)-1,
                                      &rbi,
                                      pt,
                                      NULL,
                                      (UINT)-1,
                                      FALSE);
            break;
        }

        default:
        {
            UINT Index = -1;
            BOOL AddBand = TRUE;

            if (Item->Callbacks->InsertBand != NULL)
            {
                AddBand = Item->Callbacks->InsertBand(TbDocks,
                                                      &Item->DockBar,
                                                      Item->Context,
                                                      &Index,
                                                      &rbi);
            }

            if (AddBand)
            {
                Item->Callbacks->DockBand(TbDocks,
                                          &Item->DockBar,
                                          Item->Context,
                                          NO_DOCK,
                                          Item->DockBar.Position,
                                          &rbi);

                if (rbi.fMask & RBBIM_CHILD)
                    Item->hWndClient = rbi.hwndChild;
                else
                    Item->hWndClient = NULL;

                Ret = SendMessage(TbDocks->hRebar[Position],
                                  RB_INSERTBAND,
                                  (WPARAM)Index,
                                  (LPARAM)&rbi) != 0;
                if (Ret)
                {
                    Item->PrevDock = Position;
                    Item->PrevBandIndex = (UINT)SendMessage(TbDocks->hRebar[Position],
                                                            RB_IDTOINDEX,
                                                            (WPARAM)Item->DockBar.BarId,
                                                            0);
                }
            }

            break;
        }
    }

    if (lpCaption != NULL)
    {
        LocalFree((HLOCAL)lpCaption);
    }

    return Ret;
}

BOOL
TbdAddToolbar(PTOOLBAR_DOCKS TbDocks,
              const DOCKBAR *Dockbar,
              PVOID Context,
              const DOCKBAR_ITEM_CALLBACKS *Callbacks)
{
    PDOCKBAR_ITEM Item;
    HWND hRebar;

    hRebar = TbDocks->hRebar[Dockbar->Position];
    if (hRebar != NULL)
    {
        Item = HeapAlloc(ProcessHeap,
                         0,
                         sizeof(DOCKBAR_ITEM));
        if (Item != NULL)
        {
            /* Initialize the item */
            Item->DockBar = *Dockbar;
            Item->Context = Context;
            Item->hWndTool = NULL;
            Item->PrevDock = Dockbar->Position;

            Item->Callbacks = Callbacks;

            /* Create the client control */
            if (Callbacks->CreateClient != NULL &&
                !Callbacks->CreateClient(TbDocks,
                                         &Item->DockBar,
                                         Context,
                                         hRebar,
                                         &Item->hWndClient))
            {
                HeapFree(ProcessHeap,
                         0,
                         Item);

                return FALSE;
            }

            /* Insert the item into the list */
            Item->Next = TbDocks->Items;
            TbDocks->Items = Item;

            return TdbInsertToolbar(TbDocks,
                                    Item,
                                    Dockbar->Position);
        }
    }

    return FALSE;
}

#define GWLP_TBDOCKS    0
#define GWLP_DOCKITEM   (GWLP_TBDOCKS + sizeof(PTOOLBAR_DOCKS))
#define TD_EXTRA_BYTES  (GWLP_DOCKITEM + sizeof(PDOCKBAR_ITEM))

static LRESULT CALLBACK
ToolDockWndProc(HWND hwnd,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    PTOOLBAR_DOCKS TbDocks;
    PDOCKBAR_ITEM Item;
    LRESULT Ret = 0;

    /* Get the window context */
    TbDocks = (PTOOLBAR_DOCKS)GetWindowLongPtr(hwnd,
                                               GWLP_TBDOCKS);
    Item = (PDOCKBAR_ITEM)GetWindowLongPtr(hwnd,
                                           GWLP_DOCKITEM);

    if ((TbDocks == NULL || Item == NULL) && uMsg != WM_CREATE)
    {
        goto HandleDefaultMessage;
    }

    switch (uMsg)
    {
        case WM_NCACTIVATE:
        {
            TbdHandleActivation(TbDocks,
                                hwnd,
                                &wParam,
                                &lParam);
            goto HandleDefaultMessage;
        }

        case WM_CREATE:
        {
            TbDocks = ((PTOOLDOCKWND_INIT)(((LPCREATESTRUCT)lParam)->lpCreateParams))->TbDocks;
            Item = ((PTOOLDOCKWND_INIT)(((LPCREATESTRUCT)lParam)->lpCreateParams))->Item;
            Item->hWndTool = hwnd;

            SetWindowLongPtr(hwnd,
                             GWLP_TBDOCKS,
                             (LONG_PTR)TbDocks);
            SetWindowLongPtr(hwnd,
                             GWLP_DOCKITEM,
                             (LONG_PTR)Item);

            Ret = TRUE;
            break;
        }

        case WM_DESTROY:
        {
            Item->hWndTool = NULL;

            SetWindowLongPtr(hwnd,
                             GWLP_USERDATA,
                             0);
            SetWindowLongPtr(hwnd,
                             GWLP_DOCKITEM,
                             0);
            break;
        }

        default:
        {
HandleDefaultMessage:
            Ret = DefWindowProc(hwnd,
                                uMsg,
                                wParam,
                                lParam);
            break;
        }
    }

    return Ret;
}

static LRESULT CALLBACK
RebarSubclassProc(HWND hWnd,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam,
                  UINT_PTR uIdSubclass,
                  DWORD_PTR dwRefData)
{
    LRESULT Ret;

    Ret = DefSubclassProc(hWnd,
                          uMsg,
                          wParam,
                          lParam);

    if (uMsg == WM_MOUSEMOVE && (wParam & MK_LBUTTON))
    {
        DOCK_POSITION Position, DragTo = NO_DOCK;
        RECT rcClient;
        POINT pt;
        PTOOLBAR_DOCKS TbDocks = (PTOOLBAR_DOCKS)dwRefData;
        SIZE szTearOff;

        szTearOff.cx = GetSystemMetrics(SM_CXCURSOR);
        szTearOff.cy = GetSystemMetrics(SM_CYCURSOR);

        /*
         * Check if we're dragging and if it's time to remove the band
         */
        if (TbDocks->Dragging != NULL && GetCapture() == hWnd)
        {
            GetClientRect(hWnd,
                          &rcClient);
            InflateRect(&rcClient,
                        szTearOff.cx,
                        szTearOff.cy);

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            if (!PtInRect(&rcClient,
                          pt))
            {
                REBARBANDINFO rbi;
                UINT uBand;
                RECT rc;

                /* Save all rebar band information, don't query RBBIM_HEADERSIZE because it
                   seems to cause problems with toolbars*/
                rbi.cbSize = sizeof(rbi);
                rbi.fMask = RBBIM_BACKGROUND | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_COLORS |
                    RBBIM_IDEALSIZE | RBBIM_ID | RBBIM_IMAGE | RBBIM_LPARAM | RBBIM_SIZE |
                    RBBIM_STYLE | RBBIM_TEXT;
                rbi.lpText = TbDocks->szTempText;
                rbi.cch = sizeof(TbDocks->szTempText);

                uBand = (UINT)SendMessage(hWnd,
                                          RB_IDTOINDEX,
                                          (WPARAM)TbDocks->DraggingBandId,
                                          0);

                if (uBand != (UINT)-1 &&
                    SendMessage(hWnd,
                                RB_GETBANDINFO,
                                (WPARAM)uBand,
                                (LPARAM)&rbi))
                {
                    MapWindowPoints(hWnd,
                                    HWND_DESKTOP,
                                    &pt,
                                    1);

                    /* Check if the user is trying to drag it into another dock */
                    for (Position = TOP_DOCK; Position < NO_DOCK; Position++)
                    {
                        if (TbDocks->hRebar[Position] != NULL &&
                            TbDocks->hRebar[Position] != hWnd &&
                            GetWindowRect(TbDocks->hRebar[Position],
                                          &rc))
                        {
                            InflateRect(&rc,
                                        szTearOff.cx,
                                        szTearOff.cy);

                            if (PtInRect(&rc,
                                         pt))
                            {
                                DragTo = Position;
                                break;
                            }
                        }
                    }

                    /* Get the current dock */
                    for (Position = TOP_DOCK; Position < NO_DOCK; Position++)
                    {
                        if (TbDocks->hRebar[Position] == hWnd)
                        {
                            break;
                        }
                    }

                    ReleaseCapture();

                    if (SendMessage(hWnd,
                                    RB_SHOWBAND,
                                    (WPARAM)uBand,
                                    FALSE))
                    {
                        /* Change the parent to the new rebar control */
                        if (TbDocks->Dragging->hWndClient != NULL)
                        {
                            SetWindowPos(TbDocks->Dragging->hWndClient,
                                         NULL,
                                         0,
                                         0,
                                         0,
                                         0,
                                         SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER);

                            SetParent(TbDocks->Dragging->hWndClient,
                                      TbDocks->hRebar[DragTo]);

                            SetWindowPos(TbDocks->Dragging->hWndClient,
                                         NULL,
                                         0,
                                         0,
                                         0,
                                         0,
                                         SWP_NOSIZE | SWP_NOZORDER);
                        }

                        if (DragTo == NO_DOCK)
                        {
                            if (!TbdCreateToolbarWnd(TbDocks,
                                                     TbDocks->Dragging,
                                                     Position,
                                                     uBand,
                                                     &rbi,
                                                     pt,
                                                     hWnd,
                                                     uBand,
                                                     TRUE))
                            {
                                goto MoveFailed;
                            }
                        }
                        else
                        {
                            BOOL Moved = FALSE;

                            /* Remove the band from the current rebar control */
                            if (SendMessage(hWnd,
                                            RB_DELETEBAND,
                                            (WPARAM)uBand,
                                            0))
                            {
                                UINT uIndex;

                                /* Calculate where to insert the new bar */
                                uIndex = TbdCalculateInsertIndex(TbDocks,
                                                                 DragTo,
                                                                 pt);

                                SetActiveWindow(TbDocks->hRebar[DragTo]);

                                TbDocks->Dragging->Callbacks->DockBand(TbDocks,
                                                                       &TbDocks->Dragging->DockBar,
                                                                       TbDocks->Dragging->Context,
                                                                       Position,
                                                                       DragTo,
                                                                       &rbi);

                                if (rbi.fMask & RBBIM_CHILD)
                                    TbDocks->Dragging->hWndClient = rbi.hwndChild;
                                else
                                    TbDocks->Dragging->hWndClient = NULL;

                                /* Insert the toolbar into the new rebar */
                                rbi.fMask |= RBBIM_STYLE;
                                rbi.fStyle |= RBBS_HIDDEN;
                                if (SendMessage(TbDocks->hRebar[DragTo],
                                                RB_INSERTBAND,
                                                (WPARAM)uIndex,
                                                (LPARAM)&rbi))
                                {
                                    uBand = (UINT)SendMessage(TbDocks->hRebar[DragTo],
                                                              RB_IDTOINDEX,
                                                              (WPARAM)TbDocks->DraggingBandId,
                                                              0);

                                    SendMessage(TbDocks->hRebar[DragTo],
                                                RB_SHOWBAND,
                                                (WPARAM)uBand,
                                                TRUE);

                                    /* Simulate a mouse click to continue dragging */
                                    if (uBand != (UINT)-1 &&
                                        TbDocks->Dragging->hWndClient != NULL &&
                                        GetWindowRect(TbDocks->Dragging->hWndClient,
                                                      &rc))
                                    {
                                        switch (DragTo)
                                        {
                                            case LEFT_DOCK:
                                            case RIGHT_DOCK:
                                                pt.x = rc.left + ((rc.right - rc.left) / 2);
                                                pt.y = rc.top - 1;
                                                break;

                                            default:
                                                pt.x = rc.left - 1;
                                                pt.y = rc.top + ((rc.bottom - rc.top) / 2);
                                                break;
                                        }

                                        MapWindowPoints(HWND_DESKTOP,
                                                        TbDocks->hRebar[DragTo],
                                                        &pt,
                                                        1);

                                        SetCursor(LoadCursor(NULL, IDC_SIZEALL));

                                        SendMessage(TbDocks->hRebar[DragTo],
                                                    WM_LBUTTONDOWN,
                                                    wParam,
                                                    MAKELPARAM(pt.x,
                                                               pt.y));

                                        Moved = TRUE;
                                    }
                                }
                            }

                            if (!Moved)
                            {
MoveFailed:
                                TbDocks->Dragging = NULL;

                                SendMessage(hWnd,
                                            RB_SHOWBAND,
                                            (WPARAM)uBand,
                                            TRUE);
                            }
                        }
                    }
                }
            }
        }
    }

    return Ret;
}

VOID
TbdHandleEnabling(PTOOLBAR_DOCKS TbDocks,
                  HWND hWnd,
                  BOOL Enable)
{
    PDOCKBAR_ITEM Item;

    Item = TbDocks->Items;
    while (Item != NULL)
    {
        if (Item->hWndTool != NULL &&
            Item->hWndTool != hWnd)
        {
            EnableWindow(Item->hWndTool,
                         Enable);
        }
        Item = Item->Next;
    }
}

VOID
TbdHandleActivation(PTOOLBAR_DOCKS TbDocks,
                    HWND hWnd,
                    WPARAM *wParam,
                    LPARAM *lParam)
{
    BOOL SynchronizeSiblings = TRUE;
    BOOL KeepActive = *(BOOL*)wParam;
    HWND hWndActivate = *(HWND*)lParam;
    PDOCKBAR_ITEM Item;

    Item = TbDocks->Items;
    while (Item != NULL)
    {
        if (Item->hWndTool != NULL &&
            Item->hWndTool == hWndActivate)
        {
            KeepActive = TRUE;
            SynchronizeSiblings = FALSE;
            break;
        }
        Item = Item->Next;
    }

    if (hWndActivate != (HWND)-1)
    {
        if (SynchronizeSiblings)
        {
            Item = TbDocks->Items;
            while (Item != NULL)
            {
                if (Item->hWndTool != NULL &&
                    Item->hWndTool != hWnd &&
                    Item->hWndTool != hWndActivate)
                {
                    SendMessage(Item->hWndTool,
                                WM_NCACTIVATE,
                                (WPARAM)KeepActive,
                                (LPARAM)-1);
                }
                Item = Item->Next;
            }
        }
    }
    else
        *lParam = 0;

    *wParam = (WPARAM)KeepActive;
}

VOID
TbdShowFloatingToolbars(PTOOLBAR_DOCKS TbDocks,
                        BOOL Show)
{
    PDOCKBAR_ITEM Item;

    Item = TbDocks->Items;
    while (Item != NULL)
    {
        if (Item->hWndTool != NULL)
        {
            if ((Show && !IsWindowVisible(Item->hWndTool)) ||
                (!Show && IsWindowVisible(Item->hWndTool)))
            {
                ShowWindow(Item->hWndTool,
                           (Show ? SW_SHOW : SW_HIDE));
            }
        }
        Item = Item->Next;
    }
}

VOID
TbdInitializeDocks(PTOOLBAR_DOCKS TbDocks,
                   HWND hWndParent,
                   PVOID Context,
                   PDOCKBAR_PARENTRESIZE ParentResizeProc)
{
    DWORD rbStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                        CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NOMOVEY | CCS_NOMOVEX  |
                        RBS_VARHEIGHT | RBS_AUTOSIZE;

    DOCK_POSITION Position;

    TbDocks->hParent = hWndParent;
    TbDocks->Context = Context;
    TbDocks->ParentResize = ParentResizeProc;

    for (Position = TOP_DOCK; Position < NO_DOCK; Position++)
    {
        switch (Position)
        {
            case LEFT_DOCK:
            case RIGHT_DOCK:
                rbStyle |= CCS_VERT;
                break;
            default:
                rbStyle &= ~CCS_VERT;
                break;
        }

        TbDocks->hRebar[Position] = CreateWindowEx(WS_EX_TOOLWINDOW,
                                                   REBARCLASSNAME,
                                                   NULL,
                                                   rbStyle,
                                                   0,
                                                   0,
                                                   0,
                                                   0,
                                                   TbDocks->hParent,
                                                   NULL,
                                                   hInstance,
                                                   NULL);

        if (TbDocks->hRebar[Position] != NULL)
        {
            SetWindowSubclass(TbDocks->hRebar[Position],
                              RebarSubclassProc,
                              1,
                              (DWORD_PTR)TbDocks);
        }
    }
}

BOOL
TbdInitImpl(VOID)
{
    WNDCLASSEX wc = {0};

    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ToolDockWndProc;
    wc.cbWndExtra = TD_EXTRA_BYTES;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL,
                            IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = szToolDockWndClass;

    return RegisterClassEx(&wc) != (ATOM)0;
}

VOID
TbdUninitImpl(VOID)
{
    UnregisterClass(szToolDockWndClass,
                    hInstance);
}
