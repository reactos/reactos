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

// ShellPidl.h: interface for the CShellPidl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHELLPIDL_H__D704F47E_1D35_4C7C_9F90_B3071B25DCB2__INCLUDED_)
#define AFX_SHELLPIDL_H__D704F47E_1D35_4C7C_9F90_B3071B25DCB2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Refresh.h"

class CTRL_EXT_CLASS CShellPidl  
{
public:
	CShellPidl();
	virtual ~CShellPidl();
public:
	virtual BOOL PopupTheMenu(HWND hwnd, LPSHELLFOLDER lpsfParent, LPITEMIDLIST  *lpi, UINT cidl, LPPOINT lppt);
	virtual BOOL HandleMenuMsg(HWND hwnd, LPSHELLFOLDER lpsfParent,
								LPITEMIDLIST  lpi, UINT uMsg, WPARAM wParam, LPARAM lParam);
#ifdef _UNICODE
	STDMETHOD(StrRetToStr)(STRRET StrRet, LPTSTR* str, LPITEMIDLIST pidl);
#else
	STDMETHOD(StrRetToStr)(STRRET StrRet, LPTSTR* str, LPITEMIDLIST pidl);
#endif
	STDMETHOD(ResolveChannel)(IShellFolder* pFolder, LPCITEMIDLIST pidl, LPTSTR* lpszURL);
	STDMETHOD(ResolveHistoryShortcut)(LPSHELLFOLDER pFolder,LPCITEMIDLIST *ppidl,CString &sURL);
	STDMETHOD(ResolveInternetShortcut)(LPCTSTR lpszLinkFile, LPTSTR* lpszURL);
	STDMETHOD(ResolveLink)(HWND hWnd,LPCTSTR lpszLinkFile, LPTSTR* lpszPath);
	STDMETHOD(SHPathToPidlEx)(LPCTSTR szPath, LPITEMIDLIST* ppidl, LPSHELLFOLDER pFolder=NULL);
	STDMETHOD(SHPidlToPathEx)(LPCITEMIDLIST pidl, CString &sPath, LPSHELLFOLDER pFolder=NULL,DWORD dwFlags=SHGDN_FORPARSING);
	LPITEMIDLIST CopyItemIDList(LPITEMIDLIST pidl);
	LPITEMIDLIST CopyItemID(LPITEMIDLIST pidl,int n=0);
	LPITEMIDLIST CopyLastItemID(LPITEMIDLIST pidl);
	LPITEMIDLIST CopyAbsItemID(LPITEMIDLIST pidl,int n);
	LPITEMIDLIST ConcatPidl(LPITEMIDLIST pidlDest,LPITEMIDLIST pidlSrc);
	LPITEMIDLIST Next(LPCITEMIDLIST pidl);
	LPSHELLFOLDER GetFolder(LPITEMIDLIST pidl);
	LPSHELLFOLDER GetDesktopFolder();
	LPCITEMIDLIST GetEmptyPidl();
	bool IsDesktopFolder(LPSHELLFOLDER psFolder);
	DWORD GetDragDropAttributes(COleDataObject *pDataObject);
	DWORD GetDragDropAttributes(LPTVITEMDATA ptvid);
	DWORD GetDragDropAttributes(LPLVITEMDATA plvid);
	DWORD GetDragDropAttributes(LPSHELLFOLDER pFolder,LPCITEMIDLIST pidl);
	UINT GetSize(LPCITEMIDLIST pidl);
	UINT GetCount(LPCITEMIDLIST pidl);
	void GetTypeName(LPITEMIDLIST lpi,CString &sTypeName);
	void GetDisplayName(LPITEMIDLIST lpi,CString &sDisplayName);
	int GetIcon(LPITEMIDLIST lpi, UINT uFlags);
	void GetNormalAndSelectedIcons(LPITEMIDLIST lpifq, int &iImage, int &iSelectedImage);
	BOOL GetName (LPSHELLFOLDER lpsf, LPITEMIDLIST lpi, DWORD dwFlags, CString &sFriendlyName);
	bool ComparePidls(LPSHELLFOLDER pFolder,LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2);
	bool CompareMemPidls(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2);
	IMalloc *GetMalloc();
	void FreePidl(LPITEMIDLIST pidl);
	void Free(void *pv);
protected:
	STDMETHOD(GetURL)(IDataObject *pDataObj,CString &sURL);
private:
	ITEMIDLIST m_EmptyPidl;
	LPMALLOC m_pMalloc;
	LPSHELLFOLDER m_psfDesktop;
};

////////////////////////////////////////////////
// CShellPidlCompare
////////////////////////////////////////////////
class CShellPidlCompare
{
public:
	CShellPidlCompare(LPSHELLFOLDER pFolder,LPITEMIDLIST pidl);
	CShellPidlCompare(const CShellPidlCompare &rOther);
	const CShellPidlCompare &operator=(const CShellPidlCompare &rOther);
	bool operator==(const CShellPidlCompare &rhs) const;
	bool operator<(const CShellPidlCompare &rhs) const;
    virtual ~CShellPidlCompare();
	LPCITEMIDLIST GetPidl() const;
	LPSHELLFOLDER GetFolder() const;
public:
protected:
private:
    LPITEMIDLIST m_pidl;
	LPSHELLFOLDER m_pFolder;
};


inline CShellPidlCompare::CShellPidlCompare(const CShellPidlCompare &rOther)
 : 	m_pidl(rOther.m_pidl),
	m_pFolder(rOther.m_pFolder)
{
}

inline bool CShellPidlCompare::operator==(const CShellPidlCompare &rhs) const
{
	HRESULT hr = m_pFolder->CompareIDs(0,m_pidl,rhs.m_pidl);
	return (short)hr == 0;
}

inline bool CShellPidlCompare::operator<(const CShellPidlCompare &rhs) const
{
	HRESULT hr = m_pFolder->CompareIDs(0,m_pidl,rhs.m_pidl);
	return (short)hr < 0;
}

inline const CShellPidlCompare &CShellPidlCompare::operator=(const CShellPidlCompare &rOther)
{
	if (this == &rOther)
		return *this;

	m_pidl = rOther.m_pidl;
	m_pFolder = rOther.m_pFolder;

	return *this;
}

inline CShellPidlCompare::CShellPidlCompare(LPSHELLFOLDER pFolder,LPITEMIDLIST pidl)
: m_pidl(pidl),
	m_pFolder(pFolder)
{

}

inline CShellPidlCompare::~CShellPidlCompare()
{

}

inline LPCITEMIDLIST CShellPidlCompare::GetPidl() const
{
	return m_pidl;
}

inline LPSHELLFOLDER CShellPidlCompare::GetFolder() const
{
	return m_pFolder;
}

#endif // !defined(AFX_SHELLPIDL_H__D704F47E_1D35_4C7C_9F90_B3071B25DCB2__INCLUDED_)
