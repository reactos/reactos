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

#if !defined(AFX_HTMLMSGVIEW_H__4A8E5045_F2AC_47AE_A24D_584CB7D9D084__INCLUDED_)
#define AFX_HTMLMSGVIEW_H__4A8E5045_F2AC_47AE_A24D_584CB7D9D084__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HtmlMsgView.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CUIHtmlView html view

interface IHTMLDocument2;
interface IHTMLElement;
interface IHTMLImgElement;
interface IHTMLObjectElement;

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif
#include <afxhtml.h>

#include <comdef.h>

class CTRL_EXT_CLASS CUIHtmlView : public CHtmlView
{
protected:
	CUIHtmlView();           // protected constructor used by dynamic creation
	DECLARE_DYNAMIC(CUIHtmlView)

// html Data
public:
	//{{AFX_DATA(CUIHtmlView)
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:
	void SetNotifyWnd(HWND hwnd);
	bool IsWorking() { return m_pHTMLDocument2 == NULL; }
	IHTMLDocument2 *GetHTMLDocument();
	void GetElement(LPCTSTR pszID,IHTMLElement **pElement);
	CString GetBodyText();
// Operations
public:
	virtual bool ExecScript(LPCTSTR pszScript,LPCTSTR pszLang=NULL,_variant_t *pvt=NULL);
	virtual bool SetElementText(LPCTSTR pszElemID,LPCTSTR pszText);
	virtual bool SetElementHTML(LPCTSTR pszElemID,LPCTSTR pszText);
	virtual bool SetElementValue(LPCTSTR pszElemID,LPCTSTR pszText);
	virtual bool SetImageSource(LPCTSTR pszElemID,LPCTSTR pszText);
	virtual bool AddOptionString(LPCTSTR pszElemID,LPCTSTR pszText,LPCTSTR pszValue,bool bSelect=false);
	virtual bool SetOptionString(LPCTSTR pszElemID,LPCTSTR pszText);
	virtual bool GetOptionString(LPCTSTR pszElemID,CString &sText,CString &sValue);
	virtual CString GetElementText(LPCTSTR pszElemID);
	virtual CString GetElementHTML(LPCTSTR pszElemID);
	virtual CString GetElementValue(LPCTSTR pszElemID);
	virtual void ParseDocument();

protected:
	virtual void DocumentReady();
	virtual void ActiveXControl(IHTMLObjectElement *pObj);
	virtual void ImageElement(IHTMLImgElement *pImg);
	virtual void Element(IHTMLElement *pElement);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUIHtmlView)
	public:
	virtual void OnBeforeNavigate2(LPCTSTR lpszURL, DWORD nFlags, LPCTSTR lpszTargetFrameName, CByteArray& baPostedData, LPCTSTR lpszHeaders, BOOL* pbCancel);
	virtual void OnCommandStateChange(long nCommand, BOOL bEnable);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void DocumentComplete(LPDISPATCH pDisp, VARIANT* URL);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	virtual void OnTitleChange(LPCTSTR lpszText);
	virtual void OnDocumentComplete(LPCTSTR lpszUrl);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void ReleaseDocument();
	virtual ~CUIHtmlView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CUIHtmlView)
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBrowserGoBack();
	afx_msg void OnBrowserGoForward();
	afx_msg void OnBrowserGoHome();
	afx_msg void OnBrowserRefresh();
	afx_msg void OnBrowserStop();
	afx_msg void OnUpdateBrowserGoBack(CCmdUI *pUI);
	afx_msg void OnUpdateBrowserGoForward(CCmdUI *pUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	IHTMLDocument2 *m_pHTMLDocument2;
	BOOL m_bDocumentComplete;
	BOOL m_bGoBack;
	BOOL m_bGoForward;
	HWND m_hNotifyWnd;
	bool m_bSetCursor;
};

inline void CUIHtmlView::SetNotifyWnd(HWND hwnd)
{
	m_hNotifyWnd = hwnd;
}

inline IHTMLDocument2 *CUIHtmlView::GetHTMLDocument()
{
	ASSERT(m_pHTMLDocument2);
	return m_pHTMLDocument2;
}
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HTMLMSGVIEW_H__4A8E5045_F2AC_47AE_A24D_584CB7D9D084__INCLUDED_)
