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
#include "cbformats.h"
#include "shlobj.h"
#include "ShellPidl.h"
#include <afxpriv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//static
CWDClipboardData *CWDClipboardData::Instance()
{
	static CWDClipboardData data;
	return &data;
}

CWDClipboardData::CWDClipboardData()
{
	m_aFormatIDs[e_cfString] = CF_TEXT;
	m_aFormatIDs[e_cfHDROP] = CF_HDROP;
	m_aFormatIDs[e_cfWebSiteURL] = ::RegisterClipboardFormat(CFSTR_SHELLURL);
#ifdef _UNICODE
	m_aFormatIDs[e_cfFileGroupDesc] = ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
#else
	m_aFormatIDs[e_cfFileGroupDesc] = ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
#endif
	m_aFormatIDs[e_cfHTMLFormat] = ::RegisterClipboardFormat(_T("HTML Format"));
	m_aFormatIDs[e_cfWebSite] = ::RegisterClipboardFormat(_T("CF_WD_WEBSITE"));
	m_aFormatIDs[e_cfRightMenu] = ::RegisterClipboardFormat(_T("CF_WD_RIGHTMENU"));
	m_aFormatIDs[e_cfFolder] = ::RegisterClipboardFormat(_T("CF_WD_WEBSITE_CATEGORY"));
	m_aFormatIDs[e_cfShellIDList] = ::RegisterClipboardFormat(CFSTR_SHELLIDLIST);
}

CWDClipboardData::~CWDClipboardData()
{
}

bool CWDClipboardData::IsValidFormat(CLIPFORMAT cfFormat)
{
	for(int i=0;i < e_cfMax;i++)
	{
		if (m_aFormatIDs[i] == cfFormat)
			break;
	}
	return(i != e_cfMax);
}

bool CWDClipboardData::IsDataAvailable(COleDataObject *pDataObject)
{
	// Iterate through the clipboard formats
	pDataObject->BeginEnumFormats();
	FORMATETC FormatEtc;
	bool bFound=false;
#ifdef _DEBUG
	TCHAR szBuf[128];
#endif
	while (pDataObject->GetNextFormat(&FormatEtc))
	{
#ifdef _DEBUG
		szBuf[0] = 0;
		if (FormatEtc.cfFormat > CF_MAX)
		{
			::GetClipboardFormatName(FormatEtc.cfFormat,szBuf,sizeof(szBuf)-1);
		}
		else
		{
			lstrcpy(szBuf,_T("Predefined Format"));
		}
		TRACE(_T("Enum formats returned %u(%s) %d\n"),FormatEtc.cfFormat,szBuf,FormatEtc.tymed);
#endif
		if (IsValidFormat(FormatEtc.cfFormat))
		{
#ifdef _DEBUG
			TRACE(_T("Clipboard format found %u(%s) tymed(%d), Aspect(%d)\n"),FormatEtc.cfFormat,szBuf,FormatEtc.tymed,FormatEtc.dwAspect);
#endif
			bFound=true;
		}
	}	
	return bFound;
}

CLIPFORMAT CWDClipboardData::GetClipboardFormat(eCBFormats format)
{
	return m_aFormatIDs[format]; 
}

void CWDClipboardData::SetData(COleDataSource *pOleDataSource,CObject *pObj,eCBFormats format,LPFORMATETC lpFormatEtc)
{
	ASSERT(pOleDataSource);
	ASSERT(pObj);

	// use serialization
	CSharedFile file;
	CArchive ar(&file,CArchive::store);
	pObj->Serialize(ar);
	ar.Close();

	// cache into drag drop source
	pOleDataSource->CacheGlobalData(CWDClipboardData::Instance()->GetClipboardFormat(format),file.Detach(),lpFormatEtc);		
}

bool CWDClipboardData::GetData(COleDataObject *pDataObject,CObject *pObj,eCBFormats format)
{
	ASSERT(pDataObject);
	ASSERT(pObj);
	if (pDataObject == NULL || pObj == NULL)
		return false;

	if (m_aFormatIDs[format] == CF_HDROP)
	{
		FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		STGMEDIUM medium;
		if (pDataObject->GetData(CF_HDROP, &medium, &fmte))
		{
			HDROP hDropInfo = (HDROP)medium.hGlobal;
			UINT wNumFilesDropped = DragQueryFile(hDropInfo, 0XFFFFFFFF, NULL, 0);
			TCHAR szFile[MAX_PATH];
			ASSERT_KINDOF(CCF_HDROP,pObj);
			CCF_HDROP *pHDROP = (CCF_HDROP*)pObj;
			UINT nLen;
			for(UINT i=0; i < wNumFilesDropped;i++)
			{
				nLen = DragQueryFile(hDropInfo,i,szFile,sizeof(szFile)/sizeof(TCHAR));
				if (nLen)
				{
					pHDROP->AddFileName(szFile);
				}
			}
			if (medium.pUnkForRelease)
				medium.pUnkForRelease->Release();
			else
				GlobalFree(medium.hGlobal);
		}
	}
	else
	{
		CFile *pFile = pDataObject->GetFileData(CWDClipboardData::Instance()->GetClipboardFormat(format));
		if (pFile)
		{
			CArchive ar(pFile,CArchive::load);
			pObj->Serialize(ar);
			ar.Close();
			delete pFile;
		}
	}
	return true;
}

///////////////////////////////////////////
//
//	Clipboard objects
//
///////////////////////////////////////////
IMPLEMENT_SERIAL(CCF_App, CObject, 0 );
IMPLEMENT_SERIAL(CCF_WebSites,CObject,0);
IMPLEMENT_SERIAL(CCF_String, CObject, 0 );
IMPLEMENT_SERIAL(CCF_WebSite, CObject, 0 );
IMPLEMENT_SERIAL(CCF_FolderType, CObject, 0 );
IMPLEMENT_SERIAL(CCF_DBFolderList,CObject,0);
IMPLEMENT_SERIAL(CCF_RightMenu, CObject, 0 );
IMPLEMENT_SERIAL(CCF_FileGroupDescriptor,CObject,0);
IMPLEMENT_SERIAL(CCF_ShellIDList,CObject,0);
IMPLEMENT_SERIAL(CCF_HDROP,CObject,0);
///////////////////////////////////////////
bool CCF_RightMenu::IsRightDrag()
{
	return m_bRightDrag;
}

void CCF_RightMenu::SetRightDrag(bool bRightDrag)
{
	m_bRightDrag = bRightDrag;
}

void CCF_RightMenu::Serialize(CArchive &ar)
{
	if (ar.IsLoading())
	{
		int nRightDrag;
		ar >> nRightDrag;
		m_bRightDrag = nRightDrag == 1;
	}
	else
	{
		ar << m_bRightDrag;
	}
}

CCF_FileGroupDescriptor::CCF_FileGroupDescriptor() 
{
	m_pFileDescrs = NULL;
	m_nItems = 0;
}

CCF_FileGroupDescriptor::~CCF_FileGroupDescriptor()
{
	delete []m_pFileDescrs;
}

LPFILEDESCRIPTOR CCF_FileGroupDescriptor::GetFileDescriptor(UINT nItem)
{
	if (m_pFileDescrs == NULL)
		return NULL;
	if (nItem >= m_nItems)
	{
		ASSERT(FALSE);
		return NULL;
	}
	return &m_pFileDescrs[nItem];
}

void CCF_FileGroupDescriptor::SetTitle(const CString &sTitle)
{
	m_sTitle = sTitle;
}

CString CCF_FileGroupDescriptor::GetFileName(UINT nItem)
{
	if (nItem >= m_nItems || m_pFileDescrs == NULL)
	{
		ASSERT(FALSE);
		return CString();
	}
	return m_pFileDescrs[nItem].cFileName;
}

CString CCF_FileGroupDescriptor::GetTitle(UINT nItem)
{
	if (nItem >= m_nItems || m_pFileDescrs == NULL)
	{
		ASSERT(FALSE);
		return CString();
	}
	CString strLink(m_pFileDescrs[nItem].cFileName);
	int nPos = strLink.Find(_T(".URL"));
	if (nPos < 0)
		nPos = strLink.Find(_T(".url"));
	if (nPos > 0)
	{
		return strLink.Left(nPos);
	}
	return strLink;
}

void CCF_FileGroupDescriptor::Serialize(CArchive &ar)
{
	if (ar.IsLoading())
	{
		ar.Read(&m_nItems,sizeof(UINT));
		if (m_nItems)
		{
			m_pFileDescrs = new FILEDESCRIPTOR[m_nItems];
			ar.Read(m_pFileDescrs,sizeof(FILEDESCRIPTOR)*m_nItems);
			for(UINT i=0;i < m_nItems;i++)
				TRACE3("(%u) - cFileName %s, Size=%u\n",i,m_pFileDescrs[i].cFileName,m_pFileDescrs[i].nFileSizeLow);
		}
	}
	else
	{
		UINT cItems=1;
		ar.Write(&cItems,sizeof(UINT));
		FILEDESCRIPTOR FileDesc;
		ZeroMemory(&FileDesc,sizeof(FILEDESCRIPTOR));
		FileDesc.dwFlags = (FD_LINKUI | FD_FILESIZE);
		if (!m_sTitle.IsEmpty())
		{
			lstrcpy(FileDesc.cFileName,m_sTitle);
			lstrcat(FileDesc.cFileName,_T(".url"));
			FileDesc.nFileSizeLow = lstrlen(FileDesc.cFileName)+24;
			ar.Write(&FileDesc,sizeof(FILEDESCRIPTOR));
		}
	}
}

////////////////////////////////////////////////
// CCF_HDROP
////////////////////////////////////////////////
CCF_HDROP::CCF_HDROP() 
{
	m_nFiles = 0;
	m_fNC = FALSE;
}

CCF_HDROP::~CCF_HDROP()
{
}

CString CCF_HDROP::GetFileName(UINT nItem)
{
	if (m_sFileNames.GetSize() > 0)
		return m_sFileNames[nItem];
	return _T("");
}

void CCF_HDROP::AddDropPoint(CPoint &pt,BOOL fNC)
{
	m_pt = pt;
	m_fNC = fNC;
}

void CCF_HDROP::AddFileName(LPCTSTR pszFileName)
{
	m_sFileNames.SetAtGrow(m_nFiles++,pszFileName);
}

void CCF_HDROP::Serialize(CArchive &ar)
{
	if (ar.IsLoading())
	{
		// handled in GetData
	}
	else
	{
		DROPFILES dropfiles;
		dropfiles.pFiles = sizeof(dropfiles);
		dropfiles.fNC = m_fNC;
		dropfiles.pt = m_pt;
#ifdef _UNICODE
		dropfiles.fWide = TRUE;
#else
		dropfiles.fWide = FALSE;
#endif
		ar.Write(&dropfiles,sizeof(DROPFILES));
		LPCTSTR pszFileName=NULL;
		for(int i=0; i < m_nFiles;i++)
		{
			pszFileName=m_sFileNames[i];
			ar.Write(pszFileName,lstrlen(pszFileName)+sizeof(TCHAR));
		}
		TCHAR rchar=0;
		ar.Write(&rchar,sizeof(TCHAR));
	}
}

////////////////////////////////////////////////
// CCF_ShellIDList
////////////////////////////////////////////////
CCF_ShellIDList::CCF_ShellIDList() 
{
}

CCF_ShellIDList::~CCF_ShellIDList()
{
}

LPCITEMIDLIST CCF_ShellIDList::operator[](UINT nIndex) const
{
	return GetPidl(nIndex);
}

LPCITEMIDLIST CCF_ShellIDList::GetPidl(UINT nIndex) const
{
	if (nIndex > (UINT)m_pidls.GetSize()) 
	{ 
		return NULL; 
	} 
	return(m_pidls[nIndex]); 
}

UINT CCF_ShellIDList::GetCount() const
{
	return m_pidls.GetSize();  
}

void CCF_ShellIDList::AddPidl(LPCITEMIDLIST pidl)
{
	m_pidls.Add(pidl);
}

void CCF_ShellIDList::Serialize(CArchive &ar)
{
	if (ar.IsLoading())
	{
		// read the header
		CIDA cida;
		ar.Read(&cida,sizeof(CIDA));
		LPCITEMIDLIST pidl=NULL;
		UINT nOffset;
		// read the ITEMIDLIST structures
		for(UINT i=0;i < cida.cidl+1;i++)
		{
			nOffset = cida.aoffset[i];
			ar.GetFile()->Seek(CFile::begin,nOffset*sizeof(UINT));
			ar.Read((void*)pidl,sizeof(ITEMIDLIST));
			m_pidls.Add(pidl);
		}
	}
	else
	{
		CShellPidl ShellPidl;
		CIDA cida;
		cida.cidl = m_pidls.GetSize();
		ASSERT(cida.cidl > 1);
		if (cida.cidl <= 1)
			return;
		UINT pidls=cida.cidl-1;
		ar.Write(&pidls,sizeof(pidls));
		const UINT nOffset=sizeof(UINT);
		UINT nElemOffset;
		UINT nPidlSize=0;
		for(UINT i1=0;i1 < cida.cidl;i1++)
		{
			nElemOffset = sizeof(cida.cidl)+(cida.cidl*sizeof(nElemOffset))+nOffset+nPidlSize;
			ar.Write(&nElemOffset,sizeof(nElemOffset));
			nPidlSize += ShellPidl.GetSize(m_pidls[i1]);
		}
		BYTE nZero=0;
		for(int i=0;i < nOffset;i++)
			ar.Write(&nZero,sizeof(nZero));
		TRACE1("Writing %u pidls\n",cida.cidl);
		LPSHELLFOLDER pFolder=ShellPidl.GetFolder((LPITEMIDLIST)m_pidls[0]);
		for(UINT i2=0;i2 < cida.cidl;i2++)
		{
#ifdef _DEBUG
			CString sPath;
			ShellPidl.SHPidlToPathEx((LPITEMIDLIST)m_pidls[i2],sPath,pFolder);
			TRACE3("Writing pidl %s (%u) size(%u)\n",sPath,i2,ShellPidl.GetSize(m_pidls[i2]));
#endif	
			ar.Write(m_pidls[i2],ShellPidl.GetSize(m_pidls[i2]));
		}
		if (pFolder)
			pFolder->Release();
	}
}

///////////////////////////////////////////////////////////////////
// Private clipboard formats for drag and drop
///////////////////////////////////////////////////////////////////
const CCF_WebSite &CCF_WebSite::operator=(const CCF_WebSite &rThat)
{
	if (this != &rThat)
	{
		m_strURL = rThat.m_strURL;
		m_strTitle = rThat.m_strTitle;
	}
	return *this;
}

CCF_WebSite::CCF_WebSite(const CCF_WebSite &WebSite)
{
	*this = WebSite;
}

////////////////////////////////////////////

void CCF_App::Serialize(CArchive &ar)
{
	if (ar.IsLoading())
	{
		long hWnd;
		ar >> hWnd;
		m_hWnd = (HWND)hWnd;
	}
	else
	{
		ar << (long)m_hWnd;
	}
}
/////////////////////////////////////////

void CCF_WebSites::Serialize(CArchive &ar)
{
	m_App.Serialize(ar);
	m_listWebSites.Serialize(ar);
}

/////////////////////////////////////////

CCF_String::CCF_String(LPCTSTR pszText) : m_sText(pszText) 
{
}

CCF_String::~CCF_String()
{
}

LPCTSTR CCF_String::GetString()
{
	return m_sText;
}

void CCF_String::SetString(LPCTSTR pszText)
{
	m_sText = pszText;
}

void CCF_String::Serialize(CArchive &ar)
{
	if (ar.IsLoading())
	{
		TCHAR szText[MAX_PATH+1];
		m_sText.Empty();
		while (1)
		{
			ZeroMemory(szText,sizeof(szText));
			ar.Read(szText,MAX_PATH);
			m_sText += szText;
			if (lstrlen(szText) < MAX_PATH)
				break;
		}
	}
	else
	{
		ar.Write((LPCTSTR)m_sText, m_sText.GetLength()*sizeof(TCHAR));
		int null=0;
		ar.Write(&null,sizeof(null));
	}
}

//
// Private clipboard formats for drag and drop
//
CCF_WebSite::CCF_WebSite(LPCTSTR pszURL,LPCTSTR pszTitle) : 
	m_strURL(pszURL),
		m_strTitle(pszTitle)
{
}

CCF_WebSite::~CCF_WebSite()
{
}

void CCF_WebSite::Serialize(CArchive &ar)
{
	if (ar.IsLoading())
	{
		ar >> m_strURL;
		ar >> m_strTitle;
	}
	else
	{
		ar << m_strURL;
		ar << m_strTitle;
	}
}

//
//////////////////////////////////////////////////////

CCF_FolderType::CCF_FolderType(LPCTSTR pszParentCategory,long nCategory,LPCTSTR pszCategory) :
		m_strParentCategory(pszParentCategory),
		m_nCategory(nCategory),
		m_strCategory(pszCategory)
{
}

CCF_FolderType::~CCF_FolderType()
{
}

const CCF_FolderType &CCF_FolderType::operator=(const CCF_FolderType &rThat)
{
	if (this != &rThat)
	{
		m_nCategory = rThat.m_nCategory;
		m_strCategory = rThat.m_strCategory;
		m_strParentCategory = rThat.m_strParentCategory;
	}
	return *this;
}

CCF_FolderType::CCF_FolderType(const CCF_FolderType &WebSiteCategory)
{
	*this = WebSiteCategory;
}

void CCF_FolderType::Serialize(CArchive &ar)
{
	if (ar.IsLoading())
	{
		ar >> m_nCategory;
		ar >> m_strCategory;
		ar >> m_strParentCategory;
	}
	else
	{
		ar << m_nCategory;
		ar << m_strCategory;
		ar << m_strParentCategory;
	}
}

/////////////////////////////////////////////////

void CCF_DBFolderList::Serialize(CArchive &ar)
{
	m_App.Serialize(ar);
	if (ar.IsLoading())
	{
		ar >> m_strDatabase;
	}
	else
	{
		ar << m_strDatabase;
	}
	CList<CCF_FolderType,CCF_FolderType&>::Serialize(ar);
}

template <> void AFXAPI SerializeElements <CCF_FolderType>(CArchive& ar, CCF_FolderType *pWebSite, int nCount )
{
    for ( int i = 0; i < nCount; i++, pWebSite++ )
    {
		pWebSite->Serialize(ar);
	}
}

template<> void AFXAPI DestructElements<CCF_FolderType> (CCF_FolderType *pElements, int nCount)
{
	for ( int n = 0; n < nCount; n++, pElements++ )
	{
		pElements->CCF_FolderType::~CCF_FolderType();
	}
}

template<> void AFXAPI ConstructElements<CCF_FolderType> (CCF_FolderType *pElements, int nCount)
{
	for ( int n = 0; n < nCount; n++, pElements++ )
	{
		pElements->CCF_FolderType::CCF_FolderType();
	}
}

///////////////////////////////////////////////////	
template <> void AFXAPI SerializeElements <CCF_WebSite>(CArchive& ar, CCF_WebSite *pWebSite, int nCount )
{
    for ( int i = 0; i < nCount; i++, pWebSite++ )
    {
		pWebSite->Serialize(ar);
	}
}

template<> void AFXAPI DestructElements<CCF_WebSite> (CCF_WebSite *pElements, int nCount)
{
	for ( int n = 0; n < nCount; n++, pElements++ )
	{
		pElements->CCF_WebSite::~CCF_WebSite();
	}
}

template<> void AFXAPI ConstructElements<CCF_WebSite> (CCF_WebSite *pElements, int nCount)
{
	for ( int n = 0; n < nCount; n++, pElements++ )
	{
		pElements->CCF_WebSite::CCF_WebSite();
	}
}

