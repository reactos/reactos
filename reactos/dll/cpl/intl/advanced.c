#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "intl.h"
#include "resource.h"

/* Property page dialog callback */
INT_PTR CALLBACK
AdvancedPageProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            break;
    }

    return FALSE;
}

/* EOF */
