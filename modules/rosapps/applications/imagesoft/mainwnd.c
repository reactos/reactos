#include <precomp.h>

static const TCHAR szMainWndClass[] = TEXT("ImageSoftWndClass");

#define ID_MDI_FIRSTCHILD   50000
#define ID_MDI_WINDOWMENU   5
#define NUM_FLT_WND         3

/* toolbar buttons */
TBBUTTON StdButtons[] = {
/*   iBitmap,         idCommand,   fsState,         fsStyle,     bReserved[2], dwData, iString */
    {TBICON_NEW,      ID_NEW,      TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* new */
    {TBICON_OPEN,     ID_OPEN,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* open */
    {TBICON_SAVE,     ID_SAVE,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* save */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

    {TBICON_PRINT,    ID_PRINTPRE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* print */
    {TBICON_PRINTPRE, ID_PRINT,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* print preview */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

    {TBICON_CUT,      ID_CUT,      TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* cut */
    {TBICON_COPY,     ID_COPY,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* copy */
    {TBICON_PASTE,    ID_PASTE,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },   /* paste */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

    {TBICON_UNDO,     ID_UNDO,    TBSTATE_ENABLED, BTNS_BUTTON,  {0}, 0, 0 },   /* undo */
    {TBICON_REDO,     ID_REDO,    TBSTATE_ENABLED, BTNS_BUTTON,  {0}, 0, 0 },   /* redo */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},
};

TBBUTTON TextButtons[] = {
    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

    {TBICON_BOLD,     ID_BOLD,     TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_CHECK, {0}, 0, 0},    /* bold */
    {TBICON_ITALIC,   ID_ITALIC,   TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_CHECK, {0}, 0, 0},    /* italic */
    {TBICON_ULINE,    ID_ULINE,    TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_CHECK, {0}, 0, 0},    /* underline */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

    {TBICON_TXTLEFT,  ID_TXTLEFT,  TBSTATE_ENABLED | TBSTATE_CHECKED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0 },   /* left justified */
    {TBICON_TXTCENTER,ID_TXTCENTER,TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0 },   /* centered */
    {TBICON_TXTRIGHT, ID_TXTRIGHT, TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0 },   /* right justified */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */
};



/* menu hints */
static const MENU_HINT MainMenuHintTable[] = {
    /* File Menu */
    {ID_BLANK,     IDS_HINT_BLANK},
    {ID_NEW,       IDS_HINT_NEW},
    {ID_OPEN,      IDS_HINT_OPEN},
    {ID_CLOSE,     IDS_HINT_CLOSE},
    {ID_CLOSEALL,  IDS_HINT_CLOSEALL},
    {ID_SAVE,      IDS_HINT_SAVE},
    {ID_SAVEAS,    IDS_HINT_SAVEAS},
    {ID_PRINTPRE,  IDS_HINT_PRINTPRE},
    {ID_PRINT,     IDS_HINT_PRINT},
    {ID_PROP,      IDS_HINT_PROP},
    {ID_EXIT,      IDS_HINT_EXIT},

    /* view menu */
    {ID_TOOLS,     IDS_HINT_TOOLS},
    {ID_COLOR,     IDS_HINT_COLORS},
    {ID_HISTORY,   IDS_HINT_HISTORY},
    {ID_STATUSBAR, IDS_HINT_STATUS},

    /* Window Menu */
    {ID_WINDOW_NEXT,      IDS_HINT_NEXT},
    {ID_WINDOW_CASCADE,   IDS_HINT_CASCADE},
    {ID_WINDOW_TILE_HORZ, IDS_HINT_TILE_HORZ},
    {ID_WINDOW_TILE_VERT, IDS_HINT_TILE_VERT},
    {ID_WINDOW_ARRANGE,   IDS_HINT_ARRANGE}
};

static const MENU_HINT SystemMenuHintTable[] = {
    {SC_RESTORE,    IDS_HINT_SYS_RESTORE},
    {SC_MOVE,       IDS_HINT_SYS_MOVE},
    {SC_SIZE,       IDS_HINT_SYS_SIZE},
    {SC_MINIMIZE,   IDS_HINT_SYS_MINIMIZE},
    {SC_MAXIMIZE,   IDS_HINT_SYS_MAXIMIZE},
    {SC_CLOSE,      IDS_HINT_CLOSE},
    {SC_NEXTWINDOW, IDS_HINT_NEXT},
};


/*  Toolbars */
#define ID_TOOLBAR_STANDARD 0
#define ID_TOOLBAR_TEXT     1
static const TCHAR szToolbarStandard[] = TEXT("STANDARD");
static const TCHAR szToolbarText[]     = TEXT("TEXT");


/* Test Toolbar */
#define ID_TOOLBAR_TEST 5
static const TCHAR szToolbarTest[] = TEXT("TEST");

/* Toolbars table */
static const DOCKBAR MainDockBars[] = {
    {ID_TOOLBAR_STANDARD, szToolbarStandard, IDS_TOOLBAR_STANDARD, TOP_DOCK},
    {ID_TOOLBAR_TEST, szToolbarTest, IDS_TOOLBAR_TEST, TOP_DOCK},
    {ID_TOOLBAR_TEXT, szToolbarText, IDS_TOOLBAR_TEXT, TOP_DOCK},
};


static BOOL CALLBACK
MainWndCreateToolbarClient(struct _TOOLBAR_DOCKS *TbDocks,
                           const DOCKBAR *Dockbar,
                           PVOID Context,
                           HWND hParent,
                           HWND *hwnd)
{
    const TBBUTTON *Buttons = NULL;
    UINT NumButtons = 0;
    UINT StartImageRes = 0;
    UINT NumImages = 0;
    HWND hWndClient = NULL;

    UNREFERENCED_PARAMETER(Context);

    switch (Dockbar->BarId)
    {
        case ID_TOOLBAR_STANDARD:
        {
            Buttons = StdButtons;
            NumButtons = ARRAYSIZE(StdButtons);
            StartImageRes = IDB_MAINNEW;
            NumImages = 10;
            break;
        }

        case ID_TOOLBAR_TEXT:
        {
            Buttons = TextButtons;
            NumButtons = ARRAYSIZE(TextButtons);
            StartImageRes = IDB_TEXTBOLD;
            NumImages = 6;
            break;
        }

        case ID_TOOLBAR_TEST:
        {/*
            hWndClient = CreateWindowEx(WS_EX_TOOLWINDOW,
                                        TEXT("BUTTON"),
                                        TEXT("Test Button"),
                                        WS_CHILD | WS_VISIBLE,
                                        0,
                                        0,
                                        150,
                                        25,
                                        hParent,
                                        NULL,
                                        hInstance,
                                        NULL);*/
            break;
        }
    }

    if (Buttons != NULL)
    {
        hWndClient = CreateWindowEx(0,
                                    TOOLBARCLASSNAME,
                                    NULL,
                                    WS_CHILD | WS_CLIPSIBLINGS |
                                        CCS_NOPARENTALIGN | CCS_NOMOVEY | CCS_NORESIZE | CCS_NODIVIDER |
                                        TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                                    0,
                                    0,
                                    0,
                                    0,
                                    hParent,
                                    NULL,
                                    hInstance,
                                    NULL);
        if (hWndClient != NULL)
        {
            HIMAGELIST hImageList;

            SendMessage(hWndClient,
                        TB_SETEXTENDEDSTYLE,
                        0,
                        TBSTYLE_EX_HIDECLIPPEDBUTTONS);

            SendMessage(hWndClient,
                        TB_BUTTONSTRUCTSIZE,
                        sizeof(Buttons[0]),
                        0);

            SendMessage(hWndClient,
                        TB_SETBITMAPSIZE,
                        0,
                        (LPARAM)MAKELONG(TB_BMP_WIDTH, TB_BMP_HEIGHT));

            hImageList = InitImageList(StartImageRes, NumImages);

            ImageList_Destroy((HIMAGELIST)SendMessage(hWndClient,
                                                      TB_SETIMAGELIST,
                                                      0,
                                                      (LPARAM)hImageList));

            SendMessage(hWndClient,
                        TB_ADDBUTTONS,
                        NumButtons,
                        (LPARAM)Buttons);

        }
    }

    switch (Dockbar->BarId)
    {
        case ID_TOOLBAR_TEXT:
        {
            HWND hFontType;
            HWND hFontSize;

            /* font selection combo */
            hFontType = CreateWindowEx(0,
                                       WC_COMBOBOX,
                                       NULL,
                                       WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                                        CBS_DROPDOWN | CBS_SORT | CBS_HASSTRINGS, //| CBS_OWNERDRAWFIXED,
                                       0, 0, 120, 0,
                                       hParent,
                                       NULL,
                                       hInstance,
                                       NULL);

            if (hFontType != NULL)
            {
                MakeFlatCombo(hFontType);

                SetParent(hFontType,
                          hWndClient);

                if (!ToolbarInsertSpaceForControl(hWndClient,
                                                  hFontType,
                                                  0,
                                                  ID_TXTFONTNAME,
                                                  TRUE))
                {
                    DestroyWindow(hFontType);
                }

                    /* Create the list of fonts */
                    FillFontStyleComboList(hFontType);
            }

            /* font size combo */
            hFontSize = CreateWindowEx(0,
                                       WC_COMBOBOX,
                                       NULL,
                                       WS_CHILD | WS_VISIBLE | CBS_DROPDOWN,
                                       0, 0, 50, 0,
                                       hParent,
                                       NULL,
                                       hInstance,
                                       NULL);
            if (hFontSize != NULL)
            {
                MakeFlatCombo(hFontSize);

                SetParent(hFontSize,
                          hWndClient);

                if (!ToolbarInsertSpaceForControl(hWndClient,
                                                  hFontSize,
                                                  1,
                                                  ID_TXTFONTSIZE,
                                                  TRUE))
                {
                    DestroyWindow(hFontSize);
                }

                /* Update the font-size-list */
                FillFontSizeComboList(hFontSize);
            }
            break;
        }
    }

    if (hWndClient != NULL)
    {
        *hwnd = hWndClient;
        return TRUE;
    }

    return FALSE;
}

static BOOL CALLBACK
MainWndDestroyToolbarClient(struct _TOOLBAR_DOCKS *TbDocks,
                            const DOCKBAR *Dockbar,
                            PVOID Context,
                            HWND hwnd)
{
    UNREFERENCED_PARAMETER(TbDocks);
    UNREFERENCED_PARAMETER(Dockbar);
    UNREFERENCED_PARAMETER(Context);

    DestroyWindow(hwnd);
    return TRUE;
}

static BOOL CALLBACK
MainWndToolbarInsertBand(struct _TOOLBAR_DOCKS *TbDocks,
                         const DOCKBAR *Dockbar,
                         PVOID Context,
                         UINT *Index,
                         LPREBARBANDINFO rbi)
{
    switch (rbi->wID)
    {
        case ID_TOOLBAR_TEXT:
        case ID_TOOLBAR_STANDARD:
        {
            SIZE Size;

            if (SendMessage(rbi->hwndChild,
                            TB_GETMAXSIZE,
                            0,
                            (LPARAM)&Size))
            {
                rbi->fStyle |= RBBS_USECHEVRON | RBBS_HIDETITLE;
                rbi->fMask |= RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE;
                rbi->cx = rbi->cxIdeal = Size.cx;
                rbi->cxMinChild = 0;
                rbi->cyMinChild = Size.cy;
            }
            break;
        }

        case ID_TOOLBAR_TEST:
        {
            RECT rcBtn;

            if (GetWindowRect(rbi->hwndChild,
                              &rcBtn))
            {
                rbi->fStyle |= RBBS_HIDETITLE;
                rbi->fMask |= RBBIM_SIZE | RBBIM_CHILDSIZE;
                rbi->cx = rcBtn.right - rcBtn.left;
                rbi->cxMinChild = 0;
                rbi->cyMinChild = rcBtn.bottom - rcBtn.top;
            }
            break;
        }
    }
    return TRUE;
}

static VOID
TbCustomControlChange(HWND hWndToolbar,
                      HWND hWndControl,
                      BOOL Vert)
{
    /* the toolbar changed from horizontal to vertical or vice versa... */
    return;
}

static VOID CALLBACK
MainWndToolbarDockBand(struct _TOOLBAR_DOCKS *TbDocks,
                       const DOCKBAR *Dockbar,
                       PVOID Context,
                       DOCK_POSITION DockFrom,
                       DOCK_POSITION DockTo,
                       LPREBARBANDINFO rbi)
{
    if (rbi->fMask & RBBIM_CHILD && rbi->hwndChild != NULL)
    {
        switch (rbi->wID)
        {
            case ID_TOOLBAR_TEXT:
            case ID_TOOLBAR_STANDARD:
            {
                SIZE Size;
                BOOL Vert;
                DWORD dwStyle = (DWORD)SendMessage(rbi->hwndChild,
                                                   TB_GETSTYLE,
                                                   0,
                                                   0);
                switch (DockTo)
                {
                    case LEFT_DOCK:
                    case RIGHT_DOCK:
                        dwStyle |= CCS_VERT | TBSTYLE_WRAPABLE;
                        Vert = TRUE;
                        break;

                    default:
                        dwStyle &= ~(CCS_VERT | TBSTYLE_WRAPABLE);
                        Vert = FALSE;
                        break;
                }

                SendMessage(rbi->hwndChild,
                            TB_SETSTYLE,
                            0,
                            (LPARAM)dwStyle);

                ToolbarUpdateControlSpaces(rbi->hwndChild,
                                           TbCustomControlChange);

                if (SendMessage(rbi->hwndChild,
                                TB_GETMAXSIZE,
                                0,
                                (LPARAM)&Size))
                {
                    rbi->fMask |= RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE;
                    rbi->cx = rbi->cxIdeal = (Vert ? Size.cy : Size.cx);
                    rbi->cxMinChild = 0;
                    rbi->cyMinChild = (Vert ? Size.cx : Size.cy);
                }
                break;
            }

            case ID_TOOLBAR_TEST:
            {
                if (DockTo == NO_DOCK)
                {
                    rbi->fMask |= RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE;
                    rbi->cx = rbi->cxIdeal = 150;
                    rbi->cxMinChild = 0;
                    rbi->cyMinChild = 40;
                }
                break;
            }
        }
    }
}

static VOID CALLBACK
MainWndToolbarChevronPushed(struct _TOOLBAR_DOCKS *TbDocks,
                            const DOCKBAR *Dockbar,
                            PVOID Context,
                            HWND hwndChild,
                            LPNMREBARCHEVRON lpnm)
{
    switch (lpnm->wID)
    {
        case ID_TOOLBAR_STANDARD:
        {
            MapWindowPoints(lpnm->hdr.hwndFrom,
                            HWND_DESKTOP,
                            (LPPOINT)&lpnm->rc,
                            2);
            /* Create a popup menu for all toolbar icons hidden */
            break;
        }
    }
}

static VOID
MainWndMoveFloatingWindows(PMAIN_WND_INFO Info,
                           PRECT wndOldPos)
{
    RECT wndNewPos, TbRect;
    INT i, xMoved, yMoved;
    PFLT_WND WndArr[NUM_FLT_WND];

    if (GetWindowRect(Info->hSelf,
                      &wndNewPos))
    {

        xMoved = wndNewPos.left - wndOldPos->left;
        yMoved = wndNewPos.top - wndOldPos->top;

        /* store the pointers in an array */
        WndArr[0] = Info->fltTools;
        WndArr[1] = Info->fltColors;
        WndArr[2] = Info->fltHistory;

        for (i = 0; i < NUM_FLT_WND; i++)
        {
            GetWindowRect(WndArr[i]->hSelf,
                          &TbRect);

            WndArr[i]->x = TbRect.left + xMoved;
            WndArr[i]->y = TbRect.top + yMoved;

            MoveWindow(WndArr[i]->hSelf,
                       WndArr[i]->x,
                       WndArr[i]->y,
                       WndArr[i]->Width,
                       WndArr[i]->Height,
                       TRUE);
        }

        CopyMemory(wndOldPos,
                   &wndNewPos,
                   sizeof(wndNewPos));
    }
}


static VOID
MainWndResetFloatingWindows(PMAIN_WND_INFO Info)
{
    RECT rect;

    if (GetWindowRect(Info->hMdiClient,
                      &rect))
    {

        /* tools datum */
        MoveWindow(Info->fltTools->hSelf,
                   rect.left + 5,
                   rect.top + 5,
                   Info->fltTools->Width,
                   Info->fltTools->Height,
                   TRUE);

        /* colors datum */
        MoveWindow(Info->fltColors->hSelf,
                   rect.left + 5,
                   rect.bottom - Info->fltColors->Height - 5,
                   Info->fltColors->Width,
                   Info->fltColors->Height,
                   TRUE);

        /* history datum */
        MoveWindow(Info->fltHistory->hSelf,
                   rect.right - Info->fltHistory->Width - 5,
                   rect.top + 5,
                   Info->fltHistory->Width,
                   Info->fltHistory->Height,
                   TRUE);
    }
}

static VOID
MainWndCreateFloatWindows(PMAIN_WND_INFO Info)
{
    RECT rect;
    HMENU hMenu;
    UINT Res;
    PFLT_WND WndArr[NUM_FLT_WND]; /* temp array for looping */
    INT i;

    Info->fltTools = HeapAlloc(ProcessHeap,
                               HEAP_ZERO_MEMORY,
                               sizeof(FLT_WND));
    Info->fltColors = HeapAlloc(ProcessHeap,
                                HEAP_ZERO_MEMORY,
                                sizeof(FLT_WND));
    Info->fltHistory = HeapAlloc(ProcessHeap,
                                 HEAP_ZERO_MEMORY,
                                 sizeof(FLT_WND));

    /* set window dimensions */
    Info->fltTools->Width    = 53;
    Info->fltTools->Height   = 300;
    Info->fltColors->Width   = 200;
    Info->fltColors->Height  = 200;
    Info->fltHistory->Width  = 150;
    Info->fltHistory->Height = 150;

    if (! GetWindowRect(Info->hMdiClient,
                        &rect))
    {
        return;
    }

    /* Set window datums */
    Info->fltTools->x = rect.left + 5;
    Info->fltTools->y = rect.top + 5;

    Info->fltColors->x = rect.left + 5;
    Info->fltColors->y = rect.bottom - Info->fltColors->Height - 5;

    Info->fltHistory->x = rect.right - Info->fltHistory->Width - 5;
    Info->fltHistory->y = rect.top + 5;

    /* save pointers into array incrementing within the loop*/
    WndArr[0] = Info->fltTools;
    WndArr[1] = Info->fltColors;
    WndArr[2] = Info->fltHistory;

    for (i = 0, Res = IDS_FLT_TOOLS; Res < IDS_FLT_TOOLS + NUM_FLT_WND; Res++, i++)
    {
        if (! AllocAndLoadString(&WndArr[i]->lpName,
                                 hInstance,
                                 Res))
        {
            WndArr[i]->lpName = NULL;
        }

        WndArr[i]->hSelf = CreateWindowEx(WS_EX_TOOLWINDOW,
                                          TEXT("ImageSoftFloatWndClass"),
                                          WndArr[i]->lpName,
                                          WS_POPUPWINDOW | WS_DLGFRAME | WS_VISIBLE,
                                          WndArr[i]->x,
                                          WndArr[i]->y,
                                          WndArr[i]->Width,
                                          WndArr[i]->Height,
                                          Info->hSelf,
                                          NULL,
                                          hInstance,
                                          WndArr[i]);
        ShowWindow(WndArr[i]->hSelf, SW_HIDE);
    }

    hMenu = GetMenu(Info->hSelf);

    if (Info->fltTools->hSelf != NULL)
    {
        if (FloatToolbarCreateToolsGui(Info))
        {
            CheckMenuItem(hMenu,
                          ID_TOOLS,
                          MF_CHECKED);

            ShowHideWindow(Info->fltTools->hSelf);
        }
    }

    if (Info->fltColors->hSelf != NULL)
    {
        if (FloatToolbarCreateColorsGui(Info))
        {

        }
    }

    if (Info->fltHistory->hSelf != NULL)
    {
        if (FloatToolbarCreateHistoryGui(Info))
        {

        }
    }

}

static VOID
MainWndDestroyFloatWindows(PMAIN_WND_INFO Info)
{
    if (Info->fltTools != NULL)
        HeapFree(ProcessHeap, 0, Info->fltTools);

    if (Info->fltColors != NULL)
        HeapFree(ProcessHeap, 0, Info->fltColors);

    if (Info->fltHistory != NULL)
        HeapFree(ProcessHeap, 0, Info->fltHistory);
}



static const DOCKBAR_ITEM_CALLBACKS MainWndDockBarCallbacks = {
    MainWndCreateToolbarClient,
    MainWndDestroyToolbarClient,
    MainWndToolbarInsertBand,
    MainWndToolbarDockBand,
    MainWndToolbarChevronPushed,
};

static VOID
CreateToolbars(PMAIN_WND_INFO Info)
{
    UINT i;

    for (i = 0; i < ARRAYSIZE(MainDockBars); i++)
    {
        /* FIXME - lookup whether to display the toolbar */
        TbdAddToolbar(&Info->ToolDocks,
                      &MainDockBars[i],
                      Info,
                      &MainWndDockBarCallbacks);
    }

    MainWndCreateFloatWindows(Info);
}

static VOID CALLBACK
MainWndResize(PVOID Context,
              LONG cx,
              LONG cy)
{
    RECT rcClient = {0};
    RECT rcStatus = {0};
    HDWP dwp;
    INT DocksVisible;
    PMAIN_WND_INFO Info = (PMAIN_WND_INFO)Context;

    /* Calculate the MDI client rectangle */
    rcClient.right = cx;
    rcClient.bottom = cy;

    if (Info->hStatus != NULL)
    {
        GetWindowRect(Info->hStatus,
                      &rcStatus);
        rcClient.bottom -= (rcStatus.bottom - rcStatus.top);
    }

    /* Adjust the client rect if docked toolbars are visible */
    DocksVisible = TbdAdjustUpdateClientRect(&Info->ToolDocks,
                                             &rcClient);

    dwp = BeginDeferWindowPos(2 + DocksVisible);
    if (dwp != NULL)
    {
        /* Update the toolbar docks */
        if (DocksVisible != 0)
        {
            dwp = TbdDeferDocks(dwp,
                                &Info->ToolDocks);
            if (dwp == NULL)
                return;
        }

        /* Update the MDI client */
        if (Info->hMdiClient != NULL)
        {
            dwp = DeferWindowPos(dwp,
                                 Info->hMdiClient,
                                 NULL,
                                 rcClient.left,
                                 rcClient.top,
                                 rcClient.right - rcClient.left,
                                 rcClient.bottom - rcClient.top,
                                 SWP_NOZORDER);
            if (dwp == NULL)
                return;
        }

        /* Update the status bar */
        if (Info->hStatus != NULL)
        {
            dwp = DeferWindowPos(dwp,
                                 Info->hStatus,
                                 NULL,
                                 0,
                                 cy - (rcStatus.bottom - rcStatus.top),
                                 cx,
                                 rcStatus.bottom - rcStatus.top,
                                 SWP_NOZORDER);
            if (dwp == NULL)
                return;
        }

        EndDeferWindowPos(dwp);
    }
}

static VOID
InitMainWnd(PMAIN_WND_INFO Info)
{
    CLIENTCREATESTRUCT ccs;
    INT statwidths[] = {110, -1};

    /* FIXME - create controls and initialize the application */

    /* create the status bar */
    Info->hStatus = CreateWindowEx(0,
                                   STATUSCLASSNAME,
                                   NULL,
                                   WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | CCS_NOPARENTALIGN | SBARS_SIZEGRIP,
                                   0,
                                   0,
                                   0,
                                   0,
                                   Info->hSelf,
                                   (HMENU)IDC_STATUSBAR,
                                   hInstance,
                                   NULL);

    if (Info->hStatus != NULL)
        SendMessage(Info->hStatus,
                    SB_SETPARTS,
                    ARRAYSIZE(statwidths),
                    (LPARAM)statwidths);

    /* create the MDI client window */
    ccs.hWindowMenu = GetSubMenu(GetMenu(Info->hSelf),
                                 ID_MDI_WINDOWMENU);
    ccs.idFirstChild = ID_MDI_FIRSTCHILD;
    Info->hMdiClient = CreateWindowEx(WS_EX_ACCEPTFILES | WS_EX_CLIENTEDGE,
                                      TEXT("MDICLIENT"),
                                      NULL,
                                      WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VSCROLL | WS_HSCROLL,
                                      0,
                                      0,
                                      0,
                                      0,
                                      Info->hSelf,
                                      NULL,
                                      hInstance,
                                      &ccs);

    TbdInitializeDocks(&Info->ToolDocks,
                       Info->hSelf,
                       Info,
                       MainWndResize);

    CreateToolbars(Info);

    /* initialize file open/save structure */
    FileInitialize(Info->hSelf);
}

static VOID
MainWndCommand(PMAIN_WND_INFO Info,
               WORD CmdId,
               HWND hControl)
{
    static TCHAR szFileName[MAX_PATH];
    static TCHAR szImageName[MAX_PATH];

    UNREFERENCED_PARAMETER(hControl);

    switch (CmdId)
    {
        case ID_NEW:
        {
            MessageBox(NULL, _T("Not yet implemented"), NULL, 0);
        }
        break;

        case ID_OPEN:
        {
            OPEN_IMAGE_EDIT_INFO OpenInfo;

            if (DoOpenFile(Info->hSelf,
                           szFileName,   /* full file path */
                           szImageName)) /* file name */
            {
                OpenInfo.CreateNew = FALSE;

                OpenInfo.Open.lpImagePath = szFileName;
                OpenInfo.lpImageName = szImageName;

                CreateImageEditWindow(Info,
                                      &OpenInfo);

                /* FIXME: move flt wnd's if scroll bars show
                MainWndResetFloatingWindows(Info->hMdiClient); */
            }

        }
        break;

        case ID_TOOLS:
        {
            HMENU hMenu = GetMenu(Info->hSelf);

            if (hMenu != NULL)
            {
                UINT uCheck = MF_CHECKED;

                if (ShowHideWindow(Info->fltTools->hSelf))
                    uCheck = MF_UNCHECKED;

                CheckMenuItem(hMenu,
                              ID_TOOLS,
                              uCheck);
            }
        }
        break;

        case ID_COLOR:
        {
            HMENU hMenu = GetMenu(Info->hSelf);

            if (hMenu != NULL)
            {
                UINT uCheck = MF_CHECKED;

                if (ShowHideWindow(Info->fltColors->hSelf))
                    uCheck = MF_UNCHECKED;

                CheckMenuItem(hMenu,
                              ID_COLOR,
                              uCheck);
            }
        }
        break;

        case ID_HISTORY:
        {
            HMENU hMenu = GetMenu(Info->hSelf);

            if (hMenu != NULL)
            {
                UINT uCheck = MF_CHECKED;

                if (ShowHideWindow(Info->fltHistory->hSelf))
                    uCheck = MF_UNCHECKED;

                CheckMenuItem(hMenu,
                              ID_HISTORY,
                              uCheck);
            }
        }
        break;

        case ID_BRIGHTNESS:
            DialogBoxParam(hInstance,
                           MAKEINTRESOURCE(IDD_BRIGHTNESS),
                           Info->hSelf,
                           BrightnessProc,
                           (LPARAM)Info);
        break;

        case ID_CONTRAST:
            /* FIXME : Create a window for contrast */
        break;

        case ID_BLACKANDWHITE:
        {
            if (Info->ImageEditors)
            {
                DisplayBlackAndWhite(Info->ImageEditors->hSelf,
                                     Info->ImageEditors->hDCMem,
                                     Info->ImageEditors->hBitmap);
            }
        }
        break;

        case ID_INVERTCOLORS:
        {
            if (Info->ImageEditors)
            {
                DisplayInvertedColors(Info->ImageEditors->hSelf,
                                      Info->ImageEditors->hDCMem,
                                      Info->ImageEditors->hBitmap);
            }
        }
        break;

        case ID_BLUR:
        {
            if (Info->ImageEditors)
            {
                DisplayBlur(Info->ImageEditors->hSelf,
                            Info->ImageEditors->hDCMem,
                            Info->ImageEditors->hBitmap);
            }
        }
        break;

        case ID_SHARPEN:
        {
            if (Info->ImageEditors)
            {
                DisplaySharpness(Info->ImageEditors->hSelf,
                                 Info->ImageEditors->hDCMem,
                                 Info->ImageEditors->hBitmap);
            }
        }
        break;

        case ID_EXIT:
            SendMessage(Info->hSelf,
                        WM_CLOSE,
                        0,
                        0);
            break;

        /* Window Menu */
        case ID_WINDOW_TILE_HORZ:
            SendMessage(Info->hMdiClient,
                        WM_MDITILE,
                        MDITILE_HORIZONTAL,
                        0);
            break;

        case ID_WINDOW_TILE_VERT:
            SendMessage(Info->hMdiClient,
                        WM_MDITILE,
                        MDITILE_VERTICAL,
                        0);
            break;

        case ID_WINDOW_CASCADE:
            SendMessage(Info->hMdiClient,
                        WM_MDICASCADE,
                        0,
                        0);
            break;

        case ID_WINDOW_ARRANGE:
            SendMessage(Info->hMdiClient,
                        WM_MDIICONARRANGE,
                        0,
                        0);
            break;

        case ID_WINDOW_NEXT:
            SendMessage(Info->hMdiClient,
                        WM_MDINEXT,
                        0,
                        0);
            break;

        /* Help Menu */
        case ID_ABOUT:
            DialogBox(hInstance,
                      MAKEINTRESOURCE(IDD_ABOUTBOX),
                      Info->hSelf,
                      AboutDialogProc);
            break;
    }
}

static VOID
DestroyMainWnd(PMAIN_WND_INFO Info)
{
    /* FIXME - cleanup allocated resources */

    MainWndDestroyFloatWindows(Info);
}


static VOID
UpdateMainStatusBar(PMAIN_WND_INFO Info)
{
    if (Info->hStatus != NULL)
    {
        SendMessage(Info->hStatus,
                    SB_SIMPLE,
                    (WPARAM)Info->InMenuLoop,
                    0);
    }
}

static BOOL
MainWndMenuHint(PMAIN_WND_INFO Info,
                WORD CmdId,
                const MENU_HINT *HintArray,
                DWORD HintsCount,
                UINT DefHintId)
{
    BOOL Found = FALSE;
    const MENU_HINT *LastHint;
    UINT HintId = DefHintId;

    LastHint = HintArray + HintsCount;
    while (HintArray != LastHint)
    {
        if (HintArray->CmdId == CmdId)
        {
            HintId = HintArray->HintId;
            Found = TRUE;
            break;
        }
        HintArray++;
    }

    StatusBarLoadString(Info->hStatus,
                        SB_SIMPLEID,
                        hInstance,
                        HintId);

    return Found;
}

static LRESULT CALLBACK
MainWndProc(HWND hwnd,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam)
{
    PMAIN_WND_INFO Info;
    LRESULT Ret = 0;
    static BOOL bLBMouseDown = FALSE;
    static RECT wndOldPos;

    /* Get the window context */
    Info = (PMAIN_WND_INFO)GetWindowLongPtr(hwnd,
                                            GWLP_USERDATA);
    if (Info == NULL && uMsg != WM_CREATE)
    {
        goto HandleDefaultMessage;
    }

    switch (uMsg)
    {
        case WM_SIZE:
        {
            MainWndResize(Info,
                          LOWORD(lParam),
                          HIWORD(lParam));
            /* NOTE - do *not* forward this message to DefFrameProc! Otherwise the MDI client
                      will attempt to resize itself */

                /* reposition the floating toolbars */
                if ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED))
                    MainWndResetFloatingWindows(Info);

            break;
        }

        case WM_NCLBUTTONDOWN:
            bLBMouseDown = TRUE;
            DefWindowProc(hwnd,
                          uMsg,
                          wParam,
                          lParam);
        break;

        case WM_NCLBUTTONUP:

            bLBMouseDown = FALSE;
            DefWindowProc(hwnd,
                          uMsg,
                          wParam,
                          lParam);
        break;

        case WM_MOVE:
        {
            /* if the main window is moved, move the toolbars too */
            if (bLBMouseDown)
                MainWndMoveFloatingWindows(Info, &wndOldPos);
        }
        break;

        case WM_NOTIFY:
        {
            UINT BarId;
            LPNMHDR pnmhdr = (LPNMHDR)lParam;
            if (!TbdHandleNotifications(&Info->ToolDocks,
                                        pnmhdr,
                                        &Ret))
            {
                if (TbdDockBarIdFromClientWindow(&Info->ToolDocks,
                                                 pnmhdr->hwndFrom,
                                                 &BarId))
                {
                    switch (BarId)
                    {
                        case ID_TOOLBAR_TEXT:
                            switch (pnmhdr->code)
                            {
                                case TBN_DELETINGBUTTON:
                                {
                                    LPNMTOOLBAR lpnmtb = (LPNMTOOLBAR)lParam;

                                    ToolbarDeleteControlSpace(pnmhdr->hwndFrom,
                                                              &lpnmtb->tbButton);
                                    break;
                                }
                            }
                            break;
                    }
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            MainWndCommand(Info,
                           LOWORD(wParam),
                           (HWND)lParam);
            goto HandleDefaultMessage;
        }

        case WM_MENUSELECT:
        {
            if (Info->hStatus != NULL)
            {
                if (!MainWndMenuHint(Info,
                                     LOWORD(wParam),
                                     MainMenuHintTable,
                                     ARRAYSIZE(MainMenuHintTable),
                                     IDS_HINT_BLANK))
                {
                    MainWndMenuHint(Info,
                                    LOWORD(wParam),
                                    SystemMenuHintTable,
                                    ARRAYSIZE(SystemMenuHintTable),
                                    IDS_HINT_BLANK);
                }
            }
            break;
        }

        case WM_ENTERMENULOOP:
        {
            Info->InMenuLoop = TRUE;
            UpdateMainStatusBar(Info);
            break;
        }

        case WM_EXITMENULOOP:
        {
            Info->InMenuLoop = FALSE;
            UpdateMainStatusBar(Info);
            break;
        }

        case WM_CLOSE:
        {
            DestroyWindow(hwnd);
            break;
        }

        case WM_ENABLE:
        {
            TbdHandleEnabling(&Info->ToolDocks,
                              hwnd,
                              (BOOL)wParam);
            goto HandleDefaultMessage;
        }

        case WM_NCACTIVATE:
        {
            TbdHandleActivation(&Info->ToolDocks,
                                hwnd,
                                &wParam,
                                &lParam);
            goto HandleDefaultMessage;
        }

        case WM_ACTIVATEAPP:
        {
            //TbdShowFloatingToolbars(&Info->ToolDocks,
            //                        (BOOL)wParam);
            goto HandleDefaultMessage;
        }

        case WM_CREATE:
        {
            Info = (PMAIN_WND_INFO)(((LPCREATESTRUCT)lParam)->lpCreateParams);

            /* Initialize the main window context */
            Info->hSelf = hwnd;

            SetWindowLongPtr(hwnd,
                             GWLP_USERDATA,
                             (LONG_PTR)Info);

            InitMainWnd(Info);

            /* Show the window */
            ShowWindow(hwnd,
                       Info->nCmdShow);
            /* get the windows position */
            GetWindowRect(hwnd,
                          &wndOldPos);

            break;
        }

        case WM_DESTROY:
        {
            DestroyMainWnd(Info);

            HeapFree(ProcessHeap,
                     0,
                     Info);
            SetWindowLongPtr(hwnd,
                             GWLP_USERDATA,
                             0);

            /* Break the message queue loop */
            PostQuitMessage(0);
            break;
        }

        default:
        {
HandleDefaultMessage:
            if (Info != NULL && Info->hMdiClient != NULL)
            {
                Ret = DefFrameProc(hwnd,
                                   Info->hMdiClient,
                                   uMsg,
                                   wParam,
                                   lParam);
            }
            else
            {
                Ret = DefWindowProc(hwnd,
                                    uMsg,
                                    wParam,
                                    lParam);
            }
            break;
        }
    }

    return Ret;
}

MDI_EDITOR_TYPE
MainWndGetCurrentEditor(PMAIN_WND_INFO MainWnd,
                        PVOID *Info)
{
    MDI_EDITOR_TYPE EditorType;

    if (MainWnd->ActiveEditor != NULL)
    {
        EditorType = *((PMDI_EDITOR_TYPE)MainWnd->ActiveEditor);
        *Info = MainWnd->ActiveEditor;
    }
    else
    {
        EditorType = metUnknown;
        *Info = NULL;
    }

    return EditorType;
}

VOID
MainWndSwitchEditorContext(PMAIN_WND_INFO Info,
                           HWND hDeactivate,
                           HWND hActivate)
{
    PMDI_EDITOR_TYPE EditorType;

    /* FIXME - optimize light weight switching
               when switching from and to an editor of same type */

    if (hDeactivate != NULL)
    {
        EditorType = (PMDI_EDITOR_TYPE)GetWindowLongPtr(hDeactivate,
                                                        GWLP_USERDATA);
        if (EditorType != NULL)
        {
            switch (*EditorType)
            {
                case metImageEditor:
                    SetImageEditorEnvironment((PEDIT_WND_INFO)EditorType,
                                              FALSE);
                    break;

                default:
                    break;
            }

            Info->ActiveEditor = NULL;
        }
    }

    if (hActivate != NULL)
    {
        EditorType = (PMDI_EDITOR_TYPE)GetWindowLongPtr(hActivate,
                                                        GWLP_USERDATA);
        if (EditorType != NULL)
        {
            Info->ActiveEditor = EditorType;

            switch (*EditorType)
            {
                case metImageEditor:
                    SetImageEditorEnvironment((PEDIT_WND_INFO)EditorType,
                                              TRUE);
                    break;

                default:
                    break;
            }
        }
    }
}

HWND
CreateMainWindow(LPCTSTR lpCaption,
                 int nCmdShow)
{
    PMAIN_WND_INFO Info;
    HWND hMainWnd = NULL;

    Info = HeapAlloc(ProcessHeap,
                     0,
                     sizeof(MAIN_WND_INFO));
    if (Info != NULL)
    {
        ZeroMemory(Info,
                   sizeof(MAIN_WND_INFO));
        Info->nCmdShow = nCmdShow;

        /* FIXME - load the window position from the registry */

        hMainWnd = CreateWindowEx(WS_EX_WINDOWEDGE,
                                  szMainWndClass,
                                  lpCaption,
                                  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  Info);
        if (hMainWnd == NULL)
        {
            HeapFree(ProcessHeap,
                     0,
                     Info);
        }
    }

    return hMainWnd;
}

BOOL
MainWndTranslateMDISysAccel(HWND hwnd,
                            LPMSG lpMsg)
{
    PMAIN_WND_INFO Info;

    /* Get the window context */
    Info = (PMAIN_WND_INFO)GetWindowLongPtr(hwnd,
                                            GWLP_USERDATA);
    if (Info != NULL && Info->hMdiClient != NULL)
    {
        return TranslateMDISysAccel(Info->hMdiClient,
                                    lpMsg);
    }

    return FALSE;
}

BOOL
InitMainWindowImpl(VOID)
{
    WNDCLASSEX wc = {0};

    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance,
                        MAKEINTRESOURCE(IDI_IMAGESOFTICON));
    wc.hCursor = LoadCursor(NULL,
                            IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
    wc.lpszClassName = szMainWndClass;
    wc.hIconSm = (HICON)LoadImage(hInstance,
                                  MAKEINTRESOURCE(IDI_IMAGESOFTICON),
                                  IMAGE_ICON,
                                  16,
                                  16,
                                  LR_SHARED);

    return RegisterClassEx(&wc) != (ATOM)0;
}

VOID
UninitMainWindowImpl(VOID)
{
    UnregisterClass(szMainWndClass,
                    hInstance);
}
