#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgWelco2.h"

// Intelli-menu regsitry
#define REGSTR_EXPLORER TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer")
#define REGSTR_INTELLIMENU REGSTR_EXPLORER TEXT("\\Advanced")

#define REGSTR_IE TEXT("Software\\Microsoft\\Internet Explorer\\Main")
#define STRMENU TEXT("IntelliMenus")
#define FAVMENU TEXT("FavIntelliMenus")

CWelcome2Pg::CWelcome2Pg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_WELCOME2TITLE, IDS_WELCOME2SUBTITLE)
{
	m_dwPageId = IDD_WIZWELCOME2;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
	m_pDisplayModes = NULL;
	m_nDisplayModes = 0;
	m_nBestDisplayMode = -1;
	m_IntlVal = FALSE;


	// These are our state variables so we know not to do these things twice.
	m_bMagnifierRun = FALSE;
	m_bResolutionSwitched = FALSE;
	m_bFontsChanged = FALSE;

}


CWelcome2Pg::~CWelcome2Pg(
	VOID
	)
{
	if(m_pDisplayModes)
		delete [] m_pDisplayModes;
}

LRESULT
CWelcome2Pg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	// Enumerate available video modes
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


	m_nBestDisplayMode = -1;

	memset(&m_dvmdOrig, 0, sizeof(m_dvmdOrig));

	HDC hdc = GetDC(NULL);  // Screen DC used to get current display settings
	// JMC: HOW DO WE GET dmDisplayFlags?
	// TODO: Maybe use ChangeDisplaySettings(NULL, 0) to restore original mode
	m_dvmdOrig.dmSize = sizeof(m_dvmdOrig);
	m_dvmdOrig.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | /* DM_DISPLAYFLAGS | */(g_Options.m_bWin95?0:DM_DISPLAYFREQUENCY);
	m_dvmdOrig.dmPelsWidth        = GetDeviceCaps(hdc, HORZRES);
	m_dvmdOrig.dmPelsHeight       = GetDeviceCaps(hdc, VERTRES);
	m_dvmdOrig.dmBitsPerPel       = GetDeviceCaps(hdc, BITSPIXEL);
	m_dvmdOrig.dmDisplayFrequency = g_Options.m_bWin95?0:GetDeviceCaps(hdc, VREFRESH);
	ReleaseDC(NULL, hdc);


	for(i=0;i<m_nDisplayModes;i++)
	{
		// Skip anything 'higher' than current mode
		if(		m_pDisplayModes[i].m_DevMode.dmPelsWidth > m_dvmdOrig.dmPelsWidth
			||	m_pDisplayModes[i].m_DevMode.dmPelsHeight > m_dvmdOrig.dmPelsHeight
			||	m_pDisplayModes[i].m_DevMode.dmBitsPerPel > m_dvmdOrig.dmBitsPerPel
			||	(!g_Options.m_bWin95 && m_pDisplayModes[i].m_DevMode.dmDisplayFrequency > m_dvmdOrig.dmDisplayFrequency) )
			continue;

		// Skip this if it is 'worse' than the current best mode
		if(		-1 != m_nBestDisplayMode
			&&	(		m_pDisplayModes[i].m_DevMode.dmPelsWidth < m_pDisplayModes[m_nBestDisplayMode].m_DevMode.dmPelsWidth
					||	m_pDisplayModes[i].m_DevMode.dmPelsHeight < m_pDisplayModes[m_nBestDisplayMode].m_DevMode.dmPelsHeight
					||	m_pDisplayModes[i].m_DevMode.dmBitsPerPel < m_pDisplayModes[m_nBestDisplayMode].m_DevMode.dmBitsPerPel
					||	(!g_Options.m_bWin95 && m_pDisplayModes[i].m_DevMode.dmDisplayFrequency < m_pDisplayModes[m_nBestDisplayMode].m_DevMode.dmDisplayFrequency) ) )
			continue;

		// Skip anything 'less than' 800 x 600 (JMC: Used to be 640 x 480)
		if(		m_pDisplayModes[i].m_DevMode.dmPelsWidth < 800
			||	m_pDisplayModes[i].m_DevMode.dmPelsHeight < 600 )
			continue;


		// See if this is 'smaller' than the current resolution
		if(	m_pDisplayModes[i].m_DevMode.dmPelsHeight < m_dvmdOrig.dmPelsHeight )
			m_nBestDisplayMode = i;

	}

	// Get original metrics
	GetNonClientMetrics(&m_ncmOrig, &m_lfIconOrig);
	
	SetCheckBoxesFromWelcomePageInfo();

	// Set the Personalized menu check box
	HKEY hKey;
	DWORD dwType;
	TCHAR lpszData[24];
	DWORD dwCount = 24;

	if(ERROR_SUCCESS == RegOpenKey( HKEY_CURRENT_USER, REGSTR_INTELLIMENU,&hKey))
	{
		if ( ERROR_SUCCESS == RegQueryValueEx( hKey, STRMENU, NULL, &dwType, (LPBYTE)lpszData, &dwCount ) )
		{
			if ( lstrcmp(lpszData, TEXT("No") ) == 0 )
				m_IntlVal = TRUE;
		}
	}

	Button_SetCheck(GetDlgItem(hwnd, IDC_PERMENU), m_IntlVal);

	return 1;
}


void CWelcome2Pg::UpdateControls()
{
	BOOL bChangeRes = Button_GetCheck(GetDlgItem(m_hwnd, IDC_SWITCHRESOLUTION));
	BOOL bChangeFont = Button_GetCheck(GetDlgItem(m_hwnd, IDC_CHANGEFONTS));
	BOOL bMagnifier = Button_GetCheck(GetDlgItem(m_hwnd, IDC_USEMAGNIFY));
	DWORD_PTR result;

	if(bChangeRes && !m_bResolutionSwitched)
	{
		if(IDOK != StringTableMessageBox(m_hwnd,IDS_WIZCHANGESHAPPENINGTEXT, IDS_WIZCHANGESHAPPENINGTITLE, MB_OKCANCEL))
		{
			// The user does not want to do this
			Button_SetCheck(GetDlgItem(m_hwnd, IDC_SWITCHRESOLUTION), FALSE);
		}
		else
		{
			// Lets change the resolution
			if(DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettings(&m_pDisplayModes[m_nBestDisplayMode].m_DevMode, CDS_TEST))
			{
			}
			else
				ChangeDisplaySettings(&m_pDisplayModes[m_nBestDisplayMode].m_DevMode, CDS_UPDATEREGISTRY | CDS_GLOBAL);

			if(IDOK != StringTableMessageBox(m_hwnd, IDS_WIZCANCELCHANGESTEXT, IDS_WIZCANCELCHANGESTITLE, MB_OKCANCEL))
			{
				// Restore original settings
				ChangeDisplaySettings(&m_dvmdOrig, CDS_UPDATEREGISTRY | CDS_GLOBAL);
				Button_SetCheck(GetDlgItem(m_hwnd, IDC_SWITCHRESOLUTION), FALSE);
			}
			else
				m_bResolutionSwitched = TRUE; // We REALLY changed the settings
		}
	}
	else if (!bChangeRes && m_bResolutionSwitched)
	{
		m_bResolutionSwitched = FALSE;
		// Restore original settings
		ChangeDisplaySettings(&m_dvmdOrig, CDS_UPDATEREGISTRY | CDS_GLOBAL);
	}

	if(bChangeFont && !m_bFontsChanged)
	{
		m_bFontsChanged = TRUE;

		// Get current metrics
		NONCLIENTMETRICS ncm;
		memset(&ncm, 0, sizeof(ncm));
		ncm.cbSize = sizeof(ncm);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

		LOGFONT lf;
		memset(&lf, 0, sizeof(lf));
		lf.lfHeight = -MulDiv(g_Options.m_nMinimalFontSize, g_Options.m_nLogPixelsY, 72);
		lf.lfWeight = FW_BOLD;
		lf.lfCharSet = g_Options.m_lfCharSet;
		LoadString(g_hInstDll, IDS_SYSTEMFONTNAME, lf.lfFaceName, ARRAYSIZE(lf.lfFaceName));


		// Captions are BOLD
		ncm.lfCaptionFont = lf;

		lf.lfWeight = FW_NORMAL;

		ncm.lfSmCaptionFont = lf; 
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

		/*int cyBorder = GetSystemMetrics(SM_CYBORDER);
		int nSize = abs(lf.lfHeight) + abs(tm.tmExternalLeading) + 2 * cyBorder;
		nSize = max(nSize, GetSystemMetrics(SM_CYICON)/2 + 2 * cyBorder);*/

		// The above calculation of metric sizes is incorrect, Morever, The other values
		// are also wrong..So using hardcoded values: Based on Display.cpl
		// BUG: Changes maybe required for 9x here!!
		if (g_Options.m_nMinimalFontSize >= 14 )
			ncm.iCaptionWidth = ncm.iCaptionHeight = 26;
		else
			ncm.iCaptionWidth = ncm.iCaptionHeight = 18;
		
		ncm.iSmCaptionWidth = 15;
		ncm.iSmCaptionHeight = 15;
		ncm.iMenuWidth = 18;
		ncm.iMenuHeight = 18;

		SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(ncm), &ncm, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		SystemParametersInfo(SPI_SETICONTITLELOGFONT, sizeof(lf), &lf, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		SendMessageTimeout(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0, SMTO_ABORTIFHUNG, 5000, &result );
		SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, (LPARAM)__TEXT("WindowMetrics"),
			SMTO_ABORTIFHUNG, 5000, &result);

		// HACK - TODO Remove this from here
		g_Options.m_schemePreview.m_PortableNonClientMetrics.LoadOriginal();
		g_Options.m_schemeCurrent.m_PortableNonClientMetrics.LoadOriginal();
	}
	else if (!bChangeFont && m_bFontsChanged)
	{
		m_bFontsChanged = FALSE;

		SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(m_ncmOrig), &m_ncmOrig, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		SystemParametersInfo(SPI_SETICONTITLELOGFONT, sizeof(m_lfIconOrig), &m_lfIconOrig, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		SendMessageTimeout(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0, SMTO_ABORTIFHUNG, 5000, &result);
		SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, (LPARAM)__TEXT("WindowMetrics"),
			SMTO_ABORTIFHUNG, 5000, &result);

		// HACK - TODO Remove this from here
		g_Options.m_schemePreview.m_PortableNonClientMetrics.LoadOriginal();
		g_Options.m_schemeCurrent.m_PortableNonClientMetrics.LoadOriginal();
	}
	
	if(bMagnifier && !m_bMagnifierRun)
	{
		// Start magnifier
		m_bMagnifierRun = TRUE;
		ShellExecute(NULL, NULL, __TEXT("Magnify.exe"), NULL, NULL, SW_SHOWNORMAL/*SW_SHOWMINIMIZED*/);
	}
	else if (!bMagnifier && m_bMagnifierRun)
	{
		// Stop magnifier
		m_bMagnifierRun = FALSE;
		TCHAR szMag[200];
		LoadString(g_hInstDll, IDS_NAMEOFMAGNIFIER, szMag, ARRAYSIZE(szMag));
		if(HWND hwnd = FindWindow(NULL, szMag))
			SendMessage(hwnd, WM_CLOSE, 0, 0);
	}
	
}


void CWelcome2Pg::SetCheckBoxesFromWelcomePageInfo()
{
	// This algorithm chooses which check boxes to set based on the
	// minimal legible font size specified in g_Options.m_nMinimalFontSize

	// HACK:
//	g_Options.m_nMinimalFontSize = MulDiv(abs(g_Options.m_schemePreview.m_ncm.lfCaptionFont.lfHeight), 72, g_Options.m_nLogPixelsY);

	BOOL bSwitchRes = FALSE;
	BOOL bChangeFonts = FALSE;
 	BOOL bUseMagnify = FALSE;
	switch(g_Options.m_nMinimalFontSize)
	{
	case 8:
	case 9:  // Required for JPN
	case 10:
	case 11: // Required For JPN
		bChangeFonts = TRUE;
		break;
	case 12:
		bChangeFonts = TRUE;
		bSwitchRes = TRUE;
		break;
	case 14:
	case 15: // Required for JPN
	case 16:
	case 18:
	case 20:
	case 22:
	case 24:
		bChangeFonts = TRUE;
		bUseMagnify = TRUE;
		break;
	}

	// JMC: TODO: Handle if the user does not have permission to change
	// the display settings!!!!!!!!!!!!!!

	if(-1 == m_nBestDisplayMode)
	{
		bSwitchRes = FALSE;
//		SetWindowText(GetDlgItem(m_hwnd, IDC_SZRESMESSAGE),
//			__TEXT("There are no display resolutions that would be better for the size text you chose."));
		EnableWindow(GetDlgItem(m_hwnd, IDC_SWITCHRESOLUTION), FALSE);
	}
	else
	{
#if 0 // We don't display special text any more
		TCHAR sz[200];
		TCHAR szTemp[1024];
		LoadString(g_hInstDll, IDS_DISPLAYRESOLUTIONINFO, szTemp, ARRAYSIZE(szTemp));
		wsprintf(sz, szTemp,
			m_dvmdOrig.dmPelsWidth,
			m_dvmdOrig.dmPelsHeight,
			m_pDisplayModes[m_nBestDisplayMode].m_DevMode.dmPelsWidth,
			m_pDisplayModes[m_nBestDisplayMode].m_DevMode.dmPelsHeight);
		SetWindowText(GetDlgItem(m_hwnd, IDC_SZRESMESSAGE), sz);
#endif
		EnableWindow(GetDlgItem(m_hwnd, IDC_SWITCHRESOLUTION), TRUE);
	}

	Button_SetCheck(GetDlgItem(m_hwnd, IDC_SWITCHRESOLUTION), bSwitchRes);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_CHANGEFONTS), bChangeFonts);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_USEMAGNIFY), bUseMagnify);
	UpdateControls();
}


LRESULT
CWelcome2Pg::OnCommand(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	LRESULT lResult = 1;

	WORD wNotifyCode = HIWORD(wParam);
	WORD wCtlID      = LOWORD(wParam);
	HWND hwndCtl     = (HWND)lParam;


	// NOTE: DO NOT CALL UpdateControls()
	// UpdateControls() should only be called when entering this page
	// since it sets the check boxes based on the minimal font size
	// determined by the previous wizard page
	
	
	switch(wCtlID)
	{
	case IDC_SWITCHRESOLUTION:
	case IDC_CHANGEFONTS:
	case IDC_USEMAGNIFY:
		UpdateControls();
		lResult = 0;
	break;
	
	  default:
	  break;
	  }
	
	return lResult;
}

LRESULT
CWelcome2Pg::OnPSN_SetActive(
							 HWND hwnd, 
							 INT idCtl, 
							 LPPSHNOTIFY pnmh
							 )
{
	// Call the base class
	WizardPage::OnPSN_SetActive(hwnd, idCtl, pnmh);

	// Make sure our check boxes reflect any change in the minimal
	// font size specified by g_Options.m_nMinimalFontSize
	if(g_Options.m_bWelcomePageTouched)
	{
		g_Options.m_bWelcomePageTouched = FALSE;
		SetCheckBoxesFromWelcomePageInfo();
	}
	
	return TRUE;
}

LRESULT
CWelcome2Pg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	BOOL bIMenu = Button_GetCheck(GetDlgItem(m_hwnd, IDC_PERMENU));
	
	// If Intelli-menus are changed
	if(bIMenu != m_IntlVal)
	{
		HKEY hKey;
		DWORD_PTR result;
		
		LPTSTR psz = bIMenu ?  TEXT("No") : TEXT("Yes");

		// Change the Registry entries....
		if ( ERROR_SUCCESS == RegOpenKey( HKEY_CURRENT_USER, REGSTR_INTELLIMENU, &hKey) )
		{
			RegSetValueEx( hKey, STRMENU, 0, REG_SZ, (LPBYTE)psz,
						(lstrlen(psz) + 1) * sizeof(TCHAR) );

			RegCloseKey(hKey);
		}

		if ( ERROR_SUCCESS == RegOpenKey( HKEY_CURRENT_USER, REGSTR_IE, &hKey) )
		{
			RegSetValueEx( hKey, FAVMENU, 0, REG_SZ, (LPBYTE)psz,
						(lstrlen(psz) + 1) * sizeof(TCHAR) );

			RegCloseKey(hKey);
		}

		m_IntlVal = bIMenu;
		SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 
			(LPARAM) 0, SMTO_ABORTIFHUNG, 5000, &result);
	}

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}

LRESULT
CWelcome2Pg::OnPSN_WizBack(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	
    // Undo any changes
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_SWITCHRESOLUTION), FALSE);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_CHANGEFONTS), FALSE);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_USEMAGNIFY), FALSE);
	
    // While going back. Just update variables only. Don't apply changes.
    // DONOT call UpdateControls(): a-anilk

    BOOL bChangeRes = Button_GetCheck(GetDlgItem(m_hwnd, IDC_SWITCHRESOLUTION));
	BOOL bChangeFont = Button_GetCheck(GetDlgItem(m_hwnd, IDC_CHANGEFONTS));
	BOOL bMagnifier = Button_GetCheck(GetDlgItem(m_hwnd, IDC_USEMAGNIFY));
    m_bFontsChanged = FALSE;

	return WizardPage::OnPSN_WizBack(hwnd, idCtl, pnmh);
}
