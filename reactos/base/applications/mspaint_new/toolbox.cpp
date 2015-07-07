/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/toolbox.cpp
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
    tempBm = (HBITMAP) LoadImage((HINSTANCE) GetWindowLong(GWL_HINSTANCE), MAKEINTRESOURCE(IDB_TOOLBARICONS), IMAGE_BITMAP, 256, 16, 0);
    ImageList_AddMasked(hImageList, tempBm, 0xff00ff);
    DeleteObject(tempBm);
    toolbar.SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    for(i = 0; i < 16; i++)
    {
        TBBUTTON tbbutton;
        int wrapnow = 0;

        if (i % 2 == 1)
            wrapnow = TBSTATE_WRAP;

        LoadString((HINSTANCE) GetWindowLong(GWL_HINSTANCE), IDS_TOOLTIP1 + i, tooltips[i], 30);
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
            toolsModel.SetActiveTool(1);
            break;
        case ID_RECTSEL:
            toolsModel.SetActiveTool(2);
            break;
        case ID_RUBBER:
            toolsModel.SetActiveTool(3);
            break;
        case ID_FILL:
            toolsModel.SetActiveTool(4);
            break;
        case ID_COLOR:
            toolsModel.SetActiveTool(5);
            break;
        case ID_ZOOM:
            toolsModel.SetActiveTool(6);
            break;
        case ID_PEN:
            toolsModel.SetActiveTool(7);
            break;
        case ID_BRUSH:
            toolsModel.SetActiveTool(8);
            break;
        case ID_AIRBRUSH:
            toolsModel.SetActiveTool(9);
            break;
        case ID_TEXT:
            toolsModel.SetActiveTool(10);
            break;
        case ID_LINE:
            toolsModel.SetActiveTool(11);
            break;
        case ID_BEZIER:
            toolsModel.SetActiveTool(12);
            break;
        case ID_RECT:
            toolsModel.SetActiveTool(13);
            break;
        case ID_SHAPE:
            toolsModel.SetActiveTool(14);
            break;
        case ID_ELLIPSE:
            toolsModel.SetActiveTool(15);
            break;
        case ID_RRECT:
            toolsModel.SetActiveTool(16);
            break;
    }
    return 0;
}

LRESULT CToolBox::OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    selectionWindow.ShowWindow(SW_HIDE);
    pointSP = 0;                // resets the point-buffer of the polygon and bezier functions
    return 0;
}
