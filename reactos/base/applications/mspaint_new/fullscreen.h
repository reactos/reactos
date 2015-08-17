/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/fullscreen.h
 * PURPOSE:     Window for fullscreen view
 * PROGRAMMERS: Benedikt Freisen
 */

class CFullscreenWindow : public CWindowImpl<CFullscreenWindow>
{
public:
    DECLARE_WND_CLASS_EX(_T("FullscreenWindow"), CS_DBLCLKS, COLOR_BACKGROUND)

    BEGIN_MSG_MAP(CFullscreenWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_CLOSE, OnCloseOrKeyDownOrLButtonDown)
        MESSAGE_HANDLER(WM_KEYDOWN, OnCloseOrKeyDownOrLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnCloseOrKeyDownOrLButtonDown)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        MESSAGE_HANDLER(WM_GETTEXT, OnGetText)
    END_MSG_MAP()

    LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCloseOrKeyDownOrLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetText(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
