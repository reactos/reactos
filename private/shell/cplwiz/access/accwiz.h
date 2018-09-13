#ifndef _INC_ACCWIZ_H
#define _INC_ACCWIZ_H

#include "schemes.h" // For SCHEMEDATALOCAL

// This is the maxinum number of options allowed on any one preview page
#define MAX_DISTINCT_VALUES 20
// Helper function
void LoadArrayFromStringTable(int nIdString, int *rgnValues, int *pnCountValues);

// Macros used to save debug info to/from the INI file
// JMC: HACK - Default to '1' for options!!!!!!!!
#define GET_SAVED_INT(xxx) xxx = GetPrivateProfileInt(__TEXT("Options"), __TEXT(#xxx), 1, __TEXT("AccWiz.ini"))
#define PUT_SAVED_INT(xxx) wsprintf(sz, __TEXT("%i"), xxx);WritePrivateProfileString(__TEXT("Options"), __TEXT(#xxx), sz, __TEXT("AccWiz.ini"))

// This class contains the general options for the whole wizard
class CAccWizOptions
{
public:
	CAccWizOptions()
	{
		// JMC: HACK Remove
		// Load in user defaults
		GET_SAVED_INT(m_nTypeMinText);
		GET_SAVED_INT(m_nTypeScrollBar);
		GET_SAVED_INT(m_nTypeBorder);
		GET_SAVED_INT(m_nTypeAccTimeOut);

		m_nMinimalFontSize = 8;

		///////////////////////////////////////////////
		// Calculate globals that we need
 		HDC hDC = GetDC(NULL);
		m_nLogPixelsY = GetDeviceCaps(hDC, LOGPIXELSY);
		ReleaseDC(NULL, hDC);

		///////////////////////////////////////////////
		// Load original Wiz Scheme settings
		for(int i=0;i<COLOR_MAX_97_NT5;i++)
			m_schemeOriginal.m_rgb[i] = GetSysColor(i);

		m_schemeOriginal.m_ncm.cbSize = sizeof(m_schemeOriginal.m_ncm);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(m_schemeOriginal.m_ncm), &m_schemeOriginal.m_ncm, 0);

#define LOAD_SCHEME_ORIGINAL(xxx) m_schemeOriginal.m_##xxx.cbSize = sizeof(m_schemeOriginal.m_##xxx);SystemParametersInfo(SPI_GET##xxx, sizeof(m_schemeOriginal.m_##xxx), &m_schemeOriginal.m_##xxx, 0)

		LOAD_SCHEME_ORIGINAL(FILTERKEYS);
		LOAD_SCHEME_ORIGINAL(MOUSEKEYS);
		LOAD_SCHEME_ORIGINAL(SERIALKEYS);
		LOAD_SCHEME_ORIGINAL(STICKYKEYS);
		LOAD_SCHEME_ORIGINAL(TOGGLEKEYS);
		LOAD_SCHEME_ORIGINAL(SOUNDSENTRY);
		LOAD_SCHEME_ORIGINAL(ACCESSTIMEOUT);
		LOAD_SCHEME_ORIGINAL(HIGHCONTRAST);

		m_schemeOriginal.m_bShowSounds = GetSystemMetrics(SM_SHOWSOUNDS);
		SystemParametersInfo(SPI_GETKEYBOARDPREF, 0, &m_schemeOriginal.m_bShowExtraKeyboardHelp, 0);

#pragma message("See if sound sentry works when a DLL is specified and we revert to original")
#pragma message("See if high contrast works when a default scheme is specified and we revert to original")
		

		m_schemeOriginal.m_bSwapMouseButtons = GetSystemMetrics(SM_SWAPBUTTON);

//		m_schemeOriginal.m_nIconSize = SystemParametersInfo

		// JMC: TODO: Clear out any read only bits so we don't set them (ie: see STICKYKEYS)




		// Copy to the Preview scheme and to the current scheme
		m_schemePreview = m_schemeOriginal;
		m_schemeCurrent = m_schemeOriginal;

		// JMC: TODO: Fill this in - m_schemeWindowsDefault;
#pragma message("Fill in the Windows default scheme")


	}
	~CAccWizOptions()
	{
		// JMC: HACK Remove
		// Save user options
		TCHAR sz[20];
		PUT_SAVED_INT(m_nTypeMinText);
		PUT_SAVED_INT(m_nTypeScrollBar);
		PUT_SAVED_INT(m_nTypeBorder);
		PUT_SAVED_INT(m_nTypeAccTimeOut);

	}

	void RestoreOriginalColorsToPreview()
	{
		memcpy(m_schemePreview.m_rgb, m_schemeOriginal.m_rgb, sizeof(m_schemePreview.m_rgb));
	}

	void ApplyPreview()
	{
		Apply(m_schemePreview);
	}
	void ApplyOriginal()
	{
		Apply(m_schemeOriginal);
	}
	void ApplyWindowsDefault()
	{
		Apply(m_schemeWindowsDefault);
	}

	int m_nLogPixelsY;

	int m_nMinimalFontSize;

	WIZSCHEME m_schemePreview;


	// For handling different types of pages
	int m_nTypeMinText;
	int m_nTypeScrollBar;
	int m_nTypeBorder;
	int m_nTypeAccTimeOut;

protected:
	void Apply(const WIZSCHEME &schemeNew)
	{
		// This applies the settings in the schemeNew scheme

#define APPLY_SCHEME_CURRENT(xxx) if(0 != memcmp(&schemeNew.m_##xxx, &m_schemeCurrent.m_##xxx, sizeof(schemeNew.m_##xxx))) SystemParametersInfo(SPI_SET##xxx, sizeof(schemeNew.m_##xxx), (PVOID)&schemeNew.m_##xxx, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)

		APPLY_SCHEME_CURRENT(FILTERKEYS);
		APPLY_SCHEME_CURRENT(MOUSEKEYS);
		APPLY_SCHEME_CURRENT(SERIALKEYS);
		APPLY_SCHEME_CURRENT(STICKYKEYS);
		APPLY_SCHEME_CURRENT(TOGGLEKEYS);
		APPLY_SCHEME_CURRENT(SOUNDSENTRY);
		APPLY_SCHEME_CURRENT(ACCESSTIMEOUT);
		APPLY_SCHEME_CURRENT(HIGHCONTRAST);

		// Check Show Sounds
		if(schemeNew.m_bShowSounds != m_schemeCurrent.m_bShowSounds)
			SystemParametersInfo(SPI_SETSHOWSOUNDS, schemeNew.m_bShowSounds, NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

		// Check Extra keyboard help
		if(schemeNew.m_bShowExtraKeyboardHelp != m_schemeCurrent.m_bShowExtraKeyboardHelp)
			SystemParametersInfo(SPI_SETKEYBOARDPREF, schemeNew.m_bShowExtraKeyboardHelp, NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

		// Check swap mouse buttons
		if(schemeNew.m_bSwapMouseButtons != m_schemeCurrent.m_bSwapMouseButtons)
			SwapMouseButton(schemeNew.m_bSwapMouseButtons);


		// Check for change in colors
		if(0 != memcmp(schemeNew.m_rgb, m_schemeCurrent.m_rgb, sizeof(m_schemeCurrent.m_rgb)))
		{
			int rgInts[COLOR_MAX_97_NT5];
			for(int i=0;i<COLOR_MAX_97_NT5;i++)
				rgInts[i] = i;

#pragma message("HACK  FOR WINDOWS 95 TEST BUILDS!!!!!!!!!!")
			SetSysColors(COLOR_MAX_97_NT5/*COLOR_MAX_95_NT4*/, rgInts, schemeNew.m_rgb);
		}

		// Check non client metrics
		if(0 != memcmp(&schemeNew.m_ncm, &m_schemeCurrent.m_ncm, sizeof(schemeNew.m_ncm)))
		{
			SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(schemeNew.m_ncm), (PVOID)&schemeNew.m_ncm, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		}

		
		
		m_schemeCurrent = schemeNew;
	}

	// Dialogs should not modify these copies of the scheme
	WIZSCHEME m_schemeOriginal;
	WIZSCHEME m_schemeCurrent;
	WIZSCHEME m_schemeWindowsDefault;
};

// This variable will be accessable to any derived wizard page.
// It contains information specific to this application
extern CAccWizOptions g_Options;


VOID WINAPI AccWiz_RunDllA(HWND hwnd, HINSTANCE hInstance, LPSTR pszCmdLine, INT nCmdShow);
VOID WINAPI AccWiz_RunDllW(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLine, INT nCmdShow);

#endif // _INC_ACCWIZ_H
