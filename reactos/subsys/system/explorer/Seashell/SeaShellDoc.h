// SeaShellDoc.h : interface of the CSeaShellDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SEASHELLDOC_H__8B4AAB8C_DC87_4F7D_9171_BCE406941600__INCLUDED_)
#define AFX_SEASHELLDOC_H__8B4AAB8C_DC87_4F7D_9171_BCE406941600__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CSeaShellDoc : public CDocument
{
protected: // create from serialization only
	CSeaShellDoc();
	DECLARE_DYNCREATE(CSeaShellDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSeaShellDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSeaShellDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CSeaShellDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SEASHELLDOC_H__8B4AAB8C_DC87_4F7D_9171_BCE406941600__INCLUDED_)
