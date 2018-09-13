//*******************************************************************************************
//
// Filename : XIcon.cpp
//	
//				Implementation file for CIconTemp and CXIcon
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#include "Pch.H"

#include "ThisDll.H"

#include "Unknown.H"
#include "XIcon.H"

class CIconTemp : public CObjTemp
{
public:
	CIconTemp() : CObjTemp() {}
	CIconTemp(HICON hi) : CObjTemp() {Attach(hi);}
	~CIconTemp() {if (m_hObj) DestroyIcon(Detach());}

	operator HICON() const {return((HICON)m_hObj);}

	HICON Attach(HICON hObjNew) {return((HICON)CObjTemp::Attach((HANDLE)hObjNew));}
	HICON Detach() {return((HICON)CObjTemp::Detach());}
} ;


BOOL CXIcon::Init(HWND hwndLB, UINT idiDef)
{
	m_hwndLB = hwndLB;

	m_cimlLg.Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 8);
	m_cimlSm.Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 8);

	HICON hIcon = LoadIcon(g_ThisDll.GetInstance(), MAKEINTRESOURCE(idiDef));

	return(m_cimlLg && m_cimlSm && AddIcon(hIcon)>=0);
}


UINT CXIcon::GetIDString(LPTSTR pszIDString, UINT uIDLen,
	LPCTSTR szIconFile, int iIndex, UINT wFlags)
{
	static const TCHAR szIDFmt[] = TEXT("\"%s\" %d %08x");
	TCHAR szIDString[MAX_PATH + 2*ARRAYSIZE(szIDFmt)];

	int iRet = wsprintf(szIDString, szIDFmt, szIconFile, iIndex, wFlags);
	lstrcpyn(pszIDString, szIDString, uIDLen);

	return(iRet + 1);
}


int CXIcon::GetCachedIndex(LPCTSTR szIconFile, int iIndex, UINT wFlags)
{
	TCHAR szIDString[MAX_PATH*2];
	GetIDString(szIDString, ARRAYSIZE(szIDString), szIconFile, iIndex, wFlags);
	int iLB = ListBox_FindStringExact(m_hwndLB, -1, szIDString);

	if (iLB < 0)
	{
		return(-1);
	}

	return((int)ListBox_GetItemData(m_hwndLB, iLB));
}


int CXIcon::CacheIcons(HICON hiLarge, HICON hiSmall,
	LPCTSTR szIconFile, int iIndex, UINT wFlags)
{
	CIconTemp ciLarge(hiLarge);
	CIconTemp ciSmall(hiSmall);

	if (!hiLarge || !hiSmall)
	{
		return(-1);
	}

	TCHAR szIDString[MAX_PATH*2];
	GetIDString(szIDString, ARRAYSIZE(szIDString), szIconFile, iIndex, wFlags);
	int iLB = ListBox_AddString(m_hwndLB, szIDString);

	if (iLB < 0)
	{
		return(-1);
	}

	int iLarge = AddIcon(ciLarge, AI_LARGE);
	if (iLarge >= 0)
	{
		int iSmall = AddIcon(ciSmall, AI_SMALL);
		if (iSmall >= 0)
		{
			if (iLarge == iSmall)
			{
				// Should always happen;
				ListBox_SetItemData(m_hwndLB, iLB, iLarge);
				return(iLarge);
			}

			ImageList_Remove(m_cimlSm, iSmall);
		}

		ImageList_Remove(m_cimlLg, iLarge);
	}


	ListBox_DeleteString(m_hwndLB, iLB);
	return(-1);
}


HRESULT CXIcon::ExtractIcon(LPCTSTR szIconFile, int iIndex, UINT wFlags,
	HICON *phiconLarge, HICON *phiconSmall, DWORD dwSizes)
{
	return(E_NOTIMPL);
}


int CXIcon::GetIcon(IShellFolder *psf, LPCITEMIDLIST pidl)
{
	int iImage = -1;

	IExtractIcon *pxi;
	HRESULT hres = psf->GetUIObjectOf(NULL, 1, &pidl, IID_IExtractIcon, NULL, (LPVOID *)&pxi);

	if (FAILED(hres))
	{
		return(0);
	}
	CEnsureRelease erExtractIcon(pxi);

    TCHAR szIconFile[MAX_PATH];
    int iIndex;
    UINT wFlags = 0;

    hres = pxi->GetIconLocation(GIL_FORSHELL,
		szIconFile, ARRAYSIZE(szIconFile), &iIndex, &wFlags);
    if (FAILED(hres))
    {
		return(0);
	}

    //
    // if GIL_DONTCACHE was returned by the icon handler, dont
    // lookup the previous icon, assume a cache miss.
    //
    if (!(wFlags & GIL_DONTCACHE))
	{
        iImage = GetCachedIndex(szIconFile, iIndex, wFlags);
	}

    // if we miss our cache...
    if (iImage == -1)
    {
		HICON hiconLarge = NULL;
		HICON hiconSmall = NULL;

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cxSmIcon = GetSystemMetrics(SM_CXSMICON);

        // try getting it from the Extract member fuction
        hres = pxi->Extract(szIconFile, iIndex, &hiconLarge, &hiconSmall,
        	MAKELONG(cxIcon, cxSmIcon));

		// S_FALSE means can you please do it
		if (hres == S_FALSE)
		{
			hres = ExtractIcon(szIconFile, iIndex, wFlags,
				&hiconLarge, &hiconSmall, MAKELONG(cxIcon, cxSmIcon));
		}
		if (SUCCEEDED(hres))
		{
			// Let CacheIcons check and destroy the hicon's
			iImage = CacheIcons(hiconLarge, hiconSmall, szIconFile, iIndex, wFlags);
		}

		// if we failed in any way pick a default icon
        if (iImage == -1)
        {
            if (wFlags & GIL_SIMULATEDOC)
			{
                iImage = 0; // II_DOCUMENT;
			}
            else if ((wFlags & GIL_PERINSTANCE) && PathIsExe(szIconFile))
			{
                iImage = 0; // II_APPLICATION;
			}
            else
			{
                iImage = 0; // II_DOCNOASSOC;
			}
		}
	}

    if (iImage < 0)
	{
        iImage = 0; // II_DOCNOASSOC;
	}

	return(iImage);
}
