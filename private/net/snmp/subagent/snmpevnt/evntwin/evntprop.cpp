//***********************************************************************
// evntprop.cpp
//
// This file contains the implementation of the event properties dialog.
//
// Author: SEA
//
// History:
//      20-Febuary-1996     Larry A. French
//          Made various changes to this code.  However, much of it is
//          legacy code and in dire need of being rewritten.
//
//
// Copyright (C) 1995, 1996 Microsoft Corporation.  All rights reserved.
//
//************************************************************************


#include "stdafx.h"
#include "resource.h"
#include "eventrap.h"
#include "evntprop.h"
#include "trapreg.h"
#include "globals.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define MAX_EVENT_COUNT   32767
#define MAX_TIME_INTERVAL 32767



#define IsWithinRange(value, lower, upper) (((value) >= (lower)) && ((value) <= (upper)))

void RangeError(int iLower, int iUpper)
{
    
    TCHAR szBuffer[1024];
    CString sFormat;
    sFormat.LoadString(IDS_ERR_RANGE);
    _stprintf(szBuffer, (LPCTSTR) sFormat, iLower, iUpper);
    AfxMessageBox(szBuffer);
}


/////////////////////////////////////////////////////////////////////////////
// CEventPropertiesDlg dialog


CEventPropertiesDlg::CEventPropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEventPropertiesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEventPropertiesDlg)
	m_sDescription = _T("");
	m_sSource = _T("");
	m_sEventId = _T("");
	m_sLog = _T("");
	m_sSourceOID = _T("");
	m_sFullEventID = _T("");
	//}}AFX_DATA_INIT
}

BOOL CEventPropertiesDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

    m_spinEventCount.SetRange(1, MAX_EVENT_COUNT);
    if (m_iEventCount==0) {
        m_spinEventCount.SetPos(1);
    } else {
        m_spinEventCount.SetPos(m_iEventCount);
    }


    if (m_iTimeInterval == 0) {
        m_btnWithinTime.SetCheck(0);
        m_spinTimeInterval.SetRange(0, MAX_TIME_INTERVAL);
    }
    else {
        m_btnWithinTime.SetCheck(1);
        m_spinTimeInterval.SetRange(1, MAX_TIME_INTERVAL);
    }
    m_spinTimeInterval.SetPos(m_iTimeInterval);

    m_edtTimeInterval.EnableWindow(m_btnWithinTime.GetCheck() == 1);
    m_spinTimeInterval.EnableWindow(m_btnWithinTime.GetCheck() == 1);

    // If this is not a custom configuration, do not let the user
    // modify the configuration.
    if ((g_reg.GetConfigType() != CONFIG_TYPE_CUSTOM) || (g_reg.m_bRegIsReadOnly)) {
        m_btnOK.EnableWindow(FALSE);
    }
    m_bDidEditEventCount = FALSE;

    OnWithintime();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}





void CEventPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CEventPropertiesDlg)
	DDX_Control(pDX, IDC_WITHINTIME, m_btnWithinTime);
	DDX_Control(pDX, IDC_EVENTCOUNTSPN, m_spinEventCount);
	DDX_Control(pDX, IDC_TIMEINTRVLSPN, m_spinTimeInterval);
	DDX_Control(pDX, IDC_TIMEINTERVAL, m_edtTimeInterval);
	DDX_Control(pDX, IDC_EVENTCOUNT, m_edtEventCount);
	DDX_Control(pDX, IDOK, m_btnOK);
	DDX_Text(pDX, IDC_DESCRIPTION, m_sDescription);
	DDV_MaxChars(pDX, m_sDescription, 2048);
	DDX_Text(pDX, ID_STAT_SOURCE, m_sSource);
	DDV_MaxChars(pDX, m_sSource, 256);
	DDX_Text(pDX, ID_STAT_EVENTID, m_sEventId);
	DDX_Text(pDX, ID_STAT_LOG, m_sLog);
	DDX_Text(pDX, IDC_EDIT_ENTERPRISEOID, m_sSourceOID);
	DDX_Text(pDX, IDC_EDIT_FULL_EVENT_ID, m_sFullEventID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEventPropertiesDlg, CDialog)
	//{{AFX_MSG_MAP(CEventPropertiesDlg)
	ON_BN_CLICKED(IDC_WITHINTIME, OnWithintime)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEventPropertiesDlg message handlers

void CEventPropertiesDlg::OnOK() 
{
	// TODO: Add extra validation here
    int iLower, iUpper;
    CString sText;

    m_spinEventCount.GetRange(iLower, iUpper);

    // Validate the event count edit item and set m_iEventCount
    m_edtEventCount.GetWindowText(sText);
    if (!IsDecimalInteger(sText)) {
        RangeError(iLower, iUpper);
        m_edtEventCount.SetSel(0, -1);
        m_edtEventCount.SetFocus();
        return;
    }

    m_iEventCount = _ttoi(sText);

    if (!IsWithinRange(m_iEventCount, iLower, iUpper)) {
        RangeError(iLower, iUpper);
        sText.Format(_T("%u"), m_iEventCount);
        m_edtEventCount.SetWindowText(sText);
        m_edtEventCount.SetSel(0, -1);
        m_edtEventCount.SetFocus();
        return;
    }

    // Validate the time interval and set m_iTimeInterval        
    m_spinTimeInterval.GetRange(iLower, iUpper);
    m_edtTimeInterval.GetWindowText(sText);
    if (!IsDecimalInteger(sText)) {            
        RangeError(iLower, iUpper);
        m_edtTimeInterval.SetSel(0, -1);
        m_edtTimeInterval.SetFocus();
        return;
    }

    m_iTimeInterval = _ttoi(sText);
	if (m_btnWithinTime.GetCheck() == 1) {                          
		if (m_iEventCount < 2) {
			AfxMessageBox(IDS_ERR_PROP_TIME1);
            m_edtEventCount.SetSel(0, -1);
            m_edtEventCount.SetFocus();
            return;
		}

		if (m_iTimeInterval < 1) {
			AfxMessageBox(IDS_ERR_PROP_TIME2);
            sText.Format(_T("%u"), m_iTimeInterval);
            m_edtTimeInterval.SetWindowText(sText);
            m_edtTimeInterval.SetSel(0, -1);
            m_edtTimeInterval.SetFocus();
            return;
		}

        if (!IsWithinRange(m_iTimeInterval, iLower, iUpper)) {
            RangeError(iLower, iUpper);
            sText.Format(_T("%u"), m_iTimeInterval);
            m_edtTimeInterval.SetWindowText(sText);
            m_edtTimeInterval.SetSel(0, -1);
            m_edtTimeInterval.SetFocus();
            return;
        }
	}
	else if (m_iEventCount < 1) {	
		AfxMessageBox(IDS_ERR_PROP_TIME_LESS_THAN_TWO);
        m_edtEventCount.SetSel(0, -1);
        return;
	}

    CDialog::OnOK();

    // We don't set the g_reg.m_bIsDirty flag here because we want to see if the
    // user actually changed the current settings.  This check is made in 
    // CEventPropertiesDlg::EditEventProperties on a per-event basis.
}


void CEventPropertiesDlg::OnWithintime() 
{
	// The WithinTime checkbox was clicked.
	// Enable/disable the TimeInterval control.

    
    // Check to see if the count field has been edited.  If it has been edited,
    // mark the field as being dirty.

    if (m_edtEventCount.IsDirty() || m_spinEventCount.IsDirty()) {
        m_bDidEditEventCount = TRUE;
    }

    int iEventCount;
    int iTemp;
    SCODE sc = m_edtEventCount.GetValue(iEventCount);
    if (FAILED(sc)) {
        m_spinEventCount.GetRange(iEventCount, iTemp);
        m_spinEventCount.SetPos(iEventCount);
        m_bDidEditEventCount = FALSE;
    }

	if (m_btnWithinTime.GetCheck() == 1) {
        m_edtTimeInterval.EnableWindow(TRUE);
        m_spinTimeInterval.EnableWindow(TRUE);

        if (iEventCount < 2) {
            // If the event count is less than two, it will flip to two when the spin button's
            // range is set.  In this event, we make it appear as if the user never edited the
            // value so that it will flip back when the check box is unchecked.
            m_bDidEditEventCount = FALSE;
            m_bDidFlipEventCount = TRUE;
            m_edtEventCount.ClearDirty();
            m_spinEventCount.ClearDirty();
            m_spinEventCount.SetPos(2);
        }


   	    m_spinEventCount.SetRange(2, MAX_EVENT_COUNT);
	    m_spinTimeInterval.SetRange(1, MAX_TIME_INTERVAL);
	}
	else {
        m_edtTimeInterval.EnableWindow(FALSE);
        m_spinTimeInterval.EnableWindow(FALSE);

   	    m_spinEventCount.SetRange(1, MAX_EVENT_COUNT);
        m_spinEventCount.SetPos(iEventCount);
	    m_spinTimeInterval.SetRange(0, MAX_TIME_INTERVAL);
		m_spinTimeInterval.SetPos(0);


        // If the initial event count was one and we flipped it to two when the "within time"
        // button was clicked, then flip it back to one now if it was not edited.
        if (m_bDidFlipEventCount) {       
            if (!m_bDidEditEventCount) {
                m_spinEventCount.SetPos(1);
            }
            m_bDidFlipEventCount = FALSE;
        }
	}
	m_spinTimeInterval.SetRedraw();
}



//***************************************************************************
//
//  CEventPropertiesDlg::MakeLabelsBold
//
//  This method makes the static labels bold to enhance the appearance of
//  the dialog.
//
//	This method should be called after CDIalog::InitDialog.
//
//  Parameters:
//		None.
//
//  Returns:
//		Nothing.
//
//  Status:
//		The stupid MFC2.0 library makes the labels invisible when an attempt
//		is made to change the font of a static item.  I've tried this with
//		MFC4.0 and it works.  
//      
//***************************************************************************
void CEventPropertiesDlg::MakeLabelsBold()
{
#if 0
	CFont* pfontDefault;
	LOGFONT lf;

 	// Get the LOGFONT for the default static item font and then
	// switch the logfont weight to bold.
	pfontDefault = m_statSource.GetFont();
	pfontDefault->GetObject(sizeof(lf), &lf);
	lf.lfWeight = FW_BOLD;

	// Create a bold font with all other characteristics the same as the
	// default font.  Then switch all labels to a bold font. 
	CFont fontNew;
	if (fontNew.CreateFontIndirect(&lf)) {
		m_statSource.SetFont(&fontNew, TRUE);
		m_statLog.SetFont(&fontNew, TRUE);
		m_statEventID.SetFont(&fontNew, TRUE);
		m_statEnterpriseOID.SetFont(&fontNew, TRUE);		
	}

#endif //0
}





//********************************************************************
// CEventPropertiesDlg::EditEventProperties
//
// Edit the properties of a number of events.
//
// Parameters:
//      CEventArray& aEvents
//          An array of CEvent pointers.  These are the events that
//          are to be edited.
//
// Returns:
//      BOOL
//          TRUE if the user clicked OK and the events were edited.
//          FALSE if the user clicked Cancel and the events were not
//          edited.
//
//******************************************************************
BOOL CEventPropertiesDlg::EditEventProperties(CXEventArray& aEvents)
{
    LONG nEvents = aEvents.GetSize();
    if (nEvents == 0) {
        return TRUE;
    }


    // The first event is taken as a representative of the other
    // events.  Copy the appropriate data from this event to the
    // dialog.
    CString sText;

    CXEvent* pEvent = aEvents[0];
    CXEventSource* pEventSource = pEvent->m_pEventSource;
    CXEventLog* pEventLog = pEventSource->m_pEventLog;

    LONG iEvent;
    BOOL bMultipleSources = FALSE;
    BOOL bMultipleLogs = FALSE;
    for (iEvent=0; iEvent < nEvents; ++iEvent) {
        pEvent = aEvents[iEvent];
        if (pEvent->m_pEventSource != pEventSource) {
            bMultipleSources = TRUE;
        }
        if (pEvent->m_pEventSource->m_pEventLog != pEventLog) {
            bMultipleLogs = TRUE;
        }
    }

    if (bMultipleSources) {
        m_sSource.LoadString(IDS_MULTIPLE_SEL);
        m_sSourceOID.LoadString(IDS_MULTIPLE_SEL);
    }
    else {    
        m_sSource = pEventSource->m_sName;
        pEventSource->GetEnterpriseOID(m_sSourceOID, TRUE);
    }

    if (bMultipleLogs) {        
        m_sLog.LoadString(IDS_MULTIPLE_SEL);
    }
    else {
        m_sLog = pEventSource->m_pEventLog->m_sName;
    }

    // Copy the initial values.
    m_iTimeInterval = (int) pEvent->m_dwTimeInterval;
    m_iEventCount = pEvent->m_dwCount;
    m_bDidFlipEventCount = FALSE;
//    m_bWithinTime = (m_iTimeInterval != 0);


    if (nEvents > 1) {
        m_sEventId.LoadString(IDS_MULTIPLE_SEL);
        m_sDescription.LoadString(IDS_MULTIPLE_SEL);
        m_sFullEventID.LoadString(IDS_MULTIPLE_SEL);
    }
    else {
        pEvent->m_message.GetShortId(m_sEventId);
        m_sDescription = pEvent->m_message.m_sText;
        DecString(m_sFullEventID, pEvent->m_message.m_dwId);
    }

    
    // Put up the dialog and let the user edit the data.
    BOOL bDidCancel = (DoModal() == IDCANCEL);
    if (bDidCancel) {
        // The user canceled the dialog, so do nothing.
        return FALSE;
    }

    // Control comes here if the user clicked OK.  Now we need to copy the
    // user's settings to each event that we are editing and mark the registry
    // as dirty if any of the settings changed.
    for (iEvent=0; iEvent < nEvents; ++iEvent) {
        pEvent = aEvents[iEvent];
        if (pEvent->m_dwTimeInterval != (DWORD) m_iTimeInterval) {
            g_reg.SetDirty(TRUE);
            pEvent->m_dwTimeInterval = (DWORD) m_iTimeInterval;
        }

        if (pEvent->m_dwCount !=  (DWORD) m_iEventCount) {
            g_reg.SetDirty(TRUE);
            pEvent->m_dwCount = m_iEventCount;
        }
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CEditField

CEditField::CEditField()
{
    m_bIsDirty = FALSE;
}

CEditField::~CEditField()
{
}


BEGIN_MESSAGE_MAP(CEditField, CEdit)
	//{{AFX_MSG_MAP(CEditField)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEditField message handlers

void CEditField::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default	
	CEdit::OnChar(nChar, nRepCnt, nFlags);
    m_bIsDirty = TRUE;
}

SCODE CEditField::GetValue(int& iValue)
{
    CString sValue;
    GetWindowText(sValue);
    if (!IsDecimalInteger(sValue)) {
        return E_FAIL;
    }

    iValue = _ttoi(sValue);
    return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
// CEditSpin

CEditSpin::CEditSpin()
{
    m_bIsDirty = FALSE;
    m_iSetPos = 0;
}

CEditSpin::~CEditSpin()
{
}


BEGIN_MESSAGE_MAP(CEditSpin, CSpinButtonCtrl)
	//{{AFX_MSG_MAP(CEditSpin)
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEditSpin message handlers

void CEditSpin::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CSpinButtonCtrl::OnLButtonUp(nFlags, point);
    if (GetPos() != m_iSetPos) {
        m_bIsDirty = TRUE;
    }
}


int CEditSpin::SetPos(int iPos)
{
    int iResult = CSpinButtonCtrl::SetPos(iPos);
    m_iSetPos = GetPos();
    m_bIsDirty = FALSE;
    return iResult;
}


void CEditSpin::SetRange(int iLower, int iUpper)
{
	int iPos = GetPos();
    CSpinButtonCtrl::SetRange(iLower, iUpper);

	if (iPos < iLower) {
		iPos = iLower;
	}
	
	if (iPos > iUpper) {
		iPos = iUpper;
	}

	SetPos(iPos);
    SetRedraw();

    m_iSetPos = iLower;
    m_bIsDirty = FALSE;
}


BOOL CEditSpin::IsDirty()
{
    int iCurPos = GetPos();

    return (m_bIsDirty || (m_iSetPos != iCurPos));
}

BOOL CEventPropertiesDlg::OnHelpInfo(HELPINFO *pHelpInfo) 
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW &&
        pHelpInfo->iCtrlId != IDD_NULL)
	{
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
                   AfxGetApp()->m_pszHelpFilePath,
                   HELP_WM_HELP,
                   (ULONG_PTR)g_aHelpIDs_IDD_PROPERTIESDLG);
	}
	
	return TRUE;
}

void CEventPropertiesDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (pWnd == this)
		return;

    ::WinHelp (pWnd->m_hWnd,
		       AfxGetApp()->m_pszHelpFilePath,
		       HELP_CONTEXTMENU,
		       (ULONG_PTR)g_aHelpIDs_IDD_PROPERTIESDLG);	
}
