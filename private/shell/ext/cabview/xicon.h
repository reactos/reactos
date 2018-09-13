//*******************************************************************************************
//
// Filename : XIcon.h
//	
//				defines for  CImageList and CXIcon  
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************


#ifndef _XICON_H_
#define _XICON_H_

class CImageList
{
public:
	CImageList() : m_himl(NULL) {}
	~CImageList() {if (m_himl) ImageList_Destroy(m_himl);}

	operator HIMAGELIST() {return(m_himl);}

	BOOL Create(int cx, int cy, int cGrow)
	{
		m_himl = ImageList_Create(cx, cy, ILC_MASK, cGrow, cGrow);
		return(m_himl != NULL);
	}

private:
	HIMAGELIST m_himl;
} ;

class CXIcon
{
public:
	CXIcon() {}
	~CXIcon() {}

	BOOL Init(HWND hwndLB, UINT idiDef);
	CImageList& GetIML(BOOL bLarge) {return(bLarge ? m_cimlLg : m_cimlSm);}

	enum
	{
		AI_LARGE = 0x0001,
		AI_SMALL = 0x0002,
	} ;

	int AddIcon(HICON hIcon, UINT uFlags=AI_LARGE|AI_SMALL)
	{
		int i = -1;

		if (uFlags & AI_LARGE)
		{
			i = ImageList_AddIcon(m_cimlLg, hIcon);
		}

		if (uFlags & AI_SMALL)
		{
			i = ImageList_AddIcon(m_cimlSm, hIcon);
		}

		return(i);
	}

	int GetIcon(IShellFolder *psf, LPCITEMIDLIST pidl);

private:
	int GetCachedIndex(LPCTSTR szIconFile, int iIndex, UINT wFlags);
	int CacheIcons(HICON hiconLarge, HICON hiconSmall,
		LPCTSTR szIconFile, int iIndex, UINT wFlags);
	static HRESULT ExtractIcon(LPCTSTR szIconFile, int iIndex, UINT wFlags,
		HICON *phiconLarge, HICON *phiconSmall, DWORD dwSizes);
	static UINT GetIDString(LPTSTR pszIDString, UINT uIDLen,
		LPCTSTR szIconFile, int iIndex, UINT wFlags);


	HWND m_hwndLB;

	CImageList m_cimlLg;
	CImageList m_cimlSm;
} ;

#endif // _XICON_H_
