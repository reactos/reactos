/*
 * PROJECT:         input.dll
 * FILE:            dll/cpl/input/advsettings.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 */

#include "input.h"

/* Property page dialog callback */
INT_PTR CALLBACK
AdvancedSettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            break;
        }

        case WM_NOTIFY:
        {
            switch (LOWORD(wParam))
            {
            }

            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            }

            break;
        }

        case WM_DESTROY:
            break;
    }

    return FALSE;
}

/* EOF */
