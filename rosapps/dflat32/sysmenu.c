/* ------------- sysmenu.c ------------ */

#include "dflat.h"

int DfSystemMenuProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
    int mx, my;
    DFWINDOW wnd1;
    switch (msg)    {
        case DFM_CREATE_WINDOW:
            wnd->holdmenu = DfActiveMenuBar;
            DfActiveMenuBar = &DfSystemMenu;
            DfSystemMenu.PullDown[0].Selection = 0;
            break;
        case DFM_LEFT_BUTTON:
            wnd1 = DfGetParent(wnd);
            mx = (int) p1 - DfGetLeft(wnd1);
            my = (int) p2 - DfGetTop(wnd1);
            if (DfHitControlBox(wnd1, mx, my))
                return TRUE;
            break;
        case DFM_LB_CHOOSE:
            DfPostMessage(wnd, DFM_CLOSE_WINDOW, 0, 0);
            break;
        case DOUBLE_CLICK:
            if (p2 == DfGetTop(DfGetParent(wnd)))    {
                DfPostMessage(DfGetParent(wnd), msg, p1, p2);
                DfSendMessage(wnd, DFM_CLOSE_WINDOW, TRUE, 0);
            }
            return TRUE;
        case DFM_SHIFT_CHANGED:
            return TRUE;
        case DFM_CLOSE_WINDOW:
            DfActiveMenuBar = wnd->holdmenu;
            break;
        default:
            break;
    }
    return DfDefaultWndProc(wnd, msg, p1, p2);
}

/* ------- Build a system menu -------- */
void DfBuildSystemMenu(DFWINDOW wnd)
{
	int lf, tp, ht, wd;
    DFWINDOW SystemMenuWnd;

    DfSystemMenu.PullDown[0].Selections[6].Accelerator = 
        (DfGetClass(wnd) == DF_APPLICATION) ? DF_ALT_F4 : DF_CTRL_F4;

    lf = DfGetLeft(wnd)+1;
    tp = DfGetTop(wnd)+1;
    ht = DfMenuHeight(DfSystemMenu.PullDown[0].Selections);
    wd = DfMenuWidth(DfSystemMenu.PullDown[0].Selections);

    if (lf+wd > DfGetScreenWidth()-1)
        lf = (DfGetScreenWidth()-1) - wd;
    if (tp+ht > DfGetScreenHeight()-2)
        tp = (DfGetScreenHeight()-2) - ht;

    SystemMenuWnd = DfDfCreateWindow(DF_POPDOWNMENU, NULL,
                lf,tp,ht,wd,NULL,wnd,DfSystemMenuProc, 0);

#ifdef INCLUDE_RESTORE
    if (wnd->condition == DF_SRESTORED)
        DfDeactivateCommand(&DfSystemMenu, DF_ID_SYSRESTORE);
    else
        DfActivateCommand(&DfSystemMenu, DF_ID_SYSRESTORE);
#endif

    if (DfTestAttribute(wnd, DF_MOVEABLE)
#ifdef INCLUDE_MAXIMIZE
            && wnd->condition != DF_ISMAXIMIZED
#endif
                )
        DfActivateCommand(&DfSystemMenu, DF_ID_SYSMOVE);
    else
        DfDeactivateCommand(&DfSystemMenu, DF_ID_SYSMOVE);

    if (wnd->condition != DF_SRESTORED ||
            DfTestAttribute(wnd, DF_SIZEABLE) == FALSE)
        DfDeactivateCommand(&DfSystemMenu, DF_ID_SYSSIZE);
    else
        DfActivateCommand(&DfSystemMenu, DF_ID_SYSSIZE);

#ifdef INCLUDE_MINIMIZE
    if (wnd->condition == DF_ISMINIMIZED ||
            DfTestAttribute(wnd, DF_MINMAXBOX) == FALSE)
        DfDeactivateCommand(&DfSystemMenu, DF_ID_SYSMINIMIZE);
    else
        DfActivateCommand(&DfSystemMenu, DF_ID_SYSMINIMIZE);
#endif

#ifdef INCLUDE_MAXIMIZE
    if (wnd->condition != DF_SRESTORED ||
            DfTestAttribute(wnd, DF_MINMAXBOX) == FALSE)
        DfDeactivateCommand(&DfSystemMenu, DF_ID_SYSMAXIMIZE);
    else
        DfActivateCommand(&DfSystemMenu, DF_ID_SYSMAXIMIZE);
#endif

    DfSendMessage(SystemMenuWnd, DFM_BUILD_SELECTIONS,
                (DF_PARAM) &DfSystemMenu.PullDown[0], 0);
    DfSendMessage(SystemMenuWnd, DFM_SETFOCUS, TRUE, 0);
    DfSendMessage(SystemMenuWnd, DFM_SHOW_WINDOW, 0, 0);
}

/* EOF */
