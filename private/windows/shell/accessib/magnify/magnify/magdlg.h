// MagDlg.h : header file
//

#if !defined(AFX_MAGDLG_H__C7D0DB68_D691_11D0_AD59_00C04FC2A136__INCLUDED_)
#define AFX_MAGDLG_H__C7D0DB68_D691_11D0_AD59_00C04FC2A136__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "MagBar.h" // For CMagBar
#include "NumEdit.h" // For CNumberEdit

/////////////////////////////////////////////////////////////////////////////
// CMagnifyDlg dialog
// Desktop Identifiers.

class CMagnifyDlg : public CDialog
{
	void UpdateState();
	UINT fuModifiersToggleMouseTracking, vkToggleMouseTracking;
	UINT fuModifiersToggleInvertColors, vkToggleInvertColors;
	UINT fuModifiersCopyToClipboard, vkCopyToClipboard;
	UINT fuModifiersCopyToClipboard2, vkCopyToClipboard2;
	UINT fuModifiersHideMagnifier, vkHideMagnifier;
	enum { nMinZoom = 1, nMaxZoom = 9 };
	HANDLE m_hEventAlreadyRunning;
	BOOL m_fOnInitDialogCalledYet;
	CMagBar m_wndStationary;
	BOOL CanExit();
	BOOL m_fAccessTimeOutOverWritten;
	ACCESSTIMEOUT m_AccTimeOut;
	void OverWriteAccessTimeOut();
	void ClearAccessTimeOut();
    void SaveSettings();

// Construction
public:
	CMagnifyDlg(CWnd* pParent = NULL);	// standard constructor
	~CMagnifyDlg();

	void SetStationaryHidden(); // Hide the magnifier window

// Dialog Data
	//{{AFX_DATA(CMagnifyDlg)
	enum { IDD = IDD_MAGNIFY_DIALOG };
	CNumberEdit	m_NumEdit;
	CSpinButtonCtrl	m_wndMagLevSpin;
	BOOL	m_fStationaryTrackText;
	BOOL	m_fStationaryTrackSecondaryFocus;
	BOOL	m_fStationaryTrackCursor;
	BOOL	m_fStationaryTrackFocus;
	int		m_nStationaryMagLevel;
	BOOL	m_fStationaryInvertColors;
	BOOL	m_fStationaryUseHighContrast;
	BOOL	m_fStationaryStartMinimized;
	BOOL	m_fStationaryShowMagnifier;
	BOOL	m_fShowWarning;
	//}}AFX_DATA
	BOOL    m_fUseHotKeys;
	DWORD   idEvent;

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMagnifyDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void CheckHighContrastSetting();
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMagnifyDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg void OnClose();
	virtual void OnCancel();
	afx_msg void OnChangeStationarymaglevel();
	afx_msg void OnHelp();
	afx_msg void OnExit();
	afx_msg void OnStationarytrackmousecursor();
	afx_msg void OnStationarytrackkybdfocus();
	afx_msg void OnStationarytracksecondaryfocus();
	afx_msg void OnStationarytracktext();
	afx_msg void OnStationaryinvertcolors();
	afx_msg void OnStationaryhighcontrast();
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnStationaryshowmagnifier();
	afx_msg void OnStationarystartminimized();
	afx_msg void OnTimer(UINT);
	//}}AFX_MSG
	LRESULT OnHotKey(WPARAM wParam, LPARAM lParam);
	LRESULT OnDelayedMinimize(WPARAM wParam, LPARAM lParam);
	afx_msg void OnEndSession( BOOL bEnding );

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAGDLG_H__C7D0DB68_D691_11D0_AD59_00C04FC2A136__INCLUDED_)
