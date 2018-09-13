// srvritem.h : interface of the CWordPadSrvrItem class
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

class CWordPadDoc;
class CWordPadView;

class CEmbeddedItem : public COleServerItem
{
	DECLARE_DYNAMIC(CEmbeddedItem)

// Constructors
public:
	CEmbeddedItem(CWordPadDoc* pContainerDoc, int nBeg = 0, int nEnd = -1);

// Attributes
	int m_nBeg;
	int m_nEnd;
	LPDATAOBJECT m_lpRichDataObj;
	CWordPadDoc* GetDocument() const
		{ return (CWordPadDoc*) COleServerItem::GetDocument(); }
	CWordPadView* GetView() const;

// Implementation
public:
	BOOL OnDrawEx(CDC* pDC, CSize& rSize, BOOL bOutput);
	virtual BOOL OnDraw(CDC* pDC, CSize& rSize);
	virtual BOOL OnGetExtent(DVASPECT dwDrawAspect, CSize& rSize);

protected:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
};


/////////////////////////////////////////////////////////////////////////////
