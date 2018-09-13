#include "pch.hxx" // pch
#pragma hdrstop

#include "Schemes.h"

// To use the old way of enumerating fonts to get the font list,
// and reading schemes from the registry, remove the comments from
// the two lines below
//#define ENUMERATEFONTS
//#define READSCHEMESFROMREGISTRY



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
		IDS_SCHEME_WINDOWSSTANDARD,
		25,
		{
			RGB(192, 192, 192), // Scrollbar
			RGB(  0, 128, 128), // Background
			RGB(  0,   0, 128), // ActiveTitle
			RGB(128, 128, 128), // InactiveTitle
			RGB(192, 192, 192), // Menu
			RGB(255, 255, 255), // Window
			RGB(  0,   0,   0), // WindowFrame
			RGB(  0,   0,   0), // MenuText
			RGB(  0,   0,   0), // WindowText
			RGB(255, 255, 255), // TitleText
			RGB(192, 192, 192), // ActiveBorder
			RGB(192, 192, 192), // InactiveBorder
			RGB(128, 128, 128), // AppWorkspace
			RGB(  0,   0, 128), // Hilight
			RGB(255, 255, 255), // HilightText
			RGB(192, 192, 192), // ButtonFace
			RGB(128, 128, 128), // ButtonShadow
			RGB(128, 128, 128), // GrayText
			RGB(  0,   0,   0), // ButtonText
			RGB(192, 192, 192), // InactiveTitleText
			RGB(255, 255, 255), // ButtonHilight
			RGB(  0,   0,   0), // ButtonDkShadow
			RGB(223, 223, 223), // ButtonLight
			RGB(  0,   0,   0), // InfoText
			RGB(255, 255, 225), // InfoWindow
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

//
/////////////////////////////////////////////////////////////////////
