#include "precomp.h"

static const TCHAR szMainWndClass[] = TEXT("WordPadMainWndClass");

#define ID_MDI_FIRSTCHILD   50000
#define ID_MDI_WINDOWMENU   5

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
    {ID_PRINT,     IDS_HINT_PRINT},
    {ID_PRINTPRE,  IDS_HINT_PRINTPRE},
    {ID_PAGESETUP, IDS_HINT_PAGESETUP},
    {ID_EXIT,      IDS_HINT_EXIT},

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


static VOID
CreateToolbars(PMAIN_WND_INFO Info)
{

}

static VOID CALLBACK
MainWndResize(PVOID Context,
              WORD cx,
              WORD cy)
{
    RECT rcClient = {0};
    RECT rcStatus = {0};
    HDWP dwp;
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


    dwp = BeginDeferWindowPos(2);
    if (dwp != NULL)
    {
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
                    sizeof(statwidths)/sizeof(int),
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
    static TCHAR szDocumentName[MAX_PATH];

    UNREFERENCED_PARAMETER(hControl);

    switch (CmdId)
    {
        case ID_NEW:
        {
            OPEN_EDIT_INFO OpenInfo;
            INT Ret;

            OpenInfo.CreateNew = TRUE;

            LoadAndFormatString(hInstance,
                                IDS_DEFAULT_NAME,
                                &OpenInfo.lpDocumentName,
                                ++Info->ImagesCreated);

            Ret = DialogBox(hInstance,
                            MAKEINTRESOURCE(IDD_NEWDOCSEL),
                            Info->hSelf,
                            NewDocSelDlgProc);
            if (Ret != -1)
            {
                OpenInfo.DocType = Ret;

                CreateEditWindow(Info,
                                 &OpenInfo);
            }

        }
        break;

        case ID_BOLD:
            MessageBox(NULL, _T("Bingo"), NULL, 0);
        break;

        case ID_OPEN:
        {
            OPEN_EDIT_INFO OpenInfo;

            if (DoOpenFile(Info->hSelf,
                           szFileName,   /* full file path */
                           szDocumentName)) /* file name */
            {
                OpenInfo.CreateNew = FALSE;

                OpenInfo.lpDocumentPath = szFileName;
                OpenInfo.lpDocumentName = szDocumentName;

                CreateEditWindow(Info,
                                 &OpenInfo);
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

        case WM_SIZE:
        {
            MainWndResize(Info,
                          LOWORD(lParam),
                          HIWORD(lParam));
            /* NOTE - do *not* forward this message to DefFrameProc! Otherwise the MDI client
                      will attempt to resize itself */

            break;
        }

        case WM_MOVE:
        {

        }
        break;

        case WM_NOTIFY:
        {

                /* FIXME - handle other notifications */
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
                                     IDS_HINT_BLANK))
                {
                    MainWndMenuHint(Info,
                                    LOWORD(wParam),
                                    SystemMenuHintTable,
                                    sizeof(SystemMenuHintTable) / sizeof(SystemMenuHintTable[0]),
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

            goto HandleDefaultMessage;
        }

        case WM_NCACTIVATE:
        {

            goto HandleDefaultMessage;
        }

        case WM_ACTIVATEAPP:
        {

            goto HandleDefaultMessage;
        }

        case WM_DESTROY:
        {
            DestroyMainWnd(Info);

            /* FIXME: set the windows position in registry*/
            //wndOldPos

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
                    SetEditorEnvironment((PEDIT_WND_INFO)EditorType,
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
                    SetEditorEnvironment((PEDIT_WND_INFO)EditorType,
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

    Info = (MAIN_WND_INFO*) HeapAlloc(ProcessHeap,
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
