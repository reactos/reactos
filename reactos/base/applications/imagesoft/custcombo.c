#include <precomp.h>


LRESULT WINAPI
FlatComboProc(HWND hwnd,
              UINT msg,
              WPARAM wParam,
              LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT    ps;
    RECT rect, rect2;
    POINT pt;

    WNDPROC OldComboProc = (WNDPROC)GetWindowLong(hwnd, GWL_USERDATA);

    static BOOL fMouseDown  = FALSE;
    static BOOL fButtonDown = FALSE;

    switch(msg)
    {
        case WM_PAINT:
        {
            if(wParam == 0) hdc = BeginPaint(hwnd, &ps);
            else hdc = (HDC)wParam;

            /* mask off the borders and draw ComboBox normally */
            GetClientRect(hwnd, &rect);

            InflateRect(&rect,
                        -GetSystemMetrics(SM_CXEDGE)*2,
                        -GetSystemMetrics(SM_CYEDGE)*2);

            rect.right -= GetSystemMetrics(SM_CXVSCROLL);

            IntersectClipRect(hdc,
                              rect.left,
                              rect.top,
                              rect.right,
                              rect.bottom);

            /* Draw the ComboBox */
            CallWindowProc(OldComboProc,
                           hwnd,
                           msg,
                           (WPARAM)hdc,
                           lParam);

            /* Now mask off inside and draw the borders */
            SelectClipRgn(hdc,
                          NULL);
            rect.right += GetSystemMetrics(SM_CXVSCROLL);

            ExcludeClipRect(hdc,
                            rect.left,
                            rect.top,
                            rect.right,
                            rect.bottom);

            /* draw borders */
            GetClientRect(hwnd,
                          &rect2);
            FillRect(hdc,
                     &rect2,
                     //CreateSolidBrush(RGB(0,0,0)));
                     GetSysColorBrush(COLOR_3DFACE));

            /* now draw the button */
            SelectClipRgn(hdc,
                          NULL);
            rect.left = rect.right - GetSystemMetrics(SM_CXVSCROLL);

            if(fButtonDown)
            {
                HBRUSH oldBrush;
                HPEN oldPen;
                POINT pt[3];

                FillRect(hdc, &rect, CreateSolidBrush(RGB(182,189,210)));
                rect.top -= 1;
                rect.bottom += 1;
                FrameRect(hdc, &rect, GetStockBrush(WHITE_BRUSH));

                pt[0].x = rect.right - ((GetSystemMetrics(SM_CXVSCROLL) / 2) + 2);
                pt[0].y = rect.bottom / 2;
                pt[1].x = pt[0].x + 4;
                pt[1].y = pt[0].y;
                pt[2].x = pt[1].x - 2;
                pt[2].y = pt[1].y + 2;

                oldPen = (HPEN) SelectObject(hdc, GetStockPen(WHITE_PEN));
                oldBrush = (HBRUSH) SelectObject(hdc, GetStockBrush(WHITE_BRUSH));
                Polygon(hdc, pt, 3);

                SelectObject(hdc, oldPen);
                SelectObject(hdc, oldBrush);
            }
            else
            {
                HBRUSH oldBrush;
                POINT pt[3];

                FillRect(hdc, &rect, GetSysColorBrush(COLOR_3DFACE));
                rect.top -= 1;
                rect.bottom += 1;
                FrameRect(hdc, &rect, GetStockBrush(WHITE_BRUSH));

                pt[0].x = rect.right - ((GetSystemMetrics(SM_CXVSCROLL) / 2) + 2);
                pt[0].y = rect.bottom / 2;
                pt[1].x = pt[0].x + 4;
                pt[1].y = pt[0].y;
                pt[2].x = pt[1].x - 2;
                pt[2].y = pt[1].y + 2;

                oldBrush = (HBRUSH) SelectObject(hdc, GetStockBrush(BLACK_BRUSH));
                Polygon(hdc, pt, 3);

                SelectObject(hdc, oldBrush);
            }


            if(wParam == 0)
                EndPaint(hwnd, &ps);

            return 0;
        }

        /* check if mouse is within drop-arrow area, toggle
         * a flag to say if the mouse is up/down. Then invalidate
         * the window so it redraws to show the changes. */
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        {

            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);

            GetClientRect(hwnd, &rect);

            InflateRect(&rect,
                        -GetSystemMetrics(SM_CXEDGE),
                        -GetSystemMetrics(SM_CYEDGE));
            rect.left = rect.right - GetSystemMetrics(SM_CXVSCROLL);

            if(PtInRect(&rect, pt))
            {
                /* we *should* call SetCapture, but the ComboBox does it for us */
                fMouseDown = TRUE;
                fButtonDown = TRUE;
                InvalidateRect(hwnd, 0, 0);
            }
        }
        break;

        /* mouse has moved. Check to see if it is in/out of the drop-arrow */
        case WM_MOUSEMOVE:
        {

            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);

            if(fMouseDown && (wParam & MK_LBUTTON))
            {
                GetClientRect(hwnd, &rect);

                InflateRect(&rect, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE));
                rect.left = rect.right - GetSystemMetrics(SM_CXVSCROLL);

                if(fButtonDown != PtInRect(&rect, pt))
                {
                    fButtonDown = PtInRect(&rect, pt);
                    InvalidateRect(hwnd, 0, 0);
                }
            }
        }
        break;

        case WM_LBUTTONUP:
        {

            if(fMouseDown)
            {
                /* No need to call ReleaseCapture, the ComboBox does it for us */
                fMouseDown = FALSE;
                fButtonDown = FALSE;
                InvalidateRect(hwnd, 0, 0);
            }
        }
        break;
    }

    return CallWindowProc(OldComboProc,
                          hwnd,
                          msg,
                          wParam,
                          lParam);
}

VOID MakeFlatCombo(HWND hwndCombo)
{
    LONG OldComboProc;

    /* Remember old window procedure */
    OldComboProc = GetWindowLongPtr(hwndCombo, GWL_WNDPROC);
    SetWindowLongPtr(hwndCombo,
                     GWL_USERDATA,
                     OldComboProc);

    /* Perform the subclass */
    SetWindowLongPtr(hwndCombo,
                     GWL_WNDPROC,
                     (LONG_PTR)FlatComboProc);
}
