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

#include "stdafx.h"
#include "IEShellDragDrop.h"
#include "cbformats.h"

#ifndef OleStdGetDropEffect
#define OleStdGetDropEffect(grfKeyState)    \
   ( (grfKeyState & MK_CONTROL) ?          \
      ( (grfKeyState & MK_SHIFT) ? DROPEFFECT_LINK : DROPEFFECT_COPY ) :  \
      ( (grfKeyState & MK_SHIFT) ? DROPEFFECT_MOVE : DROPEFFECT_NONE ) )
#endif

bool CIEShellDragDrop::DragEnter(CDD_OleDropTargetInfo *pInfo,LPSHELLFOLDER psfFolder,LPITEMIDLIST pidl)
{
//	DWORD dwEffect = m_ShellPidl.GetDragDropAttributes(pInfo->GetDataObject()->m_lpDataObject);
	IDropTarget *pdt=NULL;
	if (psfFolder == NULL)
		psfFolder = m_psfDesktop;
    HRESULT hr = psfFolder->GetUIObjectOf(pInfo->GetSafeHwnd(),1,(LPCITEMIDLIST*)&pidl, IID_IDropTarget, NULL, (LPVOID*)&pdt);
	if (FAILED(hr))
		return false;
	DWORD dwKeyState = pInfo->GetKeyboardState();
	CPoint pt(pInfo->GetPoint());
	::ClientToScreen(pInfo->GetSafeHwnd(),&pt);
	POINTL ptl;
	ptl.x = pt.x;
	ptl.y = pt.y;
	DWORD dwEffect=0;
    pdt->DragEnter(pInfo->GetDataObject()->m_lpDataObject, dwKeyState, ptl, &dwEffect);
	pdt->DragLeave();
	pInfo->SetDropEffect(dwEffect);
	pdt->Release();
	return true;
}

bool CIEShellDragDrop::DragLeave(CDD_OleDropTargetInfo *pInfo)
{
	return false;
}

bool CIEShellDragDrop::DragOver(CDD_OleDropTargetInfo *pInfo,LPSHELLFOLDER psfFolder,LPITEMIDLIST pidl)
{
	pInfo->SetDropEffect(DROPEFFECT_NONE);

	IDropTarget *pdt=NULL;
	if (psfFolder == NULL)
		psfFolder = m_psfDesktop;
    HRESULT hr = psfFolder->GetUIObjectOf(pInfo->GetSafeHwnd(),1,(LPCITEMIDLIST*)&pidl, IID_IDropTarget, NULL, (LPVOID*)&pdt);
	if (FAILED(hr))
		return false;

	DWORD dwKeyState = pInfo->GetKeyboardState();
	TRACE1("dwKeyState=%u\n",dwKeyState);
	CPoint pt(pInfo->GetPoint());
	::ClientToScreen(pInfo->GetSafeHwnd(),&pt);
	POINTL ptl;
	ptl.x = pt.x;
	ptl.y = pt.y;	
	DWORD dwEffect=OleStdGetDropEffect(dwKeyState);
	if (dwEffect == DROPEFFECT_NONE)
		dwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;
	TRACE1("dropEffect=%u\n",dwEffect);
	pdt->DragEnter(pInfo->GetDataObject()->m_lpDataObject, dwKeyState, ptl, &dwEffect);
	pdt->DragOver(dwKeyState, ptl, &dwEffect);
	pdt->DragLeave();
	pdt->Release();
	pInfo->SetDropEffect(dwEffect);
	return true;
}

bool CIEShellDragDrop::DragDrop(CDD_OleDropTargetInfo *pInfo,LPSHELLFOLDER psfFolder,LPITEMIDLIST pidl)
{
	IDropTarget *pdt=NULL;
	if (psfFolder == NULL)
		psfFolder = m_psfDesktop;
    HRESULT hr = psfFolder->GetUIObjectOf(pInfo->GetSafeHwnd(),1,(LPCITEMIDLIST*)&pidl, IID_IDropTarget, NULL, (LPVOID*)&pdt);
	if (FAILED(hr))
		return false;
	CPoint pt(pInfo->GetPoint());
	TRACE2("Drop received at point(client) %d,%d\n",pt.x,pt.y);
	::ClientToScreen(pInfo->GetSafeHwnd(),&pt);
	TRACE2("Drop received at point(screen) %d,%d\n",pt.x,pt.y);
	CWDClipboardData::Instance()->IsDataAvailable(pInfo->GetDataObject());
#ifdef _DEBUG
	if (pInfo->GetDataObject()->IsDataAvailable(CF_TEXT))
	{
		CCF_String cfString;
		if (CWDClipboardData::Instance()->GetData(pInfo->GetDataObject(),&cfString,CWDClipboardData::e_cfString))
		{
			TRACE1("Received CF_TEXT %s\n",cfString.GetString());
		}
	}
	if (pInfo->GetDataObject()->IsDataAvailable(CF_HDROP))
	{
		CCF_HDROP cfHDROP;
		if (CWDClipboardData::Instance()->GetData(pInfo->GetDataObject(),&cfHDROP,CWDClipboardData::e_cfHDROP))
		{
			for(UINT i=0;i < cfHDROP.GetCount();i++)
				TRACE2("Received CF_HDROP %s (%u)\n",cfHDROP.GetFileName(i),i);
		}
	}
#endif
	POINTL ptl;
	ptl.x = pt.x;
	ptl.y = pt.y;
	DWORD dwKeyState = pInfo->GetKeyboardState();
    DWORD dw = DROPEFFECT_MOVE;
    pdt->DragEnter(pInfo->GetDataObject()->m_lpDataObject, dwKeyState, ptl, &dw);
    pdt->DragOver(dwKeyState, ptl, &dw);
	DWORD dwEffect = m_ShellPidl.GetDragDropAttributes(pInfo->GetDataObject());
	TRACE2("Keyboardstate=%u DragDropEffect=%u\n",dwKeyState,dwEffect);
    pdt->Drop(pInfo->GetDataObject()->m_lpDataObject, dwKeyState, ptl, &dwEffect);
	pdt->Release();
	return true;
}
