/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/sizebox.h
 * PURPOSE:     Window procedure of the size boxes
 * PROGRAMMERS: Benedikt Freisen
 */

class CSizeboxWindow : public CWindowImpl<CSizeboxWindow>
{
public:
    DECLARE_WND_CLASS_EX(_T("Sizebox"), CS_DBLCLKS, COLOR_HIGHLIGHT)

    BEGIN_MSG_MAP(CSizeboxWindow)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    END_MSG_MAP()

    LRESULT OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
