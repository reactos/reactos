/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Window procedure of the main window and all children apart from
 *             hPalWin, hToolSettings and hSelection
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#include "precomp.h"

CToolBox toolBoxContainer;

/* FUNCTIONS ********************************************************/

LRESULT CALLBACK
CPaintToolBar::ToolBarWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldWndProc = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (uMsg == WM_LBUTTONUP)
    {
        // We have to detect clicking on toolbar even if no change of pressed button
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        INT index = (INT)::SendMessageW(hwnd, TB_HITTEST, 0, (LPARAM)&pt);
        if (index >= 0)
        {
            TBBUTTON button;
            if (::SendMessageW(hwnd, TB_GETBUTTON, index, (LPARAM)&button))
                ::PostMessageW(::GetParent(hwnd), WM_COMMAND, button.idCommand, 0);
        }
    }
    return ::CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

BOOL CPaintToolBar::DoCreate(HWND hwndParent)
{
    // NOTE: The horizontal line above the toolbar is hidden by CCS_NODIVIDER style.
    RECT toolbarPos =
    {
        0, 0,
        CX_TOOLBAR + 2 * GetSystemMetrics(SM_CXBORDER),
        CY_TOOLBAR + 2 * GetSystemMetrics(SM_CYBORDER)
    };
    DWORD style = WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | CCS_VERT | CCS_NORESIZE |
                  TBSTYLE_TOOLTIPS | TBSTYLE_FLAT;
    if (!CWindow::Create(TOOLBARCLASSNAMEW, hwndParent, toolbarPos, NULL, style))
        return FALSE;

    HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 16, 0);
    SendMessage(TB_SETIMAGELIST, 0, (LPARAM)hImageList);

    HBITMAP hbmIcons = (HBITMAP)::LoadImageW(g_hinstExe, MAKEINTRESOURCEW(IDB_TOOLBARICONS),
                                             IMAGE_BITMAP, 256, 16, 0);
    ImageList_AddMasked(hImageList, hbmIcons, RGB(255, 0, 255));
    ::DeleteObject(hbmIcons);

    SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    WCHAR szToolTip[30];
    TBBUTTON tbbutton;
    ZeroMemory(&tbbutton, sizeof(tbbutton));
    tbbutton.fsStyle = TBSTYLE_CHECKGROUP;
    for (INT i = 0; i < NUM_TOOLS; i++)
    {
        ::LoadStringW(g_hinstExe, IDS_TOOLTIP1 + i, szToolTip, _countof(szToolTip));
        tbbutton.iString   = (INT_PTR)szToolTip;
        tbbutton.fsState   = TBSTATE_ENABLED | ((i % 2 == 1) ? TBSTATE_WRAP : 0);
        tbbutton.idCommand = ID_FREESEL + i;
        tbbutton.iBitmap   = i;
        SendMessage(TB_ADDBUTTONS, 1, (LPARAM) &tbbutton);
    }

    SendMessage(TB_CHECKBUTTON, ID_PEN, MAKELPARAM(TRUE, 0));
    SendMessage(TB_SETMAXTEXTROWS, 0, 0);
    SendMessage(TB_SETBUTTONSIZE, 0, MAKELPARAM(CXY_TB_BUTTON, CXY_TB_BUTTON));

    SetWindowLongPtr(GWLP_USERDATA, SetWindowLongPtr(GWLP_WNDPROC, (LONG_PTR)ToolBarWndProc));
    return TRUE;
}

BOOL CToolBox::DoCreate(HWND hwndParent)
{
    RECT rcToolBox = { 0, 0, 0, 0 }; // Rely on mainWindow's WM_SIZE
    DWORD style = WS_CHILD | (registrySettings.ShowToolBox ? WS_VISIBLE : 0);
    return !!Create(hwndParent, rcToolBox, NULL, style);
}

LRESULT CToolBox::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    toolbar.DoCreate(m_hWnd);
    toolSettingsWindow.DoCreate(m_hWnd);
    return 0;
}

LRESULT CToolBox::OnSysColorChange(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    toolbar.SendMessage(WM_SYSCOLORCHANGE, 0, 0);
    return 0;
}

struct COMMAND_TO_TOOL
{
    UINT id;
    TOOLTYPE tool;
};

static const COMMAND_TO_TOOL CommandToToolMapping[] =
{
    { ID_FREESEL, TOOL_FREESEL },
    { ID_RECTSEL, TOOL_RECTSEL },
    { ID_RUBBER, TOOL_RUBBER },
    { ID_FILL, TOOL_FILL },
    { ID_COLOR, TOOL_COLOR },
    { ID_ZOOM, TOOL_ZOOM },
    { ID_PEN, TOOL_PEN },
    { ID_BRUSH, TOOL_BRUSH },
    { ID_AIRBRUSH, TOOL_AIRBRUSH },
    { ID_TEXT, TOOL_TEXT },
    { ID_LINE, TOOL_LINE },
    { ID_BEZIER, TOOL_BEZIER },
    { ID_RECT, TOOL_RECT },
    { ID_SHAPE, TOOL_SHAPE },
    { ID_ELLIPSE, TOOL_ELLIPSE },
    { ID_RRECT, TOOL_RRECT },
};
static_assert(_countof(CommandToToolMapping) == TOOL_MAX, "Logical error");

LRESULT CToolBox::OnCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    UINT id = LOWORD(wParam);
    for (size_t i = 0; i < _countof(CommandToToolMapping); ++i)
    {
        if (CommandToToolMapping[i].id == id)
        {
            toolsModel.SetActiveTool(CommandToToolMapping[i].tool);
            break;
        }
    }
    return 0;
}

LRESULT CToolBox::OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    selectionModel.HideSelection();
    toolsModel.resetTool(); // resets the point-buffer of the polygon and bezier functions

    // Check the toolbar button
    TOOLTYPE tool = toolsModel.GetActiveTool();
    for (size_t i = 0; i < _countof(CommandToToolMapping); ++i)
    {
        if (CommandToToolMapping[i].tool == tool)
        {
            toolbar.SendMessage(TB_CHECKBUTTON, CommandToToolMapping[i].id, TRUE);
            break;
        }
    }

    return 0;
}

LRESULT CToolBox::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetCapture();
    return 0;
}

LRESULT CToolBox::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (::GetCapture() != m_hWnd)
        return 0;
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    ClientToScreen(&pt);

    RECT rc;
    mainWindow.GetWindowRect(&rc);

    POINT ptCenter = { (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 };

    DWORD dwExpectedBar2ID = ((pt.x < ptCenter.x) ? BAR2ID_LEFT : BAR2ID_RIGHT);

    if (registrySettings.Bar2ID != dwExpectedBar2ID)
    {
        registrySettings.Bar2ID = dwExpectedBar2ID;
        mainWindow.PostMessage(WM_SIZE, 0, 0);
    }

    return 0;
}

LRESULT CToolBox::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (::GetCapture() != m_hWnd)
        return 0;

    ::ReleaseCapture();
    return 0;
}
