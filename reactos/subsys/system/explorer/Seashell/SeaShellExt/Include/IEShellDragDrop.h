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

#ifndef __IESHELLDRAGDROP_H__
#define __IESHELLDRAGDROP_H__

#include "UIDragDropCtrl.h"
#include "ShellPidl.h"
////////////////////////////////////////////////
// CIEShellDragDrop
////////////////////////////////////////////////
class CIEShellDragDrop
{
public:
	CIEShellDragDrop();
	virtual ~CIEShellDragDrop();

public:
	virtual bool DragDrop(CDD_OleDropTargetInfo *pInfo,LPSHELLFOLDER psfFolder,LPITEMIDLIST pidl);
	virtual bool DragOver(CDD_OleDropTargetInfo *pInfo,LPSHELLFOLDER psfFolder,LPITEMIDLIST pidl);
	virtual bool DragEnter(CDD_OleDropTargetInfo *pInfo,LPSHELLFOLDER psfFolder,LPITEMIDLIST pidl);
	virtual bool DragLeave(CDD_OleDropTargetInfo *pInfo);
protected:
private:
	CShellPidl m_ShellPidl;
	LPSHELLFOLDER m_psfDesktop;
};

inline CIEShellDragDrop::CIEShellDragDrop()
{
	m_psfDesktop = NULL;
	SHGetDesktopFolder(&m_psfDesktop);
}

inline CIEShellDragDrop::~CIEShellDragDrop()
{
	if (m_psfDesktop)
		m_psfDesktop->Release();
}

#endif //__IESHELLDRAGDROP_H__