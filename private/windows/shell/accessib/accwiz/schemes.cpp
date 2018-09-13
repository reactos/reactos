#include "pch.hxx" // pch
#pragma hdrstop

#include "Schemes.h"

// To use the old way of enumerating fonts to get the font list,
// and reading schemes from the registry, remove the comments from
// the two lines below
//#define ENUMERATEFONTS
//#define READSCHEMESFROMREGISTRY

void WIZSCHEME::ApplyChanges(const WIZSCHEME &schemeNew, NONCLIENTMETRICS *pForceNCM, LOGFONT *pForcelfIcon)
	{
        HKEY hk;
        TCHAR szRGB[32];
        COLORREF rgb;

        // This is a very messy way of doing things, Instead one can create a neat custom scheme and
        // load it and allow "desk.cpl" to do all the things. a-anilk

		// This applies the settings in the schemeNew scheme
		// Check for change in colors
		if(0 != memcmp(schemeNew.m_rgb, m_rgb, sizeof(m_rgb)))
		{
			int rgInts[COLOR_MAX_97_NT5];
			for(int i=0;i<COLOR_MAX_97_NT5;i++)
				rgInts[i] = i;
			SetSysColors(COLOR_MAX_97_NT5/*COLOR_MAX_95_NT4*/, rgInts, schemeNew.m_rgb);
             
            // The following lines of code will update the registry \\ControlPanel\Colors to 
            // reflect the new scheme. Ans thus will be available for the user when he logs on 
            // again :a-anilk
            if(RegCreateKey(HKEY_CURRENT_USER, szRegStr_Colors, &hk) == ERROR_SUCCESS)
            {
                // write out the color information to win.ini
                for (i = 0; i < COLOR_MAX_97_NT5; i++)
                {
                    rgb = schemeNew.m_rgb[i];
                    wsprintf(szRGB, TEXT("%d %d %d"), GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));

                    // For the time being we will update the INI file also.
                    WriteProfileString(g_szColors, s_pszColorNames[i], szRGB);
                    // Update the registry (Be sure to include the terminating zero in the byte count!)
                    RegSetValueEx(hk, s_pszColorNames[i], 0L, REG_SZ, (LPBYTE)szRGB, sizeof(TCHAR) * (lstrlen(szRGB)+1));
                }

                RegCloseKey(hk);
            }
		}

#define APPLY_SCHEME_CURRENT(xxx) if(0 != memcmp(&schemeNew.m_##xxx, &m_##xxx, sizeof(schemeNew.m_##xxx))) SystemParametersInfo(SPI_SET##xxx, sizeof(schemeNew.m_##xxx), (PVOID)&schemeNew.m_##xxx, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)

		APPLY_SCHEME_CURRENT(FILTERKEYS);
		APPLY_SCHEME_CURRENT(MOUSEKEYS);
		APPLY_SCHEME_CURRENT(STICKYKEYS);
		APPLY_SCHEME_CURRENT(TOGGLEKEYS);
		APPLY_SCHEME_CURRENT(SOUNDSENTRY);
		APPLY_SCHEME_CURRENT(ACCESSTIMEOUT);
//		APPLY_SCHEME_CURRENT(HIGHCONTRAST);
//		APPLY_SCHEME_CURRENT(SERIALKEYS);

		// Check Show Sounds
		if(schemeNew.m_bShowSounds != m_bShowSounds)
			SystemParametersInfo(SPI_SETSHOWSOUNDS, schemeNew.m_bShowSounds, NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

		// Check Extra keyboard help
		if(schemeNew.m_bShowExtraKeyboardHelp != m_bShowExtraKeyboardHelp)
		{
			// Both required: 
			SystemParametersInfo(SPI_SETKEYBOARDPREF, schemeNew.m_bShowExtraKeyboardHelp, NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
			SystemParametersInfo(SPI_SETKEYBOARDCUES, 0, (PVOID)schemeNew.m_bShowExtraKeyboardHelp, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		}

		// Check swap mouse buttons
		if(schemeNew.m_bSwapMouseButtons != m_bSwapMouseButtons)
			SwapMouseButton(schemeNew.m_bSwapMouseButtons);

		// Check Mouse Trails
		if(schemeNew.m_nMouseTrails != m_nMouseTrails)
			SystemParametersInfo(SPI_SETMOUSETRAILS, schemeNew.m_nMouseTrails, NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

		// JMC: VERIFY ON MEMPHIS THAT MOUSE SPEED USES AN INT CAST TO A POINTER
		// Check Mouse Speed
		if(schemeNew.m_nMouseSpeed != m_nMouseSpeed)
			SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)schemeNew.m_nMouseSpeed, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

		// Check icon size
		if(schemeNew.m_nIconSize != m_nIconSize)
			WIZSCHEME::SetShellLargeIconSize(schemeNew.m_nIconSize);

		// Check cursor scheme
		if(schemeNew.m_nCursorScheme != m_nCursorScheme)
			ApplyCursorScheme(schemeNew.m_nCursorScheme);

		// NonClientMetric changes
		{
			NONCLIENTMETRICS ncmOrig;
			LOGFONT lfOrig;
			GetNonClientMetrics(&ncmOrig, &lfOrig);
			if(pForceNCM)
			{
				// If they gave us a NCM, they must also give us a LOGFONT for the icon
				_ASSERTE(pForcelfIcon);
				// We were given an Original NCM to use
				if(0 != memcmp(pForceNCM, &ncmOrig, sizeof(ncmOrig)))
					SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(*pForceNCM), pForceNCM, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
				if(0 != memcmp(pForcelfIcon, &lfOrig, sizeof(lfOrig)))
					SystemParametersInfo(SPI_SETICONTITLELOGFONT, sizeof(*pForcelfIcon), pForcelfIcon, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
			}
			else
			{
				// Note: This part of apply changes does not look at schemeCurrent - it only looks
				// at what we are applying
				schemeNew.m_PortableNonClientMetrics.ApplyChanges();
			}
		}


		*this = schemeNew;
	}


/////////////////////////////////////////////////////////////////////
//  New way of enumerating fonts

#ifndef ENUMERATEFONTS

static LPCTSTR g_lpszFontNames[] =
{
	__TEXT("Arial"),
	__TEXT("MS Sans Serif"),
	__TEXT("Tahoma"),
	__TEXT("Times New Roman")
};

int GetFontCount()
{
	return ARRAYSIZE(g_lpszFontNames);
}

void GetFontLogFont(int nIndex, LOGFONT *pLogFont)
{
	_ASSERTE(nIndex < ARRAYSIZE(g_lpszFontNames));
	memset(pLogFont, 0, sizeof(*pLogFont));
	lstrcpy(pLogFont->lfFaceName, g_lpszFontNames[nIndex]);
}


#endif // ENUMERATEFONTS

//
/////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////
//  New way of storing schemes as hard coded values

#ifndef READSCHEMESFROMREGISTRY

#include "resource.h"

static SCHEMEDATALOCAL g_rgSchemeData[] = 
{
	{
		IDS_SCHEME_HIGHCONTRASTBLACKALTERNATE,
		25,
		{
			RGB(  0,   0,   0), // Scrollbar
			RGB(  0,   0,   0), // Background
			RGB(  0,   0, 255), // ActiveTitle
			RGB(  0, 255, 255), // InactiveTitle
			RGB(  0,   0,   0), // Menu
			RGB(  0,   0,   0), // Window
			RGB(255, 255, 255), // WindowFrame
			RGB(255, 255, 255), // MenuText
			RGB(255, 255,   0), // WindowText
			RGB(255, 255, 255), // TitleText
			RGB(  0,   0, 255), // ActiveBorder
			RGB(  0, 255, 255), // InactiveBorder
			RGB(  0,   0,   0), // AppWorkspace
			RGB(  0, 128,   0), // Hilight
			RGB(255, 255, 255), // HilightText
			RGB(  0,   0,   0), // ButtonFace
			RGB(128, 128, 128), // ButtonShadow
			RGB(  0, 255,   0), // GrayText
			RGB(255, 255, 255), // ButtonText
			RGB(  0,   0,   0), // InactiveTitleText
			RGB(192, 192, 192), // ButtonHilight
			RGB(255, 255, 255), // ButtonDkShadow
			RGB(255, 255, 255), // ButtonLight
			RGB(255, 255,   0), // InfoText
			RGB(  0,   0,   0), // InfoWindow
		}
	},
	{
		IDS_SCHEME_HIGHCONTRASTWHITEALTERNATE,
		25,
		{
#if 1
			RGB(  0,   0,   0), // Scrollbar
			RGB(  0,   0,   0), // Background
			RGB(  0, 255, 255), // ActiveTitle
			RGB(  0,   0, 255), // InactiveTitle
			RGB(  0,   0,   0), // Menu
			RGB(  0,   0,   0), // Window
			RGB(255, 255, 255), // WindowFrame
			RGB(  0, 255,   0), // MenuText
			RGB(  0, 255,   0), // WindowText
			RGB(  0,   0,   0), // TitleText
			RGB(  0, 255, 255), // ActiveBorder
			RGB(  0,   0, 255), // InactiveBorder
			RGB(255, 251, 240), // AppWorkspace
			RGB(  0,   0, 255), // Hilight
			RGB(255, 255, 255), // HilightText
//			RGB(255, 255, 255), // ButtonFace
			RGB(0, 0, 0), // ButtonFace
			RGB(128, 128, 128), // ButtonShadow
			RGB(  0, 255,   0), // GrayText
			RGB(  0, 255,   0), // ButtonText
			RGB(255, 255, 255), // InactiveTitleText
			RGB(192, 192, 192), // ButtonHilight
			RGB(255, 255, 255), // ButtonDkShadow
			RGB(255, 255, 255), // ButtonLight
			RGB(  0,   0,   0), // InfoText
			RGB(255, 255,   0), // InfoWindow
#else
			RGB(  0,   0,   0), // Scrollbar
			RGB(  0,   0,   0), // Background
			RGB(  0, 255, 255), // ActiveTitle
			RGB(  0,   0, 255), // InactiveTitle
			RGB(  0,   0,   0), // Menu
			RGB(255, 251, 240), // Window
			RGB(255, 255, 255), // WindowFrame
			RGB(  0, 255,   0), // MenuText
			RGB(  0,   0,   0), // WindowText
			RGB(  0,   0,   0), // TitleText
			RGB(  0, 255, 255), // ActiveBorder
			RGB(  0,   0, 255), // InactiveBorder
			RGB(255, 251, 240), // AppWorkspace
			RGB(  0,   0, 255), // Hilight
			RGB(255, 255, 255), // HilightText
			RGB(255, 255,   0), // ButtonFace
			RGB(128, 128, 128), // ButtonShadow
			RGB(  0, 255,   0), // GrayText
			RGB(255, 255,   0), // ButtonText
			RGB(255, 255, 255), // InactiveTitleText
			RGB(192, 192, 192), // ButtonHilight
			RGB(255, 255, 255), // ButtonDkShadow
			RGB(255, 255, 255), // ButtonLight
			RGB(255, 255,   0), // InfoText
			RGB(  0,   0,   0), // InfoWindow
#endif
		}
	},
	{
		IDS_SCHEME_HIGHCONTRASTBLACK,
		25,
		{
			RGB(  0,   0,   0), // Scrollbar
			RGB(  0,   0,   0), // Background
			RGB(128,   0, 128), // ActiveTitle
			RGB(  0, 128,   0), // InactiveTitle
			RGB(  0,   0,   0), // Menu
			RGB(  0,   0,   0), // Window
			RGB(255, 255, 255), // WindowFrame
			RGB(255, 255, 255), // MenuText
			RGB(255, 255, 255), // WindowText
			RGB(255, 255, 255), // TitleText
			RGB(255, 255,   0), // ActiveBorder
			RGB(  0, 128,   0), // InactiveBorder
			RGB(  0,   0,   0), // AppWorkspace
			RGB(128,   0, 128), // Hilight
			RGB(255, 255, 255), // HilightText
			RGB(  0,   0,   0), // ButtonFace
			RGB(128, 128, 128), // ButtonShadow
			RGB(  0, 255,   0), // GrayText
			RGB(255, 255, 255), // ButtonText
			RGB(255, 255, 255), // InactiveTitleText
			RGB(192, 192, 192), // ButtonHilight
			RGB(255, 255, 255), // ButtonDkShadow
			RGB(255, 255, 255), // ButtonLight
			RGB(255, 255, 255), // InfoText
			RGB(  0,   0,   0), // InfoWindow
		}
	},
	{
		IDS_SCHEME_HIGHCONTRASTWHITE,
		25,
		{
			RGB(255, 255, 255), // Scrollbar
			RGB(255, 255, 255), // Background
			RGB(  0,   0,   0), // ActiveTitle
			RGB(255, 255, 255), // InactiveTitle
			RGB(255, 255, 255), // Menu
			RGB(255, 255, 255), // Window
			RGB(  0,   0,   0), // WindowFrame
			RGB(  0,   0,   0), // MenuText
			RGB(  0,   0,   0), // WindowText
			RGB(255, 255, 255), // TitleText
			RGB(128, 128, 128), // ActiveBorder
			RGB(192, 192, 192), // InactiveBorder
			RGB(128, 128, 128), // AppWorkspace
			RGB(  0,   0,   0), // Hilight
			RGB(255, 255, 255), // HilightText
			RGB(255, 255, 255), // ButtonFace
			RGB(128, 128, 128), // ButtonShadow
			RGB(  0, 255,   0), // GrayText
			RGB(  0,   0,   0), // ButtonText
			RGB(  0,   0,   0), // InactiveTitleText
			RGB(192, 192, 192), // ButtonHilight
			RGB(  0,   0,   0), // ButtonDkShadow
			RGB(192, 192, 192), // ButtonLight
			RGB(  0,   0,   0), // InfoText
			RGB(255, 255, 255), // InfoWindow
		}
	},
	{
		IDS_SCHEME_WINDOWSSTANDARD,
		29,
		{
			RGB(212, 208, 200), // Scrollbar
			RGB( 58, 110, 165), // Background
			RGB( 10, 36, 106), // ActiveTitle
			RGB(128, 128, 128), // InactiveTitle
			RGB(212, 208, 200), // Menu
			RGB(255, 255, 255), // Window
			RGB(  0,   0,   0), // WindowFrame
			RGB(  0,   0,   0), // MenuText
			RGB(  0,   0,   0), // WindowText
			RGB(255, 255, 255), // TitleText
			RGB(212, 208, 200), // ActiveBorder
			RGB(212, 208, 200), // InactiveBorder
			RGB( 10, 36, 106), // AppWorkspace
			RGB(  0,   0, 128), // Hilight
			RGB(255, 255, 255), // HilightText
			RGB(212, 208, 200), // ButtonFace
			RGB(128, 128, 128), // ButtonShadow
			RGB(128, 128, 128), // GrayText
			RGB(  0,   0,   0), // ButtonText
			RGB(212, 208, 200), // InactiveTitleText
			RGB(255, 255, 255), // ButtonHilight
			RGB( 81, 81, 75), // ButtonDkShadow
			RGB(236, 234, 231), // ButtonLight
			RGB(  0,   0,   0), // InfoText
			RGB(255, 255, 225), // InfoWindow
			RGB(181, 181, 181), // button alternate face
			RGB(  0,  0, 128), 
			RGB( 166, 202, 240), // 16,132,208
			RGB(192, 192, 192) // 181,181,181
		}
	}
};


int GetSchemeCount()
{
	return ARRAYSIZE(g_rgSchemeData);
}

void GetSchemeName(int nIndex, LPTSTR lpszName, int nLen) // JMC: HACK - You must allocate enough space
{
	_ASSERTE(nIndex < ARRAYSIZE(g_rgSchemeData));
	LoadString(g_hInstDll, g_rgSchemeData[nIndex].nNameStringId, lpszName, nLen);
}

SCHEMEDATALOCAL &GetScheme(int nIndex)
{
	_ASSERTE(nIndex < ARRAYSIZE(g_rgSchemeData));
	return g_rgSchemeData[nIndex];
}


#endif // READSCHEMESFROMREGISTRY

//
/////////////////////////////////////////////////////////////////////





/////////////////////////////////////////////////////////////////////
// Below this point in the file, we have the old way we use
// to enumerate fonts and schemes.





/////////////////////////////////////////////////////////////////////
//  Old way of enumerating fonts
#ifdef ENUMERATEFONTS

// Global Variables
static ENUMLOGFONTEX g_rgFonts[200]; // JMC: HACK - At Most 200 Fonts
static int g_nFontCount = 0;
static BOOL bFontsAlreadyInit = FALSE;

void Font_Init();

int GetFontCount()
{
	if(!bFontsAlreadyInit)
		Font_Init();
	return g_nFontCount;
}

void GetFontLogFont(int nIndex, LOGFONT *pLogFont)
{
	if(!bFontsAlreadyInit)
		Font_Init();
	*pLogFont = g_rgFonts[nIndex].elfLogFont;
}


int CALLBACK EnumFontFamExProc(
    ENUMLOGFONTEX *lpelfe,	// pointer to logical-font data
    NEWTEXTMETRICEX *lpntme,	// pointer to physical-font data
    int FontType,	// type of font
    LPARAM lParam	// application-defined data 
   )
{
	if(g_nFontCount>200)
		return 0; // JMC: HACK - Stop enumerating if more than 200 families

	// Don't use if we already have this font name
	BOOL bHave = FALSE;
	for(int i=0;i<g_nFontCount;i++)
		if(0 == lstrcmp((TCHAR *)g_rgFonts[i].elfFullName, (TCHAR *)lpelfe->elfFullName))
		{
			bHave = TRUE;
			break;
		}
	if(!bHave)
		g_rgFonts[g_nFontCount++] = *lpelfe;
	return 1;
}

void Font_Init()
{
	// Only do the stuff in this function once.
	if(bFontsAlreadyInit)
		return;
	bFontsAlreadyInit = TRUE;

	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
//	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfCharSet = OEM_CHARSET;
	HDC hdc = GetDC(NULL);
	EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontFamExProc, 0, 0);
	ReleaseDC(NULL, hdc);
	// JMC: Make sure there is at least one font
}

#endif ENUMERATEFONTS

//
/////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////
//  Old way of reading schemes from the registry

#ifdef READSCHEMESFROMREGISTRY

extern PTSTR s_pszColorNames[]; // JMC: HACK


// Scheme data for Windows 95
typedef struct {
    SHORT version;
//    NONCLIENTMETRICSA ncm;
//    LOGFONTA lfIconTitle;
	BYTE rgDummy[390]; // This is the size of NONCLIENTMETRICSA and LOGFONTA in 16 bit Windows!!!
    COLORREF rgb[COLOR_MAX_95_NT4];
} SCHEMEDATA_95;

// New scheme data for Windows 97
typedef struct {
    SHORT version;
//    NONCLIENTMETRICSA ncm;
//    LOGFONTA lfIconTitle;
	BYTE rgDummy[390]; // This is the size of NONCLIENTMETRICSA and LOGFONTA in 16 bit Windows!!!
    COLORREF rgb[COLOR_MAX_97_NT5];
} SCHEMEDATA_97;

// Scheme data for Windows NT 4.0
typedef struct {
    SHORT version;
    WORD  wDummy;               // for alignment
    NONCLIENTMETRICSW ncm;
    LOGFONTW lfIconTitle;
    COLORREF rgb[COLOR_MAX_95_NT4];
} SCHEMEDATA_NT4;

// Scheme data for Windows NT 5.0
typedef struct {
    SHORT version;
    WORD  wDummy;               // for alignment
    NONCLIENTMETRICSW ncm;
    LOGFONTW lfIconTitle;
    COLORREF rgb[COLOR_MAX_97_NT5];
} SCHEMEDATA_NT5;

static SCHEMEDATALOCAL g_rgSchemeData[100]; // JMC: HACK - At Most 100 schemes
static TCHAR g_rgSchemeNames[100][100];
static int g_nSchemeCount = 0;
static BOOL bSchemesAlreadyInit = FALSE;

void Scheme_Init();

int GetSchemeCount()
{
	if(!bSchemesAlreadyInit)
		Scheme_Init();
	return g_nSchemeCount;
}

void GetSchemeName(int nIndex, LPTSTR lpszName, int nLen) // JMC: HACK - You must allocate enough space
{
	if(!bSchemesAlreadyInit)
		Scheme_Init();
	_tcsncpy(lpszName, g_rgSchemeNames[i], nLen - 1);
	lpstName[nLen - 1] = 0; // Guarantee NULL termination
}

SCHEMEDATALOCAL &GetScheme(int nIndex)
{
	if(!bSchemesAlreadyInit)
		Scheme_Init();
	return g_rgSchemeData[nIndex];
}


void Scheme_Init()
{
	// Only do the stuff in this function once.
	if(bSchemesAlreadyInit)
		return;
	bSchemesAlreadyInit = TRUE;

    HKEY hkSchemes;
    DWORD dw, dwSize;
    TCHAR szBuf[100];

	g_nSchemeCount = 0;

    if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_LOOKSCHEMES, &hkSchemes) != ERROR_SUCCESS)
        return;

    for (dw=0; ; dw++)
    {
		if(g_nSchemeCount>99)
			break; //JMC: HACK - At Most 100 schemes

        dwSize = ARRAYSIZE(szBuf);
        if (RegEnumValue(hkSchemes, dw, szBuf, &dwSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;  // Bail if no more values

		DWORD dwType;
		DWORD dwSize;
		RegQueryValueEx(hkSchemes, szBuf, NULL, &dwType, NULL, &dwSize);
		if(dwType == REG_BINARY)
		{
			// Always copy the current name to the name array - if there
			// is an error in the data, we just won't upcount g_nSchemeCount
			lstrcpy(g_rgSchemeNames[g_nSchemeCount], szBuf);

			// Find out which type of scheme this is, and convert to the
			// SCHEMEDATALOCAL type
			switch(dwSize)
			{
			case sizeof(SCHEMEDATA_95):
				{
					SCHEMEDATA_95 sd;
					RegQueryValueEx(hkSchemes, szBuf, NULL, &dwType, (BYTE *)&sd, &dwSize);
					if(1 != sd.version)
						break; // We have the wrong version even though the size was correct

					// Copy the color information from the registry info to g_rgSchemeData
					g_rgSchemeData[g_nSchemeCount].nColorsUsed = COLOR_MAX_95_NT4;

					// Copy the color array
					for(int i=0;i<g_rgSchemeData[g_nSchemeCount].nColorsUsed;i++)
						g_rgSchemeData[g_nSchemeCount].rgb[i] = sd.rgb[i];

					g_nSchemeCount++;
				}
				break;
			case sizeof(SCHEMEDATA_NT4):
				{
					SCHEMEDATA_NT4 sd;
					RegQueryValueEx(hkSchemes, szBuf, NULL, &dwType, (BYTE *)&sd, &dwSize);
					if(2 != sd.version)
						break; // We have the wrong version even though the size was correct

					// Copy the color information from the registry info to g_rgSchemeData
					g_rgSchemeData[g_nSchemeCount].nColorsUsed = COLOR_MAX_95_NT4;

					// Copy the color array
					for(int i=0;i<g_rgSchemeData[g_nSchemeCount].nColorsUsed;i++)
						g_rgSchemeData[g_nSchemeCount].rgb[i] = sd.rgb[i];

					g_nSchemeCount++;
				}
				break;
			case sizeof(SCHEMEDATA_97):
				{
					SCHEMEDATA_97 sd;
					RegQueryValueEx(hkSchemes, szBuf, NULL, &dwType, (BYTE *)&sd, &dwSize);
					if(3 != sd.version)
						break; // We have the wrong version even though the size was correct

					// Copy the color information from the registry info to g_rgSchemeData
					g_rgSchemeData[g_nSchemeCount].nColorsUsed = COLOR_MAX_97_NT5;

					// Copy the color array
					for(int i=0;i<g_rgSchemeData[g_nSchemeCount].nColorsUsed;i++)
						g_rgSchemeData[g_nSchemeCount].rgb[i] = sd.rgb[i];

					g_nSchemeCount++;
				}
				break;
			case sizeof(SCHEMEDATA_NT5):
				{
					SCHEMEDATA_NT5 sd;
					RegQueryValueEx(hkSchemes, szBuf, NULL, &dwType, (BYTE *)&sd, &dwSize);
					if(2 != sd.version)
						break; // We have the wrong version even though the size was correct

					// Copy the color information from the registry info to g_rgSchemeData
					g_rgSchemeData[g_nSchemeCount].nColorsUsed = COLOR_MAX_97_NT5;

					// Copy the color array
					for(int i=0;i<g_rgSchemeData[g_nSchemeCount].nColorsUsed;i++)
						g_rgSchemeData[g_nSchemeCount].rgb[i] = sd.rgb[i];

					g_nSchemeCount++;
				}
				break;
			default:
				// We had an unknown sized structure in the registry - IGNORE IT
#ifdef _DEBUG
				TCHAR sz[200];
				wsprintf(sz, __TEXT("Scheme - %s, size = %i, sizeof(95) = %i, sizeof(NT4) = %i, sizeof(97) = %i, sizeof(NT5) = %i"), szBuf, dwSize,
						sizeof(SCHEMEDATA_95),
						sizeof(SCHEMEDATA_NT4),
						sizeof(SCHEMEDATA_97),
						sizeof(SCHEMEDATA_NT5)
						);
				MessageBox(NULL, sz, NULL, MB_OK);
#endif // _DEBUG
				break;
			}
		}
    }
    RegCloseKey(hkSchemes);


	// JMC: Verify that there are at least one schemes

#if 0
	// JMC: HACK to write these to the clipboard
	TCHAR sz[20000];
	sz[0] = 0;
	for(int i=0;i<g_nSchemeCount;i++)
	{
		TCHAR sz2[100];
		wsprintf(sz2, g_rgSchemeNames[i]);
		lstrcat(sz, sz2);
		lstrcat(sz, __TEXT("\r\n"));
		for(int j=0;j<g_rgSchemeData[i].nColorsUsed;j++)
		{
			wsprintf(sz2, __TEXT("RGB(%3i, %3i, %3i), // %s\r\n"),
				(int)GetRValue(g_rgSchemeData[i].rgb[j]),
				(int)GetGValue(g_rgSchemeData[i].rgb[j]),
				(int)GetBValue(g_rgSchemeData[i].rgb[j]),
				s_pszColorNames[j]);
			lstrcat(sz, sz2);

		}
		lstrcat(sz, __TEXT("\r\n\r\n"));
	}

	int nLen = lstrlen(sz);
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (nLen + 1) * sizeof(TCHAR));
	LPTSTR lpsz = (LPTSTR)GlobalLock(hGlobal);
	lstrcpy(lpsz, sz);
	GlobalUnlock(hGlobal);

	OpenClipboard(NULL);
	EmptyClipboard();
#ifdef _UNICODE
	SetClipboardData(CF_UNICODETEXT, hGlobal);
#else
	SetClipboardData(CF_TEXT, hGlobal);
#endif
	CloseClipboard();
#endif

}

#endif // READSCHEMESFROMREGISTRY

void PORTABLE_NONCLIENTMETRICS::ApplyChanges() const
{
		NONCLIENTMETRICS ncmOrig;
		LOGFONT lfIconOrig;
		GetNonClientMetrics(&ncmOrig, &lfIconOrig);

		NONCLIENTMETRICS ncmNew;
		LOGFONT lfIconNew;

		ZeroMemory(&ncmNew, sizeof(ncmNew));
		ZeroMemory(&lfIconNew, sizeof(lfIconNew));

		ncmNew.cbSize = sizeof(ncmNew);
		ncmNew.iBorderWidth = m_iBorderWidth;
		ncmNew.iScrollWidth = m_iScrollWidth;
		ncmNew.iScrollHeight = m_iScrollHeight;
		ncmNew.iCaptionWidth = m_iCaptionWidth;
		ncmNew.iCaptionHeight = m_iCaptionHeight;
		ncmNew.lfCaptionFont.lfHeight = m_lfCaptionFont_lfHeight;
		ncmNew.lfCaptionFont.lfWeight = m_lfCaptionFont_lfWeight;
		ncmNew.iSmCaptionWidth = m_iSmCaptionWidth;
		ncmNew.iSmCaptionHeight = m_iSmCaptionHeight;
		ncmNew.lfSmCaptionFont.lfHeight = m_lfSmCaptionFont_lfHeight;
		ncmNew.lfSmCaptionFont.lfWeight = m_lfSmCaptionFont_lfWeight;
		ncmNew.iMenuWidth = m_iMenuWidth;
		ncmNew.iMenuHeight = m_iMenuHeight;
		ncmNew.lfMenuFont.lfHeight = m_lfMenuFont_lfHeight;
		ncmNew.lfMenuFont.lfWeight = m_lfMenuFont_lfWeight;
		ncmNew.lfStatusFont.lfHeight = m_lfStatusFont_lfHeight;
		ncmNew.lfStatusFont.lfWeight = m_lfStatusFont_lfWeight;
		ncmNew.lfMessageFont.lfHeight = m_lfMessageFont_lfHeight;
		ncmNew.lfMessageFont.lfWeight = m_lfMessageFont_lfWeight;
		lfIconNew.lfHeight = m_lfIconWindowsDefault_lfHeight;
		lfIconNew.lfWeight = m_lfIconWindowsDefault_lfWeight;


		// Fill in fonts
		if(m_nFontFaces)
		{
			TCHAR lfFaceName[LF_FACESIZE];
			LoadString(g_hInstDll, IDS_SYSTEMFONTNAME, lfFaceName, ARRAYSIZE(lfFaceName));

			BYTE lfCharSet;
			TCHAR szCharSet[20];
			if(LoadString(g_hInstDll,IDS_FONTCHARSET, szCharSet,sizeof(szCharSet)/sizeof(TCHAR))) {
				lfCharSet = (BYTE)_tcstoul(szCharSet,NULL,10);
			} else {
				lfCharSet = 0; // Default
			}

			ncmNew.lfCaptionFont.lfCharSet = lfCharSet;
			ncmNew.lfSmCaptionFont.lfCharSet = lfCharSet;
			ncmNew.lfMenuFont.lfCharSet = lfCharSet;
			ncmNew.lfStatusFont.lfCharSet = lfCharSet;
			ncmNew.lfMessageFont.lfCharSet = lfCharSet;
			lfIconNew.lfCharSet = lfCharSet;

			lstrcpy(ncmNew.lfCaptionFont.lfFaceName, lfFaceName);
			lstrcpy(ncmNew.lfSmCaptionFont.lfFaceName, lfFaceName);
			lstrcpy(ncmNew.lfMenuFont.lfFaceName, lfFaceName);
			lstrcpy(ncmNew.lfStatusFont.lfFaceName, lfFaceName);
			lstrcpy(ncmNew.lfMessageFont.lfFaceName, lfFaceName);
			lstrcpy(lfIconNew.lfFaceName, lfFaceName);
		}
		else
		{
			ncmNew.lfCaptionFont.lfCharSet = ncmOrig.lfCaptionFont.lfCharSet;
			ncmNew.lfSmCaptionFont.lfCharSet = ncmOrig.lfSmCaptionFont.lfCharSet;
			ncmNew.lfMenuFont.lfCharSet = ncmOrig.lfMenuFont.lfCharSet;
			ncmNew.lfStatusFont.lfCharSet = ncmOrig.lfStatusFont.lfCharSet;
			ncmNew.lfMessageFont.lfCharSet = ncmOrig.lfMessageFont.lfCharSet;
			lfIconNew.lfCharSet = lfIconOrig.lfCharSet;

			lstrcpy(ncmNew.lfCaptionFont.lfFaceName, ncmOrig.lfCaptionFont.lfFaceName);
			lstrcpy(ncmNew.lfSmCaptionFont.lfFaceName, ncmOrig.lfSmCaptionFont.lfFaceName);
			lstrcpy(ncmNew.lfMenuFont.lfFaceName, ncmOrig.lfMenuFont.lfFaceName);
			lstrcpy(ncmNew.lfStatusFont.lfFaceName, ncmOrig.lfStatusFont.lfFaceName);
			lstrcpy(ncmNew.lfMessageFont.lfFaceName, ncmOrig.lfMessageFont.lfFaceName);
			lstrcpy(lfIconNew.lfFaceName, lfIconOrig.lfFaceName);
		}


		if(0 != memcmp(&ncmNew, &ncmOrig, sizeof(ncmOrig)))
			SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(ncmNew), (PVOID)&ncmNew, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
		if(0 != memcmp(&lfIconNew, &lfIconOrig, sizeof(lfIconOrig)))
			SystemParametersInfo(SPI_SETICONTITLELOGFONT, sizeof(lfIconNew), &lfIconNew, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}


//
/////////////////////////////////////////////////////////////////////
