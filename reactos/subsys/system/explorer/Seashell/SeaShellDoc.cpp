// SeaShellDoc.cpp : implementation of the CSeaShellDoc class
//

#include "stdafx.h"
#include "SeaShell.h"

#include "SeaShellDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSeaShellDoc

IMPLEMENT_DYNCREATE(CSeaShellDoc, CDocument)

BEGIN_MESSAGE_MAP(CSeaShellDoc, CDocument)
	//{{AFX_MSG_MAP(CSeaShellDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSeaShellDoc construction/destruction

CSeaShellDoc::CSeaShellDoc()
{
	// TODO: add one-time construction code here

}

CSeaShellDoc::~CSeaShellDoc()
{
}

BOOL CSeaShellDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CSeaShellDoc serialization

void CSeaShellDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSeaShellDoc diagnostics

#ifdef _DEBUG
void CSeaShellDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSeaShellDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSeaShellDoc commands
