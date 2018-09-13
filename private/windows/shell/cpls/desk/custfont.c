/*  CUSTFONT.C
**
**  Copyright (C) Microsoft, 1993, All Rights Reserved.
**
**  History:
**
*/
#include <windows.h>
#include "desk.h"
#include "deskid.h"
#include "dragsize.h"
#include <help.h>

static struct
{
        UINT wWidth;
        UINT wHeight;
        UINT wSizeFontId;

        UINT wStartPos;
        UINT wStartPix;
} g;

static UINT g_iCurPercent;
static RECT g_rcRuler;
static TCHAR g_szRulerDirections[200];
static TCHAR g_szSample[100];
static TCHAR g_szSampleFace[32];

static int g_cxRulerDirections;

static BOOL g_bTypeTimer = FALSE;

static TCHAR szPctD[] = TEXT("%d");
static TCHAR szPercentNum[] = TEXT("%d%%");

#define NUM_DEFPERCENTS 5
static UINT g_DefaultPercents[NUM_DEFPERCENTS] = {75, 100, 125, 150, 200};

#define MAX_PERCENT	500
#define MIN_PERCENT	20

#define GETPERCENT(dpi)	((dpi * 100 + 50) / 96)
#define GETDPI(percent)	((percent * 96 + 48) /100)

#define UPDATE_CURPER	0x0001
#define UPDATE_COMBO	0x0002
#define UPDATE_SAMPLE	0x0004
#define UPDATE_RULER	0x0008
#define UPDATE_ALL	0x000F

void NEAR PASCAL DrawRuler(HWND hDlg, LPDRAWITEMSTRUCT lpdis)
{
    int nFact;
    RECT rc;
    HDC hdc;
    int nPixInch;
    int i, j;
    TCHAR szTemp[10];
    int nOldMode;

    hdc = lpdis->hDC;
    nOldMode = SetBkMode(hdc, TRANSPARENT);
    // use g_rcRuler to draw the ruler.  it's already been spaced 
    rc = g_rcRuler;

    // first, draw the directions
    i = rc.left + ((rc.right - rc.left) - g_cxRulerDirections)/2;
    TextOut(hdc, i, lpdis->rcItem.top + g.wHeight/4,
				g_szRulerDirections, lstrlen(g_szRulerDirections));

    nPixInch = GETDPI(g_iCurPercent);

    // draw the top and left edge of the ruler
    DrawEdge(hdc, &rc, EDGE_ETCHED, BF_TOPLEFT);
    // rest of drawing happens just below the top
    rc.top += GetSystemMetrics(SM_CYEDGE);

    nFact = 1;
    // draw one of the etch heights (1", 1/2", 1/4") per iteration
    for (j=0; j<3; ++j)
    {
	for (i=0; ; ++i)
	{
	    rc.left = g_rcRuler.left + (j==0 ? i*nPixInch : (2*i+1)*nPixInch/nFact);
	    if (rc.left >= g_rcRuler.right)
	    {
		break;
	    }
	    DrawEdge(hdc, &rc, EDGE_ETCHED, BF_LEFT | BF_ADJUST);

	    // dominant etch deserves a number
	    if (j == 0)
	    {
		TextOut(hdc, rc.left+1, rc.bottom-g.wHeight,
					szTemp, wsprintf(szTemp, szPctD, i));
	    }
	}

	rc.bottom -= (rc.bottom - rc.top)/2;
	nFact *= 2;
    }

    SetBkMode(hdc, nOldMode);
}

void NEAR PASCAL CF_UpdateRuler(HWND hDlg)
{
    RECT rc;
    HWND hwnd;

    /* Don't do this stuff if the dialog is not
    ** visible yet, or other windows will flash.
    */
    if (IsWindowVisible(hDlg))
    {
	// don't invalidate top and left because they never change.
	rc = g_rcRuler;
	rc.left += GetSystemMetrics(SM_CXEDGE);
	rc.top += GetSystemMetrics(SM_CYEDGE);

	hwnd = GetDlgItem(hDlg, IDC_CUSTOMRULER);
	InvalidateRect(hwnd, &rc, TRUE);
	UpdateWindow(hwnd);
    }
}
void NEAR PASCAL CF_ShowNewPercent(HWND hDlg, UINT uPer)
{
    TCHAR szBuf[10];

    g_iCurPercent = uPer;

    wsprintf(szBuf, szPercentNum, uPer);
    SetWindowText(GetDlgItem(hDlg, IDC_CUSTOMCOMBO), szBuf);
    UpdateWindow(GetDlgItem(hDlg, IDC_CUSTOMCOMBO));
}

// Build lf with given face and height
//
int CALLBACK EnumProc(CONST LOGFONT *lplf, CONST TEXTMETRIC *lptm, DWORD nType, LPARAM lpData )
{
    *(LPLOGFONT)lpData = *lplf;
    return FALSE;
}

HFONT CreateFontWithFace(HWND hwnd, int nHeight, LPCTSTR lpszFace)
{
    LOGFONT lf;
    HDC     hdc;

    if(hdc = GetDC(hwnd))
    {
        EnumFontFamilies(hdc, lpszFace, EnumProc, (LPARAM)&lf);
        ReleaseDC(hwnd,hdc);
    }

    lf.lfHeight = nHeight;
    lf.lfWidth = lf. lfEscapement = lf.lfOrientation = 0;

    return CreateFontIndirect(&lf);
}

void NEAR PASCAL CF_UpdateData(HWND hDlg, UINT uPer, UINT flags)
{
    TCHAR szBuf[100];
    HFONT hfont;
    int i;
    HWND hwnd;
    int iDPI;

    if (flags & UPDATE_CURPER)
    {
	if (uPer == g_iCurPercent)
	    return;

	if (uPer < MIN_PERCENT)
	    uPer = MIN_PERCENT;
	else if (uPer > MAX_PERCENT)
	    uPer = MAX_PERCENT;

	g_iCurPercent = uPer;
    }
    if (flags & UPDATE_COMBO)
    {
	hwnd = GetDlgItem(hDlg, IDC_CUSTOMCOMBO);
	wsprintf(szBuf, szPercentNum, g_iCurPercent);
        i = (int)SendMessage(hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)szBuf);
	SendMessage(hwnd, CB_SETCURSEL, (WPARAM)i, 0L);
	if (i == -1)
	{
	    SetWindowText(hwnd, szBuf);
	    UpdateWindow(hwnd);
	}
    }
    if (flags & UPDATE_RULER)
	CF_UpdateRuler(hDlg);

    if (flags & UPDATE_SAMPLE)
    {
	iDPI = GETDPI(g_iCurPercent);

	// build and set string with DPI info
	hwnd = GetDlgItem(hDlg, IDC_CUSTOMSAMPLE);
        wsprintf(szBuf, g_szSample, (LPTSTR)g_szSampleFace, iDPI);
	SetWindowText(hwnd, szBuf);

	hfont = CreateFontWithFace(hwnd, -10 * iDPI / 72, g_szSampleFace);
	if (hfont)
	{
	    hfont = (HFONT)SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, 1L);
	    if (hfont)
		DeleteObject(hfont);
	}
    }
}

void NEAR PASCAL CF_ReadNewPercent(HWND hDlg)
{
    TCHAR szBuf[10];
    LPTSTR pstr;
    UINT uPer = 0;

    GetWindowText(GetDlgItem(hDlg, IDC_CUSTOMCOMBO), szBuf, ARRAYSIZE(szBuf));

    pstr = szBuf;
    while (*pstr && (*pstr != TEXT('%')))
    {
        if (*pstr >= TEXT('0') && *pstr <= TEXT('9'))
            uPer = uPer * 10 + (*pstr - TEXT('0'));

	pstr++;
    }

    CF_UpdateData(hDlg, uPer, UPDATE_ALL);
}

void NEAR PASCAL CF_InitDialog(HWND hDlg, UINT uDPI)
{
    HWND hwnd;
    HDC hdc;
    HFONT hfont;
    SIZE  szSize;
    int i;
    TCHAR szBuf[10];
    int iCurSel;

    g_iCurPercent = GETPERCENT(uDPI);

    hwnd = GetDlgItem(hDlg, IDC_CUSTOMCOMBO);
    iCurSel = -1;		// assume not in list
    for (i = 0; i < NUM_DEFPERCENTS; i++)
    {
	wsprintf(szBuf, szPercentNum, g_DefaultPercents[i]);
        SendMessage(hwnd, CB_INSERTSTRING, (WPARAM)i, (LPARAM)szBuf);
        SendMessage(hwnd, CB_SETITEMDATA, (WPARAM)i, g_DefaultPercents[i]);

	if (g_iCurPercent == g_DefaultPercents[i])
	    iCurSel = i;
    }
    SendMessage(hwnd, CB_SETCURSEL, (WPARAM)iCurSel, 0L);
    if (iCurSel == -1)
    {
	wsprintf(szBuf, szPercentNum, g_iCurPercent);
	SetWindowText(hwnd, szBuf);
    }

    hdc = GetDC(hDlg);
    hfont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0L);
    if (hfont)
        hfont = SelectObject(hdc, hfont);

    //dwSize = GetTextExtentPoint32(hdc, TEXT("0"), 1);
    GetTextExtentPoint32(hdc, TEXT("0"), 1, &szSize);

    g.wWidth = szSize.cx;
    g.wHeight = szSize.cy;

    LoadString(hInstance, IDS_RULERDIRECTION, g_szRulerDirections, ARRAYSIZE(g_szRulerDirections));

    //g_cxRulerDirections = LOWORD(GetTextExtent(hdc, g_szRulerDirections, lstrlen(g_szRulerDirections)));
    GetTextExtentPoint32(hdc, g_szRulerDirections, lstrlen(g_szRulerDirections), &szSize);
    g_cxRulerDirections = szSize.cx;

    if (hfont)
	SelectObject(hdc, hfont);
    ReleaseDC(hDlg, hdc);

    // calculate the rectangle for the actual ruler drawing in relation
    // to its window
    GetClientRect(GetDlgItem(hDlg, IDC_CUSTOMRULER), &g_rcRuler);
    g_rcRuler.left += g.wWidth;
    g_rcRuler.right -= g.wWidth;
    // room for explanatory string
    g_rcRuler.top += g.wHeight + g.wHeight/2;
    // bottom offset like the sides
    g_rcRuler.bottom -= g.wWidth;

    LoadString(hInstance, IDS_10PTSAMPLE, g_szSample, ARRAYSIZE(g_szSample));
    LoadString(hInstance, IDS_10PTSAMPLEFACENAME, g_szSampleFace, ARRAYSIZE(g_szSampleFace));
    CF_UpdateData(hDlg, 0, UPDATE_SAMPLE);
}

/////////////////////////////////////////////////////////////////////////////

const static DWORD FAR aCustFontHelpIds[] = {
	IDC_NO_HELP_1,      IDH_COMM_GROUPBOX,
	IDC_CUSTOMCOMBO,    IDH_CUSTOMFONTS_FONTSCALE,
	IDC_CUSTOMRULER,    IDH_CUSTOMFONTS_RULER,
	IDC_CUSTOMSAMPLE,   IDH_CUSTOMFONTS_SAMPLE,

	0, 0
};

BOOL CALLBACK CustomFontDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    HFONT hfont;
    int i;

    switch (uMessage)
    {
	case WM_CREATE:
	    break;

	case WM_INITDIALOG:
            CF_InitDialog(hDlg, (UINT)lParam);
	    break;

	case WM_DESTROY:
	    hfont = (HFONT)SendDlgItemMessage(hDlg, IDC_CUSTOMSAMPLE, WM_GETFONT, 0, 0L);
	    if (hfont)
		DeleteObject(hfont);
	    break;

	case WM_DRAWITEM:
	    if (wParam == IDC_CUSTOMRULER)
		DrawRuler(hDlg, (LPDRAWITEMSTRUCT)lParam);
	    break;

	case WM_TIMER:
	    if (g_bTypeTimer)
	    {
		KillTimer(hDlg, 13);
		g_bTypeTimer = FALSE;
		CF_ReadNewPercent(hDlg);
	    }
	    break;

	case WM_HELP:
            WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (DWORD)aCustFontHelpIds);
	    break;

	case WM_CONTEXTMENU:	  // right mouse click
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (DWORD)aCustFontHelpIds);
	    break;

	case WM_COMMAND:
            switch (LOWORD(wParam))
	    {
		case IDOK:
		    EndDialog(hDlg, GETDPI(g_iCurPercent));
		    break;

		case IDCANCEL:
		    EndDialog(hDlg, 0);
		    break;

		case IDC_CUSTOMRULER:
                    switch (HIWORD(wParam))
		    {
			case DSN_NCCREATE:
                            SetWindowLong((HWND)lParam, GWL_EXSTYLE,
                                    GetWindowLong((HWND)lParam, GWL_EXSTYLE) | WS_EX_WINDOWEDGE);
			    break;

			case DSN_BEGINDRAG:
			    // Set the focus to the corresponding edit ctl
			    SendMessage(hDlg, WM_NEXTDLGCTL,
					(WPARAM)GetDlgItem(hDlg, IDC_CUSTOMCOMBO), 1L);

                            SendMessage((HWND)lParam, DSM_DRAGPOS, 0, (LPARAM)&(g.wStartPos));

			    if ((int)g.wStartPos < g_rcRuler.left)
			    {
				g.wStartPos = g_rcRuler.left;
			    }
			    g.wStartPix = g_iCurPercent;
			    break;

			case DSN_DRAGGING:
			{
                            UINT wNow, wPix;
                            POINT pt;

                            //wNow = LOWORD(SendMessage((HWND)lParam, DSM_DRAGPOS, 0, 0L));
                            SendMessage((HWND)lParam, DSM_DRAGPOS, 0, (LPARAM)&pt);
                            wNow = pt.x;

			    if ((int)wNow < g_rcRuler.left)
			    {
			    	wNow = g_rcRuler.left;
			    }

			    wPix = LOWORD((DWORD)wNow*g.wStartPix/g.wStartPos);
			    if (wPix < MIN_PERCENT)
			    {
			    	wPix = MIN_PERCENT;
			    }
			    if (wPix > MAX_PERCENT)
			    {
			    	wPix = MAX_PERCENT;
			    }

			    if (wPix != g_iCurPercent)
			    {
				CF_ShowNewPercent(hDlg, wPix);
				CF_UpdateRuler(hDlg);
			    }
			    break;
			}

			case DSN_ENDDRAG:
			    CF_UpdateData(hDlg, 0, UPDATE_COMBO | UPDATE_SAMPLE);
			    break;

			default:
			    break;
			}
		    break;

		case IDC_CUSTOMCOMBO:
                    switch(HIWORD(wParam))
		    {
			case CBN_SELCHANGE:
			    i = (int)SendDlgItemMessage(hDlg, IDC_CUSTOMCOMBO, CB_GETCURSEL, 0, 0L);
			    if (i != CB_ERR)
			    {
				i = LOWORD(SendDlgItemMessage(hDlg, IDC_CUSTOMCOMBO, CB_GETITEMDATA, (WPARAM)i, 0L));
				CF_UpdateData(hDlg, (UINT)i, UPDATE_CURPER | UPDATE_SAMPLE | UPDATE_RULER);
			    }
			    break;

			case CBN_EDITCHANGE:
			    if (g_bTypeTimer)
			    {
				KillTimer(hDlg, 13);
			    }
			    g_bTypeTimer = TRUE;
			    SetTimer(hDlg, 13, 500, NULL);
			    break;
		    }
		    break;

		default:
			return(FALSE);
	    }
	    break;

	default:
	    return(FALSE);
    }
    return(TRUE);
}
