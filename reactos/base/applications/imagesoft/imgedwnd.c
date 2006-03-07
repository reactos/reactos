#include <precomp.h>

static const TCHAR szImageEditWndClass[] = TEXT("ImageSoftEditWndClass");

#define IMAGE_FRAME_SIZE    1


static VOID
EditWndUpdateScrollInfo(PEDIT_WND_INFO Info)
{
    SCROLLINFO si;
    RECT rcClient;

    GetClientRect(Info->hSelf,
                  &rcClient);

    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_RANGE;
    si.nPage = rcClient.right - (2 * IMAGE_FRAME_SIZE);
    si.nMin = 0;
    si.nMax = Info->Width;

    SetScrollInfo(Info->hSelf,
                  SB_HORZ,
                  &si,
                  TRUE);

    si.nPage = rcClient.bottom - (2 * IMAGE_FRAME_SIZE);
    si.nMax = Info->Height;
    SetScrollInfo(Info->hSelf,
                  SB_VERT,
                  &si,
                  TRUE);
}

static BOOL
InitEditWnd(PEDIT_WND_INFO Info)
{
    HDC hDC;
    LONG cxBitmap, cyBitmap;

    Info->Zoom = 100;

    if (Info->OpenInfo != NULL)
    {
        /* set bitmap dimensions */
        cxBitmap = Info->OpenInfo->New.Width;
        cyBitmap = Info->OpenInfo->New.Height;

        /* create bitmap */
        hDC = GetDC(Info->hSelf);
        Info->hBitmap = CreateCompatibleBitmap(hDC, cxBitmap, cyBitmap);
        Info->hDCMem  = CreateCompatibleDC(hDC);
        ReleaseDC(Info->hSelf, hDC);

        if (!Info->hBitmap)
        {
            DeleteDC(Info->hDCMem);
            return FALSE;
        }

        if (Info->OpenInfo->CreateNew)
        {
            /* what is this for? Does Info->OpenInfo become obsolete? */
            Info->Width = Info->OpenInfo->New.Width;
            Info->Height = Info->OpenInfo->New.Height;

            SelectObject(Info->hDCMem, Info->hBitmap);
            PatBlt(Info->hDCMem, 0, 0, cxBitmap, cxBitmap, WHITENESS);
        }
        else
        {
            /* Load the image from file */
        }

        Info->OpenInfo = NULL;
    }

    EditWndUpdateScrollInfo(Info);

    /* Add image editor to the list */
    Info->Next = Info->MainWnd->ImageEditors;
    Info->MainWnd->ImageEditors = Info;

    /* FIXME - if returning FALSE, remove the image editor from the list! */
    return TRUE;
}

static VOID
DestroyEditWnd(PEDIT_WND_INFO Info)
{
    PEDIT_WND_INFO *PrevEditor;
    PEDIT_WND_INFO Editor;

    /* FIXME - free resources and run down editor */

    /* Remove the image editor from the list */
    PrevEditor = &Info->MainWnd->ImageEditors;
    Editor = Info->MainWnd->ImageEditors;
    do
    {
        if (Editor == Info)
        {
            *PrevEditor = Info->Next;
            break;
        }
        PrevEditor = &Editor->Next;
        Editor = Editor->Next;
    } while (Editor != NULL);
}

static VOID
ImageEditWndRepaint(PEDIT_WND_INFO Info,
                    HDC hDC,
                    LPPAINTSTRUCT lpps)
{
    BitBlt(hDC, 0, 0, Info->Width, Info->Height, Info->hDCMem, 0, 0, SRCCOPY);
}

static LRESULT CALLBACK
ImageEditWndProc(HWND hwnd,
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
        case WM_ERASEBKGND:
            if (Info->Width != 0 && Info->Height != 0)
            {
                Ret = TRUE;
            }
            break;

        case WM_PAINT:
        {
            if (Info->Width != 0 && Info->Height != 0)
            {
                PAINTSTRUCT ps;
                HDC hDC;

                hDC = BeginPaint(hwnd,
                                 &ps);
                if (hDC != NULL)
                {
                    ImageEditWndRepaint(Info,
                                        hDC,
                                        &ps);

                    EndPaint(hwnd,
                             &ps);
                }
            }
            break;
        }

        case WM_SIZE:
        {
            EditWndUpdateScrollInfo(Info);
            goto HandleDefaultMessage;
        }

        case WM_MENUSELECT:
        case WM_ENTERMENULOOP:
        case WM_EXITMENULOOP:
            /* forward these messages to the main window procedure */
            Ret = SendMessage(Info->MainWnd->hSelf,
                              uMsg,
                              wParam,
                              lParam);
            break;

        case WM_MDIACTIVATE:
            /* Switch the main window context if neccessary */
            MainWndSwitchEditorContext(Info->MainWnd,
                                       (HWND)wParam,
                                       (HWND)lParam);
            break;

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

        case WM_DESTROY:
        {
            DestroyEditWnd(Info);

            HeapFree(ProcessHeap,
                     0,
                     Info);
            SetWindowLongPtr(hwnd,
                             GWLP_USERDATA,
                             0);
            break;
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
SetImageEditorEnvironment(PEDIT_WND_INFO Info,
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
CreateImageEditWindow(struct _MAIN_WND_INFO *MainWnd,
                      POPEN_IMAGE_EDIT_INFO OpenInfo)
{
    PEDIT_WND_INFO Info;
    HWND hWndEditor;

    Info = HeapAlloc(ProcessHeap,
                     0,
                     sizeof(EDIT_WND_INFO));
    if (Info != NULL)
    {
        ZeroMemory(Info,
                   sizeof(EDIT_WND_INFO));
        Info->MainWnd = MainWnd;
        Info->MdiEditorType = metImageEditor;
        Info->OpenInfo = OpenInfo;

        hWndEditor = CreateMDIWindow(szImageEditWndClass,
                                     OpenInfo->New.lpImageName,
                                     WS_HSCROLL | WS_VSCROLL,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     OpenInfo->New.Width,
                                     OpenInfo->New.Height,
                                     MainWnd->hMdiClient,
                                     hInstance,
                                     (LPARAM)Info);

        if (hWndEditor != NULL)
        {
            return TRUE;
        }

        HeapFree(ProcessHeap,
                 0,
                 Info);
    }

    return FALSE;
}

BOOL
InitImageEditWindowImpl(VOID)
{
    WNDCLASSEX wc = {0};

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ImageEditWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance,
                        MAKEINTRESOURCE(IDI_ICON));
    wc.hCursor = LoadCursor(NULL,
                            IDC_CROSS);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = szImageEditWndClass;
    wc.hIconSm = (HICON)LoadImage(hInstance,
                                  MAKEINTRESOURCE(IDI_ICON),
                                  IMAGE_ICON,
                                  16,
                                  16,
                                  LR_SHARED);

    return RegisterClassEx(&wc) != (ATOM)0;
}

VOID
UninitImageEditWindowImpl(VOID)
{
    UnregisterClass(szImageEditWndClass,
                    hInstance);
}
