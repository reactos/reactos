#include "priv.h"
#include "fadetsk.h"
#include "apithk.h"

/// Fade Rect Support
// {2DECD184-21B0-11d2-8385-00C04FD918D0}
const GUID TASKID_Fader = 
{ 0x2decd184, 0x21b0, 0x11d2, { 0x83, 0x85, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0 } };

CFadeTask::CFadeTask() : CRunnableTask(RTF_DEFAULT)
{
    ASSERT(g_bRunOnNT5);    // This should only get created on NT5
    WNDCLASSEX wc = {0};

    if (!GetClassInfoEx(g_hinst, TEXT("SysFader"), &wc)) 
    {
        wc.cbSize          = sizeof(wc);
        wc.lpfnWndProc     = DefWindowProc;
        wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
        wc.hInstance       = g_hinst;
        wc.lpszClassName   = TEXT("SysFader");
        wc.hbrBackground   = (HBRUSH)(COLOR_BTNFACE + 1); // NULL;

        if (!RegisterClassEx(&wc))
           return;
    }

    _hwndFader = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | 
                            WS_EX_TOPMOST | WS_EX_TOOLWINDOW, 
                            TEXT("SysFader"), TEXT("SysFader"),
                            WS_POPUP,
                            0, 0, 0, 0, NULL, (HMENU) 0, 
                            g_hinst, NULL);
}

CFadeTask::~CFadeTask()
{
    if (_hwndFader)
        DestroyWindow(_hwndFader);
}

#define ALPHASTART (200)

BOOL CFadeTask::FadeRect(PRECT prc, PFNFADESCREENRECT pfn, LPVOID pvParam)
{
    if (IsRunning() == IRTIR_TASK_RUNNING)
        return FALSE;

    InterlockedExchange(&_lState, IRTIR_TASK_NOT_RUNNING);

    _rect = *prc;
    _pfn = pfn;
    _pvParam = pvParam;

    POINT   pt;
    POINT   ptSrc = {0, 0};
    SIZE    size;

    // prc and pt are in screen coordinates.
    pt.x = _rect.left;
    pt.y = _rect.top;

    // Get the size of the rectangle for the blits.
    size.cx = RECTWIDTH(_rect);
    size.cy = RECTHEIGHT(_rect);

    // Get the DC for the screen and window.
    HDC hdcScreen = GetDC(NULL);
    HDC hdcWin = GetDC(_hwndFader);
    if (hdcWin && hdcScreen)
    {
        // If we don't have a HDC for the fade, then create one.
        if (!_hdcFade)
        {
            _hdcFade = CreateCompatibleDC(hdcScreen);
            if (!_hdcFade)
                goto Stop;

            // Create a bitmap that covers the fade region, instead of the whole screen.
            _hbm = CreateCompatibleBitmap(hdcScreen, size.cx, size.cy);
            if (!_hbm)
                goto Stop;

            // select it in, saving the old bitmap's handle
            _hbmOld = (HBITMAP)SelectBitmap(_hdcFade, _hbm);
        }

        // Get the stuff from the screen and squirt it into the fade dc.
        BitBlt(_hdcFade, 0, 0, size.cx, size.cy, hdcScreen, pt.x, pt.y, SRCCOPY);

        // Now let user do it's magic. We're going to mimic user and start with a slightly
        // faded, instead of opaque, rendering (Looks smoother and cleaner.
        BlendLayeredWindow(_hwndFader, hdcWin, &pt, &size, _hdcFade, &ptSrc, ALPHASTART);

        // Now that we have it all build up, display it on screen.
        SetWindowPos(_hwndFader, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
Stop:
        ReleaseDC(NULL, hdcScreen);
        ReleaseDC(_hwndFader, hdcWin);
    }

    if (_pfn)
        _pfn(FADE_BEGIN, _pvParam);

    return TRUE;
}



#define FADE_TIMER_ID 10
#define FADE_TIMER_TIMEOUT 10 // milliseconds
#define FADE_TIMEOUT 350 // milliseconds
#define FADE_ITERATIONS 35
#define QUAD_PART(a) ((a)##.QuadPart)

void CFadeTask::_StopFade()
{
    if (_hwndFader)
    {

        ShowWindow(_hwndFader, SW_HIDE);
    }
    if (_pfn)
        _pfn(FADE_END, _pvParam);

    if (_hdcFade)
    {
        if (_hbmOld)
        {
            SelectBitmap(_hdcFade, _hbmOld);
        }
        DeleteDC(_hdcFade);
        _hdcFade = NULL;
    }
    
    if (_hbm)
    {
        DeleteObject(_hbm);
        _hbm = NULL;
    }
}
 
STDMETHODIMP CFadeTask::RunInitRT(void)
{
    BOOL    fRet = FALSE;
    LARGE_INTEGER liDiff;
    LARGE_INTEGER liFreq;
    LARGE_INTEGER liStart;
    DWORD dwElapsed;
    BYTE bBlendConst;

    // Start the fade timer and the count-down for the fade.
    QueryPerformanceFrequency(&liFreq);
    QueryPerformanceCounter(&liStart);

    // Do this until the conditions specified in the loop.
    while ( TRUE )
    {
        // Calculate the elapsed time in milliseconds.
        QueryPerformanceCounter(&liDiff);
        QUAD_PART(liDiff) -= QUAD_PART(liStart);
        dwElapsed = (DWORD)((QUAD_PART(liDiff) * 1000) / QUAD_PART(liFreq));

        if (dwElapsed >= FADE_TIMEOUT) 
        {
            goto Stop;
        }

        bBlendConst = (BYTE)(ALPHASTART * (FADE_TIMEOUT - 
                dwElapsed) / FADE_TIMEOUT);

        if (bBlendConst <= 1) 
        {
            goto Stop;
        }

        // Since only the alpha is updated, there is no need to pass
        // anything but the new alpha function. This saves a source copy.
        BlendLayeredWindow(_hwndFader, NULL, NULL, NULL, NULL, NULL, bBlendConst);
        Sleep(FADE_TIMER_TIMEOUT);
    }

Stop:
    _StopFade();
    return S_OK;
}
