//******************************************************************
// trapdlg.cpp
//
// This is the source file for eventrap's main dialog.
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

#include "stdafx.h"
#include "Eventrap.h"
#include "trapdlg.h"
#include "evntprop.h"
#include "settings.h"
#include "busy.h"
#include "trapreg.h"
#include "globals.h"
#include "evntfind.h"
#include "export.h"
#include "dlgsavep.h"

//#include "smsalloc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg message handlers

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CenterWindow();
	
	// TODO: Add extra about dlg initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}


/////////////////////////////////////////////////////////////////////////////
// CEventTrapDlg dialog

CEventTrapDlg::CEventTrapDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CEventTrapDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CEventTrapDlg)
	//}}AFX_DATA_INIT


    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINICON);

    m_source.Create(this);
	m_bExtendedView = FALSE;
    m_bSaveInProgress = FALSE;
}

CEventTrapDlg::~CEventTrapDlg()
{
    PostQuitMessage(0);
}

void CEventTrapDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CEventTrapDlg)
	DDX_Control(pDX, IDC_APPLY, m_btnApply);
	DDX_Control(pDX, ID_BUTTON_EXPORT, m_btnExport);
	DDX_Control(pDX, IDC_EVENTLIST, m_lcEvents);
	DDX_Control(pDX, IDC_TV_SOURCES, m_tcSource);
    DDX_Control(pDX, IDC_STAT_LABEL0, m_statLabel0);
	DDX_Control(pDX, IDC_STAT_LABEL1, m_statLabel1);
	DDX_Control(pDX, IDC_STAT_LABEL2, m_statLabel2);
    DDX_Control(pDX, IDC_LV_SOURCES, m_lcSource);
	DDX_Control(pDX, IDOK, m_btnOK);
	DDX_Control(pDX, IDCANCEL, m_btnCancel);
	DDX_Control(pDX, ID_SETTINGS, m_btnSettings);
	DDX_Control(pDX, ID_PROPERTIES, m_btnProps);
	DDX_Control(pDX, ID_VIEW, m_btnView);
	DDX_Control(pDX, ID_REMOVE, m_btnRemove);
	DDX_Control(pDX, ID_ADD, m_btnAdd);
	DDX_Control(pDX, ID_FIND, m_btnFind);
	DDX_Control(pDX, IDC_STAT_GRP_CONFIG_TYPE, m_btnConfigTypeBox);
    DDX_Control(pDX, IDC_RADIO_CUSTOM, m_btnConfigTypeCustom);
    DDX_Control(pDX, IDC_RADIO_DEFAULT, m_btnConfigTypeDefault);
	//}}AFX_DATA_MAP

}

BEGIN_MESSAGE_MAP(CEventTrapDlg, CDialog)
    //{{AFX_MSG_MAP(CEventTrapDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(ID_ADD, OnAdd)
    ON_BN_CLICKED(ID_PROPERTIES, OnProperties)
    ON_BN_CLICKED(ID_SETTINGS, OnSettings)
	ON_NOTIFY(NM_DBLCLK, IDC_EVENTLIST, OnDblclkEventlist)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_EVENTLIST, OnColumnclickEventlist)
	ON_WM_SIZE()
	ON_BN_CLICKED(ID_VIEW, OnView)
	ON_BN_CLICKED(ID_REMOVE, OnRemove)
	ON_BN_CLICKED(ID_FIND, OnFind)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TV_SOURCES, OnSelchangedTvSources)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LV_SOURCES, OnColumnclickLvSources)
	ON_NOTIFY(NM_DBLCLK, IDC_LV_SOURCES, OnDblclkLvSources)
	ON_BN_CLICKED(ID_BUTTON_EXPORT, OnButtonExport)
	ON_NOTIFY(LVN_KEYDOWN, IDC_EVENTLIST, OnKeydownEventlist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_EVENTLIST, OnItemchangedEventlist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LV_SOURCES, OnItemchangedLvSources)
	ON_BN_CLICKED(IDC_RADIO_CUSTOM, OnRadioCustom)
	ON_BN_CLICKED(IDC_RADIO_DEFAULT, OnRadioDefault)
	ON_WM_DRAWITEM()
	ON_COMMAND(ID_HELP, OnHelp)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_APPLY, OnApply)
	ON_BN_CLICKED(IDC_DEFAULT, OnDefault)
	ON_NOTIFY(TVN_ITEMEXPANDED, IDC_TV_SOURCES, OnTvSourcesExpanded)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventTrapDlg message handlers

void CEventTrapDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
/*
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
*/
	CDialog::OnSysCommand(nID, lParam);
    m_lcEvents.SetFocus();

}

void CEventTrapDlg::OnDestroy()
{
	CDialog::OnDestroy();
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CEventTrapDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEventTrapDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


//*************************************************************************
// CEventTrapDlg::OnInitDialog
//
// Initialize the dialog.
//

// Parameters:
//      None.
//
// Returns:
//      BOOL
//          TRUE if Windows should set the focus to the first control
//          in the dialog box.  FALSE if the focus has already been set
//          and Windows should leave it alone.
//
//*************************************************************************
BOOL CEventTrapDlg::OnInitDialog()
{
    CBusy busy;
    CDialog::OnInitDialog();
	CenterWindow();

//    VERIFY(m_lcSource.SubclassDlgItem(IDC_LV_SOURCES, this));


    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    m_layout.Initialize(this);

    // Layout the dialog view for the small (non-extended) view
    m_bExtendedView = FALSE;
    m_layout.LayoutView(FALSE);

    // The registry class is keeping a pointer to the 'Apply' pointer in order to
    // enable disable it according to the 'dirty' state.
    g_reg.SetApplyButton(&m_btnApply);

    // Step the progress indicator for loading the configuration.
    // Note that if you add more steps here, you must modify
    // CTrapReg::CTrapReg to account for these extra steps.
    //=========================================================
    g_reg.m_pdlgLoadProgress->StepProgress();
    ++g_reg.m_nLoadSteps;

    CString sText;
    sText.LoadString(IDS_TITLE_EDIT_BUTTON);
    m_btnView.SetWindowText(sText);

    // Notify the message source container and the events list control
    // that this dialog has been initialized so that they can initialize
    // their windows and so on.  Note that this must be called after the
    // g_reg.m_aEventLogs is deserialized because information contained therein
    // will be displayed
	m_source.CreateWindowEpilogue();
    g_reg.m_pdlgLoadProgress->StepProgress();
    ++g_reg.m_nLoadSteps;


    m_lcEvents.CreateWindowEpilogue();			
    g_reg.m_pdlgLoadProgress->StepProgress();
    ++g_reg.m_nLoadSteps;


    m_lcEvents.AddEvents(m_source, g_reg.m_aEventLogs);
    g_reg.m_pdlgLoadProgress->StepProgress();
    ++g_reg.m_nLoadSteps;


    m_sExportTitle.LoadString(IDS_EXPORT_DEFAULT_FILENAME);

    CheckEventlistSelection();
    m_btnAdd.EnableWindow(FALSE);

    if ((g_reg.GetConfigType() == CONFIG_TYPE_CUSTOM)) {
        CheckRadioButton(IDC_RADIO_CUSTOM, IDC_RADIO_DEFAULT, IDC_RADIO_CUSTOM);
    }
    else {
        CheckRadioButton(IDC_RADIO_CUSTOM, IDC_RADIO_DEFAULT, IDC_RADIO_DEFAULT);
    }


    if ((g_reg.GetConfigType() == CONFIG_TYPE_CUSTOM) && !g_reg.m_bRegIsReadOnly) {
        m_btnView.EnableWindow(TRUE);
    }
    else {
        m_btnView.EnableWindow(FALSE);
    }


    // If eventrap will be used without the SMS Admin UI, then we want to hide the
    // configuration type group box because it it meaningless if SMS will not be
    // distributing jobs containing the default configuration.
    if (!g_reg.m_bShowConfigTypeBox) {
    	m_btnConfigTypeBox.ShowWindow(SW_HIDE);
        m_btnConfigTypeCustom.ShowWindow(SW_HIDE);
        m_btnConfigTypeDefault.ShowWindow(SW_HIDE);
    }


    // Now that we know what the configuration type is, we can update the
    // dialog's title.  But first we will save the default dialog title so
    // that we can use it as the base that will be extended with an optional
    // machine name and configuration type.
    GetWindowText(m_sBaseDialogCaption);
    UpdateDialogTitle();

    delete g_reg.m_pdlgLoadProgress;
    g_reg.m_pdlgLoadProgress = NULL;

    // initially, once the registry gets loaded, the dirty state is 'false'
    g_reg.SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


//*************************************************************************
// CEventTrapDlg::OnAdd
//
// Add the messages that are currently selected in the message source list
// to the event list.
//
// The event list is the list control in the upper part of the dialog box.
// The message source list is the list control in the lower-right side of
// the dialog box.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//*************************************************************************
void CEventTrapDlg::OnAdd()
{
    CBusy busy;

    // Get an array containing the selected messages
    CXMessageArray aMessages;
    m_source.GetSelectedMessages(aMessages);
    if (aMessages.GetSize() == 0) {
        AfxMessageBox(IDS_WARNING_NO_MESSAGE_SELECTED);
        m_lcEvents.SetFocus();
        return;
    }

    // Create a set of events corresponding to the messages.
    CXEventArray aEvents;
    CXEventArray aEventsAlreadyTrapped;
    LONG nMessages = aMessages.GetSize();
    m_lcEvents.UpdateWindow();

    aEvents.RemoveAll();
    for (LONG iMessage = 0; iMessage < nMessages; ++iMessage) {
        CXMessage* pMessage = aMessages[iMessage];
        CXEvent* pEvent;
        pEvent = pMessage->m_pEventSource->FindEvent(pMessage->m_dwId);
        if (pEvent == NULL) {
            CXEvent* pEvent = new CXEvent(pMessage);
            aEvents.Add(pEvent);
        }
        else {
            aEventsAlreadyTrapped.Add(pEvent);
        }
    }

    if (aEvents.GetSize() > 0) {
        // Now we need to ask the user for the "settings" for these events.
        CEventPropertiesDlg dlg;
        if (!dlg.EditEventProperties(aEvents)) {
            aEvents.DeleteAll();
            m_lcEvents.SetFocus();
            return;
        }

        m_lcEvents.AddEvents(m_source, aEvents);

        aEvents.RemoveAll();
        g_reg.SetDirty(TRUE);
    }

    if (aEventsAlreadyTrapped.GetSize() > 0) {
        m_lcEvents.SelectEvents(aEventsAlreadyTrapped);
        aEventsAlreadyTrapped.RemoveAll();
        if (nMessages == aEventsAlreadyTrapped.GetSize()) {
            AfxMessageBox(IDS_ALREADYTRAPPING);
        }
        else {
            AfxMessageBox(IDS_SOMETRAPPING);
        }
    }
    m_lcEvents.SetFocus();

}








//*************************************************************************
// CEventTrapDlg::OnProperties
//
// Edit the properties of the selected events in the event-list.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//*************************************************************************
void CEventTrapDlg::OnProperties()
{
    CXEventArray aEvents;
    m_lcEvents.GetSelectedEvents(aEvents);

    // Nothing selected.
    if (aEvents.GetSize() == 0)
    {
        CString sMsg;
        sMsg.LoadString(IDS_MSG_SELECTEVENT);
        MessageBox(sMsg, NULL, MB_ICONEXCLAMATION);
    }
    else {
        // Put up the dialog to edit the event properties.
        CEventPropertiesDlg dlg;
        if (dlg.EditEventProperties(aEvents)) {
            m_lcEvents.RefreshEvents(aEvents);
        }
    }
    m_lcEvents.SetFocus();
}



//*************************************************************************
// CEventTrapDlg::OnSettings
//
// Edit the global settings.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//*************************************************************************
void CEventTrapDlg::OnSettings()
{
    // Setup and load the dialog.
    CTrapSettingsDlg dlg(this);
    dlg.EditSettings();
    m_lcEvents.SetFocus();
}



//*************************************************************************
// CEventTrapDlg::OnRemove
//
// Remove the events currently selected in the CLcEvents list control.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//*************************************************************************
void CEventTrapDlg::OnRemove()
{
    // If nothing was selected, warn the user.
    CString sText;
    if (!m_lcEvents.HasSelection()) {
        sText.LoadString(IDS_MSG_SELECTEVENT);
        MessageBox(sText, NULL, MB_ICONEXCLAMATION);
        return;  // Nothing to do.
    }

    // Make sure the user wants to delete these items.
    sText.LoadString(IDS_MSG_DELETEEVENT);
    if (MessageBox(sText, NULL, MB_ICONQUESTION | MB_OKCANCEL) != IDOK)
        return;

    // We must notify the source control that the events are deleted
    // so that the trapping flag can be updated.
    CBusy busy;
    m_lcEvents.DeleteSelectedEvents(m_source);
    g_reg.SetDirty(TRUE);

    // All of the selected events were removed, so now there is no selection
    // and the export and properties buttons should be disabled.
    m_btnProps.EnableWindow(FALSE);
    m_btnExport.EnableWindow(FALSE);
    m_btnRemove.EnableWindow(FALSE);
    m_lcEvents.SetFocus();
}




//*********************************************************************
// CEventTrapDlg::OnOK
//
// This method is called when the "OK" button is clicked.  All we
// have to do is save the current configuration.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//********************************************************************
void CEventTrapDlg::OnOK()
{
    CBusy busy;

    // Set the save in progress flag so that we aren't interrupted in the
    // middle of writing to the registry.
    m_bSaveInProgress = TRUE;

    // Clear the "lost connection" flag so that the user can attempt to save
    // again.
    SCODE sc = g_reg.Serialize();
    if ((sc == S_SAVE_CANCELED) || FAILED(sc)) {
        // Control comes here if the user elected to cancel the save.  We clear
        // the m_bSaveInProgress dialog so that the user can cancel out of this
        // application altogether if he or she chooses to do so.
        m_bSaveInProgress = FALSE;
        return;
    }

    CDialog::OnOK();
    delete this;
}

//*********************************************************************
// CEventTrapDlg::OnApply
//
// This method is called when the "Apply" button is clicked.  All we
// have to do is save the current configuration.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//********************************************************************
void CEventTrapDlg::OnApply()
{
    CBusy busy;

    // Set the save in progress flag so that we aren't interrupted in the
    // middle of writing to the registry.
    m_bSaveInProgress = TRUE;

    // Clear the "lost connection" flag so that the user can attempt to save
    // again.
    SCODE sc = g_reg.Serialize();

    // Control comes here if the user elected to cancel the save.  We clear
    // the m_bSaveInProgress dialog so that the user can cancel out of this
    // application altogether if he or she chooses to do so.
    m_bSaveInProgress = FALSE;
}


//********************************************************************
// CEventTrapDlg::OnDblclkEventlist
//
// This method is called when the user double clicks an item within
// the event-list.  This is equivallent to clicking the "Properties"
// button.
//
// Parameters:
//      NMHDR* pNMHDR
//
//      LRESULT* pResult
//
// Returns:
//      Nothing.
//******************************************************************
void CEventTrapDlg::OnDblclkEventlist(NMHDR* pNMHDR, LRESULT* pResult)
{
    OnProperties();
	*pResult = 0;
}





//************************************************************************
// CEventTrapDlg::OnColumnclickEventlist
//
// This method is called when the user click a column header in the
// eventlist.  When this occurs, the event list must be resorted
// according to the criteria for that column.
//
// Ideally, this method would be a member of the CLcEvents class, but the
// class wizard and MFC wouldn't let me do it (MFC4.0 and VC++4.0 do let
// you do it).
//
// Parameters:
//      See the MFC documentation.
//
// Returns:
//      Nothing.
//
//***********************************************************************
void CEventTrapDlg::OnColumnclickEventlist(NMHDR* pNMHDR, LRESULT* pResult)
{
    CBusy busy;

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    ASSERT(pNMListView->iSubItem < ICOL_LcEvents_MAX);

    // First flip the sort order for the column and then do the sort.
    g_abLcEventsSortAscending[pNMListView->iSubItem] = ! g_abLcEventsSortAscending[pNMListView->iSubItem];
    m_lcEvents.SortItems(pNMListView->iSubItem);
	*pResult = 0;
}




//************************************************************************
// CEventTrapDlg::OnSize
//
// This method is called when the trap dialog changes sizes.  When this
// occurs, the dialog layout must be recalculated because the dialog is
// laid out dynamically.
//
// Parameters:
//      See the MFC documentation.
//
// Returns:
//      Nothing.
//
//************************************************************************
void CEventTrapDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (!::IsWindow(m_btnOK.m_hWnd)) {
		return;
	}

	m_layout.LayoutAndRedraw(m_bExtendedView, cx, cy);
}






//*********************************************************************
// CEventTrapDlg::OnView
//
// This method is called when the user clicks the View/Edit button.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//*********************************************************************
void CEventTrapDlg::OnView()
{
    // Flip the normal/extended view type and redo the dialog layout
    // to reflect the change.
	m_bExtendedView = !m_bExtendedView;	
    m_layout.LayoutView(m_bExtendedView);

    // Flip the title of the View/Edit button to the other state.
    CString sText;
    sText.LoadString(m_bExtendedView ? IDS_TITLE_VIEW_BUTTON : IDS_TITLE_EDIT_BUTTON);
    m_btnView.SetWindowText(sText);
    if (m_bExtendedView)
        m_tcSource.SetFocus();
    else
        m_lcEvents.SetFocus();
}




//********************************************************************
// CEventTrapDlg::OnFind
//
// This method is called when the user clicks the "Find" button.  Pass
// the notification onto the CSource object.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//********************************************************************
void CEventTrapDlg::OnFind()
{
    m_source.OnFind(this);	
}


//********************************************************************
// CEventTrapDlg::OnSelchangedTvSources
//
// This method is changed when the message source treeview selection
// changes.  Ideally, this method would be part of the CTcSource class,
// but MFC3.0 doesn't allow this (or at least you can't do it through
// the VC++ class wizard).  So, the message needs to be passed along
// to the CTcSource class.
//
// Parameters:
//      See the MFC documentation.
//
// Returns:
//      Nothing.
//
//********************************************************************
void CEventTrapDlg::OnSelchangedTvSources(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	
    // We update the window so that the old selection will be unhighlighted
    // immediately. This is useful if it takes a long time to complete
    // the selection change.  Without the update, the user may get the impression
    // that there is a multiple selection.
    m_tcSource.UpdateWindow();
   	m_tcSource.SelChanged();
	*pResult = 0;
}



//*******************************************************************
// CEventTrapDlg::OnColumnclickLvSources
//
// This method is called when a column is clicked in the message source
// listview. When this occurs, the messages must be resorted according
// to the sorting criteria for the clicked column.
//
// Parameters:
//      See the MFC documentation.
//
// Returns:
//      Nothing.
//
//******************************************************************
void CEventTrapDlg::OnColumnclickLvSources(NMHDR* pNMHDR, LRESULT* pResult)
{
    CBusy busy;

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    ASSERT(pNMListView->iSubItem < ICOL_LcSource_MAX);

    // First flip the sort order for the column and then do the sort.
    g_abLcSourceSortAscending[pNMListView->iSubItem] = ! g_abLcSourceSortAscending[pNMListView->iSubItem];
	m_lcSource.SortItems(pNMListView->iSubItem);
	*pResult = 0;

}


//*******************************************************************
// CEventTrapDlg::OnDblclkLvSources
//
// This method is called when the user double clicks a message in the
// source list.  This is equivallent to clicking the "Add" button.
//
// Parameters:
//      See the MFC documentation.
//
// Returns:
//      Nothing.
//
//*******************************************************************
void CEventTrapDlg::OnDblclkLvSources(NMHDR* pNMHDR, LRESULT* pResult)
{
	OnAdd();
	*pResult = 0;
}



//********************************************************************
// CEventTrapDlg::OnButtonExport
//
// This method is called when the "Export" button is clicked. This is
// where events can be exported by writing a trap-text or trap tool
// files corresponding to the selected events.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//********************************************************************
void CEventTrapDlg::OnButtonExport()
{
    CXEventArray aEvents;
    m_lcEvents.GetSelectedEvents(aEvents);

    // Nothing selected.
    if (aEvents.GetSize() == 0)
    {
        AfxMessageBox(IDS_MSG_SELECTEVENT, MB_ICONEXCLAMATION);
    }
    else {
        m_dlgExport.DoModal(aEvents);
    }


    m_lcEvents.SetFocus();
}



//*******************************************************************
// CEventTrapDlg::OnCancel
//
// This method is called when the cancel button is clicked.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//*******************************************************************
void CEventTrapDlg::OnCancel()
{
    if (m_bSaveInProgress) {
        return;
    }

	CDialog::OnCancel();
    delete this;
}



//********************************************************************
// CEventTrapDlg::CheckEventlistSelection
//
// Check to see if any events are currently selected in the event
// list. If no events are selected, then the buttons that operate on
// events are disabled.  If at least one event is selected  then the
// buttons that operate on events are enabled.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//*******************************************************************
void CEventTrapDlg::CheckEventlistSelection()
{
    LONG nSelected = m_lcEvents.GetSelectedCount();
    if (nSelected > 0) {
        m_btnProps.EnableWindow(TRUE);
        m_btnExport.EnableWindow(TRUE);
        m_btnRemove.EnableWindow(TRUE);
    }
    else {
        m_btnProps.EnableWindow(FALSE);
        m_btnExport.EnableWindow(FALSE);
        m_btnRemove.EnableWindow(FALSE);
    }

}



//********************************************************************
// CEventTrapDlg::CheckSourcelistSelection
//
// Check to see if any messages are currently selected in the message
// source list. If no messages are selected, then the "Add" button needs
// to be disabled.  If one or more messages are selected, then the "Add"
// button is enabled, allowing the user to add the message to the event
// list.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//*******************************************************************
void CEventTrapDlg::CheckSourcelistSelection()
{
    LONG nSelected = m_lcSource.GetSelectedCount();
    if (nSelected > 0) {
        m_btnAdd.EnableWindow(TRUE);
    }
    else {
        m_btnAdd.EnableWindow(FALSE);
    }
}



//********************************************************************
// CEventTrapDlg::OnKeydownEventlist
//
// This method is called when a keydown message is sent to the
// event list.  There are reasons why we monitor keydown events here:
//
//      1. To delete the selected event when the user hits the delete key.
//
// Parameters:
//      See the MFC documentation.
//
// Returns:
//      Nothing.
//
//********************************************************************
void CEventTrapDlg::OnKeydownEventlist(NMHDR* pNMHDR, LRESULT* pResult)
{

    #define VKEY_DELETE 46
	LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;

    // Check to see if the delete key was entered.  If so, delete the
    // selected event.  Note that events can be deleted only if this
    // is a "Custom" configuration.
    if (pLVKeyDow->wVKey == VKEY_DELETE) {
        if (g_reg.GetConfigType() == CONFIG_TYPE_CUSTOM) {
            if (pLVKeyDow->wVKey == VKEY_DELETE) {
                OnRemove();
            }
        	*pResult = 0;

        } else {
            MessageBeep(MB_ICONQUESTION);
        }
    }

}



//***************************************************************************
// CEventTrapDlg::OnItemchangedEventlist
//
// This method is called when an item changes in the event list.  When this
// occurs, various buttons may have to be enabled or disabled depending on
// whether or not anything is selected.
//
// Parameters:
//      Please see the MFC documentation.
//
// Returns:
//      Nothing.
//
//**************************************************************************
void CEventTrapDlg::OnItemchangedEventlist(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
    CheckEventlistSelection();
	
	*pResult = 0;
}



//***********************************************************************
// CEventTrapDlg::OnItemchangedLvSources
//
// This method is called when an item in the message source list changes.
// When this occurs, buttons such as "Add" and "Remove" may have to
// be enabled or disabled depending on whether or not anything is selected
// in the list.
//
// Parameters:
//      NMHDR* pNMHDR
//
//      LRESULT* pResult
//
// Returns:
//      Nothing.
//
//***********************************************************************

void CEventTrapDlg::OnItemchangedLvSources(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    CheckSourcelistSelection();
	*pResult = 0;
}


//*********************************************************************
// CEventTrapDlg::NotifySourceSelChanged
//
// This method is called when the selection changes in the message source
// list.  When this occurs, buttons such as "Add" and "Remove" may have
// to be enabled or disabled depending on whether or not anything is
// selected in the list.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//*********************************************************************
void CEventTrapDlg::NotifySourceSelChanged()
{
    CheckSourcelistSelection();
}



//*********************************************************************
// CEventTrapDlg::OnRadioCustom
//
// This method is called when the "Custom" radio button in the
// "Configuration Type" groupbox is clicked. When the user selects
// the custom configuration type, he or she is allowed to edit the
// current configuration.  Also the registry will be marked so that
// the next time SMS distributes an "Event to Trap" configuration job,
// the current configuration will not be replaced with the default configuration.
//
// There are three possible configuration states: custom, default, and
// default pending.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//**********************************************************************
void CEventTrapDlg::OnRadioCustom()
{
    CheckRadioButton(IDC_RADIO_CUSTOM, IDC_RADIO_DEFAULT, IDC_RADIO_CUSTOM);
    if (!g_reg.m_bRegIsReadOnly) {
        m_btnView.EnableWindow(TRUE);
    }
    g_reg.SetConfigType(CONFIG_TYPE_CUSTOM);
    UpdateDialogTitle();
}


//*********************************************************************
// CEventTrapDlg::OnRadioDefault
//
// This method is called when the "Default" radio button in the
// "Configuration Type" groupbox is clicked. When the user selects
// the default configuration, he or she is prevented from editing the
// current configuration.  Also the registry will be marked so that
// the next time SMS distributes an "Event to Trap" configuration job,
// the current configuration will be replaced with the default configuration.
//
// There are three possible configuration states: custom, default, and
// default pending.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//**********************************************************************
void CEventTrapDlg::OnRadioDefault()
{
    CheckRadioButton(IDC_RADIO_CUSTOM, IDC_RADIO_DEFAULT, IDC_RADIO_DEFAULT);

    // When the "Default" configuration is slected, the user is not allowed
    // to edit the event list, so if the extended dialog view is currently
    // being displayed, it is flipped back to the non-extended state and
    // the edit button is disabled.
    if (m_bExtendedView) {
        OnView();
    }
    m_btnView.EnableWindow(FALSE);

    // Mark the registry with the current config type so that when the
    // SMS event to trap job comes, it knows that it can overwrite the
    // current settings.
    g_reg.SetConfigType(CONFIG_TYPE_DEFAULT);

    // Update the dialog title to indicate the configuration state.
    UpdateDialogTitle();
}



//**********************************************************************
// CEventTrapDlg::UpdateDialogTitle
//
// This method updates the dialog's title.  The format of the title is
//
//      Event to Trap Translator - Machine Name - [configuration type]
//
// If the registry of the local machine is being edited, then the
// machine name is omitted.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//**********************************************************************
void CEventTrapDlg::UpdateDialogTitle()
{
    // Map the configuration type to a string-table resource id.
    LONG idsConfigType;
    switch(g_reg.GetConfigType()) {
    case CONFIG_TYPE_CUSTOM:
        idsConfigType = IDS_CONFIGTYPE_CUSTOM;
        break;
    case CONFIG_TYPE_DEFAULT:
        idsConfigType = IDS_CONFIGTYPE_DEFAULT;
        break;
    case CONFIG_TYPE_DEFAULT_PENDING:
        idsConfigType = IDS_CONFIGTYPE_DEFAULT_PENDING;
        break;
    default:
        ASSERT(FALSE);
        break;
    }

    CString sConfigType;
    sConfigType.LoadString(idsConfigType);

    CString sCaption = m_sBaseDialogCaption;
    if (!g_reg.m_sComputerName.IsEmpty()) {
        sCaption = sCaption + _T(" - ") + g_reg.m_sComputerName;
    }
    sCaption = sCaption + _T(" - [") + sConfigType + _T(']');
    SetWindowText(sCaption);

}

void CEventTrapDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	// TODO: Add your message handler code here and/or call default
	
	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

BOOL CEventTrapDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
                   AfxGetApp()->m_pszHelpFilePath,
                   HELP_WM_HELP,
                   (ULONG_PTR)g_aHelpIDs_IDD_EVNTTRAPDLG);
	}
	
	return TRUE;
}

void CEventTrapDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	CMenu contextMenus;

    if (this == pWnd)
		return;

	contextMenus.LoadMenu(IDR_CTXMENUS);

	if (pWnd->m_hWnd == m_lcEvents.m_hWnd)
	{
		CMenu * pMenuLcEvents;

		pMenuLcEvents = contextMenus.GetSubMenu(0);

		if (pMenuLcEvents != NULL)
		{

			if (!m_lcEvents.HasSelection())
			{
				pMenuLcEvents->EnableMenuItem(0, MF_GRAYED | MF_BYPOSITION);
				pMenuLcEvents->EnableMenuItem(3, MF_GRAYED | MF_BYPOSITION);
				pMenuLcEvents->EnableMenuItem(5, MF_GRAYED | MF_BYPOSITION);
			}

			pMenuLcEvents->TrackPopupMenu(
				TPM_LEFTALIGN | TPM_LEFTBUTTON,
				point.x,
				point.y,
				this,
				NULL);
		}
	}
	else if (pWnd->m_hWnd == m_lcSource.m_hWnd)
	{
		CMenu *pMenuLcSource;

		pMenuLcSource = contextMenus.GetSubMenu(1);

		if (pMenuLcSource != NULL)
		{
			if (m_lcSource.GetNextItem(-1, LVNI_SELECTED) == -1)
			{
				pMenuLcSource->EnableMenuItem(0, MF_GRAYED | MF_BYPOSITION);
			}

			pMenuLcSource->TrackPopupMenu(
				TPM_LEFTALIGN | TPM_LEFTBUTTON,
				point.x,
				point.y,
				this,
				NULL);
		}
	}
	else
	{
	   ::WinHelp (pWnd->m_hWnd,
              AfxGetApp()->m_pszHelpFilePath,
              HELP_CONTEXTMENU,
              (ULONG_PTR)g_aHelpIDs_IDD_EVNTTRAPDLG);
	}
}

void CEventTrapDlg::OnDefault()
{
	HTREEITEM hti;
	DWORD ctrlID = GetFocus()->GetDlgCtrlID();

	switch(ctrlID)
	{
	case IDC_EVENTLIST:
		if (m_lcEvents.HasSelection())
			OnProperties();
		else
			OnSettings();
		break;
	case IDC_TV_SOURCES:
		hti = m_tcSource.GetSelectedItem();
		if (hti != NULL)
			m_tcSource.Expand(hti, TVE_TOGGLE);
		break;
	case IDC_LV_SOURCES:
		OnAdd();
		m_lcSource.SetFocus();
		break;
	case IDC_RADIO_CUSTOM:
		OnRadioDefault();
		m_btnConfigTypeDefault.SetFocus();
		break;
	case IDC_RADIO_DEFAULT:
		OnRadioCustom();
		m_btnConfigTypeCustom.SetFocus();
		break;
	default:
		OnOK();
	}
}

void CEventTrapDlg::OnTvSourcesExpanded(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    INT          nImage = (pNMTreeView->itemNew.state & TVIS_EXPANDED) ?
                                1 : // node is expanded -> 'open' folder icon is the second in the list
                                0 ; // node is contracted -> 'close' folder icon is the first in the list
	// TODO: Add your control notification handler code here

    m_tcSource.SetItemImage(pNMTreeView->itemNew.hItem, nImage, nImage);

	*pResult = 0;
}
