//
//	config.c
//
//	Screensaver Configuration dialog
//
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "resource.h"
#include "globals.h"
#include "message.h"
#include "matrix.h"

// needed for slider-controls
//#pragma comment(lib, "comctl32.lib")

//
//	Called once for every font - add to list
//
int CALLBACK EnumFontProc(ENUMLOGFONT *lpelfe, NEWTEXTMETRIC *lpntme, int FontType, LPARAM lParam)
{
	SendMessage((HWND)lParam, CB_ADDSTRING, 0, (LPARAM)lpelfe->elfLogFont.lfFaceName);
	return 1;
}

//
// Add every fontname into specified combobox
//
void AddFonts(HWND hwndCombo)
{
	HDC		hdc;
	LOGFONT lf;

	lf.lfCharSet		= ANSI_CHARSET;
	lf.lfPitchAndFamily = 0;
	lf.lfFaceName[0]    = _T('\0');

	hdc = GetDC(0);
	EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontProc, (LONG)hwndCombo, 0);
	ReleaseDC(0, hdc);
}

//
//	Redraw preview control with current font/fontsize
//
void UpdatePreview(HWND hwnd)
{
	g_nFontSize = SendDlgItemMessage(hwnd, IDC_SLIDER4, TBM_GETPOS,0, 0);
	SetMessageFont(hwnd, g_szFontName, g_nFontSize, g_fFontBold);
	InvalidateRect(GetDlgItem(hwnd, IDC_PREVIEW), 0, 0);
}

//
//	Dialogbox procedure for Configuration window
//
BOOL CALLBACK ConfigDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static TCHAR buf[256];
	HWND	hwndCombo, hwndCtrl;
	int		index, items, val;
	RECT	rect;
	static  int prevwidth, prevheight;

	switch(uMsg)
	{
	case WM_INITDIALOG:

		prevwidth  = GetSystemMetrics(SM_CXSCREEN) / GLYPH_WIDTH;
		prevheight = GetSystemMetrics(SM_CYSCREEN) / GLYPH_HEIGHT + 1;

		//Add any saved messages to the combo box
		for(index = 0; index < g_nNumMessages; index++)
		{
			if(lstrlen(g_szMessages[index]) > 0)
				SendDlgItemMessage(hwnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)g_szMessages[index]);
		}

		//select the first message, and preview it
		SendDlgItemMessage(hwnd, IDC_COMBO1, CB_SETCURSEL, 0, 0);
		
		SendDlgItemMessage(hwnd, IDC_SLIDER1, TBM_SETRANGE, 0, MAKELONG(SPEED_MIN, SPEED_MAX));
		SendDlgItemMessage(hwnd, IDC_SLIDER2, TBM_SETRANGE, 0, MAKELONG(DENSITY_MIN, DENSITY_MAX));
		SendDlgItemMessage(hwnd, IDC_SLIDER3, TBM_SETRANGE, 0, MAKELONG(MSGSPEED_MIN, MSGSPEED_MAX));
		SendDlgItemMessage(hwnd, IDC_SLIDER4, TBM_SETRANGE, 0, MAKELONG(FONT_MIN, FONT_MAX));

		//SendDlgItemMessage(hwnd, IDC_SLIDER1, TBM_SETTICFREQ, 5, 0);
		SendDlgItemMessage(hwnd, IDC_SLIDER2, TBM_SETTICFREQ, 5, 0);
		SendDlgItemMessage(hwnd, IDC_SLIDER3, TBM_SETTICFREQ, 50, 0);
		SendDlgItemMessage(hwnd, IDC_SLIDER4, TBM_SETTICFREQ, 2, 0);
		
		SendDlgItemMessage(hwnd, IDC_SLIDER1, TBM_SETPOS, TRUE, g_nMatrixSpeed);
		SendDlgItemMessage(hwnd, IDC_SLIDER2, TBM_SETPOS, TRUE, g_nDensity);
		SendDlgItemMessage(hwnd, IDC_SLIDER3, TBM_SETPOS, TRUE, g_nMessageSpeed);
		SendDlgItemMessage(hwnd, IDC_SLIDER4, TBM_SETPOS, TRUE, g_nFontSize);

		CheckDlgButton(hwnd, IDC_RANDOM, g_fRandomizeMessages);
		CheckDlgButton(hwnd, IDC_BOLD, g_fFontBold);

		AddFonts(GetDlgItem(hwnd, IDC_COMBO2));
		index = SendDlgItemMessage(hwnd, IDC_COMBO2, CB_FINDSTRING, 0, (LPARAM)g_szFontName);
		SendDlgItemMessage(hwnd, IDC_COMBO2, CB_SETCURSEL, index, 0);
		UpdatePreview(hwnd);
		return 0;

	case WM_DESTROY:
		//DeInitMessage();
		return 0;

	case WM_CTLCOLORSTATIC:

		if(GetDlgCtrlID((HWND)lParam) == IDC_ABOUT)
		{
			SetTextColor((HDC)wParam, RGB(0,80,0));
			SetBkColor((HDC)wParam, GetSysColor(COLOR_3DFACE));
			return (BOOL)GetSysColorBrush(COLOR_3DFACE);
		}
		else if(GetDlgCtrlID((HWND)lParam) == IDC_PREVIEW)
		{
			HDC hdc = (HDC)wParam;
			RECT clip;

			GetDlgItemText(hwnd, IDC_COMBO1, buf, 256);
			
			GetClientRect((HWND)lParam, &rect);

			if(prevwidth < rect.right)
			{
				rect.left = (rect.right-prevwidth) / 2;
				rect.right = rect.left + prevwidth;
			}
			else
			{
				rect.left  = 0;
				rect.right = prevwidth;
			}		

			if(prevheight < rect.bottom)
			{
				rect.top = (rect.bottom-prevheight) / 2;
				rect.bottom = rect.top + prevheight;
			}
			else
			{
				rect.top = 0;
				rect.bottom = prevheight;
			}

			SetTextColor(hdc, RGB(128,255,128));
			SetBkColor(hdc, 0);

			//SetRect(&rect, 0, 0, PrevMessage->width, MAXMSG_HEIGHT);
			CopyRect(&clip, &rect);
			FillRect(hdc, &rect, GetStockObject(BLACK_BRUSH));

			SelectObject(hdc, g_hFont);

			InflateRect(&clip, 2, 2);

			FrameRect(hdc, &clip, GetSysColorBrush(COLOR_3DSHADOW));
			IntersectClipRect(hdc, rect.left, rect.top, rect.right, rect.bottom);

			// figure out where the bounding rectangle should be
			DrawText(hdc, buf, -1, &rect, DT_CENTER|DT_VCENTER|DT_WORDBREAK|DT_CALCRECT);
			OffsetRect(&rect, (prevwidth-(rect.right-rect.left))/2, (prevheight-(rect.bottom-rect.top))/2);

			// now draw it!
			DrawText(hdc, buf, -1, &rect, DT_CENTER|DT_VCENTER|DT_WORDBREAK);
			

			return (BOOL)GetStockObject(NULL_BRUSH);
		}	
		else
		{
			break;
		}

	case WM_HSCROLL:

		if((HWND)lParam == GetDlgItem(hwnd, IDC_SLIDER4))
		{
			// one of the sliders changed..update
			UpdatePreview(hwnd);
		}

		return 0;

	case WM_COMMAND:

		switch(HIWORD(wParam))
		{
		case CBN_EDITCHANGE:
		case CBN_SELCHANGE:

			//fall through to Preview:
			index = SendDlgItemMessage(hwnd, IDC_COMBO2, CB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hwnd, IDC_COMBO2, CB_GETLBTEXT, index, (LPARAM)g_szFontName);
			//SetMessageFont(hwnd, g_szFontName, g_nFontSize, TRUE);
			
			UpdatePreview(hwnd);
			return 0;
		}

		switch(LOWORD(wParam))
		{
		case IDC_RANDOM:
			g_fRandomizeMessages = IsDlgButtonChecked(hwnd, IDC_RANDOM);
			break;

		case IDC_BOLD:
			g_fFontBold = IsDlgButtonChecked(hwnd, IDC_BOLD);
			UpdatePreview(hwnd);
			break;

		case IDOK:
			
			hwndCtrl = GetDlgItem(hwnd, IDC_COMBO1);

			items = min(MAX_MESSAGES, SendMessage(hwndCtrl, CB_GETCOUNT, 0, 0));

			for(index = 0; index < items; index++)
			{
				SendMessage(hwndCtrl, CB_GETLBTEXT, index, (LPARAM)g_szMessages[index]);
			}

			g_nNumMessages = items;
			
			//matrix speed
			val = SendDlgItemMessage(hwnd, IDC_SLIDER1, TBM_GETPOS, 0, 0);	
			if(val >= SPEED_MIN && val <= SPEED_MAX)
				g_nMatrixSpeed = val;

			//density
			val = SendDlgItemMessage(hwnd, IDC_SLIDER2, TBM_GETPOS, 0, 0);	
			if(val >= DENSITY_MIN && val <= DENSITY_MAX)
				g_nDensity = val;

			//message speed
			val = SendDlgItemMessage(hwnd, IDC_SLIDER3, TBM_GETPOS, 0, 0);	
			if(val >= MSGSPEED_MIN && val <= MSGSPEED_MAX)
				g_nMessageSpeed = val;

			//font size
			val = SendDlgItemMessage(hwnd, IDC_SLIDER4, TBM_GETPOS, 0, 0);	
			if(val >= FONT_MIN && val <= FONT_MAX)
				g_nFontSize = val;

			SaveSettings();
			EndDialog(hwnd, 0);
			return 0;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;

		case IDC_ADD:
			
			hwndCombo = GetDlgItem(hwnd, IDC_COMBO1);
			
			if(GetWindowText(hwndCombo, buf, 256))
			{
				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)buf);
			}
			
			UpdatePreview(hwnd);

			return 0;

		case IDC_REMOVE:
			hwndCombo = GetDlgItem(hwnd, IDC_COMBO1);
			GetWindowText(hwndCombo, buf, 256);

			index = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
			SendMessage(hwndCombo, CB_DELETESTRING, index, 0);

			SendMessage(hwndCombo, CB_SETCURSEL, 0, 0);
			UpdatePreview(hwnd);
			return 0;
		}
		return 0;

	case WM_CLOSE:
		EndDialog(hwnd, 0);
		return 0;
	}
	return 0;
}

//
//	Display the configuration dialog
//
int Configure(HWND hwndParent)
{
	INITCOMMONCONTROLSEX icc;

	icc.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;
	icc.dwSize = sizeof(icc);

	InitCommonControlsEx(&icc);

#ifdef _DEBUG
	if(hwndParent == NULL)
		hwndParent = 0;
#else
	if(hwndParent == NULL)
		hwndParent = GetForegroundWindow();
#endif

	DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_CONFIG), hwndParent, ConfigDlgProc);
	
	return 0;
}

