/*
 * PROJECT:                 ReactOS Software Control Panel
 * FILE:                    dll/cpl/appwiz/removestartmenuitems.c
 * PURPOSE:                 ReactOS Software Control Panel
 * PROGRAMMERS:             Jared Smudde (computerwhiz02@hotmail.com)
 */

#include "appwiz.h"

INT_PTR CALLBACK RemoveStartMenuItemsProc(HWND hwndCPl, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
        case WM_INITDIALOG:
        return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndCPl, IDOK);
                break;
                case IDCANCEL:
                    EndDialog(hwndCPl, IDCANCEL);
                break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}

LONG CALLBACK
ShowRemoveStartMenuItems(HWND hwndCPl, LPWSTR szPath)
{
    DialogBox(hApplet, MAKEINTRESOURCEW(IDD_CONFIG_STARTMENU), hwndCPl, RemoveStartMenuItemsProc);
    return TRUE;
}

LONG
CALLBACK
ConfigStartMenu(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    return ShowRemoveStartMenuItems(hwndCPl, (LPWSTR) lParam1);
}
