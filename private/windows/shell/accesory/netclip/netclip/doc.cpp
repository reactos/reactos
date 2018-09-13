// NetClipDoc.cpp : implementation of the CNetClipDoc class
//

#include "stdafx.h"
#include "NetClip.h"
#include "Doc.h"
#include "View.h"

#include "MainFrm.h"

#include "guids.h"

#include "cntritem.h"
#include "DataObj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetClipDoc

IMPLEMENT_DYNCREATE(CNetClipDoc, CRichEditDoc)

BEGIN_MESSAGE_MAP(CNetClipDoc, CRichEditDoc)
	//{{AFX_MSG_MAP(CNetClipDoc)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetClipDoc construction/destruction

CNetClipDoc::CNetClipDoc()
{
    EnableCompoundFile();
}

CNetClipDoc::~CNetClipDoc()
{
}

// We never prompt if the user closes/opens etc..
//
BOOL CNetClipDoc::SaveModified()
{
    return 1;
}

void CNetClipDoc::DeleteContents()
{
	CRichEditDoc::DeleteContents();
}

BOOL CNetClipDoc::OnNewDocument()
{
    SetModifiedFlag(TRUE);
    // Default implementation calls DeleteContents
	if (!CRichEditDoc::OnNewDocument())
		return FALSE;

    return TRUE;
}

// OnOpenDocument attempts to open the specified docfile and
// uses the IPersistStorage::Load method of the generic data object
// to read the data.
//
// We do not keep the file open.
//
BOOL CNetClipDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	USES_CONVERSION;

	ASSERT(lpszPathName);

	// abort changes to current docfile
	DeleteContents();
    if(m_lpRootStg)
    {
    	m_lpRootStg->Release();
        m_lpRootStg = NULL;
    }

	BOOL bResult = FALSE;
	try
	{
		LPCOLESTR lpsz = T2COLE(lpszPathName);

		// use STGM_CONVERT if necessary
		SCODE sc;
		LPSTORAGE lpStorage = NULL;
		// open new storage file
		sc = StgOpenStorage(lpsz, NULL,
			STGM_READWRITE|STGM_TRANSACTED|STGM_SHARE_EXCLUSIVE,
			0, 0, &lpStorage);
		if (FAILED(sc) || lpStorage == NULL)
			sc = StgOpenStorage(lpsz, NULL,
				STGM_READ|STGM_TRANSACTED|STGM_SHARE_EXCLUSIVE,
				0, 0, &lpStorage);
		if (FAILED(sc))
			AfxThrowOleException(sc);

		ASSERT(lpStorage != NULL);
		m_lpRootStg = lpStorage;

		// Create a generic IDataObject, and initialize it
        // with the storage.  Then set the clipboard.
        //
        HRESULT hr;
        CGenericDataObject* pGeneric = new CGenericDataObject();
        IPersistStorage* pPersist = (IPersistStorage*)pGeneric->GetInterface(&IID_IPersistStorage);
        ASSERT(pPersist);

        hr = pPersist->Load(m_lpRootStg);
        if (FAILED(hr))
        {
            pPersist->Release();
            AfxThrowOleException(hr);
        }

        CMainFrame* pfrm = (CMainFrame*)AfxGetMainWnd();
        ASSERT(pfrm);
        IDataObject* pdo = (IDataObject*)pGeneric->GetInterface(&IID_IDataObject);
        ASSERT(pdo);
        if (pfrm->m_pClipboard == NULL)
            hr = OleSetClipboard(pdo);
        else
            hr = pfrm->m_pClipboard->SetClipboard(pdo);

        // We could use either pdo or pPersist here because
        // GetInterface does not AddRef
        pdo->Release();

        if (FAILED(hr))
            AfxThrowOleException(hr);

        if (m_lpRootStg)
        {
    		m_lpRootStg->Release();
            m_lpRootStg = NULL;
        }
		SetModifiedFlag(TRUE);
		bResult = TRUE;
	}
	catch(CException* e)
	{
		DeleteContents();   // removed failed contents
        if (m_lpRootStg)
        {
    		m_lpRootStg->Release();
            m_lpRootStg = NULL;
        }

		// if not file-based load, return exceptions to the caller
		if (lpszPathName == NULL)
		{
			throw;
			ASSERT(FALSE);  // not reached
		}

		try
		{
			ReportSaveLoadException(lpszPathName, e,
				FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
		}
		catch(...)
        {}

		e->Delete();
	}
	
	return bResult;
}

void CNetClipDoc::OnFileSaveAs()
{
	if (!DoSave(NULL, FALSE)) { // bReplace == FALSE means don't change m_strPathName
		TRACE0("Warning: File save-as failed.\n");
    }
}

// OnSaveDocument creates the specified docfile and uses
// the IPeristStorage::Save method of the generic data object
// to save the clipboard.
//
BOOL CNetClipDoc::OnSaveDocument(LPCTSTR lpszPathName)
	// lpszPathName must be fully qualified
{
    USES_CONVERSION;
	ASSERT(lpszPathName);

	BOOL bResult = FALSE;

    if (m_lpRootStg)
    {
    	m_lpRootStg->Release();
        m_lpRootStg = NULL;
    }

	try
	{
		LPSTORAGE lpStorage;
		SCODE sc = ::StgCreateDocfile(T2COLE(lpszPathName),
			STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_CREATE,
			0, &lpStorage);
		if (sc != S_OK)
			AfxThrowOleException(sc);

		ASSERT(lpStorage != NULL);
		m_lpRootStg = lpStorage;

        // Copy clipboard dataobject.  CGenericDataObject implements
        // IPersistStorage, we just pass him our IStorage and he does
        // all the work.
        CMainFrame* pfrm = (CMainFrame*)AfxGetMainWnd();
        ASSERT(pfrm);
        IDataObject* pdo ;
        HRESULT hr;
        if (pfrm->m_pClipboard == NULL)
            hr = OleGetClipboard(&pdo);
        else
            hr = pfrm->m_pClipboard->GetClipboard(&pdo);
        if (FAILED(hr))
            AfxThrowOleException(hr);

        CGenericDataObject* pGeneric = new CGenericDataObject(pdo);
        pdo->Release();

        COleMessageFilter* pFilter = AfxOleGetMessageFilter();
	    ASSERT(pFilter != NULL);
	    pFilter->EnableBusyDialog(FALSE);

        IPersistStorage* pPersist = (IPersistStorage*)pGeneric->GetInterface(&IID_IPersistStorage);
        ASSERT(pPersist);
        hr = pPersist->Save(m_lpRootStg, FALSE);
        pPersist->Release();
	    pFilter->EnableBusyDialog(TRUE);

        if (FAILED(hr))
            AfxThrowOleException(hr);

		m_lpRootStg->Release();
		m_lpRootStg = NULL;

		SetModifiedFlag(TRUE);
		bResult = TRUE;
	}
	catch(CException* e)
	{

        if (m_lpRootStg)
        {
    		m_lpRootStg->Release();
	    	m_lpRootStg = NULL;
        }

		try
		{
			ReportSaveLoadException(lpszPathName, e,
				TRUE, AFX_IDP_FAILED_TO_SAVE_DOC);
		}
		catch(...)
        {
        }
		e->Delete();
	}
	
	// cleanup
	m_bSameAsLoad = FALSE;
	//m_bRemember = TRUE;

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CNetClipDoc diagnostics

#ifdef _DEBUG
void CNetClipDoc::AssertValid() const
{
	CRichEditDoc::AssertValid();
}

void CNetClipDoc::Dump(CDumpContext& dc) const
{
	CRichEditDoc::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNetClipDoc commands

CRichEditCntrItem* CNetClipDoc::CreateClientItem(REOBJECT* preo) const
{
	// cast away constness of this
	return new CNetClipCntrItem(preo, (CNetClipDoc*)this);
}

#if 0
void CWordPadDoc::OnFileSendMail()
{
	if (m_strTitle.Find('.') == -1)
	{
		// add the extension because the default extension will be wrong
		CString strOldTitle = m_strTitle;
		m_strTitle += GetExtFromType(m_nDocType);
		CRichEditDoc::OnFileSendMail();
		m_strTitle = strOldTitle;
	}
	else
		CRichEditDoc::OnFileSendMail();
}
#endif

