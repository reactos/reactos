// SeaShellView.h : interface of the CSeaShellView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SEASHELLVIEW_H__0E1DCA0F_387A_4C9E_859F_F2C0C981E5A8__INCLUDED_)
#define AFX_SEASHELLVIEW_H__0E1DCA0F_387A_4C9E_859F_F2C0C981E5A8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IEShellListView.h"

class CSeaShellView : public CIEShellListView
{
protected: // create from serialization only
	CSeaShellView();
	DECLARE_DYNCREATE(CSeaShellView)

// Attributes
public:
	CSeaShellDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSeaShellView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSeaShellView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CSeaShellView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in SeaShellView.cpp
inline CSeaShellDoc* CSeaShellView::GetDocument()
   { return (CSeaShellDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SEASHELLVIEW_H__0E1DCA0F_387A_4C9E_859F_F2C0C981E5A8__INCLUDED_)
