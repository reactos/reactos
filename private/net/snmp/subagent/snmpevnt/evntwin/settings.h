// settings.h : header file
//

#ifndef SETTINGS_H
#define	SETTINGS_H

/////////////////////////////////////////////////////////////////////////////
// CTrapSettingsDlg dialog
class CTrapSettingsDlg;
class CEventArray;

// this message is posted by background threads to the UI thread, requesting
// changes in the UI. wParam identifies the UI command (from the #defines below),
// lParam identifies the actual parameters of the command.
#define WM_UIREQUEST (WM_USER + 13)

// the 'enable' state of the 'Reset' button should be changed to the state indicated
// in lParam.
#define UICMD_ENABLE_RESET 1

UINT _thrRun(CTrapSettingsDlg *trapDlg);

class CTrapSettingsDlg : public CDialog
{
// Construction
public:
    UINT thrRun();

	CTrapSettingsDlg(CWnd* pParent = NULL);   // standard constructor
    BOOL EditSettings();

// Dialog Data
	//{{AFX_DATA(CTrapSettingsDlg)
	enum { IDD = IDD_SETTINGSDLG };
	CStatic	m_statTrapLength;
	CEdit	m_edtMessageLength;
	CEdit	m_edtSeconds;
	CEdit	m_edtTrapCount;
	CSpinButtonCtrl	m_spinMessageLength;
	CButton	m_btnResetThrottle;
	BOOL	m_bLimitMsgLength;
	//}}AFX_DATA

    BOOL m_bTrimMessagesFirst;
    BOOL m_bThrottleEnabled;

    CWinThread*     m_pthRegNotification;
    CEvent          m_evTermination;
    CEvent          m_evRegNotification;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrapSettingsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTrapSettingsDlg)
	afx_msg void OnLimitMessageLength();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnRadioDisable();
	afx_msg void OnRadioEable();
	afx_msg void OnButtonReset();
	afx_msg BOOL OnHelpInfo(HELPINFO*);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnClose();
    afx_msg LRESULT OnUIRequest(WPARAM cmd, LPARAM lParam);
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    void EnableThrottleWindows(BOOL bEnableThrottle);
    SCODE GetTrapsPerSecond(LONG* pnTraps, LONG* pnSeconds);
    SCODE GetMessageLength(LONG* pnChars);
    void TerminateBackgroundThread();
};

#endif //SETTINGS_H
