/* Solitaire victory animation. */
#include "solitaire.h"

static HWND animationWindow;
static HDC canvas;
static HBITMAP canvasBitmap;
static DWORD *canvasPixels;
static HGDIOBJ oldBitmap;
static bool startPending, finishPending;
static int canvasWidth, canvasHeight;
static bool desktopAnimation, previewAnimation;
static DWORD animationSpeed, previewStarted;
static HWND previewOwner;
static POINT canvasOrigin;
static int originY, speedRemainder;

bool IsDesktopWinAnimation(void)
{
    return animationWindow != NULL && desktopAnimation;
}

static bool PresentDesktop(void)
{
    SIZE size = {canvasWidth, canvasHeight};
    POINT source = {0, 0};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    return UpdateLayeredWindow(animationWindow, NULL, &canvasOrigin, &size,
                               canvas, &source, 0, &blend, ULW_ALPHA) != FALSE;
}

struct FlyingCard
{
    int cards[13];
    int next, origin, stack;
    double x, y, vx, vy;
};
static FlyingCard flying[4];

static void PaintCard(HDC dc, int x, int y, int card)
{
    CardBlt(dc, x, y, card);
    if (desktopAnimation)
    {
        // GDI does not populate alpha. The same shape used by CardBlt gives
        // opaque card pixels, including black, and leaves rounded corners clear.
        GdiFlush();
        int radius = CardCornerRadius(__cardwidth, __cardheight);
        for (int row = max(0, y); row < min(canvasHeight, y + __cardheight); ++row)
        {
            int edge = min(row - y, y + __cardheight - 1 - row);
            int inset = edge < radius ? CardCornerInset(radius, edge) : 0;
            for (int column = max(0, x + inset);
                 column < min(canvasWidth, x + __cardwidth - inset); ++column)
                canvasPixels[row * canvasWidth + column] |= 0xff000000;
        }
    }
}

static void LaunchCard(FlyingCard &card)
{
    if (desktopAnimation)
    {
        // Previews never modify the real board or its undo history.
        if (!previewAnimation)
        {
            pSuitStack[card.stack]->Pop();
            pSuitStack[card.stack]->Redraw();
        }
    }
    else
    {
        RECT rect = {card.origin, Y_BORDERWITHFRAME,
                     card.origin + __cardwidth, Y_BORDERWITHFRAME + __cardheight};
        HBRUSH brush = CreateSolidBrush(SolWnd.GetBackColor());
        FillRect(canvas, &rect, brush);
        DeleteObject(brush);
        if (card.next < 12)
            PaintCard(canvas, card.origin, Y_BORDERWITHFRAME, card.cards[card.next + 1]);
    }

    card.x = card.origin;
    card.y = originY;
    card.vx = (4 + rand() % 5) * (__cardwidth / 72.0) * (rand() % 2 ? 1 : -1);
    card.vy = -(2 + rand() % 5) * (__cardheight / 112.0);
}

void StopWinAnimation(bool prompt)
{
    bool wasActive = startPending || finishPending || animationWindow != NULL;
    bool wasPreview = previewAnimation;
    if (wasPreview) prompt = false;
    startPending = false;
    finishPending = false;
    if (animationWindow)
        DestroyWindow(animationWindow);
    if (wasActive || prompt)
    {
        if (!wasPreview) SolWnd.EmptyStacks();
        SolWnd.Redraw();
    }
    previewAnimation = false;
    if (wasPreview && IsWindow(previewOwner)) SetFocus(previewOwner);
    if (prompt)
    {
        finishPending = true;
        PostMessage(hwndMain, WM_SOL_WINFINISH, 0, 0);
    }
}

static LRESULT CALLBACK AnimationProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_PAINT:
        {
            if (desktopAnimation)
            {
                ValidateRect(hwnd, NULL);
                return 0;
            }
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            BitBlt(dc, ps.rcPaint.left, ps.rcPaint.top,
                   ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
                   canvas, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_TIMER:
        {
            if (previewAnimation && GetTickCount() - previewStarted >= 5000)
            {
                StopWinAnimation(false);
                return 0;
            }
            // Fixed simulation steps retain the classic trajectory and trail density.
            // At 100 percent this is exactly one original step per timer message.
            speedRemainder += animationSpeed;
            while (speedRemainder >= WIN_SPEED_DEFAULT)
            {
                speedRemainder -= WIN_SPEED_DEFAULT;
                bool active = false;
                double scale = __cardheight / 112.0;
                for (int i = 0; i < 4; ++i)
                {
                    FlyingCard &card = flying[i];
                    if (card.next >= 13)
                        continue;
                    card.x += card.vx;
                    card.y += card.vy;
                    card.vy += 0.55 * scale;
                    int floor = canvasHeight;
                    if (desktopAnimation)
                    {
                        POINT point = {canvasOrigin.x + (LONG)card.x + __cardwidth / 2,
                                       canvasOrigin.y + (LONG)card.y + __cardheight / 2};
                        MONITORINFO monitor = {sizeof(monitor)};
                        if (GetMonitorInfo(MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST), &monitor))
                            floor = monitor.rcMonitor.bottom - canvasOrigin.y;
                    }
                    if (card.y >= floor - __cardheight && card.vy > 0)
                    {
                        card.y = floor - __cardheight;
                        card.vy = -max(card.vy * 0.78, 4.0 * scale);
                    }
                    PaintCard(canvas, (int)card.x, (int)card.y, card.cards[card.next]);
                    if (card.x < -__cardwidth || card.x > canvasWidth)
                    {
                        if (++card.next < 13)
                            LaunchCard(card);
                    }
                    active |= card.next < 13;
                }
                if (!active)
                {
                    StopWinAnimation(true);
                    return 0;
                }
            }
            if (desktopAnimation)
            {
                if (!PresentDesktop())
                    StopWinAnimation(true);
            }
            else
                InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            StopWinAnimation(true);
            return 0;
        case WM_DISPLAYCHANGE:
            StopWinAnimation(false);
            return 0;
        case WM_MOUSEACTIVATE:
            if (desktopAnimation)
                return MA_NOACTIVATE;
            break;
        case WM_DESTROY:
            KillTimer(hwnd, 1);
            SelectObject(canvas, oldBitmap);
            DeleteObject(canvasBitmap);
            DeleteDC(canvas);
            canvas = NULL;
            canvasBitmap = NULL;
            canvasPixels = NULL;
            animationWindow = NULL;
            InvalidateRect(SolWnd, NULL, FALSE);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

void QueueWinAnimation(void)
{
    previewAnimation = false;
    startPending = true;
    // Let CardLib finish the last drag/drop before taking a snapshot.
    PostMessage(hwndMain, WM_SOL_WINSTART, 0, 0);
}

void StartWinAnimation(void)
{
    if (!startPending)
        return;
    startPending = false;
    animationSpeed = dwWinAnimationSpeed;
    RECT rect;
    GetClientRect(SolWnd, &rect);
    int gameWidth = rect.right;
    desktopAnimation = dwWinAnimationMode != WIN_ANIMATION_WINDOW;
    canvasOrigin.x = canvasOrigin.y = 0;
    POINT gameOrigin = {0, 0};
    originY = Y_BORDERWITHFRAME;
    speedRemainder = 0;
    if (desktopAnimation)
    {
        ClientToScreen(SolWnd, &gameOrigin);
        if (dwWinAnimationMode == WIN_ANIMATION_MONITOR)
        {
            MONITORINFO monitor = {sizeof(monitor)};
            if (!GetMonitorInfo(MonitorFromWindow(hwndMain, MONITOR_DEFAULTTONEAREST), &monitor))
            {
                StopWinAnimation(true);
                return;
            }
            rect = monitor.rcMonitor;
        }
        else
        {
            rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
            rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
            rect.right = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
            rect.bottom = rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
        }
        canvasOrigin.x = rect.left;
        canvasOrigin.y = rect.top;
        originY += gameOrigin.y - canvasOrigin.y;
    }
    canvasWidth = rect.right - rect.left;
    canvasHeight = rect.bottom - rect.top;
    SolWnd.Redraw();
    UpdateWindow(SolWnd);
    HDC dc = GetDC(SolWnd);
    canvas = CreateCompatibleDC(dc);
    canvasPixels = NULL;
    if (desktopAnimation)
    {
        BITMAPINFO info = {0};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = canvasWidth;
        info.bmiHeader.biHeight = -canvasHeight;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        canvasBitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS,
                                        (void **)&canvasPixels, NULL, 0);
    }
    else
        canvasBitmap = CreateCompatibleBitmap(dc, canvasWidth, canvasHeight);
    if (!canvas || !canvasBitmap)
    {
        DeleteDC(canvas);
        DeleteObject(canvasBitmap);
        canvas = NULL;
        canvasBitmap = NULL;
        ReleaseDC(SolWnd, dc);
        StopWinAnimation(true);
        return;
    }
    oldBitmap = SelectObject(canvas, canvasBitmap);
    if (desktopAnimation)
    {
        RECT bounds = {0, 0, canvasWidth, canvasHeight};
        HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(canvas, &bounds, brush);
        DeleteObject(brush);
    }
    else
        BitBlt(canvas, 0, 0, canvasWidth, canvasHeight, dc, 0, 0, SRCCOPY);
    ReleaseDC(SolWnd, dc);

    for (int i = 0; i < 4; ++i)
    {
        FlyingCard &card = flying[i];
        for (int j = 0; j < 13; ++j)
            card.cards[j] = previewAnimation ? MAKE_CARD(13 - j, i) : pSuitStack[i]->GetCardStack()[j].Idx();
        card.next = 0;
        card.stack = i;
        card.origin = (gameWidth - (__cardwidth & ~1)) / 2 + i * (__cardwidth + X_SUITSTACK_BORDER);
        if (desktopAnimation)
            card.origin += gameOrigin.x - canvasOrigin.x;
        LaunchCard(card);
    }

    WNDCLASS wc = {0};
    wc.lpfnWndProc = AnimationProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = TEXT("SolitaireVictory");
    RegisterClass(&wc);
    if (desktopAnimation)
        animationWindow = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                         wc.lpszClassName, NULL, WS_POPUP,
                                         canvasOrigin.x, canvasOrigin.y, canvasWidth, canvasHeight,
                                         hwndMain, NULL, wc.hInstance, NULL);
    else
        animationWindow = CreateWindow(wc.lpszClassName, NULL, WS_CHILD | WS_VISIBLE,
                                       0, 0, canvasWidth, canvasHeight, SolWnd, NULL, wc.hInstance, NULL);
    if (!animationWindow)
    {
        SelectObject(canvas, oldBitmap);
        DeleteObject(canvasBitmap);
        DeleteDC(canvas);
        canvas = NULL;
        canvasBitmap = NULL;
        StopWinAnimation(true);
        return;
    }
    if (desktopAnimation)
    {
        if (!PresentDesktop())
        {
            StopWinAnimation(true);
            return;
        }
        ShowWindow(animationWindow, SW_SHOWNOACTIVATE);
    }
    else if (!previewAnimation)
        SetFocus(animationWindow);
    if (!SetTimer(animationWindow, 1, 16, NULL))
        StopWinAnimation(true);
}

void FinishWinAnimation(void)
{
    if (!finishPending)
        return;
    finishPending = false;
    extern TCHAR MsgDeal[128];
    if (MessageBox(hwndMain, MsgDeal, szAppName, MB_YESNO | MB_ICONQUESTION) == IDYES)
        NewGame();
}

void PreviewWinAnimation(DWORD mode, DWORD speed, HWND owner)
{
    StopWinAnimation(false);
    DWORD oldMode = dwWinAnimationMode, oldSpeed = dwWinAnimationSpeed;
    dwWinAnimationMode = mode;
    dwWinAnimationSpeed = speed;
    previewAnimation = true;
    previewStarted = GetTickCount();
    previewOwner = owner;
    startPending = true;
    StartWinAnimation();
    dwWinAnimationMode = oldMode;
    dwWinAnimationSpeed = oldSpeed;
}
