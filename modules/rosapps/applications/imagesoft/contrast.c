#include "precomp.h"

#define BASECOLOUR 100


VOID
AdjustContrast(HBITMAP hOrigBitmap,
               HBITMAP hNewBitmap,
               HWND hwnd,
               HDC hdcMem,
               INT RedVal,
               INT GreenVal,
               INT BlueVal)
{
    BITMAPINFO bi;
    BITMAP bitmap;
    BOOL bRes;
    DWORD Count = 0;
    INT i, j;
    PBYTE pBits;
    RECT rc;

    GetObject(hNewBitmap,
              sizeof(bitmap),
              &bitmap);

    /* Bitmap header */
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = bitmap.bmWidth;
    bi.bmiHeader.biHeight = bitmap.bmHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = bitmap.bmWidth * bitmap.bmHeight * 4;
    bi.bmiHeader.biClrUsed = 0;
    bi.bmiHeader.biClrImportant = 0;

    /* Buffer */
    pBits = (PBYTE)HeapAlloc(ProcessHeap,
                             0,
                             bitmap.bmWidth * bitmap.bmHeight * 4);
    if (!pBits)
        return;

    /* get the bits from the original bitmap */
    bRes = GetDIBits(hdcMem,
                     hOrigBitmap,
                     0,
                     bitmap.bmHeight,
                     pBits,
                     &bi,
                     DIB_RGB_COLORS);

    for (i = 0; i < bitmap.bmHeight; i++)
    {
        for (j = 0; j < bitmap.bmWidth; j++)
        {
            DWORD Val = 0;
            INT b, g, r;

            CopyMemory(&Val,
                       &pBits[Count],
                       4);

            /* Get pixels in reverse order */
            b = GetRValue(Val);
            g = GetGValue(Val);
            r = GetBValue(Val);

            r = ((r - 128) * RedVal) / 100 + 128;
            g = ((g - 128) * GreenVal) / 100 + 128;
            b = ((b - 128) * BlueVal) / 100 + 128;

            /* Red */
            if (r > 255) r = 255;
            else if (r < 0) r = 0;

            /* Green */
            if (g > 255) g = 255;
            else if (g < 0) g = 0;

            /* Blue */
            if (b > 255) b = 255;
            else if (b < 0) b = 0;

            /* Store in reverse order */
            Val = RGB(b, g, r);
            CopyMemory(&pBits[Count],
                       &Val,
                       4);

            /* RGB color take 4 bytes.The high-order byte must be zero */
            Count += 4;
        }
    }

    /* Set the new pixel bits */
    SetDIBits(hdcMem,
              hNewBitmap,
              0,
              bRes,
              pBits,
              &bi,
              DIB_RGB_COLORS);

    HeapFree(ProcessHeap,
             0,
             pBits);

    GetClientRect(hwnd,
                  &rc);

    InvalidateRect(hwnd,
                   &rc,
                   FALSE);
}


static PIMAGEADJUST
Cont_OnInitDialog(PIMAGEADJUST pImgAdj,
             HWND hDlg,
             LPARAM lParam)
{
    pImgAdj = (IMAGEADJUST*) HeapAlloc(ProcessHeap,
                        0,
                        sizeof(IMAGEADJUST));
    if (!pImgAdj)
        return NULL;


    pImgAdj->Info = (PMAIN_WND_INFO)lParam;
    if (!pImgAdj->Info->ImageEditors)
        goto fail;


    pImgAdj->hPicPrev = GetDlgItem(hDlg, IDC_PICPREVIEW);
    GetClientRect(pImgAdj->hPicPrev,
                  &pImgAdj->ImageRect);

    /* Make a static copy of the main image */
    pImgAdj->hBitmap = (HBITMAP) CopyImage(pImgAdj->Info->ImageEditors->hBitmap,
                                 IMAGE_BITMAP,
                                 pImgAdj->ImageRect.right,
                                 pImgAdj->ImageRect.bottom,
                                 LR_CREATEDIBSECTION);
    if (!pImgAdj->hBitmap)
        goto fail;

    /* Make a copy which will be updated */
    pImgAdj->hPreviewBitmap = (HBITMAP) CopyImage(pImgAdj->Info->ImageEditors->hBitmap,
                                        IMAGE_BITMAP,
                                        pImgAdj->ImageRect.right,
                                        pImgAdj->ImageRect.bottom,
                                        LR_CREATEDIBSECTION);
    if (!pImgAdj->hPreviewBitmap)
        goto fail;


    pImgAdj->RedVal = pImgAdj->BlueVal = pImgAdj->GreenVal = 100;

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
                       (LPARAM)BASECOLOUR);
    SetDlgItemText(hDlg,
                   IDC_BRI_EDIT,
                   _T("100"));

    return pImgAdj;

fail:
    HeapFree(ProcessHeap,
             0,
             pImgAdj);
    return NULL;
}


static VOID
Cont_OnDrawItem(PIMAGEADJUST pImgAdj,
           LPARAM lParam)
{
    LPDRAWITEMSTRUCT lpDrawItem;
    HDC hdcMem;

    lpDrawItem = (LPDRAWITEMSTRUCT)lParam;

    hdcMem = CreateCompatibleDC(lpDrawItem->hDC);

    if(lpDrawItem->CtlID == IDC_PICPREVIEW)
    {
        SelectObject(hdcMem,
                     pImgAdj->hPreviewBitmap);

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
}


static VOID
Cont_OnTrackBar(PIMAGEADJUST pImgAdj,
           HWND hDlg)
{
    HDC hdcMem;
    DWORD TrackPos;

    TrackPos = (DWORD)SendDlgItemMessage(hDlg,
                                         IDC_BRI_TRACKBAR,
                                         TBM_GETPOS,
                                         0,
                                         0);

    SetDlgItemInt(hDlg,
                  IDC_BRI_EDIT,
                  TrackPos,
                  FALSE);

    if (IsDlgButtonChecked(hDlg, IDC_BRI_FULL) == BST_CHECKED)
    {
        pImgAdj->RedVal = pImgAdj->GreenVal = pImgAdj->BlueVal = TrackPos - BASECOLOUR + 100;
    }
    else if (IsDlgButtonChecked(hDlg, IDC_BRI_RED) == BST_CHECKED)
    {
        pImgAdj->RedVal = TrackPos - BASECOLOUR + 100;
    }
    else if (IsDlgButtonChecked(hDlg, IDC_BRI_GREEN) == BST_CHECKED)
    {
        pImgAdj->GreenVal = TrackPos - BASECOLOUR + 100;
    }
    else if (IsDlgButtonChecked(hDlg, IDC_BRI_BLUE) == BST_CHECKED)
    {
        pImgAdj->BlueVal = TrackPos - BASECOLOUR + 100;
    }

    hdcMem = GetDC(pImgAdj->hPicPrev);

    AdjustContrast(pImgAdj->hBitmap,
                     pImgAdj->hPreviewBitmap,
                     pImgAdj->hPicPrev,
                     hdcMem,
                     pImgAdj->RedVal,
                     pImgAdj->GreenVal,
                     pImgAdj->BlueVal);

    ReleaseDC(pImgAdj->hPicPrev, hdcMem);
}


static BOOL
Cont_OnCommand(PIMAGEADJUST pImgAdj,
          HWND hDlg,
          UINT uID)
{
    switch (uID)
    {
        case IDOK:
        {
            HDC hdcMem;

            hdcMem = GetDC(pImgAdj->Info->ImageEditors->hSelf);

            AdjustContrast(pImgAdj->Info->ImageEditors->hBitmap,
                             pImgAdj->Info->ImageEditors->hBitmap,
                             pImgAdj->Info->ImageEditors->hSelf,
                             hdcMem,
                             pImgAdj->RedVal,
                             pImgAdj->GreenVal,
                             pImgAdj->BlueVal);

            ReleaseDC(pImgAdj->Info->ImageEditors->hSelf,
                      hdcMem);

            EndDialog(hDlg,
                      uID);

            return TRUE;
        }

        case IDCANCEL:
        {
            EndDialog(hDlg,
                      uID);
            return TRUE;
        }
    }

    return FALSE;
}


INT_PTR CALLBACK
ContrastProc(HWND hDlg,
             UINT message,
             WPARAM wParam,
             LPARAM lParam)
{
    static PIMAGEADJUST pImgAdj = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            pImgAdj = Cont_OnInitDialog(pImgAdj,
                                        hDlg,
                                        lParam);
            if (!pImgAdj)
            {
                EndDialog(hDlg, -1);
                return FALSE;
            }

            return TRUE;
        }

        case WM_DRAWITEM:
        {
            Cont_OnDrawItem(pImgAdj,
                            lParam);
            return TRUE;
        }

        case WM_HSCROLL:
        {
            if (LOWORD(wParam) == TB_THUMBTRACK ||
                LOWORD(wParam) == TB_ENDTRACK)
            {
                Cont_OnTrackBar(pImgAdj,
                                hDlg);
            }

            return TRUE;
        }

        case WM_COMMAND:
        {
            return Cont_OnCommand(pImgAdj,
                                  hDlg,
                                  LOWORD(wParam));
        }

        case WM_DESTROY:
        {
            if (pImgAdj)
            {
                if (pImgAdj->hBitmap)
                    DeleteObject(pImgAdj->hBitmap);
                if (pImgAdj->hPreviewBitmap)
                    DeleteObject(pImgAdj->hPreviewBitmap);

                HeapFree(ProcessHeap,
                         0,
                         pImgAdj);
            }
        }
    }

    return FALSE;
}
