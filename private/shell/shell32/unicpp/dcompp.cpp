/*  DCOMPP.CPP
**
**  Copyright (C) Microsoft, 1997, All Rights Reserved.
**
**  window class to display a preview of the desktop components,
**
*/

#include "stdafx.h"
#pragma hdrstop

#ifdef POSTSPLIT

#define THISCLASS CCompPreview

class CCompPreview
{
public:
protected:
    HWND _hwnd;
    HBITMAP _hbmMonitor;
    HDC _hdcCompMemory;
    int _iScreenWidth;
    int _iScreenHeight;
    int _iXBorders;
    int _iYBorders;

    static LRESULT CALLBACK CompPreviewWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    friend BOOL RegisterCompPreviewClass(void);

    LONG _OnCreate(HWND hwnd);
    void _OnDestroy(void);
    void _OnPaint(void);
    void _RecalcMetrics(void);
};

void THISCLASS::_RecalcMetrics(void)
{
    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, FALSE);
    _iScreenWidth = rect.right - rect.left;
    _iScreenHeight = rect.bottom - rect.top;
    _iXBorders = (2 * GET_CXSIZE);
    _iYBorders = (GET_CYSIZE + GET_CYCAPTION);
}

LONG THISCLASS::_OnCreate(HWND hwnd)
{
    LONG lRet = 0;

    _hwnd = hwnd;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);

    HDC hdc = GetDC(NULL);
    _hdcCompMemory = CreateCompatibleDC(hdc);
    ReleaseDC(NULL, hdc);

    _hbmMonitor = LoadMonitorBitmap();

    if (_hbmMonitor == NULL)
    {
        lRet = -1;
    }

    _RecalcMetrics();  //Initialize the screen width and height etc.,

    return lRet;
}

void THISCLASS::_OnDestroy()
{
    if (_hbmMonitor)
    {
        DeleteObject(_hbmMonitor);
    }
    if (_hdcCompMemory)
    {
        DeleteDC(_hdcCompMemory);
    }
    delete this;
}

void THISCLASS::_OnPaint()
{
    PAINTSTRUCT     ps;
    BITMAP          bm;
    RECT            rc;

    BeginPaint(_hwnd,&ps);
    if (_hbmMonitor)
    {
        DWORD dwDefWidth = (_iScreenWidth / (COMPONENT_PER_ROW + 1)) - _iXBorders;
        DWORD dwDefHeight = (_iScreenHeight / (COMPONENT_PER_COL + 1)) - _iYBorders;

        //
        // Select the monitor bitmap into an hdc.
        //
        HBITMAP hbmOld = (HBITMAP)SelectObject(_hdcCompMemory, _hbmMonitor);

        //
        // Get the size of the bitmap and of our window.
        //
        GetClientRect(_hwnd, &rc);
        GetObject(_hbmMonitor, sizeof(bm), &bm);

        //
        // Center the bitmap in the window.
        //
        rc.left = ( rc.right - bm.bmWidth ) / 2;
        rc.top = ( rc.bottom - bm.bmHeight ) / 2;
        BitBlt(ps.hdc, rc.left, rc.top, bm.bmWidth, bm.bmHeight, _hdcCompMemory,
            0, 0, SRCCOPY);

        SelectObject(_hdcCompMemory, hbmOld);

        COMPONENTSOPT co = { SIZEOF(co) };
        g_pActiveDesk->GetDesktopItemOptions(&co, 0);
        if (co.fActiveDesktop)
        {
            //
            // From now on, only paint in the "monitor" area of the bitmap.
            //
            IntersectClipRect(ps.hdc, rc.left + MON_X, rc.top + MON_Y, rc.left + MON_X + MON_DX, rc.top + MON_Y + MON_DY);

            //
            // Determine who the selected component is.
            //
            int iSelectedComponent;
            SendMessage(GetParent(_hwnd), WM_COMP_GETCURSEL, 0, (LPARAM)&iSelectedComponent);

            //
            // Create two new brush/pen combos, and remember the original
            // brush & pen.
            //
            HBRUSH hbrushActComp = CreateSolidBrush(GetSysColor(COLOR_ACTIVECAPTION));
            HPEN hpenActComp = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_CAPTIONTEXT));

            HBRUSH hbrushComp = CreateSolidBrush(GetSysColor(COLOR_INACTIVECAPTION));
            HPEN hpenComp = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_INACTIVECAPTIONTEXT));

            HBRUSH hbrushOld = (HBRUSH)SelectObject(ps.hdc, hbrushComp);
            HPEN hpenOld = (HPEN)SelectObject(ps.hdc, hpenComp);

            int iPrimaryMonitorX = -GetSystemMetrics(SM_XVIRTUALSCREEN);
            int iPrimaryMonitorY = -GetSystemMetrics(SM_YVIRTUALSCREEN);
            int iPrimaryMonitorCX = GetSystemMetrics(SM_CXSCREEN);
            int iPrimaryMonitorCY = GetSystemMetrics(SM_CYSCREEN);
            //
            // Draw each component in the "monitor" area of the bitmap.
            //
            int i, cComp;
            g_pActiveDesk->GetDesktopItemCount(&cComp, 0);
            for (i=0; i < cComp; i++)
            {
                COMPONENT comp;
                comp.dwSize = sizeof(COMPONENT);
                if (SUCCEEDED(g_pActiveDesk->GetDesktopItem(i, &comp, 0)) && (comp.fChecked))
                {
                    // BUGBUG: We show only components in the primary monitor in IE v4.01
                    if (comp.cpPos.iLeft < iPrimaryMonitorX
                            || comp.cpPos.iLeft > iPrimaryMonitorX + iPrimaryMonitorCX
                            || comp.cpPos.iTop < iPrimaryMonitorY
                            || comp.cpPos.iTop > iPrimaryMonitorY + iPrimaryMonitorCY)
                    {
                        continue;
                    }

                    //BUGBUG: If the width or Height is -1, then we don't know what the actual
                    // size is going to be. So, we try to give a default size here for comp
                    // in the preview bitmap.
                    DWORD dwCompWidth = (comp.cpPos.dwWidth == COMPONENT_DEFAULT_WIDTH)? dwDefWidth : comp.cpPos.dwWidth;
                    DWORD dwCompHeight = (comp.cpPos.dwHeight == COMPONENT_DEFAULT_HEIGHT)? dwDefHeight : comp.cpPos.dwHeight;

                    if (i == iSelectedComponent)
                    {
                        SelectObject(ps.hdc, hbrushActComp);
                        SelectObject(ps.hdc, hpenActComp);
                    }

                    int nLeft = rc.left + MON_X + MulDiv(comp.cpPos.iLeft - iPrimaryMonitorX, MON_DX, GetDeviceCaps(_hdcCompMemory, HORZRES));
                    int nTop = rc.top + MON_Y + MulDiv(comp.cpPos.iTop - iPrimaryMonitorY, MON_DY, GetDeviceCaps(_hdcCompMemory, VERTRES));
                    int nRight = rc.left + MON_X + MulDiv((comp.cpPos.iLeft - iPrimaryMonitorX) + dwCompWidth, MON_DX, GetDeviceCaps(_hdcCompMemory, HORZRES));
                    int nBottom = rc.top + MON_Y + MulDiv((comp.cpPos.iTop - iPrimaryMonitorY)+ dwCompHeight, MON_DY, GetDeviceCaps(_hdcCompMemory, VERTRES));

                    Rectangle(ps.hdc, nLeft, nTop, nRight, nBottom);

                    if (i == iSelectedComponent)
                    {
                        SelectObject(ps.hdc, hbrushComp);
                        SelectObject(ps.hdc, hpenComp);
                    }
                }
            }

            SelectObject(ps.hdc, hpenOld);
            SelectObject(ps.hdc, hbrushOld);

            DeleteObject(hpenComp);
            DeleteObject(hbrushComp);
            DeleteObject(hpenActComp);
            DeleteObject(hbrushActComp);
        }
    }

    EndPaint(_hwnd,&ps);
}

LRESULT CALLBACK THISCLASS::CompPreviewWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CCompPreview *pcp = (CCompPreview *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(message)
    {
    case WM_CREATE:
        pcp = new CCompPreview();
        return pcp ? pcp->_OnCreate(hwnd) : -1;

    case WM_DESTROY:
        pcp->_OnDestroy();
        break;

    case WM_PAINT:
        pcp->_OnPaint();
        return 0;

    case WM_DISPLAYCHANGE:
    case WM_WININICHANGE:
        pcp->_RecalcMetrics();
        break;

//  98/09/01 vtan #190588: WM_SYSCOLORCHANGE is passed when the desktop
//  background color is changed. This message is passed to the property
//  sheet common control which sends the message through to all the
//  children. The message is now processed here. The old monitor background
//  bitmap is discarded and a new one created with the current (new)
//  setting.

    case WM_SYSCOLORCHANGE:
        if (pcp->_hbmMonitor != NULL)
        {
            DeleteObject(pcp->_hbmMonitor);
            pcp->_hbmMonitor = LoadMonitorBitmap();
        }
        break;
    }
    return DefWindowProc(hwnd,message,wParam,lParam);
}

BOOL RegisterCompPreviewClass(void)
{
    WNDCLASS wc;

    if (!GetClassInfo(HINST_THISDLL, c_szComponentPreview, &wc)) {
        wc.style = 0;
        wc.lpfnWndProc = THISCLASS::CompPreviewWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = HINST_THISDLL;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = c_szComponentPreview;

        if (!RegisterClass(&wc))
            return FALSE;
    }

    return TRUE;
}
#endif
