// prototype.cpp : définit le point d'entrée pour l'application.
//

// Fichiers d'en-tête C RunTime
#include <stdlib.h>
#include <tchar.h>

// Fichiers d'en-tête Windows
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "resource.h"
#include "../../../../sdk/include/reactos/ui/branding.h"

HINSTANCE hInst;

INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY
_tWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    hInst = hInstance;
    return DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_TESTBOX), NULL, About);
}


// Timer ID for the animated dialog bar.
#define IDT_BAR 1

typedef struct _DLG_DATA
{
    PVOID pgContext;
    BRAND Brand;
    DWORD BarCounter;
    HBRUSH hbrLol;
} DLG_DATA, *PDLG_DATA;

static PDLG_DATA
DlgData_Create(
    _In_ HWND hwndDlg,
    _In_ PVOID pgContext)
{
    PDLG_DATA pDlgData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pDlgData));
    if (pDlgData)
    {
        pDlgData->pgContext = pgContext;
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)pDlgData);
    }
    return pDlgData;
}

static VOID
DlgData_Destroy(
    _In_ HWND hwndDlg)
{
    PDLG_DATA pDlgData;

    pDlgData = (PDLG_DATA)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);
    if (!pDlgData)
        return;

    SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)NULL);

    Brand_Cleanup(&pDlgData->Brand);

    HeapFree(GetProcessHeap(), 0, pDlgData);
}

INT_PTR CALLBACK
About(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PDLG_DATA pDlgData;

    pDlgData = (PDLG_DATA)GetWindowLongPtrW(hDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pDlgData = DlgData_Create(hDlg, (PVOID)lParam);
            if (pDlgData == NULL)
                return FALSE;

            Brand_LoadBitmaps(&pDlgData->Brand, hInst, IDI_ROSLOGO, IDI_BAR, DT_CENTER);

            /*
             * Enable or disable this in order to move or not move the controls.
             * When they are not moved, they are over the animated bar, and one can
             * observe different painting effects depending on the choice of how
             * the animated bar is painted.
             */
            Brand_MoveControls(hDlg, &pDlgData->Brand);

            /** TESTING PURPOSES ONLY! Custom dialog background **/
            pDlgData->hbrLol = CreateSolidBrush(RGB(37, 127, 90));

            SetTimer(hDlg, IDT_BAR, /*20*/200, NULL);
            return TRUE;
        }

        case WM_TIMER:
        {
            /*
             * Default rotation bar image width is 413 (same as logo).
             * We can divide 413 by 7 without remainder.
             */
            pDlgData->BarCounter = (pDlgData->BarCounter + 7) % pDlgData->Brand.BarSize.cx;

#if 0
            {
            RECT rc = pDlgData->Brand.rcClient;
            if (IsRectEmpty(&rc))
                GetClientRect(hDlg, &rc);
            rc.top += pDlgData->Brand.LogoSize.cy;
            rc.bottom = rc.top + pDlgData->Brand.BarSize.cy;
            InvalidateRect(hDlg, &rc, TRUE);
            //UpdateWindow(hDlg); // This call doesn't do much.
            }
#else // Doing the paint call here seems to reduce flicker when the bar is over some controls...
            Brand_Paint(hDlg, NULL, &pDlgData->Brand, TRUE, pDlgData->BarCounter, pDlgData->hbrLol);
#endif

            return TRUE;
        }

#if 1
        case WM_ERASEBKGND:
        {
            HDC hDC = (HDC)wParam;
            RECT rc;
            Brand_Paint(hDlg, hDC, &pDlgData->Brand, FALSE, pDlgData->BarCounter, pDlgData->hbrLol);

            /**
             ** TEMP Consider: Fill the rest of the dialog background with dialog color.
             **/
            rc = pDlgData->Brand.rcClient; // Can be initialized by Brand_Paint().
            rc.top += (pDlgData->Brand.LogoSize.cy + pDlgData->Brand.BarSize.cy);
            FillRectEx(hDC, &rc, pDlgData->hbrLol /*GetColorDlg(hDlg, hDC)*/);
            return TRUE;
        }
#endif

        /** TESTING PURPOSES ONLY! Custom dialog background **/
        case WM_CTLCOLORSTATIC:
            SetBkMode((HDC)wParam, TRANSPARENT);
        case WM_CTLCOLORDLG:
            return (INT_PTR)pDlgData->hbrLol;

#if 0
        case WM_PAINT:
        {
            if (pDlgData)
            {
                PAINTSTRUCT ps;
                BeginPaint(hDlg, &ps);
                Brand_Paint(hDlg, ps.hdc, &pDlgData->Brand, FALSE, pDlgData->BarCounter, pDlgData->hbrLol);
                EndPaint(hDlg, &ps);
            }
            return TRUE;
        }
#endif

        case WM_DESTROY:
        {
            KillTimer(hDlg, IDT_BAR);
            DeleteObject(pDlgData->hbrLol);
            DlgData_Destroy(hDlg);
            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}
