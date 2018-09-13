#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgMinTx2.h"

CMinText2Pg::CMinText2Pg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_LKPREV_MINTEXT2TITLE, IDS_LKPREV_MINTEXT2SUBTITLE)
{
	m_dwPageId = IDD_FNTWIZMINTEXT2;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
	m_pDisplayModes = NULL;
	m_nDisplayModes = 0;
	m_nBestDisplayMode = -1;
}


CMinText2Pg::~CMinText2Pg(
	VOID
	)
{
	if(m_pDisplayModes)
		delete [] m_pDisplayModes;
}

LRESULT
CMinText2Pg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	// Enumerate available video modes
	// JMC: TODO: Make Multi Monitory support
	// Check if SM_CMONITORS is > 0 then set text so we don't
	// change resolution.

	DEVMODE dm;
	// Calculate number of display modes
	for(m_nDisplayModes=0;m_nDisplayModes<2000;m_nDisplayModes++) // Limit to 2000 display modes.  If it is this high, something is wrong
		if(!EnumDisplaySettings(NULL, m_nDisplayModes, &dm))
			break;

	m_pDisplayModes = new CDisplayModeInfo[m_nDisplayModes];
	for(int i=0;i<m_nDisplayModes;i++)
		EnumDisplaySettings(NULL, i, &m_pDisplayModes[i].m_DevMode);

	UpdateControls();
	return 1;
}


void CMinText2Pg::UpdateControls()
{
	// This algorithm chooses which check boxes to set based on the
	// minimal legible font size specified in g_Options.m_nMinimalFontSize

	// HACK:
	g_Options.m_nMinimalFontSize = MulDiv(abs(g_Options.m_schemePreview.m_ncm.lfCaptionFont.lfHeight), 72, g_Options.m_nLogPixelsY);

	BOOL bSwitchRes = FALSE;
	BOOL bChangeFonts = FALSE;
	BOOL bUseMagnify = FALSE;
	switch(g_Options.m_nMinimalFontSize)
	{
	case 8:
	case 10:
	case 12:
		bChangeFonts = TRUE;
		break;
	case 14:
	case 16:
	case 18:
		bChangeFonts = TRUE;
		bSwitchRes = TRUE;
		break;
	case 20:
	case 22:
	case 24:
		bChangeFonts = TRUE;
		bUseMagnify = TRUE;
	}

	m_nBestDisplayMode = -1;

	DEVMODE dvmdOrig;
	memset(&dvmdOrig, 0, sizeof(dvmdOrig));

	HDC hdc = GetDC(NULL);  // Screen DC used to get current display settings
	dvmdOrig.dmPelsWidth        = GetDeviceCaps(hdc, HORZRES);
	dvmdOrig.dmPelsHeight       = GetDeviceCaps(hdc, VERTRES);
	dvmdOrig.dmBitsPerPel       = GetDeviceCaps(hdc, BITSPIXEL);
	dvmdOrig.dmDisplayFrequency = GetDeviceCaps(hdc, VREFRESH);
	ReleaseDC(NULL, hdc);
	
	for(int i=0;i<m_nDisplayModes;i++)
	{
		// Skip anything 'higher' than current mode
		if(		m_pDisplayModes[i].m_DevMode.dmPelsWidth > dvmdOrig.dmPelsWidth
			||	m_pDisplayModes[i].m_DevMode.dmPelsHeight > dvmdOrig.dmPelsHeight
			||	m_pDisplayModes[i].m_DevMode.dmBitsPerPel > dvmdOrig.dmBitsPerPel
			||	m_pDisplayModes[i].m_DevMode.dmDisplayFrequency > dvmdOrig.dmDisplayFrequency )
			continue;

		// Skip this if it is 'worse' than the current best mode
		if(		-1 != m_nBestDisplayMode
			&&	(		m_pDisplayModes[i].m_DevMode.dmPelsWidth < m_pDisplayModes[m_nBestDisplayMode].m_DevMode.dmPelsWidth
					||	m_pDisplayModes[i].m_DevMode.dmPelsHeight < m_pDisplayModes[m_nBestDisplayMode].m_DevMode.dmPelsHeight
					||	m_pDisplayModes[i].m_DevMode.dmBitsPerPel < m_pDisplayModes[m_nBestDisplayMode].m_DevMode.dmBitsPerPel
					||	m_pDisplayModes[i].m_DevMode.dmDisplayFrequency < m_pDisplayModes[m_nBestDisplayMode].m_DevMode.dmDisplayFrequency ) )
			continue;

		// Skip anything 'less than' 640 x 480
		if(		m_pDisplayModes[i].m_DevMode.dmPelsWidth < 640
			||	m_pDisplayModes[i].m_DevMode.dmPelsHeight < 480 )
			continue;


		// See if this is 'smaller' than the current resolution
		if(	m_pDisplayModes[i].m_DevMode.dmPelsHeight < dvmdOrig.dmPelsHeight )
			m_nBestDisplayMode = i;

	}


	// JMC: TODO: Handle if the user does not have permission to change
	// the display settings!!!!!!!!!!!!!!

	if(-1 == m_nBestDisplayMode)
	{
		bSwitchRes = FALSE;
		SetWindowText(GetDlgItem(m_hwnd, IDC_SZRESMESSAGE),
			__TEXT("There are no display resolutions that would be better for the size text you chose."));
		EnableWindow(GetDlgItem(m_hwnd, IDC_SWITCHRESOLUTION), FALSE);
	}
	else
	{
		TCHAR sz[200];
		wsprintf(sz, __TEXT("You are currently at %i x %i.  By switching to %i x %i, the text on screen will be more readable."),
			dvmdOrig.dmPelsWidth,
			dvmdOrig.dmPelsHeight,
			m_pDisplayModes[m_nBestDisplayMode].m_DevMode.dmPelsWidth,
			m_pDisplayModes[m_nBestDisplayMode].m_DevMode.dmPelsHeight);
		SetWindowText(GetDlgItem(m_hwnd, IDC_SZRESMESSAGE), sz);
		EnableWindow(GetDlgItem(m_hwnd, IDC_SWITCHRESOLUTION), TRUE);
	}

	Button_SetCheck(GetDlgItem(m_hwnd, IDC_SWITCHRESOLUTION), bSwitchRes);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_CHANGEFONTS), bChangeFonts);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_USEMAGNIFY), bUseMagnify);
}


LRESULT
CMinText2Pg::OnCommand(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	LRESULT lResult = 1;

	WORD wNotifyCode = HIWORD(wParam);
	WORD wCtlID      = LOWORD(wParam);
	HWND hwndCtl     = (HWND)lParam;


	// JMC: NOTE: DO NOT CALL UpdateControls()
	// UpdateControls() should only be called when entering this page
	// since it sets the check boxes based on the minimal font size
	// determined by the previous wizard page
	
	/*
	switch(wCtlID)
	{
	case IDC_BTN_INCREASE_SIZE:
	UpdateControls();
	lResult = 0;
	break;
	
	  default:
	  break;
	  }
	*/
	return lResult;
}

LRESULT
CMinText2Pg::OnPSN_SetActive(
							 HWND hwnd, 
							 INT idCtl, 
							 LPPSHNOTIFY pnmh
							 )
{
	// Call the base class
	WizardPage::OnPSN_SetActive(hwnd, idCtl, pnmh);

	// Make sure our check boxes reflect any change in the minimal
	// font size specified by g_Options.m_nMinimalFontSize
	UpdateControls();

	return TRUE;
}

LRESULT
CMinText2Pg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	// Get original data
	DEVMODE dvmdOrig;
	memset(&dvmdOrig, 0, sizeof(dvmdOrig));

	HDC hdc = GetDC(NULL);  // Screen DC used to get current display settings
	int nLogPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
	// JMC: HOW DO WE GET dmDisplayFlags?
	// TODO: Maybe use ChangeDisplaySettings(NULL, 0) to restore original mode
	dvmdOrig.dmSize = sizeof(dvmdOrig);
	dvmdOrig.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | /* DM_DISPLAYFLAGS | */DM_DISPLAYFREQUENCY;
	dvmdOrig.dmPelsWidth        = GetDeviceCaps(hdc, HORZRES);
	dvmdOrig.dmPelsHeight       = GetDeviceCaps(hdc, VERTRES);
	dvmdOrig.dmBitsPerPel       = GetDeviceCaps(hdc, BITSPIXEL);
	dvmdOrig.dmDisplayFrequency = GetDeviceCaps(hdc, VREFRESH);
	ReleaseDC(NULL, hdc);

	NONCLIENTMETRICS ncm;
	memset(&ncm, 0, sizeof(ncm));
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

	NONCLIENTMETRICS ncmOrig = ncm;
	LOGFONT lfIconOrig;
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lfIconOrig), &lfIconOrig, 0);

	BOOL bChangeRes = Button_GetCheck(GetDlgItem(m_hwnd, IDC_SWITCHRESOLUTION)) && -1 != m_nBestDisplayMode;
	BOOL bChangeFont = Button_GetCheck(GetDlgItem(m_hwnd, IDC_CHANGEFONTS));
	BOOL bMagnifier = Button_GetCheck(GetDlgItem(m_hwnd, IDC_USEMAGNIFY));
	BOOL bSomethingChanged = bChangeRes || bChangeFont || bMagnifier;

	if(bSomethingChanged)
	{
		if(IDOK != MessageBox(m_hwnd,
			__TEXT("These changes are going to take effect immediately.  If you don't \
like the new settings, you will be given an opportunity to undo the changes.  If for some \
reason your display becomes illegible, press the ESCAPE key once."), __TEXT("Accessability Wizard"), MB_OKCANCEL))
		{
			SetWindowLong(hwnd, DWL_MSGRESULT, m_dwPageId);
			return TRUE;
		}
	}


	if(bChangeRes)
	{
		// Change the resolution
		if(DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettings(&m_pDisplayModes[m_nBestDisplayMode].m_DevMode, CDS_TEST))
		{
			// JMC: TODO: What should we do.  This may mean that a RESTART is required
		}
		else
			ChangeDisplaySettings(&m_pDisplayModes[m_nBestDisplayMode].m_DevMode, CDS_UPDATEREGISTRY | CDS_GLOBAL);
	}
	if(bChangeFont)
	{
		LOGFONT lf;
		memset(&lf, 0, sizeof(lf));
		lf.lfHeight = -MulDiv(g_Options.m_nMinimalFontSize, nLogPixelsY, 72);
		lf.lfWeight = FW_BOLD;
		lstrcpy(lf.lfFaceName, __TEXT("Tahoma"));

		// Captions are BOLD
		ncm.lfCaptionFont = lf;

		lf.lfWeight = FW_NORMAL;

		ncm.lfSmCaptionFont = lf;// JMC: TODO: Use a different Small Caption Font
		ncm.lfMenuFont = lf;
		ncm.lfStatusFont = lf;
		ncm.lfMessageFont = lf;

		// DYNAMICS
		// JMC: TODO: Change caption height / menu height / button width to match.
		// JMC: HACK
		lf.lfWeight = FW_BOLD; // Caption is BOLD
		HFONT hFont = CreateFontIndirect(&lf);
		lf.lfWeight = FW_NORMAL; // Still need lf for ICON
		TEXTMETRIC tm;
		HDC hdc = GetDC(m_hwnd);
		HFONT hfontOld = (HFONT)SelectObject(hdc, hFont);
		GetTextMetrics(hdc, &tm);
		if (hfontOld)
			SelectObject(hdc, hfontOld);
		ReleaseDC(m_hwnd, hdc);


		int cyBorder = GetSystemMetrics(SM_CYBORDER);
		int nSize = abs(lf.lfHeight) + abs(tm.tmExternalLeading) + 2 * cyBorder;
		nSize = max(nSize, GetSystemMetrics(SM_CYICON)/2 + 2 * cyBorder);

		ncm.iCaptionWidth = nSize;
		ncm.iCaptionHeight = nSize;
		ncm.iSmCaptionWidth = nSize;
		ncm.iSmCaptionHeight = nSize;
		ncm.iMenuWidth = nSize;
		ncm.iMenuHeight = nSize;

		SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(ncm), &ncm, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		SystemParametersInfo(SPI_SETICONTITLELOGFONT, sizeof(lf), &lf, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		SendNotifyMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
		SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, (LPARAM)__TEXT("WindowMetrics"));
	}
	if(bMagnifier)
	{
		ShellExecute(NULL, NULL, __TEXT("Magnify.exe"), NULL, NULL, SW_SHOWNORMAL);
	}
	if(bSomethingChanged && IDYES != MessageBox(m_hwnd, __TEXT("Are these settings acceptable"), __TEXT("Accessability Wizard"), MB_YESNOCANCEL))
	{
		// Restore original settings
		ChangeDisplaySettings(&dvmdOrig, CDS_UPDATEREGISTRY | CDS_GLOBAL);

		SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(ncmOrig), &ncmOrig, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		SystemParametersInfo(SPI_SETICONTITLELOGFONT, sizeof(lfIconOrig), &lfIconOrig, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		SendNotifyMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
		SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, (LPARAM)__TEXT("WindowMetrics"));

		SetWindowLong(hwnd, DWL_MSGRESULT, m_dwPageId);
		return TRUE;
	}
	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}