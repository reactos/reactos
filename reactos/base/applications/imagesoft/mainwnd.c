#include <precomp.h>

static const TCHAR szMainWndClass[] = TEXT("ImageSoftWndClass");

#define ID_MDI_FIRSTCHILD   50000
#define ID_MDI_WINDOWMENU   5

/* menu hints */
static const MENU_HINT MainMenuHintTable[] = {
    /* File Menu */
    {ID_CLOSE, IDS_HINT_SYS_CLOSE},
    {ID_EXIT, IDS_HINT_EXIT},

    /* Window Menu */
    {ID_WINDOW_NEXT, IDS_HINT_SYS_NEXT}
};

static const MENU_HINT SystemMenuHintTable[] = {
    {SC_RESTORE, IDS_HINT_SYS_RESTORE},
    {SC_MOVE, IDS_HINT_SYS_MOVE},
    {SC_SIZE, IDS_HINT_SYS_SIZE},
    {SC_MINIMIZE, IDS_HINT_SYS_MINIMIZE},
    {SC_MAXIMIZE, IDS_HINT_SYS_MAXIMIZE},
    {SC_CLOSE, IDS_HINT_SYS_CLOSE},
    {SC_NEXTWINDOW, IDS_HINT_SYS_NEXT},
};

/* Standard Toolbar */
#define ID_TOOLBAR_STANDARD 0
static const TCHAR szToolbarStandard[] = TEXT("STANDARD");
static const TBBUTTON StdButtons[] = {
    /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
    {STD_FILENEW,  ID_NEW,      TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, 0},    /* new */
    {STD_FILEOPEN, ID_OPEN,     TBSTATE_ENABLED, BTNS_BUTTON,    {0}, 0, 0},    /* open */
    {STD_FILESAVE, ID_SAVE,     TBSTATE_INDETERMINATE, BTNS_BUTTON,    {0}, 0, 0},    /* save */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

    {STD_PRINTPRE, ID_PRINTPRE, TBSTATE_INDETERMINATE, BTNS_BUTTON,    {0}, 0, 0 },   /* print */
    {STD_PRINT,    ID_PRINT,    TBSTATE_INDETERMINATE, BTNS_BUTTON,    {0}, 0, 0 },   /* print preview */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

    {STD_CUT,      ID_CUT,      TBSTATE_INDETERMINATE, BTNS_BUTTON,    {0}, 0, 0 },   /* cut */
    {STD_COPY,     ID_COPY,     TBSTATE_INDETERMINATE, BTNS_BUTTON,    {0}, 0, 0 },   /* copy */
    {STD_PASTE,    ID_PASTE,    TBSTATE_INDETERMINATE, BTNS_BUTTON,    {0}, 0, 0 },   /* paste */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},                              /* separator */

    {STD_UNDO,     ID_UNDO,    TBSTATE_INDETERMINATE, BTNS_BUTTON,     {0}, 0, 0 },   /* undo */
    {STD_REDOW,    ID_REDO,    TBSTATE_INDETERMINATE, BTNS_BUTTON,     {0}, 0, 0 },   /* redo */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},
};

/* Test Toolbar */
#define ID_TOOLBAR_TEST 1
static const TCHAR szToolbarTest[] = TEXT("TEST");

/* Toolbars table */
static const DOCKBAR MainDockBars[] = {
    {ID_TOOLBAR_STANDARD, szToolbarStandard, IDS_TOOLBAR_STANDARD, TOP_DOCK},
    {ID_TOOLBAR_TEST, szToolbarTest, IDS_TOOLBAR_TEST, TOP_DOCK},
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
    HWND hWndClient = NULL;

    UNREFERENCED_PARAMETER(Context);

    /* Standard toolbar */
    switch (Dockbar->BarId)
    {
        case ID_TOOLBAR_STANDARD:
        {
            Buttons = StdButtons;
            NumButtons = sizeof(StdButtons) / sizeof(StdButtons[0]);
            break;
        }

        case ID_TOOLBAR_TEST:
        {
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
                                        NULL);
            break;
        }
    }

    if (Buttons != NULL)
    {
        TBADDBITMAP tbab;

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
            SendMessage(hWndClient,
                        TB_SETEXTENDEDSTYLE,
                        0,
                        TBSTYLE_EX_HIDECLIPPEDBUTTONS);

            /* Send the TB_BUTTONSTRUCTSIZE message, which is required for backward compatibility */
            SendMessage(hWndClient,
                        TB_BUTTONSTRUCTSIZE,
                        sizeof(Buttons[0]),
                        0);

            /* Add standard images */
            tbab.hInst = HINST_COMMCTRL;
            tbab.nID = IDB_STD_SMALL_COLOR;
            SendMessage(hWndClient,
                        TB_ADDBITMAP,
                        (WPARAM)NumButtons,
                        (LPARAM)&tbab);

            /* Add buttons to toolbar */
            SendMessage(hWndClient,
                        TB_ADDBUTTONS,
                        (WPARAM)NumButtons,
                        (LPARAM)Buttons);
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
            case ID_TOOLBAR_STANDARD:
            {
                SIZE Size;
                BOOL Vert;
                DWORD dwStyle = SendMessage(rbi->hwndChild,
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

    for (i = 0; i < sizeof(MainDockBars) / sizeof(MainDockBars[0]); i++)
    {
        /* FIXME - lookup whether to display the toolbar */
        TbdAddToolbar(&Info->ToolDocks,
                      &MainDockBars[i],
                      Info,
                      &MainWndDockBarCallbacks);
    }
}

static VOID CALLBACK
MainWndResize(PVOID Context,
              WORD cx,
              WORD cy)
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
}

static VOID
MainWndCommand(PMAIN_WND_INFO Info,
               WORD CmdId,
               HWND hControl)
{
    UNREFERENCED_PARAMETER(hControl);

    switch (CmdId)
    {
        /* File Menu */
        case ID_NEW:
        {
            OPEN_IMAGE_EDIT_INFO OpenInfo;
            PIMAGE_PROP ImageProp = NULL;
            LPTSTR lpCaption = NULL;

            LoadAndFormatString(hInstance,
                                IDS_IMAGE_NAME,
                                &lpCaption,
                                ++Info->ImagesCreated);

            ImageProp = (PIMAGE_PROP)DialogBox(hInstance,
                                               MAKEINTRESOURCE(IDD_IMAGE_PROP),
                                               Info->hSelf,
                                               ImagePropDialogProc);

            if (ImageProp != 0)
            {
                OpenInfo.CreateNew = TRUE;
                OpenInfo.Type = ImageProp->Type;
                OpenInfo.Resolution = ImageProp->Resolution;
                OpenInfo.New.Width = ImageProp->Width;
                OpenInfo.New.Height = ImageProp->Height;

                HeapFree(ProcessHeap,
                         0,
                         ImageProp);

                CreateImageEditWindow(Info,
                                      lpCaption,
                                      &OpenInfo);
            }

            if (lpCaption != NULL)
                LocalFree((HLOCAL)lpCaption);
            break;
        }

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
            break;
        }

        case WM_NOTIFY:
        {
            if (!TbdHandleNotifications(&Info->ToolDocks,
                                        (LPNMHDR)lParam,
                                        &Ret))
            {
                /* FIXME - handle other notifications */
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
                                     sizeof(MainMenuHintTable) / sizeof(MainMenuHintTable[0]),
                                     IDS_READY))
                {
                    MainWndMenuHint(Info,
                                    LOWORD(wParam),
                                    SystemMenuHintTable,
                                    sizeof(SystemMenuHintTable) / sizeof(SystemMenuHintTable[0]),
                                    IDS_READY);
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

        case WM_ACTIVATEAPP:
        {
            //TbdShowFloatingToolbars(&Info->ToolDocks,
            //                        (BOOL)wParam);
            break;
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

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance,
                        MAKEINTRESOURCE(IDI_ICON));
    wc.hCursor = LoadCursor(NULL,
                            IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
    wc.lpszClassName = szMainWndClass;
    wc.hIconSm = (HICON)LoadImage(hInstance,
                                  MAKEINTRESOURCE(IDI_ICON),
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
