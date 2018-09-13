//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1996               **
//*********************************************************************
//
// DLYLDDLL.H - globals for delay loading of DLLs
//

extern HINSTANCE g_hinstWinInet;
extern HINSTANCE g_hinstCrypt32;

extern HINSTANCE g_hinstCryptui;

extern HINSTANCE g_hinstRatings;
extern const TCHAR c_tszRatingsDLL[];

extern HINSTANCE g_hinstUrlMon;

extern HINSTANCE g_hinstMSHTML;
extern const TCHAR c_tszMSHTMLDLL[];

extern HINSTANCE g_hinstMPR;
extern const TCHAR c_tszMPRDLL[];

extern HINSTANCE g_hinstShdocvw;
extern const TCHAR c_tszShdocvw[];
#define URLSUB_ALL     0xFFFFFFFF

HRESULT URLSubRegQueryA(LPCSTR pszKey, LPCSTR pszValue, BOOL fUseHKCU, LPSTR pszUrlOut, DWORD cchSize, DWORD dwSubstitutions);

extern HINSTANCE g_hinstOCCache;
