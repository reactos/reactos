#include <precomp.h>

static PIMAGE_PROP ImageProp;
static HWND hImageType, hUnitType, hHeightUnit, hWidthUnit, hResUnit;

INT_PTR CALLBACK
ImagePropDialogProc(HWND hDlg,
                    UINT message,
                    WPARAM wParam,
                    LPARAM lParam)
{
    TCHAR buf[35];

    switch (message)
    {
        case WM_INITDIALOG:

            ImageProp = HeapAlloc(ProcessHeap,
                                  0,
                                  sizeof(IMAGE_PROP));
            if (ImageProp == NULL)
                EndDialog(hDlg, 0);

            /* get handles to the windows */
            hImageType  = GetDlgItem(hDlg, IDC_IMAGETYPE);
            hUnitType   = GetDlgItem(hDlg, IDC_UNIT);
            hWidthUnit  = GetDlgItem(hDlg, IDC_WIDTH_STAT);
            hHeightUnit = GetDlgItem(hDlg, IDC_HEIGHT_STAT);
            hResUnit    = GetDlgItem(hDlg, IDC_RES_STAT);

            /* fill image type combo box */
            LoadString(hInstance, IDS_IMAGE_MONOCHROME, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hImageType, CB_ADDSTRING, 0, (LPARAM)buf);
            LoadString(hInstance, IDS_IMAGE_GREYSCALE, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hImageType, CB_ADDSTRING, 0, (LPARAM)buf);
            LoadString(hInstance, IDS_IMAGE_PALETTE, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hImageType, CB_ADDSTRING, 0, (LPARAM)buf);
            LoadString(hInstance, IDS_IMAGE_TRUECOLOR, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hImageType, CB_ADDSTRING, 0, (LPARAM)buf);
            /* default 24bit */
            SendMessage(hImageType, CB_SETCURSEL, 3, 0);

            /* fill unit combo box */
            LoadString(hInstance, IDS_UNIT_PIXELS, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hUnitType, CB_ADDSTRING, 0, (LPARAM)buf);
            LoadString(hInstance, IDS_UNIT_CM, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hUnitType, CB_ADDSTRING, 0, (LPARAM)buf);
            LoadString(hInstance, IDS_UNIT_INCHES, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hUnitType, CB_ADDSTRING, 0, (LPARAM)buf);
            /* default pixels */
            SendMessage(hUnitType, CB_SETCURSEL, 0, 0);

            /* default pixels */
            LoadString(hInstance, IDS_UNIT_PIXELS, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hWidthUnit, WM_SETTEXT, 0, (LPARAM)buf);
            SendMessage(hHeightUnit, WM_SETTEXT, 0, (LPARAM)buf);
            LoadString(hInstance, IDS_UNIT_DPI, buf, sizeof(buf) / sizeof(TCHAR));
            SendMessage(hResUnit, WM_SETTEXT, 0, (LPARAM)buf);

            /* temperary. Default vals should be loaded from registry */
            SendDlgItemMessage(hDlg, IDC_WIDTH_EDIT, WM_SETTEXT, 0, (LPARAM)_T("400"));
            SendDlgItemMessage(hDlg, IDC_HEIGHT_EDIT, WM_SETTEXT, 0, (LPARAM)_T("300"));
            SendDlgItemMessage(hDlg, IDC_RES_EDIT, WM_SETTEXT, 0, (LPARAM)_T("50"));

        break;

    case WM_COMMAND:

        switch(LOWORD(wParam))
        {
            case IDOK:
            {
                /* FIXME: default vals should be taken from registry */

                INT Ret = GetTextFromEdit(ImageProp->lpImageName, hDlg, IDC_IMAGE_NAME_EDIT);
                if (Ret == 0)
                    ImageProp->lpImageName = NULL;

                ImageProp->Type       = SendMessage(hImageType, CB_GETCURSEL, 0, 0);
                ImageProp->Resolution = GetNumFromEdit(hDlg, IDC_RES_EDIT);
                ImageProp->Width      = GetNumFromEdit(hDlg, IDC_WIDTH_EDIT);
                ImageProp->Height     = GetNumFromEdit(hDlg, IDC_HEIGHT_EDIT);
                ImageProp->Unit       = SendMessage(hUnitType, CB_GETCURSEL, 0, 0);

                EndDialog(hDlg, (int)ImageProp);
            }
            break;

            case IDCANCEL:
                EndDialog(hDlg, 0);
            break;

            case IDC_UNIT:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        INT unit = SendMessage(hUnitType, CB_GETCURSEL, 0, 0);

                        switch (unit)
                        {
                            case 0:
                                /* pixels */
                                LoadString(hInstance, IDS_UNIT_PIXELS, buf, sizeof(buf) / sizeof(TCHAR));
                                SendMessage(hWidthUnit, WM_SETTEXT, 0, (LPARAM)buf);
                                SendMessage(hHeightUnit, WM_SETTEXT, 0, (LPARAM)buf);
                                LoadString(hInstance, IDS_UNIT_DPI, buf, sizeof(buf) / sizeof(TCHAR));
                                SendMessage(hResUnit, WM_SETTEXT, 0, (LPARAM)buf);
                            break;

                            case 1:
                                /* cm */
                                LoadString(hInstance, IDS_UNIT_CM, buf, sizeof(buf) / sizeof(TCHAR));
                                SendMessage(hWidthUnit, WM_SETTEXT, 0, (LPARAM)buf);
                                SendMessage(hHeightUnit, WM_SETTEXT, 0, (LPARAM)buf);
                                LoadString(hInstance, IDS_UNIT_DOTSCM, buf, sizeof(buf) / sizeof(TCHAR));
                                SendMessage(hResUnit, WM_SETTEXT, 0, (LPARAM)buf);
                            break;

                            case 2:
                                /* inch */
                                LoadString(hInstance, IDS_UNIT_INCHES, buf, sizeof(buf) / sizeof(TCHAR));
                                SendMessage(hWidthUnit, WM_SETTEXT, 0, (LPARAM)buf);
                                SendMessage(hHeightUnit, WM_SETTEXT, 0, (LPARAM)buf);
                                LoadString(hInstance, IDS_UNIT_DPI, buf, sizeof(buf) / sizeof(TCHAR));
                                SendMessage(hResUnit, WM_SETTEXT, 0, (LPARAM)buf);
                            break;
                        }
                    }
                break;
        }
        break;

        default:
            return FALSE;
    }

    return TRUE;
}
