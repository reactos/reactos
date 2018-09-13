
#ifndef _INC_SCHEMES_H
#define _INC_SCHEMES_H

// HACK - THESE VALUES ARE HARD CODED
#define COLOR_MAX_95_NT4		25
#define COLOR_MAX_97_NT5		29

#include "resource.h"

#include "CurSchme.h"

// Specify all external variables
extern PTSTR s_pszColorNames[]; // JMC: HACK
extern TCHAR g_szColors[]; // = TEXT("colors");           // colors section name

// Location of the Colors subkey in Registry; Defined in RegStr.h
extern TCHAR szRegStr_Colors[]; // = REGSTR_PATH_COLORS;

extern TCHAR g_winScheme[];

// Scheme data used locally by this app - NOTE: This structure
// does NOT use or need the A and W forms for its members.  The other schemedata's
// MUST use the A and W forms since that's how they are stored in the registry
typedef struct {
	int nNameStringId;
	int nColorsUsed;
    COLORREF rgb[COLOR_MAX_97_NT5];
} SCHEMEDATALOCAL;


struct PORTABLE_NONCLIENTMETRICS
{
	// Non-ClientMetric storage area
	int m_iBorderWidth;
	int m_iScrollWidth;
	int m_iScrollHeight;
	int m_iCaptionWidth;
	int m_iCaptionHeight;
	int m_lfCaptionFont_lfHeight;
	int m_lfCaptionFont_lfWeight;
	int m_iSmCaptionWidth;
	int m_iSmCaptionHeight;
	int m_lfSmCaptionFont_lfHeight;
	int m_lfSmCaptionFont_lfWeight;
	int m_iMenuWidth;
	int m_iMenuHeight;
	int m_lfMenuFont_lfHeight;
	int m_lfMenuFont_lfWeight;
	int m_lfStatusFont_lfHeight;
	int m_lfStatusFont_lfWeight;
	int m_lfMessageFont_lfHeight;
	int m_lfMessageFont_lfWeight;
	int m_lfIconWindowsDefault_lfHeight;
	int m_lfIconWindowsDefault_lfWeight;

	int m_nFontFaces; // 0 = NoChanges, 1 = Use WindowsDefault font face

	void SetToWindowsDefault()
	{
		m_nFontFaces = 1;

		int rgnValues[MAX_DISTINCT_VALUES];
		int nCountValues;
		LoadArrayFromStringTable(IDS_WINDOWSDEFAULTSIZES, rgnValues, &nCountValues);
		_ASSERTE(21 == nCountValues);
		if(21 != nCountValues)
		{
			// Below is the hard-coded defaults for the window metrics
			m_iBorderWidth = 1;
			m_iScrollWidth = 16;
			m_iScrollHeight = 16;
			m_iCaptionWidth = 18;
			m_iCaptionHeight = 18;
			m_lfCaptionFont_lfHeight = -11;
			m_lfCaptionFont_lfWeight = 700;
			m_iSmCaptionWidth = 15;
			m_iSmCaptionHeight = 15;
			m_lfSmCaptionFont_lfHeight = -11;
			m_lfSmCaptionFont_lfWeight = 700;
			m_iMenuWidth = 18;
			m_iMenuHeight = 18;
			m_lfMenuFont_lfHeight = -11;
			m_lfMenuFont_lfWeight = 400;
			m_lfStatusFont_lfHeight = -11;
			m_lfStatusFont_lfWeight = 400;
			m_lfMessageFont_lfHeight = -11;
			m_lfMessageFont_lfWeight = 400;
			m_lfIconWindowsDefault_lfHeight = -11;
			m_lfIconWindowsDefault_lfWeight = 400;
		}
		else
		{
			m_iBorderWidth = rgnValues[0];
			m_iScrollWidth = rgnValues[1];
			m_iScrollHeight = rgnValues[2];
			m_iCaptionWidth = rgnValues[3];
			m_iCaptionHeight = rgnValues[4];
			m_lfCaptionFont_lfHeight = rgnValues[5];
			m_lfCaptionFont_lfWeight = rgnValues[6];
			m_iSmCaptionWidth = rgnValues[7];
			m_iSmCaptionHeight = rgnValues[8];
			m_lfSmCaptionFont_lfHeight = rgnValues[9];
			m_lfSmCaptionFont_lfWeight = rgnValues[10];
			m_iMenuWidth = rgnValues[11];
			m_iMenuHeight = rgnValues[12];
			m_lfMenuFont_lfHeight = rgnValues[13];
			m_lfMenuFont_lfWeight = rgnValues[14];
			m_lfStatusFont_lfHeight = rgnValues[15];
			m_lfStatusFont_lfWeight = rgnValues[16];
			m_lfMessageFont_lfHeight = rgnValues[17];
			m_lfMessageFont_lfWeight = rgnValues[18];
			m_lfIconWindowsDefault_lfHeight = rgnValues[19];
			m_lfIconWindowsDefault_lfWeight = rgnValues[20];
		}
	}

	void LoadOriginal()
	{
		NONCLIENTMETRICS ncmTemp;
		LOGFONT lfIcon;
		GetNonClientMetrics(&ncmTemp, &lfIcon);

		m_iBorderWidth = ncmTemp.iBorderWidth;
		m_iScrollWidth = ncmTemp.iScrollWidth;
		m_iScrollHeight = ncmTemp.iScrollHeight;
		m_iCaptionWidth = ncmTemp.iCaptionWidth;
		m_iCaptionHeight = ncmTemp.iCaptionHeight;
		m_lfCaptionFont_lfHeight = ncmTemp.lfCaptionFont.lfHeight;
		m_lfCaptionFont_lfWeight = ncmTemp.lfCaptionFont.lfWeight;
		m_iSmCaptionWidth = ncmTemp.iSmCaptionWidth;
		m_iSmCaptionHeight = ncmTemp.iSmCaptionHeight;
		m_lfSmCaptionFont_lfHeight = ncmTemp.lfSmCaptionFont.lfHeight;
		m_lfSmCaptionFont_lfWeight = ncmTemp.lfSmCaptionFont.lfWeight;
		m_iMenuWidth = ncmTemp.iMenuWidth;
		m_iMenuHeight = ncmTemp.iMenuHeight;
		m_lfMenuFont_lfHeight = ncmTemp.lfMenuFont.lfHeight;
		m_lfMenuFont_lfWeight = ncmTemp.lfMenuFont.lfWeight;
		m_lfStatusFont_lfHeight = ncmTemp.lfStatusFont.lfHeight;
		m_lfStatusFont_lfWeight = ncmTemp.lfStatusFont.lfWeight;
		m_lfMessageFont_lfHeight = ncmTemp.lfMessageFont.lfHeight;
		m_lfMessageFont_lfWeight = ncmTemp.lfMessageFont.lfWeight;
		m_lfIconWindowsDefault_lfHeight = lfIcon.lfHeight;
		m_lfIconWindowsDefault_lfWeight = lfIcon.lfWeight;

		m_nFontFaces = 0;
	}

	void ApplyChanges() const;

};

struct WIZSCHEME
{
	WIZSCHEME()
	{
		ZeroMemory(this, sizeof(*this));
		m_cbSize = sizeof(*this);
		m_dwVersion = 0x000000FF;
	}
	DWORD m_cbSize;
	DWORD m_dwVersion;

	COLORREF m_rgb[COLOR_MAX_97_NT5];

	FILTERKEYS m_FILTERKEYS;
	MOUSEKEYS m_MOUSEKEYS;
	STICKYKEYS m_STICKYKEYS;
	TOGGLEKEYS m_TOGGLEKEYS;
	SOUNDSENTRY m_SOUNDSENTRY;
	ACCESSTIMEOUT m_ACCESSTIMEOUT;
//	HIGHCONTRAST m_HIGHCONTRAST;
//	SERIALKEYS m_SERIALKEYS;

	BOOL m_bShowSounds;
	BOOL m_bShowExtraKeyboardHelp;
	BOOL m_bSwapMouseButtons;
	int m_nMouseTrails;
	int m_nMouseSpeed;
	int m_nIconSize;
	int m_nCursorScheme;
	// int m_nScrollWidth;
	// int m_nBorderWidth;

	PORTABLE_NONCLIENTMETRICS m_PortableNonClientMetrics;


#ifdef _DEBUG
	void Dump()
	{
		FILE *pStream = fopen( "c:\\txt.acw", "w" );
		if(pStream)
		{
			for(int i=0;i<COLOR_MAX_97_NT5;i++)
				fprintf(pStream, "m_rgb[%2i] = RGB(%3i,%3i,%3i);\r\n", i, GetRValue(m_rgb[i]), GetGValue(m_rgb[i]), GetBValue(m_rgb[i]));
#define TEMP_MAC(xxx, yyy) fprintf(pStream, "m_" #xxx "." #yyy " = %i;\r\n", m_##xxx.yyy)
			TEMP_MAC(FILTERKEYS, cbSize);
			TEMP_MAC(FILTERKEYS, dwFlags);
			TEMP_MAC(FILTERKEYS, iWaitMSec);
			TEMP_MAC(FILTERKEYS, iDelayMSec);
			TEMP_MAC(FILTERKEYS, iRepeatMSec);
			TEMP_MAC(FILTERKEYS, iBounceMSec);

			TEMP_MAC(MOUSEKEYS, cbSize);
			TEMP_MAC(MOUSEKEYS, dwFlags);
			TEMP_MAC(MOUSEKEYS, iMaxSpeed);
			TEMP_MAC(MOUSEKEYS, iTimeToMaxSpeed);
			TEMP_MAC(MOUSEKEYS, iCtrlSpeed);
			TEMP_MAC(MOUSEKEYS, dwReserved1);
			TEMP_MAC(MOUSEKEYS, dwReserved2);

			TEMP_MAC(STICKYKEYS, cbSize);
			TEMP_MAC(STICKYKEYS, dwFlags);

			TEMP_MAC(TOGGLEKEYS, cbSize);
			TEMP_MAC(TOGGLEKEYS, dwFlags);

			TEMP_MAC(SOUNDSENTRY, cbSize);
			TEMP_MAC(SOUNDSENTRY, dwFlags);
			TEMP_MAC(SOUNDSENTRY, iFSTextEffect);
			TEMP_MAC(SOUNDSENTRY, iFSTextEffectMSec);
			TEMP_MAC(SOUNDSENTRY, iFSTextEffectColorBits);
			TEMP_MAC(SOUNDSENTRY, iFSGrafEffect);
			TEMP_MAC(SOUNDSENTRY, iFSGrafEffectMSec);
			TEMP_MAC(SOUNDSENTRY, iFSGrafEffectColor);
			TEMP_MAC(SOUNDSENTRY, iWindowsEffect);
			TEMP_MAC(SOUNDSENTRY, iWindowsEffectMSec);
			TEMP_MAC(SOUNDSENTRY, lpszWindowsEffectDLL);
			TEMP_MAC(SOUNDSENTRY, iWindowsEffectOrdinal);

			TEMP_MAC(ACCESSTIMEOUT, cbSize);
			TEMP_MAC(ACCESSTIMEOUT, dwFlags);
			TEMP_MAC(ACCESSTIMEOUT, iTimeOutMSec);

#define TEMP_MAC2(xxx) fprintf(pStream, #xxx " = %i;\r\n", xxx)
			TEMP_MAC2(m_bShowSounds);
			TEMP_MAC2(m_bShowExtraKeyboardHelp);
			TEMP_MAC2(m_bSwapMouseButtons);
			TEMP_MAC2(m_nMouseTrails);
			TEMP_MAC2(m_nMouseSpeed);
			TEMP_MAC2(m_nIconSize);
			TEMP_MAC2(m_nCursorScheme);

			NONCLIENTMETRICS ncm;
			LOGFONT lf;
			GetNonClientMetrics(&ncm, &lf);

#define TEMP_MAC3(xxx) fprintf(pStream, "m_ncmWindowsDefault." #xxx " = %i;\n", ncm.xxx)
#define TEMP_MAC4(xxx) fprintf(pStream, "m_ncmWindowsDefault." #xxx ".lfHeight = %i;\nm_ncmWindowsDefault." #xxx ".lfWeight = %i;\n", ncm.xxx.lfHeight, ncm.xxx.lfWeight)
			TEMP_MAC3(cbSize);
			TEMP_MAC3(iBorderWidth);
			TEMP_MAC3(iScrollWidth);
			TEMP_MAC3(iScrollHeight);
			TEMP_MAC3(iCaptionWidth);
			TEMP_MAC3(iCaptionHeight);
			TEMP_MAC4(lfCaptionFont);
			TEMP_MAC3(iSmCaptionWidth);
			TEMP_MAC3(iSmCaptionHeight);
			TEMP_MAC4(lfSmCaptionFont);
			TEMP_MAC3(iMenuWidth);
			TEMP_MAC3(iMenuHeight);
			TEMP_MAC4(lfMenuFont);
			TEMP_MAC4(lfStatusFont);
			TEMP_MAC4(lfMessageFont);

			fprintf(pStream, "m_lfIconWindowsDefault.lfHeight = %i;\nm_lfIconWindowsDefault.lfWeight = %i;\n", lf.lfHeight, lf.lfWeight);


			// Print for string table
#undef TEMP_MAC3
#undef TEMP_MAC4
#define TEMP_MAC3(xxx) fprintf(pStream, "%i ", ncm.xxx)
#define TEMP_MAC4(xxx) fprintf(pStream, "%i %i ", ncm.xxx.lfHeight, ncm.xxx.lfWeight)
			TEMP_MAC3(cbSize);
			TEMP_MAC3(iBorderWidth);
			TEMP_MAC3(iScrollWidth);
			TEMP_MAC3(iScrollHeight);
			TEMP_MAC3(iCaptionWidth);
			TEMP_MAC3(iCaptionHeight);
			TEMP_MAC4(lfCaptionFont);
			TEMP_MAC3(iSmCaptionWidth);
			TEMP_MAC3(iSmCaptionHeight);
			TEMP_MAC4(lfSmCaptionFont);
			TEMP_MAC3(iMenuWidth);
			TEMP_MAC3(iMenuHeight);
			TEMP_MAC4(lfMenuFont);
			TEMP_MAC4(lfStatusFont);
			TEMP_MAC4(lfMessageFont);

			fprintf(pStream, "%i %i\n", lf.lfHeight, lf.lfWeight);

			fclose(pStream);
		}

	}
#endif

	void SetToWindowsDefault()
	{
		m_rgb[ 0] = RGB(212,208,200); //192,192,192
		m_rgb[ 1] = RGB( 58,110,165);
		m_rgb[ 2] = RGB( 10, 36,106); // 0,0,128
		m_rgb[ 3] = RGB(128,128,128);
		m_rgb[ 4] = RGB(212,208,200); //192,192,192
		m_rgb[ 5] = RGB(255,255,255);
		m_rgb[ 6] = RGB(  0,  0,  0);
		m_rgb[ 7] = RGB(  0,  0,  0);
		m_rgb[ 8] = RGB(  0,  0,  0);
		m_rgb[ 9] = RGB(255,255,255);
		m_rgb[10] = RGB(212,208,200); //192,192,192
		m_rgb[11] = RGB(212,208,200); //192,192,192
		m_rgb[12] = RGB(128,128,128);
		m_rgb[13] = RGB( 10, 36,106); // 0,0,128
		m_rgb[14] = RGB(255,255,255);
		m_rgb[15] = RGB(212,208,200); //192,192,192
		m_rgb[16] = RGB(128,128,128);
		m_rgb[17] = RGB(128,128,128);
		m_rgb[18] = RGB(  0,  0,  0);
		m_rgb[19] = RGB(212,208,200); //192,192,192
		m_rgb[20] = RGB(255,255,255);
		m_rgb[21] = RGB( 81, 81, 75); // 0,0,0
		m_rgb[22] = RGB(236,234,231); // 192,192,192
		m_rgb[23] = RGB(  0,  0,  0);
		m_rgb[24] = RGB(255,255,225);
		m_rgb[25] = RGB(181,181,181); // button alternate face
		m_rgb[26] = RGB(  0,  0,128); 
		m_rgb[27] = RGB( 166,202,240); // 16,132,208
		m_rgb[28] = RGB(192,192,192); // 181,181,181
		m_FILTERKEYS.cbSize = 24;
		m_FILTERKEYS.dwFlags = 126;
		m_FILTERKEYS.iWaitMSec = 1000;
		m_FILTERKEYS.iDelayMSec = 1000; //500
		m_FILTERKEYS.iRepeatMSec = 500; //1000
		m_FILTERKEYS.iBounceMSec = 0;
		m_MOUSEKEYS.cbSize = 28;
		m_MOUSEKEYS.dwFlags = 62; //58
		m_MOUSEKEYS.iMaxSpeed = 40;
		m_MOUSEKEYS.iTimeToMaxSpeed = 300;
		m_MOUSEKEYS.iCtrlSpeed = 80;
		m_MOUSEKEYS.dwReserved1 = 0;
		m_MOUSEKEYS.dwReserved2 = 0;
		m_STICKYKEYS.cbSize = 8;
		m_STICKYKEYS.dwFlags = 510; //506
		m_TOGGLEKEYS.cbSize = 8;
		m_TOGGLEKEYS.dwFlags = 30; //26
		m_SOUNDSENTRY.cbSize = 48;
		m_SOUNDSENTRY.dwFlags = 2;
		m_SOUNDSENTRY.iFSTextEffect = 2;
		m_SOUNDSENTRY.iFSTextEffectMSec = 500;
		m_SOUNDSENTRY.iFSTextEffectColorBits = 0;
		m_SOUNDSENTRY.iFSGrafEffect = 3;
		m_SOUNDSENTRY.iFSGrafEffectMSec = 500;
		m_SOUNDSENTRY.iFSGrafEffectColor = 0;
		m_SOUNDSENTRY.iWindowsEffect = 1;
		m_SOUNDSENTRY.iWindowsEffectMSec = 500;
		m_SOUNDSENTRY.lpszWindowsEffectDLL = 0;
		m_SOUNDSENTRY.iWindowsEffectOrdinal = 0;
		m_ACCESSTIMEOUT.cbSize = 12;
		m_ACCESSTIMEOUT.dwFlags = 2; //3
		m_ACCESSTIMEOUT.iTimeOutMSec = 300000;
		m_bShowSounds = 0;
		m_bShowExtraKeyboardHelp = 0;
		m_bSwapMouseButtons = 0;
		m_nMouseTrails = 0;
		m_nMouseSpeed = 10;
		m_nIconSize = 32;
		m_nCursorScheme = 1;

		_ASSERTE(sizeof(m_FILTERKEYS) == m_FILTERKEYS.cbSize);
		_ASSERTE(sizeof(m_MOUSEKEYS) == m_MOUSEKEYS.cbSize);
		_ASSERTE(sizeof(m_STICKYKEYS) == m_STICKYKEYS.cbSize);
		_ASSERTE(sizeof(m_TOGGLEKEYS) == m_TOGGLEKEYS.cbSize);
		_ASSERTE(sizeof(m_SOUNDSENTRY) == m_SOUNDSENTRY.cbSize);
		_ASSERTE(sizeof(m_ACCESSTIMEOUT) == m_ACCESSTIMEOUT.cbSize);
		// _ASSERTE(sizeof(m_HIGHCONTRAST) == m_HIGHCONTRAST.cbSize);

		m_PortableNonClientMetrics.SetToWindowsDefault();

	}

	void LoadOriginal()
	{
		for(int i=0;i<COLOR_MAX_97_NT5;i++)
			m_rgb[i] = GetSysColor(i);

#define LOAD_SCHEME_ORIGINAL(xxx) m_##xxx.cbSize = sizeof(m_##xxx);SystemParametersInfo(SPI_GET##xxx, sizeof(m_##xxx), &m_##xxx, 0)

		LOAD_SCHEME_ORIGINAL(FILTERKEYS);
		LOAD_SCHEME_ORIGINAL(MOUSEKEYS);
		LOAD_SCHEME_ORIGINAL(STICKYKEYS);
		LOAD_SCHEME_ORIGINAL(TOGGLEKEYS);
		LOAD_SCHEME_ORIGINAL(SOUNDSENTRY);
		LOAD_SCHEME_ORIGINAL(ACCESSTIMEOUT);
//		LOAD_SCHEME_ORIGINAL(HIGHCONTRAST);
//		LOAD_SCHEME_ORIGINAL(SERIALKEYS);

//#pragma message("See if sound sentry works when a DLL is specified and we revert to original")
//#pragma message("See if high contrast works when a default scheme is specified and we revert to original")

		m_bShowSounds = GetSystemMetrics(SM_SHOWSOUNDS);
		SystemParametersInfo(SPI_GETKEYBOARDPREF, 0, &m_bShowExtraKeyboardHelp, 0);
		m_bSwapMouseButtons = GetSystemMetrics(SM_SWAPBUTTON);
		SystemParametersInfo(SPI_GETMOUSETRAILS, 0, &m_nMouseTrails, 0);
		SystemParametersInfo(SPI_GETMOUSESPEED,0, &m_nMouseSpeed, 0);

		// JMC: TODO: Do we care if x,y of icon are different.  What if they are using a nonstandard size
		m_nIconSize = GetSystemMetrics(SM_CXICON);
		m_nIconSize = SetShellLargeIconSize(0); // This just gets the current size
		// GetSystemMetrics(SM_CXICON) is not reflecting the registry change

		m_nCursorScheme = 0; // We are always using the 'current' cursor scheme =)

		m_PortableNonClientMetrics.LoadOriginal();

		// JMC: TODO: Clear out any read only bits so we don't set them (ie: see STICKYKEYS)
	}

	void AddChangesLine(int nId, LPTSTR szBuffer)
	{
		TCHAR szTemp[80];
		LoadString(g_hInstDll, nId, szTemp, ARRAYSIZE(szTemp));
		lstrcat(szBuffer, szTemp);
		lstrcat(szBuffer, __TEXT("\r\n"));
	}

	void ReportChanges(const WIZSCHEME &schemeOriginal, HWND hwndChanges)
	{
		TCHAR szChanges[80*20];
		szChanges[0] = 0;

		// Check for change in colors
		if(0 != memcmp(schemeOriginal.m_rgb, m_rgb, sizeof(m_rgb)))
			AddChangesLine(IDS_CHANGESCOLOR, szChanges);

#define TEST_CHANGES(xxx) if(0 != memcmp(&schemeOriginal.m_##xxx, &m_##xxx, sizeof(schemeOriginal.m_##xxx))) AddChangesLine(IDS_CHANGES##xxx, szChanges)
 		TEST_CHANGES(FILTERKEYS);
		TEST_CHANGES(MOUSEKEYS);
		TEST_CHANGES(STICKYKEYS);
		TEST_CHANGES(TOGGLEKEYS);
		TEST_CHANGES(SOUNDSENTRY);
		TEST_CHANGES(ACCESSTIMEOUT);
//		TEST_CHANGES(HIGHCONTRAST);
//		TEST_CHANGES(SERIALKEYS);

#define TEST_CHANGES2(xxx) if(schemeOriginal.m_b##xxx != m_b##xxx) AddChangesLine(IDS_CHANGES##xxx, szChanges)
 		TEST_CHANGES2(ShowSounds);
		TEST_CHANGES2(ShowExtraKeyboardHelp);
		TEST_CHANGES2(SwapMouseButtons);

#define TEST_CHANGES3(xxx) if(schemeOriginal.m_n##xxx != m_n##xxx) AddChangesLine(IDS_CHANGES##xxx, szChanges)
		TEST_CHANGES3(MouseTrails);
		TEST_CHANGES3(MouseSpeed);
		TEST_CHANGES3(IconSize);
		TEST_CHANGES3(CursorScheme);

		// TODO: ScrollWidth and BorderWidth have been removed

		// TODO: This provieds only one broad change line for all metric changes (including border/scroll bar)
		// NOTE: we have to check if any of our portable metrics are different, OR, windows is currently
		// not using the default windows font.
		PORTABLE_NONCLIENTMETRICS pncm1(schemeOriginal.m_PortableNonClientMetrics);
		PORTABLE_NONCLIENTMETRICS pncm2(m_PortableNonClientMetrics);
		pncm1.m_nFontFaces = pncm2.m_nFontFaces = 0; // WE MUST IGNORE THIS VALUE WHEN COMPARING

		if(		0 != memcmp(&pncm1, &pncm2, sizeof(pncm1))
			||	(m_PortableNonClientMetrics.m_nFontFaces == 1 && IsCurrentFaceNamesDifferent()))
			AddChangesLine(IDS_CHANGESNONCLIENTMETRICS, szChanges);

		if(!lstrlen(szChanges))
			AddChangesLine(IDS_CHANGESNOCHANGES, szChanges);

		SetWindowText(hwndChanges, szChanges);
	}

    void ApplyChanges(const WIZSCHEME &schemeNew, NONCLIENTMETRICS *pForceNCM = NULL, LOGFONT *pForcelfIcon = NULL);


	////////////////////////////////////////////////////////////
	// Setting the icon size

	static DWORD SetShellLargeIconSize( DWORD dwNewSize )
	{
		#define MAX_LENGTH   512
		DWORD   dwOldSize, dwLength = MAX_LENGTH, dwType = REG_SZ;
		TCHAR   szBuffer[MAX_LENGTH];
		HKEY   hKey;

		// Get the Key
		RegOpenKey( HKEY_CURRENT_USER, __TEXT("Control Panel\\desktop\\WindowMetrics"),&hKey);
		// Save the last size
		RegQueryValueEx( hKey, __TEXT("Shell Icon Size"), NULL, &dwType, (LPBYTE)szBuffer, &dwLength);
		dwOldSize = _ttol( szBuffer );
		// We will allow only values >=16 and <=72
		if( (dwNewSize>=16) && (dwNewSize<=72) )
		{
			wsprintf( szBuffer, __TEXT("%d"), dwNewSize );
			RegSetValueEx( hKey, __TEXT("Shell Icon Size"), 0, REG_SZ, (LPBYTE)szBuffer,
					(lstrlen(szBuffer) + 1) * sizeof(TCHAR) );
			SendMessage( HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETICONMETRICS,
					(LPARAM)("WindowMetrics") );
		}
		// Clean up
		RegCloseKey( hKey );
		// Let everyone know that things changed
		return dwOldSize;
		#undef MAX_LENGTH
	}


};

int GetSchemeCount();
void GetSchemeName(int nIndex, LPTSTR lpszName, int nLen);
SCHEMEDATALOCAL &GetScheme(int nIndex);


///////////////////////////////////////////
// Stuff for Fonts
int GetFontCount();
void GetFontLogFont(int nIndex, LOGFONT *pLogFont);

#endif // _INC_SCHEMES_H
