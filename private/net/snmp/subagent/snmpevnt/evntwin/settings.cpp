//***********************************************************************
// settings.cpp
//
// This file contains the implementation of the "Settings" dialog class.
//
//
// Author: SEA
//
// History:
//      Febuary-1996     Larry A. French
//          Modified the code to fix various problems.  Regrettably, this
//          file still contains a fair amount of legacy code that I didn't
//          have time to fully rewrite.  Also, I did not have time to go 
//          though and fully comment the code.
//
//
// Copyright (C) 1995, 1996 Microsoft Corporation.  All rights reserved.
//
//************************************************************************



// settings.cpp : implementation file
//

#include "stdafx.h"
#include "eventrap.h"
#include "settings.h"
#include "globals.h"
#include "trapreg.h"

// This macro handles comparing bool values for the cases where TRUE can be and
// non-zero value.
#define BOOLS_ARE_DIFFERENT(b1, b2) ((b1 & !b2) || (!b1 & b2)) 


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

UINT _thrRun(CTrapSettingsDlg *trapDlg)
{
    return trapDlg->thrRun();
}

/////////////////////////////////////////////////////////////////////////////
// CTrapSettingsDlg dialog

UINT CTrapSettingsDlg::thrRun()
{
    HANDLE hEvents[2];
    DWORD retCode;
    CRegistryKey regkey;
    CRegistryValue regval;
    BOOL bThrottleIsTripped = FALSE;

    hEvents[0] = (HANDLE)m_evRegNotification;
    hEvents[1] = (HANDLE)m_evTermination;
    
    if (!g_reg.m_regkeySnmp.GetSubKey(SZ_REGKEY_PARAMETERS, regkey))
        return 0;

    do
    {
        m_evRegNotification.SetEvent();

        if (RegNotifyChangeKeyValue(
                regkey.m_hkeyOpen,
                TRUE,
                REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_NAME,
                (HANDLE)m_evRegNotification,
                TRUE) == ERROR_SUCCESS)
        {
            if (regkey.GetValue(SZ_REGKEY_PARAMS_THRESHOLD, regval) && 
                *(DWORD*)regval.m_pData == THROTTLE_TRIPPED)
                PostMessage(WM_UIREQUEST, UICMD_ENABLE_RESET, TRUE);
            else
                PostMessage(WM_UIREQUEST, UICMD_ENABLE_RESET, FALSE);
        }
    } while(WaitForMultipleObjects(2, hEvents, FALSE, INFINITE) == WAIT_OBJECT_0);

    regkey.Close();

    return 0;
}

CTrapSettingsDlg::CTrapSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTrapSettingsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTrapSettingsDlg)
	m_bLimitMsgLength = FALSE;
	//}}AFX_DATA_INIT
}


#define I_MAX_LONG 0x7fffffffL
#define I_MIN_TRAPCOUNT 2
#define I_MAX_TRAPCOUNT 9999999

#define I_MIN_SECONDS   1
#define I_MAX_SECONDS   9999999

#define I_MIN_MESSAGE_LENGTH 400
#define I_MAX_MESSAGE_LENGTH 0x7fff




SCODE CTrapSettingsDlg::GetMessageLength(LONG* pnChars)
{
    CString sValue;

	CButton* pbtnLimit = (CButton*)GetDlgItem(IDC_LIMITMSGLNGTH);
    BOOL bLimitEnabled = (pbtnLimit->GetCheck() == 1) ? TRUE : FALSE;
        

    m_edtMessageLength.GetWindowText(sValue);
    SCODE sc;
    LONG nChars = _ttol(sValue);
    sc = AsciiToLong(sValue, &nChars);
    if (FAILED(sc))
    {
        // They shouldn't have garbage in this edit control even if
        // they haven't selected a message limit.  Let the user fix it.
        AfxMessageBox(IDS_ERR_SETTINGS_MESSAGELENGTH_NOT_INT);
        m_edtMessageLength.SetFocus();
        m_edtMessageLength.SetSel(0, -1);
        return E_FAIL;
    }


    if (bLimitEnabled)
    {
        if (nChars < I_MIN_MESSAGE_LENGTH || nChars > I_MAX_MESSAGE_LENGTH)
        {
            if (pbtnLimit->GetCheck() == 1)
            {
                CString sError;
                CString sRangeMessage;
                sError.LoadString(IDS_SETTINGS_MESSAGE_LENGTH_RANGE);
                GenerateRangeMessage(sRangeMessage, I_MIN_MESSAGE_LENGTH, I_MAX_MESSAGE_LENGTH);
                sError += sRangeMessage;
                AfxMessageBox(sError);
                sValue.Format(_T("%u"),nChars);
                m_edtMessageLength.SetWindowText(sValue);
                m_edtMessageLength.SetFocus();
                m_edtMessageLength.SetSel(0, -1);
                return E_FAIL;
            }
        }
    }
    *pnChars = nChars;
    return S_OK;
}

SCODE CTrapSettingsDlg::GetTrapsPerSecond(LONG* pnTraps, LONG* pnSeconds)
{
    CString sSeconds;
    CString sTraps;
    CString sError;
    CString sRangeMessage;
    LONG nTraps;
    LONG nSeconds;
    SCODE sc;

    // First make sure that the trap count and seconds fields don't have garbage in them.
    // If a non-integer value is specified, force the user to fix it regardless of whether
    // or not the throttle is enabled.
    m_edtTrapCount.GetWindowText(sTraps);
    sc = AsciiToLong(sTraps, &nTraps);
    if (FAILED(sc))
    {
        AfxMessageBox(IDS_ERR_SETTINGS_TRAPCOUNT_NOT_INT);
        m_edtTrapCount.SetFocus();
        m_edtTrapCount.SetSel(0, -1);
        return E_FAIL;
    }

    m_edtSeconds.GetWindowText(sSeconds);
    sc = AsciiToLong(sSeconds, &nSeconds);
    if (FAILED(sc))
    {
        AfxMessageBox(IDS_ERR_SETTINGS_TRAPSECONDS_NOT_INT);
        m_edtSeconds.SetFocus();
        m_edtSeconds.SetSel(0, -1);
        return E_FAIL;
    }

    BOOL bThrottleEnabled;
    if (GetCheckedRadioButton(IDC_RADIO_ENABLE, IDC_RADIO_DISABLE) == IDC_RADIO_ENABLE)
        bThrottleEnabled = TRUE;
    else
        bThrottleEnabled = FALSE;

    if (bThrottleEnabled)
    {
        if  (nTraps < I_MIN_TRAPCOUNT || nTraps > I_MAX_TRAPCOUNT)
        {
            sError.LoadString(IDS_ERR_SETTINGS_TRAPCOUNT_RANGE);
            GenerateRangeMessage(sRangeMessage, I_MIN_TRAPCOUNT, I_MAX_TRAPCOUNT);
            sError += sRangeMessage;
            AfxMessageBox(sError);
            sTraps.Format(_T("%u"), nTraps);
            m_edtTrapCount.SetWindowText(sTraps);
            m_edtTrapCount.SetFocus();
            m_edtTrapCount.SetSel(0, -1);
            return E_FAIL;
        }
    
        if (nSeconds < I_MIN_SECONDS || nSeconds > I_MAX_SECONDS)
        {
            sError.LoadString(IDS_SETTINGS_TRAPSECONDS_RANGE);
            GenerateRangeMessage(sRangeMessage, I_MIN_SECONDS, I_MAX_SECONDS);
            sError += sRangeMessage;
            AfxMessageBox(sError);
            sSeconds.Format(_T("%u"),nSeconds);
            m_edtSeconds.SetWindowText(sSeconds);
            m_edtSeconds.SetFocus();
            m_edtSeconds.SetSel(0, -1);
            return E_FAIL;
        }
    }
    

    *pnTraps = nTraps;
    *pnSeconds = nSeconds;

    return S_OK;
}

void CTrapSettingsDlg::TerminateBackgroundThread()
{
    if (m_pthRegNotification)
    {
	    m_evTermination.SetEvent();
        WaitForSingleObject(m_pthRegNotification->m_hThread, INFINITE);
    }
}

void CTrapSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTrapSettingsDlg)
	DDX_Control(pDX, IDC_STAT_TRAP_LENGTH, m_statTrapLength);
	DDX_Control(pDX, IDC_EDIT_MESSAGELENGTH, m_edtMessageLength);
	DDX_Control(pDX, IDC_EDIT_TRAP_SECONDS, m_edtSeconds);
	DDX_Control(pDX, IDC_EDIT_TRAP_COUNT, m_edtTrapCount);
	DDX_Control(pDX, IDC_MSGLENGTHSPN, m_spinMessageLength);
	DDX_Control(pDX, IDC_BUTTON_RESET, m_btnResetThrottle);
	DDX_Check(pDX, IDC_LIMITMSGLNGTH, m_bLimitMsgLength);
	//}}AFX_DATA_MAP


    CString sValue;
    if (pDX->m_bSaveAndValidate) {
        // Saving the value trapsize, seconds, and trapcount is handled by 
        // CTrapSettingsDlg::OnOK so that it can set the focus back to the
        // offending item if the value is out of range. If the data transfer
        // fails here, the focus is always set back to the dialog and not
        // the offending item (is there a way around this?)
    }
    else {

        m_spinMessageLength.SetRange(I_MIN_MESSAGE_LENGTH, I_MAX_MESSAGE_LENGTH);
        m_spinMessageLength.SetPos(g_reg.m_params.m_trapsize.m_dwMaxTrapSize);

        DecString(sValue, g_reg.m_params.m_throttle.m_nSeconds);
        m_edtSeconds.SetWindowText(sValue);

        DecString(sValue, g_reg.m_params.m_throttle.m_nTraps);
        m_edtTrapCount.SetWindowText(sValue);


    }            
}


BEGIN_MESSAGE_MAP(CTrapSettingsDlg, CDialog)
	//{{AFX_MSG_MAP(CTrapSettingsDlg)
	ON_BN_CLICKED(IDC_LIMITMSGLNGTH, OnLimitMessageLength)
	ON_BN_CLICKED(IDC_RADIO_DISABLE, OnRadioDisable)
	ON_BN_CLICKED(IDC_RADIO_ENABLE, OnRadioEable)
	ON_BN_CLICKED(IDC_BUTTON_RESET, OnButtonReset)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_WM_CLOSE()
    ON_MESSAGE(WM_UIREQUEST, OnUIRequest)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTrapSettingsDlg message handlers

LRESULT CTrapSettingsDlg::OnUIRequest(WPARAM cmd, LPARAM lParam)
{
    switch(cmd)
    {
    case UICMD_ENABLE_RESET:
        m_btnResetThrottle.EnableWindow((BOOL)lParam);
        break;
    default:
        break;
    }

    return (LRESULT)0;
}

void CTrapSettingsDlg::OnLimitMessageLength() 
{
	// The LimitMsgLength checkbox was clicked.
	// Enable/disable the edit control.

	// Get the controls.
    CButton* pbtnLimitBox = (CButton*) GetDlgItem(IDC_LIMITMSGLNGTH);
    CButton *pRadio1 = (CButton*)GetDlgItem(IDC_RADIO1);
    CButton *pRadio2 = (CButton*)GetDlgItem(IDC_RADIO2);
	
	// It's checked; enable
	if (pbtnLimitBox->GetCheck() == 1)
    {
        m_edtMessageLength.EnableWindow();
		pRadio1->EnableWindow();
		pRadio2->EnableWindow();
        GetDlgItem(IDC_STATIC_BYTES)->EnableWindow();
        m_statTrapLength.EnableWindow();
    }
	// Disable
	else
    {
        m_edtMessageLength.EnableWindow(FALSE);
		pRadio1->EnableWindow(FALSE);
		pRadio2->EnableWindow(FALSE);
        GetDlgItem(IDC_STATIC_BYTES)->EnableWindow(FALSE);
        m_statTrapLength.EnableWindow(FALSE);
    }
}


BOOL CTrapSettingsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

    CButton *pRadio1 = (CButton*)GetDlgItem(IDC_RADIO1);
    CButton *pRadio2 = (CButton*)GetDlgItem(IDC_RADIO2);
	

    m_statTrapLength.EnableWindow(m_bLimitMsgLength);


    m_edtMessageLength.EnableWindow(m_bLimitMsgLength);
	if (m_bLimitMsgLength)
    {
		pRadio1->EnableWindow();
		pRadio2->EnableWindow();
        GetDlgItem(IDC_STATIC_BYTES)->EnableWindow();
    }
	// Disable
	else
    {
		pRadio1->EnableWindow(FALSE);
		pRadio2->EnableWindow(FALSE);
        GetDlgItem(IDC_STATIC_BYTES)->EnableWindow(FALSE);
    }



    if (m_bTrimMessagesFirst)
        CheckRadioButton(IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
    else
        CheckRadioButton(IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);

    if (m_bThrottleEnabled) 
        CheckRadioButton(IDC_RADIO_ENABLE, IDC_RADIO_DISABLE, IDC_RADIO_ENABLE);
    else
        CheckRadioButton(IDC_RADIO_ENABLE, IDC_RADIO_DISABLE, IDC_RADIO_DISABLE);

    EnableThrottleWindows(m_bThrottleEnabled);

    m_pthRegNotification = AfxBeginThread((AFX_THREADPROC)_thrRun, this);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



void CTrapSettingsDlg::OnOK() 
{

    LONG nchMessageLength;
    LONG nTraps;
    LONG nSeconds;
    SCODE sc = GetMessageLength(&nchMessageLength);
    if (FAILED(sc))
    {
        return;
    }

    sc = GetTrapsPerSecond(&nTraps, &nSeconds);
    if (FAILED(sc))
    {
        return;
    }


    // Pull various values off of the dialog and store them into member variables.
    // Note that there are other member variables that are set directly
    // as a response to user input.
    //===========================================================================
    m_bTrimMessagesFirst = (GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO2) == IDC_RADIO2);
    m_bThrottleEnabled = (GetCheckedRadioButton(IDC_RADIO_ENABLE, IDC_RADIO_DISABLE) == IDC_RADIO_ENABLE);


    if (g_reg.m_params.m_trapsize.m_dwMaxTrapSize != (DWORD) nchMessageLength) {
        g_reg.SetDirty(TRUE);
        g_reg.m_params.m_trapsize.m_dwMaxTrapSize = nchMessageLength;
    }

    if(g_reg.m_params.m_throttle.m_nSeconds != nSeconds) {
        g_reg.SetDirty(TRUE);
        g_reg.m_params.m_throttle.m_nSeconds = nSeconds;
    }

    if (g_reg.m_params.m_throttle.m_nTraps != nTraps) {
        g_reg.SetDirty(TRUE);
        g_reg.m_params.m_throttle.m_nTraps = nTraps;
    }

    TerminateBackgroundThread();
	CDialog::OnOK();
}




BOOL CTrapSettingsDlg::EditSettings()
{
    m_bLimitMsgLength = g_reg.m_params.m_trapsize.m_bTrimFlag;
    m_bTrimMessagesFirst = g_reg.m_params.m_trapsize.m_bTrimMessages;
    m_bThrottleEnabled = g_reg.m_params.m_throttle.m_bIsEnabled;


    // Save the data.
    if (DoModal() == IDOK)
    {
        if (BOOLS_ARE_DIFFERENT(g_reg.m_params.m_trapsize.m_bTrimFlag, m_bLimitMsgLength)) {
            g_reg.m_params.m_trapsize.m_bTrimFlag = m_bLimitMsgLength;
            g_reg.SetDirty(TRUE);
        }

        if (BOOLS_ARE_DIFFERENT(g_reg.m_params.m_trapsize.m_bTrimMessages, m_bTrimMessagesFirst)) {
            g_reg.m_params.m_trapsize.m_bTrimMessages = m_bTrimMessagesFirst;
            g_reg.SetDirty(TRUE);
        }

        if (BOOLS_ARE_DIFFERENT(g_reg.m_params.m_throttle.m_bIsEnabled, m_bThrottleEnabled)) {
            g_reg.m_params.m_throttle.m_bIsEnabled = m_bThrottleEnabled;
            g_reg.SetDirty(TRUE);
        }
                
        return TRUE;
    }
    else {
        return FALSE;
    }

}




void CTrapSettingsDlg::OnRadioDisable() 
{
    EnableThrottleWindows(FALSE);
}

void CTrapSettingsDlg::OnRadioEable() 
{
    EnableThrottleWindows(TRUE);
}

void CTrapSettingsDlg::EnableThrottleWindows(BOOL bEnableThrottle)
{
    m_edtSeconds.EnableWindow(bEnableThrottle);
    GetDlgItem(IDC_STATIC_MSG)->EnableWindow(bEnableThrottle);
    GetDlgItem(IDC_STATIC_NTRAPS)->EnableWindow(bEnableThrottle);
    GetDlgItem(IDC_STATIC_INTERVAL)->EnableWindow(bEnableThrottle);
    m_edtTrapCount.EnableWindow(bEnableThrottle);
}


//****************************************************************
// CTrapSettingsDlg::OnButtonReset
//
// Reset the extension agent so that it starts sending traps again.
// The extension agent will stop sending traps if the throttle limit
// is exceeded (more than x number of traps per second are set).
//
// Parameters:
//      None.
//
// Returns.
//      Nothing.
//
//*****************************************************************
void CTrapSettingsDlg::OnButtonReset() 
{
    if (SUCCEEDED(g_reg.m_params.ResetExtensionAgent())) {
        m_btnResetThrottle.EnableWindow(FALSE);
    }
}

BOOL CTrapSettingsDlg::OnHelpInfo(HELPINFO *pHelpInfo) 
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW &&
        pHelpInfo->iCtrlId != IDC_STATIC_MSG &&
        pHelpInfo->iCtrlId != IDC_STATIC_BYTES)
	{
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
                   AfxGetApp()->m_pszHelpFilePath,
                   HELP_WM_HELP,
                   (ULONG_PTR)g_aHelpIDs_IDD_SETTINGSDLG);
	}
	
	return TRUE;
}

void CTrapSettingsDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (this == pWnd)
		return;

    ::WinHelp (pWnd->m_hWnd,
		       AfxGetApp()->m_pszHelpFilePath,
		       HELP_CONTEXTMENU,
		       (ULONG_PTR)g_aHelpIDs_IDD_SETTINGSDLG);
}

void CTrapSettingsDlg::OnClose() 
{
    TerminateBackgroundThread();
	CDialog::OnClose();
}

void CTrapSettingsDlg::OnCancel() 
{
    TerminateBackgroundThread();
	CDialog::OnCancel();
}
