// formatba.h : header file
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __FORMATBA_H__
#define __FORMATBA_H__

class CWordPadView;

/*
typedef struct tagNMHDR
{
    HWND  hwndFrom;
    UINT  idFrom;
    UINT  code;         // NM_ code
}   NMHDR;
*/

struct CHARHDR : public tagNMHDR
{
	CHARFORMAT cf;
	CHARHDR() {cf.cbSize = sizeof(CHARFORMAT);}
};

#define FN_SETFORMAT	0x1000
#define FN_GETFORMAT	0x1001

/////////////////////////////////////////////////////////////////////////////
// CLocalComboBox

class CLocalComboBox : public CComboBox
{
public:

//Attributes
	CPtrArray m_arrayFontDesc;
	static int m_nFontHeight;
	int m_nLimitText;
	BOOL HasFocus()
	{
		HWND hWnd = ::GetFocus();
		return (hWnd == m_hWnd || ::IsChild(m_hWnd, hWnd));
	}
	void GetTheText(CString& str);
	void SetTheText(LPCTSTR lpszText,BOOL bMatchExact = FALSE);

//Operations
	BOOL LimitText(int nMaxChars);

// Implementation
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	// Generated message map functions
	//{{AFX_MSG(CLocalComboBox)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CFontComboBox : public CLocalComboBox
{
public:
	CFontComboBox();

//Attributes
	CBitmap m_bmFontType;

//Operations
	void EnumFontFamiliesEx(CDC& dc, BYTE nCharSet = DEFAULT_CHARSET);
	void AddFont(ENUMLOGFONT* pelf, DWORD dwType, LPCTSTR lpszScript = NULL);
	void MatchFont(LPCTSTR lpszName, BYTE nCharSet);
	void EmptyContents();

	static BOOL CALLBACK AFX_EXPORT EnumFamScreenCallBack(
		ENUMLOGFONT* pelf, NEWTEXTMETRICEX* /*lpntm*/, int FontType, 
		LPVOID pThis);
	static BOOL CALLBACK AFX_EXPORT EnumFamPrinterCallBack(
		ENUMLOGFONT* pelf, NEWTEXTMETRICEX* /*lpntm*/, int FontType, 
		LPVOID pThis);
	static BOOL CALLBACK AFX_EXPORT EnumFamScreenCallBackEx(
		ENUMLOGFONTEX* pelf, NEWTEXTMETRICEX* /*lpntm*/, int FontType, 
		LPVOID pThis);
	static BOOL CALLBACK AFX_EXPORT EnumFamPrinterCallBackEx(
		ENUMLOGFONTEX* pelf, NEWTEXTMETRICEX* /*lpntm*/, int FontType, 
		LPVOID pThis);

//Overridables
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
	virtual int CompareItem(LPCOMPAREITEMSTRUCT lpCIS);
	// Generated message map functions
	//{{AFX_MSG(CFontComboBox)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CSizeComboBox : public CLocalComboBox
{
public:
	CSizeComboBox();

//Attributes
	int m_nLogVert;
	int m_nTwipsLast;
public:
	void EnumFontSizes(CDC& dc, LPCTSTR pFontName);
	static BOOL FAR PASCAL EnumSizeCallBack(LOGFONT FAR* lplf, 
		LPNEWTEXTMETRIC lpntm,int FontType, LPVOID lpv);
	void TwipsToPointString(LPTSTR lpszBuf, int nTwips);
	void SetTwipSize(int nSize);
	int GetTwipSize();
	void InsertSize(int nSize);
};

/////////////////////////////////////////////////////////////////////////////
// CFormatBar dialog
class CFormatBar : public CToolBar
{
// Construction
public:
	CFormatBar();

// Operations
public:
	void PositionCombos();
	void SyncToView();

// Attributes
public:
	CDC m_dcPrinter;
	CSize m_szBaseUnits;
	CFontComboBox m_comboFontName;
	CSizeComboBox m_comboFontSize;

// Implementation
public:
	void NotifyOwner(UINT nCode);

protected:
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	void SetCharFormat(CCharFormat& cf);

	// Generated message map functions
	//{{AFX_MSG(CFormatBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnFontNameKillFocus();
	afx_msg void OnFontSizeKillFocus();
	afx_msg void OnFontSizeDropDown();
	afx_msg void OnComboCloseUp();
	afx_msg void OnComboSetFocus();
	afx_msg LONG OnPrinterChanged(UINT, LONG); //handles registered message
	DECLARE_MESSAGE_MAP()
};

#endif
