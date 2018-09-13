/*********************************************************************
Registration Wizard
CStaticText.h

11/14/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation
**********************************************************************/
#ifndef __CStaticText__
#define __CStaticText__
#include <tchar.h>

class CStaticText
{
public:
	CStaticText(HINSTANCE hInstance, HWND hwndDlg,int idControl,int idString1,int idString2);
	virtual ~CStaticText();

	LRESULT PASCAL CtlWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LPTSTR LoadExtendedString(HINSTANCE hInstance,int idString1,int idString2);

private:
	HINSTANCE m_hInstance;
	LPTSTR m_szText;
	FARPROC m_lpfnOrigWndProc;
	HFONT m_hFont;
};
	
#endif
