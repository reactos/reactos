/* ------------- sysmenu.c ------------ */

#include "dflat.h"

int SystemMenuProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    int mx, my;
    DFWINDOW wnd1;
    switch (msg)    {
        case CREATE_WINDOW:
            wnd->holdmenu = ActiveMenuBar;
            ActiveMenuBar = &SystemMenu;
            SystemMenu.PullDown[0].Selection = 0;
            break;
        case LEFT_BUTTON:
            wnd1 = GetParent(wnd);
            mx = (int) p1 - GetLeft(wnd1);
            my = (int) p2 - GetTop(wnd1);
            if (HitControlBox(wnd1, mx, my))
                return TRUE;
            break;
        case LB_CHOOSE:
            DfPostMessage(wnd, CLOSE_WINDOW, 0, 0);
            break;
        case DOUBLE_CLICK:
            if (p2 == GetTop(GetParent(wnd)))    {
                DfPostMessage(GetParent(wnd), msg, p1, p2);
                DfSendMessage(wnd, CLOSE_WINDOW, TRUE, 0);
            }
            return TRUE;
        case SHIFT_CHANGED:
            return TRUE;
        case CLOSE_WINDOW:
            ActiveMenuBar = wnd->holdmenu;
            break;
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}

/* ------- Build a system menu -------- */
void BuildSystemMenu(DFWINDOW wnd)
{
	int lf, tp, ht, wd;
    DFWINDOW SystemMenuWnd;

    SystemMenu.PullDown[0].Selections[6].Accelerator = 
        (GetClass(wnd) == APPLICATION) ? ALT_F4 : CTRL_F4;

    lf = GetLeft(wnd)+1;
    tp = GetTop(wnd)+1;
    ht = MenuHeight(SystemMenu.PullDown[0].Selections);
    wd = MenuWidth(SystemMenu.PullDown[0].Selections);

    if (lf+wd > DfGetScreenWidth()-1)
        lf = (DfGetScreenWidth()-1) - wd;
    if (tp+ht > DfGetScreenHeight()-2)
        tp = (DfGetScreenHeight()-2) - ht;

    SystemMenuWnd = DfCreateWindow(POPDOWNMENU, NULL,
                lf,tp,ht,wd,NULL,wnd,SystemMenuProc, 0);

#ifdef INCLUDE_RESTORE
    if (wnd->condition == ISRESTORED)
        DeactivateCommand(&SystemMenu, ID_SYSRESTORE);
    else
        ActivateCommand(&SystemMenu, ID_SYSRESTORE);
#endif

    if (TestAttribute(wnd, MOVEABLE)
#ifdef INCLUDE_MAXIMIZE
            && wnd->condition != ISMAXIMIZED
#endif
                )
        ActivateCommand(&SystemMenu, ID_SYSMOVE);
    else
        DeactivateCommand(&SystemMenu, ID_SYSMOVE);

    if (wnd->condition != ISRESTORED ||
            TestAttribute(wnd, SIZEABLE) == FALSE)
        DeactivateCommand(&SystemMenu, ID_SYSSIZE);
    else
        ActivateCommand(&SystemMenu, ID_SYSSIZE);

#ifdef INCLUDE_MINIMIZE
    if (wnd->condition == ISMINIMIZED ||
            TestAttribute(wnd, MINMAXBOX) == FALSE)
        DeactivateCommand(&SystemMenu, ID_SYSMINIMIZE);
    else
        ActivateCommand(&SystemMenu, ID_SYSMINIMIZE);
#endif

#ifdef INCLUDE_MAXIMIZE
    if (wnd->condition != ISRESTORED ||
            TestAttribute(wnd, MINMAXBOX) == FALSE)
        DeactivateCommand(&SystemMenu, ID_SYSMAXIMIZE);
    else
        ActivateCommand(&SystemMenu, ID_SYSMAXIMIZE);
#endif

    DfSendMessage(SystemMenuWnd, BUILD_SELECTIONS,
                (PARAM) &SystemMenu.PullDown[0], 0);
    DfSendMessage(SystemMenuWnd, SETFOCUS, TRUE, 0);
    DfSendMessage(SystemMenuWnd, SHOW_WINDOW, 0, 0);
}

/* EOF */
