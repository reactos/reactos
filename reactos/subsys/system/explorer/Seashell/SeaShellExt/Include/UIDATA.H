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

#ifndef __LISTCTRLDATA_H__
#define __LISTCTRLDATA_H__ 

////////////////////////////////////////////////
// CUIODColumnCtrl
// Interface for an owner draw control in a list control column
////////////////////////////////////////////////
class CUIODColumnCtrl : public CObject
{
	DECLARE_DYNAMIC(CUIODColumnCtrl);
public:
	CUIODColumnCtrl();
	virtual ~CUIODColumnCtrl();
public:
	virtual void DoPaint(CDC *PaintDC,CRect rcClient,bool bSelected)=0;
protected:
private:
};

inline CUIODColumnCtrl::CUIODColumnCtrl()
{
}

inline CUIODColumnCtrl::~CUIODColumnCtrl()
{
}

// Internal extension data for each row in the list control
// if you wish to override the default behaviour
// a class should be derived from this one and the GetListCtrlData method
// should be overridden in the list control class
class CTRL_EXT_CLASS CUIListCtrlData : public CObject 
{
public:
    DECLARE_DYNAMIC(CUIListCtrlData)
    CUIListCtrlData(int nCols=1);
    ~CUIListCtrlData();
	void Init(int nCols);
    // functions to change the color of each row
    COLORREF GetTextColor(int nCol=0) const;
    COLORREF GetBkColor(int nCol=0) const;
	const CFont *GetFont(int nCol=0) const;
    void SetBkColor(COLORREF BkColor,int nCol=-1);
    void SetTextColor(COLORREF TextColor,int nCol=-1);
	void SetDefaultBkColor(int nCol=-1);
	void SetDefaultTextColor(int nCol=-1);
	void SetFont(CFont *pFont,int nCol=-1);
	void SetCtrl(CUIODColumnCtrl *pCtrl,int nCol);
	void DestroyCtrl(int nCol);
	// emulate the CListCtrl class by allowing user defined extension data 
	DWORD GetExtData() const;
	void SetExtData(DWORD dwExtData);
	BOOL IsFontSet(int nCol=-1) const;
protected:
	void DestroyFonts();
	void DestroyCtrls();
	void CreateNewFont(int nCol,LOGFONT &lf);
// Get/Set Methods
public:
	void SetAutoDelete(bool bAutoDelete);
	bool GetAutoDelete() const;
	int GetCtrlCount() const;
	CUIODColumnCtrl *GetCtrl(int nCol) const;
	void SetDeleted(bool bDeleted);
	bool IsDeleted() const;
private:
    CDWordArray m_arBkColors;
    CDWordArray m_arTextColors;
	CObArray m_arFonts;
	DWORD m_dwExtData;
	CObArray m_arCtrl;	
	bool m_bAutoDelete;
	bool m_bDeleted;
};

inline int CUIListCtrlData::GetCtrlCount() const
{
	int nCount=0;
	for(int i=0;i < m_arCtrl.GetSize();i++)
	{
		if (m_arCtrl.GetAt(i) != NULL)
		{
			nCount++;
		}
	}
	return nCount;
}

inline CUIODColumnCtrl *CUIListCtrlData::GetCtrl(int nCol) const
{
	CUIODColumnCtrl *pCtrl = (CUIODColumnCtrl*)m_arCtrl.GetAt(nCol);
	return pCtrl;
}

inline void CUIListCtrlData::DestroyCtrl(int nCol)
{
	CUIODColumnCtrl *pOldCtrl = (CUIODColumnCtrl*)m_arCtrl[nCol];
	delete pOldCtrl;
	m_arCtrl[nCol] = NULL;
}

inline void CUIListCtrlData::SetCtrl(CUIODColumnCtrl *pCtrl,int nCol) 
{
	DestroyCtrl(nCol);
	ASSERT_KINDOF(CUIODColumnCtrl,pCtrl);
	m_arCtrl[nCol] = pCtrl;
}

inline void CUIListCtrlData::SetAutoDelete(bool bAutoDelete)
{
	m_bAutoDelete = bAutoDelete;
}

inline bool CUIListCtrlData::GetAutoDelete() const
{
	return m_bAutoDelete;
}

inline void CUIListCtrlData::SetDeleted(bool bDeleted)
{
	m_bDeleted = bDeleted;
}

inline bool CUIListCtrlData::IsDeleted() const
{
	return m_bDeleted;
}

inline const CFont *CUIListCtrlData::GetFont(int nCol) const
{ 
	return (const CFont*)m_arFonts[nCol]; 
}

inline COLORREF CUIListCtrlData::GetTextColor(int nCol) const
{ 
	return m_arTextColors[nCol]; 
}

inline COLORREF CUIListCtrlData::GetBkColor(int nCol) const 
{ 
	return m_arBkColors[nCol]; 
}

inline DWORD CUIListCtrlData::GetExtData() const 
{ 
	return m_dwExtData; 
}

inline void CUIListCtrlData::SetBkColor(COLORREF BkColor,int nCol) 
{ 
	if (nCol == -1)
	{
		for(int i=0;i < m_arBkColors.GetSize();i++)
		{
			m_arBkColors[i] = BkColor;
		}
	}
	else
		m_arBkColors[nCol] = BkColor; 
}

inline void CUIListCtrlData::SetTextColor(COLORREF TextColor,int nCol) 
{ 
	if (nCol == -1)
	{
		for(int i=0;i < m_arTextColors.GetSize();i++)
		{
			m_arTextColors[i] = TextColor;
		}
	}
	else
		m_arTextColors[nCol] = TextColor; 
}

inline void CUIListCtrlData::SetDefaultTextColor(int nCol) 
{ 
	SetTextColor(::GetSysColor(COLOR_WINDOWTEXT),nCol); 
}

inline void CUIListCtrlData::SetDefaultBkColor(int nCol) 
{ 
	SetBkColor(::GetSysColor(COLOR_WINDOW),nCol); 
}

inline void CUIListCtrlData::SetExtData(DWORD dwExtData) 
{ 
	m_dwExtData = dwExtData; 
}

// for ODBC databases
class CTRL_EXT_CLASS CUIDBListCtrlData : public CUIListCtrlData 
{
public:
	CUIDBListCtrlData(int nCols) : CUIListCtrlData(nCols) { }
    DECLARE_DYNAMIC(CUIDBListCtrlData)
	const GetRecNum() { return m_nRecNum; }
	void SetRecNum(long nRecNum) { m_nRecNum = nRecNum; }
private:
	long m_nRecNum;			// for ODBC connection to record sets
};

// used by the Ownerdraw combo box
class CTRL_EXT_CLASS CUIComboBoxData : public CUIListCtrlData 
{
    DECLARE_DYNAMIC(CUIComboBoxData)
public:
	CUIComboBoxData(int nCols) : CUIListCtrlData(nCols), m_nImageIndex(-1) {}
	CString GetText(int nCol = -1) const;
	int GetImageIndex() const { return m_nImageIndex; } 
	void SetText(LPCTSTR szText) { m_strText = szText; }
	void SetImageIndex(int nImageIndex) { m_nImageIndex = nImageIndex; }
private:
	CString m_strText;			// for displaying text in OD combo boxes with columns
	int m_nImageIndex;
};

// keep our own copy of strings in the list control to ease sorting
class CTRL_EXT_CLASS CUIStrListCtrlData : public CUIListCtrlData 
{
public:
    DECLARE_DYNAMIC(CUIStrListCtrlData)
	CUIStrListCtrlData(int nColumns);
	CUIStrListCtrlData();
	LPTSTR GetString(int nCol);
	const CString &GetExtraString() const;
	void AddExtraString(LPCTSTR pszExtraText);
	int GetStringLen(int nCol) const;
	BOOL AddString(int nCol,LPCTSTR szText);
private:
	CStringArray m_StringArray;
	CString m_strExtraText;
};

inline int CUIStrListCtrlData::GetStringLen(int nCol) const
{ 
	return m_StringArray[nCol].GetLength();
}

inline void CUIStrListCtrlData::AddExtraString(LPCTSTR pszExtraText) 
{ 
	m_strExtraText = pszExtraText;
}

inline const CString &CUIStrListCtrlData::GetExtraString() const
{ 
	return m_strExtraText;
}

// Used internally to help sorting
class CUIODListCtrlSortInfo : public CObject
{
public:
	CUIODListCtrlSortInfo(int nSubItem,int ColType,BOOL bSortAscending);
	BOOL Ascending() const;
	int GetColumn() const;
	int GetColType() const;
private:
	BOOL m_bSortAscending;
	int m_nSubItem;
	int m_ColType;
};

inline CUIODListCtrlSortInfo::CUIODListCtrlSortInfo(int nSubItem,int ColType,BOOL bSortAscending) 
	: m_nSubItem(nSubItem),m_ColType(ColType),m_bSortAscending(bSortAscending)
{
}

inline BOOL CUIODListCtrlSortInfo::Ascending() const
{
	return m_bSortAscending;
}

inline int CUIODListCtrlSortInfo::GetColumn() const
{
	return m_nSubItem;
}

inline int CUIODListCtrlSortInfo::GetColType() const
{
	return m_ColType;
}

#endif // __LISTCTRLDATA_H__
