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

#ifndef __ODLISTCTRL_H__
#define __ODLISTCTRL_H__

#define NUM_BITMAPS 2
#define MAX_TEXT	MAX_PATH

#include "UICont.h"
#include "UIDropTarget.h"
#include "UIMessages.h"
#include "LocaleInfo.h"

class CInPlaceEdit;
class CTextProgressCtrl;

//////////////////////////////////////////////////////////
//CODHeaderCtrl
//////////////////////////////////////////////////////////
class CTRL_EXT_CLASS CODHeaderCtrl : public CHeaderCtrl
{
public:
	CODHeaderCtrl();
	~CODHeaderCtrl();
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	int SetSortImage(int nCol, BOOL bAsc);
protected:	
	int	m_nSortCol;
	BOOL	m_bSortAsc;
};

//////////////////////////////////////////////////////////////////////////
// CUIODListCtrl
// Extension of the MFC class that enacapsulates the windows list control
// makes some things simpler some things harder and some things better.
//////////////////////////////////////////////////////////////////////////
class CTRL_EXT_CLASS CUIODListCtrl : public CListCtrl 
{
public:
	enum eColTypes
	{
		e_Text,
		e_Numeric,
		e_NumericFormat,
		e_NumericFormatComma,
		e_NumericFormatTime,
		e_Double,
		e_DoubleFormat,
		e_DoubleFormatComma,
		e_DoubleFormatTime,
		e_DateTime
	};
	DECLARE_SERIAL(CUIODListCtrl)
public:
	CUIODListCtrl(bool bDragDrop=true);
	virtual ~CUIODListCtrl();  

// Attributes
	const COleDropTarget &GetDropTarget();

// Operations
public:
	void RegisterDropTarget();
	virtual void SetRowFont(int nRow,CFont *pFont);
	virtual void SetRowBold(int nRow,BOOL bBold=TRUE);
	virtual void SetColFont(int nRow,int nCol,CFont *pFont);
	virtual void SetColBold(int nRow,int nCol,BOOL bBold=TRUE);
	virtual BOOL SetColumnFormat(int nCol,int fmt);
	virtual void Serialize(CArchive& ar);
	 
	// called from CUIListView::OnCreate
	virtual void ChangeStyle(UINT &dwStyle);
	virtual void Init();
	virtual void UpdateEvent(LPARAM lHint,CObject *pHint);
	// Initialize the list control 
	// The first parameter is a pointer to an array or single icon
	// that will be added to an image list
	// The second parameter will create the columns and add the heading
	// but must be in this format Column1|Column2 delimited by the or sign
	// This MUST BE CALLED first unless you use CListCtrl or API methods to add columns
	// then you use AddTextItem to add a row passing the returned row number every time
	virtual int InitListCtrl(const UINT *pIconIDs,LPCTSTR szHeadings,LPCTSTR pszProfile=NULL);
	virtual int InitListCtrl(UINT IconID,LPCTSTR szHeadings,LPCTSTR pszProfile=NULL);
	virtual int InitListCtrl(LPCTSTR szHeadings,LPCTSTR pszProfile=NULL);
	// used to add strings after a text row has been created
	virtual BOOL AddString(int nRow,int nCol,LPCTSTR szText,CUIODListCtrl::eColTypes type=CUIODListCtrl::e_Text);
	virtual BOOL AddString(int nRow,int nCol,int nValue,CUIODListCtrl::eColTypes type=CUIODListCtrl::e_NumericFormatComma);
	virtual BOOL AddString(int nRow,int nCol,double dValue,CUIODListCtrl::eColTypes type=CUIODListCtrl::e_DoubleFormatComma);
	virtual BOOL AddString(int nRow,int nCol,COleDateTime &dtValue,CUIODListCtrl::eColTypes type=CUIODListCtrl::e_DateTime);
	// Refreshes the row after a row has changed after a call to addstring
	virtual void UpdateString(int nRow);
	// add a new column will return index or -1 on failure
	virtual void AddExtraString(int nRow,LPCTSTR pszExtraText);
	// Adds a new column 
	virtual int AddColumn(LPCTSTR szText);
	// Change the the window that receives the popup command messages
	// default is the control
	void SetPopupWnd(CWnd *pPopupWnd);
	// Use your own popup menu
	void SetPopupID(UINT nPopupID);
	// Use your own popup menu when multiple item have been selected
	void SetMultiPopupID(UINT nMultiPopupID);
	// find a column index by text
	int FindColumn(LPCTSTR pszText);
	// same as the list control
	DWORD GetItemData(int nIndex) const;
	// add a row with data on demand see 'GetDispInfo' method
	int AddCallBackItem(DWORD dwData=0,int nImage=0);
	// add a text row where you supply the string
	int AddTextItem(int nImage=0);
	// same as list control with extra flag
	BOOL SetItemData(int nIndex,DWORD dwData,bool bAutoDelete=false);
	// Sets the item data but will also delete the pointer(so make sure it is one)
	// be careful with this one
	BOOL SetItemDataAutoDelete(int nIndex,CObject *pObj);
	void SetDefaultTextColor( int nRow, int nCol);
	void SetDefaultBkColor( int nRow, int nCol);
	// set the row color (only works with ownerdraw control)
	void SetTextColor( int nRow, int nCol, COLORREF TextColor);
	// set the row color (only works with ownerdraw control)
	void SetBkColor( int nRow, int nCol, COLORREF BkColor);
	// toggle between large and small icon
	void SetIconSize(int nImageList);
	// set the icon passing an ICON id from the resources
	int SetIcon(int nRow,UINT nIconID);
	// set the icon passing the image index into the internal image list 
	int SetIcon(int nRow,int nImage);
	// find an item by extension data
	int FindItem(DWORD dwExtData);
	// set the current selection
	int SetCurSel(int nSelect);
	void SetDragDrop(bool bDragDrop);
	void SetDropFiles(bool bDropFiles);
	void SetEditSubItems(bool bEdit);
	// toggles the column sizing permission
	void SetColumnSizing(bool bSet);
	void SetToolTips(bool bSet);
	BOOL SetGridLines(bool bSet);
	BOOL SetTrackSelect(bool bSet);
	BOOL SetCheckBoxes(bool bSet);
	BOOL SetHeaderDragDrop(bool bSet);
	void SetFullRowSel(bool bSet);
	// changes the view type between LVS_ICON, LVS_SMALLICON, LVS_REPORT, etc
	BOOL SetEditLabels(bool bSet);

	void ToggleColumnSizing();
	void ToggleToolTips();
	// toggles grid lines(IE3.0 controls only)
	BOOL ToggleGridLines();
	// toggles hot tracking lines(IE3.0 controls only)
	BOOL ToggleTrackSelect();
	// toggles check boxes (IE3.0 controls only)
	BOOL ToggleCheckBoxes();
	// toggles between moving columns by dragging the header (IE3.0 controls only)
	BOOL ToggleHeaderDragDrop();
	// toggles full row selection lines(IE3.0 controls only)
	void ToggleFullRowSel();
	// changes the view type between LVS_ICON, LVS_SMALLICON, LVS_REPORT, etc
	BOOL ToggleEditLabels();

	BOOL SetViewType(DWORD dwViewType,UINT nFlags=0);
	BOOL GetFullRowSel() const;
	DWORD GetViewType() const;
	// (IE3.0 controls only)
	// set the extended style for the new IE3.0 common controls 
	BOOL SetExStyle(UINT nStyle,BOOL bModify=TRUE);
	DWORD GetExStyle();

	int GetIconSize() const;
	// call this first to get the current or first selection
	// returns -1 if no selection
	int GetCurSel() const;
	// used to get multiple selection
	// returns -1 if no selection
	int GetNextSel(int item) const;
	// number of rows
	int GetCount() const;
	// number of columns
	int GetColumnCount() const;
	// allow the right mouse click to toggle between icons sizes
	void SetRCLickChangeIconSize(BOOL bSize);
	int HitTestEx(CPoint &point, int *col) const;
	CEdit* EditSubLabel( int nItem, int nCol );
	CTextProgressCtrl *AddProgressBar(int nItem,int nCol,int nMin,int nMax);
	virtual void UpdateProgressBar(int nRow,int nCol,int nPos);
	void DeleteProgressBar(int nItem,int nCol);
	// Add an icon to the image list and return the index into the list
	int AddIcon(UINT nIconID);
	int AddIcon(HICON hIcon,UINT nIconID=0);
	int AddIcon(LPCTSTR pszIcon);
	// set the column data type primarily for sorting
	void SetColType(int nCol,eColTypes ColType);
	void SetSection(LPCTSTR pszSection);
	void SetSortHeader();
	void SetSortColumn(int nCol,BOOL bAsc);

	// sort by column
	virtual void Sort();
	// save our profiles
	virtual void SaveProfile();
	virtual void LoadProfile();
	void DelayPaint(bool bDelay);
	static CString StripNonNumeric(LPCTSTR pszOldNum,CUIODListCtrl::eColTypes type);
//////////////////
// Overrides
public:
	virtual DROPEFFECT SelectCurrentTarget(CDD_OleDropTargetInfo *pInfo);
	// ovwner draw
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	// ovwner draw
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItem);
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void ConvertToTime(CString &str);
	virtual void AddThousandSeps(CString &str);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	// override if you added a callback row and fill in the structure
	virtual PFNLVCOMPARE GetCompareFunc();
	virtual BOOL GetDispInfo(LV_DISPINFO *pDispInfo);
	virtual BOOL ColumnClick(NM_LISTVIEW* pNMListView);
	virtual BOOL DoubleClick(NM_LISTVIEW* pNMListView);
	virtual BOOL OnEnter(NM_LISTVIEW* pNMListView);
	virtual BOOL OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnSelChanged(int nItem, LRESULT* pResult);
	// return true to delete the item from the list control
	virtual bool DeleteKey(int nRow);
	// called when the user has pressed ALt-Enter
	virtual void ShowProperties(int nRow);
	// called when the user has pressed F5
	virtual void Refresh();
	// called when the user has pressed the backspace key
	virtual void GoBack(int nRow);
	// called when the user has has finshed editing a column
	virtual bool EndLabelEdit(int nRow,int nCol,LPCTSTR pszText);
	virtual void PostNcDestroy();
	virtual int OnToolHitTest(CPoint point,TOOLINFO *pTI) const;
	virtual void PreSubclassWindow();
	virtual int CellRectFromPoint(CPoint & point, RECT * cellrect, int * col) const;
	virtual void AllItemsDeleted();
	virtual void GetImageLists(CImageList **pILSmall,CImageList **pILLarge);
	virtual void CreateImageLists();
	// callback called when a new row is added passing the extension data and row index 
	// not valid for AddTextItem only AddCallbackItem
	virtual CUIListCtrlData *GetNewListCtrlData(DWORD dwData,int nItem);
	// option to show context menu if user uses keyboard or mouse(point in client co-ords)
	virtual void ShowPopupMenu(int nRow,int nCol,CPoint point);
	virtual BOOL PopupMenuItem(int nItem,int nCol,CMenu *pPopup,CPoint point);
	virtual BOOL ItemPrePaint(LPNMLVCUSTOMDRAW  lplvcd,LRESULT *pResult);
	virtual BOOL ItemPostPaint(LPNMLVCUSTOMDRAW  lplvcd,LRESULT *pResult);
	virtual BOOL SubItemPrePaint(LPNMLVCUSTOMDRAW lplvcd,LRESULT *pResult);
	virtual BOOL SubItemPostPaint(LPNMLVCUSTOMDRAW lplvcd,LRESULT *pResult);
	//WM_DROPFILES
	virtual void OnDropFile(int nRow,LPCTSTR pszFile,UINT nFlags);
	// OLE Drag and Drop
	// This called when a drop source is dropped on the tree control
	virtual bool DragDrop(CDD_OleDropTargetInfo *pInfo);
	// This called when a drop source is currently over the tree control
	virtual bool DragOver(CDD_OleDropTargetInfo *pInfo);
	// This called when the user entered the window with a drop source
	virtual bool DragEnter(CDD_OleDropTargetInfo *pInfo);
	// This called when the user leaves the window with a drop source
	virtual bool DragLeave(CDD_OleDropTargetInfo *pInfo);
	// This called when the user initiates a OLE drag and drop
	virtual DROPEFFECT DoDragDrop(int *npRows,COleDataSource *pOleDataSource);
	virtual void Empty();

	void SetNoItemsMess(const CString &strNoItemsMess);
	CString GetNoItemsMess() const;
	bool IsDragDrop();
	bool IsDropFiles();
	void OnRClickHeader(CPoint point,int nColIndex);
	// return a pointer to an extension data object
	CUIListCtrlData *GetListCtrlData(int nItem) const;

	int GetHeaderText(CHeaderCtrl *pHeader,int nPos,CString &strText);
	void ReplaceString(CString &rStr,LPCTSTR pszOldText,LPCTSTR pszNewText);
	// owner draw
	BOOL CalcStringEllipsis(HDC hdc, LPTSTR lpszString, int cchMax, UINT uColWidth);
	// owner draw
	void DrawItemColumn(LPDRAWITEMSTRUCT lpDrawItemStruct, int nSubItem, LPRECT prcClip);
	LPCTSTR GetThousandSep() const;
	LPCTSTR GetDecimalSep() const;
	LPCTSTR GetNegativeSign() const;
	LPCTSTR GetTimeSep() const;
	LPCTSTR GetDateSep() const;
	LPCTSTR GetSection() const;
	BOOL InsertItem(LPTSTR szText,LPARAM lParam,int nImage);
	void ChangeIconSize();
	int GetImageIndex(UINT nIconID);
	// owner draw
	static LPCTSTR MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset);
	void RepaintSelectedItems();
	void DoOleDrag(NM_LISTVIEW* pNMListView,bool bRightMenu);	
	// for sorting

protected:
	COLORREF m_clrBkgnd;
	COLORREF m_clrTextBk;
	COLORREF m_clrText;
	int m_cxClient;
	int m_cxStateImageOffset;
	int m_nImageList;
	BOOL m_bFullRowSel;
	BOOL m_bClientWidthSel;
	BOOL m_bColumnSizing;
	BOOL m_bHasFocus;
	BOOL m_bRClickChangeIconSize;
	BOOL m_bSortAscending;
	BOOL m_bToolTips;
	CUIObjList m_objList;
	int m_nColClicked;
	int m_nSortColumn;
	int m_nImage;
	int m_nItems;
	int m_nSubItems;
	int m_cxSmallIcon;
	int m_cySmallIcon;
	int m_cxLargeIcon;
	int m_cyLargeIcon;
	int *m_pColWidths;
	int *m_pColOrder;
	int *m_pColTypes;
	DWORD m_dwExStyle;
	DWORD m_dwViewType;
	CImageList m_ImageSmall;
	CImageList m_ImageLarge;
	CString m_strHeadings;
	CPoint m_PopupPoint;
	HFONT m_hOrigFont;
	CString m_strSection;
	CMap<UINT,UINT,int,int> m_mapImageIndex;
	CMap<CString,LPCTSTR,int,int> m_mapImageFile;
	CODHeaderCtrl m_HeaderCtrl;	

	afx_msg LRESULT OnGetItemText(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetImageList(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetTextColor(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetTextBkColor(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetBkColor(WPARAM wParam, LPARAM lParam);
///* * Generated message map functions
	//{{AFX_MSG(CUIODListCtrl)
	afx_msg LRESULT OnSetColumnWidth(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnDeleteAllItems(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnDeleteItem(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnDeleteColumn(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnUpdateHeaderWidth(WPARAM wParam,LPARAM lParam);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu,UINT nIndex,BOOL bSysMenu);
	afx_msg void OnContextMenu(CWnd*,CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnViewLargeIcons();
	afx_msg void OnViewSmallIcons();
	afx_msg void OnViewDetails();
	afx_msg void OnViewList();
	afx_msg void OnViewFullRowSelection();
	afx_msg void OnViewColumnOrdering();
	afx_msg void OnViewColumnSizing();
	afx_msg void OnViewGridlines();
	afx_msg void OnViewCheckboxes();
	afx_msg void OnViewTrackSelect();
	afx_msg void OnViewEditColumn();
	afx_msg void OnHeaderRemoveColumn();
	afx_msg void OnHeaderEditText();
	afx_msg void OnHeaderReset();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnBeginRDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnToolTipText(UINT id,NMHDR *pNMHDR,LRESULT *pResult);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg LRESULT OnAppPropertiesKey(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnAppDeleteKey(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnAppRefreshKey(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnAppBackspaceKey(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnAppContextMenuKey(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnAppEditKey(WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	afx_msg LRESULT OnDragDrop(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnDragOver(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnDragEnter(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnDragLeave(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnDDDoDragDrop(WPARAM wParam,LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	static LPCTSTR szEntryHeadings;
	static LPCTSTR szEntryStyle;
	static LPCTSTR szEntryRowSel;
	static LPCTSTR szEntryViewType;
	static LPCTSTR szEntryColumnSizing;
	static LPCTSTR szEntrySortColumn;
	static LPCTSTR szEntrySubItems;
	static LPCTSTR szEntryColOrder;
	static LPCTSTR szEntryColWidths;
	CListDropTarget m_OleDropTarget;
	COleDataSource m_OleDataSource;
	CString m_sToolTipText;
	CString m_strNoItemsMess;
	bool m_bDelayPaint;
	int m_iItemDrop;
	bool m_bDragDrop;
	bool m_bDropFiles;
	bool m_bEditSubItems;
	CWnd *m_pPopupWnd;
protected:
	UINT m_PopupID;
	UINT m_MultiPopupID;
};

/////////////////////
// inlines
/////////////////////
inline void CUIODListCtrl::SetEditSubItems(bool bEdit)
{
	m_bEditSubItems = bEdit;
}

inline void CUIODListCtrl::SetNoItemsMess(const CString &strNoItemsMess)
{
	m_strNoItemsMess = strNoItemsMess;
}

inline CString CUIODListCtrl::GetNoItemsMess() const
{
	return m_strNoItemsMess;
}

inline void CUIODListCtrl::SetPopupWnd(CWnd *pPopupWnd)
{
	m_pPopupWnd = pPopupWnd;
}

inline void CUIODListCtrl::SetPopupID(UINT nPopupID)
{
	m_PopupID = nPopupID;
}

inline void CUIODListCtrl::SetMultiPopupID(UINT nMultiPopupID)
{
	m_MultiPopupID = nMultiPopupID;
}

inline bool CUIODListCtrl::IsDragDrop()
{
	return m_bDragDrop;
}

inline bool CUIODListCtrl::IsDropFiles()
{
	return m_bDropFiles;
}

inline void CUIODListCtrl::SetDragDrop(bool bDragDrop)
{
	m_bDragDrop = bDragDrop;
}

inline void CUIODListCtrl::SetDropFiles(bool bDropFiles)
{
	m_bDropFiles = bDropFiles;
}

inline const COleDropTarget &CUIODListCtrl::GetDropTarget()
{
	return m_OleDropTarget;
}

inline	int CUIODListCtrl::GetIconSize() const
{
	return m_nImageList;
}

inline	int CUIODListCtrl::GetCurSel() const
{
	return GetNextItem(-1,LVNI_SELECTED);
}

inline	int CUIODListCtrl::GetNextSel(int item) const
{
	return GetNextItem(item,LVNI_SELECTED);
}

inline	int CUIODListCtrl::GetCount() const
{
	return GetItemCount();
}

inline void CUIODListCtrl::SetRCLickChangeIconSize(BOOL bSize)
{
	m_bRClickChangeIconSize = bSize;
}

inline void CUIODListCtrl::SetColumnSizing(bool bSet)
{
	m_bColumnSizing = !bSet;
	SaveProfile();
}

inline void CUIODListCtrl::ToggleColumnSizing()
{
	m_bColumnSizing = !m_bColumnSizing;
	SaveProfile();
}

inline void CUIODListCtrl::ToggleFullRowSel()
{
	SetFullRowSel(!m_bFullRowSel);
}

inline BOOL CUIODListCtrl::ToggleHeaderDragDrop()
{
	return SetHeaderDragDrop(!(GetExStyle() & LVS_EX_HEADERDRAGDROP));
}

inline BOOL CUIODListCtrl::SetHeaderDragDrop(bool bSet)
{
	return SetExStyle(LVS_EX_HEADERDRAGDROP,bSet);
}

inline BOOL CUIODListCtrl::ToggleGridLines()
{
	return SetGridLines(!(GetExStyle() & LVS_EX_GRIDLINES));
}

inline BOOL CUIODListCtrl::SetGridLines(bool bSet)
{
	return SetExStyle(LVS_EX_GRIDLINES,bSet);
}

inline BOOL CUIODListCtrl::ToggleTrackSelect()
{
	return SetTrackSelect(!(GetExStyle() & LVS_EX_TRACKSELECT));
}

inline BOOL CUIODListCtrl::SetTrackSelect(bool bSet)
{
	return SetExStyle(LVS_EX_TRACKSELECT,bSet);
}

inline BOOL CUIODListCtrl::ToggleCheckBoxes()
{
	return SetCheckBoxes(!(GetExStyle() & LVS_EX_CHECKBOXES)); 
}

inline BOOL CUIODListCtrl::SetCheckBoxes(bool bSet)
{
	return SetExStyle(LVS_EX_CHECKBOXES,bSet);
}

inline BOOL CUIODListCtrl::ToggleEditLabels()
{
	return SetEditLabels(!(GetStyle() & LVS_EDITLABELS));
}

inline BOOL CUIODListCtrl::SetEditLabels(bool bSet)
{
	return bSet ? ModifyStyle(0,LVS_EDITLABELS) : ModifyStyle(LVS_EDITLABELS,0);
}

inline void CUIODListCtrl::ToggleToolTips()
{
	m_bToolTips = !m_bToolTips;
	SetToolTips(m_bToolTips == TRUE);
}

inline void CUIODListCtrl::SetToolTips(bool bSet)
{
	m_bToolTips = bSet;
	EnableToolTips(m_bToolTips);
}

inline void CUIODListCtrl::SetColType(int nCol,eColTypes ColType)
{
	ASSERT(nCol < m_nSubItems);
	ASSERT(m_pColTypes);
	m_pColTypes[nCol] = ColType;
}

inline int CUIODListCtrl::GetColumnCount() const
{
	return m_nSubItems;
}

inline LPCTSTR CUIODListCtrl::GetDecimalSep() const
{
	return CLocaleInfo::Instance()->GetDecimalSep();
}

inline LPCTSTR CUIODListCtrl::GetThousandSep() const
{
	return CLocaleInfo::Instance()->GetThousandSep();
}

inline LPCTSTR CUIODListCtrl::GetNegativeSign() const
{
	return CLocaleInfo::Instance()->GetNegativeSign();
}

inline LPCTSTR CUIODListCtrl::GetTimeSep() const
{
	return CLocaleInfo::Instance()->GetTimeSep();
}

inline LPCTSTR CUIODListCtrl::GetDateSep() const
{
	return CLocaleInfo::Instance()->GetDateSep();
}

inline void CUIODListCtrl::DelayPaint(bool bDelay)
{
	m_bDelayPaint = bDelay;
}

////////////////////////////////////////////////
// CChangeViewType
////////////////////////////////////////////////
class CChangeViewType
{
public:
	CChangeViewType(CUIODListCtrl *pLC,DWORD dwType);
	virtual ~CChangeViewType();
public:
protected:
private:
	CUIODListCtrl *m_pLC;
	DWORD m_dwType;
};


#endif
