// cntritem.h : interface of the CNetClipCntrItem class
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

class CNetClipDoc;
class CPictView;

class CNetClipCntrItem : public CRichEditCntrItem
{
	DECLARE_SERIAL(CNetClipCntrItem)

// Constructors
public:
	CNetClipCntrItem(REOBJECT* preo = NULL, CNetClipDoc* pContainer = NULL);
		// Note: pContainer is allowed to be NULL to enable IMPLEMENT_SERIALIZE.
		//  IMPLEMENT_SERIALIZE requires the class have a constructor with
		//  zero arguments.  Normally, OLE items are constructed with a
		//  non-NULL document pointer.

// Attributes
public:
	CNetClipDoc* GetDocument()
		{ return (CNetClipDoc*)COleClientItem::GetDocument(); }
	CPictView* GetActiveView()
		{ return (CPictView*)COleClientItem::GetActiveView(); }

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetClipCntrItem)
	public:
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
