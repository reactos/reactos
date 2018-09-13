// wordpvw.cpp : implementation of the CWordPadView class
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.


#include "stdafx.h"
#include "wordpad.h"
#include "cntritem.h"
#include "srvritem.h"

#include "wordpdoc.h"
#include "wordpvw.h"
#include "formatta.h"
#include "datedial.h"
#include "formatpa.h"
#include "formatba.h"
#include "ruler.h"
#include "strings.h"
#include "colorlis.h"
#include "pageset.h"
#include <penwin.h>
#include "fixhelp.h"


extern CLIPFORMAT cfEmbeddedObject;
extern CLIPFORMAT cfRTO;

BOOL g_fInternalDragDrop = FALSE ;
BOOL g_fRightButtonDrag = FALSE;

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BOOL CCharFormat::operator==(CCharFormat& cf)
{
    return
        dwMask == cf.dwMask
        && dwEffects == cf.dwEffects
        && yHeight == cf.yHeight
        && yOffset == cf.yOffset
        && crTextColor == cf.crTextColor
        && bPitchAndFamily == cf.bPitchAndFamily
        && (lstrcmp(szFaceName, cf.szFaceName) == 0);
}

BOOL CParaFormat::operator==(PARAFORMAT& pf)
{
    if(
        dwMask != pf.dwMask
        || wNumbering != pf.wNumbering
        || wReserved != pf.wReserved
        || dxStartIndent != pf.dxStartIndent
        || dxRightIndent != pf.dxRightIndent
        || dxOffset != pf.dxOffset
        || cTabCount != pf.cTabCount
        )
    {
        return FALSE;
    }
    for (int i=0;i<pf.cTabCount;i++)
    {
        if (rgxTabs[i] != pf.rgxTabs[i])
            return FALSE;
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CWordPadView

IMPLEMENT_DYNCREATE(CWordPadView, CRichEdit2View)

//WM_WININICHANGE -- default printer might have changed
//WM_FONTCHANGE -- pool of fonts changed
//WM_DEVMODECHANGE -- printer settings changes

BEGIN_MESSAGE_MAP(CWordPadView, CRichEdit2View)
    ON_COMMAND(ID_OLE_INSERT_NEW, OnInsertObject)
    ON_COMMAND(ID_CANCEL_EDIT_CNTR, OnCancelEditCntr)
    ON_COMMAND(ID_CANCEL_EDIT_SRVR, OnCancelEditSrvr)
    //{{AFX_MSG_MAP(CWordPadView)
    ON_COMMAND(ID_PAGE_SETUP, OnPageSetup)
    ON_COMMAND(ID_CHAR_BOLD, OnCharBold)
    ON_UPDATE_COMMAND_UI(ID_CHAR_BOLD, OnUpdateCharBold)
    ON_COMMAND(ID_CHAR_ITALIC, OnCharItalic)
    ON_UPDATE_COMMAND_UI(ID_CHAR_ITALIC, OnUpdateCharItalic)
    ON_COMMAND(ID_CHAR_UNDERLINE, OnCharUnderline)
    ON_UPDATE_COMMAND_UI(ID_CHAR_UNDERLINE, OnUpdateCharUnderline)
    ON_COMMAND(ID_PARA_CENTER, OnParaCenter)
    ON_UPDATE_COMMAND_UI(ID_PARA_CENTER, OnUpdateParaCenter)
    ON_COMMAND(ID_PARA_LEFT, OnParaLeft)
    ON_UPDATE_COMMAND_UI(ID_PARA_LEFT, OnUpdateParaLeft)
    ON_COMMAND(ID_PARA_RIGHT, OnParaRight)
    ON_UPDATE_COMMAND_UI(ID_PARA_RIGHT, OnUpdateParaRight)
    ON_WM_CREATE()
    ON_COMMAND(ID_INSERT_DATE_TIME, OnInsertDateTime)
   ON_COMMAND(ID_FORMAT_PARAGRAPH, OnFormatParagraph)
   ON_COMMAND(ID_FORMAT_FONT, OnFormatFont)
    ON_COMMAND(ID_EDIT_PASTE_SPECIAL, OnEditPasteSpecial)
    ON_COMMAND(ID_OLE_EDIT_PROPERTIES, OnEditProperties)
    ON_COMMAND(ID_EDIT_FIND, OnEditFind)
    ON_COMMAND(ID_EDIT_REPLACE, OnEditReplace)
    ON_COMMAND(ID_FORMAT_TABS, OnFormatTabs)
    ON_COMMAND(ID_COLOR16, OnColorDefault)
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_WM_MEASUREITEM()
    ON_COMMAND(ID_PEN_BACKSPACE, OnPenBackspace)
    ON_COMMAND(ID_PEN_NEWLINE, OnPenNewline)
    ON_COMMAND(ID_PEN_PERIOD, OnPenPeriod)
    ON_COMMAND(ID_PEN_SPACE, OnPenSpace)
    ON_WM_SIZE()
    ON_WM_KEYDOWN()
    ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrint)
    ON_WM_DROPFILES()
    ON_COMMAND(ID_PEN_LENS, OnPenLens)
    ON_COMMAND(ID_PEN_TAB, OnPenTab)
   ON_COMMAND(ID_DELAYED_INVALIDATE, OnDelayedInvalidate)
    ON_WM_PALETTECHANGED()
    ON_WM_QUERYNEWPALETTE()
    ON_WM_WININICHANGE()
    //}}AFX_MSG_MAP
    // Standard printing commands
    ON_COMMAND(ID_INSERT_BULLET, CRichEdit2View::OnBullet)
    ON_UPDATE_COMMAND_UI(ID_INSERT_BULLET, CRichEdit2View::OnUpdateBullet)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
    ON_COMMAND_RANGE(ID_COLOR0, ID_COLOR16, OnColorPick)
    ON_EN_CHANGE(AFX_IDW_PANE_FIRST, OnEditChange)
    ON_WM_MOUSEACTIVATE()
    ON_REGISTERED_MESSAGE(CWordPadApp::m_nPrinterChangedMsg, OnPrinterChangedMsg)
    ON_NOTIFY(FN_GETFORMAT, ID_VIEW_FORMATBAR, OnGetCharFormat)
    ON_NOTIFY(FN_SETFORMAT, ID_VIEW_FORMATBAR, OnSetCharFormat)
    ON_NOTIFY(NM_SETFOCUS, ID_VIEW_FORMATBAR, OnBarSetFocus)
    ON_NOTIFY(NM_KILLFOCUS, ID_VIEW_FORMATBAR, OnBarKillFocus)
    ON_NOTIFY(NM_RETURN, ID_VIEW_FORMATBAR, OnBarReturn)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWordPadView construction/destruction

CWordPadView::CWordPadView()
{
    m_bSyncCharFormat = m_bSyncParaFormat = TRUE;
    m_uTimerID = 0;
    m_bDelayUpdateItems = FALSE;
    m_bOnBar = FALSE;
    m_bInPrint = FALSE;
    m_nPasteType = 0;
    m_rectMargin = theApp.m_rectPageMargin;
}

BOOL CWordPadView::PreCreateWindow(CREATESTRUCT& cs)
{
    BOOL bRes = CRichEdit2View::PreCreateWindow(cs);
    cs.style |= ES_SELECTIONBAR;
    return bRes;
}


 
/////////////////////////////////////////////////////////////////////////////
// CWordPadView attributes

BOOL CWordPadView::IsFormatText()
{
    // this function checks to see if any formatting is not default text
    BOOL bRes = FALSE;
    CHARRANGE cr;
    CCharFormat cf;
    CParaFormat pf;
    GetRichEditCtrl().GetSel(cr);
    GetRichEditCtrl().HideSelection(TRUE, FALSE);
    GetRichEditCtrl().SetSel(0,-1);

    if (!(GetRichEditCtrl().GetSelectionType() & (SEL_OBJECT|SEL_MULTIOBJECT)))
    {
      GetRichEditCtrl().GetSelectionCharFormat(cf);

      //
      // Richedit sometimes returns these masks which are not important to us
      //

      cf.dwMask &= ~(CFM_LINK | CFM_CHARSET) ;

      //
      // Richedit sometimes returns the wrong thing here.  This is not that
      // important for the CHARFORMAT comparison, but it fouls things up if
      // we don't work around it.
      //

      cf.bPitchAndFamily = m_defTextCharFormat.bPitchAndFamily ;

      if (cf == m_defTextCharFormat)
        {
            GetRichEditCtrl().GetParaFormat(pf);

            if (pf == m_defParaFormat) //compared using CParaFormat::operator==
                bRes = TRUE;
        }
    }

    GetRichEditCtrl().SetSel(cr);
    GetRichEditCtrl().HideSelection(FALSE, FALSE);
    return bRes;
}

HMENU CWordPadView::GetContextMenu(WORD, LPOLEOBJECT, CHARRANGE* )
{
    CRichEdit2CntrItem* pItem = GetSelectedItem();
    if (pItem == NULL || !pItem->IsInPlaceActive())
    {
        CMenu menuText;
        menuText.LoadMenu(IDR_TEXT_POPUP);
        CMenu* pMenuPopup = menuText.GetSubMenu(0);
        menuText.RemoveMenu(0, MF_BYPOSITION);
        if (!GetSystemMetrics(SM_PENWINDOWS))
        {
            //delete pen specific stuff
            // remove Insert Keystrokes
            pMenuPopup->DeleteMenu(ID_PEN_LENS, MF_BYCOMMAND);
            int nIndex = pMenuPopup->GetMenuItemCount()-1; //index of last item
            // remove Edit Text...
            pMenuPopup->DeleteMenu(nIndex, MF_BYPOSITION);
            // remove separator
            pMenuPopup->DeleteMenu(nIndex-1, MF_BYPOSITION);
        }
        return pMenuPopup->Detach();
    }
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CWordPadView operations

void CWordPadView::WrapChanged()
{
    CWaitCursor wait;
    CFrameWnd* pFrameWnd = GetParentFrame();
    ASSERT(pFrameWnd != NULL);
    pFrameWnd->SetMessageText(IDS_FORMATTING);
    CWnd* pBarWnd = pFrameWnd->GetMessageBar();
    if (pBarWnd != NULL)
        pBarWnd->UpdateWindow();

    CRichEdit2View::WrapChanged();

    pFrameWnd->SetMessageText(AFX_IDS_IDLEMESSAGE);
    if (pBarWnd != NULL)
        pBarWnd->UpdateWindow();
}

void CWordPadView::SetUpdateTimer()
{
    if (m_uTimerID != 0) // if outstanding timer kill it
        KillTimer(m_uTimerID);
    m_uTimerID = SetTimer(1, 1000, NULL); //set a timer for 1000 milliseconds
    if (m_uTimerID == 0) // no timer available so force update now
        GetDocument()->UpdateAllItems(NULL);
    else
        m_bDelayUpdateItems = TRUE;
}

void CWordPadView::DeleteContents()
{
    ASSERT_VALID(this);
    ASSERT(m_hWnd != NULL);
    CRichEdit2View::DeleteContents();
    SetDefaultFont(IsTextType(GetDocument()->m_nNewDocType));
}

void CWordPadView::SetDefaultFont(BOOL bText)
{
    ASSERT_VALID(this);
    ASSERT(m_hWnd != NULL);
    m_bSyncCharFormat = m_bSyncParaFormat = TRUE;
    CHARFORMAT* pCharFormat = bText ? &m_defTextCharFormat : &m_defCharFormat;
    // set the default character format -- the FALSE makes it the default
    GetRichEditCtrl().SetSel(0,-1);
    GetRichEditCtrl().SetDefaultCharFormat(*pCharFormat);
    GetRichEditCtrl().SetSelectionCharFormat(*pCharFormat);

    GetRichEditCtrl().SetParaFormat(m_defParaFormat);

    GetRichEditCtrl().SetSel(0,0);
    GetRichEditCtrl().EmptyUndoBuffer();
    GetRichEditCtrl().SetModify(FALSE);
    ASSERT_VALID(this);
}

/////////////////////////////////////////////////////////////////////////////
// CWordPadView drawing

/////////////////////////////////////////////////////////////////////////////
// CWordPadView printing

void CWordPadView::OnBeginPrinting(CDC* pDC, CPrintInfo* printInfo)
{
    ASSERT_VALID(this);
    ASSERT_VALID(pDC);
    // initialize page start vector
    ASSERT(m_aPageStart.GetSize() == 0);
    ASSERT(NULL != printInfo);
    ASSERT(NULL != printInfo->m_pPD);

    OnPrinterChanged(*pDC);

    m_aPageStart.Add(0);
    ASSERT(m_aPageStart.GetSize() > 0);

    if (printInfo->m_pPD->PrintSelection())
    {
        CHARRANGE   range;

        GetRichEditCtrl().GetSel(range);
        m_aPageStart[0] = range.cpMin;
    }

    GetRichEditCtrl().FormatRange(NULL, FALSE); // required by RichEdit to clear out cache

    ASSERT_VALID(this);
}

void CWordPadView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{
    ASSERT_VALID(this);
    ASSERT_VALID(pDC);
    ASSERT(pInfo != NULL);
    ASSERT(pInfo->m_bContinuePrinting);
    ASSERT(NULL != pInfo->m_pPD);

    UINT nPage              = pInfo->m_nCurPage;
    ASSERT(nPage <= (UINT)m_aPageStart.GetSize());

    long nIndex             = (long) m_aPageStart[nPage-1];
    BOOL bPrintSelection    = pInfo->m_pPD->PrintSelection();
    long nFinalCharIndex;

    if (bPrintSelection)
    {
        CHARRANGE range;

        GetRichEditCtrl().GetSel(range);
        
        nFinalCharIndex = range.cpMax;
    }
    else
    {
        GETTEXTLENGTHEX textlen;

        textlen.flags = GTL_DEFAULT;
#ifdef UNICODE
        textlen.codepage = 1200;            // Unicode code page
#else
        textlen.codepage = CP_ACP;
#endif

        nFinalCharIndex = this->SendMessage(
                                    EM_GETTEXTLENGTHEX, 
                                    (WPARAM) &textlen,
                                    0);
    }

    // print as much as possible in the current page.
    nIndex = PrintPage(pDC, nIndex, nFinalCharIndex);

    if (nIndex >= nFinalCharIndex)
    {
        TRACE0("End of Document\n");
        pInfo->SetMaxPage(nPage);
    }

    // update pagination information for page just printed
    if (nPage == (UINT)m_aPageStart.GetSize())
    {
        if (nIndex < nFinalCharIndex)
            m_aPageStart.Add(nIndex);
    }
    else
    {
        ASSERT(nPage+1 <= (UINT)m_aPageStart.GetSize());
        ASSERT(nIndex == (long)m_aPageStart[nPage+1-1]);
    }

    if (pInfo != NULL && pInfo->m_bPreview)
        DrawMargins(pDC);
}

void CWordPadView::DrawMargins(CDC* pDC)
{
    if (pDC->m_hAttribDC != NULL)
    {
        CRect rect;
        rect.left = m_rectMargin.left;
        rect.right = m_sizePaper.cx - m_rectMargin.right;
        rect.top = m_rectMargin.top;
        rect.bottom = m_sizePaper.cy - m_rectMargin.bottom;
        //rect in twips
        int logx = ::GetDeviceCaps(pDC->m_hDC, LOGPIXELSX);
        int logy = ::GetDeviceCaps(pDC->m_hDC, LOGPIXELSY);
        rect.left = MulDiv(rect.left, logx, 1440);
        rect.right = MulDiv(rect.right, logx, 1440);
        rect.top = MulDiv(rect.top, logy, 1440);
        rect.bottom = MulDiv(rect.bottom, logy, 1440);
        CPen pen(PS_DOT, 0, pDC->GetTextColor());
        CPen* ppen = pDC->SelectObject(&pen);
        pDC->MoveTo(0, rect.top);
        pDC->LineTo(10000, rect.top);
        pDC->MoveTo(rect.left, 0);
        pDC->LineTo(rect.left, 10000);
        pDC->MoveTo(0, rect.bottom);
        pDC->LineTo(10000, rect.bottom);
        pDC->MoveTo(rect.right, 0);
        pDC->LineTo(rect.right, 10000);
        pDC->SelectObject(ppen);
    }
}

BOOL CWordPadView::OnPreparePrinting(CPrintInfo* pInfo)
{
   CWordPadApp *pApp = NULL ;

   pApp = (CWordPadApp *) AfxGetApp() ;

   if (NULL != pApp)
   {
       if ( (pApp->cmdInfo.m_nShellCommand == CCommandLineInfo::FilePrintTo) ||
            (pApp->cmdInfo.m_nShellCommand == CCommandLineInfo::FilePrint) )
       {
           if (pInfo->m_pPD->m_pd.hDevNames == NULL)
           {
               HGLOBAL hDn = pApp->GetDevNames() ;

               if (hDn != NULL)
               {
                   pInfo->m_pPD->m_pd.hDevNames = hDn ;
               }
           }
       }
   }
        
    if (SEL_EMPTY != GetRichEditCtrl().GetSelectionType())
    {
        pInfo->m_pPD->m_pd.Flags = pInfo->m_pPD->m_pd.Flags & ~PD_NOSELECTION;
    }
    
    return DoPreparePrinting(pInfo);
}


void CWordPadView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
    ASSERT_VALID(this);
    ASSERT_VALID(pDC);
    ASSERT(pInfo != NULL);  // overriding OnPaint -- never get this.

    pDC->SetMapMode(MM_TEXT);

    if (pInfo->m_nCurPage > (UINT)m_aPageStart.GetSize() &&
        !PaginateTo(pDC, pInfo))
    {
        // can't paginate to that page, thus cannot print it.
        pInfo->m_bContinuePrinting = FALSE;
    }
    ASSERT_VALID(this);
}

BOOL CWordPadView::PaginateTo(CDC* pDC, CPrintInfo* pInfo)
    // attempts pagination to pInfo->m_nCurPage, TRUE == success
{
    ASSERT_VALID(this);
    ASSERT_VALID(pDC);

    CRect rectSave = pInfo->m_rectDraw;
    UINT nPageSave = pInfo->m_nCurPage;
    ASSERT(nPageSave > 1);
    ASSERT(nPageSave >= (UINT)m_aPageStart.GetSize());
    pDC->IntersectClipRect(0, 0, 0, 0);
    pInfo->m_nCurPage = m_aPageStart.GetSize();
    while (pInfo->m_nCurPage < nPageSave)
    {
        ASSERT(pInfo->m_nCurPage == (UINT)m_aPageStart.GetSize());
        OnPrepareDC(pDC, pInfo);
        ASSERT(pInfo->m_bContinuePrinting);
        pInfo->m_rectDraw.SetRect(0, 0,
            pDC->GetDeviceCaps(HORZRES), pDC->GetDeviceCaps(VERTRES));
        pDC->DPtoLP(&pInfo->m_rectDraw);
        OnPrint(pDC, pInfo);
        if (pInfo->m_nCurPage == (UINT)m_aPageStart.GetSize())
            break;
        ++pInfo->m_nCurPage;
    }
    BOOL bResult = pInfo->m_nCurPage == nPageSave;
    pInfo->m_nCurPage = nPageSave;
    pInfo->m_rectDraw = rectSave;
   pDC->SelectClipRgn(NULL) ;
    ASSERT_VALID(this);
    return bResult;
}


/////////////////////////////////////////////////////////////////////////////
// OLE Client support and commands

inline int roundleast(int n)
{
    int mod = n%10;
    n -= mod;
    if (mod >= 5)
        n += 10;
    else if (mod <= -5)
        n -= 10;
    return n;
}

static void RoundRect(LPRECT r1)
{
    r1->left = roundleast(r1->left);
    r1->right = roundleast(r1->right);
    r1->top = roundleast(r1->top);
    r1->bottom = roundleast(r1->bottom);
}

static void MulDivRect(LPRECT r1, LPRECT r2, int num, int div)
{
    r1->left = MulDiv(r2->left, num, div);
    r1->top = MulDiv(r2->top, num, div);
    r1->right = MulDiv(r2->right, num, div);
    r1->bottom = MulDiv(r2->bottom, num, div);
}

void CWordPadView::OnPageSetup()
{
    theApp.EnsurePrinterIsInitialized();

    CPageSetupDialog dlg;
    PAGESETUPDLG& psd = dlg.m_psd;
    BOOL bMetric = theApp.GetUnits() == 1; //centimeters
   BOOL fUpdateWrap = FALSE ;
    psd.Flags |= PSD_MARGINS | (bMetric ? PSD_INHUNDREDTHSOFMILLIMETERS :
        PSD_INTHOUSANDTHSOFINCHES);
    int nUnitsPerInch = bMetric ? 2540 : 1000;
    MulDivRect(&psd.rtMargin, m_rectMargin, nUnitsPerInch, 1440);
    RoundRect(&psd.rtMargin);
    // get the current device from the app
    PRINTDLG pd;
    pd.hDevNames = NULL;
    pd.hDevMode = NULL;
    theApp.GetPrinterDeviceDefaults(&pd);
    psd.hDevNames = pd.hDevNames;
    psd.hDevMode = pd.hDevMode;

    SetHelpFixHook() ;

    if (dlg.DoModal() == IDOK)
    {
        RoundRect(&psd.rtMargin);
        MulDivRect(m_rectMargin, &psd.rtMargin, 1440, nUnitsPerInch);
        theApp.m_rectPageMargin = m_rectMargin;
        
        //
        // SelectPrinter will free the existing devnames and devmodes if the
        // third parameter is TRUE.  We don't want to do that because the 
        // print dialog frees them and allocates new ones.
        //

        theApp.SelectPrinter(psd.hDevNames, psd.hDevMode, FALSE);
        theApp.NotifyPrinterChanged();
        fUpdateWrap = TRUE ;
    }

    RemoveHelpFixHook() ;

    // PageSetupDlg failed
    if (CommDlgExtendedError() != 0)
    {
        CPageSetupDlg dlg;
        dlg.m_nBottomMargin = m_rectMargin.bottom;
        dlg.m_nLeftMargin = m_rectMargin.left;
        dlg.m_nRightMargin = m_rectMargin.right;
        dlg.m_nTopMargin = m_rectMargin.top;
        if (dlg.DoModal() == IDOK)
        {
            m_rectMargin.SetRect(dlg.m_nLeftMargin, dlg.m_nTopMargin,
                dlg.m_nRightMargin, dlg.m_nBottomMargin);
            // m_page will be changed at this point
            theApp.m_rectPageMargin = m_rectMargin;
            theApp.NotifyPrinterChanged();
         fUpdateWrap = TRUE ;
        }
    }

   if (fUpdateWrap)
   {
       CRichEdit2View::WrapChanged();
   }
}

/////////////////////////////////////////////////////////////////////////////
// OLE Server support

// The following command handler provides the standard keyboard
//  user interface to cancel an in-place editing session.  Here,
//  the server (not the container) causes the deactivation.
void CWordPadView::OnCancelEditSrvr()
{
    GetDocument()->OnDeactivateUI(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CWordPadView diagnostics

#ifdef _DEBUG
void CWordPadView::AssertValid() const
{
    CRichEdit2View::AssertValid();
}

void CWordPadView::Dump(CDumpContext& dc) const
{
    CRichEdit2View::Dump(dc);
}

CWordPadDoc* CWordPadView::GetDocument() // non-debug version is inline
{
    return (CWordPadDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWordPadView message helpers

/////////////////////////////////////////////////////////////////////////////
// CWordPadView message handlers

int CWordPadView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CRichEdit2View::OnCreate(lpCreateStruct) == -1)
        return -1;
    theApp.m_listPrinterNotify.AddTail(m_hWnd);

    if (theApp.m_bWordSel)
        GetRichEditCtrl().SetOptions(ECOOP_OR, ECO_AUTOWORDSELECTION);
    else
        GetRichEditCtrl().SetOptions(ECOOP_AND, ~(DWORD)ECO_AUTOWORDSELECTION);
//      GetRichEditCtrl().SetOptions(ECOOP_OR, ECO_SELECTIONBAR);

    GetDefaultFont(m_defTextCharFormat, IDS_DEFAULTTEXTFONT);
    GetDefaultFont(m_defCharFormat, IDS_DEFAULTFONT);
        
    GetRichEditCtrl().GetParaFormat(m_defParaFormat);
    m_defParaFormat.cTabCount = 0;

   //
   // Insert our own wrapper interface callback here to get around MFC defaults
   //

   VERIFY(GetRichEditCtrl().SetOLECallback(&m_xWordPadRichEditOleCallback));

    return 0;
}

void CWordPadView::GetDefaultFont(CCharFormat& cf, UINT nFontNameID)
{
    USES_CONVERSION;
    CString strDefFont;
    VERIFY(strDefFont.LoadString(nFontNameID));
    ASSERT(cf.cbSize == sizeof(CHARFORMAT));
    cf.dwMask = CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE|CFM_STRIKEOUT|CFM_SIZE|
        CFM_COLOR|CFM_OFFSET|CFM_PROTECTED;
    cf.dwEffects = CFE_AUTOCOLOR;
    cf.yHeight = 200; //10pt
    cf.yOffset = 0;
    cf.crTextColor = RGB(0, 0, 0);
    cf.bCharSet = 0;
    cf.bPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    ASSERT(strDefFont.GetLength() < LF_FACESIZE);
    lstrcpyn(cf.szFaceName, strDefFont, LF_FACESIZE);
    cf.dwMask |= CFM_FACE;
}

void CWordPadView::OnInsertDateTime()
{
    CDateDialog dlg;
    if (dlg.DoModal() == IDOK)
    {
        GetRichEditCtrl().ReplaceSel(dlg.m_strSel, TRUE);
    }
}

void CWordPadView::OnFormatParagraph()
{
    CFormatParaDlg dlg(GetParaFormatSelection());
    dlg.m_nWordWrap = m_nWordWrap;
    if (dlg.DoModal() == IDOK)
        SetParaFormat(dlg.m_pf);
}

void CWordPadView::OnFormatTabs()
{
    CFormatTabDlg dlg(GetParaFormatSelection());
    if (dlg.DoModal() == IDOK)
        SetParaFormat(dlg.m_pf);
}

void CWordPadView::OnTextNotFound(LPCTSTR lpStr)
{
    ASSERT_VALID(this);
    MessageBeep(0);
    AfxMessageBox(IDS_FINISHED_SEARCH,MB_OK|MB_ICONINFORMATION);
    CRichEdit2View::OnTextNotFound(lpStr);
}

void CWordPadView::OnColorPick(UINT nID)
{
    CRichEdit2View::OnColorPick(CColorMenu::GetColor(nID));
}

void CWordPadView::OnTimer(UINT nIDEvent)
{
    if (m_uTimerID != nIDEvent) // not our timer
        CRichEdit2View::OnTimer(nIDEvent);
    else
    {
        KillTimer(m_uTimerID); // kill one-shot timer
        m_uTimerID = 0;
        if (m_bDelayUpdateItems)
            GetDocument()->UpdateAllItems(NULL);
        m_bDelayUpdateItems = FALSE;
    }
}

void CWordPadView::OnEditChange()
{
    SetUpdateTimer();
}

void CWordPadView::OnDestroy()
{
    POSITION pos = theApp.m_listPrinterNotify.Find(m_hWnd);
    ASSERT(pos != NULL);
    theApp.m_listPrinterNotify.RemoveAt(pos);

    CRichEdit2View::OnDestroy();
    
    if (m_uTimerID != 0) // if outstanding timer kill it
        OnTimer(m_uTimerID);
    ASSERT(m_uTimerID == 0);

   CWnd *pWnd = AfxGetMainWnd() ;

   if (NULL == pWnd)
   {
       return ;
   }

   pWnd = pWnd->GetTopLevelParent() ;

   if (NULL == pWnd)
   {
       return ;
   }

   ::WinHelp(pWnd->m_hWnd, WORDPAD_HELP_FILE, HELP_QUIT, 0) ;
}

void CWordPadView::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
    int nOldWidth = lpClientRect->right - lpClientRect->left;
    CRichEdit2View::CalcWindowRect(lpClientRect, nAdjustType);

    if (theApp.m_bWin4 && nAdjustType != 0 && (GetStyle() & WS_VSCROLL))
        lpClientRect->right--;

    // if the ruler is visible then slide the view up under the ruler to avoid
    // showing the top border of the view
    if (GetExStyle() & WS_EX_CLIENTEDGE)
    {
        CFrameWnd* pFrame = GetParentFrame();
        if (pFrame != NULL)
        {
            CRulerBar* pBar = (CRulerBar*)pFrame->GetControlBar(ID_VIEW_RULER);
            if (pBar != NULL)
            {
                BOOL bVis = pBar->IsVisible();
                if (pBar->m_bDeferInProgress)
                    bVis = !bVis;
                if (bVis)
                    lpClientRect->top -= 2;
            }
        }
    }
}

void CWordPadView::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
    lpMIS->itemID = (UINT)(WORD)lpMIS->itemID;
    CRichEdit2View::OnMeasureItem(nIDCtl, lpMIS);
}

void CWordPadView::OnPenBackspace()
{
    SendMessage(WM_KEYDOWN, VK_BACK, 0);
    SendMessage(WM_KEYUP, VK_BACK, 0);
}

void CWordPadView::OnPenNewline()
{
    SendMessage(WM_CHAR, '\n', 0);
}

void CWordPadView::OnPenPeriod()
{
    SendMessage(WM_CHAR, '.', 0);
}

void CWordPadView::OnPenSpace()
{
    SendMessage(WM_CHAR, ' ', 0);
}

void CWordPadView::OnPenTab()
{
    SendMessage(WM_CHAR, VK_TAB, 0);
}

void CWordPadView::OnDelayedInvalidate()
{
    Invalidate() ;
}

void CWordPadView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == VK_F10 && GetKeyState(VK_SHIFT) < 0)
    {
        long nStart, nEnd;
        GetRichEditCtrl().GetSel(nStart, nEnd);
        CPoint pt = GetRichEditCtrl().GetCharPos(nEnd);
        SendMessage(WM_CONTEXTMENU, (WPARAM)m_hWnd, MAKELPARAM(pt.x, pt.y));
    }

   CRichEdit2View::OnKeyDown(nChar, nRepCnt, nFlags);
}

HRESULT CWordPadView::GetClipboardData(CHARRANGE* lpchrg, DWORD /*reco*/,
    LPDATAOBJECT lpRichDataObj,     LPDATAOBJECT* lplpdataobj)
{
    CHARRANGE& cr = *lpchrg;

    if (NULL == lpRichDataObj)
        return E_INVALIDARG;

    if ((cr.cpMax - cr.cpMin == 1) &&
        GetRichEditCtrl().GetSelectionType() == SEL_OBJECT)
    {
        return E_NOTIMPL;
    }

    BeginWaitCursor();
    //create the data source
    COleDataSource* pDataSource = new COleDataSource;

    // put the formats into the data source
    LPENUMFORMATETC lpEnumFormatEtc;
    lpRichDataObj->EnumFormatEtc(DATADIR_GET, &lpEnumFormatEtc);
    if (lpEnumFormatEtc != NULL)
    {
        FORMATETC etc;
        while (lpEnumFormatEtc->Next(1, &etc, NULL) == S_OK)
        {
            STGMEDIUM stgMedium;
            lpRichDataObj->GetData(&etc, &stgMedium);
            pDataSource->CacheData(etc.cfFormat, &stgMedium, &etc);
        }
        lpEnumFormatEtc->Release();
    }

    CEmbeddedItem item(GetDocument(), cr.cpMin, cr.cpMax);
    item.m_lpRichDataObj = lpRichDataObj;
    // get wordpad formats
    item.GetClipboardData(pDataSource);

    // get the IDataObject from the data source
    *lplpdataobj =  (LPDATAOBJECT)pDataSource->GetInterface(&IID_IDataObject);

    EndWaitCursor();
    return S_OK;
}


HRESULT CWordPadView::PasteHDROPFormat(HDROP hDrop)
{
    HRESULT hr = S_OK ;
    UINT i ;
    TCHAR szFile[MAX_PATH + 1] ;
    CHARRANGE cr ;
    LONG tmp ;
    UINT cFiles ;

    cFiles = DragQueryFile(hDrop, (UINT) -1, NULL, 0) ;

    GetRichEditCtrl().GetSel(cr);

    tmp = cr.cpMin ;

    for (i=0; i<cFiles; i++)
    {
        ::DragQueryFile(hDrop, i, szFile, MAX_PATH) ;

        if (FILE_ATTRIBUTE_DIRECTORY == GetFileAttributes(szFile))
        {
            continue ;
        }

        //
        // Fix the selection state up so that multiple objects insert
        // at the right spot
        //

        cr.cpMin = cr.cpMax ;

        GetRichEditCtrl().SetSel(cr);

        //
        // Insert from file
        //

        InsertFileAsObject(szFile) ;
    }

    GetRichEditCtrl().SetSel(cr);

    return hr ;
}

HRESULT CWordPadView::QueryAcceptData(LPDATAOBJECT lpdataobj,
    CLIPFORMAT* lpcfFormat, DWORD reco, BOOL bReally,
    HGLOBAL hMetaPict)
{
   HRESULT hr = S_OK ;

   if (!bReally)
   {
       g_fRightButtonDrag = 0x8000 & GetAsyncKeyState(
                                            GetSystemMetrics(SM_SWAPBUTTON)
                                            ? VK_LBUTTON 
                                            : VK_RBUTTON);
   }

   //
   // If we are doing an inproc drag-drop, we want our drop
   // effect to be DROPEFFECT_MOVE but if we are drag-dropping
   // from another application, we want our effect to be
   // DROPEFFECT_COPY -- in particular so that we don't delete
   // icons dragged from the explorer or text dragged from Word!
   //
   // The reason for this hack is that richedit doesn't supply
   // any mechanism for us to determine whether or not we are
   // both the drop source and the drop target.
   //

   if (!bReally)
   {
       LPUNKNOWN pUnk = NULL ;

       if (S_OK == lpdataobj->QueryInterface(
                        IID_IProxyManager,
                        (LPVOID *) &pUnk))
       {
           //
           // We got an IProxyManager pointer, so we are NOT doing an
           // inproc drag drop
           //

           pUnk->Release() ;

           g_fInternalDragDrop = FALSE ;
       }
       else
       {
           g_fInternalDragDrop = TRUE ;
       }
   }
   else
   {
       g_fInternalDragDrop = FALSE ;
   }

   //
   // Check for native data first
   //

    if (bReally && *lpcfFormat == 0 && (m_nPasteType == 0))
    {
        COleDataObject dataobj;
        dataobj.Attach(lpdataobj, FALSE);
        if (!dataobj.IsDataAvailable(cfRTO)) // native avail, let richedit do as it wants
        {
            if (dataobj.IsDataAvailable(cfEmbeddedObject))
            {
                if (PasteNative(lpdataobj))
            {
               hr = S_FALSE ;

               goto errRet ;
            }
            }
        }
    }

   //
   // We need to support HDROP format from the explorer
   // and the desktop
   //

   if (bReally)
   {
       FORMATETC fe ;

       fe.cfFormat = CF_HDROP ;
       fe.ptd = NULL ;
       fe.dwAspect = DVASPECT_CONTENT ;
       fe.lindex = -1 ;
       fe.tymed = TYMED_HGLOBAL ;

       if (S_OK == lpdataobj->QueryGetData(&fe))
       {
           STGMEDIUM sm ;

           sm.tymed = TYMED_NULL ;
           sm.hGlobal = (HGLOBAL) 0 ;
           sm.pUnkForRelease = NULL ;

           if (S_OK == lpdataobj->GetData(&fe, &sm))
           {
               //
               // If we have a single file in our HDROP data then
               // embed source might *also* be available in which case we
               // should just use the default richedit logic and
               // skip PasteHDROPFormat().  We should not ever get
               // embed source AND an HDROP data block containing
               // multiple files because OLE only supports one drop
               // source per drag-drop operation.  The default richedit
               // logic should handle all cases while dropping a single
               // file, we just have to special case things while dropping
               // multiple files.
               //

               if (DragQueryFile((HDROP) sm.hGlobal, (UINT) -1, NULL, 0) > 1)
               {
                   PasteHDROPFormat((HDROP) sm.hGlobal) ;
                   hr = S_FALSE ;
               }
               else
               {
                   hr = S_OK ;
               }

               ::ReleaseStgMedium(&sm) ;

               if (S_FALSE == hr)
               {
                   goto errRet ;
               }
           }
       }
   }

   //
   // If all else fails, let richedit give it a try
   //

   hr = CRichEdit2View::QueryAcceptData(lpdataobj, lpcfFormat, reco, bReally,
        hMetaPict);

errRet:

   if (bReally)
   {
       //
       // We post a message to ourselves here instead of just calling
       // ::Invalidate() because the richedit control doesn't always
       // repaint unless it is completely done with the data transfer operation.
       //

       PostMessage(WM_COMMAND, ID_DELAYED_INVALIDATE, 0) ;
   }

   return hr ;
}


BOOL CWordPadView::PasteNative(LPDATAOBJECT lpdataobj)
{
    // check data object for wordpad object
    // if true, suck out RTF directly

    FORMATETC etc = {NULL, NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE};
    etc.cfFormat = (CLIPFORMAT)cfEmbeddedObject;
    STGMEDIUM stgMedium = {TYMED_ISTORAGE, 0, NULL};

    // create an IStorage to transfer the data in
    LPLOCKBYTES lpLockBytes;
    if (FAILED(::CreateILockBytesOnHGlobal(NULL, TRUE, &lpLockBytes)))
        return FALSE;
    ASSERT(lpLockBytes != NULL);

    HRESULT hr = ::StgCreateDocfileOnILockBytes(lpLockBytes,
        STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_READWRITE, 0, &stgMedium.pstg);
    lpLockBytes->Release(); //storage addref'd
    if (FAILED(hr))
        return FALSE;

    ASSERT(stgMedium.pstg != NULL);

    CLSID clsid;

    BOOL bRes = FALSE; //let richedit do what it wants

    if (SUCCEEDED(lpdataobj->GetDataHere(&etc, &stgMedium)) &&
        SUCCEEDED(ReadClassStg(stgMedium.pstg, &clsid)) &&
        clsid == GetDocument()->GetClassID())
    {
        //suck out RTF now
        // open Contents stream

        COleStreamFile file;
        CFileException fe;
        if (file.OpenStream(stgMedium.pstg, szContents,
            CFile::modeReadWrite|CFile::shareExclusive, &fe))
        {
            CRichEdit2Doc  *doc = GetDocument();
            BOOL            bRTF = doc->m_bRTF;
            BOOL            bUnicode = doc->m_bUnicode;

            // Force the "current" stream type to be rtf

            doc->m_bRTF = TRUE;
            doc->m_bUnicode = FALSE;

            // load it with CArchive (loads from Contents stream)
            CArchive loadArchive(&file, CArchive::load |
                CArchive::bNoFlushOnDelete);
            Stream(loadArchive, TRUE); //stream in selection

            // Restore the "current" stream type

            doc->m_bRTF = bRTF;
            doc->m_bUnicode = bUnicode;

            bRes = TRUE; // don't let richedit do anything
        }
    }
    ::ReleaseStgMedium(&stgMedium);
    return bRes;
}

// things to fix
// if format==0 we are doing a straight EM_PASTE
//      look for native formats
//              richedit specific -- allow richedit to handle (these will be first)
//              look for RTF, CF_TEXT.  If there paste special as these
//      Do standard OLE scenario

// if pasting a particular format (format != 0)
//      if richedit specific, allow through
//      if RTF, CF_TEXT. paste special
//      if OLE format, do standard OLE scenario


void CWordPadView::OnFilePrint()
{
    theApp.EnsurePrinterIsInitialized();

    // don't allow winini changes to occur while printing
    m_bInPrint = TRUE;

    SetHelpFixHook() ;

    CRichEdit2View::OnFilePrint();

    RemoveHelpFixHook() ;
    
    // printer may have changed
    theApp.NotifyPrinterChanged(); // this will cause a GetDocument()->PrinterChanged();
    m_bInPrint = FALSE;
}

void CWordPadView::OnFilePrintPreview()
{
    theApp.EnsurePrinterIsInitialized();
    
    CRichEdit2View::OnFilePrintPreview();
}

int CWordPadView::OnMouseActivate(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (m_bOnBar)
    {
        SetFocus();
        return MA_ACTIVATEANDEAT;
    }
    else
        return CRichEdit2View::OnMouseActivate(pWnd, nHitTest, message);
}

typedef BOOL (WINAPI *PCWPROC)(HWND, LPSTR, UINT, LPVOID, DWORD, DWORD);
void CWordPadView::OnPenLens()
{
    USES_CONVERSION;
    HINSTANCE hLib = LoadLibrary(_T("PENWIN32.DLL"));
    if (hLib == NULL)
        return;
    PCWPROC pCorrectWriting = (PCWPROC)GetProcAddress(hLib, "CorrectWriting");
    ASSERT(pCorrectWriting != NULL);
    if (pCorrectWriting != NULL)
    {
        CHARRANGE cr;
        GetRichEditCtrl().GetSel(cr);
        int nCnt = 2*(cr.cpMax-cr.cpMin);
        BOOL bSel = (nCnt != 0);
        nCnt = max(1024, nCnt);
        char* pBuf = new char[nCnt];
        pBuf[0] = NULL;
        if (bSel)
            GetRichEditCtrl().GetSelText(pBuf);
        if (pCorrectWriting(m_hWnd, pBuf, nCnt, 0, bSel ? 0 : CWR_INSERT, 0))
            GetRichEditCtrl().ReplaceSel(A2T(pBuf));
        delete [] pBuf;
    }
    FreeLibrary(hLib);
}

LONG CWordPadView::OnPrinterChangedMsg(UINT, LONG)
{
    CDC dc;
    AfxGetApp()->CreatePrinterDC(dc);
    OnPrinterChanged(dc);
    return 0;
}

static void ForwardPaletteChanged(HWND hWndParent, HWND hWndFocus)
{
    // this is a quick and dirty hack to send the WM_QUERYNEWPALETTE to a window that is interested
    HWND hWnd = NULL;
    for (hWnd = ::GetWindow(hWndParent, GW_CHILD); hWnd != NULL; hWnd = ::GetWindow(hWnd, GW_HWNDNEXT))
    {
        if (hWnd != hWndFocus)
        {
            ::SendMessage(hWnd, WM_PALETTECHANGED, (WPARAM)hWndFocus, 0L);
            ForwardPaletteChanged(hWnd, hWndFocus);
        }
    }
}

void CWordPadView::OnPaletteChanged(CWnd* pFocusWnd)
{
    ForwardPaletteChanged(m_hWnd, pFocusWnd->GetSafeHwnd());
    // allow the richedit control to realize its palette
    // remove this if if richedit fixes their code so that
    // they don't realize their palette into foreground
    if (::GetWindow(m_hWnd, GW_CHILD) == NULL)
        CRichEdit2View::OnPaletteChanged(pFocusWnd);
}

static BOOL FindQueryPalette(HWND hWndParent)
{
    // this is a quick and dirty hack to send the WM_QUERYNEWPALETTE to a window that is interested
    HWND hWnd = NULL;
    for (hWnd = ::GetWindow(hWndParent, GW_CHILD); hWnd != NULL; hWnd = ::GetWindow(hWnd, GW_HWNDNEXT))
    {
        if (::SendMessage(hWnd, WM_QUERYNEWPALETTE, 0, 0L))
            return TRUE;
        else if (FindQueryPalette(hWnd))
            return TRUE;
    }
    return FALSE;
}

BOOL CWordPadView::OnQueryNewPalette()
{
    if(FindQueryPalette(m_hWnd))
        return TRUE;
    return CRichEdit2View::OnQueryNewPalette();
}

void CWordPadView::OnWinIniChange(LPCTSTR lpszSection)
{
    CRichEdit2View::OnWinIniChange(lpszSection);
    //printer might have changed
    if (!m_bInPrint)
    {
        if (lstrcmpi(lpszSection, _T("windows")) == 0)
            theApp.NotifyPrinterChanged(TRUE); // force update to defaults
    }
}

void CWordPadView::OnSize(UINT nType, int cx, int cy)
{
    CRichEdit2View::OnSize(nType, cx, cy);
    CRect rect(HORZ_TEXTOFFSET, VERT_TEXTOFFSET, cx, cy);
    GetRichEditCtrl().SetRect(rect);
}

void CWordPadView::OnGetCharFormat(NMHDR* pNMHDR, LRESULT* pRes)
{
    ASSERT(pNMHDR != NULL);
    ASSERT(pRes != NULL);

    ((CHARHDR*)pNMHDR)->cf = GetCharFormatSelection();
    *pRes = 1;
}

void CWordPadView::OnSetCharFormat(NMHDR* pNMHDR, LRESULT* pRes)
{
    ASSERT(pNMHDR != NULL);
    ASSERT(pRes != NULL);
    SetCharFormat(((CHARHDR*)pNMHDR)->cf);
    *pRes = 1;
}

void CWordPadView::OnBarSetFocus(NMHDR*, LRESULT*)
{
    m_bOnBar = TRUE;
}

void CWordPadView::OnBarKillFocus(NMHDR*, LRESULT*)
{
    m_bOnBar = FALSE;
}

void CWordPadView::OnBarReturn(NMHDR*, LRESULT* )
{
    SetFocus();
}

void CWordPadView::OnFormatFont()
{
    SetHelpFixHook() ;

    CRichEdit2View::OnFormatFont() ;

    RemoveHelpFixHook() ;
}

void CWordPadView::OnInsertObject()
{
    g_fDisableStandardHelp = TRUE ;

    SetHelpFixHook() ;

    CRichEdit2View::OnInsertObject() ;

    RemoveHelpFixHook() ;

   g_fDisableStandardHelp = FALSE ;
}

void CWordPadView::OnEditPasteSpecial()
{
    g_fDisableStandardHelp = TRUE ;

    SetHelpFixHook() ;

    CRichEdit2View::OnEditPasteSpecial() ;

    RemoveHelpFixHook() ;

    g_fDisableStandardHelp = FALSE ;
}

void CWordPadView::OnEditFind()
{
    SetHelpFixHook() ;

    CRichEdit2View::OnEditFind() ;

    RemoveHelpFixHook() ;
}

void CWordPadView::OnEditReplace()
{
    SetHelpFixHook() ;

    CRichEdit2View::OnEditReplace() ;

    RemoveHelpFixHook() ;
}

void CWordPadView::OnEditProperties()
{
    g_fDisableStandardHelp = TRUE ;

    SetHelpFixHook() ;

    CRichEdit2View::OnEditProperties() ;

    RemoveHelpFixHook() ;

   g_fDisableStandardHelp = FALSE ;
}


/////////////////////////////////////////////////////////////////////////////
// CWordPadView::XRichEditOleCallback
//
// We implement this so we can override the defaults that MFC has set up.  For
// the most part, we just delegate to MFC.
//

BEGIN_INTERFACE_MAP(CWordPadView, CCtrlView)
    // we use IID_IUnknown because richedit doesn't define an IID
     INTERFACE_PART(CWordPadView, IID_IUnknown, WordPadRichEditOleCallback)
END_INTERFACE_MAP()

STDMETHODIMP_(ULONG) CWordPadView::XWordPadRichEditOleCallback::AddRef()
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

    return pThis->m_xRichEditOleCallback.AddRef() ;
}

STDMETHODIMP_(ULONG) CWordPadView::XWordPadRichEditOleCallback::Release()
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

    return pThis->m_xRichEditOleCallback.Release() ;
}

STDMETHODIMP CWordPadView::XWordPadRichEditOleCallback::QueryInterface(
    REFIID iid, LPVOID* ppvObj)
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

    return pThis->m_xRichEditOleCallback.QueryInterface(iid, ppvObj) ;
}

STDMETHODIMP CWordPadView::XWordPadRichEditOleCallback::GetNewStorage(LPSTORAGE* ppstg)
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

    return pThis->m_xRichEditOleCallback.GetNewStorage(ppstg) ;
}

STDMETHODIMP CWordPadView::XWordPadRichEditOleCallback::GetInPlaceContext(
    LPOLEINPLACEFRAME* lplpFrame, LPOLEINPLACEUIWINDOW* lplpDoc,
    LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

    return pThis->m_xRichEditOleCallback.GetInPlaceContext(lplpFrame, lplpDoc, lpFrameInfo) ;
}

STDMETHODIMP CWordPadView::XWordPadRichEditOleCallback::ShowContainerUI(BOOL fShow)
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

    return pThis->m_xRichEditOleCallback.ShowContainerUI(fShow) ;
}

STDMETHODIMP CWordPadView::XWordPadRichEditOleCallback::QueryInsertObject(
    LPCLSID lpclsid, LPSTORAGE pstg, LONG cp)
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

    return pThis->m_xRichEditOleCallback.QueryInsertObject(lpclsid, pstg, cp) ;
}

STDMETHODIMP CWordPadView::XWordPadRichEditOleCallback::DeleteObject(LPOLEOBJECT lpoleobj)
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

    return pThis->m_xRichEditOleCallback.DeleteObject(lpoleobj) ;
}

STDMETHODIMP CWordPadView::XWordPadRichEditOleCallback::QueryAcceptData(
    LPDATAOBJECT lpdataobj, CLIPFORMAT* lpcfFormat, DWORD reco,
    BOOL fReally, HGLOBAL hMetaPict)
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

    return pThis->m_xRichEditOleCallback.QueryAcceptData(lpdataobj, lpcfFormat, reco,
                fReally, hMetaPict) ;
}

STDMETHODIMP CWordPadView::XWordPadRichEditOleCallback::ContextSensitiveHelp(BOOL fEnterMode)
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

     return pThis->m_xRichEditOleCallback.ContextSensitiveHelp(fEnterMode) ;
}

STDMETHODIMP CWordPadView::XWordPadRichEditOleCallback::GetClipboardData(
    CHARRANGE* lpchrg, DWORD reco, LPDATAOBJECT* lplpdataobj)
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

    return pThis->m_xRichEditOleCallback.GetClipboardData(lpchrg, reco, lplpdataobj) ;
}

STDMETHODIMP CWordPadView::XWordPadRichEditOleCallback::GetDragDropEffect(
    BOOL fDrag, DWORD grfKeyState, LPDWORD pdwEffect)
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

    if (!fDrag) // allowable dest effects
    {
        DWORD   dwEffect;

        // check for force link
#ifndef _MAC
        if ((grfKeyState & (MK_CONTROL|MK_SHIFT)) == (MK_CONTROL|MK_SHIFT))
#else
        if ((grfKeyState & (MK_OPTION|MK_SHIFT)) == (MK_OPTION|MK_SHIFT))
#endif
            dwEffect = DROPEFFECT_LINK;
        // check for force copy
#ifndef _MAC
        else if ((grfKeyState & MK_CONTROL) == MK_CONTROL)
#else
        else if ((grfKeyState & MK_OPTION) == MK_OPTION)
#endif
            dwEffect = DROPEFFECT_COPY;
        // check for force move
        else if ((grfKeyState & MK_ALT) == MK_ALT)
            dwEffect = DROPEFFECT_MOVE;
        // default -- recommended action is 'copy' (overridden from MFC default)
        else
        {
            if (g_fInternalDragDrop)
            {
                dwEffect = DROPEFFECT_MOVE ;
            }
            else
            {
                dwEffect = DROPEFFECT_COPY;
            }
        }

        pThis->m_nPasteType = 0;

        if (dwEffect & *pdwEffect) // make sure allowed type
        {
            *pdwEffect = dwEffect;

            if (DROPEFFECT_LINK == dwEffect)
                pThis->m_nPasteType = COlePasteSpecialDialog::pasteLink;
        }
    }
    return S_OK;
}

STDMETHODIMP CWordPadView::XWordPadRichEditOleCallback::GetContextMenu(
    WORD seltype, LPOLEOBJECT lpoleobj, CHARRANGE* lpchrg,
    HMENU* lphmenu)
{
    METHOD_PROLOGUE_EX_(CWordPadView, WordPadRichEditOleCallback)

    HRESULT hr;

    if (g_fRightButtonDrag)
        hr = E_FAIL;
    else 
        hr = pThis->m_xRichEditOleCallback.GetContextMenu(
                                                    seltype, 
                                                    lpoleobj, 
                                                    lpchrg, 
                                                    lphmenu);

    g_fRightButtonDrag = FALSE;

    return hr;
}
