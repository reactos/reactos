/***************************************************************************/
/* NOTE:                                                                   */
/* This document is copyright (c) by Oz Solomonovich.  All non-commercial  */
/* use is allowed, as long as this document is not altered in any way, and */
/* due credit is given.                                                    */
/***************************************************************************/

// ShellContextMenu.h : header file
//

#if !defined(AFX_SHELLCONTEXTMENU_H__24BAC666_2B03_11D3_B9C1_0000861DFCE7__INCLUDED_)
#define AFX_SHELLCONTEXTMENU_H__24BAC666_2B03_11D3_B9C1_0000861DFCE7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#define USESTL

#if defined USESTL
#include <vector>
#include <algorithm>
using namespace std;
typedef vector<CMenu*> vecODMenu;
#define STL_FOR_ITERATOR(t,v) for(t::iterator it=v.begin();it != v.end();it++)
#define STL_ERASE_ALL(v) v.erase(v.begin(),v.end())
#define STL_GET_CURRENT(v) *it
#define STL_ADD_ITEM(v,i) v.push_back(i);
#define STL_EMPTY(v) v.empty()
#define STL_SORT(v,f,c) TRACE0("Sorting with STL\n"); is_less_than_pidl::psf = f; sort(v.begin(),v.end(),c);
#else
#include <afxtempl.h>
typedef CArray<CMenu*,CMenu*> vecODMenu;
#define STL_FOR_ITERATOR(t,v) for (int i=0;i < v.GetSize();i++)
#define STL_ERASE_ALL(v) v.RemoveAll()
#define STL_GET_CURRENT(v) v.GetAt(i) 
#define STL_ADD_ITEM(v,i) v.Add(i);
#define STL_EMPTY(v) v.GetSize() == 0
#define STL_SORT(v,f,c) TRACE0("Sorting with MFC\n"); is_less_than_pidl::psf = f; qsort(v.GetData(),v.GetSize(),sizeof(void*),c);
#endif

#include "PIDL.h"

class CShellContextMenu
{
    LPCONTEXTMENU           m_lpcm;
	LPSHELLFOLDER			m_psfParent;
	LPITEMIDLIST			m_pidl;
	LPITEMIDLIST			*m_ppidl;
	UINT					m_cidl;
	CString					m_sAbsPath;
    HWND                    m_hWnd;
    CMenu *                 m_pSendToMenu;
	vecODMenu               m_OwnerDrawMenus;
	CSize					m_szOldButtonSize;
public:    
    CShellContextMenu(HWND m_hWnd, const CString &sAbsPath, LPITEMIDLIST *ppidl, UINT cidl, LPSHELLFOLDER psfParent);
    ~CShellContextMenu();

    bool IsMenuCommand(int iCmd) const;
    void InvokeCommand(int iCmd) const;

    void CShellContextMenu::SetMenu(CMenu *pMenu);

protected:    
	void AddKey(CString &sDestKey,const CString &sSrcKey) const;
	CString GetExt(const CString &sPath) const;
	void GetAppDetails(const CString &sAppName,CString &sDisplayName,CString &sCommand,HICON &hAppIcon) const;
    void CShellContextMenu::FillSendToMenu(CMenu *pMenu, 
        LPSHELLFOLDER pSF, UINT &idm);
    void FillOpenWithMenu(CMenu *pMenu,const CString &sExt); 
};

/////////////////////////////////////////////////////////////////////////////
// Addition: Philip Oldaker
// Maybe overkill to sort the SendTo menu but could be useful
/////////////////////////////////////////////////////////////////////////////
class CShCMSort
{
public:
    CShCMSort(UINT nID,LPITEMIDLIST pidl,HICON hIcon,const CString &sText,DWORD dwItemData);
    CShCMSort();
    ~CShCMSort();
    CShCMSort(const CShCMSort &rOther);
    const CShCMSort &operator=(const CShCMSort &rOther);
// Properties
	void SetIndent(int nIndent);
	int GetIndent() const;
public:
    void SetImage(int nImage);
    int GetImage() const;
    void SetSelImage(int nSelImage);
    int GetSelImage() const;
    void SetOverlayImage(int nOverlayImage);
    int GetOverlayImage() const;
    void SetPidl(LPITEMIDLIST pidl);
    const LPITEMIDLIST GetPidl() const;
    void SetIcon(HICON hIcon);
    HICON GetIcon() const;
    void SetItemID(UINT nID);
    UINT GetItemID() const;
    void SetText(const CString &sText);
    const CString &GetText() const;
    void SetItemData(DWORD dwItemData);
    DWORD GetItemData() const;
private:
    // relative pidl
    LPITEMIDLIST    m_pidl;   
    int             m_nIndent;
    int             m_nImage;
    int             m_nSelImage;
    int             m_nOverlayImage;
    HICON           m_hIcon;
    UINT            m_ID;
    CString         m_sText;
    DWORD           m_dwItemData;       
};

inline void CShCMSort::SetIndent(int nIndent)
{
	m_nIndent = nIndent;
}

inline int CShCMSort::GetIndent() const
{
	return m_nIndent;
}

inline void CShCMSort::SetImage(int nImage)
{
	m_nImage = nImage;
}

inline int CShCMSort::GetImage() const
{
	return m_nImage;
}

inline void CShCMSort::SetSelImage(int nSelImage)
{
	m_nSelImage = nSelImage;
}

inline int CShCMSort::GetSelImage() const
{
	return m_nSelImage;
}

inline void CShCMSort::SetOverlayImage(int nOverlayImage)
{
	m_nOverlayImage = nOverlayImage;
}

inline int CShCMSort::GetOverlayImage() const
{
	return m_nOverlayImage;
}

inline CShCMSort::CShCMSort()   
{
    m_ID = 0;
    m_dwItemData = 0;
    m_pidl = NULL;
    m_hIcon = NULL;
	m_nIndent = 0;
	m_nImage = 0;
	m_nSelImage = 0;
	m_nOverlayImage = 0;
}

inline CShCMSort::CShCMSort(UINT nID,LPITEMIDLIST pidl,HICON hIcon,const CString &sText,DWORD dwItemData)
: 	m_ID(nID),
	m_pidl(pidl),
	m_hIcon(hIcon),
	m_sText(sText),
	m_dwItemData(dwItemData)
{
}

inline CShCMSort::~CShCMSort()
{

}


inline CShCMSort::CShCMSort(const CShCMSort &rOther)
 :  m_pidl(rOther.m_pidl),
    m_hIcon(rOther.m_hIcon),
    m_ID(rOther.m_ID),
    m_sText(rOther.m_sText),
    m_dwItemData(rOther.m_dwItemData),
	m_nIndent(rOther.m_nIndent),
	m_nImage(rOther.m_nImage),
	m_nSelImage(rOther.m_nSelImage),
	m_nOverlayImage(rOther.m_nOverlayImage)
{
}

inline const CShCMSort &CShCMSort::operator=(const CShCMSort &rOther)
{
    if (this == &rOther)
        return *this;

    m_pidl = rOther.m_pidl;
    m_hIcon = rOther.m_hIcon;
    m_ID = rOther.m_ID;
    m_sText = rOther.m_sText;
    m_dwItemData = rOther.m_dwItemData;
	m_nIndent = rOther.m_nIndent;
	m_nImage = rOther.m_nImage;
	m_nSelImage = rOther.m_nSelImage;
	m_nOverlayImage = rOther.m_nOverlayImage;

    return *this;
}

inline void CShCMSort::SetPidl(LPITEMIDLIST	pidl)
{
	m_pidl = pidl;
}

inline const LPITEMIDLIST CShCMSort::GetPidl() const
{
	return m_pidl;
}

inline void CShCMSort::SetIcon(HICON hIcon)
{
	m_hIcon = hIcon;
}

inline HICON CShCMSort::GetIcon() const
{
	return m_hIcon;
}

inline void CShCMSort::SetItemID(UINT nID)
{
	m_ID = nID;
}

inline UINT CShCMSort::GetItemID() const
{
	return m_ID;
}

inline void CShCMSort::SetText(const CString &sText)
{
	m_sText = sText;
}

inline const CString &CShCMSort::GetText() const
{
	return m_sText;
}

inline void CShCMSort::SetItemData(DWORD dwItemData)
{
	m_dwItemData = dwItemData;
}

inline DWORD CShCMSort::GetItemData() const
{
	return m_dwItemData;
}

// STL predicate helpers

// sort by pidl
class is_less_than_pidl
{                           
public:
	is_less_than_pidl() {}
	// STL
	bool operator()(const CShCMSort *x,const CShCMSort *y)
	{
		return ComparePidls(x,y) < 0;
	}
	int ComparePidls(const CShCMSort *x,const CShCMSort *y)
	{
		ASSERT(is_less_than_pidl::psf);
		HRESULT hr = is_less_than_pidl::psf->CompareIDs(0, x->GetPidl(), y->GetPidl());
		if (FAILED(hr))
			return 0;
		return (short)hr;
	}
	// MFC
	static int compare(const void *x, const void *y)
	{
		const CShCMSort *x1 = *(const CShCMSort**)x;
		const CShCMSort *y1 = *(const CShCMSort**)y;
		return is_less_than_pidl().ComparePidls(x1,y1);
	}
public:
	CTRL_EXT_CLASS static LPSHELLFOLDER psf;
};

class is_greater_than_pidl
{                           
public:
	is_greater_than_pidl() { }
	// STL
	bool operator()(const CShCMSort *x,const CShCMSort *y)
	{
		return ComparePidls(x,y) > 0;
	}
	int ComparePidls(const CShCMSort *x,const CShCMSort *y)
	{
		HRESULT hr = is_greater_than_pidl::psf->CompareIDs(0, x->GetPidl(), y->GetPidl());
		if (FAILED (hr))
			return 0;
		return (short)hr;
	}
	// MFC
	static int compare(const void *x, const void *y)
	{
		const CShCMSort *x1 = *(const CShCMSort**)x;
		const CShCMSort *y1 = *(const CShCMSort**)y;
		return is_greater_than_pidl().ComparePidls(x1,y1);
	}
public:
	CTRL_EXT_CLASS static LPSHELLFOLDER psf;
};

// sort by text
class is_less_than_text
{                           
public:
	// STL
	bool operator()(const CShCMSort *x,const CShCMSort *y)
	{
		return CompareText(x,y) < 0;
	}
	int CompareText(const CShCMSort *x,const CShCMSort *y)
	{
		return x->GetText() < y->GetText();
	}
	// MFC
	static int compare(const void *x, const void *y)
	{
		const CShCMSort *x1 = *(const CShCMSort**)x;
		const CShCMSort *y1 = *(const CShCMSort**)y;
		return is_less_than_text().CompareText(x1,y1);
	}
};

class is_greater_than_text
{                           
public:
	// STL
	bool operator()(const CShCMSort *x,const CShCMSort *y)
	{
		return CompareText(x,y) < 0;
	}
	int CompareText(const CShCMSort *x,const CShCMSort *y)
	{
		return x->GetText() > y->GetText();
	}
	// MFC
	static int compare(const void *x, const void *y)
	{
		const CShCMSort *x1 = *(const CShCMSort**)x;
		const CShCMSort *y1 = *(const CShCMSort**)y;
		return is_greater_than_text().CompareText(x1,y1);
	}
};

#if defined USESTL
typedef vector<CShCMSort*> vecCMSort;
#define STL_SORT_FUNC is_less_than_pidl()
#else
typedef CArray<CShCMSort*,CShCMSort*> vecCMSort;
#define STL_SORT_FUNC is_less_than_pidl::compare
#endif

/////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHELLCONTEXTMENU_H__24BAC666_2B03_11D3_B9C1_0000861DFCE7__INCLUDED_)
