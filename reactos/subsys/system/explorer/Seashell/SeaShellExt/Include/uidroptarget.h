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

#if !defined(AFX_DRAGDROPLIST_H__A4325D31_FF52_11D0_ADBF_0000E81B9EF1__INCLUDED_)
#define AFX_DRAGDROPLIST_H__A4325D31_FF52_11D0_ADBF_0000E81B9EF1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DragDropList.h : header file
//

#include "UIImageDropTarget.h"

class CListDropTarget : public CUI_ImageDropTarget
{
public:
	CListDropTarget();

	virtual DROPEFFECT OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point );
	virtual DROPEFFECT OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point );
	virtual BOOL OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point );
	virtual void OnDragLeave( CWnd* pWnd);
protected:
	DWORD m_dwKeyboardState;
};

inline CListDropTarget::CListDropTarget() 
{
	m_dwKeyboardState = 0;
}


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DRAGDROPLIST_H__A4325D31_FF52_11D0_ADBF_0000E81B9EF1__INCLUDED_)
