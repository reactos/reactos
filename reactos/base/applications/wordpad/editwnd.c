#include "precomp.h"

static const TCHAR szEditWndClass[] = TEXT("WordPadEditWndClass");


static BOOL
InitEditWnd(PEDIT_WND_INFO Info)
{
    HANDLE hDLL;
    HFONT hfDefault;

    hDLL = LoadLibrary(_T("RICHED20.DLL"));
    if (hDLL == NULL)
    {
        GetError(0);
        return FALSE;
    }

    Info->hEdit = CreateWindowEx(0, //WS_EX_CLIENTEDGE,
                                 RICHEDIT_CLASS,
                                 NULL,
                                 WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                                   ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                                 0,
                                 0,
                                 100,
                                 100,
                                 Info->hSelf,
                                 NULL,
                                 hInstance,
                                 NULL);
    if(Info->hEdit == NULL)
    {
        GetError(0);

        return FALSE;
    }

    hfDefault = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(Info->hEdit,
                WM_SETFONT,
                (WPARAM)hfDefault,
                MAKELPARAM(FALSE, 0));

    return TRUE;
}


static LRESULT CALLBACK
EditWndProc(HWND hwnd,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam)
{
    PEDIT_WND_INFO Info;
    LRESULT Ret = 0;

    /* Get the window context */
    Info = (PEDIT_WND_INFO)GetWindowLongPtr(hwnd,
                                            GWLP_USERDATA);
    if (Info == NULL && uMsg != WM_CREATE)
    {
        goto HandleDefaultMessage;
    }

    switch (uMsg)
    {
        case WM_CREATE:
        {
            Info = (PEDIT_WND_INFO)(((LPMDICREATESTRUCT)((LPCREATESTRUCT)lParam)->lpCreateParams)->lParam);
            Info->hSelf = hwnd;

            SetWindowLongPtr(hwnd,
                             GWLP_USERDATA,
                             (LONG_PTR)Info);

            if (!InitEditWnd(Info))
            {
                Ret = (LRESULT)-1;
                break;
            }
            break;
        }

        case WM_SIZE:
        {
            RECT rcClient;

            if (GetClientRect(Info->hSelf,
                              &rcClient))
            {
                SetWindowPos(Info->hEdit,
                             NULL,
                             0,
                             0,
                             rcClient.right,
                             rcClient.bottom,
                             SWP_NOZORDER);
            }
        }

        default:
HandleDefaultMessage:
            Ret = DefMDIChildProc(hwnd,
                                  uMsg,
                                  wParam,
                                  lParam);
            break;
    }

    return Ret;

}


VOID
SetEditorEnvironment(PEDIT_WND_INFO Info,
                          BOOL Setup)
{
    if (Setup)
    {
        /* FIXME - setup editor environment (e.g. show toolbars, enable menus etc) */
    }
    else
    {
        /* FIXME - cleanup editor environment (e.g. hide toolbars, disable menus etc) */
    }
}


BOOL
CreateEditWindow(struct _MAIN_WND_INFO *MainWnd,
                 POPEN_EDIT_INFO OpenInfo)
{
    PEDIT_WND_INFO Info;
    HWND hWndEditor;

    Info = (EDIT_WND_INFO*) HeapAlloc(ProcessHeap,
                     0,
                     sizeof(EDIT_WND_INFO));
    if (Info != NULL)
    {
        ZeroMemory(Info,
                   sizeof(EDIT_WND_INFO));
        Info->MainWnd = MainWnd;
        Info->MdiEditorType = metImageEditor;
        Info->OpenInfo = OpenInfo;

        hWndEditor = CreateMDIWindow(szEditWndClass,
                                     OpenInfo->lpDocumentName,
                                     WS_MAXIMIZE,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     MainWnd->hMdiClient,
                                     hInstance,
                                     (LPARAM)Info);

        if (hWndEditor != NULL)
            return TRUE;


        HeapFree(ProcessHeap,
                 0,
                 Info);
    }

    return FALSE;
}

BOOL
InitEditWindowImpl(VOID)
{
    WNDCLASSEX wc = {0};

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = EditWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance,
                        MAKEINTRESOURCE(IDI_ICON));
    wc.hCursor = LoadCursor(NULL,
                            IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szEditWndClass;
    wc.hIconSm = (HICON)LoadImage(hInstance,
                                  MAKEINTRESOURCE(IDI_ICON),
                                  IMAGE_ICON,
                                  16,
                                  16,
                                  LR_SHARED);

    return RegisterClassEx(&wc) != (ATOM)0;
}

VOID
UninitEditWindowImpl(VOID)
{
    UnregisterClass(szEditWndClass,
                    hInstance);
}
