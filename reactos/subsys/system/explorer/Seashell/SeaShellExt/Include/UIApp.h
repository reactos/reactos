//*******************************************************************************
// COPYRIGHT NOTES
// ---------------
// You may use this source code, compile or redistribute it as part of your application 
// for free. You cannot redistribute it as a part of a software development 
// library without the agreement of the author. If the sources are 
// distributed along with the application, you should leave the original 
// copyright notes in the source code without any changes.
// This code can be used WITHOUT ANY WARRANTIES at your own risk.
// 
// For the latest updates to this code, check this site:
// http://www.masmex.com 
// after Sept 2000
// 
// Copyright(C) 2000 Philip Oldaker <email: philip@masmex.com>
//*******************************************************************************

#ifndef __UIAPP_H__
#define __UIAPP_H__

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

class CTRL_EXT_CLASS CUIApp : public CWinApp
{
	DECLARE_DYNAMIC(CUIApp)
public:
	CUIApp();

	void SetStatusBarText(UINT nPaneID,LPCTSTR Text,UINT nIconID=0);
	void SetStatusBarText(UINT nPaneID,LPCTSTR Text,CView *pView,UINT nIconID=0);
	void SetStatusBarText(UINT nPaneID,UINT nResID,UINT nIconID=0);
	void SetStatusBarText(LPCTSTR Text);
	void SetStatusBarIcon(UINT nPaneID,UINT nIconID,bool bAdd);
	void SetStatusBarIdleMessage();
	CString GetRegAppKey();

	static bool COMMessage(HRESULT hr,UINT nID);
	static void ErrorMessage(UINT nID,DWORD dwError=0);
	static bool COMMessage(HRESULT hr,LPCTSTR pszMess=NULL);
	static void ErrorMessage(LPCTSTR pszMess=NULL,DWORD dwError=0);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUIApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL
//	CMultiDocTemplate* m_pWebSitesDocTemplate;
	void ChangeProfile(LPCTSTR szRegistryKey,LPCTSTR szProfileName);
	void RestoreProfile();
// Implementation
	CFrameWnd *GetMainFrame(CWnd *pWnd);
	CView *GetView(CRuntimeClass *pClass);
	CDocTemplate *GetFirstDocTemplate();
	CDocument *GetDocument();
protected:
	bool RegisterMyClass();
	BOOL FirstInstance();
	//{{AFX_MSG(CUIApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CString m_strOldProfileName;
	CString m_strOldRegistryKey;
	CString m_sMyClassName;
	UINT m_IDMyIcon;
	bool m_bMyClassRegistered;
public:
	void SetMyClass(const CString &sMyClassName,UINT nIconID);
	LPCTSTR GetMyClass() const;
	bool IsMyClassRegistered() const;
};

inline bool CUIApp::IsMyClassRegistered() const
{
	return m_bMyClassRegistered;
}

inline void CUIApp::SetMyClass(const CString &sMyClassName,UINT nIconID)
{
	m_sMyClassName = sMyClassName;
	m_IDMyIcon = nIconID;
}

inline LPCTSTR CUIApp::GetMyClass() const
{
	return m_sMyClassName;
}

/////////////////////////////////////////////////////////////////////////////
class CTRL_EXT_CLASS CAppReg 
{
public:
	CAppReg(CWinApp *pApp,LPCTSTR szRegistryKey,LPCTSTR szProfileName);
	~CAppReg();
	CString GetProfileString( LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault = NULL );
	BOOL WriteProfileString( LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue );
private:
	CUIApp *m_pApp;
};

inline BOOL CAppReg::WriteProfileString( LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue )
{
	return m_pApp->WriteProfileString(lpszSection,lpszEntry,lpszValue);
}

inline CString CAppReg::GetProfileString( LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault)
{
	return m_pApp->GetProfileString(lpszSection,lpszEntry,lpszDefault);
}


#endif //__UIAPP_H__