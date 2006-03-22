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
    BITMAPFILEHEADER bmfh;
    PBITMAPINFO pbmi = NULL;
    PBYTE pBits;
    HANDLE hFile;
    HDC hDC;
    BITMAP bitmap;

    Info->Zoom = 100;

    if (Info->OpenInfo != NULL)
    {
        if (Info->OpenInfo->CreateNew)
        {
            /* FIXME: convert this to a DIB Section */

            /* set bitmap dimensions */
            Info->Width = Info->OpenInfo->New.Width;
            Info->Height = Info->OpenInfo->New.Height;

            /* create bitmap */
            hDC = GetDC(Info->hSelf);
            Info->hBitmap = CreateCompatibleBitmap(hDC, Info->Width, Info->Height);
            //Info->hDCMem  = CreateCompatibleDC(hDC);
            ReleaseDC(Info->hSelf, hDC);

            if (!Info->hBitmap)
            {
                DeleteDC(Info->hDCMem);
                return FALSE;
            }

            //SelectObject(Info->hDCMem, Info->hBitmap);
            PatBlt(Info->hDCMem, 0, 0, Info->Width, Info->Height, WHITENESS);
        }
        else
        {
            DWORD InfoSize, BytesRead;
            BOOL bSuccess;

            hFile = CreateFile(Info->OpenInfo->Open.lpImagePath,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
            if (hFile == INVALID_HANDLE_VALUE)
                return FALSE;

            bSuccess = ReadFile(hFile,
                                &bmfh,
                                sizeof(BITMAPFILEHEADER),
                                &BytesRead,
                                NULL);

            if ( bSuccess && (BytesRead == sizeof(BITMAPFILEHEADER))
                          /*&& (bmfh.bfType == *(WORD *)_T("BM"))*/)
            {
                InfoSize = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER);

                pbmi = HeapAlloc(ProcessHeap,
                                 0,
                                 InfoSize);

                bSuccess = ReadFile(hFile,
                                    pbmi,
                                    InfoSize,
                                    &BytesRead,
                                    NULL);

                if (bSuccess && (BytesRead == InfoSize))
                {
                    Info->hBitmap = CreateDIBSection(NULL,
                                                     pbmi,
                                                     DIB_RGB_COLORS,
                                                     (VOID *)&pBits,
                                                     NULL,
                                                     0);
                    if (Info->hBitmap != NULL)
                    {
                        ReadFile(hFile,
                                 pBits,
                                 bmfh.bfSize - bmfh.bfOffBits,
                                 &BytesRead,
                                 NULL);
                    }
                    else
                    {
                        goto fail;
                    }
                }
                else
                {
                    goto fail;
                }
            }
            else
            {
                if (! bSuccess)
                    GetError(0);

                goto fail;
            }

            CloseHandle(hFile);

            HeapFree(ProcessHeap,
                     0,
                     pbmi);
        }

        Info->OpenInfo = NULL;
    }

    /* get bitmap dimensions */
    GetObject(Info->hBitmap,
                  sizeof(BITMAP),
                  &bitmap);

    Info->Width = bitmap.bmWidth;
    Info->Height = bitmap.bmHeight;

    EditWndUpdateScrollInfo(Info);

    /* Add image editor to the list */
    Info->Next = Info->MainWnd->ImageEditors;
    Info->MainWnd->ImageEditors = Info;

    InvalidateRect(Info->hSelf,
                   NULL,
                   TRUE);

    /* FIXME - if returning FALSE, remove the image editor from the list! */
    return TRUE;


fail:
    if (! hFile)
        CloseHandle(hFile);
    if (! pbmi)
        HeapFree(ProcessHeap,
                 0,
                 pbmi);
    return FALSE;
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
    if (Info->hBitmap)
    {
        Info->hDCMem = CreateCompatibleDC(hDC);

        SelectObject(Info->hDCMem,
                     Info->hBitmap);

        BitBlt(hDC,
               0,
               0,
               Info->Width,
               Info->Height,
               Info->hDCMem,
               0,
               0,
               SRCCOPY);

        DeleteDC(Info->hDCMem);
    }
}

static LRESULT CALLBACK
ImageEditWndProc(HWND hwnd,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    PEDIT_WND_INFO Info;
    LRESULT Ret = 0;
    static BOOL bLMButtonDown = FALSE;

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

        case WM_ERASEBKGND:
            if (Info->Width != 0 && Info->Height != 0)
            {
                Ret = TRUE;
            }
            break;

        case WM_LBUTTONDOWN:
            SetCursor(LoadCursor(hInstance,
                                 MAKEINTRESOURCE(IDC_PAINTBRUSHCURSORMOUSEDOWN)));
            bLMButtonDown = TRUE;

        break;

        case WM_LBUTTONUP:
            bLMButtonDown = FALSE;

        break;

        case WM_MOUSEMOVE:
            if (bLMButtonDown)
                SetCursor(LoadCursor(hInstance,
                                     MAKEINTRESOURCE(IDC_PAINTBRUSHCURSORMOUSEDOWN)));
            else
                SetCursor(LoadCursor(hInstance,
                                     MAKEINTRESOURCE(IDC_PAINTBRUSHCURSOR)));
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
    LONG Width, Height;

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

        if (OpenInfo->CreateNew)
        {
            Width = OpenInfo->New.Width;
            Height = OpenInfo->New.Height;
        }
        else
        {
            Width = CW_USEDEFAULT;
            Height = CW_USEDEFAULT;
        }

        hWndEditor = CreateMDIWindow(szImageEditWndClass,
                                     OpenInfo->lpImageName,
                                     WS_HSCROLL | WS_VSCROLL | WS_MAXIMIZE,
                                     200,
                                     200,
                                     Width,
                                     Height,
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
    wc.hCursor = LoadCursor(hInstance,
                            MAKEINTRESOURCE(IDC_PAINTBRUSHCURSOR));
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
