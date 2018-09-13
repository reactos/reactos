#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgWelcom.h"

#include "select.h"
extern HPALETTE g_hpal3D;

UINT IDMapT[3][2] = { 0, IDC_TEXT1,
					  1, IDC_TEXT2,
					  2, IDC_TEXT3
					};

// a-anilk; Re-write to use owner drawn controls: 05/08/99
CWelcomePg::CWelcomePg(
						   LPPROPSHEETPAGE ppsp
						   ) : WizardPage(ppsp, IDS_TEXTSIZETITLE, IDS_TEXTSIZESUBTITLE)
{
	m_dwPageId = IDD_WIZWELCOME;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
	
	m_nCurrentHilight = 0;
	m_nCurValueIndex = 0;
}


CWelcomePg::~CWelcomePg(
							VOID
							)
{
}


LRESULT
CWelcomePg::OnCommand(
						HWND hwnd,
						WPARAM wParam,
						LPARAM lParam
						)
{
	LRESULT lResult = 1;
	
	return lResult;
}

LRESULT CWelcomePg::OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	SetFocus(GetDlgItem(hwnd, IDC_TEXT1));

	LoadString(g_hInstDll, IDS_WELCOMETEXT1, m_szWelcomeText[0], ARRAYSIZE(m_szWelcomeText[0]));
	LoadString(g_hInstDll, IDS_WELCOMETEXT2, m_szWelcomeText[1], ARRAYSIZE(m_szWelcomeText[1]));
	LoadString(g_hInstDll, IDS_WELCOMETEXT3, m_szWelcomeText[2], ARRAYSIZE(m_szWelcomeText[2]));
	LoadString(g_hInstDll, IDS_WELCOMETEXT4, m_szWelcomeText[3], ARRAYSIZE(m_szWelcomeText[3]));

	LoadArrayFromStringTable(IDS_LKPREV_WELCOME_MINTEXTSIZES, m_rgnValues, &m_nCountValues);

	return 1;
}

void CWelcomePg::InvalidateRects(int PrevHilight)
{
	InvalidateRect(GetDlgItem(m_hwnd, IDMapT[PrevHilight][1]), NULL, TRUE);
}

// These is to set the Focus and sync the painting
LRESULT CWelcomePg::OnPSN_SetActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
{
	syncInit = FALSE;
	uIDEvent = SetTimer(hwnd, NULL, 100, NULL);
	m_nCurrentHilight = m_nCurValueIndex = 0;

	// Localization taken care... 9,11,15 in JPN
	if ( g_Options.m_nMinimalFontSize <=9 )
		m_nCurrentHilight = 0;
	else if (g_Options.m_nMinimalFontSize <=12 )
		m_nCurrentHilight = 1;
	else if (g_Options.m_nMinimalFontSize <=16 )
		m_nCurrentHilight = 2;

	m_nCurValueIndex = m_nCurrentHilight;

	return 0;
}

LRESULT CWelcomePg::OnTimer( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
	KillTimer(hwnd, uIDEvent);
	syncInit = TRUE;
	return 1;
}

// Sets the focus to the current item in OnInitDialog. 
void CWelcomePg::SetFocussedItem(int m_nCurrentHilight)
{
	SetFocus(GetDlgItem(m_hwnd, IDMapT[m_nCurrentHilight][1]));
}

LRESULT CWelcomePg::OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UINT idCtrl = (UINT) wParam;
	LPDRAWITEMSTRUCT lpDrawItemStruct = (LPDRAWITEMSTRUCT) lParam;
	int index;
	
	if ( !syncInit)
		SetFocussedItem(m_nCurrentHilight);


	switch(idCtrl)
	{

	case IDC_TEXT1:
		index = 0;
		break;
	
	case IDC_TEXT2:
		index = 1;
		break;
		
	case IDC_TEXT3:
		index = 2;
		break;

	default:
		_ASSERTE(FALSE);

	}
	
	// For each button, Check the state, And if the button is selected,
	// means that it has current focus, So Re-paint the previously hilighted and 
	// the current selected buttons....
	// Make sure we ignore the initial events so that we minimize the flicker...
	if ( (lpDrawItemStruct->itemState & ODS_FOCUS) && (m_nCurrentHilight != index))
	{
		if ( syncInit )
		{
			// Erase the previous one...
			InvalidateRects(m_nCurrentHilight);
			Sleep(100);
			m_nCurrentHilight= m_nCurValueIndex = index;
		}
	}
	Draw( lpDrawItemStruct, index );

	return 1;
}


void CWelcomePg::Draw(LPDRAWITEMSTRUCT ldi, int i)
{
	int nOldBkMode = SetBkMode(ldi->hDC, TRANSPARENT);
	HDC hdc = ldi->hDC;
	
	RECT rcOriginal = ldi->rcItem;
	HFONT hFontOld = (HFONT)SelectObject(hdc, g_Options.GetClosestMSSansSerif(m_rgnValues[i], (m_nCurrentHilight == i)));
	TextOut(hdc, rcOriginal.left + 10 , rcOriginal.top + 10 - i, m_szWelcomeText[i], lstrlen(m_szWelcomeText[i]));
	SelectObject(hdc, hFontOld);
	
	SetBkMode(ldi->hDC, nOldBkMode);

	//If current hi-lighted item, Then draw the bounding rectangle. 
	if ( m_nCurrentHilight == i)
	{
		DrawHilight(m_hwnd, ldi);
	}
}


LRESULT
CWelcomePg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	// Tell the second page that we've done something
	g_Options.m_bWelcomePageTouched = TRUE;

	g_Options.m_nMinimalFontSize = m_rgnValues[m_nCurValueIndex];

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
#if 0 
	// We are going to allow people to 'back' to this page
	// As a HACK, we want this page to come out only once.
	// If we try to remove ourselves, and the call the
	// default OnPSN_WizNext(), we'll get an assert when
	// that function tries to find the 'next' page.  Instead
	// we manually do the things from OnPSN_WizNext()

	// Remove ourself from the wizard after we select a size.
	DWORD dwTemp = IDD_WIZWELCOME;
	sm_WizPageOrder.RemovePages(&dwTemp, 1);
	SetWindowLong(hwnd, DWL_MSGRESULT, IDD_WIZWELCOME2);
	return TRUE;
#endif
}

