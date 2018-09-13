// wordpdoc.cpp : implementation of the CWordPadDoc class
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
#include "wordpdoc.h"
#include "wordpvw.h"
#include "cntritem.h"
#include "srvritem.h"
#include "formatba.h"

#include "mainfrm.h"
#include "ipframe.h"
#include "buttondi.h"
#include "helpids.h"
#include "strings.h"
#include "unitspag.h"
#include "docopt.h"
#include "optionsh.h"

#include "multconv.h"

#include "fixhelp.h"

BOOL AskAboutFormatLoss(CWordPadDoc *pDoc) ;

//
// These defines are from ..\shell\userpri\uconvert.h
//

#define REVERSE_BYTE_ORDER_MARK   0xFFFE
#define BYTE_ORDER_MARK           0xFEFF

BOOL CheckForUnicodeTextFile(LPCTSTR lpszPathName) ;


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
extern UINT AFXAPI AfxGetFileTitle(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);

#ifndef OFN_EXPLORER
#define OFN_EXPLORER 0x00080000L
#endif
/////////////////////////////////////////////////////////////////////////////
// CWordPadDoc
IMPLEMENT_DYNCREATE(CWordPadDoc, CRichEdit2Doc)

BEGIN_MESSAGE_MAP(CWordPadDoc, CRichEdit2Doc)
	//{{AFX_MSG_MAP(CWordPadDoc)
	ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)
	ON_UPDATE_COMMAND_UI(ID_OLE_VERB_POPUP, OnUpdateOleVerbPopup)
	ON_COMMAND(ID_FILE_SEND_MAIL, OnFileSendMail)
	ON_UPDATE_COMMAND_UI(ID_FILE_NEW, OnUpdateIfEmbedded)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPEN, OnUpdateIfEmbedded)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateIfEmbedded)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateIfEmbedded)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_DIRECT, OnUpdateIfEmbedded)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_PREVIEW, OnUpdateIfEmbedded)
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_FILE_SEND_MAIL, OnUpdateFileSendMail)
	ON_COMMAND(ID_OLE_EDIT_LINKS, OnEditLinks)
	ON_UPDATE_COMMAND_UI(ID_OLE_VERB_FIRST, CRichEdit2Doc::OnUpdateObjectVerbMenu)
	ON_UPDATE_COMMAND_UI(ID_OLE_EDIT_CONVERT, CRichEdit2Doc::OnUpdateObjectVerbMenu)
	ON_UPDATE_COMMAND_UI(ID_OLE_EDIT_LINKS, CRichEdit2Doc::OnUpdateEditLinksMenu)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWordPadDoc construction/destruction

CWordPadDoc::CWordPadDoc()
{
	m_nDocType = -1;
	m_nNewDocType = -1;
}

BOOL CWordPadDoc::OnNewDocument()
{
	if (!CRichEdit2Doc::OnNewDocument())
		return FALSE;

 	//correct type already set in theApp.m_nNewDocType;
 	int nDocType = (IsEmbedded()) ? RD_EMBEDDED : theApp.m_nNewDocType;

	GetView()->SetDefaultFont(IsTextType(nDocType));
	SetDocType(nDocType);

	return TRUE;
}

void CWordPadDoc::ReportSaveLoadException(LPCTSTR lpszPathName,
	CException* e, BOOL bSaving, UINT nIDP)
{
	if (!m_bDeferErrors && e != NULL)
	{
		ASSERT_VALID(e);
		if (e->IsKindOf(RUNTIME_CLASS(CFileException)))
		{
			switch (((CFileException*)e)->m_cause)
			{
			case CFileException::fileNotFound:
			case CFileException::badPath:
				nIDP = AFX_IDP_FAILED_INVALID_PATH;
				break;
			case CFileException::diskFull:
				nIDP = AFX_IDP_FAILED_DISK_FULL;
				break;
			case CFileException::accessDenied:
                nIDP = AFX_IDP_FILE_ACCESS_DENIED;

				if (((CFileException*)e)->m_lOsError == ERROR_WRITE_PROTECT)
					nIDP = IDS_WRITEPROTECT;
				break;
			case CFileException::tooManyOpenFiles:
				nIDP = IDS_TOOMANYFILES;
				break;
			case CFileException::directoryFull:
				nIDP = IDS_DIRFULL;
				break;
			case CFileException::sharingViolation:
				nIDP = IDS_SHAREVIOLATION;
				break;
			case CFileException::lockViolation:
			case CFileException::badSeek:
			case CFileException::generic:
			case CFileException::invalidFile:
			case CFileException::hardIO:
				nIDP = bSaving ? AFX_IDP_FAILED_IO_ERROR_WRITE :
						AFX_IDP_FAILED_IO_ERROR_READ;
				break;
			default:
				break;
			}
			CString prompt;
			AfxFormatString1(prompt, nIDP, lpszPathName);
			AfxMessageBox(prompt, MB_ICONEXCLAMATION, nIDP);
			return;
		}
	}
	CRichEdit2Doc::ReportSaveLoadException(lpszPathName, e, bSaving, nIDP);
	return;
}


BOOL CheckForUnicodeTextFile(LPCTSTR lpszPathName)
{
    BOOL fRet = FALSE ;
    HANDLE hFile = (HANDLE) 0 ;
    WORD wBOM ;
    DWORD dwBytesRead = 0 ;
    BOOL bTmp ;

    if (lpszPathName == NULL)
    {
       return FALSE ;
    }

    hFile = CreateFile(
                lpszPathName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL) ;

    if (hFile == INVALID_HANDLE_VALUE)
    {
       return FALSE ;
    }

    bTmp = ReadFile(
               hFile,
               &wBOM,
               sizeof(WORD),
               &dwBytesRead,
               NULL) ;

    if (bTmp)
    {
        if (dwBytesRead == sizeof(WORD))
        {
            if ( (wBOM == BYTE_ORDER_MARK) ||
                 (wBOM == REVERSE_BYTE_ORDER_MARK) )
            {
                fRet = TRUE ;

            }
        }
    }

    CloseHandle(hFile) ;

    return fRet ;
}

BOOL CWordPadDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if (m_lpRootStg != NULL) // we are embedded
	{
		// we really want to use the converter on this storage
		m_nNewDocType = RD_EMBEDDED;
	}
	else
	{            
		if (theApp.cmdInfo.m_bForceTextMode)
			m_nNewDocType = RD_TEXT;
		else
		{
			CFileException fe;
			m_nNewDocType = GetDocTypeFromName(lpszPathName, fe);
			if (m_nNewDocType == -1)
			{
				ReportSaveLoadException(lpszPathName, &fe, FALSE,
					AFX_IDP_FAILED_TO_OPEN_DOC);
				return FALSE;
			}

            if (RD_FEWINWORD5 == m_nNewDocType)
            {
                AfxMessageBox(IDS_FEWINWORD5_DOC, MB_OK, MB_ICONINFORMATION);
                return FALSE;
            }

			if (m_nNewDocType == RD_TEXT && theApp.m_bForceOEM)
				m_nNewDocType = RD_OEMTEXT;
		}
		ScanForConverters();
		if (!doctypes[m_nNewDocType].bRead)
		{
			CString str;
			CString strName = doctypes[m_nNewDocType].GetString(DOCTYPE_DOCTYPE);
			AfxFormatString1(str, IDS_CANT_LOAD, strName);
			AfxMessageBox(str, MB_OK|MB_ICONINFORMATION);
			return FALSE;
		}
	}

    if (RD_TEXT == m_nNewDocType)
    {
        if (CheckForUnicodeTextFile(lpszPathName))
            m_nNewDocType = RD_UNICODETEXT;
    }

	if (!CRichEdit2Doc::OnOpenDocument(lpszPathName))
		return FALSE;

    // Update any Ole links

    COleUpdateDialog(this).DoModal();

	return TRUE;
}

void CWordPadDoc::Serialize(CArchive& ar)
{
	COleMessageFilter* pFilter = AfxOleGetMessageFilter();
	ASSERT(pFilter != NULL);
	pFilter->EnableBusyDialog(FALSE);

	if (ar.IsLoading())
		SetDocType(m_nNewDocType);

    //
    // Strip (or output) the byte order mark if this is a Unicode file
    //

    if (m_bUnicode)
    {
        if (ar.IsLoading())
        {
            WORD byte_order_mark;

            ar >> byte_order_mark;

            // No support for byte-reversed files

            ASSERT(BYTE_ORDER_MARK == byte_order_mark);
        }
        else
        {
            ar << (WORD) BYTE_ORDER_MARK;
        }
    }   

	CRichEdit2Doc::Serialize(ar);
	pFilter->EnableBusyDialog(TRUE);
}



BOOL AskAboutFormatLoss(CWordPadDoc *pDoc)
{
    return (IDYES == AfxMessageBox(IDS_SAVE_FORMAT_TEXT, MB_YESNO));
}

BOOL CWordPadDoc::DoSave(LPCTSTR pszPathName, BOOL bReplace /*=TRUE*/)
	// Save the document data to a file
	// pszPathName = path name where to save document file
	// if pszPathName is NULL then the user will be prompted (SaveAs)
	// note: pszPathName can be different than 'm_strPathName'
	// if 'bReplace' is TRUE will change file name if successful (SaveAs)
	// if 'bReplace' is FALSE will not change path name (SaveCopyAs)
{
	CString newName = pszPathName;
	int nOrigDocType = m_nDocType;  //saved in case of SaveCopyAs or failure
   int nDocType ;
   int nRes ;

	//	newName		bWrite	type	result
	//	empty		TRUE	-		SaveAs dialog
	//	empty		FALSE	-		SaveAs dialog
	//	notempty	TRUE	-		nothing
	//	notempty	FALSE	W6		warn (change to wordpad, save as, cancel)
	//	notempty	FALSE	other	warn (save as, cancel)

	BOOL bModified = IsModified();

	ScanForConverters();

	BOOL bSaveAs = FALSE;

	if (newName.IsEmpty())
		bSaveAs = TRUE;

	else if (!doctypes[m_nDocType].bWrite)
	{
		if (m_nDocType == RD_WINWORD6)
		{
			nRes = CButtonDialog::DisplayMessageBox(
				MAKEINTRESOURCE(IDS_WORD_WARNING), AfxGetAppName(),
				MAKEINTRESOURCE(IDS_WORD_WARNING_BUTTONS),
				MB_ICONQUESTION, 1, 2);

			if (nRes == 0) // Save
				SetDocType(RD_WORDPAD, TRUE);
			else if (nRes == 2) // Cancel
				return FALSE;
			else
				bSaveAs = TRUE;
			// else save as
		}
        else if (m_nDocType == RD_WORD97)
        {
            //
            // We can't save Word97 files.  Ask the user if they want to
            // save in a different format
            //

            nRes = AfxMessageBox(IDS_WORD97_WARNING, MB_OKCANCEL);

            if (IDOK == nRes)
                bSaveAs = TRUE;
            else
                return FALSE;
        }
		else //
		{
			if (AfxMessageBox(IDS_SAVE_UNSUPPORTED,
				MB_YESNO | MB_ICONQUESTION) != IDYES)
			{
				return FALSE;
			}
			else
				bSaveAs = TRUE;
		}
	}

	if (m_lpRootStg == NULL && IsTextType(m_nDocType) &&
		!bSaveAs && !GetView()->IsFormatText())
	{
        if (!AskAboutFormatLoss(this))
            bSaveAs = TRUE;
	}

	GetView()->GetParentFrame()->RecalcLayout();

	if (bSaveAs)
	{
      newName = m_strPathName;

		if (bReplace && newName.IsEmpty())
		{
			newName = m_strTitle;
			int iBad = newName.FindOneOf(_T(" #%;/\\"));    // dubious filename
			if (iBad != -1)
				newName.ReleaseBuffer(iBad);

			// append the default suffix if there is one
			newName += GetExtFromType(m_nDocType);
		}

		nDocType = m_nDocType;

promptloop:

		if (!theApp.PromptForFileName(newName,
			bReplace ? AFX_IDS_SAVEFILE : AFX_IDS_SAVEFILECOPY,
			OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, &nDocType))
		{
			SetDocType(nOrigDocType, TRUE);
			return FALSE;       // don't even try to save
		}
      else
      {
          //
          // If we are transitioning from non-text to text, we need
          // to warn the user if there is any formatting / graphics
          // that will be lost
          //

          if (IsTextType(nDocType))
          {
              if (m_lpRootStg == NULL && !GetView()->IsFormatText())
              {
                if (!AskAboutFormatLoss(this))
                    goto promptloop;
              }
          }
      }

		SetDocType(nDocType, TRUE);
	}

	BeginWaitCursor();

	if (!OnSaveDocument(newName))
	{
		if (pszPathName == NULL)
		{
			// be sure to delete the file
			TRY
			{
				CFile::Remove(newName);
			}
			CATCH_ALL(e)
			{
				TRACE0("Warning: failed to delete file after failed SaveAs\n");
			}
			END_CATCH_ALL
		}
		// restore orginal document type
		SetDocType(nOrigDocType, TRUE);
		EndWaitCursor();
		return FALSE;
	}

	EndWaitCursor();
	if (bReplace)
	{
		int nType = m_nDocType;
		SetDocType(nOrigDocType, TRUE);
		SetDocType(nType);
		// Reset the title and change the document name
		SetPathName(newName, TRUE);
		ASSERT(m_strPathName == newName);       // must be set
	}
	else // SaveCopyAs
	{
		SetDocType(nOrigDocType, TRUE);
		SetModifiedFlag(bModified);
	}
	return TRUE;        // success
}


class COIPF : public COleIPFrameWnd
{
public:
	CFrameWnd* GetMainFrame() { return m_pMainFrame;}
	CFrameWnd* GetDocFrame() { return m_pDocFrame;}
};

void CWordPadDoc::OnDeactivateUI(BOOL bUndoable)
{
	if (GetView()->m_bDelayUpdateItems)
		UpdateAllItems(NULL);
	SaveState(m_nDocType);
	CRichEdit2Doc::OnDeactivateUI(bUndoable);
	COIPF* pFrame = (COIPF*)m_pInPlaceFrame;
	if (pFrame != NULL)
	{
		if (pFrame->GetMainFrame() != NULL)
			ForceDelayed(pFrame->GetMainFrame());
		if (pFrame->GetDocFrame() != NULL)
			ForceDelayed(pFrame->GetDocFrame());
	}
}

void CWordPadDoc::ForceDelayed(CFrameWnd* pFrameWnd)
{
	ASSERT_VALID(this);
	ASSERT_VALID(pFrameWnd);

	POSITION pos = pFrameWnd->m_listControlBars.GetHeadPosition();
	while (pos != NULL)
	{
		// show/hide the next control bar
		CControlBar* pBar =
			(CControlBar*)pFrameWnd->m_listControlBars.GetNext(pos);

		BOOL bVis = pBar->GetStyle() & WS_VISIBLE;
		UINT swpFlags = 0;
		if ((pBar->m_nStateFlags & CControlBar::delayHide) && bVis)
			swpFlags = SWP_HIDEWINDOW;
		else if ((pBar->m_nStateFlags & CControlBar::delayShow) && !bVis)
			swpFlags = SWP_SHOWWINDOW;
		pBar->m_nStateFlags &= ~(CControlBar::delayShow|CControlBar::delayHide);
		if (swpFlags != 0)
		{
			pBar->SetWindowPos(NULL, 0, 0, 0, 0, swpFlags|
				SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CWordPadDoc Attributes
CLSID CWordPadDoc::GetClassID()
{
	return (m_pFactory == NULL) ? CLSID_NULL : m_pFactory->GetClassID();
}

void CWordPadDoc::SetDocType(int nNewDocType, BOOL bNoOptionChange)
{
	ASSERT(nNewDocType != -1);
	if (nNewDocType == m_nDocType)
		return;

	m_bRTF = !IsTextType(nNewDocType);
    m_bUnicode = (nNewDocType == RD_UNICODETEXT);

	if (bNoOptionChange)
		m_nDocType = nNewDocType;
	else
	{
		SaveState(m_nDocType);
		m_nDocType = nNewDocType;
		RestoreState(m_nDocType);
	}
}

CWordPadView* CWordPadDoc::GetView()
{
	POSITION pos = GetFirstViewPosition();
	return (CWordPadView* )GetNextView( pos );
}

/////////////////////////////////////////////////////////////////////////////
// CWordPadDoc Operations

CFile* CWordPadDoc::GetFile(LPCTSTR pszPathName, UINT nOpenFlags, CFileException* pException)
{
	CTrackFile* pFile = NULL;
	CFrameWnd* pWnd = GetView()->GetParentFrame();
#ifdef CONVERTERS
	ScanForConverters();

	// if writing use current doc type otherwise use new doc type
	int nType = (nOpenFlags & CFile::modeReadWrite) ? m_nDocType : m_nNewDocType;
	// m_nNewDocType will be same as m_nDocType except when opening a new file
	if (doctypes[nType].pszConverterName != NULL)
		pFile = new CConverter(doctypes[nType].pszConverterName, pWnd);
	else
#endif
	if (nType == RD_OEMTEXT)
		pFile = new COEMFile(pWnd);
	else
		pFile = new CTrackFile(pWnd);
 	if (!pFile->Open(pszPathName, nOpenFlags, pException))
	{
 		delete pFile;
		return NULL;
	}
	if (nOpenFlags & (CFile::modeWrite | CFile::modeReadWrite))
		pFile->m_dwLength = 0; // can't estimate this
	else
		pFile->m_dwLength = pFile->GetLength();
	return pFile;
}

CRichEdit2CntrItem* CWordPadDoc::CreateClientItem(REOBJECT* preo) const
{
	// cast away constness of this
	return new CWordPadCntrItem(preo, (CWordPadDoc*)this);
}

/////////////////////////////////////////////////////////////////////////////
// CWordPadDoc server implementation

COleServerItem* CWordPadDoc::OnGetEmbeddedItem()
{
	// OnGetEmbeddedItem is called by the framework to get the COleServerItem
	//  that is associated with the document.  It is only called when necessary.

	CEmbeddedItem* pItem = new CEmbeddedItem(this);
	ASSERT_VALID(pItem);
	return pItem;
}

/////////////////////////////////////////////////////////////////////////////
// CWordPadDoc serialization

/////////////////////////////////////////////////////////////////////////////
// CWordPadDoc diagnostics

#ifdef _DEBUG
void CWordPadDoc::AssertValid() const
{
	CRichEdit2Doc::AssertValid();
}

void CWordPadDoc::Dump(CDumpContext& dc) const
{
	CRichEdit2Doc::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWordPadDoc commands

int CWordPadDoc::MapType(int nType)
{
	if (nType == RD_OEMTEXT || nType == RD_UNICODETEXT)
		nType = RD_TEXT;
	else if (!IsInPlaceActive() && nType == RD_EMBEDDED)
		nType = RD_RICHTEXT;
	return nType;
}

void CWordPadDoc::OnViewOptions()
{
	int nType = MapType(m_nDocType);
	int nFirstPage = 3;
	if (nType == RD_TEXT)
		nFirstPage = 1;
	else if (nType == RD_RICHTEXT)
		nFirstPage = 2;
	else if (nType == RD_WRITE)
		nFirstPage = 4;
	else if (nType == RD_EMBEDDED)
		nFirstPage = 5;

	SaveState(nType);

	COptionSheet sheet(IDS_OPTIONS, NULL, nFirstPage);

	if (sheet.DoModal() == IDOK)
	{
		CWordPadView* pView = GetView();
		if (theApp.m_bWordSel)
			pView->GetRichEditCtrl().SetOptions(ECOOP_OR, ECO_AUTOWORDSELECTION);
		else
		{
			pView->GetRichEditCtrl().SetOptions(ECOOP_AND,
				~(DWORD)ECO_AUTOWORDSELECTION);
		}
		RestoreState(nType);
	}
}

void CWordPadDoc::OnUpdateOleVerbPopup(CCmdUI* pCmdUI)
{
	pCmdUI->m_pParentMenu = pCmdUI->m_pMenu;
	CRichEdit2Doc::OnUpdateObjectVerbMenu(pCmdUI);
}

BOOL CWordPadDoc::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if (nCode == CN_COMMAND && nID == ID_OLE_VERB_POPUP)
		nID = ID_OLE_VERB_FIRST;	
	return CRichEdit2Doc::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CWordPadDoc::SaveState(int nType)
{
	if (nType == -1)
		return;
	nType = MapType(nType);
	CWordPadView* pView = GetView();
	if (pView != NULL)
	{
		CFrameWnd* pFrame = pView->GetParentFrame();
		ASSERT(pFrame != NULL);
		// save current state
		pFrame->SendMessage(WPM_BARSTATE, 0, nType);
		theApp.GetDocOptions(nType).m_nWordWrap = pView->m_nWordWrap;
	}
}

void CWordPadDoc::RestoreState(int nType)
{
	if (nType == -1)
		return;
	nType = MapType(nType);
	CWordPadView* pView = GetView();
	if (pView != NULL)
	{
		CFrameWnd* pFrame = pView->GetParentFrame();
		ASSERT(pFrame != NULL);
		// set new state
		pFrame->SendMessage(WPM_BARSTATE, 1, nType);
		int nWrapNew = theApp.GetDocOptions(nType).m_nWordWrap;
		if (pView->m_nWordWrap != nWrapNew)
		{
			pView->m_nWordWrap = nWrapNew;
			pView->WrapChanged();
		}
	}
}

void CWordPadDoc::OnCloseDocument()
{
	SaveState(m_nDocType);
	CRichEdit2Doc::OnCloseDocument();
}

void CWordPadDoc::PreCloseFrame(CFrameWnd* pFrameArg)
{
	CRichEdit2Doc::PreCloseFrame(pFrameArg);
	SaveState(m_nDocType);
}

void CWordPadDoc::OnFileSendMail()
{
	if (m_strTitle.Find('.') == -1)
	{
		// add the extension because the default extension will be wrong
		CString strOldTitle = m_strTitle;
		m_strTitle += GetExtFromType(m_nDocType);
		CRichEdit2Doc::OnFileSendMail();
		m_strTitle = strOldTitle;
	}
	else
		CRichEdit2Doc::OnFileSendMail();
}

void CWordPadDoc::OnUpdateIfEmbedded(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!IsEmbedded());
}


void CWordPadDoc::OnEditLinks()
{
    g_fDisableStandardHelp = TRUE ;

    SetHelpFixHook() ;

    CRichEdit2Doc::OnEditLinks() ;

    RemoveHelpFixHook() ;

    g_fDisableStandardHelp = FALSE ;
}
