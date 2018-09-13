// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXRICH2_H__
#define __AFXRICH2_H__

#ifndef __AFXWIN_H__
	#include <afxwin.h>
#endif
#ifndef __AFXDLGS_H__
	#include <afxdlgs.h>
    #include <afxdlgs2.h>
#endif
#ifndef __AFXOLE_H__
	#include <afxole.h>
#endif
#ifndef _RICHEDIT_
	#include <richedit.h>
#endif
#ifndef _RICHOLE_
	#include <richole.h>
	#define _RICHOLE_
#endif
#ifndef __AFXCMN2_H__
	#include <afxcmn2.h>
#endif

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif
#ifndef _AFX_FULLTYPEINFO
#pragma component(mintypeinfo, on)
#endif

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

/////////////////////////////////////////////////////////////////////////////
// AFXRICH - RichEdit2 classes

// Classes declared in this file

//CObject
	//CCmdTarget;
		//CWnd
			//CView
				//CCtrlView
					class CRichEdit2View;// rich text editor view

		//CDocument
			//COleDocument
				class CRichEdit2Doc;
		//CDocItem
			//COleClientItem
				class CRichEdit2CntrItem;

#undef AFX_DATA
#define AFX_DATA

/////////////////////////////////////////////////////////////////////////////
// CRichEdit2View

class _AFX_RICHEDIT2_STATE;  // private to implementation

class CRichEdit2View : public CCtrlView
{
	DECLARE_DYNCREATE(CRichEdit2View)

// Construction
public:
	CRichEdit2View();

// Attributes
public:
	enum WordWrapType
	{
		WrapNone = 0,
		WrapToWindow = 1,
		WrapToTargetDevice = 2
	};
	int m_nWordWrap;
	int m_nBulletIndent;

	void SetPaperSize(CSize sizePaper);
	CSize GetPaperSize() const;
	void SetMargins(const CRect& rectMargin);
	CRect GetMargins() const;
	int GetPrintWidth() const;
	CRect GetPrintRect() const;
	CRect GetPageRect() const;

	//formatting
	CHARFORMAT& GetCharFormatSelection();
	PARAFORMAT& GetParaFormatSelection();
	void SetCharFormat(CHARFORMAT cf);
	void SetParaFormat(PARAFORMAT& pf);
	CRichEdit2CntrItem* GetSelectedItem() const;
	CRichEdit2CntrItem* GetInPlaceActiveItem() const;

	// CEdit control access
	CRichEdit2Ctrl& GetRichEditCtrl() const;
	CRichEdit2Doc* GetDocument() const;

	// other attributes
	long GetTextLength() const;
	static BOOL AFX_CDECL IsRichEdit2Format(CLIPFORMAT cf);
	BOOL CanPaste() const;

// Operations
public:
	void AdjustDialogPosition(CDialog* pDlg);
	HRESULT InsertItem(CRichEdit2CntrItem* pItem);
	void InsertFileAsObject(LPCTSTR lpszFileName);
	BOOL FindText(LPCTSTR lpszFind, BOOL bCase = TRUE, BOOL bWord = TRUE);
	BOOL FindTextSimple(LPCTSTR lpszFind, BOOL bCase = TRUE,
		BOOL bWord = TRUE);
	long PrintInsideRect(CDC* pDC, RECT& rectLayout, long nIndexStart,
		long nIndexStop, BOOL bOutput);
	long PrintPage(CDC* pDC, long nIndexStart, long nIndexStop);
	void DoPaste(COleDataObject& dataobj, CLIPFORMAT cf,
		HMETAFILEPICT hMetaPict);

// Helpers
	void OnCharEffect(DWORD dwMask, DWORD dwEffect);
	void OnUpdateCharEffect(CCmdUI* pCmdUI, DWORD dwMask, DWORD dwEffect) ;
	void OnParaAlign(WORD wAlign);
	void OnUpdateParaAlign(CCmdUI* pCmdUI, WORD wAlign);

// Overrideables
protected:
	virtual BOOL IsSelected(const CObject* pDocItem) const;
	virtual void OnInitialUpdate();
	virtual void OnFindNext(LPCTSTR lpszFind, BOOL bNext, BOOL bCase, BOOL bWord);
	virtual void OnReplaceSel(LPCTSTR lpszFind, BOOL bNext, BOOL bCase,
		BOOL bWord, LPCTSTR lpszReplace);
	virtual void OnReplaceAll(LPCTSTR lpszFind, LPCTSTR lpszReplace,
		BOOL bCase, BOOL bWord);
	virtual void OnTextNotFound(LPCTSTR lpszFind);
	virtual void OnPrinterChanged(const CDC& dcPrinter);
	virtual void WrapChanged();

// Advanced
	virtual BOOL OnPasteNativeObject(LPSTORAGE lpStg);
	virtual HMENU GetContextMenu(WORD, LPOLEOBJECT, CHARRANGE* );
	virtual HRESULT GetClipboardData(CHARRANGE* lpchrg, DWORD dwReco,
		LPDATAOBJECT lpRichDataObj, LPDATAOBJECT* lplpdataobj);
	virtual HRESULT QueryAcceptData(LPDATAOBJECT, CLIPFORMAT*, DWORD,
		BOOL, HGLOBAL);

// Implementation
public:
	LPRICHEDITOLE m_lpRichEditOle;
	CDC m_dcTarget;
	long m_lInitialSearchPos;
	UINT m_nPasteType;
	BOOL m_bFirstSearch;

	void TextNotFound(LPCTSTR lpszFind);
	BOOL FindText(_AFX_RICHEDIT2_STATE* pEditState);
	BOOL FindTextSimple(_AFX_RICHEDIT2_STATE* pEditState);
	long FindAndSelect(DWORD dwFlags, FINDTEXTEX& ft);
	void Stream(CArchive& ar, BOOL bSelection);
	HRESULT GetWindowContext(LPOLEINPLACEFRAME* lplpFrame,
		LPOLEINPLACEUIWINDOW* lplpDoc, LPOLEINPLACEFRAMEINFO lpFrameInfo);
	HRESULT ShowContainerUI(BOOL b);
	static DWORD CALLBACK EditStreamCallBack(DWORD_PTR dwCookie,
		LPBYTE pbBuff, LONG cb, LONG *pcb);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual void Serialize(CArchive& ar);
	virtual void DeleteContents();
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo);

	static AFX_DATA ULONG lMaxSize; // maximum number of characters supported

protected:
	CRect m_rectMargin;
	CSize m_sizePaper;
	CDWordArray m_aPageStart;    // array of starting pages
	PARAFORMAT m_paraformat;
	CHARFORMAT m_charformat;
	BOOL m_bSyncCharFormat;
	BOOL m_bSyncParaFormat;

	// construction
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	// printing support
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo = NULL);
	BOOL PaginateTo(CDC* pDC, CPrintInfo* pInfo);

	// find & replace support
	void OnEditFindReplace(BOOL bFindOnly);
	BOOL SameAsSelected(LPCTSTR lpszCompare, BOOL bCase, BOOL bWord);

	// special overrides for implementation

	//{{AFX_MSG(CRichEdit2View)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnUpdateNeedSel(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNeedClip(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNeedText(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNeedFind(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditClear();
	afx_msg void OnEditUndo();
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditFind();
	afx_msg void OnEditReplace();
	afx_msg void OnEditRepeat();
	afx_msg void OnDestroy();
	afx_msg void OnEditProperties();
	afx_msg void OnUpdateEditProperties(CCmdUI* pCmdUI);
	afx_msg void OnInsertObject();
	afx_msg void OnCancelEditCntr();
	afx_msg void OnCharBold();
	afx_msg void OnUpdateCharBold(CCmdUI* pCmdUI);
	afx_msg void OnCharItalic();
	afx_msg void OnUpdateCharItalic(CCmdUI* pCmdUI);
	afx_msg void OnCharUnderline();
	afx_msg void OnUpdateCharUnderline(CCmdUI* pCmdUI);
	afx_msg void OnParaCenter();
	afx_msg void OnUpdateParaCenter(CCmdUI* pCmdUI);
	afx_msg void OnParaLeft();
	afx_msg void OnUpdateParaLeft(CCmdUI* pCmdUI);
	afx_msg void OnParaRight();
	afx_msg void OnUpdateParaRight(CCmdUI* pCmdUI);
	afx_msg void OnBullet();
	afx_msg void OnUpdateBullet(CCmdUI* pCmdUI);
	afx_msg void OnFormatFont();
	afx_msg void OnColorPick(COLORREF cr);
	afx_msg void OnColorDefault();
	afx_msg void OnEditPasteSpecial();
	afx_msg void OnUpdateEditPasteSpecial(CCmdUI* pCmdUI);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnDevModeChange(LPTSTR lpDeviceName);
	//}}AFX_MSG
	afx_msg LRESULT OnFindReplaceCmd(WPARAM, LPARAM lParam);
	afx_msg void OnSelChange(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

// Interface Map
public:
	BEGIN_INTERFACE_PART(RichEditOleCallback, IRichEditOleCallback)
		INIT_INTERFACE_PART(CRichEdit2View, RichEditOleCallback)
		STDMETHOD(GetNewStorage) (LPSTORAGE*);
		STDMETHOD(GetInPlaceContext) (LPOLEINPLACEFRAME*,
									  LPOLEINPLACEUIWINDOW*,
									  LPOLEINPLACEFRAMEINFO);
		STDMETHOD(ShowContainerUI) (BOOL);
		STDMETHOD(QueryInsertObject) (LPCLSID, LPSTORAGE, LONG);
		STDMETHOD(DeleteObject) (LPOLEOBJECT);
		STDMETHOD(QueryAcceptData) (LPDATAOBJECT, CLIPFORMAT*, DWORD,BOOL, HGLOBAL);
		STDMETHOD(ContextSensitiveHelp) (BOOL);
		STDMETHOD(GetClipboardData) (CHARRANGE*, DWORD, LPDATAOBJECT*);
		STDMETHOD(GetDragDropEffect) (BOOL, DWORD, LPDWORD);
		STDMETHOD(GetContextMenu) (WORD, LPOLEOBJECT, CHARRANGE*, HMENU*);
	END_INTERFACE_PART(RichEditOleCallback)

	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CRichEdit2Doc

class CRichEdit2Doc : public COleServerDoc
{
protected: // create from serialization only
	CRichEdit2Doc();
	DECLARE_DYNAMIC(CRichEdit2Doc)

// Attributes
public:
	BOOL m_bRTF;        // TRUE when formatted, FALSE when plain text
    BOOL m_bUnicode;    // TRUE if the doc is Unicode

	virtual CRichEdit2CntrItem* CreateClientItem(REOBJECT* preo = NULL) const = 0;

	virtual CRichEdit2View* GetView() const;
	int GetStreamFormat() const;
    BOOL IsUnicode() const;

// Implementation
protected:
	virtual COleServerItem* OnGetEmbeddedItem();
	void MarkItemsClear() const;
	void DeleteUnmarkedItems() const;
	void UpdateObjectCache();
public:
	BOOL m_bUpdateObjectCache;
	virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU);
	virtual void SetTitle(LPCTSTR lpszTitle);
	virtual COleClientItem* GetPrimarySelectedItem(CView* pView);
	virtual void DeleteContents();
	virtual POSITION GetStartPosition() const;
	virtual void PreCloseFrame(CFrameWnd* pFrameWnd);
	virtual void UpdateModifiedFlag();
	virtual BOOL IsModified();
	virtual void SetModifiedFlag(BOOL bModified = TRUE);
	virtual COleClientItem* GetInPlaceActiveItem(CWnd* pWnd);
	CRichEdit2CntrItem* LookupItem(LPOLEOBJECT lpobj) const;
	void InvalidateObjectCache();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CRichEdit2CntrItem

class CRichEdit2CntrItem : public COleClientItem
{
	DECLARE_SERIAL(CRichEdit2CntrItem)

// Constructors
public:
	CRichEdit2CntrItem(REOBJECT* preo = NULL, CRichEdit2Doc* pContainer = NULL);
		// Note: pContainer is allowed to be NULL to enable IMPLEMENT_SERIALIZE.
		//  IMPLEMENT_SERIALIZE requires the class have a constructor with
		//  zero arguments.  Normally, OLE items are constructed with a
		//  non-NULL document pointer.

// Operations
	void SyncToRichEditObject(REOBJECT& reo);

// Implementation
public:
	~CRichEdit2CntrItem();
	LPOLECLIENTSITE m_lpClientSite;
	BOOL m_bMark;
	BOOL m_bLock;   // lock it during creation to avoid deletion
	void Mark(BOOL b);
	BOOL IsMarked();
	CRichEdit2Doc* GetDocument();
	CRichEdit2View* GetActiveView();
	HRESULT ShowContainerUI(BOOL b);
	HRESULT GetWindowContext(LPOLEINPLACEFRAME* lplpFrame,
		LPOLEINPLACEUIWINDOW* lplpDoc, LPOLEINPLACEFRAMEINFO lpFrameInfo);
	virtual LPOLECLIENTSITE GetClientSite();
	virtual BOOL ConvertTo(REFCLSID clsidNew);
	virtual BOOL ActivateAs(LPCTSTR lpszUserType, REFCLSID clsidOld,
		REFCLSID clsidNew);
	virtual void SetDrawAspect(DVASPECT nDrawAspect);
	virtual void OnDeactivateUI(BOOL bUndoable);
	virtual BOOL CanActivate();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual BOOL OnChangeItemPosition(const CRect& rectPos);
};

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#ifdef _AFX_ENABLE_INLINES
#define _AFXRICH_INLINE inline
#include <afxrich2.inl>
#endif

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif
#ifndef _AFX_FULLTYPEINFO
#pragma component(mintypeinfo, off)
#endif

#endif //__AFXRICH2_H__

/////////////////////////////////////////////////////////////////////////////
