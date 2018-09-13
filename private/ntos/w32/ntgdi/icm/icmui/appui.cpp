/******************************************************************************

  Source File:  AppUI.CPP

  This file implements the Application UI.  This consists of two (ANSI/UNICODE)
  functions that allow an application to specify profiles to be used, and so
  forth.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:
  12-11-96  a-robkj@microsoft.com   Created it

******************************************************************************/

#include    "ICMUI.H"

CONST DWORD ApplicationUIHelpIds[] = {
    ApplyButton,          IDH_ICMUI_APPLY,
    EnableICM,            IDH_APPUI_ICM,
    EnableBasic,          IDH_APPUI_BASIC,
    EnableProofing,       IDH_APPUI_PROOF,
    MonitorProfile,       IDH_APPUI_MONITOR,
    MonitorProfileLabel,  IDH_APPUI_MONITOR,
    PrinterProfile,       IDH_APPUI_PRINTER,
    PrinterProfileLabel,  IDH_APPUI_PRINTER,
    RenderingIntent,      IDH_APPUI_INTENT,
    RenderingIntentLabel, IDH_APPUI_INTENT,
    TargetProfile,        IDH_APPUI_EMULATE,
    TargetProfileLabel,   IDH_APPUI_EMULATE,
    TargetIntent,         IDH_APPUI_INTENT,
    TargetIntentLabel,    IDH_APPUI_INTENT,
#if !defined(_WIN95_) // context-sentitive help
//  SourceProfile,        IDH_APPUI_SOURCE,
    SourceProfileLabel,   IDH_APPUI_SOURCE,
#endif
    0, 0
};

class   CColorMatchDialog : public CDialog {

    BOOL            m_bSuccess, m_bEnableICM, m_bEnableProofing, m_bColorPrinter;

    CString         m_csSource, m_csMonitor, m_csPrinter,
                    m_csMonitorProfile, m_csPrinterProfile, m_csTargetProfile;

    CString         m_csMonitorDisplayName; // since displayName != deviceName for monitor.

    CStringArray    m_csaMonitor, m_csaPrinter, m_csaTarget;
    CStringArray    m_csaMonitorDesc, m_csaPrinterDesc, m_csaTargetDesc;

    //  For handy reference (from Setup structure)

    DWORD   m_dwRenderIntent, m_dwProofIntent;

    //  To reduce stack usage and code size

    HWND    m_hwndRenderIntent, m_hwndTargetIntent, m_hwndMonitorList,
            m_hwndPrinterList, m_hwndTargetList, m_hwndIntentText1,
            m_hwndIntentLabel, m_hwndTargetProfileLabel, m_hwndTargetIntentLabel,
            m_hwndMonitorProfileLabel, m_hwndPrinterProfileLabel;

    //  For Apply callback

    PCMSCALLBACK m_dpApplyCallback;
    LPARAM       m_lpApplyCallback;

    //  To identify callee

    BOOL    m_bAnsiCall;

    //  Display profile description or filename

    BOOL    m_bUseProfileDescription;

    //  Intent control

    BOOL    m_bDisableIntent, m_bDisableRenderIntent;

    // Pointer to COLORMATCHSETUP[A|W]

    PVOID   m_pvCMS;

    void    Fail(DWORD dwError);
    BOOL    GoodParms(PCOLORMATCHSETUP pcms);
    void    CompleteInitialization();
    void    UpdateControls(BOOL bChanged = FALSE);

    void    FillStructure(COLORMATCHSETUPA *pcms);
    void    FillStructure(COLORMATCHSETUPW *pcms);

    void    EnableApplyButton(BOOL bEnable = TRUE);

public:

    CColorMatchDialog(COLORMATCHSETUPA *pcms);
    CColorMatchDialog(COLORMATCHSETUPW *pcms);

    ~CColorMatchDialog() {
        m_csaMonitor.Empty();
        m_csaPrinter.Empty();
        m_csaTarget.Empty();
        m_csaMonitorDesc.Empty();
        m_csaPrinterDesc.Empty();
        m_csaTargetDesc.Empty();
        m_csSource.Empty();
        m_csMonitor.Empty();
        m_csPrinter.Empty();
        m_csMonitorProfile.Empty();
        m_csPrinterProfile.Empty();
        m_csTargetProfile.Empty();
        m_csMonitorDisplayName.Empty();
    }

    BOOL    Results() const { return m_bSuccess; }

    virtual BOOL    OnInit();

    virtual BOOL OnCommand(WORD wNotifyCode, WORD wid, HWND hwndCtl);

    virtual BOOL OnHelp(LPHELPINFO pHelp);
    virtual BOOL OnContextMenu(HWND hwnd);
};

//  Record a failure

void    CColorMatchDialog::Fail(DWORD dwError) {
    SetLastError(dwError);
    m_bSuccess = FALSE;
}

//  Report input parameter validity

BOOL    CColorMatchDialog::GoodParms(PCOLORMATCHSETUP pcms) {

    m_bSuccess = TRUE;

    if  (!pcms || pcms -> dwVersion != COLOR_MATCH_VERSION ||
         pcms -> dwSize != sizeof *pcms || !pcms -> pMonitorProfile ||
         !pcms -> pPrinterProfile || !pcms -> pTargetProfile ||
         !pcms -> ccMonitorProfile || !pcms -> ccPrinterProfile ||
         !pcms -> ccTargetProfile) {
        Fail(ERROR_INVALID_PARAMETER);
        return  FALSE;
    }

    if  (pcms -> dwFlags & CMS_USEHOOK && !pcms -> lpfnHook)
        Fail(ERROR_INVALID_PARAMETER);

    if  (pcms -> dwFlags & CMS_USEAPPLYCALLBACK && !pcms -> lpfnApplyCallback)
        Fail(ERROR_INVALID_PARAMETER);

    if  (pcms -> dwFlags & CMS_SETRENDERINTENT &&
         pcms -> dwRenderIntent > INTENT_ABSOLUTE_COLORIMETRIC)
         Fail(ERROR_INVALID_PARAMETER);

    if  (pcms -> dwFlags & CMS_SETPROOFINTENT &&
         pcms -> dwFlags & CMS_ENABLEPROOFING &&
         pcms -> dwProofingIntent > INTENT_ABSOLUTE_COLORIMETRIC)
         Fail(ERROR_INVALID_PARAMETER);

    //  Setup the hooking, if needed

    if  (pcms -> dwFlags & CMS_USEHOOK) {
        m_dpHook = pcms -> lpfnHook;
        m_lpHook = pcms -> lParam;
    }

    //  Setup the callback for apply, if needed

    if  (pcms -> dwFlags & CMS_USEAPPLYCALLBACK) {
        m_dpApplyCallback = pcms -> lpfnApplyCallback;
        m_lpApplyCallback = pcms -> lParamApplyCallback;
    } else {
        m_dpApplyCallback = NULL;
        m_lpApplyCallback = 0L;
    }

    //  Cache the flags...

    DWORD   dwFlags = pcms -> dwFlags;

    //  Init the intents

    m_dwRenderIntent = (dwFlags & CMS_SETRENDERINTENT) ?
        pcms -> dwRenderIntent : INTENT_PERCEPTUAL;

    m_dwProofIntent = (dwFlags & CMS_SETPROOFINTENT) && (dwFlags & CMS_ENABLEPROOFING) ?
        pcms -> dwProofingIntent : m_dwRenderIntent;

    //  Init the flags

    m_bEnableICM = !(dwFlags & CMS_DISABLEICM);
    m_bEnableProofing = !!(dwFlags & CMS_ENABLEPROOFING);

    m_bUseProfileDescription = !!(dwFlags & CMS_USEDESCRIPTION);

    m_bDisableIntent = !!(dwFlags & CMS_DISABLEINTENT);
    m_bDisableRenderIntent = !!(dwFlags & CMS_DISABLERENDERINTENT);

    //  Init the pointer to buffer

    m_pvCMS = (PVOID) pcms;

    return  m_bSuccess;
}

//  Encoding-independent construction actions

void    CColorMatchDialog::CompleteInitialization() {

    //  Determine the appropriate source, monitor and printer names

    if  (m_csSource.IsEmpty())
        m_csSource.Load(DefaultSourceString);

    //  Check the validation for monitor name.

    CMonitorList    cml;
    cml.Enumerate();

    if  (!cml.IsValidDeviceName(m_csMonitor)) {

        // Get primary device.

        m_csMonitor = cml.PrimaryDeviceName();
    }

    //  Map display name from device name

    m_csMonitorDisplayName = cml.DeviceNameToDisplayName(m_csMonitor);

    //  Check the validation for printer name.

    HANDLE hPrinter;

    if  (!m_csPrinter.IsEmpty() && OpenPrinter(m_csPrinter,&hPrinter,NULL)) {

        // The specified printer has been found.

        ClosePrinter(hPrinter);

    } else {

        //  The specified printer has not been found,
        //  Get the default printer name- do it the old, slimy way...

        TCHAR   acBuffer[MAX_PATH];

        GetProfileString(_TEXT("Windows"), _TEXT("Device"), _TEXT(""),
            acBuffer, MAX_PATH);

        //  The buffer will contains "PrinterName,DriverName,Port".
        //  What we need is only printer name.

        TCHAR *pTmp = acBuffer;

        while (*pTmp) {
            if (*pTmp == __TEXT(',')) {
                *pTmp = NULL;
                break;
            }
            pTmp++;
        }

        m_csPrinter = acBuffer;
    }

    if  (CGlobals::ThisIsAColorPrinter(m_csPrinter))
        m_bColorPrinter = TRUE;
     else
        m_bColorPrinter = FALSE;

    //  Now, we collect the names

    ENUMTYPE et = { sizeof et, ENUM_TYPE_VERSION, (ET_DEVICENAME|ET_DEVICECLASS) };

    //  Enumrate monitor.

    et.pDeviceName   = m_csMonitor;
    et.dwDeviceClass = CLASS_MONITOR;
    CProfile::Enumerate(et, m_csaMonitor, m_csaMonitorDesc);

    //  Enumrate only for Color Printer.

    if (m_bColorPrinter) {
        et.pDeviceName   = m_csPrinter;
        et.dwDeviceClass = CLASS_PRINTER;
        CProfile::Enumerate(et, m_csaPrinter, m_csaPrinterDesc);
    } else {
        m_csaPrinter.Empty();
        m_csaPrinterDesc.Empty();
    }

    et.dwFields = 0;

    CProfile::Enumerate(et, m_csaTarget, m_csaTargetDesc);

    //  Fix up the default names for the profiles

    if  (m_csaPrinter.Map(m_csPrinterProfile) == m_csaPrinter.Count())
    {
        _RPTF2(_CRT_WARN, "Printer Profile %s isn't associated with "
            "the monitor (%s)", (LPCTSTR) m_csPrinterProfile,
            (LPCTSTR) m_csPrinter);
        if (m_csaPrinter.Count())
        {
            m_csPrinterProfile = m_csaPrinter[0];
        }
        else
        {
            m_csPrinterProfile = (LPCTSTR) NULL;
        }
    }

    if  (m_csaMonitor.Map(m_csMonitorProfile) == m_csaMonitor.Count())
    {
        _RPTF2(_CRT_WARN, "Monitor Profile %s isn't associated with "
            "the monitor (%s)", (LPCTSTR) m_csMonitorProfile,
            (LPCTSTR) m_csMonitor);
        if (m_csaMonitor.Count())
        {
            m_csMonitorProfile = m_csaMonitor[0];
        }
        else
        {
            m_csMonitorProfile = (LPCTSTR) NULL;
        }
    }

    //  If the target profile name is invalid, use the printer profile

    if  (m_csaTarget.Map(m_csTargetProfile) == m_csaTarget.Count())
    {
        _RPTF1(_CRT_WARN, "Target Profile %s isn't installed",
            (LPCTSTR) m_csTargetProfile);
        if (m_csaPrinter.Count())
        {
            m_csTargetProfile = m_csaPrinter[0];
        }
        else
        {
            // And then, there is no printer profile, it will
            // be Windows color space profile.

            TCHAR TargetProfileName[MAX_PATH];
            DWORD dwSize = MAX_PATH;

            if (GetStandardColorSpaceProfile(NULL,LCS_WINDOWS_COLOR_SPACE,TargetProfileName,&dwSize)) {
                m_csTargetProfile = (LPCTSTR) TargetProfileName;
                m_csTargetProfile = (LPCTSTR) m_csTargetProfile.NameAndExtension();
            } else {
                m_csTargetProfile = (LPCTSTR) NULL;
            }
        }
    }
}

//  Update the controls

void    CColorMatchDialog::UpdateControls(BOOL bChanged) {

    //  Switch Proofing Controls based on setting

    ShowWindow(m_hwndIntentText1, m_bEnableProofing && m_bEnableICM ? SW_SHOWNORMAL : SW_HIDE);

    EnableWindow(m_hwndTargetProfileLabel, m_bEnableProofing && m_bEnableICM);
    EnableWindow(m_hwndTargetList, m_bEnableProofing && m_bEnableICM);

    EnableWindow(m_hwndTargetIntentLabel, m_bEnableProofing && m_bEnableICM && !m_bDisableIntent);
    EnableWindow(m_hwndTargetIntent, m_bEnableProofing && m_bEnableICM && !m_bDisableIntent);

    //  Switch the other Controls, as well...

    EnableWindow(m_hwndMonitorProfileLabel, m_bEnableICM);
    EnableWindow(m_hwndMonitorList, m_bEnableICM);

    EnableWindow(m_hwndPrinterProfileLabel, m_bEnableICM && !m_csPrinter.IsEmpty());
    EnableWindow(m_hwndPrinterList, m_bEnableICM && m_bColorPrinter && !m_csPrinter.IsEmpty());

    if (m_bEnableProofing) {
        EnableWindow(m_hwndIntentLabel, m_bEnableICM && !m_bDisableIntent && !m_bDisableRenderIntent);
        EnableWindow(m_hwndRenderIntent, m_bEnableICM && !m_bDisableIntent && !m_bDisableRenderIntent);
    } else {
        EnableWindow(m_hwndIntentLabel, m_bEnableICM && !m_bDisableIntent);
        EnableWindow(m_hwndRenderIntent, m_bEnableICM && !m_bDisableIntent);
    }

    EnableWindow(GetDlgItem(m_hwnd, EnableBasic), m_bEnableICM);
    EnableWindow(GetDlgItem(m_hwnd, EnableProofing), m_bEnableICM);

    EnableApplyButton(bChanged);
}

//  Update the Apply buttom

void    CColorMatchDialog::EnableApplyButton(BOOL bEnable) {

    EnableWindow(GetDlgItem(m_hwnd, ApplyButton), bEnable);

}

//  Flags for buffer overflow (combined)

#define BAD_BUFFER_FLAGS    (CMS_MONITOROVERFLOW | CMS_PRINTEROVERFLOW | \
                             CMS_TARGETOVERFLOW)

//  By moving the ANSI / Unicode issues into the CString class
//  it becomes feasible to code these two versions so they look
//  encoding-independent.  In other words, the code for both
//  of these versions is written identically, and the compiler
//  does all the work, just like it ought to...

void CColorMatchDialog::FillStructure(COLORMATCHSETUPA *pcms) {

    if  (m_bEnableICM) {

        pcms -> dwFlags = CMS_SETRENDERINTENT | CMS_SETPRINTERPROFILE |
                          CMS_SETMONITORPROFILE;

        pcms -> dwRenderIntent = m_dwRenderIntent;

        //  03-20-1997  Bob_Kjelgaard@Prodigy.Net   RAID 21091 (Memphis)
        //  Don't fail if there is no monitor or printer profile.  Set
        //  them to an empty string and succeed, instead.
        //  We can always do this, because 0 counts and empty pointers
        //  have already been screened out.

        if  (m_csMonitorProfile.IsEmpty())
            pcms -> pMonitorProfile[0] = '\0';
        else {
            lstrcpynA(pcms -> pMonitorProfile, m_csMonitorProfile,
                pcms -> ccMonitorProfile);
            if  (lstrcmpA(pcms -> pMonitorProfile, m_csMonitorProfile))
                pcms -> dwFlags |= CMS_MONITOROVERFLOW;
        }

        if  (m_csPrinterProfile.IsEmpty() || !m_bColorPrinter)
            pcms -> pPrinterProfile[0] = '\0';
        else {
            lstrcpynA(pcms -> pPrinterProfile, m_csPrinterProfile,
                pcms -> ccPrinterProfile);

            if  (lstrcmpA(pcms -> pPrinterProfile, m_csPrinterProfile))
                pcms -> dwFlags |= CMS_PRINTEROVERFLOW;
        }

        if  (m_bEnableProofing) {
            pcms -> dwFlags |=
                CMS_ENABLEPROOFING | CMS_SETTARGETPROFILE;
            pcms -> dwProofingIntent = m_dwProofIntent;
            lstrcpynA(pcms -> pTargetProfile, m_csTargetProfile,
                pcms -> ccTargetProfile);
            if  (lstrcmpA(pcms -> pTargetProfile, m_csTargetProfile))
                pcms -> dwFlags |= CMS_TARGETOVERFLOW;
        } else {
            pcms -> pTargetProfile[0] = '\0';
        }

        if  (pcms -> dwFlags & BAD_BUFFER_FLAGS)
            Fail(ERROR_INSUFFICIENT_BUFFER);
    }
    else
    {
        pcms -> dwFlags = CMS_DISABLEICM;   //  No other flags are valid!
        pcms -> pMonitorProfile[0] = '\0';  //  No color profiles are choosed
        pcms -> pPrinterProfile[0] = '\0';
        pcms -> pTargetProfile[0]  = '\0';
    }
}

void CColorMatchDialog::FillStructure(COLORMATCHSETUPW *pcms) {

    if  (m_bEnableICM) {

        pcms -> dwFlags = CMS_SETRENDERINTENT | CMS_SETPRINTERPROFILE |
                          CMS_SETMONITORPROFILE;

        pcms -> dwRenderIntent = m_dwRenderIntent;

        //  03-20-1997  Bob_Kjelgaard@Prodigy.Net   RAID 21091 (Memphis)
        //  Don't fail if there is no monitor or printer profile.  Set
        //  them to an empty string and succeed, instead.
        //  We can always do this, because 0 counts and empty pointers
        //  have already been screened out.

        if  (m_csMonitorProfile.IsEmpty())
            pcms -> pMonitorProfile[0] = '\0';
        else {
            lstrcpynW(pcms -> pMonitorProfile, m_csMonitorProfile,
                pcms -> ccMonitorProfile);
            if  (lstrcmpW(pcms -> pMonitorProfile, m_csMonitorProfile))
                pcms -> dwFlags |= CMS_MONITOROVERFLOW;
        }

        if  (m_csPrinterProfile.IsEmpty() || !m_bColorPrinter)
            pcms -> pPrinterProfile[0] = '\0';
        else {
            lstrcpynW(pcms -> pPrinterProfile, m_csPrinterProfile,
                pcms -> ccPrinterProfile);

            if  (lstrcmpW(pcms -> pPrinterProfile, m_csPrinterProfile))
                pcms -> dwFlags |= CMS_PRINTEROVERFLOW;
        }

        if  (m_bEnableProofing) {
            pcms -> dwFlags |=
                CMS_ENABLEPROOFING | CMS_SETTARGETPROFILE | CMS_SETPROOFINTENT;
            pcms -> dwProofingIntent = m_dwProofIntent;
            lstrcpynW(pcms -> pTargetProfile, m_csTargetProfile,
                pcms -> ccTargetProfile);
            if  (lstrcmpW(pcms -> pTargetProfile, m_csTargetProfile))
                pcms -> dwFlags |= CMS_TARGETOVERFLOW;
        } else {
            pcms -> pTargetProfile[0] = '\0';
        }

        if  (pcms -> dwFlags & BAD_BUFFER_FLAGS)
            Fail(ERROR_INSUFFICIENT_BUFFER);
    }
    else
    {
        pcms -> dwFlags = CMS_DISABLEICM;   //  No other flags are valid!
        pcms -> pMonitorProfile[0] = '\0';  //  No color profiles are choosed
        pcms -> pPrinterProfile[0] = '\0';
        pcms -> pTargetProfile[0]  = '\0';
    }
}

CColorMatchDialog::CColorMatchDialog(COLORMATCHSETUPA *pcms) :
    CDialog(CGlobals::Instance(), ApplicationUI, pcms -> hwndOwner) {

    if  (!GoodParms((PCOLORMATCHSETUP) pcms))
        return;

    //  Make sure we've initialized these, if we have to.

    if  (pcms -> dwFlags & CMS_SETMONITORPROFILE)
        m_csMonitorProfile = pcms -> pMonitorProfile;

    if  (pcms -> dwFlags & CMS_SETPRINTERPROFILE)
        m_csPrinterProfile = pcms -> pPrinterProfile;

    if  (pcms -> dwFlags & CMS_SETTARGETPROFILE)
        m_csTargetProfile = pcms -> pTargetProfile;

    m_csSource  = pcms -> pSourceName;
    m_csMonitor = pcms -> pDisplayName;
    m_csPrinter = pcms -> pPrinterName;

    //  Ansi version call

    m_bAnsiCall = TRUE;

    CompleteInitialization();

    //  Display the UI, and watch what happens...

    switch  (DoModal()) {

        case    IDOK:
            if  (!m_bSuccess)
                return;

            // Fill up return buffer.

            FillStructure(pcms);
            return;

        case    IDCANCEL:
            Fail(ERROR_SUCCESS);
            return;

        default:
            Fail(GetLastError());
    }
}

CColorMatchDialog::CColorMatchDialog(COLORMATCHSETUPW *pcms) :
    CDialog(CGlobals::Instance(), ApplicationUI, pcms -> hwndOwner) {

    if  (!GoodParms((PCOLORMATCHSETUP) pcms))
        return;

    //  Make sure we've initialized these, if we have to.

    if  (pcms -> dwFlags & CMS_SETMONITORPROFILE) {
        m_csMonitorProfile = pcms -> pMonitorProfile;
        m_csMonitorProfile = m_csMonitorProfile.NameAndExtension();
    }

    if  (pcms -> dwFlags & CMS_SETPRINTERPROFILE) {
        m_csPrinterProfile = pcms -> pPrinterProfile;
        m_csPrinterProfile = m_csPrinterProfile.NameAndExtension();
    }

    if  (pcms -> dwFlags & CMS_SETTARGETPROFILE) {
        m_csTargetProfile = pcms -> pTargetProfile;
        m_csTargetProfile = m_csTargetProfile.NameAndExtension();
    }

    m_csSource  = pcms -> pSourceName;
    m_csMonitor = pcms -> pDisplayName;
    m_csPrinter = pcms -> pPrinterName;

    //  Unicode version call

    m_bAnsiCall = FALSE;

    CompleteInitialization();

    //  Display the UI, and watch what happens...

    switch  (DoModal()) {

        case    IDOK:
            if  (!m_bSuccess)
                return;

            // Fill up return buffer.

            FillStructure(pcms);
            return;

        case    IDCANCEL:
            Fail(ERROR_SUCCESS);
            return;

        default:
            Fail(GetLastError());
    }
}

//  Dialog initialization function

BOOL    CColorMatchDialog::OnInit() {

    //  Collect the common handles

    m_hwndRenderIntent = GetDlgItem(m_hwnd, RenderingIntent);
    m_hwndTargetIntent = GetDlgItem(m_hwnd, TargetIntent);
    m_hwndPrinterList = GetDlgItem(m_hwnd, PrinterProfile);
    m_hwndMonitorList = GetDlgItem(m_hwnd, MonitorProfile);
    m_hwndTargetList = GetDlgItem(m_hwnd, TargetProfile);

    m_hwndMonitorProfileLabel = GetDlgItem(m_hwnd, MonitorProfileLabel);
    m_hwndPrinterProfileLabel = GetDlgItem(m_hwnd, PrinterProfileLabel);
    m_hwndIntentLabel = GetDlgItem(m_hwnd, RenderingIntentLabel);
    m_hwndTargetProfileLabel = GetDlgItem(m_hwnd, TargetProfileLabel);
    m_hwndTargetIntentLabel = GetDlgItem(m_hwnd, TargetIntentLabel);

    m_hwndIntentText1 = GetDlgItem(m_hwnd, RenderingIntentText1);

    //  Fill in the source name
    SetDlgItemText(m_hwnd, SourceProfile, m_csSource);

    //  Set the Check Boxes

    CheckDlgButton(m_hwnd, EnableICM,
        m_bEnableICM ? BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(m_hwnd, EnableBasic,
        m_bEnableProofing ? BST_UNCHECKED : BST_CHECKED);
    CheckDlgButton(m_hwnd, EnableProofing,
        m_bEnableProofing ? BST_CHECKED : BST_UNCHECKED);

    //  Fill in the list(s) of Rendering Intents

    CString csWork; //  There's plenty of it to do...

    for (int i = INTENT_PERCEPTUAL; i <= INTENT_ABSOLUTE_COLORIMETRIC; i++) {
        csWork.Load(i + PerceptualString);
        SendMessage(m_hwndRenderIntent, CB_ADDSTRING, 0,
            (LPARAM) (LPCTSTR) csWork);
        SendMessage(m_hwndTargetIntent, CB_ADDSTRING, 0,
            (LPARAM) (LPCTSTR) csWork);
    }

    //  Init the rendering intents

    SendMessage(m_hwndRenderIntent, CB_SETCURSEL, m_dwRenderIntent, 0);
    SendMessage(m_hwndTargetIntent, CB_SETCURSEL, m_dwProofIntent, 0);

    //  Init the profile lists

    //  03-20-1997    Bob_Kjelgaard@Prodigy.Net   RAID Memphis:22213
    //  The algorithm used to determine which profile to select was incorrect.
    //  There's a much simpler and direct way, anyway.

    LRESULT id;

    //  Target Profiles

    for (unsigned u = 0; u < m_csaTarget.Count(); u++) {
        if (m_bUseProfileDescription) {
            id = SendMessage(m_hwndTargetList, CB_ADDSTRING,
                             0, (LPARAM)((LPTSTR) m_csaTargetDesc[u]));
        } else {
            id = SendMessage(m_hwndTargetList, CB_ADDSTRING,
                             0, m_csaTarget[u].NameOnly());
        }
        SendMessage(m_hwndTargetList, CB_SETITEMDATA, id, u);

        if (m_csaTarget[u].IsEqualString(m_csTargetProfile)) {
            SendMessage(m_hwndTargetList, CB_SETCURSEL, id, 0);
        }
    }

    //  Set Target profile if specified, otherwise the default

    if  (!m_csaTarget.Count()) {
        CString csWork;
        csWork.Load(NoProfileString);

        SendMessage(m_hwndTargetList, CB_ADDSTRING, 0, csWork);
        SendMessage(m_hwndTargetList, CB_SETITEMDATA, 0, -1);
        SendMessage(m_hwndTargetList, CB_SETCURSEL, 0, 0);
    }

    //  Monitor Profiles

    //  03-20-1997  Bob_Kjelgaard@Prodigy.Net   Memphis RAID #22289

    csWork.Load(GetDlgItem(m_hwnd, MonitorProfileLabel));
    csWork = csWork + m_csMonitorDisplayName + _TEXT(")");
    SetDlgItemText(m_hwnd, MonitorProfileLabel, csWork);

    for (u = 0; u < m_csaMonitor.Count(); u++) {
        if (m_bUseProfileDescription) {
            id = SendMessage(m_hwndMonitorList, CB_ADDSTRING,
                             0, (LPARAM)((LPTSTR) m_csaMonitorDesc[u]));
        } else {
            id = SendMessage(m_hwndMonitorList, CB_ADDSTRING,
                             0, m_csaMonitor[u].NameOnly());
        }
        SendMessage(m_hwndMonitorList, CB_SETITEMDATA, id, u);

        if (m_csaMonitor[u].IsEqualString(m_csMonitorProfile)) {
            SendMessage(m_hwndMonitorList, CB_SETCURSEL, id, 0);
        }
    }

    //  Set Monitor profile if specified

    if  (!m_csaMonitor.Count()) {
        CString csWork;
        csWork.Load(NoProfileString);

        SendMessage(m_hwndMonitorList, CB_ADDSTRING, 0, csWork);
        SendMessage(m_hwndMonitorList, CB_SETITEMDATA, 0, -1);
        SendMessage(m_hwndMonitorList, CB_SETCURSEL, 0, 0);
    }

    //  Printer Profiles

    //  03-20-1997  Bob_Kjelgaard@Prodigy.Net   RAID Memphis:22290
    //  If there's no printer, then we should disable all of the related
    //  controls.

    if  (m_csPrinter.IsEmpty()) {
        csWork.Load(NoPrintersInstalled);
    } else {
        csWork.Load(GetDlgItem(m_hwnd, PrinterProfileLabel));
        csWork = csWork + m_csPrinter + _TEXT(")");
    }

    SetDlgItemText(m_hwnd, PrinterProfileLabel, csWork);

    for (u = 0; u < m_csaPrinter.Count(); u++) {
        if (m_bUseProfileDescription) {
            id = SendMessage(m_hwndPrinterList, CB_ADDSTRING,
                             0, (LPARAM)((LPTSTR) m_csaPrinterDesc[u]));
        } else {
            id = SendMessage(m_hwndPrinterList, CB_ADDSTRING,
                             0, m_csaPrinter[u].NameOnly());
        }
        SendMessage(m_hwndPrinterList, CB_SETITEMDATA, id, u);

        if (m_csaPrinter[u].IsEqualString(m_csPrinterProfile)) {
            SendMessage(m_hwndPrinterList, CB_SETCURSEL, id, 0);
        }
    }

    //  Set Printer profile if specified

    if  (!m_csaPrinter.Count()) {
        CString csWork;

        if (!m_csPrinter.IsEmpty() && !m_bColorPrinter) {
            // Printer are specified, but it is not color printer.
            csWork.Load(NotColorPrinter);
        } else {
            csWork.Load(NoProfileString);
        }

        SendMessage(m_hwndPrinterList, CB_ADDSTRING, 0, csWork);
        SendMessage(m_hwndPrinterList, CB_SETITEMDATA, 0, -1);
        SendMessage(m_hwndPrinterList, CB_SETCURSEL, 0, 0);
    }

    //  End RAID Memphis:22213, 22289, 22290 03-20-1997

    //  If Apply callback does not provided, disable apply button.

    if  (m_dpApplyCallback == NULL) {
        RECT  rcApply, rcCancel;
        POINT ptApply, ptCancel;

        // Get current "Apply" and "Cancel" buttom position

        GetWindowRect(GetDlgItem(m_hwnd, ApplyButton), &rcApply);
        GetWindowRect(GetDlgItem(m_hwnd, IDCANCEL), &rcCancel);

        // Convert the buttom coordinate to parent dialog coord from screen coord.

        ptApply.x = rcApply.left;   ptApply.y = rcApply.top;
        ptCancel.x = rcCancel.left; ptCancel.y = rcCancel.top;

        ScreenToClient(m_hwnd,&ptApply);
        ScreenToClient(m_hwnd,&ptCancel);

        // Move "Apply" button away... and shift "Cancel" and "OK"

        MoveWindow(GetDlgItem(m_hwnd, ApplyButton),0,0,0,0,TRUE);
        MoveWindow(GetDlgItem(m_hwnd, IDCANCEL),
                   ptApply.x,ptApply.y,
                   rcApply.right - rcApply.left,
                   rcApply.bottom - rcApply.top,TRUE);
        MoveWindow(GetDlgItem(m_hwnd, IDOK),
                   ptCancel.x,ptCancel.y,
                   rcCancel.right - rcCancel.left,
                   rcCancel.bottom - rcCancel.top,TRUE);
    }

    //  Enable/Disable controls based upon settings

    UpdateControls(FALSE);

    return  FALSE;  //  Because we've probably moved it...
}

//  Command Processing override

BOOL CColorMatchDialog::OnCommand(WORD wNotifyCode, WORD wid, HWND hwndCtl) {

    switch  (wNotifyCode) {

        case    BN_CLICKED:

            switch  (wid) {

                case    EnableICM:
                    m_bEnableICM = !m_bEnableICM;
                    UpdateControls(TRUE);
                    return  TRUE;

                case    EnableBasic:

                    if (m_bEnableProofing)
                    {
                        m_bEnableProofing = FALSE;

                        // Copy proof intent to rendering intent
                        //
                        m_dwRenderIntent = m_dwProofIntent;

                        // Update UI
                        //
                        SendMessage(m_hwndTargetIntent, CB_SETCURSEL,
                            m_dwProofIntent, 0);
                        SendMessage(m_hwndRenderIntent, CB_SETCURSEL,
                            m_dwRenderIntent, 0);
                        UpdateControls(TRUE);
                    }
                    return TRUE;

                case    EnableProofing:

                    if (m_bEnableProofing == FALSE)
                    {
                        m_bEnableProofing = TRUE;

                        //  Copy the original rendering intent to the proofing
                        //  intent, and set original to Absolute Colorimetric

                        m_dwProofIntent = m_dwRenderIntent;
                        m_dwRenderIntent = INTENT_ABSOLUTE_COLORIMETRIC;

                        // Update UI
                        //
                        SendMessage(m_hwndTargetIntent, CB_SETCURSEL,
                            m_dwProofIntent, 0);
                        SendMessage(m_hwndRenderIntent, CB_SETCURSEL,
                            m_dwRenderIntent, 0);
                        UpdateControls(TRUE);
                    }
                    return  TRUE;

                case ApplyButton: {

                    // Disable apply button

                    EnableApplyButton(FALSE);

                    // Callback supplied function

                    if (m_dpApplyCallback) {

                        if (m_bAnsiCall) {
                            PCOLORMATCHSETUPA pcms = (PCOLORMATCHSETUPA) m_pvCMS;

                            FillStructure(pcms);
                            (*(PCMSCALLBACKA)m_dpApplyCallback)(pcms,m_lpApplyCallback);
                        } else {
                            PCOLORMATCHSETUPW pcms = (PCOLORMATCHSETUPW) m_pvCMS;

                            FillStructure(pcms);
                            (*(PCMSCALLBACKW)m_dpApplyCallback)(pcms,m_lpApplyCallback);
                        }
                    }

                    return TRUE;
                }
            }

            break;

        case    CBN_SELCHANGE: {

            DWORD idItem = (DWORD)SendMessage(hwndCtl, CB_GETCURSEL, 0, 0);
            unsigned uItem = (unsigned)SendMessage(hwndCtl, CB_GETITEMDATA, idItem, 0);

            switch  (wid) {

                case    RenderingIntent:

                    if (m_dwRenderIntent != idItem)
                    {
                        m_dwRenderIntent = idItem;

                        // If proofing is disabled, proof intent follows
                        // render intent

                        if (! m_bEnableProofing)
                        {
                            m_dwProofIntent = idItem;
                            SendMessage(m_hwndTargetIntent, CB_SETCURSEL,
                                        m_dwProofIntent, 0);
                        }

                        EnableApplyButton();
                    }

                    return  TRUE;

                case    TargetIntent:

                    if (m_dwProofIntent != idItem)
                    {
                        m_dwProofIntent = idItem;

                        EnableApplyButton();
                    }

                    return  TRUE;

                case    TargetProfile:

                    //  If there are no installed profiles, don't bother

                    if  (!m_csaTarget.Count())
                        return  TRUE;

                    if (m_csTargetProfile.IsEqualString(m_csaTarget[uItem]) == FALSE)
                    {
                        m_csTargetProfile = m_csaTarget[uItem];

                        EnableApplyButton();
                    }

                    return  TRUE;

                case    MonitorProfile:

                    //  If there are no installed profiles, don't bother

                    if  (!m_csaMonitor.Count())
                        return  TRUE;

                    if (m_csMonitorProfile.IsEqualString(m_csaMonitor[uItem]) == FALSE)
                    {
                        m_csMonitorProfile = m_csaMonitor[uItem];

                        EnableApplyButton();
                    }

                    return  TRUE;

                case    PrinterProfile:

                    //  If there are no installed profiles, don't bother

                    if  (!m_csaPrinter.Count())
                        return  TRUE;

                    if (m_csPrinterProfile.IsEqualString(m_csaPrinter[uItem]) == FALSE)
                    {
                        m_csPrinterProfile = m_csaPrinter[uItem];

                        EnableApplyButton();
                    }

                    return  TRUE;
            }

        }

    }

    //  Pass anything we didn't handle above to the base class

    return  CDialog::OnCommand(wNotifyCode, wid, hwndCtl);

}

//  Context-sensitive help handler

BOOL CColorMatchDialog::OnHelp(LPHELPINFO pHelp) {

    if (pHelp->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) pHelp->hItemHandle, WINDOWS_HELP_FILE,
                HELP_WM_HELP, (ULONG_PTR) (LPSTR) ApplicationUIHelpIds);
    }

    return (TRUE);
}

BOOL CColorMatchDialog::OnContextMenu(HWND hwnd) {

    WinHelp(hwnd, WINDOWS_HELP_FILE,
            HELP_CONTEXTMENU, (ULONG_PTR) (LPSTR) ApplicationUIHelpIds);

    return (TRUE);
}

//  This are the real honest-to-goodness API!

extern "C" BOOL WINAPI  SetupColorMatchingA(PCOLORMATCHSETUPA pcms) {

    CColorMatchDialog   ccmd(pcms);

    return  ccmd.Results();
}

extern "C" BOOL WINAPI  SetupColorMatchingW(PCOLORMATCHSETUPW pcms) {

    CColorMatchDialog   ccmd(pcms);

    return  ccmd.Results();
}
