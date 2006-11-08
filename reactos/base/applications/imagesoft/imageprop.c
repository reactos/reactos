#include <precomp.h>

static HWND hImageType, hUnitType, hHeightUnit, hWidthUnit, hResUnit;

UINT ConvertValue(HWND hDlg, UINT EdBoxChanged, UINT LastUnitSel)
{
    LONG Resolution = GetDlgItemInt(hDlg, IDC_RES_EDIT, NULL, FALSE);
    FLOAT Width     = (FLOAT)GetDlgItemInt(hDlg, IDC_WIDTH_EDIT, NULL, FALSE);
    FLOAT Height    = (FLOAT)GetDlgItemInt(hDlg, IDC_HEIGHT_EDIT, NULL, FALSE);
    USHORT CurUnit  = (USHORT)SendMessage(hUnitType, CB_GETCURSEL, 0, 0);

    /* if the user typed in the resolution box */
    if ((EdBoxChanged == IDC_RES_EDIT) && (CurUnit != PIXELS))
    {
        Width = Width / Resolution * 100;
        Height = Height / Resolution * 100;


        /* something wrong with these */
        SetDlgItemInt(hDlg, IDC_WIDTH_EDIT, Width, TRUE);
        SetDlgItemInt(hDlg, IDC_HEIGHT_EDIT, Height, FALSE);

        return LastUnitSel;
    }


    /* if the user changed the unit combobox */
    if (EdBoxChanged == IDC_UNIT)
    {
        switch(LastUnitSel)
        {
            case PIXELS:
                if (CurUnit == CENTIMETERS)
                    ;
                else if (CurUnit == INCHES)
                    ;
            break;

            case CENTIMETERS:
                if (CurUnit == PIXELS)
                    ;
                else if (CurUnit == INCHES)
                {
                    Width /= 2.54;
                    Height /= 2.54;
                    Resolution *= 2.54;

                    SetDlgItemInt(hDlg, IDC_WIDTH_EDIT, Width, FALSE);
                    SetDlgItemInt(hDlg, IDC_HEIGHT_EDIT, Height, FALSE);
                    SetDlgItemInt(hDlg, IDC_RES_EDIT, Resolution, FALSE);
                }
            break;

            case INCHES:
                if (CurUnit == PIXELS)
                    ;
                else if (CurUnit == CENTIMETERS)
                {
                    Width *= 2.54;
                    Height *= 2.54;
                    Resolution /= 2.54;

                    SetDlgItemInt(hDlg, IDC_WIDTH_EDIT, Width, FALSE);
                    SetDlgItemInt(hDlg, IDC_HEIGHT_EDIT, Height, FALSE);
                    SetDlgItemInt(hDlg, IDC_RES_EDIT, Resolution, FALSE);
                }
            break;
        }
    }

    return CurUnit;
}


VOID SetImageSize(HWND hDlg)
{
    DWORD Size;
    USHORT Type = 0;
    TCHAR buf[20];
    TCHAR SizeUnit[25];

    FLOAT Width  = GetDlgItemInt(hDlg, IDC_WIDTH_EDIT, NULL, FALSE);
    FLOAT Height = GetDlgItemInt(hDlg, IDC_HEIGHT_EDIT, NULL, FALSE);
    USHORT sel  = SendMessage(hImageType, CB_GETCURSEL, 0, 0);

    if (sel == 0)
        Type = MONOCHROMEBITS;
    else if (sel == 1)
        Type = GREYSCALEBITS;
    else if (sel == 2)
        Type = PALLETEBITS;
    else if (sel == 3)
        Type = TRUECOLORBITS;

    Size = ((Width * Height * Type) / 8) / 1024;

    if (Size > 1000)
    {
        Size /= 1024;
        LoadString(hInstance, IDS_UNIT_MB, SizeUnit, sizeof(SizeUnit) / sizeof(TCHAR));
    }
    else
        LoadString(hInstance, IDS_UNIT_KB, SizeUnit, sizeof(SizeUnit) / sizeof(TCHAR));

    _sntprintf(buf, sizeof(buf) / sizeof(TCHAR), SizeUnit, Size);
    SendDlgItemMessage(hDlg, IDC_IMAGE_SIZE, WM_SETTEXT, 0, (LPARAM)buf);

}

INT_PTR CALLBACK
ImagePropDialogProc(HWND hDlg,
                    UINT message,
                    WPARAM wParam,
                    LPARAM lParam)
{
    static PIMAGE_PROP ImageProp = NULL;
    static UINT LastUnitSel;
    TCHAR buf[35];

    switch (message)
    {
        case WM_INITDIALOG:

            ImageProp = (PIMAGE_PROP)lParam;

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
            LastUnitSel = PIXELS;

            /* temperary. Default vals should be loaded from registry */
            SendDlgItemMessage(hDlg, IDC_WIDTH_EDIT, WM_SETTEXT, 0, (LPARAM)_T("400"));
            SendDlgItemMessage(hDlg, IDC_HEIGHT_EDIT, WM_SETTEXT, 0, (LPARAM)_T("300"));
            SendDlgItemMessage(hDlg, IDC_RES_EDIT, WM_SETTEXT, 0, (LPARAM)_T("50"));
            SetImageSize(hDlg);

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
                ImageProp->Resolution = GetDlgItemInt(hDlg, IDC_RES_EDIT, NULL, FALSE);
                ImageProp->Width      = GetDlgItemInt(hDlg, IDC_WIDTH_EDIT, NULL, FALSE);
                ImageProp->Height     = GetDlgItemInt(hDlg, IDC_HEIGHT_EDIT, NULL, FALSE);
                ImageProp->Unit       = SendMessage(hUnitType, CB_GETCURSEL, 0, 0);

                EndDialog(hDlg, 1);
            }
            break;

            case IDCANCEL:
                ImageProp = NULL;
                EndDialog(hDlg, 0);
            break;

            case IDC_UNIT:
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    INT unit = SendMessage(hUnitType, CB_GETCURSEL, 0, 0);

                    LastUnitSel = ConvertValue(hDlg, IDC_UNIT, LastUnitSel);

                    switch (unit)
                    {
                        case 0: /* pixels */
                        {
                            LoadString(hInstance, IDS_UNIT_PIXELS, buf, sizeof(buf) / sizeof(TCHAR));
                            SendMessage(hWidthUnit, WM_SETTEXT, 0, (LPARAM)buf);
                            SendMessage(hHeightUnit, WM_SETTEXT, 0, (LPARAM)buf);
                            LoadString(hInstance, IDS_UNIT_DPI, buf, sizeof(buf) / sizeof(TCHAR));
                            SendMessage(hResUnit, WM_SETTEXT, 0, (LPARAM)buf);
                        }
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

            case IDC_IMAGETYPE:
                if (HIWORD(wParam) == CBN_SELCHANGE)
                    SetImageSize(hDlg);
            break;

            case IDC_WIDTH_EDIT:
            case IDC_HEIGHT_EDIT:
                if (HIWORD(wParam) == EN_UPDATE)
                    SetImageSize(hDlg);
            break;

            case IDC_RES_EDIT:
                if (HIWORD(wParam) == EN_UPDATE)
                    ConvertValue(hDlg, IDC_RES_EDIT, LastUnitSel);
            break;
        }
        break;

        default:
            return FALSE;
    }

    return TRUE;
}
