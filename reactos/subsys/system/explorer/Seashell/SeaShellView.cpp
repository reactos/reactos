// SeaShellView.cpp : implementation of the CSeaShellView class
//

#include "stdafx.h"
#include "SeaShell.h"

#include "SeaShellDoc.h"
#include "SeaShellView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSeaShellView

IMPLEMENT_DYNCREATE(CSeaShellView, CIEShellListView)

BEGIN_MESSAGE_MAP(CSeaShellView, CIEShellListView)
	//{{AFX_MSG_MAP(CSeaShellView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CIEShellListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CIEShellListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CIEShellListView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSeaShellView construction/destruction

CSeaShellView::CSeaShellView()
{
	// TODO: add construction code here

}

CSeaShellView::~CSeaShellView()
{
}

BOOL CSeaShellView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CIEShellListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CSeaShellView drawing

void CSeaShellView::OnInitialUpdate()
{
	CIEShellListView::OnInitialUpdate();


	// TODO: You may populate your ListView with items by directly accessing
	//  its list control through a call to GetListCtrl().
}

/////////////////////////////////////////////////////////////////////////////
// CSeaShellView printing

BOOL CSeaShellView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CSeaShellView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CSeaShellView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CSeaShellView diagnostics

#ifdef _DEBUG
void CSeaShellView::AssertValid() const
{
	CIEShellListView::AssertValid();
}

void CSeaShellView::Dump(CDumpContext& dc) const
{
	CIEShellListView::Dump(dc);
}

CSeaShellDoc* CSeaShellView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSeaShellDoc)));
	return (CSeaShellDoc*)m_pDocument;
}
#endif //_DEBUG

