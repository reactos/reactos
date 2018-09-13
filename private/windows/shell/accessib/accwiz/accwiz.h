#ifndef _INC_ACCWIZ_H
#define _INC_ACCWIZ_H

#include "schemes.h" // For SCHEMEDATALOCAL
#include "resource.h"

// Helper function
void LoadArrayFromStringTable(int nIdString, int *rgnValues, int *pnCountValues);

// Macros used to save debug info to/from the INI file
// JMC: HACK - Default to '1' for options!!!!!!!!
#define GET_SAVED_INT(xxx) xxx = GetPrivateProfileInt(__TEXT("Options"), __TEXT(#xxx), 1, __TEXT("AccWiz.ini"))
#define PUT_SAVED_INT(xxx) wsprintf(sz, __TEXT("%i"), xxx);WritePrivateProfileString(__TEXT("Options"), __TEXT(#xxx), sz, __TEXT("AccWiz.ini"))
#define HC_KEY                __TEXT("Control Panel\\Accessibility\\HighContrast")
#define HIGHCONTRAST_SCHEME   __TEXT("High Contrast Scheme")
#define VOLATILE_SCHEME       __TEXT("Volital HC Scheme")

// TCHAR g_winScheme[] = TEXT("Windows Standard");

// This class contains the general options for the whole wizard
class CAccWizOptions
{
public:
	CAccWizOptions()
	{
		OSVERSIONINFO osvi;
		ZeroMemory(&osvi, sizeof(osvi));
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		GetVersionEx(&osvi);
		m_bWin95 = (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);

		m_nMinimalFontSize = -1; // This will be set by the welcome page

		///////////////////////////////////////////////
		// Calculate globals that we need
 		HDC hDC = GetDC(NULL);
		m_nLogPixelsY = GetDeviceCaps(hDC, LOGPIXELSY);
		ReleaseDC(NULL, hDC);



		///////////////////////////////////////////////
		// Get the default char set for fonts
		TCHAR szCharSet[20];
		if(LoadString(g_hInstDll,IDS_FONTCHARSET, szCharSet,sizeof(szCharSet)/sizeof(TCHAR))) {
			m_lfCharSet = (BYTE)_tcstoul(szCharSet,NULL,10);
		} else {
			m_lfCharSet = 0; // Default
		}

		///////////////////////////////////////////////
		// Get the standard MS Sans Serif fonts
		// JMC: HACK - Free these resources
		int rgnStandardMSSansSerifFontSizes[] = {8, 10, 12, 14, 18, 24};
		LOGFONT lf;
		ZeroMemory(&lf, sizeof(lf));
		lf.lfCharSet = m_lfCharSet;
		LoadString(g_hInstDll, IDS_SYSTEMFONTNAME, lf.lfFaceName, ARRAYSIZE(lf.lfFaceName));
		

		for(int i=0;i<6;i++)
		{
			lf.lfHeight = 0 - (int)((float)m_nLogPixelsY * (float)rgnStandardMSSansSerifFontSizes[i]/ (float)72 + (float).5);
			m_rgnStdMSSansSerifFonts[i] = CreateFontIndirect(&lf);

			// Create underlined version
			lf.lfUnderline = 1;
			m_rgnStdMSSansSerifFonts[i + 6] = CreateFontIndirect(&lf);
			lf.lfUnderline = 0;

		}
		
		// Store away original non-client metrics
		// Get original metrics
		GetNonClientMetrics(&m_ncmOrig, &m_lfIconOrig);

		// Load original Wiz Scheme settings
		m_schemeOriginal.LoadOriginal();

		// Copy to the Preview scheme and to the current scheme
		m_schemePreview = m_schemeOriginal;
		m_schemeCurrent = m_schemeOriginal;

		// This is set by the welcome page, so that the second part knows to update it's check boxes.
		// The second page clears this flag
		m_bWelcomePageTouched = FALSE;

		// JMC: TODO: Fill this in - m_schemeWindowsDefault;
		m_schemeWindowsDefault.SetToWindowsDefault();
#ifdef _DEBUG
		m_schemeOriginal.Dump();
#endif
	}
	~CAccWizOptions()
	{
	}

	void RestoreOriginalColorsToPreview()
	{
		memcpy(m_schemePreview.m_rgb, m_schemeOriginal.m_rgb, sizeof(m_schemePreview.m_rgb));
	}

	void ApplyPreview()
	{
		m_schemeCurrent.ApplyChanges(m_schemePreview);
	}
    void ApplyOriginal()
    {
        
        m_schemeCurrent.ApplyChanges(m_schemeOriginal, &m_ncmOrig, &m_lfIconOrig);

#if 0
        HKEY hkey;
        DWORD dwDisposition;
        DWORD len;
        m_schemeCurrent.m_HIGHCONTRAST.dwFlags |= 0x10000000;

        SystemParametersInfo(SPI_SETHIGHCONTRAST, sizeof(m_schemeCurrent.m_HIGHCONTRAST), 
        &m_schemeCurrent.m_HIGHCONTRAST, SPIF_UPDATEINIFILE);
        
        // Also update the existing High Contrast registry settings
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
            len = (lstrlen(m_schemeCurrent.m_HIGHCONTRAST.lpszDefaultScheme) + 1) * sizeof(TCHAR);

             RegSetValueEx(hkey, HIGHCONTRAST_SCHEME, 0, REG_SZ, 
                 (PBYTE) m_schemeCurrent.m_HIGHCONTRAST.lpszDefaultScheme, len);
             RegSetValueEx(hkey, VOLATILE_SCHEME, 0, REG_SZ, 
                 (PBYTE) m_schemeCurrent.m_HIGHCONTRAST.lpszDefaultScheme, len);
             
             RegCloseKey(hkey);
             hkey = NULL;
        }
#endif

	}
	
    void ApplyWindowsDefault();

	BOOL m_bWelcomePageTouched;

	int m_nLogPixelsY;

	int m_nMinimalFontSize;

	HFONT GetClosestMSSansSerif(int nPointSize, BOOL bUnderlined = FALSE)
	{
		// For Underlined fonts, add '6' the the index
		int nOffset = bUnderlined?6:0;

		if(nPointSize <= 8)
			return m_rgnStdMSSansSerifFonts[0 + nOffset];
		else if(nPointSize <= 10)
			return m_rgnStdMSSansSerifFonts[1 + nOffset];
		else if(nPointSize <= 12)
			return m_rgnStdMSSansSerifFonts[2 + nOffset];
		else if(nPointSize <= 14)
			return m_rgnStdMSSansSerifFonts[3 + nOffset];
		else if(nPointSize <= 18)
			return m_rgnStdMSSansSerifFonts[4 + nOffset];
		return m_rgnStdMSSansSerifFonts[5];
	}

	void ReportChanges(HWND hwndChanges)
	{
		m_schemeCurrent.ReportChanges(m_schemeOriginal, hwndChanges);
	}
    
    // New method to check the original settings flag
    DWORD GetOrigHCFlag()
    {
#if 0
        return m_schemeOriginal.m_HIGHCONTRAST.dwFlags;
#endif
		return 1;
    }

	WIZSCHEME m_schemePreview;
	WIZSCHEME m_schemeOriginal;

	BOOL m_bWin95;
	BYTE m_lfCharSet;

protected:
	NONCLIENTMETRICS m_ncmOrig;
	LOGFONT m_lfIconOrig;

	HFONT m_rgnStdMSSansSerifFonts[6 * 2]; // 0-5 are for 8, 10, 12, 14, 18, 24.  6-11 are for the same things, but underlined

	// Dialogs should not modify these copies of the scheme
	WIZSCHEME m_schemeCurrent;
	WIZSCHEME m_schemeWindowsDefault;

	friend class CWelcome2Pg; // TODO: HACK - This is only here to give CWelcome2Pg access to m_schemeCurrent
};

// This variable will be accessible to any derived wizard page.
// It contains information specific to this application
extern CAccWizOptions g_Options;


VOID WINAPI AccWiz_RunDllA(HWND hwnd, HINSTANCE hInstance, LPSTR pszCmdLine, INT nCmdShow);
VOID WINAPI AccWiz_RunDllW(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLine, INT nCmdShow);

#endif // _INC_ACCWIZ_H
