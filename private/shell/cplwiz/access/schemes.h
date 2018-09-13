
#ifndef _INC_SCHEMES_H
#define _INC_SCHEMES_H

// HACK - THESE VALUES ARE HARD CODED
#define COLOR_MAX_95_NT4		25
#define COLOR_MAX_97_NT5		29

// Scheme data used locally by this app - NOTE: This structure
// does NOT use or need the A and W forms for its members.  The other schemedata's
// MUST use the A and W forms since that's how they are stored in the registry
typedef struct {
	int nNameStringId;
	int nColorsUsed;
    COLORREF rgb[COLOR_MAX_97_NT5];
} SCHEMEDATALOCAL;


struct WIZSCHEME
{
	COLORREF m_rgb[COLOR_MAX_97_NT5];
	NONCLIENTMETRICS m_ncm;

	FILTERKEYS m_FILTERKEYS;
	MOUSEKEYS m_MOUSEKEYS;
	SERIALKEYS m_SERIALKEYS;
	STICKYKEYS m_STICKYKEYS;
	TOGGLEKEYS m_TOGGLEKEYS;
	SOUNDSENTRY m_SOUNDSENTRY;
	ACCESSTIMEOUT m_ACCESSTIMEOUT;
	HIGHCONTRAST m_HIGHCONTRAST;

	BOOL m_bShowSounds;
	BOOL m_bShowExtraKeyboardHelp;
	BOOL m_bSwapMouseButtons;

	int m_nIconSize;

};

int GetSchemeCount();
void GetSchemeName(int nIndex, LPTSTR lpszName, int nLen);
SCHEMEDATALOCAL &GetScheme(int nIndex);


///////////////////////////////////////////
// Stuff for Fonts
int GetFontCount();
void GetFontLogFont(int nIndex, LOGFONT *pLogFont);

#endif // _INC_SCHEMES_H
