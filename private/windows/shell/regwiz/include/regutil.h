/*********************************************************************
Registration Wizard
regutil.h

12/7/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation
*********************************************************************/
#include <tchar.h>
#define  RWZ_MAKE_BOLD 1
void InitDlgNormalBoldFont();
void DeleteDlgNormalBoldFont();
HFONT NormalizeDlgItemFont(HWND hwndDlg,int idControl, int iMakeBold=0);
void ReplaceDialogText(HWND hwndDlg,int idControl,LPTSTR szText);
BOOL ValidateInvDialog(HWND hwndDlg,int iStrID);
void GetEditFieldProperties(HWND hwndDlg,int stringID,short* maxLen,BOOL* isRequired);
void UpgradeDlg(HWND hwndDlg);
void LoadAndCombineString(HINSTANCE hInstance,LPCTSTR szTarget,int idReplacementString,LPTSTR szString);
void StripCharFromString(LPTSTR szInString, LPTSTR szOutString, _TCHAR charToStrip);

BOOL GetIndexedRegKeyValue(HINSTANCE hInstance, int enumIndex, LPTSTR szBaseKey,int valueStrID, LPTSTR szValue);
BOOL FileExists(LPTSTR szPathName);
void UppercaseString(LPTSTR sz);
LONG GetResNumber(HINSTANCE hInstance, int iStrResID);
BOOL Windows95OrGreater( void );
void DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, int xStart,int yStart, 
	int xWidth, int yWidth, int xSrc, int ySrc, COLORREF cTransparentColor);
BOOL GetSystemLanguageInfo(LPTSTR lpszLanguage, DWORD dwBufferSize,LANGID* lpLangId);
void GetRegWizardVersionString(HINSTANCE hIns, LPTSTR lpszVersion);
void RegWizardInfo(HWND hwndDlg);
UINT GetRegKeyValue32 ( HKEY hRootKey, LPTSTR const cszcSubKey, LPTSTR const cszcValueName,
	 PDWORD pdwType, PTBYTE pbData, UINT cbData );

BOOL FResSetDialogTabOrder(HWND hwndDlg, UINT wResStringID);
BOOL FSetDialogTabOrder(HWND hwndDlg, LPTSTR szTabOrder);

