// dlgsavep.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgSaveProgress dialog

class CDlgSaveProgress : public CDialog
{
// Construction
public:
	CDlgSaveProgress(BOOL bIsSaving = FALSE);   // standard constructor
    BOOL StepProgress(LONG nSteps = 1);
    void SetStepCount(LONG nSteps);

// Dialog Data
	//{{AFX_DATA(CDlgSaveProgress)
	enum { IDD = IDD_SAVE_PROGRESS };
	CProgressCtrl	m_progress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgSaveProgress)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgSaveProgress)
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    void ProgressYield();
    BOOL m_bWasCanceled;
    BOOL m_bIsSaving;
};
