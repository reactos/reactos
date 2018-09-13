/*********************************************************************
Registration Wizard
CBitmap.h

11/14/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation
**********************************************************************/
#ifndef __CBitmap__
#define __CBitmap__


class CBitmap
{
public:
	CBitmap(HINSTANCE hInstance, HWND hwndDlg,int idDlgCtl, int idBitmap);
	virtual ~CBitmap();
	LRESULT PASCAL CtlWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	HBITMAP   GetBmp();
	HINSTANCE m_hInstance;
	FARPROC m_lpfnOrigWndProc;
	HBITMAP m_hBitmap;
	int		m_nIdBitmap;
	HPALETTE m_hPal;
	BOOL    m_isActivePal;
	
};
	
#endif
