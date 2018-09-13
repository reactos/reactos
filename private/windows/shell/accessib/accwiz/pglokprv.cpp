#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgLokPrv.h"

#define HC_KEY                __TEXT("Control Panel\\Accessibility\\HighContrast")
#define HIGHCONTRAST_SCHEME   __TEXT("High Contrast Scheme")
#define VOLATILE_SCHEME       __TEXT("Volital HC Scheme")


CLookPreviewPg::CLookPreviewPg(
							   LPPROPSHEETPAGE ppsp,
							   int dwPageId,
							   int nIdTitle,
							   int nIdSubTitle,
							   int nIdValueString
							   ) : WizardPage(ppsp, nIdTitle, nIdSubTitle)
{
	m_dwPageId = dwPageId;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);

	m_nIdValueString = nIdValueString;

	m_nCountValues = 0;
	m_nCurValueIndex = 0;

	m_hwndSlider = 0;
	m_hwndListBox = 0;
}



CLookPreviewPg::~CLookPreviewPg(
	VOID
	)
{
}

#include "LookPrev.h"

void PASCAL Look_RebuildSysStuff(BOOL fInit);
void SetMyNonClientMetrics(LPNONCLIENTMETRICS lpncm);

LRESULT
CLookPreviewPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	SendMessage(GetDlgItem(m_hwnd, IDC_LOOKPREV), LPM_RECALC, 0, 0);
	SendMessage(GetDlgItem(m_hwnd, IDC_LOOKPREV), LPM_REPAINT, 0, 0);


	LoadValueArray();

	// Figure out what UI Type we have
	// The dialog template MUST have either
	// IDC_PREVLIST or (IDC_BTNUP,IDC_BTNDOWN) or IDC_PREVSLIDER


	// JMC: The only type now used in the wizard is the UILISTBOX
#ifdef USE_SLIDER_OR_BUTTONS
	if(GetDlgItem(m_hwnd, IDC_BTNUP))
		m_UIType = UIBUTTONS;
	else if(m_hwndListBox = GetDlgItem(m_hwnd, IDC_PREVLIST))
		m_UIType = UILISTBOX;
	else if(m_hwndSlider = GetDlgItem(m_hwnd, IDC_PREVSLIDER))
		m_UIType = UISLIDER;
	else
		_ASSERTE(FALSE); // Dialog template is not valid
#else
	m_hwndListBox = GetDlgItem(m_hwnd, IDC_PREVLIST);
	m_UIType = UILISTBOX;
#endif


	// Get the initial value for the type of preview we are doing
	int nCurrentSize = GetCurrentValue(NULL/*&g_Options.m_schemePreview.m_ncm*/);
	// Find next larger
	for(int i=m_nCountValues - 1;i>=0;i--)
	{
		if(m_rgnValues[i] >= nCurrentSize)
			m_nCurValueIndex = i;
		else break;
	}


	// Initialize the controls for the loaded values
	switch(m_UIType)
	{
	case UIBUTTONS:
		break; // Nothing to do
	case UILISTBOX:
		for(i=0;i<m_nCountValues;i++)
		{
			TCHAR sz[100];
			GetValueItemText(i, sz, ARRAYSIZE(sz));
			ListBox_AddString(m_hwndListBox, sz);
		}
		ListBox_SetCurSel(m_hwndListBox, m_nCurValueIndex);
		break;
	case UISLIDER:
		SendMessage(m_hwndSlider, TBM_SETRANGE, TRUE, MAKELONG(0, m_nCountValues - 1));
		// JMC: NOTE - We have to reverse the range for vertical sliders
		SendMessage(m_hwndSlider, TBM_SETPOS, TRUE, m_nCountValues - 1 - m_nCurValueIndex);
		break;
	}

	UpdateControls();
	return 1;
}


void CLookPreviewPg::LoadValueArray()
{
	LoadArrayFromStringTable(m_nIdValueString, m_rgnValues, &m_nCountValues);
}


void CLookPreviewPg::UpdateControls()
{
}

LRESULT
CLookPreviewPg::OnCommand(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	LRESULT lResult = 1;

	WORD wNotifyCode = HIWORD(wParam);
	WORD wCtlID      = LOWORD(wParam);
	HWND hwndCtl     = (HWND)lParam;

	switch(wCtlID)
	{
	case IDC_PREVLIST:
		switch(wNotifyCode)
		{
		case LBN_SELCHANGE:
			UpdatePreview(wCtlID);
			break;
		default:
			break;
		}
#ifdef USE_SLIDER_OR_BUTTONS
	case IDC_BTNUP:
	case IDC_BTNDOWN:
		UpdatePreview(wCtlID);
		break;
#endif
	default:
		break;
	}

	return lResult;
}

void CLookPreviewPg::ResetColors()
{
	if(0 != m_nCurValueIndex)
	{
		int nCount = GetScheme(m_nCurValueIndex-1).nColorsUsed;
		for(int i=0;i<nCount;i++)
			g_Options.m_schemePreview.m_rgb[i] = GetScheme(m_nCurValueIndex-1).rgb[i];

		if(nCount < COLOR_MAX_97_NT5)
		{
			// JMC: HACK: WHAT ABOUT COLOR_3DALTFACE????????

			g_Options.m_schemePreview.m_rgb[COLOR_HOTLIGHT] = g_Options.m_schemePreview.m_rgb[COLOR_ACTIVECAPTION];
			g_Options.m_schemePreview.m_rgb[COLOR_GRADIENTACTIVECAPTION] = g_Options.m_schemePreview.m_rgb[COLOR_ACTIVECAPTION];
			g_Options.m_schemePreview.m_rgb[COLOR_GRADIENTINACTIVECAPTION] = g_Options.m_schemePreview.m_rgb[COLOR_INACTIVECAPTION];
		}

	}
	else
	{
		// Use scheme that we came in here with
		g_Options.RestoreOriginalColorsToPreview();
	}


	Look_RebuildSysStuff(TRUE);
	SendMessage(GetDlgItem(m_hwnd, IDC_LOOKPREV), LPM_REPAINT, 0, 0);
}

LRESULT
CLookPreviewPg::HandleMsg(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
	)
{
	switch (uMsg)
	{
		HANDLE_MSG(hwnd, WM_HSCROLL,    OnVHScroll);
		HANDLE_MSG(hwnd, WM_VSCROLL, OnVHScroll);
	}
	return 0;
}

void CLookPreviewPg::OnVHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
#ifdef USE_SLIDER_OR_BUTTONS
	UpdatePreview(IDC_PREVSLIDER);
#endif
}

void CLookPreviewPg::UpdatePreview(int nActionCtl)
{
	// Calculate new index based on the control that generated this event
	switch(nActionCtl)
	{
	case IDC_PREVLIST:
		m_nCurValueIndex = ListBox_GetCurSel(m_hwndListBox);
		break;
#ifdef USE_SLIDER_OR_BUTTONS
	case IDC_PREVSLIDER:
		m_nCurValueIndex = m_nCountValues - 1 - SendMessage(m_hwndSlider, TBM_GETPOS, 0, 0);
		break;
	case IDC_BTNUP:
		m_nCurValueIndex = min(m_nCurValueIndex+1, m_nCountValues - 1);
		break;
	case IDC_BTNDOWN:
		m_nCurValueIndex = max(m_nCurValueIndex-1, 0);
		break;
#endif
	default:
		break;
	}

	// Make sure we are withing range
	m_nCurValueIndex = max(m_nCurValueIndex, 0);
	m_nCurValueIndex = min(m_nCurValueIndex, m_nCountValues - 1);

	// Modify the ncm structure

	// JMC: The wizard currently does not muck with m_ncm any more - we only change the colors
	// Therefore, we can call ModifyMyNonClientMetrics() with a dummy variable
	NONCLIENTMETRICS ncmDummy;
	ModifyMyNonClientMetrics(ncmDummy/*g_Options.m_schemePreview.m_ncm*/);

	// JMC: The wizard currently does not muck with m_ncm any more - we only change the colors.
	// Therefore, we don't have to call SetMyNonClientMetrics();
//	SetMyNonClientMetrics(&g_Options.m_schemePreview.m_ncm);


	Look_RebuildSysStuff(TRUE);
	SendMessage(GetDlgItem(m_hwnd, IDC_LOOKPREV), LPM_RECALC, 0, 0);
	SendMessage(GetDlgItem(m_hwnd, IDC_LOOKPREV), LPM_REPAINT, 0, 0);
}

LRESULT
CLookPreviewPg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	g_Options.ApplyPreview();
    
    HIGHCONTRAST l_hc;
    HKEY hkey, hkSchemes;
    DWORD dwDisposition;
   
    l_hc.cbSize = sizeof(l_hc);

    TCHAR szScheme[100];

    // Get the current HighContrast settings. This is done in the begining
    // and written to a HighContrast structure. m_HIGHCONTRAST
    /*SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(l_hc), 
        &l_hc, SPIF_UPDATEINIFILE);

    // based on the selection, Set or clear flag
    // Consider the case of Current selection being HighContrast. 
    // Check if originally the flag was SET, And the 
    // selection m_nCurValueIndex is 0. That means retain the 
    // Original options. 
    if (m_nCurValueIndex == 0)
    {
       // Check original options.
       l_hc.dwFlags = g_Options.GetOrigHCFlag();

       // Get the current scheme name and update
       GetSchemeName(m_nCurValueIndex, szScheme, ARRAYSIZE(szScheme));
        
       // This is the correct scheme name.
       lstrcpy(l_hc.lpszDefaultScheme, szScheme);
    }
    else 
    {
        // If it is any of Hight contrast modes. Flag is ON
        // Otherwise OFF
        if ( (m_nCurValueIndex > 0) && (m_nCurValueIndex <= 4) )
            l_hc.dwFlags |= HCF_HIGHCONTRASTON;

        else
            l_hc.dwFlags &= (~HCF_HIGHCONTRASTON);

        GetValueItemText(m_nCurValueIndex, szScheme, ARRAYSIZE(szScheme));

        // This is the correct scheme name.
        lstrcpy(l_hc.lpszDefaultScheme, szScheme);
    }
    
        l_hc.dwFlags |= 0x10000000;

    // In case the selected scheme was a high contrast scheme
    // call SystemParametersInfo with SPI_SETHCFLAG/ SPI_UNUSED39
    SystemParametersInfo(SPI_SETHIGHCONTRAST, sizeof(l_hc), 
        &l_hc, SPIF_UPDATEINIFILE);

    // Here write everything into the registry
      if (ERROR_SUCCESS == RegCreateKeyEx(
         HKEY_CURRENT_USER, 
         HC_KEY,
         0,
         __TEXT(""),
         REG_OPTION_NON_VOLATILE,
         KEY_SET_VALUE,
         NULL,
         &hkey,
         &dwDisposition))
      {  
         RegSetValueEx(hkey, HIGHCONTRAST_SCHEME, 0, REG_SZ, (PBYTE) szScheme,
            (lstrlen(szScheme) + 1) * sizeof(TCHAR));
         RegSetValueEx(hkey, VOLATILE_SCHEME, 0, REG_SZ, (PBYTE) szScheme,
            (lstrlen(szScheme) + 1) * sizeof(TCHAR));
         
         RegCloseKey(hkey);
         hkey = NULL;
      }
        */
    // The new scheme name is GetScheme(m_nCurValueIndex-1).nNameStringId
	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}


/*
APPLY
{
	int rgInts[COLOR_MAX_97_NT5];
	for(int i=0;i<COLOR_MAX_97_NT5;i++)
		rgInts[i] = i;
	SetSysColors(COLOR_MAX_97_NT5, rgInts, g_Options.m_schemePreview.m_rgb);
}
*/
