/*
 * ReactOS Column List Box
 * Copyright (C) 2005 Thomas Weidenmueller
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * PROJECT:         ReactOS Column List Box
 * FILE:            base/applications/regedit/clb/clb.c
 * PURPOSE:         Column List Box
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      10/29/2005  Created
 */
#include <precomp.h>

static HINSTANCE hDllInstance;

static const WCHAR ClbClassName[] = L"ColumnListBox";
static const WCHAR ClbColumns[] = L"Column1;Column2;Column3";

typedef struct _CLB_PRIVATEDATA
{
    HWND hwnd;
} CLB_PRIVATEDATA, *PCLB_PRIVATEDATA;

static const CLBS_INFO ClbsSupportedStyles[] =
{
    {
        CLBS_NOTIFY,
        0x0,
        L"CLBS_NOTIFY"
    },
    {
        CLBS_SORT,
        0x0,
        L"CLBS_SORT"
    },
    {
        CLBS_DISABLENOSCROLL,
        0x0,
        L"CLBS_DISABLENOSCROLL"
    },
    {
        CLBS_VSCROLL,
        0x0,
        L"CLBS_VSCROLL"
    },
    {
        CLBS_BORDER,
        0x0,
        L"CLBS_BORDER"
    },
    {
        CLBS_POPOUT_HEADINGS,
        0x0,
        L"CLBS_POPOUT_HEADINGS"
    },
    {
        CLBS_SPRINGLY_COLUMNS,
        0x0,
        L"CLBS_SPRINGLY_COLUMNS"
    },
    {
        LBS_OWNERDRAWFIXED,
        0x0,
        L"LBS_OWNERDRAWFIXED"
    }
};

/*
 * @unimplemented
 */
DWORD
WINAPI
ClbAddData(IN DWORD Unknown1,
           IN DWORD Unknown2,
           IN DWORD Unknown3)
{
    DPRINT1("ClbAddData(0x%x, 0x%x, 0x%x)\n", Unknown1, Unknown2, Unknown3);
    return 0;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
ClbSetColumnWidths(IN DWORD Unknown1,
                   IN DWORD Unknown2,
                   IN DWORD Unknown3)
{
    DPRINT1("ClbSetColumnWidths(0x%x, 0x%x, 0x%x)\n", Unknown1, Unknown2, Unknown3);
    return 0;
}


/*
 * @unimplemented
 */
LRESULT
CALLBACK
ClbWndProc(IN HWND hwnd,
           IN UINT uMsg,
           IN WPARAM wParam,
           IN LPARAM lParam)
{
    PCLB_PRIVATEDATA PrivData;
    LRESULT Ret = 0;

    DPRINT1("ClbWndProc(0x%p, 0x%x, 0x%p, 0x%p)\n", hwnd, uMsg, wParam, lParam);

    PrivData = (PCLB_PRIVATEDATA)GetWindowLongPtr(hwnd,
               0);
    if (PrivData == NULL && uMsg != WM_CREATE)
    {
        goto HandleDefMsg;
    }

    switch (uMsg)
    {
    case WM_CREATE:
        PrivData = HeapAlloc(GetProcessHeap(),
                             0,
                             sizeof(CLB_PRIVATEDATA));
        if (PrivData == NULL)
        {
            Ret = (LRESULT)-1;
            break;
        }
        PrivData->hwnd = hwnd;
        break;

    case WM_DESTROY:
        HeapFree(GetProcessHeap(),
                 0,
                 PrivData);
        break;

    default:
HandleDefMsg:
        Ret = DefWindowProc(hwnd,
                            uMsg,
                            wParam,
                            lParam);
        break;
    }

    return Ret;
}


static INT_PTR CALLBACK
ClbpStyleDlgProc(IN HWND hwndDlg,
                 IN UINT uMsg,
                 IN WPARAM wParam,
                 IN LPARAM lParam)
{
    INT_PTR Ret = FALSE;

    DPRINT1("ClbpStyleDlgProc(0x%p, 0x%x, 0x%p, 0x%p)\n", hwndDlg, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwndDlg,
                      (INT_PTR)LOWORD(wParam));
            break;
        }
        break;

    case WM_CLOSE:
        EndDialog(hwndDlg,
                  IDCANCEL);
        break;

    case WM_INITDIALOG:
        Ret = TRUE;
        break;
    }

    return Ret;
}


/*
 * @implemented
 */
INT_PTR
WINAPI
ClbStyleW(IN HWND hWndParent,
          IN LPARAM dwInitParam)
{
    return DialogBoxParam(hDllInstance,
                          MAKEINTRESOURCE(IDD_COLUMNLISTBOXSTYLES),
                          hWndParent,
                          ClbpStyleDlgProc,
                          dwInitParam);
}


/*
 * @implemented
 */
BOOL
WINAPI
CustomControlInfoW(OUT LPCUSTOM_CONTROL_INFO CustomControlInfo  OPTIONAL)
{
    if (CustomControlInfo != NULL)
    {
        wcscpy(CustomControlInfo->ClassName,
               ClbClassName);

        CustomControlInfo->Zero1 = 0;

        wcscpy(CustomControlInfo->ClassName2,
               ClbClassName);

        CustomControlInfo->Unknown1 = 0x60; /* FIXME - ??? */
        CustomControlInfo->Unknown2 = 0x50; /* FIXME - ??? */
        CustomControlInfo->Unknown3 = 0x50A10013; /* FIXME - ??? */

        CustomControlInfo->Zero2 = 0;
        CustomControlInfo->Zero3 = 0;

        CustomControlInfo->StylesCount = sizeof(ClbsSupportedStyles) / sizeof(ClbsSupportedStyles[0]);
        CustomControlInfo->SupportedStyles = ClbsSupportedStyles;

        wcscpy(CustomControlInfo->Columns,
               ClbColumns);

        CustomControlInfo->ClbStyleW = ClbStyleW;

        CustomControlInfo->Zero4 = 0;
        CustomControlInfo->Zero5 = 0;
        CustomControlInfo->Zero6 = 0;
    }

    return TRUE;
}

BOOL
WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    BOOL Ret = TRUE;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        WNDCLASS ClbWndClass;

        hDllInstance = hinstDLL;

        InitCommonControls();

        /* register the control's window class */
        ClbWndClass.style = CS_GLOBALCLASS | CS_OWNDC;
        ClbWndClass.lpfnWndProc = ClbWndProc;
        ClbWndClass.cbClsExtra = 0;
        ClbWndClass.cbWndExtra = sizeof(PCLB_PRIVATEDATA);
        ClbWndClass.hInstance = hinstDLL,
                    ClbWndClass.hIcon = NULL;
        ClbWndClass.hCursor = LoadCursor(NULL,
                                         (LPWSTR)IDC_ARROW);
        ClbWndClass.hbrBackground = NULL;
        ClbWndClass.lpszMenuName = NULL;
        ClbWndClass.lpszClassName = ClbClassName;

        if (!RegisterClass(&ClbWndClass))
        {
            Ret = FALSE;
            break;
        }
        break;
    }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        UnregisterClass(ClbClassName,
                        hinstDLL);
        break;
    }
    return Ret;
}

/* EOF */
