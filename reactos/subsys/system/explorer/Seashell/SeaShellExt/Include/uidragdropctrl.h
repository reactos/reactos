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

#if !defined(AFX_DRAGDROPCTRL_H__BC21D3A2_332C_11D1_ADE9_0000E81B9EF1__INCLUDED_)
#define AFX_DRAGDROPCTRL_H__BC21D3A2_332C_11D1_ADE9_0000E81B9EF1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DragDropCtrl.h : header file
//
class CDD_OleDropTargetInfo : public CObject
{
	DECLARE_DYNAMIC(CDD_OleDropTargetInfo)
public:
	CDD_OleDropTargetInfo(HWND hWnd,CPoint &point,COleDataObject *pDataObject);
	CDD_OleDropTargetInfo(HWND hWnd);
// Operations
	void SetItem(HTREEITEM hItem);
	void SetItem(int nItem);
	void SetDropEffect(DROPEFFECT dropEffect);
	void SetKeyboardState(DWORD dwKeyboardState);
// Attributes
	HWND GetSafeHwnd() const;
	CPoint GetPoint() const;
	DWORD GetKeyboardState() const;
	const COleDataObject *GetDataObject() const;
	COleDataObject *GetDataObject();
	HTREEITEM GetTreeItem() const;
	int GetListItem() const;
	DROPEFFECT GetDropEffect() const;
protected:
	void InitInfo(HWND hWnd,
					const CPoint *pPoint=NULL,
					COleDataObject *pDataObject=NULL,
					DROPEFFECT dropeEffect=DROPEFFECT_NONE,
					DWORD dwKeyState=0,
					HTREEITEM hItem=NULL,
					int iItem=-1);
private:
	HWND m_hWnd; // window mouse currently over
	CPoint m_point; // mouse position in client coordinates
	DROPEFFECT m_dropEffect; // The type of drop anticipated
	COleDataObject *m_pDataObject; // The OLE data that was cached at startup
	DWORD m_dwKeyState; // keyboard state when message was received
	HTREEITEM m_hDropItem; // tree item the mouse is over or currently selected (NULL if source not a tree control)
	int m_nDropItem; // list control item the mouse is over (-1 if source not a list control)
};

inline CDD_OleDropTargetInfo::CDD_OleDropTargetInfo(HWND hWnd,CPoint &point,COleDataObject *pDataObject)
{
	InitInfo(hWnd,&point,pDataObject);
}

inline CDD_OleDropTargetInfo::CDD_OleDropTargetInfo(HWND hWnd)
{
	InitInfo(hWnd);
}

inline void CDD_OleDropTargetInfo::InitInfo(HWND hWnd,
											const CPoint *pPoint,
											COleDataObject *pDataObject,
											DROPEFFECT dropEffect,
											DWORD dwKeyState,
											HTREEITEM hItem,
											int iItem)
{
	m_hWnd = hWnd;
	if (pPoint)
		m_point = *pPoint;
	else
		m_point = CPoint();
	m_pDataObject = pDataObject;
	m_dropEffect = dropEffect;
	m_dwKeyState = dwKeyState;
	m_hDropItem = hItem;
	m_nDropItem = iItem;
}

inline void CDD_OleDropTargetInfo::SetItem(int nItem)
{
	m_nDropItem = nItem;
}

inline void CDD_OleDropTargetInfo::SetItem(HTREEITEM hItem)
{
	m_hDropItem = hItem;
}

inline void CDD_OleDropTargetInfo::SetDropEffect(DROPEFFECT dropEffect)
{
	m_dropEffect = dropEffect;
}

inline void CDD_OleDropTargetInfo::SetKeyboardState(DWORD dwKeyboardState)
{
	m_dwKeyState = dwKeyboardState;
}

inline DROPEFFECT CDD_OleDropTargetInfo::GetDropEffect() const
{
	return m_dropEffect;
}

inline HWND CDD_OleDropTargetInfo::GetSafeHwnd() const
{
	return m_hWnd;
}

inline CPoint CDD_OleDropTargetInfo::GetPoint() const
{
	return m_point;
}

inline DWORD CDD_OleDropTargetInfo::GetKeyboardState() const
{
	return m_dwKeyState;
}

inline const COleDataObject *CDD_OleDropTargetInfo::GetDataObject() const
{
	return m_pDataObject;
}

inline COleDataObject *CDD_OleDropTargetInfo::GetDataObject()
{
	return m_pDataObject;
}

inline HTREEITEM CDD_OleDropTargetInfo::GetTreeItem() const
{
	return m_hDropItem;
}

inline int CDD_OleDropTargetInfo::GetListItem() const
{
	return m_nDropItem;
}

/////////////////////////////////////////////////////////////////////////////
// CDragDropCtrl window

class CTRL_EXT_CLASS CDragDropCtrl : public COleDropTarget
{
// Construction
public:
	CDragDropCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	virtual DROPEFFECT OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point );
	virtual DROPEFFECT OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point );
	virtual BOOL OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point );
	virtual void OnDragLeave( CWnd* pWnd);

// Implementation
public:
	virtual ~CDragDropCtrl();

private:
	DWORD m_dwKeyboardState;
	DWORD m_dwEnterKeyboardState;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DRAGDROPCTRL_H__BC21D3A2_332C_11D1_ADE9_0000E81B9EF1__INCLUDED_)
