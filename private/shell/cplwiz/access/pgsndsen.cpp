#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgSndSen.h"

CSoundSentryShowSoundsPg::CSoundSentryShowSoundsPg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_SNDWIZSENTRYSHOWSOUNDSTITLE, IDS_SNDWIZSENTRYSHOWSOUNDSSUBTITLE)
{
	m_dwPageId = IDD_SNDWIZSENTRYSHOWSOUNDS;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CSoundSentryShowSoundsPg::~CSoundSentryShowSoundsPg(
	VOID
	)
{
}

LRESULT
CSoundSentryShowSoundsPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	HWND hwndWindowed = GetDlgItem(m_hwnd, IDC_SS_WINDOWED);
	HWND hwndText = GetDlgItem(m_hwnd, IDC_SS_TEXT);

	TCHAR szTemp[50];

	LoadString(g_hInstDll, IDS_SNDSEN_SSWF_NONE, szTemp, ARRAYSIZE(szTemp));
	ComboBox_InsertString(hwndWindowed, 0, szTemp);
	LoadString(g_hInstDll, IDS_SNDSEN_SSWF_TITLE, szTemp, ARRAYSIZE(szTemp));
	ComboBox_InsertString(hwndWindowed, 1, szTemp);
	LoadString(g_hInstDll, IDS_SNDSEN_SSWF_WINDOW, szTemp, ARRAYSIZE(szTemp));
	ComboBox_InsertString(hwndWindowed, 2, szTemp);
	LoadString(g_hInstDll, IDS_SNDSEN_SSWF_DISPLAY, szTemp, ARRAYSIZE(szTemp));
	ComboBox_InsertString(hwndWindowed, 3, szTemp);
	
	LoadString(g_hInstDll, IDS_SNDSEN_SSTF_NONE, szTemp, ARRAYSIZE(szTemp));
	ComboBox_InsertString(hwndText, 0, szTemp);
	LoadString(g_hInstDll, IDS_SNDSEN_SSTF_CHARS, szTemp, ARRAYSIZE(szTemp));
	ComboBox_InsertString(hwndText, 1, szTemp);
	LoadString(g_hInstDll, IDS_SNDSEN_SSTF_BORDER, szTemp, ARRAYSIZE(szTemp));
	ComboBox_InsertString(hwndText, 2, szTemp);
	LoadString(g_hInstDll, IDS_SNDSEN_SSTF_DISPLAY, szTemp, ARRAYSIZE(szTemp));
	ComboBox_InsertString(hwndText, 3, szTemp);


	// Sound Sentry settings
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_SS_ENABLE_SOUND), g_Options.m_schemePreview.m_SOUNDSENTRY.dwFlags & SSF_SOUNDSENTRYON);

	switch(g_Options.m_schemePreview.m_SOUNDSENTRY.iWindowsEffect)
	{
	case SSWF_NONE:
		ComboBox_SetCurSel(hwndWindowed, 0);
		break;
	case SSWF_TITLE:
		ComboBox_SetCurSel(hwndWindowed, 1);
		break;
	case SSWF_WINDOW:
		ComboBox_SetCurSel(hwndWindowed, 2);
		break;
	case SSWF_DISPLAY:
		ComboBox_SetCurSel(hwndWindowed, 3);
		break;
	case SSWF_CUSTOM:
#pragma message("What do we do if the user comes in with a custom setting")
		ComboBox_SetCurSel(hwndWindowed, 1);
		break;
	default:
		ComboBox_SetCurSel(hwndWindowed, 1);
		break;
	}

	switch(g_Options.m_schemePreview.m_SOUNDSENTRY.iFSTextEffect)
	{
	case SSTF_NONE:
		ComboBox_SetCurSel(hwndText, 0);
		break;
	case SSTF_CHARS:
		ComboBox_SetCurSel(hwndText, 1);
		break;
	case SSTF_BORDER:
		ComboBox_SetCurSel(hwndText, 2);
		break;
	case SSTF_DISPLAY:
		ComboBox_SetCurSel(hwndText, 3);
		break;
	default:
		ComboBox_SetCurSel(hwndText, 2);
		break;
	}

	// Show Sounds settings
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_SS_ENABLE_SHOW), g_Options.m_schemePreview.m_bShowSounds);



	UpdateControls();
	return 1;
}


void CSoundSentryShowSoundsPg::UpdateControls()
{
	BOOL bUseSoundSentry = Button_GetCheck(GetDlgItem(m_hwnd, IDC_SS_ENABLE_SOUND));

	EnableWindow(GetDlgItem(m_hwnd, IDC_SS_LBL1), bUseSoundSentry);
	EnableWindow(GetDlgItem(m_hwnd, IDC_SS_LBL2), bUseSoundSentry);
	EnableWindow(GetDlgItem(m_hwnd, IDC_SS_LBL3), bUseSoundSentry);

	EnableWindow(GetDlgItem(m_hwnd, IDC_SS_WINDOWED), bUseSoundSentry);
	EnableWindow(GetDlgItem(m_hwnd, IDC_SS_TEXT), bUseSoundSentry);
}


LRESULT
CSoundSentryShowSoundsPg::OnCommand(
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
	case IDC_SS_ENABLE_SOUND:
		// These commands require us to re-enable/disable the appropriate controls
		UpdateControls();
		lResult = 0;
		break;

	default:
		break;
	}

	return lResult;
}


LRESULT
CSoundSentryShowSoundsPg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{

	if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_SS_ENABLE_SOUND)))
	{
		g_Options.m_schemePreview.m_SOUNDSENTRY.dwFlags |= SSF_SOUNDSENTRYON;
		// Set options if we are turning the feature on

		HWND hwndWindowed = GetDlgItem(m_hwnd, IDC_SS_WINDOWED);
		HWND hwndText = GetDlgItem(m_hwnd, IDC_SS_TEXT);
		switch(ComboBox_GetCurSel(hwndWindowed))
		{
		case 0:
			g_Options.m_schemePreview.m_SOUNDSENTRY.iWindowsEffect = SSWF_NONE;
			break;
		case 1:
			g_Options.m_schemePreview.m_SOUNDSENTRY.iWindowsEffect = SSWF_TITLE;
			break;
		case 2:
			g_Options.m_schemePreview.m_SOUNDSENTRY.iWindowsEffect = SSWF_WINDOW;
			break;
		case 3:
			g_Options.m_schemePreview.m_SOUNDSENTRY.iWindowsEffect = SSWF_DISPLAY;
			break;
		default:
			// No Change
			break;
		}

		switch(ComboBox_GetCurSel(hwndText))
		{
		case 0:
			g_Options.m_schemePreview.m_SOUNDSENTRY.iFSTextEffect = SSTF_NONE;
			break;
		case 1:
			g_Options.m_schemePreview.m_SOUNDSENTRY.iFSTextEffect = SSTF_CHARS;
			break;
		case 2:
			g_Options.m_schemePreview.m_SOUNDSENTRY.iFSTextEffect = SSTF_BORDER;
			break;
		case 3:
			g_Options.m_schemePreview.m_SOUNDSENTRY.iFSTextEffect = SSTF_DISPLAY;
			break;
		default:
			// No Change
			break;
		}
	}
	else
		g_Options.m_schemePreview.m_SOUNDSENTRY.dwFlags &= ~SSF_SOUNDSENTRYON;


	g_Options.m_schemePreview.m_bShowSounds = Button_GetCheck(GetDlgItem(m_hwnd, IDC_SS_ENABLE_SHOW));
	
	
	g_Options.ApplyPreview();

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}
