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


#if !defined(AFX_IMSELECTION_H__BE28E652_2B85_11d1_9B50_006008284B53__INCLUDED_)
#define AFX_IMSELECTION_H__BE28E652_2B85_11d1_9B50_006008284B53__INCLUDED_
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#ifndef DROPEFFECT_ALL
#define DROPEFFECT_ALL DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK
#endif

//////////////////////////////////////////
class CTRL_EXT_CLASS CCF_App : public CObject
{
public:
	CCF_App() : m_hWnd(NULL) {};
	void SetHwnd(HWND hWnd);
	HANDLE GetHwnd();
	virtual void Serialize(CArchive &ar);
private:
	HWND m_hWnd;
	DECLARE_SERIAL(CCF_App)
};

inline void CCF_App::SetHwnd(HWND hWnd)
{
	m_hWnd = hWnd;
}

inline HANDLE CCF_App::GetHwnd()
{
	return m_hWnd;
}

//////////////////////////////////////////
class CTRL_EXT_CLASS CCF_FolderType : public CObject
{
public:
	CCF_FolderType(LPCTSTR pszParentCategory,long nCategory,LPCTSTR pszCategory);
	CCF_FolderType() { m_nCategory=0; };
	~CCF_FolderType();
public:
	CCF_FolderType(const CCF_FolderType &WebSite);
	const CCF_FolderType &operator=(const CCF_FolderType &rThat);
	// virtual
	virtual void Serialize(CArchive &ar);
	// inline
	CString GetParentCategory();
	long GetCategory();
	CString GetCategoryName();
protected:
	long m_nCategory;
	CString m_strParentCategory;
	CString m_strCategory;

	DECLARE_SERIAL(CCF_FolderType)
};

inline CString CCF_FolderType::GetParentCategory()
{
	return m_strParentCategory;
}

inline long CCF_FolderType::GetCategory()
{
	return m_nCategory;
}

inline CString CCF_FolderType::GetCategoryName()
{
	return m_strCategory;
}

//////////////////////////////////////////
class CTRL_EXT_CLASS CCF_ShellIDList : public CObject
{
public:
	CCF_ShellIDList();
	virtual ~CCF_ShellIDList();
public:
	// virtual
	virtual void Serialize(CArchive &ar);
	virtual void AddPidl(LPCITEMIDLIST pidl);
	virtual LPCITEMIDLIST GetPidl(UINT nIndex) const;
	virtual UINT GetCount() const;
	LPCITEMIDLIST operator[](UINT nIndex) const;
protected:
	DECLARE_SERIAL(CCF_ShellIDList)
private:
	typedef CArray<LPCITEMIDLIST,LPCITEMIDLIST> arItemIDList;
	arItemIDList m_pidls;
};


//////////////////////////////////////////
class CTRL_EXT_CLASS CCF_WebSite : public CObject
{
public:
	CCF_WebSite() {};
	CCF_WebSite(LPCTSTR pszURL,LPCTSTR pszTitle);
	~CCF_WebSite();
public:
	CCF_WebSite(const CCF_WebSite &WebSite);
	const CCF_WebSite &operator=(const CCF_WebSite &rThat);
	// virtual
	virtual void Serialize(CArchive &ar);
	// inline
	virtual LPCTSTR GetURL();
	virtual LPCTSTR GetTitle();
protected:
	CString m_strURL;
	CString m_strTitle;
//
	DECLARE_SERIAL(CCF_WebSite)
};

inline	LPCTSTR CCF_WebSite::GetURL()
{
	return m_strURL;
}

inline	LPCTSTR CCF_WebSite::GetTitle()
{
	return m_strTitle;
}

class CTRL_EXT_CLASS CCF_DBFolderList : public CList<CCF_FolderType,CCF_FolderType&>
{
public:
	CCF_DBFolderList(HWND hWnd=NULL) { m_App.SetHwnd(hWnd); };
	~CCF_DBFolderList() {};
	void SetDatabase(const CString &strDatabase);
	CString GetDatabase() const;
	virtual void Serialize(CArchive &ar);
public:
	CCF_App m_App;
protected:
	CString m_strDatabase;
	DECLARE_SERIAL(CCF_DBFolderList)
};

inline void CCF_DBFolderList::SetDatabase(const CString &strDatabase)
{
	m_strDatabase = strDatabase;
}

inline CString CCF_DBFolderList::GetDatabase() const
{
	return m_strDatabase;
}

// template CCF_DBFolderList
/////////////////////////////////////
template<> void CTRL_EXT_CLASS AFXAPI SerializeElements <CCF_FolderType>(CArchive& ar, CCF_FolderType *pWebSiteCategory, int nCount );
template<> void CTRL_EXT_CLASS AFXAPI DestructElements<CCF_FolderType>(CCF_FolderType *pElements, int nCount);
template<> void CTRL_EXT_CLASS AFXAPI ConstructElements<CCF_FolderType>(CCF_FolderType *pElements, int nCount);

// template CList
/////////////////////////////////////
typedef CList<CCF_WebSite,CCF_WebSite&> CListWebSites;
template<> void CTRL_EXT_CLASS AFXAPI SerializeElements <CCF_WebSite>(CArchive& ar, CCF_WebSite *pWebSite, int nCount );
template<> void CTRL_EXT_CLASS AFXAPI DestructElements<CCF_WebSite>(CCF_WebSite *pElements, int nCount);
template<> void CTRL_EXT_CLASS AFXAPI ConstructElements<CCF_WebSite>(CCF_WebSite *pElements, int nCount);

/////////////////////////////////////
class CTRL_EXT_CLASS CCF_WebSites : public CObject
{
public:
	CCF_WebSites(HWND hWnd=NULL) { m_App.SetHwnd(hWnd); };
	~CCF_WebSites() {};
	CListWebSites &GetListWebSites();
	POSITION GetHeadPosition();
	CCF_WebSite &GetNext(POSITION &pos);
	virtual void Serialize(CArchive &ar);
protected:
	CCF_App m_App;
	CListWebSites m_listWebSites;
	DECLARE_SERIAL(CCF_WebSites)
};

inline CCF_WebSite &CCF_WebSites::GetNext(POSITION &pos)
{
	return m_listWebSites.GetNext(pos);
}

inline POSITION CCF_WebSites::GetHeadPosition()
{
	return m_listWebSites.GetHeadPosition();
}

inline CListWebSites &CCF_WebSites::GetListWebSites()
{
	return m_listWebSites;
}

//////////////////////////////////////////
class CTRL_EXT_CLASS CCF_String : public CObject
{
public:
	CCF_String() {};
	CCF_String(LPCTSTR pszText);
	~CCF_String();
public:
	// virtual
	virtual void Serialize(CArchive &ar);
	virtual LPCTSTR GetString();
	void SetString(LPCTSTR pszText);
	DECLARE_SERIAL(CCF_String)
private:
	CString m_sText;
};

class CTRL_EXT_CLASS CCF_RightMenu : public CObject
{
public:
	CCF_RightMenu() { m_bRightDrag = false; };
	~CCF_RightMenu() {};
public:
	void SetRightDrag(bool bRightDrag);
	// virtual
	virtual void Serialize(CArchive &ar);
	virtual bool IsRightDrag();

	DECLARE_SERIAL(CCF_RightMenu)
private:
	bool m_bRightDrag;
};

class CTRL_EXT_CLASS CCF_FileGroupDescriptor : public CObject
{
public:
	CCF_FileGroupDescriptor();
	~CCF_FileGroupDescriptor();
public:
	CString GetFileName(UINT nItem);
	CString GetTitle(UINT nItem);
	void SetTitle(const CString &sTitle);
	LPFILEDESCRIPTOR GetFileDescriptor(UINT nItem);
	// virtual
	virtual void Serialize(CArchive &ar);

	DECLARE_SERIAL(CCF_FileGroupDescriptor)
private:
	CString m_sTitle;
	UINT m_nItems;
	LPFILEDESCRIPTOR m_pFileDescrs;
};

//////////////////////////////////
class CTRL_EXT_CLASS CCF_HDROP : public CObject
{
public:
	CCF_HDROP();
	~CCF_HDROP();
public:
	UINT GetCount() { return m_sFileNames.GetSize(); }
	CString GetFileName(UINT nItem);
	void AddFileName(LPCTSTR pszFileName);
	void AddDropPoint(CPoint &pt,BOOL bfNC=FALSE);
	// virtual
	virtual void Serialize(CArchive &ar);

	DECLARE_SERIAL(CCF_HDROP)
private:
	CStringArray m_sFileNames;
	int m_nFiles;
	CPoint m_pt;
	BOOL m_fNC;
};

class CTRL_EXT_CLASS CWDClipboardData : public CObject
{
public:
	enum eCBFormats
	{
		e_cfString,
		e_cfHDROP,
		e_cfWebSiteURL,
		e_cfWebSite,
		e_cfHTMLFormat,
		e_cfFolder,
		e_cfRightMenu,
		e_cfFileGroupDesc,
		e_cfShellIDList,
		e_cfMax
	};
	CWDClipboardData();
	~CWDClipboardData();
public:
	void SetData(COleDataSource *pDataSource,CObject *pObj,eCBFormats format,LPFORMATETC lpFormatEtc=NULL);
	bool GetData(COleDataObject *pDataObject,CObject *pObj,eCBFormats format);
	CLIPFORMAT GetClipboardFormat(eCBFormats format);
	bool IsDataAvailable(COleDataObject *pDataObject);
	static CWDClipboardData *Instance();
protected:
	bool IsValidFormat(CLIPFORMAT cfFormat);
private:
	CLIPFORMAT m_aFormatIDs[e_cfMax];
};

#endif // BE28E652-2B85-11d1-9B50-006008284B53

