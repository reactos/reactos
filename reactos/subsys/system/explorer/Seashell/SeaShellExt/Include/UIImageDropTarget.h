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

#if !defined(AFX_IM_OLEIMAGEDROPTARGET_H__BC21D3A1_332C_11D1_ADE9_0000E81B9EF1__INCLUDED_)
#define AFX_IM_OLEIMAGEDROPTARGET_H__BC21D3A1_332C_11D1_ADE9_0000E81B9EF1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// IM_OleImageDropTarget.h : header file
//

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CUI_ImageDropTarget window
#include "UIDragDropCtrl.h"

class CDragDropImage;

class CUI_ImageDropTarget : public COleDropTarget
{
// Construction
public:
	CUI_ImageDropTarget();

// Attributes
public:
	inline UINT GetClipboardFormat() const;
	inline BOOL ImageSetup() const;

// Operations
public:

// Overrides
	virtual DROPEFFECT OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point );
	virtual DROPEFFECT OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point );
	virtual BOOL OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point );
	virtual void OnDragLeave( CWnd* pWnd);
// Implementation
public:
	virtual ~CUI_ImageDropTarget();
	void EraseOldImage();
	void SetImageWin(CWnd *pWnd);

protected:
	const CDragDropImage *GetDropImage();
	CWnd *GetImageWin(CWnd *pWnd);
private:
    int m_nImage;
	CRect m_rcTotalItem;
	CPoint m_ptItem;
    CPoint m_ptPrevPos;
    CPoint m_ptOldImage;
    CSize m_sizeDelta;
    CDragDropImage *m_pImage;
	UINT m_nCFFormat;
	CWnd *m_pWnd;
};

inline UINT CUI_ImageDropTarget::GetClipboardFormat() const
{
	return m_nCFFormat;
}

inline const CDragDropImage *CUI_ImageDropTarget::GetDropImage()
{
	return m_pImage;
}

inline void CUI_ImageDropTarget::SetImageWin(CWnd *pWnd)
{
	m_pWnd = pWnd;
}

inline CWnd *CUI_ImageDropTarget::GetImageWin(CWnd *pWnd) 
{
	// use main frame window as default
	if (m_pWnd == NULL)
		m_pWnd = pWnd;
	ASSERT(m_pWnd);
	return m_pWnd;
}

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IM_OLEIMAGEDROPTARGET_H__BC21D3A1_332C_11D1_ADE9_0000E81B9EF1__INCLUDED_)
