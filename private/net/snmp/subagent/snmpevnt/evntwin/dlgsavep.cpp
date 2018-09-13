// dlgsavep.cpp : implementation file
//

#include "stdafx.h"
#include "eventrap.h"
#include "dlgsavep.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgSaveProgress dialog


CDlgSaveProgress::CDlgSaveProgress(BOOL bIsSaving)
	: CDialog(CDlgSaveProgress::IDD, NULL)
{
	//{{AFX_DATA_INIT(CDlgSaveProgress)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_bWasCanceled = FALSE;
    m_bIsSaving = bIsSaving;    // May indicate loading or saving.
}


void CDlgSaveProgress::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgSaveProgress)
	DDX_Control(pDX, IDC_PROGRESS, m_progress);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgSaveProgress, CDialog)
	//{{AFX_MSG_MAP(CDlgSaveProgress)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDlgSaveProgress message handlers

void CDlgSaveProgress::OnCancel()
{
	// TODO: Add extra cleanup here
    if (!m_bIsSaving) {
        // Cancel is currently enabled only for a load
        m_bWasCanceled = TRUE;	
    	CDialog::OnCancel();
    }
}

void CDlgSaveProgress::ProgressYield()
{
    MSG msg;

    // Remove all available messages for any window that belongs to
    // the current application.
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        // Translate and siapatch the given message if the window handle is
        // null or the given message is not for the modeless dialog box hwnd.
        if (!m_hWnd || !IsDialogMessage(&msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}


BOOL CDlgSaveProgress::StepProgress(LONG nSteps)
{
    ProgressYield();
    while (--nSteps >= 0) {
        m_progress.StepIt();
    }

    return m_bWasCanceled;
}

void CDlgSaveProgress::SetStepCount(LONG nSteps)
{
#if _MFC_VER >= 0x0600
    m_progress.SetRange32(0, nSteps);
#else
    m_progress.SetRange(0, nSteps);
#endif
    m_progress.SetPos(0);
    m_progress.SetStep(1);
}
