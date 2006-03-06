#include "precomp.h"


static LONG
GetTextFromEdit(HWND hDlg, UINT Res)
{
    LONG num = 0;
    INT len = GetWindowTextLength(GetDlgItem(hDlg, Res));
    TCHAR buf[len+1];

    if(len > 0)
    {
        GetDlgItemText(hDlg, Res, buf, len + 1);
        num = _ttol(buf);
    }

    return num;
}


#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif
INT_PTR CALLBACK
ImagePropDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hImagetype;
    PIMAGE_PROP ImageProp;
    TCHAR buf[25];

    ImageProp = HeapAlloc(ProcessHeap,
                          0,
                          sizeof(IMAGE_PROP));
    if (ImageProp == NULL)
        EndDialog(hDlg, 0);

    switch (message)
    {
        case WM_INITDIALOG:

            hImagetype = GetDlgItem(hDlg, IDC_IMAGETYPE);

            LoadString(hInstance, IDS_IMAGE_MONOCHROME, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hImagetype, CB_ADDSTRING, 0, (LPARAM)buf);
            LoadString(hInstance, IDS_IMAGE_GREYSCALE, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hImagetype, CB_ADDSTRING, 0, (LPARAM)buf);
            LoadString(hInstance, IDS_IMAGE_PALETTE, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hImagetype, CB_ADDSTRING, 0, (LPARAM)buf);
            LoadString(hInstance, IDS_IMAGE_TRUECOLOR, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hImagetype, CB_ADDSTRING, 0, (LPARAM)buf);

            SendMessage(hImagetype, CB_SETCURSEL, 3, 0);

        break;

    case WM_COMMAND:

        switch(LOWORD(wParam))
        {
            case IDOK:
                /* FIXME: default vals should be taken from registry */
                ImageProp->Type = SendMessage(GetDlgItem(hDlg, IDC_IMAGETYPE), CB_GETCURSEL, 0, 0);
                ImageProp->Resolution = GetTextFromEdit(hDlg, IDC_RES_EDIT);
                ImageProp->Width = GetTextFromEdit(hDlg, IDC_WIDTH_EDIT);
                ImageProp->Height = GetTextFromEdit(hDlg, IDC_HEIGHT_EDIT);

                EndDialog(hDlg, (int)ImageProp);
            break;

            case IDCANCEL:
                EndDialog(hDlg, 0);
            break;
        }
        break;

        default:
            return FALSE;
    }

    return TRUE;
}
