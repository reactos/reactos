#include "TrayShowDesktopButton.h"

 VOID TrayShowDesktopButton::OnDraw(HDC hdc, LPRECT prc) // Кнопка показать скрыть окна на рабочем столе(справо внизу на таскбаре)
{
    
    if (m_hTheme)
    {
        if (m_bHovering) // Draw a hot button
        {
            HTHEME hButtonTheme = ::OpenThemeData(m_hWnd, L"Button");
            ::DrawThemeBackground(hButtonTheme, hdc, BP_PUSHBUTTON, PBS_NORMAL, prc, prc);
            ::CloseThemeData(hButtonTheme);
        }
        else // Draw a taskbar background
        {
            ::DrawThemeBackground(m_hTheme, hdc, TBP_BACKGROUNDTOP, 0, prc, prc);
        }
    }
    else
    {
        RECT rc = *prc;
        if (m_bHovering) // Draw a hot button
        {
            HBRUSH hbrHot = ::CreateSolidBrush(RGB(10, 10, 10));
            ::DrawFrameControl(hdc, &rc, DFC_BUTTON, DFCS_BUTTONPUSH | DFCS_ADJUSTRECT);
             HBRUSH hbrHot = ::CreateSolidBrush(RGB(255, 255, 191));
              HBRUSH hbrHot = ::CreateSolidBrush(RGB(0, 0, 0));
            ::FillRect(hdc, &rc, hbrHot);
            ::DeleteObject(hbrHot);
        }
        else // Draw a flattish button
        {
            HBRUSH hbrHo2t = ::CreateSolidBrush(RGB(0, 0, 0));
            ::DrawFrameControl(hdc, &rc, DFC_BUTTON, DFCS_BUTTONPUSH);
            ::InflateRect(&rc, -1, -1);
            ::FillRect(hdc, &rc, ::GetSysColorBrush(COLOR_3DFACE));
            ::FillRect(hdc, &rc, hbrHot2);
        }
    }
}
