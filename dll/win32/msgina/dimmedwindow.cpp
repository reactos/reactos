/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/dimmedwindow.cpp
 * PURPOSE:         Implementation of ShellDimScreen
 * PROGRAMMER:      Mark Jansen
 */

#define COM_NO_WINDOWS_H
#include "msgina.h"
#include <wingdi.h>
#include <atlbase.h>
#include <atlcom.h>
#include <pseh/pseh2.h>

CComModule gModule;

// Please note: The INIT_TIMER is a workaround because ReactOS does not redraw the desktop in time,
//              so the start menu is still visible on the dimmed screen.
#define INIT_TIMER_ID   0x112233
#define FADE_TIMER_ID   0x12345

class CDimmedWindow :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    IUnknown
{
private:
    HWND m_hwnd;
    HDC m_hdc;
    HBITMAP m_hbitmap;
    HGDIOBJ m_oldbitmap;
    LONG m_width;
    LONG m_height;
    BITMAPINFO m_bi;
    UCHAR* m_bytes;
    int m_step;

    static LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    CDimmedWindow()
        : m_hwnd(NULL)
        , m_hdc(NULL)
        , m_hbitmap(NULL)
        , m_oldbitmap(NULL)
        , m_width(0)
        , m_height(0)
        , m_bytes(NULL)
        , m_step(0)
    {
        WNDCLASSEXW wndclass = {sizeof(wndclass)};
        wndclass.lpfnWndProc = WndProc;
        wndclass.hInstance = hDllInstance;
        wndclass.hCursor = LoadCursor(0, IDC_ARROW);
        wndclass.lpszClassName = L"DimmedWindowClass";

        if (!RegisterClassExW(&wndclass))
            return;

        m_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        m_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        memset(&m_bi, 0, sizeof(m_bi));
        m_bi.bmiHeader.biSize = sizeof(m_bi);
        m_bi.bmiHeader.biWidth = m_width;
        m_bi.bmiHeader.biHeight = m_height;
        m_bi.bmiHeader.biPlanes = 1;
        m_bi.bmiHeader.biBitCount = 32;
        m_bi.bmiHeader.biCompression = BI_RGB;
        m_bi.bmiHeader.biSizeImage = m_width * 4 * m_height;
        m_bytes = new UCHAR[m_width * 4 * m_height];

        LONG x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        LONG y = GetSystemMetrics(SM_YVIRTUALSCREEN);

        m_hwnd = CreateWindowExW(WS_EX_TOPMOST,
                                 L"DimmedWindowClass",
                                 NULL,
                                 WS_POPUP,
                                 x, y,
                                 m_width, m_height,
                                 NULL, NULL,
                                 hDllInstance,
                                 (LPVOID)this);
    }

    ~CDimmedWindow()
    {
        if (m_hwnd)
            DestroyWindow(m_hwnd);
        UnregisterClassW(L"DimmedWindowClass", hDllInstance);
        if (m_oldbitmap)
             SelectObject(m_hdc, m_oldbitmap);
        if (m_hbitmap)
            DeleteObject(m_hbitmap);
        if (m_hdc)
            DeleteObject(m_hdc);
        if (m_bytes)
            delete[] m_bytes;
    }

    // This is needed so that we do not capture the start menu while it's closing.
    void WaitForInit()
    {
        MSG msg;

        while (IsWindow(m_hwnd) && !IsWindowVisible(m_hwnd))
        {
            while (::PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);

                if (IsWindowVisible(m_hwnd))
                    break;
            }
        }
    }

    void Init()
    {
        Capture();

        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
        EnableWindow(m_hwnd, FALSE);

        SetTimer(m_hwnd, FADE_TIMER_ID, 200, NULL);
    }

    void Capture()
    {
        HWND desktopWnd = GetDesktopWindow();
        HDC desktopDC = GetDC(desktopWnd);

        m_hdc = CreateCompatibleDC(desktopDC);

        m_hbitmap = CreateCompatibleBitmap(desktopDC, m_width, m_height);
        m_oldbitmap = SelectObject(m_hdc, m_hbitmap);
        BitBlt(m_hdc, 0, 0, m_width, m_height, desktopDC, 0, 0, SRCCOPY);

        ReleaseDC(desktopWnd, desktopDC);
    }

    bool Step()
    {
        // Stop after 10 steps
        if (m_step++ > 10 || !m_bytes)
            return false;

        int lines = GetDIBits(m_hdc, m_hbitmap, 0, m_height, m_bytes, &m_bi, DIB_RGB_COLORS);
        if (lines)
        {
            for (int xh = 0; xh < m_height; ++xh)
            {
                int h = m_width * 4 * xh;
                for (int w = 0; w < m_width; ++w)
                {
                    UCHAR b = m_bytes[(h + w * 4) + 0];
                    UCHAR g = m_bytes[(h + w * 4) + 1];
                    UCHAR r = m_bytes[(h + w * 4) + 2];

                    // Standard formula to convert a color.
                    int gray = (r * 30 + g * 59 + b * 11) / 100;
                    if (gray < 0)
                        gray = 0;

                    // Do not fade too fast.
                    r = (r*2 + gray) / 3;
                    g = (g*2 + gray) / 3;
                    b = (b*2 + gray) / 3;

                    m_bytes[(h + w * 4) + 0] = b;
                    m_bytes[(h + w * 4) + 1] = g;
                    m_bytes[(h + w * 4) + 2] = r;
                }
            }
            SetDIBits(m_hdc, m_hbitmap, 0, lines, m_bytes, &m_bi, DIB_RGB_COLORS);
        }
        return true;
    }

    void Blt(HDC hdc)
    {
        BitBlt(hdc, 0, 0, m_width, m_height, m_hdc, 0, 0, SRCCOPY);
    }

    HWND Wnd()
    {
        return m_hwnd;
    }


    BEGIN_COM_MAP(CDimmedWindow)
        COM_INTERFACE_ENTRY_IID(IID_IUnknown, IUnknown)
    END_COM_MAP()

};


LRESULT WINAPI CDimmedWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_NCCREATE:
    {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        CDimmedWindow* info = static_cast<CDimmedWindow*>(lpcs->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)info);
        SetTimer(hWnd, INIT_TIMER_ID, 50, NULL);
        break;
    }

    case WM_PAINT:
    {
        CDimmedWindow* info = reinterpret_cast<CDimmedWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (info)
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            info->Blt(ps.hdc);
            EndPaint(hWnd, &ps);
        }
        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == INIT_TIMER_ID)
        {
            CDimmedWindow* info = reinterpret_cast<CDimmedWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
            KillTimer(hWnd, INIT_TIMER_ID);
            info->Init();
        }
        else if (wParam == FADE_TIMER_ID)
        {
            CDimmedWindow* info = reinterpret_cast<CDimmedWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
            if (info && info->Step())
                InvalidateRect(hWnd, NULL, TRUE);
            else
                KillTimer(hWnd, FADE_TIMER_ID);
        }
        return 0;
    }

    default:
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


extern "C"
HRESULT WINAPI
ShellDimScreen(void** pUnknown, HWND* hWindow)
{
    CComObject<CDimmedWindow> *pWindow;
    HRESULT hr = CComObject<CDimmedWindow>::CreateInstance(&pWindow);
    ULONG refcount;

    pWindow->WaitForInit();

    if (!IsWindow(pWindow->Wnd()))
    {
        refcount = pWindow->AddRef();
        while (refcount)
            refcount = pWindow->Release();

        return E_FAIL;
    }

    _SEH2_TRY
    {
        hr = pWindow->QueryInterface(IID_IUnknown, pUnknown);
        *hWindow = pWindow->Wnd();
        hr = S_OK;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
        refcount = pWindow->AddRef();
        while (refcount)
            refcount = pWindow->Release();
    }
    _SEH2_END

    return hr;
}
