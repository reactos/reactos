/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     ISC (https://spdx.org/licenses/ISC)
 * PURPOSE:     Tests window redrawing when scrollbars appear or disappear
 * COPYRIGHT:   Copyright 2024 Marek Benc <benc.marek.elektro98@proton.me>
 */

#include <windows.h>
#include "wine/test.h"

#define TEST_CLASS_NAME   "ScrollBarRedraw"
#define TEST_WINDOW_TITLE "ScrollBarRedraw"

static LRESULT CALLBACK WindowProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

#define TEST_COLOR_COUNT 16
static COLORREF Colors[TEST_COLOR_COUNT] = { 0 };
static HBRUSH   ColorBrushes[TEST_COLOR_COUNT] = { 0 };

static BOOL HaveHRedraw = FALSE;
static BOOL HaveVRedraw = FALSE;

typedef enum
{
    FSM_STATE_START,
    FSM_STATE_VSCR_SHOWN,
    FSM_STATE_VSCR_HIDDEN,
    FSM_STATE_HSCR_SHOWN,
    FSM_STATE_HSCR_HIDDEN,
    FSM_STATE_BSCR_SHOWN,
    FSM_STATE_BSCR_HIDDEN,
    FSM_STATE_WIDTH_SHRUNK,
    FSM_STATE_WIDTH_EXPANDED,
    FSM_STATE_HEIGHT_SHRUNK,
    FSM_STATE_HEIGHT_EXPANDED,
    FSM_STATE_BOTH_SHRUNK,
    FSM_STATE_BOTH_EXPANDED,
    FSM_STATE_END
} FSM_STATE;

#define FSM_STEP_PERIOD_MS 250

static UINT_PTR FsmTimer = 0;
static UINT CurrentColor = 0;
static FSM_STATE FsmState = FSM_STATE_START;

static int ClientWidth = 0;
static int ClientHeight = 0;

static int OrigWidth = 0;
static int OrigHeight = 0;
static int SmallWidth = 0;
static int SmallHeight = 0;

static void ColorsCleanup()
{
    UINT Iter;

    for (Iter = 0; Iter < TEST_COLOR_COUNT; Iter++)
    {
        if (ColorBrushes[Iter] != NULL)
        {
            DeleteObject(ColorBrushes[Iter]);
            ColorBrushes[Iter] = NULL;
        }
    }
}

static BOOL ColorsInit(void)
{
    UINT Iter;

    if (TEST_COLOR_COUNT != 16)
    {
        return FALSE;
    }

    /* Standard Windows 16-Color VGA Color palette. */
    Colors[0]  = RGB(0x00, 0x00, 0x00);  /* Black */
    Colors[1]  = RGB(0x00, 0x00, 0x80);  /* Dark Blue */
    Colors[2]  = RGB(0x00, 0x80, 0x00);  /* Dark Green */
    Colors[3]  = RGB(0x00, 0x80, 0x80);  /* Dark Cyan */
    Colors[4]  = RGB(0x80, 0x00, 0x00);  /* Dark Red */
    Colors[5]  = RGB(0x80, 0x00, 0x80);  /* Dark Magenta */
    Colors[6]  = RGB(0x80, 0x80, 0x00);  /* Dark Yellow */
    Colors[7]  = RGB(0xC0, 0xC0, 0xC0);  /* Light Gray */
    Colors[8]  = RGB(0x80, 0x80, 0x80);  /* Dark Gray */
    Colors[9]  = RGB(0x00, 0x00, 0xFF);  /* Blue */
    Colors[10] = RGB(0x00, 0xFF, 0x00);  /* Green */
    Colors[11] = RGB(0x00, 0xFF, 0xFF);  /* Cyan */
    Colors[12] = RGB(0xFF, 0x00, 0x00);  /* Red */
    Colors[13] = RGB(0xFF, 0x00, 0xFF);  /* Magenta */
    Colors[14] = RGB(0xFF, 0xFF, 0x00);  /* Yellow */
    Colors[15] = RGB(0xFF, 0xFF, 0xFF);  /* White */

    for (Iter = 0; Iter < TEST_COLOR_COUNT; Iter++)
    {
        ColorBrushes[Iter] = CreateSolidBrush(Colors[Iter]);
        if (ColorBrushes[Iter] == NULL)
        {
            ColorsCleanup();
            return FALSE;
        }
    }

    return TRUE;
}

static void RunTestWindow(const char *ClassName, const char *WindowTitle, UINT ClassStyle)
{
    WNDCLASS Class = { 0 };
    HWND Window;
    MSG  Message;

    CurrentColor = 0;

    Class.style         = ClassStyle;
    Class.lpfnWndProc   = WindowProc;
    Class.cbClsExtra    = 0;
    Class.cbWndExtra    = 0;
    Class.hInstance     = GetModuleHandleA(NULL);
    Class.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    Class.hCursor       = LoadCursor(NULL, IDC_ARROW);
    Class.hbrBackground = ColorBrushes[CurrentColor];
    Class.lpszMenuName  = NULL;
    Class.lpszClassName = ClassName;

    if (!RegisterClass(&Class))
    {
        skip("Failed to register window class \"%s\", code: %ld\n",
             ClassName, GetLastError());
        return;
    }

    Window = CreateWindow(ClassName,
                          WindowTitle,
                          WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          NULL,
                          NULL,
                          GetModuleHandleA(NULL),
                          NULL);

    if (Window == NULL)
    {
        skip("Failed to create window of class \"%s\", code: %ld\n",
             ClassName, GetLastError());
        return;
    }

    ShowWindow(Window, SW_SHOWNORMAL);
    UpdateWindow(Window);

    while (GetMessage(&Message, NULL, 0, 0))
    {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
}

START_TEST(ScrollBarRedraw)
{
    if (!ColorsInit())
    {
        skip("Failed to initialize colors and solid color brushes\n");
        return;
    }

    trace("Running test without specifying either CS_HREDRAW or CS_HREDRAW\n");
    HaveHRedraw = FALSE;
    HaveVRedraw = FALSE;
    RunTestWindow(TEST_CLASS_NAME   "NoRedraw",
                  TEST_WINDOW_TITLE " (No Redraw Flags)",
                  0);

    trace("Running test with CS_HREDRAW\n");
    HaveHRedraw = TRUE;
    HaveVRedraw = FALSE;
    RunTestWindow(TEST_CLASS_NAME   "HRedraw",
                  TEST_WINDOW_TITLE " (CS_HREDRAW)",
                  CS_HREDRAW);

    trace("Running test with CS_VREDRAW\n");
    HaveHRedraw = FALSE;
    HaveVRedraw = TRUE;
    RunTestWindow(TEST_CLASS_NAME   "VRedraw",
                  TEST_WINDOW_TITLE " (CS_VREDRAW)",
                  CS_VREDRAW);

    trace("Running test with both CS_HREDRAW and CS_VREDRAW\n");
    HaveHRedraw = TRUE;
    HaveVRedraw = TRUE;
    RunTestWindow(TEST_CLASS_NAME   "HRedrawVRedraw",
                  TEST_WINDOW_TITLE " (CS_HREDRAW | CS_VREDRAW)",
                  CS_HREDRAW | CS_VREDRAW);

    trace("Test complete\n");
    ColorsCleanup();
}

static void HideVertScrollBar(HWND Window)
{
    SCROLLINFO ScrollInfo;

    ScrollInfo.cbSize = sizeof(ScrollInfo);
    ScrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    ScrollInfo.nPage = ClientHeight;

    ScrollInfo.nMin = 0;
    ScrollInfo.nMax = ClientHeight - 1;
    ScrollInfo.nPos = 0;

    SetScrollInfo(Window, SB_VERT, &ScrollInfo, TRUE);
}

static void ShowVertScrollBar(HWND Window)
{
    SCROLLINFO ScrollInfo;

    ScrollInfo.cbSize = sizeof(ScrollInfo);
    ScrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    ScrollInfo.nPage = ClientHeight;

    ScrollInfo.nMin = 0;
    ScrollInfo.nMax = (3 * ClientHeight) - 1;
    ScrollInfo.nPos = 0;

    SetScrollInfo(Window, SB_VERT, &ScrollInfo, TRUE);
}

static void HideHorzScrollBar(HWND Window)
{
    SCROLLINFO ScrollInfo;

    ScrollInfo.cbSize = sizeof(ScrollInfo);
    ScrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    ScrollInfo.nPage = ClientWidth;

    ScrollInfo.nMin = 0;
    ScrollInfo.nMax = ClientWidth - 1;
    ScrollInfo.nPos = 0;

    SetScrollInfo(Window, SB_HORZ, &ScrollInfo, TRUE);
}

static void ShowHorzScrollBar(HWND Window)
{
    SCROLLINFO ScrollInfo;

    ScrollInfo.cbSize = sizeof(ScrollInfo);
    ScrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    ScrollInfo.nPage = ClientWidth;

    ScrollInfo.nMin = 0;
    ScrollInfo.nMax = (3 * ClientWidth) - 1;
    ScrollInfo.nPos = 0;

    SetScrollInfo(Window, SB_HORZ, &ScrollInfo, TRUE);
}

static int FsmStep(HWND Window)
{
    static COLORREF PrevColor = CLR_INVALID;
    COLORREF Color = CLR_INVALID;
    HDC hdc = NULL;

    if (FsmState != FSM_STATE_END)
    {
        hdc = GetDC(Window);

        if (hdc == NULL)
        {
            skip("Failed to get device context\n");

            DestroyWindow(Window);
            FsmState = FSM_STATE_END;

            return 0;
        }
        Color = GetPixel(hdc, ClientWidth / 4, ClientHeight / 4);

        ReleaseDC(Window, hdc);
        hdc = NULL;

        if (Color == CLR_INVALID)
        {
            skip("Failed to get window color\n");

            DestroyWindow(Window);
            FsmState = FSM_STATE_END;

            return 0;
        }
    }

    trace("FsmState: %d, Color: 0x%.8lX\n", FsmState, Color);

    switch (FsmState)
    {
        case FSM_STATE_START:
            ShowVertScrollBar(Window);
            FsmState = FSM_STATE_VSCR_SHOWN;
            break;

        case FSM_STATE_VSCR_SHOWN:
            if (HaveHRedraw)
            {
                ok(Color != PrevColor,
                   "CS_HREDRAW specified, but appearence of vertical scroll bar"
                   " didn't trigger redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }
            else
            {
                ok(Color == PrevColor,
                   "CS_HREDRAW not specified, but appearence of vertical scroll bar"
                   " triggered unneccessary redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }

            HideVertScrollBar(Window);
            FsmState = FSM_STATE_VSCR_HIDDEN;

            break;

        case FSM_STATE_VSCR_HIDDEN:
            if (HaveHRedraw)
            {
                ok(Color != PrevColor,
                   "CS_HREDRAW specified, but disappearence of vertical scroll bar"
                   " didn't trigger redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }
            else
            {
                ok(Color == PrevColor,
                   "CS_HREDRAW not specified, but disappearence of vertical scroll bar"
                   " triggered unneccessary redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }

            ShowHorzScrollBar(Window);
            FsmState = FSM_STATE_HSCR_SHOWN;
            break;

        case FSM_STATE_HSCR_SHOWN:
            if (HaveVRedraw)
            {
                ok(Color != PrevColor,
                   "CS_VREDRAW specified, but appearence of horizontal scroll bar"
                   " didn't trigger redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }
            else
            {
                ok(Color == PrevColor,
                   "CS_VREDRAW not specified, but appearence of horizontal scroll bar"
                   " triggered unneccessary redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }

            HideHorzScrollBar(Window);
            FsmState = FSM_STATE_HSCR_HIDDEN;
            break;

        case FSM_STATE_HSCR_HIDDEN:
            if (HaveVRedraw)
            {
                ok(Color != PrevColor,
                   "CS_VREDRAW specified, but disappearence of horizontal scroll bar"
                   " didn't trigger redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }
            else
            {
                ok(Color == PrevColor,
                   "CS_VREDRAW not specified, but disappearence of horizontal scroll bar"
                   " triggered unneccessary redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }

            ShowVertScrollBar(Window);
            ShowHorzScrollBar(Window);

            FsmState = FSM_STATE_BSCR_SHOWN;
            break;

        case FSM_STATE_BSCR_SHOWN:
            if (HaveHRedraw || HaveVRedraw)
            {
                ok(Color != PrevColor,
                   "CS_HREDRAW or CS_VREDRAW specified, but appearence of both scroll bars"
                   " didn't trigger redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }
            else
            {
                ok(Color == PrevColor,
                   "Neither CS_HREDRAW nor CS_VREDRAW specified, but appearence"
                   " of both scroll bars triggered unneccessary redraw,"
                   " PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }

            HideVertScrollBar(Window);
            HideHorzScrollBar(Window);

            FsmState = FSM_STATE_BSCR_HIDDEN;
            break;

        case FSM_STATE_BSCR_HIDDEN:
            if (HaveHRedraw || HaveVRedraw)
            {
                ok(Color != PrevColor,
                   "CS_HREDRAW or CS_VREDRAW specified, but disappearence of both scroll bars"
                   " didn't trigger redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }
            else
            {
                ok(Color == PrevColor,
                   "Neither CS_HREDRAW nor CS_VREDRAW specified, but disappearence"
                   " of both scroll bars triggered unneccessary redraw,"
                   " PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }

            SetWindowPos(Window, HWND_TOPMOST, 0, 0, SmallWidth, OrigHeight, SWP_NOMOVE);
            FsmState = FSM_STATE_WIDTH_SHRUNK;
            break;

        case FSM_STATE_WIDTH_SHRUNK:
            if (HaveHRedraw)
            {
                ok(Color != PrevColor,
                   "CS_HREDRAW specified, but horizontal window shrinkage"
                   " didn't trigger redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }
            else
            {
                ok(Color == PrevColor,
                   "CS_HREDRAW not specified, but horizontal window shrinkage"
                   " triggered unneccessary redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }

            SetWindowPos(Window, HWND_TOPMOST, 0, 0, OrigWidth, OrigHeight, SWP_NOMOVE);
            FsmState = FSM_STATE_WIDTH_EXPANDED;
            break;

        case FSM_STATE_WIDTH_EXPANDED:
            if (HaveHRedraw)
            {
                ok(Color != PrevColor,
                   "CS_HREDRAW specified, but horizontal window expansion"
                   " didn't trigger redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }
            else
            {
                ok(Color == PrevColor,
                   "CS_HREDRAW not specified, but horizontal window expansion"
                   " triggered unneccessary redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }

            SetWindowPos(Window, HWND_TOPMOST, 0, 0, OrigWidth, SmallHeight, SWP_NOMOVE);
            FsmState = FSM_STATE_HEIGHT_SHRUNK;
            break;

        case FSM_STATE_HEIGHT_SHRUNK:
            if (HaveVRedraw)
            {
                ok(Color != PrevColor,
                   "CS_VREDRAW specified, but vertical window shrinkage"
                   " didn't trigger redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }
            else
            {
                ok(Color == PrevColor,
                   "CS_VREDRAW not specified, but vertical window shrinkage"
                   " triggered unneccessary redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }

            SetWindowPos(Window, HWND_TOPMOST, 0, 0, OrigWidth, OrigHeight, SWP_NOMOVE);
            FsmState = FSM_STATE_HEIGHT_EXPANDED;
            break;

        case FSM_STATE_HEIGHT_EXPANDED:
            if (HaveVRedraw)
            {
                ok(Color != PrevColor,
                   "CS_VREDRAW specified, but vertical window expansion"
                   " didn't trigger redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }
            else
            {
                ok(Color == PrevColor,
                   "CS_VREDRAW not specified, but vertical window expansion"
                   " triggered unneccessary redraw, PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }

            SetWindowPos(Window, HWND_TOPMOST, 0, 0, SmallWidth, SmallHeight, SWP_NOMOVE);

            FsmState = FSM_STATE_BOTH_SHRUNK;
            break;

        case FSM_STATE_BOTH_SHRUNK:
            if (HaveHRedraw || HaveVRedraw)
            {
                ok(Color != PrevColor,
                   "CS_HREDRAW or CS_VREDRAW specified, but combined"
                   " vertical/horizontal shrinkage didn't trigger redraw,"
                   " PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }
            else
            {
                ok(Color == PrevColor,
                   "Neither CS_HREDRAW nor CS_VREDRAW specified, but combined"
                   " vertical/horizontal shrinkage triggered unneccessary redraw,"
                   " PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }

            SetWindowPos(Window, HWND_TOPMOST, 0, 0, OrigWidth, OrigHeight, SWP_NOMOVE);

            FsmState = FSM_STATE_BOTH_EXPANDED;
            break;

        case FSM_STATE_BOTH_EXPANDED:
            if (HaveHRedraw || HaveVRedraw)
            {
                ok(Color != PrevColor,
                   "CS_HREDRAW or CS_VREDRAW specified, but combined"
                   " vertical/horizontal expansion didn't trigger redraw,"
                   " PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }
            else
            {
                ok(Color == PrevColor,
                   "Neither CS_HREDRAW nor CS_VREDRAW specified, but combined"
                   " vertical/horizontal expansion triggered unneccessary redraw,"
                   " PrevColor: 0x%.8lX, Color: 0x%.8lX\n",
                   PrevColor, Color);
            }

            DestroyWindow(Window);

            FsmState = FSM_STATE_END;
            break;

        case FSM_STATE_END:
            break;
    }

    PrevColor = Color;
    return 0;
}

static int OnPaint(HWND Window)
{
    HRGN Region;
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = BeginPaint(Window, &ps);
    if (hdc == NULL)
    {
        skip("Failed to get device context\n");
        DestroyWindow(Window);
        return 0;
    }

    Region = CreateRectRgn(ps.rcPaint.left,
                           ps.rcPaint.top,
                           ps.rcPaint.right,
                           ps.rcPaint.bottom);

    if (Region == NULL)
    {
        skip("Failed to create drawing region\n");
        EndPaint(Window, &ps);
        DestroyWindow(Window);
        return 0;
    }

    if(!FillRgn(hdc, Region, ColorBrushes[CurrentColor]))
    {
        skip("Failed to paint the window\n");
        DestroyWindow(Window);
    }

    DeleteObject(Region);
    EndPaint(Window, &ps);

    return 0;
}

static LRESULT CALLBACK WindowProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
    RECT Rect;
    int NewWidth;
    int NewHeight;

    switch (Message)
    {
        case WM_CREATE:
            /* It's important for the test that the entire Window is visible. */
            if (!SetWindowPos(Window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE))
            {
                skip("Failed to set window as top-most, code: %ld\n", GetLastError());
                return -1;
            }

            if (!GetClientRect(Window, &Rect))
            {
                skip("Failed to retrieve client area dimensions, code: %ld\n", GetLastError());
                return -1;
            }
            ClientWidth = Rect.right;
            ClientHeight = Rect.bottom;

            if (!GetWindowRect(Window, &Rect))
            {
                skip("Failed to retrieve window dimensions, code: %ld\n", GetLastError());
                return -1;
            }
            OrigWidth  = Rect.right - Rect.left;
            OrigHeight = Rect.bottom - Rect.top;

            SmallWidth  = max((OrigWidth  * 3) / 4, 1);
            SmallHeight = max((OrigHeight * 3) / 4, 1);
            OrigWidth   = max(OrigWidth,  SmallWidth  + 1);
            OrigHeight  = max(OrigHeight, SmallHeight + 1);

            trace("OrigWidth: %d, OrigHeight: %d, SmallWidth: %d, SmallHeight: %d\n",
                  OrigWidth, OrigHeight, SmallWidth, SmallHeight);

            HideVertScrollBar(Window);
            HideHorzScrollBar(Window);

            FsmState = FSM_STATE_START;
            FsmTimer = 0;

            return 0;

        case WM_PAINT:
            if (FsmTimer == 0)
            {
                FsmTimer = SetTimer(Window, 1, FSM_STEP_PERIOD_MS, NULL);

                if (FsmTimer == 0)
                {
                    skip("Failed to initialize FSM timer, code: %ld\n", GetLastError());
                    DestroyWindow(Window);
                    return 0;
                }
            }
            OnPaint(Window);
            break;

        case WM_SIZE:
            NewWidth = LOWORD(lParam);
            NewHeight = HIWORD(lParam);

            if (NewWidth != 0 && NewHeight != 0 &&
                (NewWidth != ClientWidth || NewHeight != ClientHeight))
            {
                CurrentColor = (CurrentColor + 1) % TEST_COLOR_COUNT;
                SetClassLongPtr(Window,
                                GCLP_HBRBACKGROUND,
                                (LONG_PTR)ColorBrushes[CurrentColor]);

                trace("New window size: %d x %d, new color: 0x%.8lX\n",
                      NewWidth, NewHeight, Colors[CurrentColor]);

                ClientWidth = NewWidth;
                ClientHeight = NewHeight;
            }
            return 0;

        case WM_ERASEBKGND:
            return 1; /* We use WM_PAINT instead. */

        case WM_TIMER:
            if (wParam != 0 && wParam == FsmTimer)
            {
                return FsmStep(Window);
            }
            break;

        case WM_NCDESTROY:
            if (FsmTimer != 0)
            {
                KillTimer(Window, FsmTimer);
                FsmTimer = 0;
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(Window, Message, wParam, lParam);
}
