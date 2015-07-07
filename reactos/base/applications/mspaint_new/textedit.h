/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/textedit.h
 * PURPOSE:     Text editor and font chooser for the text tool
 * PROGRAMMERS: Benedikt Freisen
 */

class CTextEditWindow : public CWindowImpl<CTextEditWindow>
{
public:
    DECLARE_WND_CLASS_EX(_T("TextEdit"), CS_DBLCLKS, COLOR_BTNFACE)

    BEGIN_MSG_MAP(CPaletteWindow)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    END_MSG_MAP()

    LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
