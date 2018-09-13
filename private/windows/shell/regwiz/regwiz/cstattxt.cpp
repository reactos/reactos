/*********************************************************************
Registration Wizard
Class: CStaticText

--- This class subclasses a Window control to create a custom static 
text control.

11/14/94 - Tracy Ferrier
04/15/97 - Modified to take care of crashing in Memphis as the default destoy was not handled 

(c) 1994-95 Microsoft Corporation
**********************************************************************/
#include <Windows.h>
#include <stdio.h>
#include "cstattxt.h"
#include "Resource.h"

LRESULT PASCAL StaticTextWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

CStaticText::CStaticText(HINSTANCE hInstance, HWND hwndDlg,int idControl,int idString1,int idString2)
/*********************************************************************
Constructor for our CStaticText class.  
**********************************************************************/
{
	m_hInstance = hInstance;
	m_szText = LoadExtendedString(hInstance,idString1,idString2);

	HWND hwndCtl = GetDlgItem(hwndDlg,idControl);
	m_lpfnOrigWndProc = (FARPROC) GetWindowLongPtr(hwndCtl,GWLP_WNDPROC);
	SetWindowLongPtr(hwndCtl,GWLP_WNDPROC,(LONG_PTR) StaticTextWndProc);
	SetWindowLongPtr(hwndCtl,GWLP_USERDATA,(LONG_PTR) this);

	HFONT hfont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0L);
	if (hfont != NULL)
	{
		LOGFONT lFont;
		if (!GetObject(hfont, sizeof(LOGFONT), (LPTSTR)&lFont))
		{
			m_hFont = NULL;
		}
		else
		{
			lFont.lfWeight = FW_NORMAL;
			hfont = CreateFontIndirect((LPLOGFONT)&lFont);
			if (hfont != NULL)
			{
				m_hFont = hfont;
			}
		}
	}

}


CStaticText::~CStaticText()
/*********************************************************************
Destructor for our CStaticText class
**********************************************************************/
{
	if (m_szText) GlobalFree(m_szText);
	if (m_hFont) DeleteObject(m_hFont);
}


LPTSTR CStaticText::LoadExtendedString(HINSTANCE hInstance,int idString1,int idString2)
/*********************************************************************
This function builds a single string out of the string resources whose 
ID's are given by the idString1 and idString2 parameters (if the
idString2 parameter is given as NULL, only the string resource 
specified by idString1 will be used.  LoadExtendedString allocates
space for the extended string on the heap, and returns a pointer to
it as the function result.
**********************************************************************/
{
	_TCHAR szTextBuffer[512];
	int resSize = LoadString(hInstance,idString1,szTextBuffer,255);
	if (idString2 != NULL)
	{
		_TCHAR szTextBuffer2[256];
		resSize = LoadString(hInstance,idString2,szTextBuffer2,255);
		_tcscat(szTextBuffer,szTextBuffer2);
	}
	HGLOBAL szReturnBuffer = GlobalAlloc(LMEM_FIXED,(_tcslen(szTextBuffer) + 1)* sizeof(_TCHAR));
	_tcscpy((LPTSTR) szReturnBuffer,szTextBuffer);
	return (LPTSTR) szReturnBuffer;
}


LRESULT PASCAL CStaticText::CtlWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
/*********************************************************************
**********************************************************************/
{
	switch (message)
	{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT wndRect;
			GetClientRect(hwnd,&wndRect);
			HDC hdc = BeginPaint(hwnd,&ps);
			SelectObject(hdc,m_hFont);
			SetBkMode(hdc,TRANSPARENT);
			DrawText(hdc,m_szText,-1,&wndRect,DT_LEFT | DT_WORDBREAK);
			EndPaint(hwnd,&ps);
			break;
		}
		case WM_GETTEXT:
		{
			LPTSTR szBuffer = (LPTSTR) lParam;
			WPARAM userBufferSize = wParam;
			_tcsncpy(szBuffer,m_szText,(size_t)userBufferSize);
			break;
		}
		case WM_SETTEXT:
		{
			if (m_szText) GlobalFree(m_szText);
			LPTSTR szBuffer = (LPTSTR) lParam;
			m_szText = (LPTSTR) GlobalAlloc(LMEM_FIXED,(_tcslen(szBuffer) + 1)* sizeof(_TCHAR));
			_tcscpy(m_szText,szBuffer);
			break;
		}
	
		case WM_DESTROY:
			SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LONG_PTR) m_lpfnOrigWndProc);

		default:
#ifdef _WIN95
		return CallWindowProc(m_lpfnOrigWndProc,hwnd,message,wParam,lParam);
#else
		return CallWindowProc((WNDPROC) m_lpfnOrigWndProc,hwnd,message,wParam,lParam);
#endif

			break;
	}
	return 0;
}


LRESULT PASCAL StaticTextWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
/*********************************************************************
**********************************************************************/
{
	CStaticText* pclStaticText = (CStaticText*) GetWindowLongPtr(hwnd,GWLP_USERDATA);
	LRESULT lret;
	switch (message)
	{
		case WM_DESTROY:
			lret = pclStaticText->CtlWndProc(hwnd,message,wParam,lParam);

			delete pclStaticText;
			return lret;
		default:
			return pclStaticText->CtlWndProc(hwnd,message,wParam,lParam);
	}
}

