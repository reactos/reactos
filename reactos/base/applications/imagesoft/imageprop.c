#include <precomp.h>

INT_PTR CALLBACK
BrightnessProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    static PMAIN_WND_INFO Info = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            Info = (PMAIN_WND_INFO)lParam;

            SendDlgItemMessage(hDlg,
                               IDC_BRI_FULL,
                               BM_SETCHECK,
                               BST_CHECKED,
                               0);

            SendDlgItemMessage(hDlg,
                               IDC_BRI_TRACKBAR,
                               TBM_SETRANGE,
                               TRUE,
                               (LPARAM)MAKELONG(0, 200));

            SendDlgItemMessage(hDlg,
                               IDC_BRI_TRACKBAR,
                               TBM_SETPOS,
                               TRUE,
                               (LPARAM)100);

            SetDlgItemText(hDlg,
                           IDC_BRI_EDIT,
                           _T("100"));


            return TRUE;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            HWND hPicPrev = GetDlgItem(hDlg, IDC_PICPREVIEW);
            RECT ImageRect = {0};
            HDC hdcMem;

            lpDrawItem = (LPDRAWITEMSTRUCT)lParam;

            GetClientRect(hPicPrev,
                          &ImageRect);

            hdcMem = CreateCompatibleDC(lpDrawItem->hDC);

            if(lpDrawItem->CtlID == IDC_PICPREVIEW)
            {
                SelectObject(hdcMem, 
                             Info->ImageEditors->hBitmap);

                StretchBlt(lpDrawItem->hDC,
                           ImageRect.left,
                           ImageRect.top,
                           ImageRect.right,
                           ImageRect.bottom,
                           hdcMem,
                           0,
                           0,
                           Info->ImageEditors->Width,
                           Info->ImageEditors->Height,
                           SRCCOPY);

                DeleteDC(hdcMem);
            }
            return TRUE;
        }

        case WM_HSCROLL:
        {
            if (LOWORD(wParam) == TB_THUMBTRACK ||
                LOWORD(wParam) == TB_ENDTRACK)
            {
                DWORD Pos = (DWORD)SendDlgItemMessage(hDlg,
                                                      IDC_BRI_TRACKBAR,
                                                      TBM_GETPOS,
                                                      0,
                                                      0);
                SetDlgItemInt(hDlg,
                              IDC_BRI_EDIT,
                              Pos,
                              FALSE);
            }

            return TRUE;
        }

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK ||
                LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg,
                          LOWORD(wParam));
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
