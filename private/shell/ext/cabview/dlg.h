//*******************************************************************************************
//
// Filename : Dlg.h
//	
//				Definitions of CDlg, CFileDlg and CPropPage
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#ifndef _Dlg_H_
#define _Dlg_H_

class CDlg
{
public:
	CDlg() {}
	~CDlg() {}

	int DoModal(UINT idRes, HWND hParent);
	HWND DoModeless(UINT idRes, HWND hParent);

protected:
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HWND m_hDlg;

private:
	virtual INT_PTR RealDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
} ;

#ifdef UNICODE
#define tagOFN tagOFNW
#else  // UNICODE
#define tagOFN tagOFNA
#endif // UNICODE

class CFileDlg : public tagOFN
{
public:
	CFileDlg(HWND hwndParent, LPCTSTR szFilter, LPTSTR szFile, UINT uFileLen, LPCTSTR szTitle);
	~CFileDlg() {}

	DWORD GetDlgError() {return(m_dwError);}

protected:
	DWORD m_dwError;
	HWND m_hDlg;

	static BOOL CALLBACK HookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	virtual BOOL RealHookProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
} ;

class CFileOpenDlg : public CFileDlg
{
public:
	CFileOpenDlg(HWND hwndParent, LPCTSTR szFilter, LPTSTR szFile, UINT uFileLen, LPCTSTR szTitle)
	: CFileDlg(hwndParent, szFilter, szFile, uFileLen, szTitle) {}
	~CFileOpenDlg() {}

	BOOL DoModal();
} ;


class CPropPage : public PROPSHEETPAGE
{
public:
	CPropPage(LPCTSTR szTmplt);
	virtual ~CPropPage();

	HRESULT DoModeless(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

protected:
	static INT_PTR CALLBACK PageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HWND m_hPage;

private:
	static UINT CALLBACK PageRelease(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

	virtual INT_PTR RealPageProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

	CPropPage *m_pThis;
} ;

#endif // _Dlg_H_
