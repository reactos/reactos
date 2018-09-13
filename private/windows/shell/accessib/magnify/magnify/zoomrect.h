#if !defined(AFX_ZOOMRECT_H__CCBDFADF_E296_11D0_AD57_00C04FC2A136__INCLUDED_)
#define AFX_ZOOMRECT_H__CCBDFADF_E296_11D0_AD57_00C04FC2A136__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ZoomRect.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CZoomRect window

class CZoomRect : public CWnd
{
// Construction
public:
	CZoomRect();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CZoomRect)
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetLocation(int nLeft, int nTop, int nRight, int nBottom);
	virtual ~CZoomRect();

	// Generated message map functions
protected:
	CRect m_rcOutter;
	int m_nBorderWidth;
	//{{AFX_MSG(CZoomRect)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ZOOMRECT_H__CCBDFADF_E296_11D0_AD57_00C04FC2A136__INCLUDED_)
