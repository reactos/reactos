/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/toolbox.cpp
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

LRESULT CToolBox::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HIMAGELIST hImageList;
    HBITMAP tempBm;
    int i;
    TCHAR tooltips[16][30];

    /*
     * FIXME: Unintentionally there is a line above the tool bar (hidden by y-offset).
     * To prevent cropping of the buttons height has been increased from 200 to 205
     */
    RECT toolbarPos = {1, -2, 1 + 50, -2 + 205};
    toolbar.Create(TOOLBARCLASSNAME, m_hWnd, toolbarPos, NULL,
                   WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | CCS_VERT | CCS_NORESIZE | TBSTYLE_TOOLTIPS);
    hImageList = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 16, 0);
    toolbar.SendMessage(TB_SETIMAGELIST, 0, (LPARAM) hImageList);
    tempBm = (HBITMAP) LoadImage(hProgInstance, MAKEINTRESOURCE(IDB_TOOLBARICONS), IMAGE_BITMAP, 256, 16, 0);
    ImageList_AddMasked(hImageList, tempBm, 0xff00ff);
    DeleteObject(tempBm);
    toolbar.SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    for(i = 0; i < 16; i++)
    {
        TBBUTTON tbbutton;
        int wrapnow = 0;

        if (i % 2 == 1)
            wrapnow = TBSTATE_WRAP;

        LoadString(hProgInstance, IDS_TOOLTIP1 + i, tooltips[i], 30);
        ZeroMemory(&tbbutton, sizeof(TBBUTTON));
        tbbutton.iString   = (INT_PTR) tooltips[i];
        tbbutton.fsStyle   = TBSTYLE_CHECKGROUP;
        tbbutton.fsState   = TBSTATE_ENABLED | wrapnow;
        tbbutton.idCommand = ID_FREESEL + i;
        tbbutton.iBitmap   = i;
        toolbar.SendMessage(TB_ADDBUTTONS, 1, (LPARAM) &tbbutton);
    }

    toolbar.SendMessage(TB_CHECKBUTTON, ID_PEN, MAKELPARAM(TRUE, 0));
    toolbar.SendMessage(TB_SETMAXTEXTROWS, 0, 0);
    toolbar.SendMessage(TB_SETBUTTONSIZE, 0, MAKELPARAM(25, 25));

    return 0;
}

LRESULT CToolBox::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return 0;
}

LRESULT CToolBox::OnSysColorChange(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    toolbar.SendMessage(WM_SYSCOLORCHANGE, 0, 0);
    return 0;
}

LRESULT CToolBox::OnCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    switch (LOWORD(wParam))
    {
        case ID_FREESEL:
            toolsModel.SetActiveTool(TOOL_FREESEL);
            break;
        case ID_RECTSEL:
            toolsModel.SetActiveTool(TOOL_RECTSEL);
            break;
        case ID_RUBBER:
            toolsModel.SetActiveTool(TOOL_RUBBER);
            break;
        case ID_FILL:
            toolsModel.SetActiveTool(TOOL_FILL);
            break;
        case ID_COLOR:
            toolsModel.SetActiveTool(TOOL_COLOR);
            break;
        case ID_ZOOM:
            toolsModel.SetActiveTool(TOOL_ZOOM);
            break;
        case ID_PEN:
            toolsModel.SetActiveTool(TOOL_PEN);
            break;
        case ID_BRUSH:
            toolsModel.SetActiveTool(TOOL_BRUSH);
            break;
        case ID_AIRBRUSH:
            toolsModel.SetActiveTool(TOOL_AIRBRUSH);
            break;
        case ID_TEXT:
            toolsModel.SetActiveTool(TOOL_TEXT);
            break;
        case ID_LINE:
            toolsModel.SetActiveTool(TOOL_LINE);
            break;
        case ID_BEZIER:
            toolsModel.SetActiveTool(TOOL_BEZIER);
            break;
        case ID_RECT:
            toolsModel.SetActiveTool(TOOL_RECT);
            break;
        case ID_SHAPE:
            toolsModel.SetActiveTool(TOOL_SHAPE);
            break;
        case ID_ELLIPSE:
            toolsModel.SetActiveTool(TOOL_ELLIPSE);
            break;
        case ID_RRECT:
            toolsModel.SetActiveTool(TOOL_RRECT);
            break;
    }
    return 0;
}

LRESULT CToolBox::OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    selectionWindow.ShowWindow(SW_HIDE);
    toolsModel.resetTool(); // resets the point-buffer of the polygon and bezier functions
    return 0;
}
