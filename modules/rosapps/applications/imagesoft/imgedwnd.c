#include "precomp.h"

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
LoadBlankCanvas(PEDIT_WND_INFO Info)
{
    /* FIXME: convert this to a DIB Section */
    /* set bitmap dimensions */
    Info->Width = Info->OpenInfo->New.Width;
    Info->Height = Info->OpenInfo->New.Height;

    return TRUE;
}

static BOOL
LoadDIBImage(PEDIT_WND_INFO Info)
{
    BITMAPFILEHEADER bmfh;
    HANDLE hFile;
    BITMAP bitmap;
    DWORD BytesRead;
    BOOL bSuccess, bRet = FALSE;

    hFile = CreateFile(Info->OpenInfo->Open.lpImagePath,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_SEQUENTIAL_SCAN,
                       NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return bRet;

    bSuccess = ReadFile(hFile,
                        &bmfh,
                        sizeof(bmfh),
                        &BytesRead,
                        NULL);

    if (bSuccess && (BytesRead == sizeof(bmfh))
                 && (bmfh.bfType == *(WORD *)"BM"))
    {
        DWORD InfoSize = bmfh.bfOffBits - sizeof(bmfh);

        Info->pbmi = HeapAlloc(ProcessHeap,
                               0,
                               InfoSize);
        if (Info->pbmi)
        {
            bSuccess = ReadFile(hFile,
                                Info->pbmi,
                                InfoSize,
                                &BytesRead,
                                NULL);

            if (bSuccess && (BytesRead == InfoSize))
            {
                Info->hBitmap = CreateDIBSection(NULL,
                                                 Info->pbmi,
                                                 DIB_RGB_COLORS,
                                                 (VOID *)&Info->pBits,
                                                 NULL,
                                                 0);
                if (Info->hBitmap != NULL)
                {
                    ReadFile(hFile,
                             Info->pBits,
                             bmfh.bfSize - bmfh.bfOffBits,
                             &BytesRead,
                             NULL);

                    GetObject(Info->hBitmap,
                              sizeof(bitmap),
                              &bitmap);

                    Info->Width = bitmap.bmWidth;
                    Info->Height = bitmap.bmHeight;

                    bRet = TRUE;
                }
            }
        }
    }
    else if (!bSuccess)
    {
        GetError(0);
    }

    CloseHandle(hFile);

    return bRet;
}


static BOOL
InitEditWnd(PEDIT_WND_INFO Info)
{
    //BOOL bRet = FALSE;

    Info->Zoom = 100;

    if (Info->OpenInfo != NULL)
    {
        HDC hDC;

        if (Info->hDCMem)
        {
            DeleteObject(Info->hDCMem);
            Info->hDCMem = NULL;
        }

        hDC = GetDC(Info->hSelf);
        Info->hDCMem = CreateCompatibleDC(hDC);
        ReleaseDC(Info->hSelf, hDC);

        if (Info->OpenInfo->CreateNew)
        {
            LoadBlankCanvas(Info);
        }
        else
        {
            LoadDIBImage(Info);
        }

        Info->OpenInfo = NULL;
    }

    EditWndUpdateScrollInfo(Info);

    /* Add image editor to the list */
    Info->Next = Info->MainWnd->ImageEditors;
    Info->MainWnd->ImageEditors = Info;

    InvalidateRect(Info->hSelf,
                   NULL,
                   TRUE);

    /* FIXME - if returning FALSE, remove the image editor from the list! */
    return TRUE;
}

static VOID
DestroyEditWnd(PEDIT_WND_INFO Info)
{
    PEDIT_WND_INFO *PrevEditor;
    PEDIT_WND_INFO Editor;

    DeleteDC(Info->hDCMem);

    /* FIXME - free resources and run down editor */
    HeapFree(ProcessHeap,
             0,
             Info->pbmi);
    HeapFree(ProcessHeap,
             0,
             Info->pBits);

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
    HBITMAP hOldBitmap;

    if (Info->hBitmap)
    {
        hOldBitmap = (HBITMAP) SelectObject(Info->hDCMem,
                                            Info->hBitmap);

        BitBlt(hDC,
               lpps->rcPaint.left,
               lpps->rcPaint.top,
               lpps->rcPaint.right - lpps->rcPaint.left,
               lpps->rcPaint.bottom - lpps->rcPaint.top,
               Info->hDCMem,
               lpps->rcPaint.left,
               lpps->rcPaint.top,
               SRCCOPY);

        Info->hBitmap = SelectObject(Info->hDCMem, hOldBitmap);
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
    HDC hDC;
    static INT xMouse, yMouse;
    static BOOL bLeftButtonDown, bRightButtonDown;

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
/*
        case WM_ERASEBKGND:
            if (Info->Width != 0 && Info->Height != 0)
            {
                Ret = TRUE;
            }
            break;
*/
        case WM_LBUTTONDOWN:
            if (! bRightButtonDown)
                SetCapture(Info->hSelf);

            bLeftButtonDown = TRUE;
            xMouse = LOWORD(lParam);
            yMouse = HIWORD(lParam);

            SetCursor(LoadCursor(hInstance,
                                 MAKEINTRESOURCE(IDC_PAINTBRUSHCURSORMOUSEDOWN)));
        break;

        case WM_LBUTTONUP:
            if (bLeftButtonDown)
                SetCapture(NULL);

            bLeftButtonDown = FALSE;

        break;

        case WM_RBUTTONDOWN:
            if (! bLeftButtonDown)
                SetCapture(Info->hSelf);

            bRightButtonDown = TRUE;
            xMouse = LOWORD(lParam);
            yMouse = HIWORD(lParam);

            SetCursor(LoadCursor(hInstance,
                                 MAKEINTRESOURCE(IDC_PAINTBRUSHCURSORMOUSEDOWN)));
        break;

        case WM_RBUTTONUP:
            if (bRightButtonDown)
                SetCapture(NULL);

            bRightButtonDown = FALSE;

        break;

        case WM_MOUSEMOVE:
        {
            HPEN hPen, hPenOld;

            if (!bLeftButtonDown && !bRightButtonDown)
                break;

            hDC = GetDC(Info->hSelf);

            SelectObject(Info->hDCMem,
                         Info->hBitmap);

            if (bLeftButtonDown)
                hPen = CreatePen(PS_SOLID,
                                 3,
                                 RGB(0, 0, 0));
            else
                hPen = CreatePen(PS_SOLID,
                                 3,
                                 RGB(255, 255, 255));

            hPenOld = SelectObject(hDC,
                                   hPen);
            SelectObject(Info->hDCMem,
                         hPen);

            MoveToEx(hDC,
                     xMouse,
                     yMouse,
                     NULL);

            MoveToEx(Info->hDCMem,
                     xMouse,
                     yMouse,
                     NULL);

            xMouse = LOWORD(lParam);
            yMouse = HIWORD(lParam);

            LineTo(hDC,
                   xMouse,
                   yMouse);

            LineTo(Info->hDCMem,
                   xMouse,
                   yMouse);

            SelectObject(hDC,
                         hPenOld);
            DeleteObject(SelectObject(Info->hDCMem,
                                      hPenOld));

            ReleaseDC(Info->hSelf,
                      hDC);
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
CreateImageEditWindow(PMAIN_WND_INFO MainWnd,
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

    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ImageEditWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance,
                        MAKEINTRESOURCE(IDI_IMAGESOFTICON));
    wc.hCursor = LoadCursor(hInstance,
                            MAKEINTRESOURCE(IDC_PAINTBRUSHCURSOR));
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = szImageEditWndClass;
    wc.hIconSm = (HICON)LoadImage(hInstance,
                                  MAKEINTRESOURCE(IDI_IMAGESOFTICON),
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
