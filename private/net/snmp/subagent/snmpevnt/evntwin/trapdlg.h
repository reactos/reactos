//******************************************************************
// trapdlg.h
//
// This is the header file for the main dialog for eventrap.
//
// Author: Larry A. French
//
// History:
//      December-1995       SEA - Wrote it
//          SEA - wrote it.
//
//      20-Febuary-1996     Larry A. French
//          Totally rewrote it to fix the spagetti code and huge
//          methods.  The original author seemed to have little or
//          no ability to form meaningful abstractions.
//
//
// Copyright (C) 1995, 1996 Microsoft Corporation.  All rights reserved.
//******************************************************************

#ifndef TRAPDLG_H
#define TRAPDLG_H

#include "regkey.h"
#include "source.h"         // The message source container
#include "tcsource.h"       // The message source tree control
#include "lcsource.h"       // The message source list control
#include "lcevents.h"
#include "trapreg.h"
#include "layout.h"
#include "export.h"


class CMainLayout;
class CExtendedLayout;




/////////////////////////////////////////////////////////////////////////////
// CEventTrapDlg dialog

class CEventTrapDlg : public CDialog
{
// Construction
public:
	CEventTrapDlg(CWnd* pParent = NULL);   // standard constructor
	~CEventTrapDlg();

    BOOL IsExtendedView() {return m_bExtendedView; }
    void NotifySourceSelChanged();

	CSource     m_source;           // The message source


// Dialog Data
	//{{AFX_DATA(CEventTrapDlg)
	enum { IDD = IDD_EVNTTRAPDLG };
	CButton	m_btnApply;
	CButton	m_btnExport;
	CLcEvents	m_lcEvents;
	CTcSource	m_tcSource;
    CStatic m_statLabel0;
    CStatic	m_statLabel1;
	CStatic	m_statLabel2;
	CLcSource m_lcSource;
	CButton	m_btnOK;
	CButton	m_btnCancel;
	CButton	m_btnSettings;
	CButton	m_btnProps;
	CButton	m_btnView;
	CButton	m_btnRemove;
	CButton	m_btnAdd;
	CButton	m_btnFind;
	CButton	m_btnConfigTypeBox;
    CButton m_btnConfigTypeCustom;
    CButton m_btnConfigTypeDefault;
	//}}AFX_DATA



// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventTrapDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEventTrapDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnAdd();
	afx_msg void OnProperties();
	afx_msg void OnSettings();
	virtual void OnOK();
	afx_msg void OnDblclkEventlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickEventlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnView();
	afx_msg void OnRemove();
	afx_msg void OnFind();
	afx_msg void OnSelchangedTvSources(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickLvSources(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkLvSources(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonExport();
	virtual void OnCancel();
	afx_msg void OnKeydownEventlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedEventlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedLvSources(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRadioCustom();
	afx_msg void OnRadioDefault();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg BOOL OnHelpInfo(HELPINFO*);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnApply();
	afx_msg void OnDefault();
	afx_msg void OnTvSourcesExpanded(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

protected:
    HICON m_hIcon;

private:
    void CheckEventlistSelection();
    void CheckSourcelistSelection();
    void UpdateDialogTitle();


    BOOL m_bSaveInProgress;
	BOOL m_bExtendedView;
    CLayout m_layout;
    CString m_sExportTitle;
    CDlgExport m_dlgExport;
    CString m_sBaseDialogCaption;
};


#endif //TRAPDLG_H
