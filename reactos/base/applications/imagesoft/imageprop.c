#include <precomp.h>

INT_PTR CALLBACK
BrightnessProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    static PIMAGEADJUST pImgAdj = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            pImgAdj = HeapAlloc(ProcessHeap,
                                0,
                                sizeof(IMAGEADJUST));
            if (!pImgAdj)
                return -1;

            /* setup values */
            pImgAdj->Info = (PMAIN_WND_INFO)lParam;
            pImgAdj->hPicPrev = GetDlgItem(hDlg, IDC_PICPREVIEW);
            GetClientRect(pImgAdj->hPicPrev,
                          &pImgAdj->ImageRect);

            pImgAdj->hBitmap = CopyImage(pImgAdj->Info->ImageEditors->hBitmap,
                                         IMAGE_BITMAP,
                                         pImgAdj->ImageRect.right,
                                         pImgAdj->ImageRect.bottom,
                                         LR_CREATEDIBSECTION);

            pImgAdj->OldTrackPos = 100;
            pImgAdj->RedVal = pImgAdj->BlueVal = pImgAdj->GreenVal = 0;

            /* setup dialog */
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
            HDC hdcMem;

            lpDrawItem = (LPDRAWITEMSTRUCT)lParam;

            hdcMem = CreateCompatibleDC(lpDrawItem->hDC);

            if(lpDrawItem->CtlID == IDC_PICPREVIEW)
            {
                SelectObject(hdcMem,
                             pImgAdj->hBitmap);

                BitBlt(lpDrawItem->hDC,
                       pImgAdj->ImageRect.left,
                       pImgAdj->ImageRect.top,
                       pImgAdj->ImageRect.right,
                       pImgAdj->ImageRect.bottom,
                       hdcMem,
                       0,
                       0,
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
                HDC hdcMem;
                DWORD TrackPos = (DWORD)SendDlgItemMessage(hDlg,
                                                           IDC_BRI_TRACKBAR,
                                                           TBM_GETPOS,
                                                           0,
                                                           0);

                /* quick hack, change all the colours regardless */
                pImgAdj->RedVal = pImgAdj->BlueVal = pImgAdj->GreenVal = TrackPos - pImgAdj->OldTrackPos;
                pImgAdj->OldTrackPos = TrackPos;

                SetDlgItemInt(hDlg,
                              IDC_BRI_EDIT,
                              TrackPos,
                              FALSE);

                hdcMem = GetDC(pImgAdj->hPicPrev);

                AdjustBrightness(pImgAdj,
                                 hdcMem);

                ReleaseDC(pImgAdj->hPicPrev, hdcMem);

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

        case WM_DESTROY:
        {
            DeleteObject(pImgAdj->hBitmap);

            HeapFree(ProcessHeap,
                     0,
                     pImgAdj);
        }

    }

    return FALSE;
}
