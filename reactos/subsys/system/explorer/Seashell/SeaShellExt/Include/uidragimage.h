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

#if !defined(AFX_WIDGBASE_H__877329CA_C22E_11D0_B2D8_444553540000__INCLUDED_)
#define AFX_WIDGBASE_H__877329CA_C22E_11D0_B2D8_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CDragDropImage command target
class CDragDropItem : public CObject
{
public:
	CDragDropItem(LPCRECT prcItem,LPCRECT prcIcon);
    CRect m_rcItem;
    CRect m_rcIcon;
};

inline CDragDropItem::CDragDropItem(LPCRECT prcItem,LPCRECT prcIcon) : m_rcItem(prcItem),m_rcIcon(prcIcon)
{
}

struct DD_ImageData 
{
	CRect m_rcItem;
	CRect m_rcIcon;
	CPoint m_ptDrag;
};

class CDragDropImage : public CCmdTarget
{
    DECLARE_DYNCREATE(CDragDropImage)
protected:
	CDragDropImage();// protected constructor used by dynamic creation
public:
    CDragDropImage(int nSelected,int nType);           
    virtual ~CDragDropImage();
// Attributes
public:

// Operations
public:
    void AddItem(LPCRECT prcItem,LPCRECT prcIcon);           

    virtual void DrawDragImage (CDC *pDC,POINT point,POINT ActionPoint);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDragDropImage)
    //}}AFX_VIRTUAL

// Implementation
protected:
	void DrawItemImage(CDC *pDC, POINT point,const CRect &rcItem,const CRect &rcIcon);

	int m_nSelected;
	int m_nType;
	CTypedPtrList<CObList,CDragDropItem*> m_itemList;

    // Generated message map functions
    //{{AFX_MSG(CDragDropImage)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIDGBASE_H__877329CA_C22E_11D0_B2D8_444553540000__INCLUDED_)
