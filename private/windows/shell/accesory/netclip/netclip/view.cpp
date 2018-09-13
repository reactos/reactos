// NetClipView.cpp : implementation of the CNetClipView class
//
// CNetClipView is derived from CRichEditView found in MFC 4.0.
// Normally this would give us a RichEdit control that the user
// could type into, but we set it to read only.
//
// We use the ImportDataObject method of the rich edit control
// to show the clipboard contents. Simple but effective.
//

#include "stdafx.h"
#include "NetClip.h"

#include "Doc.h"
#include "MainFrm.h"
#include "View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetClipView

IMPLEMENT_DYNCREATE(CNetClipView, CRichEditView)

BEGIN_MESSAGE_MAP(CNetClipView, CRichEditView)
	//{{AFX_MSG_MAP(CNetClipView)
	ON_WM_CREATE()
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUTLOCAL, OnUpdateNeedSel)
    ON_COMMAND(ID_EDIT_CUTLOCAL, OnEditCut)
	ON_COMMAND(ID_EDIT_COPYLOCAL, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTELOCAL, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPYLOCAL, OnUpdateNeedSel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetClipView construction/destruction

// Well known clipboard formats that are not pre-defined
// by Windows.
UINT cf_ObjectDiscriptor;
UINT cf_EmbedSource;
UINT cf_LinkSource;
UINT cf_RichTextFormat;
UINT cf_RichTextFormatWithoutObjects;
UINT cf_RichTextAndObjects;

CNetClipView::CNetClipView()
{
    cf_ObjectDiscriptor = RegisterClipboardFormat(_T("Object Descriptor"));
    cf_EmbedSource = RegisterClipboardFormat(_T("Embed Source"));
    cf_LinkSource = RegisterClipboardFormat(_T("Link Source"));
    cf_RichTextFormat = RegisterClipboardFormat(_T("Rich Text Format"));
    cf_RichTextFormatWithoutObjects = RegisterClipboardFormat(_T("Rich Text Format Without Objects"));
    cf_RichTextAndObjects = RegisterClipboardFormat(_T("Rich Text And Objects"));
}

CNetClipView::~CNetClipView()
{
}

// BUGBUG: Remove this since we don't do anything
BOOL CNetClipView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CRichEditView::PreCreateWindow(cs);
}

int CNetClipView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CRichEditView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	GetRichEditCtrl().SetEventMask(ENM_KEYEVENTS|ENM_MOUSEEVENTS|GetRichEditCtrl().GetEventMask());
	
	return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CNetClipView drawing

void CNetClipView::OnDraw(CDC* pDC)
{
    // OnDraw is pure-virtual in the base so we have to 
    // provide at least an empty impl.
}

/////////////////////////////////////////////////////////////////////////////
// CNetClipView diagnostics

#ifdef _DEBUG
void CNetClipView::AssertValid() const
{
	CRichEditView::AssertValid();
}

void CNetClipView::Dump(CDumpContext& dc) const
{
	CRichEditView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNetClipView message handlers

// BUGBUG: Remove this since we don't do anything
void CNetClipView::OnInitialUpdate() 
{
	CRichEditView::OnInitialUpdate();
}

void CNetClipView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
    if (theApp.m_fNoUpdate == TRUE)
    {
        // Don't update
        return;
    }

    BeginWaitCursor();

    CMainFrame* pfrm = (CMainFrame*)AfxGetMainWnd();
    pfrm->SetMessageText(IDS_STATUS_UPDATE);

    // If we have an in-place active item then deactivate and
    // close it. 
    CRichEditCntrItem* pItem = GetInPlaceActiveItem();
    if (pItem)
    {
        TRACE(_T("Deactivating in-place active item\n."));
		pItem->Deactivate();
		pItem->Close(OLECLOSE_NOSAVE);
    }

    GetRichEditCtrl().SetRedraw(FALSE);
    GetRichEditCtrl( ).SetReadOnly(FALSE);
    GetRichEditCtrl( ).SetSel(0, -1);
    GetRichEditCtrl( ).Clear();
    if (pfrm->m_cfDisplay == 0 ||
        pfrm->m_cfDisplay == CF_TEXT || 
        pfrm->m_cfDisplay == CF_OEMTEXT ||
        pfrm->m_cfDisplay == CF_UNICODETEXT)
    {
        /*
        HFONT hfont;
        // Set the font to the default font
        if (pfrm->m_cfDisplay == CF_OEMTEXT)
            hfont = (HFONT)GetStockObject(OEM_FIXED_FONT);
        else
            hfont = (HFONT)GetStockObject(ANSI_VAR_FONT);

        LOGFONT lf;
        //hdc = ::GetDC(GetSafeHwnd());
        GetObject(hfont, sizeof(LOGFONT), &lf);
        */

        CHARFORMAT cfmt;
        memset(&cfmt, '\0', sizeof(CHARFORMAT));
        cfmt.cbSize=sizeof(CHARFORMAT);
        cfmt.dwEffects = CFE_AUTOCOLOR;
        if (pfrm->m_cfDisplay == CF_OEMTEXT)
        {
            cfmt.yHeight = 10;
            cfmt.dwMask = CFM_SIZE | CFM_FACE | CFM_COLOR | CFM_BOLD;
            cfmt.bCharSet = ANSI_CHARSET;
            cfmt.bPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
#if defined(_UNICODE) && !defined(_NTBUILD)
            WideCharToMultiByte(CP_ACP, 0, L"Courier", -1, cfmt.szFaceName, sizeof(cfmt.szFaceName)/sizeof(cfmt.szFaceName[0]), NULL, NULL);
#else
            lstrcpy(cfmt.szFaceName, _T("Courier"));
#endif
        }
        else
        {
            cfmt.yHeight = 10;
            cfmt.dwMask = CFM_SIZE | CFM_FACE | CFM_COLOR | CFM_BOLD;
            cfmt.bCharSet = ANSI_CHARSET;
            cfmt.bPitchAndFamily = VARIABLE_PITCH | FF_SWISS ;
#if defined(_UNICODE) && !defined(_NTBUILD)
            WideCharToMultiByte(CP_ACP, 0, L"MS Sans Serif", -1, cfmt.szFaceName, sizeof(cfmt.szFaceName)/sizeof(cfmt.szFaceName[0]), NULL, NULL);
#else
            lstrcpy(cfmt.szFaceName, _T("MS Sans Serif"));
#endif
        }
        GetRichEditCtrl().SetDefaultCharFormat(cfmt);
    }


    if (pfrm->m_fDisplayAsIcon)
    {
        // TODO: Implement
    }
    else
    {
        IRichEditOle* prich = GetRichEditCtrl().GetIRichEditOle();
        if (prich)
        {
            IDataObject* pdo = NULL;
            if (pfrm->m_pClipboard)
                pfrm->m_pClipboard->GetClipboard(&pdo);
            else
                OleGetClipboard(&pdo);

            pfrm->SetMessageText(IDS_STATUS_GETTING_CLIPDATA);
            if (pdo)
            {
                if (pfrm->m_cfDisplay == CF_OEMTEXT)
                    prich->ImportDataObject(pdo, CF_TEXT, NULL);
                else
                    prich->ImportDataObject(pdo, (CLIPFORMAT)pfrm->m_cfDisplay, NULL);
	            pdo->Release();
            }
            prich->Release();
        }
    }

    GetRichEditCtrl().SetSel(0, 0);
    GetRichEditCtrl().LineScroll(0,0);
    GetRichEditCtrl().SetReadOnly(TRUE);
    GetRichEditCtrl().SetRedraw(TRUE);
    GetRichEditCtrl().InvalidateRect(NULL);
    GetRichEditCtrl().UpdateWindow();

    EndWaitCursor();
    pfrm->SetMessageText(AFX_IDS_IDLEMESSAGE);
}

BOOL CNetClipView::CanDisplay(UINT cf)
{
    switch(cf)
    {
    case CF_TEXT:
    case CF_OEMTEXT:
    case CF_BITMAP:
    case CF_METAFILEPICT:
    case CF_DIB:
    case CF_UNICODETEXT:
    case CF_ENHMETAFILE:
        return TRUE;
    }

    if (
        cf == cf_EmbedSource ||
        cf == cf_LinkSource ||
        cf == cf_RichTextFormat ||
        cf == cf_RichTextFormatWithoutObjects ||
        cf == cf_RichTextAndObjects
        )
        return TRUE;

    return FALSE;
}

HRESULT CNetClipView::QueryAcceptData(LPDATAOBJECT lpdataobj,
	CLIPFORMAT* lpcfFormat, DWORD dwReco, BOOL bReally, HGLOBAL hMetaPict)
{
	ASSERT(lpcfFormat != NULL);
	// if direct pasting a particular native format allow it
	if (!bReally) // not actually pasting
    {
	    if (IsRichEditFormat(*lpcfFormat))
		    return S_OK;
        else
            return S_FALSE;
    }

    return CRichEditView::QueryAcceptData(lpdataobj,lpcfFormat, dwReco, bReally, hMetaPict);
}

void CNetClipView::OnUpdateNeedSel(CCmdUI* pCmdUI)
{
	ASSERT_VALID(this);
    CMainFrame* pfrm = (CMainFrame*)AfxGetMainWnd();
	long nStartChar, nEndChar;
	GetRichEditCtrl().GetSel(nStartChar, nEndChar);
	pCmdUI->Enable(pfrm->m_pClipboard!=NULL && (nStartChar != nEndChar));
	ASSERT_VALID(this);
}

void CNetClipView::OnEditCut() 
{
    CRichEditView::OnEditCut();
}

void CNetClipView::OnEditCopy() 
{
    CRichEditView::OnEditCopy();
}


void CNetClipView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
	ASSERT_VALID(this);
    CMainFrame* pfrm = (CMainFrame*)AfxGetMainWnd();
	pCmdUI->Enable(pfrm->m_pClipboard!=NULL);
	ASSERT_VALID(this);
}
