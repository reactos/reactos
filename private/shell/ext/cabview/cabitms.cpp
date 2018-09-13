//*******************************************************************************************
//
// Filename : CabItms.cpp
//	
//				Implementation file for CMemFile, CCabEnum and CCabExtract
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#include "pch.h"
#include "ccstock.h"
#include "thisdll.h"

#include "resource.h"

#include "fdi.h"
#include "cabitms.h"

class CMemFile
{
public:
	CMemFile(HGLOBAL *phMem, DWORD dwSize);
	~CMemFile() {}

	BOOL Create(LPCTSTR pszFile);
	BOOL Open(LPCTSTR pszFile, int oflag);
	LONG Seek(LONG dist, int seektype);
	UINT Read(LPVOID pv, UINT cb);
	UINT Write(LPVOID pv, UINT cb);
	HANDLE Close();

private:
	HANDLE m_hf;

	HGLOBAL *m_phMem;
	DWORD m_dwSize;
	LONG m_lLoc;
} ;


CMemFile::CMemFile(HGLOBAL *phMem, DWORD dwSize) : m_hf(INVALID_HANDLE_VALUE), m_lLoc(0)
{
	m_phMem = phMem;
	m_dwSize = dwSize;

	if (phMem)
	{
		*phMem = NULL;
	}
}


BOOL CMemFile::Create(LPCTSTR pszFile)
{
	if (m_phMem)
	{
		if (*m_phMem)
		{
			return(FALSE);
		}

		*m_phMem = GlobalAlloc(LMEM_FIXED, m_dwSize);
		return(*m_phMem != NULL);
	}
	else
	{
		if (m_hf != INVALID_HANDLE_VALUE)
		{
			return(FALSE);
		}

        m_hf = CreateFile(pszFile,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);
		return (m_hf != INVALID_HANDLE_VALUE);
	}
}


BOOL CMemFile::Open(LPCTSTR pszFile, int oflag)
{
	if (m_phMem)
	{
		return(FALSE);
	}
	else
	{
		if (m_hf != INVALID_HANDLE_VALUE)
		{
			return(FALSE);
		}

        m_hf = CreateFile(pszFile,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          oflag,
                          NULL);
		return (m_hf != INVALID_HANDLE_VALUE);
	}
}


LONG CMemFile::Seek(LONG dist, int seektype)
{
	if (m_phMem)
	{
		if (!*m_phMem)
		{
			return -1;
		}

		switch (seektype)
		{
		case FILE_BEGIN:
			break;

		case FILE_CURRENT:
			dist += m_lLoc;
			break;

		case FILE_END:
			dist = m_dwSize - dist;
			break;

		default:
			return -1;
		}

		if (dist<0 || dist>(LONG)m_dwSize)
		{
			return -1;
		}

		m_lLoc = dist;
		return(dist);
	}
	else
	{
		return(_llseek((HFILE)HandleToUlong(m_hf), dist, seektype));
	}
}


UINT CMemFile::Read(LPVOID pv, UINT cb)
{
	if (m_phMem)
	{
		if (!*m_phMem)
		{
			return -1;
		}

		if (cb > m_dwSize - m_lLoc)
		{
			cb = m_dwSize - m_lLoc;
		}

		hmemcpy(pv, (LPSTR)(*m_phMem)+m_lLoc, cb);
		m_lLoc += cb;
		return(cb);
	}
	else
	{
		return(_lread((HFILE)HandleToUlong(m_hf), pv, cb));
	}
}


UINT CMemFile::Write(LPVOID pv, UINT cb)
{
	if (m_phMem)
	{
		if (!*m_phMem)
		{
			return -1;
		}

		if (cb > m_dwSize - m_lLoc)
		{
			cb = m_dwSize - m_lLoc;
		}

		hmemcpy((LPSTR)(*m_phMem)+m_lLoc, pv, cb);
		m_lLoc += cb;
		return(cb);
	}
	else
	{
		return(_lwrite((HFILE)HandleToUlong(m_hf), (LPCSTR)pv, cb));
	}
}


HANDLE CMemFile::Close()
{
	HANDLE hRet;

	if (m_phMem)
	{
		hRet = *m_phMem ? 0 : INVALID_HANDLE_VALUE;
	}
	else
	{
		hRet = (HANDLE) _lclose((HFILE)HandleToUlong(m_hf));
	}

	delete this;

	return(hRet);
}

//*****************************************************************************
//
// CCabEnum
//
// Purpose:
//
//        Class encapsulating all the FDI operations
//
// Comments:
//
//*****************************************************************************

class CCabEnum
{
public:
	CCabEnum() : m_hfdi(0) {}
	~CCabEnum() {}

protected:
	static void HUGE * FAR DIAMONDAPI CabAlloc(ULONG cb);
	static void FAR DIAMONDAPI CabFree(void HUGE *pv);
	static INT_PTR FAR DIAMONDAPI CabOpen(char FAR *pszFile, int oflag, int pmode);
	static UINT FAR DIAMONDAPI CabRead(INT_PTR hf, void FAR *pv, UINT cb);
	static UINT FAR DIAMONDAPI CabWrite(INT_PTR hf, void FAR *pv, UINT cb);
	static int  FAR DIAMONDAPI CabClose(INT_PTR hf);
	static long FAR DIAMONDAPI CabSeek(INT_PTR hf, long dist, int seektype);

	BOOL StartEnum();
	BOOL SimpleEnum(LPCTSTR szCabFile, PFNFDINOTIFY pfnCallBack, LPVOID pv);
	void EndEnum();

	HFDI m_hfdi;
	ERF  m_erf;

private:
	static CMemFile * s_hSpill;
} ;


CMemFile * CCabEnum::s_hSpill = NULL;

void HUGE * FAR DIAMONDAPI CCabEnum::CabAlloc(ULONG cb)
{
    return(GlobalAllocPtr(GHND, cb));
}

void FAR DIAMONDAPI CCabEnum::CabFree(void HUGE *pv)
{
    GlobalFreePtr(pv);
}

INT_PTR FAR DIAMONDAPI CCabEnum::CabOpen(char FAR *pszFile, int oflag, int pmode)
{
    if(!pszFile)
    {
      return -1;
    }

    // See if we are opening the spill file.
    if( *pszFile=='*' )
    {
		TCHAR szSpillFile[MAX_PATH];
		TCHAR szTempPath[MAX_PATH];

        if(s_hSpill != NULL)
            return -1;

		GetTempPath(ARRAYSIZE(szTempPath), szTempPath);
		GetTempFileName(szTempPath, TEXT("fdi"), 0, szSpillFile);

        s_hSpill = new CMemFile(NULL, 0);
		if (!s_hSpill)
		{
			return(-1);
		}
        if (!s_hSpill->Create(szSpillFile))
		{
			delete s_hSpill;
			s_hSpill = NULL;
			return(-1);
		}

        // Set its extent.
        if( s_hSpill->Seek( ((FDISPILLFILE FAR *)pszFile)->cbFile-1, 0) == HFILE_ERROR)
        {
			s_hSpill->Close();
			s_hSpill = NULL;
			return -1;
        }
        s_hSpill->Write(szSpillFile, 1);

        return (INT_PTR)s_hSpill;
    }

    CMemFile *hFile = new CMemFile(NULL, 0);
	if (!hFile)
	{
		return(-1);
	}

    TCHAR szFile[MAX_PATH];
    SHAnsiToTChar(pszFile, szFile, ARRAYSIZE(szFile));
    while (!hFile->Open(szFile, oflag))
    {
#if 1	// TODO: No UI for inserting a disk at this point
		delete hFile;
		return(-1);
#else
       // Failed to open the source.
       if (!LoadString (g_hInst, IDS_DISKPROMPT, szText, MAX_STRTABLE_LEN))
           return -1;

	   char szText[MAX_PATH];

       wsprintf (g_pErrorBuffer, (LPSTR)szText, (LPSTR)g_pCabName);

       // Use hwndIniting to have a parent window
       if ( MyMessageBox(g_hwndIniting, g_pErrorBuffer,
                    MAKEINTRESOURCE(IDS_DISKPROMPT_TIT),
                    MB_OKCANCEL|MB_ICONSTOP, 0) == IDOK )
          continue;
       else
          return -1;
#endif
    }
   	
    return((INT_PTR)hFile);
}


UINT FAR DIAMONDAPI CCabEnum::CabRead(INT_PTR hf, void FAR *pv, UINT cb)
{
	CMemFile *hFile = (CMemFile *)hf;

    return(hFile->Read(pv,cb));
}


UINT FAR DIAMONDAPI CCabEnum::CabWrite(INT_PTR hf, void FAR *pv, UINT cb)
{
	CMemFile *hFile = (CMemFile *)hf;

	return(hFile->Write(pv,cb));
}


int FAR DIAMONDAPI CCabEnum::CabClose(INT_PTR hf)
{
	CMemFile *hFile = (CMemFile *)hf;

    // Special case for the deletion of the spill file.
    if(hFile == s_hSpill)
	{
        s_hSpill = NULL;
	}

    return (int)HandleToUlong(hFile->Close());
}


long FAR DIAMONDAPI CCabEnum::CabSeek(INT_PTR hf, long dist, int seektype)
{
	CMemFile *hFile = (CMemFile *)hf;

    return(hFile->Seek(dist, seektype));
}


BOOL CCabEnum::StartEnum()
{
	if (m_hfdi)
	{
		// We seem to already be enumerating
		return(FALSE);
	}

	m_hfdi = FDICreate(
		CabAlloc,
        CabFree,
        CabOpen,
        CabRead,
        CabWrite,
        CabClose,
        CabSeek,
        cpu80386,
        &m_erf);

	return(m_hfdi != NULL);
}


BOOL CCabEnum::SimpleEnum(LPCTSTR szCabFile, PFNFDINOTIFY pfnCallBack, LPVOID pv)
{
	char szCabPath[MAX_PATH];
	char szCabName[MAX_PATH];

	// Path should be fully qualified
    char szFile[MAX_PATH];
    SHTCharToAnsi(szCabFile, szFile, ARRAYSIZE(szFile));
    lstrcpynA(szCabPath, szFile, sizeof(szCabPath));
	LPSTR pszName = PathFindFileNameA(szCabPath);
	if (!pszName)
	{
		return(FALSE);
	}

	lstrcpyA(szCabName, pszName);
	*pszName = '\0';

	if (!StartEnum())
	{
		return(FALSE);
	}

	BOOL bRet = FDICopy(
		m_hfdi,
		szCabName,
		szCabPath,		// path to cabinet files
		0,				// flags
		pfnCallBack,
		NULL,
		pv);

	EndEnum();

	return(bRet);
}


void CCabEnum::EndEnum()
{
	if (!m_hfdi)
	{
		return;
	}

	FDIDestroy(m_hfdi);
	m_hfdi = NULL;
}


class CCabItemsCB : private CCabEnum
{
public:
	CCabItemsCB(CCabItems::PFNCABITEM pfnCallBack, LPARAM lParam)
	{
		m_pfnCallBack = pfnCallBack;
		m_lParam = lParam;
	}
	~CCabItemsCB() {}

	BOOL DoEnum(LPCTSTR szCabFile)
	{
		return(SimpleEnum(szCabFile, CabItemsNotify, this));
	}

private:
	static INT_PTR FAR DIAMONDAPI CabItemsNotify(FDINOTIFICATIONTYPE fdint,
		PFDINOTIFICATION pfdin);

	CCabItems::PFNCABITEM m_pfnCallBack;
	LPARAM m_lParam;
} ;


INT_PTR FAR DIAMONDAPI CCabItemsCB::CabItemsNotify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
	CCabItemsCB *pThis = (CCabItemsCB *)pfdin->pv;			

    // uiYield( g_hwndSetup );
    TCHAR szFile[MAX_PATH];
    if (NULL != pfdin->psz1)
    {
        // NOTE: CP_UTF8 is not supported on Win9x!
        SHAnsiToTCharCP((_A_NAME_IS_UTF & pfdin->attribs) ? CP_UTF8 : CP_ACP,
                        pfdin->psz1,
                        szFile,
                        ARRAYSIZE(szFile));
    }

    switch (fdint)
    {
    case fdintCOPY_FILE:
        pThis->m_pfnCallBack(pfdin->psz1 ? szFile : NULL,
                             pfdin->cb,
                             pfdin->date,
                             pfdin->time,
                             pfdin->attribs,
                             pThis->m_lParam);
		break;

	default:
		break;
    } // end switch

    return 0;
}

//*****************************************************************************
//
// CCabItems::EnumItems
//
// Purpose:
//
//        Enumerate the items in the cab file
//
//
// Comments:
//
//         lParam contains pointer to CCabFolder
//
//*****************************************************************************

BOOL CCabItems::EnumItems(PFNCABITEM pfnCallBack, LPARAM lParam)
{
	CCabItemsCB cItems(pfnCallBack, lParam);

	return(cItems.DoEnum(m_szCabFile));
}

//*****************************************************************************
//
// CCabExtractCB
//
// Purpose:
//
//        handles the call back while extracting Cab files
//
//
//*****************************************************************************

class CCabExtractCB : private CCabEnum
{
public:
	CCabExtractCB(LPCTSTR szDir, HWND hwndOwner, CCabExtract::PFNCABEXTRACT pfnCallBack,
		LPARAM lParam)
	{
		m_szDir = szDir;
		m_hwndOwner = hwndOwner;
		m_pfnCallBack = pfnCallBack;
		m_lParam = lParam;
		m_bTryNextCab = FALSE;
	}
	~CCabExtractCB() {}

	BOOL DoEnum(LPCTSTR szCabFile)
	{
		return(SimpleEnum(szCabFile, CabExtractNotify, this));
	}

private:
	static INT_PTR FAR DIAMONDAPI CabExtractNotify(FDINOTIFICATIONTYPE fdint,
		PFDINOTIFICATION pfdin);
	static int CALLBACK CCabExtractCB::BrowseNotify(HWND hwnd, UINT uMsg, LPARAM lParam,
		LPARAM lpData);

	LPCTSTR m_szDir;
	HWND m_hwndOwner;
	CCabExtract::PFNCABEXTRACT m_pfnCallBack;
	LPARAM m_lParam;
	BOOL m_bTryNextCab;
	PFDINOTIFICATION m_pfdin;
} ;


int CALLBACK CCabExtractCB::BrowseNotify(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	CCabExtractCB *pThis = (CCabExtractCB *)lpData;			

	switch (uMsg)
	{
	case BFFM_INITIALIZED:
	{
		// Set initial folder
		LPSTR pszEnd = PathAddBackslashA(pThis->m_pfdin->psz3);
		if (pszEnd - pThis->m_pfdin->psz3 > 3)
		{
			// No problems if not drive root
			*(pszEnd - 1) = '\0';
		}
		SendMessage(hwnd, BFFM_SETSELECTION, 1, (LPARAM)pThis->m_pfdin->psz3);
		break;
	}

	default:
		return(0);
	}

	return(1);
}


INT_PTR FAR DIAMONDAPI CCabExtractCB::CabExtractNotify(FDINOTIFICATIONTYPE fdint,
	PFDINOTIFICATION pfdin)
{
	CCabExtractCB *pThis = (CCabExtractCB *)pfdin->pv;			

    // uiYield( g_hwndSetup );

    switch (fdint)
    {
	case fdintCABINET_INFO:
		pThis->m_bTryNextCab = TRUE;
		break;

	case fdintNEXT_CABINET:
	{
		if (pThis->m_bTryNextCab)
		{
			// Automatically open next cab if already in default dir
			pThis->m_bTryNextCab = FALSE;
			return(1);
		}

		pThis->m_pfdin = pfdin;

		TCHAR szFormat[80];
		TCHAR szTitle[80 + 2*MAX_PATH];
		if (pfdin->psz2[0] != '\0')
		{
			LoadString(g_ThisDll.GetInstance(), IDS_NEXTDISKBROWSE, szFormat, ARRAYSIZE(szFormat));
		}
		else
		{
			LoadString(g_ThisDll.GetInstance(), IDS_NEXTCABBROWSE, szFormat, ARRAYSIZE(szFormat));
		}
		wsprintf(szTitle, szFormat, (LPSTR) (pfdin->psz1), (LPSTR) (pfdin->psz2));

		BROWSEINFO bi;
		bi.hwndOwner = pThis->m_hwndOwner;
		bi.pidlRoot = NULL;
		bi.pszDisplayName = NULL;
		bi.lpszTitle = szTitle;
		bi.ulFlags = BIF_RETURNONLYFSDIRS;
		bi.lpfn = BrowseNotify;
		bi.lParam = (LPARAM)pThis;

		LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

		if (bi.pidlRoot)
		{
			ILFree((LPITEMIDLIST)bi.pidlRoot);
		}

		if (!pidl)
		{
			return(-1);
		}

		BOOL bSuccess = SHGetPathFromIDListA(pidl, pfdin->psz3);
		ILFree(pidl);

		if (bSuccess)
		{
			PathAddBackslashA(pfdin->psz3);
			return(1);
		}

		return(-1);
	}

    case fdintCOPY_FILE:
	{
	    TCHAR szFile[MAX_PATH];
        if (NULL != pfdin->psz1)
        {
            // NOTE: CP_UTF8 is not supported on Win9x!
            SHAnsiToTCharCP((_A_NAME_IS_UTF & pfdin->attribs) ? CP_UTF8 : CP_ACP,
                            pfdin->psz1,
                            szFile,
                            ARRAYSIZE(szFile));
        }
        else
        {
            szFile[0] = TEXT('\0');
        }

        HGLOBAL *phMem = pThis->m_pfnCallBack(pfdin->psz1 ? szFile : NULL,
                                              pfdin->cb,
                                              pfdin->date,
                                              pfdin->time,
                                              pfdin->attribs,
                                              pThis->m_lParam);
		if (!phMem)
		{
			break;
		}

		TCHAR szTemp[MAX_PATH];

		CMemFile *hFile;

		if (pThis->m_szDir == DIR_MEM)
		{
			*szTemp = '\0';
			hFile = new CMemFile(phMem, pfdin->cb);
		}
		else
		{
			PathCombine(szTemp, pThis->m_szDir, PathFindFileName(szFile));
			hFile = new CMemFile(NULL, 0);
		}

		if (!hFile)
		{
			break;
		}

		if (hFile->Create(szTemp))
		{
			return((INT_PTR)hFile);
		}

		delete hFile;

		break;
	}

    case fdintCLOSE_FILE_INFO:
	{
		CMemFile *hFile = (CMemFile *)pfdin->hf;

		return(hFile->Close() == 0);
	}

	default:
		break;
    } // end switch

    return 0;
}

HRESULT CCabExtract::_DoDragDrop(HWND hwnd, IDataObject* pdo, LPCITEMIDLIST pidlFolder)
{
    IShellFolder *psf;
    HRESULT hres = SHBindToObject(NULL, IID_IShellFolder, pidlFolder, (LPVOID*)&psf);

    // This should always succeed because the caller (SHBrowseForFolder) should
    // have weeded out the non-folders.
    if (SUCCEEDED(hres))
    {
        IDropTarget *pdrop;

        // BUGBUG: do we want to pass the hwnd here?
        hres = psf->CreateViewObject(NULL, IID_IDropTarget, (void**)&pdrop);
        if (SUCCEEDED(hres))    // Will fail for some targets. (Like Nethood->Entire Network)
        {
            // May fail if items aren't compatible for drag/drop. (Nethood is one example)
            // MK_CONTROL | MKLBUTTON is used to suggest a "copy":
            hres = SHSimulateDrop(pdrop, pdo, MK_CONTROL | MK_LBUTTON, NULL, NULL);
            pdrop->Release();
        }

        psf->Release();
    }

    return hres;
}

BOOL CCabExtract::ExtractToFolder(HWND hwndOwner, IDataObject* pdo, PFNCABEXTRACT pfnCallBack, LPARAM lParam)
{
    // ASSERT(pdo);
	TCHAR szTitle[80];
	LoadString(g_ThisDll.GetInstance(), IDS_EXTRACTBROWSE, szTitle, ARRAYSIZE(szTitle));

	BROWSEINFO bi;
	bi.hwndOwner = hwndOwner;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = NULL;
	bi.lpszTitle = szTitle;
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (!pidl)
	{
		return(FALSE);
	}

	BOOL bSuccess = SUCCEEDED(_DoDragDrop(hwndOwner, pdo, pidl));

	ILFree(pidl);
	
	return bSuccess;
}


BOOL CCabExtract::ExtractItems(HWND hwndOwner, LPCTSTR szDir, PFNCABEXTRACT pfnCallBack, LPARAM lParam)
{
	// ASSERT(szDir);
	CCabExtractCB cExtract(szDir, hwndOwner, pfnCallBack, lParam);

	// Display Wait cursor until done copying
	CWaitCursor cWait;

	return(cExtract.DoEnum(m_szCabFile));
}

