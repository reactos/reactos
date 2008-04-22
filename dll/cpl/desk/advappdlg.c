/* $Id: advappdlg.c 24836 2007-02-12 03:12:56Z tkreuzer $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/advappdlg.c
 * PURPOSE:         Advanced appearance dialog
 *
 * PROGRAMMER:     Timo Kreuzer (timo[dot]kreuzer[at]web[dot]de)
 *
 */

#include "desk.h"
#include "appearance.h"
#include "preview.h"



/* Draw the current color on the color picker buttons */
static VOID
UpdateButtonColor(HWND hwndDlg, GLOBALS* g, INT ID, INT nButton, INT nColor)
{
	HDC hdcColorButton, hdcCompat;
	RECT rect;
	HBRUSH hbrush;
	HWND hwndColorButton;
	COLORREF crColor = g->ThemeAdv.crColor[nColor];
	HGDIOBJ hgdiTmp;

	if (nColor != -1)
	{
		/* Create a DC to draw on */
		hwndColorButton = GetDlgItem(hwndDlg, ID);
		hdcColorButton = GetDC(hwndColorButton);
		hdcCompat = CreateCompatibleDC(hdcColorButton);
		ReleaseDC(hwndColorButton, hdcColorButton);

		/* Select the button image to it */
		hgdiTmp = SelectObject(hdcCompat, g->hbmpColor[nButton]);

		/* Create a brush and draw the rectangle */
		rect.left = 2;
		rect.top = 2;
		rect.right = 22;
		rect.bottom = 13;
		hbrush = CreateSolidBrush(crColor);
		FillRect(hdcCompat, &rect, hbrush);
		DeleteObject(hbrush);

		/* hdcCompat is not needed anymore */
		SelectObject(hdcCompat,hgdiTmp);
		DeleteDC(hdcCompat);

		SendDlgItemMessage(hwndDlg, ID, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g->hbmpColor[nButton]);
		EnableWindow(GetDlgItem(hwndDlg, ID), TRUE);
	}
	else
	{
		SendDlgItemMessage(hwndDlg, ID, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)NULL);
		EnableWindow(GetDlgItem(hwndDlg, ID), FALSE);
	}
}


/* Create the basic bitmaps for the color picker buttons */
static VOID
InitColorButtons(HWND hwndDlg, GLOBALS* g)
{
	INT i;
	HDC hdcColorButton, hdcCompat;
	RECT rect;
	HBRUSH hbrush;
	HPEN hPen;
	HWND hwndColorButton;
	HGDIOBJ hgdiTemp;

	const POINT Points[3] = {{29,6},{33,6},{31,8}};

	hwndColorButton = GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_COLOR1_B);
	hdcColorButton = GetDC(hwndColorButton);
	for (i = 0; i <= 2; i++)
	{
		/* Create a DC to draw on */
		hdcCompat = CreateCompatibleDC(hdcColorButton);

		/* Create the button image */
		g->hbmpColor[i] = CreateCompatibleBitmap(hdcColorButton, 36, 15);

		/* Select the button image to the DC */
		hgdiTemp = SelectObject(hdcCompat, g->hbmpColor[i]);

		/* Draw the buttons background color */
		rect.left = 0;
		rect.top = 0;
		rect.right = 36;
		rect.bottom = 15;
		hbrush = CreateSolidBrush(g->crCOLOR_BTNFACE);
		FillRect(hdcCompat, &rect, hbrush);
		DeleteObject(hbrush);

		/* Draw the rectangle */
		rect.left = 1;
		rect.top = 1;
		rect.right = 23;
		rect.bottom = 14;
		hbrush = CreateSolidBrush(g->crCOLOR_BTNTEXT);
		FillRect(hdcCompat, &rect, hbrush);
		DeleteObject(hbrush);

		/* Draw left side of line */
		hPen = CreatePen(PS_SOLID, 1, g->crCOLOR_BTNSHADOW);
		SelectObject(hdcCompat, hPen);
		MoveToEx(hdcCompat, 26, 1, NULL);
		LineTo(hdcCompat, 26, 14);
		SelectObject(hdcCompat, GetStockObject(BLACK_PEN));
		DeleteObject(hPen);

		/* Draw right side of line */
		hPen = CreatePen(PS_SOLID, 1, g->crCOLOR_BTNHIGHLIGHT);
		SelectObject(hdcCompat,hPen);
		MoveToEx(hdcCompat, 27, 1, NULL);
		LineTo(hdcCompat, 27, 14);
		SelectObject(hdcCompat, GetStockObject(BLACK_PEN));
		DeleteObject(hPen);

		/* Draw triangle */
		hPen = CreatePen(PS_SOLID, 1, g->crCOLOR_BTNTEXT);
		hbrush = CreateSolidBrush(g->crCOLOR_BTNTEXT);
		SelectObject(hdcCompat, hPen);
		SelectObject(hdcCompat, hbrush);
		SetPolyFillMode(hdcCompat, WINDING);

		/* FIXME: HACK, see Points definition */
		Polygon(hdcCompat, Points, 3);

		/* Cleanup */
		SelectObject(hdcCompat,hgdiTemp);
		DeleteDC(hdcCompat);
		DeleteObject(hPen);
		DeleteObject(hbrush);
	}

	ReleaseDC(hwndColorButton, hdcColorButton);

	/* Set the images of the buttons */
	SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_COLOR1_B, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g->hbmpColor[0]);
	SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_COLOR2_B, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g->hbmpColor[1]);
	SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTCOLOR_B, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g->hbmpColor[2]);
}


/* This is the callback function to add the installed fonts to the font combo */
static int CALLBACK
EnumFontFamExProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD dwFontType, LPARAM lParam)
{
	/* Don't enumerate more than 100 fonts */
	if (SendMessage((HWND)lParam, CB_GETCOUNT, 0, 0) >= 100)
		return 0;

	/* Only add the string once */
	if (SendMessage((HWND)lParam, CB_FINDSTRINGEXACT, -1, (WPARAM)&(lpelfe->elfLogFont.lfFaceName)) != CB_ERR)
		return 2;

	SendMessage((HWND)lParam, CB_ADDSTRING, 0, (WPARAM)&(lpelfe->elfLogFont.lfFaceName));

	return 1;
}


/* Update all the controls with the current values for the selected screen element */
static VOID
UpdateControls(HWND hwndDlg, GLOBALS *g)
{
	INT iElement;
	HDC hdcDlg;

	iElement = g->CurrentElement;

	/* First enable / disable the controls */
	EnableWindow(GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_SIZE_E), (g_Assignment[iElement].Size != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_SIZE_UD), (g_Assignment[iElement].Size != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_SIZE_T), (g_Assignment[iElement].Size != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_COLOR1_T), (g_Assignment[iElement].Color1 != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_COLOR2_T), (g_Assignment[iElement].Color2 != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_FONT_T), (g_Assignment[iElement].Font != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_FONT_C), (g_Assignment[iElement].Font != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_FONTSIZE_T), (g_Assignment[iElement].Font != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_FONTSIZE_E), (g_Assignment[iElement].Font != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_FONTCOLOR_T), (g_Assignment[iElement].FontColor != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_FONTBOLD), (g_Assignment[iElement].Font != -1));
	EnableWindow(GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_FONTITALIC), (g_Assignment[iElement].Font != -1));

	/* Update the colors of the color buttons */
	UpdateButtonColor(hwndDlg, g, IDC_ADVAPPEARANCE_COLOR1_B, 0, g_Assignment[iElement].Color1);
	UpdateButtonColor(hwndDlg, g, IDC_ADVAPPEARANCE_COLOR2_B, 1, g_Assignment[iElement].Color2);
	UpdateButtonColor(hwndDlg, g, IDC_ADVAPPEARANCE_FONTCOLOR_B, 2, g_Assignment[iElement].FontColor);

	if (g_Assignment[iElement].Size != -1)
		SetDlgItemInt(hwndDlg, IDC_ADVAPPEARANCE_SIZE_E, g->ThemeAdv.Size[g_Assignment[iElement].Size], FALSE);
	else
		SetDlgItemText(hwndDlg, IDC_ADVAPPEARANCE_SIZE_E, TEXT(""));

	hdcDlg = GetDC(hwndDlg);
	if (g_Assignment[iElement].Font != -1)
	{
		LOGFONT lfFont = g->ThemeAdv.lfFont[g_Assignment[iElement].Font];

		SetDlgItemText(hwndDlg, IDC_ADVAPPEARANCE_FONT_C, lfFont.lfFaceName);
		SetDlgItemInt(hwndDlg, IDC_ADVAPPEARANCE_FONTSIZE_E, -MulDiv(g->ThemeAdv.lfFont[g_Assignment[iElement].Font].lfHeight, 72, GetDeviceCaps(hdcDlg, LOGPIXELSY)),FALSE);
		SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONT_C, CB_FINDSTRINGEXACT, -1, (WPARAM)lfFont.lfFaceName);
		SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTBOLD, BM_SETCHECK, g->ThemeAdv.lfFont[g_Assignment[iElement].Font].lfWeight == FW_BOLD?1:0, 0);
		SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTITALIC, BM_SETCHECK, g->ThemeAdv.lfFont[g_Assignment[iElement].Font].lfItalic, 0);
	}
	else
	{
		SetDlgItemText(hwndDlg, IDC_ADVAPPEARANCE_FONT_C, NULL);
		SetDlgItemText(hwndDlg, IDC_ADVAPPEARANCE_FONTSIZE_E, NULL);
		SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTBOLD, BM_SETCHECK, 0, 0);
		SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTITALIC, BM_SETCHECK, 0, 0);
	}

	ReleaseDC(hwndDlg, hdcDlg);
}


static VOID
SaveCurrentValues(HWND hwndDlg, GLOBALS *g)
{
	BOOL bTranslated;
	HDC hdcDlg = GetDC(hwndDlg);

	if (g_Assignment[g->CurrentElement].Size != -1)
	{
		g->ThemeAdv.Size[g_Assignment[g->CurrentElement].Size] = GetDlgItemInt(hwndDlg, IDC_ADVAPPEARANCE_SIZE_E, &bTranslated, FALSE);
	}

	if (g_Assignment[g->CurrentElement].Font != -1)
	{
		g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfHeight = -MulDiv(GetDlgItemInt(hwndDlg, IDC_ADVAPPEARANCE_FONTSIZE_E, &bTranslated, FALSE), GetDeviceCaps(hdcDlg, LOGPIXELSY), 72);
		g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfWeight = (SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTBOLD, BM_GETCHECK, 0, 0) == 1)?FW_BOLD:FW_NORMAL;
		g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfItalic = SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTITALIC, BM_GETCHECK, 0, 0);
		GetDlgItemText(hwndDlg, IDC_ADVAPPEARANCE_FONT_C, g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfFaceName, LF_FACESIZE * sizeof(TCHAR));
	}

	ReleaseDC(hwndDlg, hdcDlg);
}


/* Select a color using a color picker */
static BOOL
GetColor(HWND hwndDlg, GLOBALS* g, INT nButton)
{
	CHOOSECOLOR cc;
	COLORREF crCustom[16] = { 0 };
	COLORREF crColor;
	INT ID = 0;
	INT ColorIndex = 0;

	/* Get the color index from the element index and button number */
	switch (nButton)
	{
		case 0:
			ColorIndex = g_Assignment[g->CurrentElement].Color1;
			ID = IDC_ADVAPPEARANCE_COLOR1_B;
			break;

		case 1:
			ColorIndex = g_Assignment[g->CurrentElement].Color2;
			ID = IDC_ADVAPPEARANCE_COLOR2_B;
			break;

		case 2:
			ColorIndex = g_Assignment[g->CurrentElement].FontColor;
			ID = IDC_ADVAPPEARANCE_FONTCOLOR_B;
			break;
	}

	crColor = g->ThemeAdv.crColor[ColorIndex];

	/* Prepare cc structure */
	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = hwndDlg;
	cc.hInstance = NULL;
	cc.rgbResult = crColor;
	cc.lpCustColors = crCustom;
	cc.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;
	cc.lCustData = 0;
	cc.lpfnHook = NULL;
	cc.lpTemplateName = NULL;

	/* Create the colorpicker */
	if (ChooseColor(&cc))
	{
		g->ThemeAdv.crColor[ColorIndex] = cc.rgbResult;
		if (crColor != cc.rgbResult)
		{
			UpdateButtonColor(hwndDlg, g, ID, nButton, ColorIndex);
			SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCOLOR, ColorIndex, cc.rgbResult);
			return TRUE;
		}
	}

	return FALSE;
}


/* Initialize the advanced appearance dialog */
static VOID
AdvAppearanceDlg_Init(HWND hwndDlg, GLOBALS *g)
{
	INT i, iElement, iListIndex, iDeskIndex = 0;
	TCHAR tstrText[80];
	LOGFONT lfFont;
	LOGFONT lfButtonFont;
	HFONT hMyFont;
	HDC hScreenDC;
	TCHAR Size[4];

	/* Copy the current theme values */
	g->ThemeAdv = g->Theme;

	/* Add the elements to the combo */
	for (iElement = 0; iElement < NUM_ELEMENTS; iElement++)
	{
		LoadString(hApplet, IDS_ELEMENT_1 + iElement, (LPTSTR)&tstrText, 79);
		iListIndex = SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_ELEMENT, CB_ADDSTRING, 0, (LPARAM)&tstrText);
		SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_ELEMENT, CB_SETITEMDATA, (WPARAM)iListIndex, (LPARAM)iElement);
	}

	/* Get the list index of the desktop element */
	for (iListIndex = 0; iListIndex < NUM_ELEMENTS; iListIndex++)
	{
		iElement = SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_ELEMENT, CB_GETITEMDATA, (WPARAM)iListIndex, 0);
		if (iElement == 0)
		{
			iDeskIndex = iListIndex;
			break;
		}
	}

	SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_ELEMENT, CB_SETCURSEL, iDeskIndex, 0);

	/* Set colors for the color buttons */
	g->crCOLOR_BTNFACE = g->Theme.crColor[COLOR_BTNFACE];
	g->crCOLOR_BTNTEXT = g->Theme.crColor[COLOR_BTNTEXT];
	g->crCOLOR_BTNSHADOW = g->Theme.crColor[COLOR_BTNSHADOW];
	g->crCOLOR_BTNHIGHLIGHT = g->Theme.crColor[COLOR_BTNHIGHLIGHT];

	/* Create font for bold button */
	lfButtonFont = g->Theme.lfFont[FONT_DIALOG];
	lfButtonFont.lfWeight = FW_BOLD;
	lfButtonFont.lfItalic = FALSE;
	hMyFont = CreateFontIndirect(&lfButtonFont);
	if (hMyFont)
	{
		if (g->hBoldFont)
			DeleteObject(g->hBoldFont);

		g->hBoldFont = hMyFont;
	}

	/* Create font for italic button */
	lfButtonFont.lfWeight = FW_REGULAR;
	lfButtonFont.lfItalic = TRUE;
	hMyFont = CreateFontIndirect(&lfButtonFont);
	if (hMyFont)
	{
		if (g->hItalicFont)
			DeleteObject(g->hItalicFont);

		g->hItalicFont = hMyFont;
	}

	/* Set the fonts for the font style buttons */
	SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTBOLD, WM_SETFONT, (WPARAM)g->hBoldFont, (LPARAM)TRUE);
	SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTITALIC, WM_SETFONT, (WPARAM)g->hItalicFont, (LPARAM)TRUE);

	/* Draw Bitmaps for the colorbuttons */
	InitColorButtons(hwndDlg, g);

	/* Make the UpDown control count correctly */
	SendMessage (GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_SIZE_UD), UDM_SETRANGE, 0L, MAKELONG (200, 1));

	/* Fill font selection combo */
	lfFont.lfCharSet = DEFAULT_CHARSET;
	lfFont.lfFaceName[0] = (TCHAR)0;
	lfFont.lfPitchAndFamily = 0;
	hScreenDC = GetDC(0);
	EnumFontFamiliesEx(hScreenDC, &lfFont, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)GetDlgItem(hwndDlg, IDC_ADVAPPEARANCE_FONT_C), 0);
	ReleaseDC(0, hScreenDC);

	/* Fill font size combo */
	for (i = 6; i <= 24; i++)
	{
		wsprintf(Size, TEXT("%d"), i);
		SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTSIZE_E, CB_ADDSTRING, 0, (LPARAM)&Size);
	}

	/* Update the controls */
	iListIndex = SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_ELEMENT, CB_GETCURSEL, 0, 0);
	g->CurrentElement = SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_ELEMENT, CB_GETITEMDATA, (WPARAM)iListIndex, 0);
	UpdateControls(hwndDlg, g);
}


static VOID
AdvAppearanceDlg_CleanUp(HWND hwndDlg, GLOBALS* g)
{
	DeleteObject(g->hBoldFont);
	DeleteObject(g->hItalicFont);
}


static VOID
SelectComboByElement(HWND hwnd, INT id, LPARAM lParam)
{
    INT nCount;
    INT i;

    nCount = SendDlgItemMessage(hwnd, id, CB_GETCOUNT, 0, 0);
    if (nCount == CB_ERR)
        return;

    for (i = 0; i < nCount; i++)
    {
        if (SendDlgItemMessage(hwnd, id, CB_GETITEMDATA, (WPARAM)i, 0) == lParam)
        {
            SendDlgItemMessage(hwnd, id, CB_SETCURSEL, (WPARAM)i, 0);
            break;
        }
    }
}


static VOID
GetSelectedComboText(HWND hwnd, INT id, LPWSTR lpStr)
{
    INT nCount;

    nCount = SendDlgItemMessage(hwnd, id, CB_GETCURSEL, 0, 0);
    if (nCount == CB_ERR)
    {
        *lpStr = 0;
        return;
    }

    nCount = SendDlgItemMessage(hwnd, id, CB_GETLBTEXT, (WPARAM)nCount, (LPARAM)lpStr);
    if (nCount == CB_ERR)
    {
        *lpStr = 0;
    }
}


static INT
GetSelectedComboInt(HWND hwnd, INT id)
{
    TCHAR szBuffer[80];
    INT nCount;

    nCount = SendDlgItemMessage(hwnd, id, CB_GETCURSEL, 0, 0);
    if (nCount == CB_ERR)
        return 0;

    nCount = SendDlgItemMessage(hwnd, id, CB_GETLBTEXT, (WPARAM)nCount, (LPARAM)szBuffer);
    if (nCount == CB_ERR)
        return 0;

    return _ttoi(szBuffer);
}


static INT
GetEditedComboInt(HWND hwnd, INT id)
{
    INT nCount;
    BOOL bTranslated;

    nCount = GetDlgItemInt(hwnd, id, &bTranslated, FALSE);
    if (bTranslated == FALSE)
        return 12;

    return nCount;
}



INT_PTR CALLBACK
AdvAppearanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	INT iListIndex;
	GLOBALS* g;

	g = (GLOBALS*)GetWindowLongPtr(hwndDlg, DWLP_USER);

	switch (uMsg)
	{
		case WM_INITDIALOG:
			g = (GLOBALS*)lParam;
			SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)g);
			AdvAppearanceDlg_Init(hwndDlg, g);
			break;

		case WM_DESTROY:
			AdvAppearanceDlg_CleanUp(hwndDlg, g);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
					SaveCurrentValues(hwndDlg, g);
					EndDialog(hwndDlg, 0);
					break;

				case IDCANCEL:
					g->ThemeAdv = g->Theme;
					EndDialog(hwndDlg, 0);
					break;

				case IDC_APPEARANCE_PREVIEW:
					SaveCurrentValues(hwndDlg, g);
					SelectComboByElement(hwndDlg, IDC_ADVAPPEARANCE_ELEMENT, lParam);
					g->CurrentElement = (INT)lParam;
					UpdateControls(hwndDlg, g);
					break;

				case IDC_ADVAPPEARANCE_ELEMENT:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						SaveCurrentValues(hwndDlg, g);
						iListIndex = SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_ELEMENT, CB_GETCURSEL, 0, 0);
						g->CurrentElement = SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_ELEMENT, CB_GETITEMDATA, (WPARAM)iListIndex, 0);
						UpdateControls(hwndDlg, g);
					}
					break;

				case IDC_ADVAPPEARANCE_SIZE_E:
					if (g && HIWORD(wParam) == EN_CHANGE)
					{
						INT i = (INT)LOWORD(SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_SIZE_UD, UDM_GETPOS,0,0L));

						switch (g->CurrentElement)
						{
							case IDX_INACTIVE_CAPTION:
							case IDX_ACTIVE_CAPTION:
							case IDX_CAPTION_BUTTON:
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCYCAPTION, 0, i);
								break;

							case IDX_MENU:
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCYMENU, 0, i);
								break;

							case IDX_SCROLLBAR:
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCXSCROLLBAR, 0, i);
								break;

							case IDX_INACTIVE_BORDER:
							case IDX_ACTIVE_BORDER:
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCYSIZEFRAME, 0, i);
								break;
						}
					}
					break;

				case IDC_ADVAPPEARANCE_FONT_C:
					if (g && HIWORD(wParam) == CBN_SELCHANGE)
					{
						switch (g->CurrentElement)
						{
							case IDX_INACTIVE_CAPTION:
							case IDX_ACTIVE_CAPTION:
								GetSelectedComboText(hwndDlg, IDC_ADVAPPEARANCE_FONT_C,
									g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfFaceName);
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCAPTIONFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;

							case IDX_MENU:
								GetSelectedComboText(hwndDlg, IDC_ADVAPPEARANCE_FONT_C,
									g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfFaceName);
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETMENUFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;

							case IDX_DIALOG:
								GetSelectedComboText(hwndDlg, IDC_ADVAPPEARANCE_FONT_C,
									g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfFaceName);
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETDIALOGFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;
						}
					}
					break;

				case IDC_ADVAPPEARANCE_FONTSIZE_E:
					if (g && HIWORD(wParam) == CBN_SELCHANGE)
					{
						HDC hdcDlg = GetDC(hwndDlg);
						INT i;

						switch (g->CurrentElement)
						{
							case IDX_INACTIVE_CAPTION:
							case IDX_ACTIVE_CAPTION:
								i = GetSelectedComboInt(hwndDlg, IDC_ADVAPPEARANCE_FONTSIZE_E);
								g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfHeight =
									-MulDiv(i , GetDeviceCaps(hdcDlg, LOGPIXELSY), 72);
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCAPTIONFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;

							case IDX_MENU:
								i = GetSelectedComboInt(hwndDlg, IDC_ADVAPPEARANCE_FONTSIZE_E);
								g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfHeight =
									-MulDiv(i , GetDeviceCaps(hdcDlg, LOGPIXELSY), 72);
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETMENUFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;

							case IDX_DIALOG:
								i = GetSelectedComboInt(hwndDlg, IDC_ADVAPPEARANCE_FONTSIZE_E);
								g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfHeight =
									-MulDiv(i , GetDeviceCaps(hdcDlg, LOGPIXELSY), 72);
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETDIALOGFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;
						}

						ReleaseDC(hwndDlg, hdcDlg);
					}
					else if (g && HIWORD(wParam) == CBN_EDITCHANGE)
					{
						HDC hdcDlg = GetDC(hwndDlg);
						INT i;

						switch (g->CurrentElement)
						{
							case IDX_INACTIVE_CAPTION:
							case IDX_ACTIVE_CAPTION:
								i = GetEditedComboInt(hwndDlg, IDC_ADVAPPEARANCE_FONTSIZE_E);
								g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfHeight =
									-MulDiv(i , GetDeviceCaps(hdcDlg, LOGPIXELSY), 72);
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCAPTIONFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;

							case IDX_MENU:
								i = GetEditedComboInt(hwndDlg, IDC_ADVAPPEARANCE_FONTSIZE_E);
								g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfHeight =
									-MulDiv(i , GetDeviceCaps(hdcDlg, LOGPIXELSY), 72);
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETMENUFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;

							case IDX_DIALOG:
								i = GetEditedComboInt(hwndDlg, IDC_ADVAPPEARANCE_FONTSIZE_E);
								g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfHeight =
									-MulDiv(i , GetDeviceCaps(hdcDlg, LOGPIXELSY), 72);
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETDIALOGFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;
						}

						ReleaseDC(hwndDlg, hdcDlg);
					}
					break;

				case IDC_ADVAPPEARANCE_FONTBOLD:
					if (g && HIWORD(wParam) == BN_CLICKED)
					{
						INT i;

						switch (g->CurrentElement)
						{
							case IDX_INACTIVE_CAPTION:
							case IDX_ACTIVE_CAPTION:
								i = (INT)SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTBOLD, BM_GETCHECK, 0, 0);
								g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfWeight =
									(i == BST_CHECKED) ? FW_BOLD : FW_NORMAL;

								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCAPTIONFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;

							case IDX_MENU:
								i = (INT)SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTBOLD, BM_GETCHECK, 0, 0);

								g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfWeight =
									(i == BST_CHECKED) ? FW_BOLD : FW_NORMAL;
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETMENUFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;

							case IDX_DIALOG:
								i = (INT)SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTBOLD, BM_GETCHECK, 0, 0);

								g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfWeight =
									(i == BST_CHECKED) ? FW_BOLD : FW_NORMAL;

								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETDIALOGFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;
						}
					}
					break;

				case IDC_ADVAPPEARANCE_FONTITALIC:
					if (g && HIWORD(wParam) == BN_CLICKED)
					{
						INT i;

						switch (g->CurrentElement)
						{
							case IDX_INACTIVE_CAPTION:
							case IDX_ACTIVE_CAPTION:
								i = (INT)SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTITALIC, BM_GETCHECK, 0, 0);

								g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfItalic =
									(i == BST_CHECKED) ? TRUE : FALSE;

								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETCAPTIONFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;

							case IDX_MENU:
								i = (INT)SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTITALIC, BM_GETCHECK, 0, 0);

								g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfItalic =
									(i == BST_CHECKED) ? TRUE : FALSE;

								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETMENUFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;

							case IDX_DIALOG:
								i = (INT)SendDlgItemMessage(hwndDlg, IDC_ADVAPPEARANCE_FONTITALIC, BM_GETCHECK, 0, 0);

								g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font].lfItalic =
									(i == BST_CHECKED) ? TRUE : FALSE;

								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SETDIALOGFONT, 0,
									(LPARAM)&g->ThemeAdv.lfFont[g_Assignment[g->CurrentElement].Font]);
								break;
						}
					}
					break;

				case IDC_ADVAPPEARANCE_COLOR1_B:
					GetColor(hwndDlg, g, 0);
					break;

				case IDC_ADVAPPEARANCE_COLOR2_B:
					GetColor(hwndDlg, g, 1);
					break;

				case IDC_ADVAPPEARANCE_FONTCOLOR_B:
					GetColor(hwndDlg, g, 2);
					break;

				default:
					return FALSE;
			}
			break;

		default:
			return FALSE;
	}

	return TRUE;
}
