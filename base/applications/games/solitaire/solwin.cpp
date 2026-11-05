/* Solitaire victory animation. */
#include "solitaire.h"

static HWND animationWindow;
static HDC canvas;
static HBITMAP canvasBitmap;
static HGDIOBJ oldBitmap;
static bool startPending, finishPending;
static int canvasWidth, canvasHeight;

struct FlyingCard
{
    int cards[13];
    int next, origin;
    double x, y, vx, vy;
};
static FlyingCard flying[4];

static void PaintCard(HDC dc, int x, int y, int card)
{
    CardBlt(dc, x, y, card);
}

static void LaunchCard(FlyingCard &card)
{
    RECT rect = {card.origin, Y_BORDERWITHFRAME,
                 card.origin + __cardwidth, Y_BORDERWITHFRAME + __cardheight};
    HBRUSH brush = CreateSolidBrush(SolWnd.GetBackColor());
    FillRect(canvas, &rect, brush);
    DeleteObject(brush);
    if (card.next < 12)
        PaintCard(canvas, card.origin, Y_BORDERWITHFRAME, card.cards[card.next + 1]);

    card.x = card.origin;
    card.y = Y_BORDERWITHFRAME;
    card.vx = (4 + rand() % 5) * (__cardwidth / 72.0) * (rand() % 2 ? 1 : -1);
    card.vy = -(2 + rand() % 5) * (__cardheight / 112.0);
}

void StopWinAnimation(bool prompt)
{
    bool wasActive = startPending || finishPending || animationWindow != NULL;
    startPending = false;
    finishPending = false;
    if (animationWindow)
        DestroyWindow(animationWindow);
    if (wasActive || prompt)
    {
        SolWnd.EmptyStacks();
        SolWnd.Redraw();
    }
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
                if (card.y >= canvasHeight - __cardheight && card.vy > 0)
                {
                    card.y = canvasHeight - __cardheight;
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
            InvalidateRect(hwnd, NULL, FALSE);
            if (!active)
                StopWinAnimation(true);
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            StopWinAnimation(true);
            return 0;
        case WM_DESTROY:
            KillTimer(hwnd, 1);
            SelectObject(canvas, oldBitmap);
            DeleteObject(canvasBitmap);
            DeleteDC(canvas);
            canvas = NULL;
            canvasBitmap = NULL;
            animationWindow = NULL;
            InvalidateRect(SolWnd, NULL, FALSE);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

void QueueWinAnimation(void)
{
    startPending = true;
    // Let CardLib finish the last drag/drop before taking a snapshot.
    PostMessage(hwndMain, WM_SOL_WINSTART, 0, 0);
}

void StartWinAnimation(void)
{
    if (!startPending)
        return;
    startPending = false;
    RECT rect;
    GetClientRect(SolWnd, &rect);
    canvasWidth = rect.right;
    canvasHeight = rect.bottom;
    SolWnd.Redraw();
    UpdateWindow(SolWnd);
    HDC dc = GetDC(SolWnd);
    canvas = CreateCompatibleDC(dc);
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
    BitBlt(canvas, 0, 0, canvasWidth, canvasHeight, dc, 0, 0, SRCCOPY);
    ReleaseDC(SolWnd, dc);

    for (int i = 0; i < 4; ++i)
    {
        FlyingCard &card = flying[i];
        for (int j = 0; j < 13; ++j)
            card.cards[j] = pSuitStack[i]->GetCardStack()[j].Idx();
        card.next = 0;
        card.origin = (canvasWidth - (__cardwidth & ~1)) / 2 + i * (__cardwidth + X_SUITSTACK_BORDER);
        LaunchCard(card);
    }

    WNDCLASS wc = {0};
    wc.lpfnWndProc = AnimationProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = TEXT("SolitaireVictory");
    RegisterClass(&wc);
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
