//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       firstpin.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "firstpin.h"
#include "folder.h"
#include "idlhelp.h"
#include "msgbox.h"
#include "cscst.h"

//
// Base class for all "first pin" wizard pages.
// Contains the single dialog proc used for all pages.  Specialization
// for individual pages is achieved through deriviation and implementing
// virtual functions.
//
class CWizardPage
{
    public:
        enum { WM_WIZARDFINISHED = (WM_USER + 1) };

        CWizardPage(HINSTANCE hInstance,
                    UINT idDlgTemplate,
                    UINT idsHdrTitle,
                    UINT idsHdrSubtitle,
                    DWORD dwPgFlags,
                    DWORD dwBtnFlags);

        virtual ~CWizardPage(void);

        UINT GetDlgTemplate(void) const
            { return m_idDlgTemplate; }
        UINT GetHeaderTitle(void) const
            { return m_idsHdrTitle; }
        UINT GetHeaderSubtitle(void) const
            { return m_idsHdrSubtitle; }
        DWORD GetPageFlags(void) const
            { return m_dwPgFlags; }
        DWORD GetBtnFlags(void) const
            { return m_dwBtnFlags; }
        DLGPROC GetDlgProc(void) const
            { return DlgProc; }

        virtual BOOL OnInitDialog(WPARAM wParam, LPARAM lParam)
            { return TRUE; }

        virtual BOOL OnPSNSetActive(void);

        virtual BOOL OnPSNWizFinish(void)
            { SetWindowLong(m_hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR); return FALSE; }

        virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam)
            { return FALSE; }

        virtual BOOL OnWizardFinished(void) { return FALSE; }

    protected:
        HINSTANCE m_hInstance;
        HWND      m_hwndDlg;
        HFONT     m_hTitleFont;       // Used only by cover and finish pages.
        UINT      m_cyTitleFontHt;    // Title font height in pts.

        int FontPtsToHt(HWND hwnd, int pts);
        BOOL FormatTitleFont(UINT idcTitle);

    private:
        UINT   m_idDlgTemplate;    // Dialog resource template.
        UINT   m_idsHdrTitle;      // String ID for pg header title.
        UINT   m_idsHdrSubtitle;   // String ID for pg header subtitle.
        DWORD  m_dwBtnFlags;       // PSB_WIZXXXXX flags.
        DWORD  m_dwPgFlags;        // PSP_XXXX flags.
        
        static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


//
// Welcome page.
//
class CWizardPgWelcome : public CWizardPage
{
    public:
        CWizardPgWelcome(HINSTANCE hInstance,
                         UINT idDlgTemplate, 
                         UINT idsHdrTitle, 
                         UINT idsHdrSubtitle,
                         DWORD dwPgFlags, 
                         DWORD dwBtnFlags
                         ) : CWizardPage(hInstance,
                                         idDlgTemplate,
                                         idsHdrTitle,
                                         idsHdrSubtitle,
                                         dwPgFlags,
                                         dwBtnFlags)
                                         { }


        BOOL OnInitDialog(WPARAM wParam, LPARAM lParam);
};

//
// Pinning page.
//
class CWizardPgPin : public CWizardPage
{
    public:
        CWizardPgPin(HINSTANCE hInstance,
                     UINT idDlgTemplate, 
                     UINT idsHdrTitle, 
                     UINT idsHdrSubtitle,
                     DWORD dwPgFlags, 
                     DWORD dwBtnFlags
                     ) : CWizardPage(hInstance, 
                                     idDlgTemplate,
                                     idsHdrTitle,
                                     idsHdrSubtitle,
                                     dwPgFlags,
                                     dwBtnFlags) { }

        BOOL OnInitDialog(WPARAM wParam, LPARAM lParam);
        BOOL OnWizardFinished(void);
};

//
// Offline page.
//
class CWizardPgOffline : public CWizardPage
{
    public:
        CWizardPgOffline(HINSTANCE hInstance,
                         UINT idDlgTemplate, 
                         UINT idsHdrTitle, 
                         UINT idsHdrSubtitle,
                         DWORD dwPgFlags, 
                         DWORD dwBtnFlags
                         ) : CWizardPage(hInstance,
                                         idDlgTemplate,
                                         idsHdrTitle,
                                         idsHdrSubtitle,
                                         dwPgFlags,
                                         dwBtnFlags) { }

        BOOL OnInitDialog(WPARAM wParam, LPARAM lParam);
        BOOL OnPSNWizFinish(void);
        BOOL OnWizardFinished(void);
};

//
// Class encapsulating the functionality of the entire wizard.
// It contains member instances of each of the page types.
//
class CFirstPinWizard
{
    public:
        CFirstPinWizard(HINSTANCE hInstance, HWND hwndParent);

        HRESULT Run(void);

    private:
        enum { PG_WELCOME, 
               PG_PIN, 
               PG_OFFLINE,
               PG_NUMPAGES };

        HINSTANCE         m_hInstance;
        HWND              m_hwndParent;
        CWizardPgWelcome  m_PgWelcome;
        CWizardPgPin      m_PgPin;
        CWizardPgOffline  m_PgOffline;
        CWizardPage      *m_rgpWizPages[PG_NUMPAGES];
};


//
// CWizardPage members --------------------------------------------------------
//
CWizardPage::CWizardPage(
    HINSTANCE hInstance,
    UINT idDlgTemplate,
    UINT idsHdrTitle,
    UINT idsHdrSubtitle,
    DWORD dwPgFlags,
    DWORD dwBtnFlags
    ) : m_hInstance(hInstance),
        m_idDlgTemplate(idDlgTemplate),
        m_idsHdrTitle(idsHdrTitle),
        m_idsHdrSubtitle(idsHdrSubtitle),
        m_dwPgFlags(dwPgFlags),
        m_dwBtnFlags(dwBtnFlags),
        m_cyTitleFontHt(12),
        m_hwndDlg(NULL),
        m_hTitleFont(NULL)
{
    //
    // Get the title font height from a resource string.  That way localizers can
    // play with the font dimensions if necessary.
    //
    TCHAR szFontHt[20];
    if (0 < LoadString(m_hInstance, IDS_FIRSTPIN_FONTHT_PTS, szFontHt, ARRAYSIZE(szFontHt)))
    {
        m_cyTitleFontHt = StrToInt(szFontHt);
    }
}


CWizardPage::~CWizardPage(
    void
    )
{
    if (NULL != m_hTitleFont)
    {
        DeleteObject(m_hTitleFont);
    }
}


//
// PSN_SETACTIVE handler.
//
BOOL 
CWizardPage::OnPSNSetActive(
    void
    )
{
    PropSheet_SetWizButtons(GetParent(m_hwndDlg), m_dwBtnFlags);
    return FALSE;
}

//
// Dialog proc used by all pages in this wizard.
//
INT_PTR CALLBACK 
CWizardPage::DlgProc(
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    CWizardPage *pPage = (CWizardPage *)GetWindowLongPtr(hwnd, DWLP_USER);

    BOOL bResult = FALSE;
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            PROPSHEETPAGE *ppsp = (PROPSHEETPAGE *)lParam;
            pPage = (CWizardPage *)ppsp->lParam;

            TraceAssert(NULL != pPage);
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pPage);
            pPage->m_hwndDlg = hwnd;
            bResult = pPage->OnInitDialog(wParam, lParam);
            break;
        }

        case WM_COMMAND:
            if (NULL != pPage)
                bResult = pPage->OnCommand(wParam, lParam);
            break;

        case WM_NOTIFY:
            switch(((LPNMHDR)lParam)->code)
            {
                case PSN_SETACTIVE:
                    bResult = pPage->OnPSNSetActive();
                    break;

                case PSN_WIZFINISH:
                    bResult = pPage->OnPSNWizFinish();
                    break;
            }
            break;

        case PSM_QUERYSIBLINGS:
            if (CWizardPage::WM_WIZARDFINISHED == wParam)
                bResult = pPage->OnWizardFinished();
            break;

        default:
            break;
    }
    return bResult;
}



//
// Helper function to convert a font point size to a height value
// used in a LOGFONT structure.
//
int 
CWizardPage::FontPtsToHt(
    HWND hwnd, 
    int pts
    )
{
    int ht  = 10;
    HDC hdc = GetDC(hwnd);
    if (NULL != hdc)
    {
        ht = -MulDiv(pts, GetDeviceCaps(hdc, LOGPIXELSY), 72); 
        ReleaseDC(hwnd, hdc);
    }
    return ht;
}


//
// The title text on the cover and finish pages is enlarged and bold.
// This code modifies the text in the dialog accordingly.
// On return, m_hTitleFont contains the handle to the title font.
//
BOOL
CWizardPage::FormatTitleFont(
    UINT idcTitle
    )
{
    BOOL bResult   = FALSE;
    HWND hwndTitle = GetDlgItem(m_hwndDlg, idcTitle);
    HFONT hFont    = (HFONT)SendMessage(hwndTitle, WM_GETFONT, 0, 0);
    if (NULL != hFont)
    {
        if (NULL == m_hTitleFont)
        {
            LOGFONT lf;
            if (GetObject(hFont, sizeof(lf), &lf))
            {
                lf.lfHeight = FontPtsToHt(hwndTitle, m_cyTitleFontHt);
                m_hTitleFont = CreateFontIndirect(&lf);
            }
        }
        if (NULL != m_hTitleFont)
        {
            SendMessage(hwndTitle, WM_SETFONT, (WPARAM)m_hTitleFont, 0);
            bResult = TRUE;
        }
    }
    return bResult;
}


//
// CWizardPgWelcome members -----------------------------------------------------
//
//
// WM_INITDIALOG handler.
//
BOOL 
CWizardPgWelcome::OnInitDialog(
    WPARAM wParam, 
    LPARAM lParam
    )
{
    FormatTitleFont(IDC_TXT_FIRSTPIN_WELCOME_TITLE);
    return CWizardPage::OnInitDialog(wParam, lParam);
}


//
// CWizardPgPin members -------------------------------------------------------
//
//
// WM_INITDIALOG handler.
//
BOOL 
CWizardPgPin::OnInitDialog(
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HRESULT hr = IsRegisteredForSyncAtLogonAndLogoff();
    CheckDlgButton(m_hwndDlg, 
                   IDC_CBX_FIRSTPIN_AUTOSYNC, 
                   S_OK == hr ? BST_CHECKED : BST_UNCHECKED);

    return CWizardPage::OnInitDialog(wParam, lParam);
}

//
// PSN_WIZFINISH handler.
//
BOOL 
CWizardPgPin::OnWizardFinished(
    void
    )
{
    HRESULT hr;
    if (BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_FIRSTPIN_AUTOSYNC))
    {
        const DWORD dwFlags = SYNCMGRREGISTERFLAG_CONNECT | SYNCMGRREGISTERFLAG_PENDINGDISCONNECT;
    
        hr = RegisterForSyncAtLogonAndLogoff(dwFlags, dwFlags);
        if (SUCCEEDED(hr))
        {
            SetSyncMgrInitialized();
        }
        else
        {
            CscMessageBox(m_hwndDlg,
                          MB_OK | MB_ICONERROR,
                          Win32Error(HRESULT_CODE(hr)),
                          m_hInstance,
                          IDS_ERR_REGSYNCATLOGONLOGOFF);
        }
    }
    return CWizardPage::OnWizardFinished();
}



//
// CWizardPgOffline members ---------------------------------------------------
//
//
// WM_INITDIALOG handler.
//
BOOL 
CWizardPgOffline::OnInitDialog(
    WPARAM wParam, 
    LPARAM lParam
    )
{
    //
    // If policy allows configuration of the reminders, check the "enable reminders"
    // checkbox.
    //
    CConfig& config = CConfig::GetSingleton();
    bool bNoConfigReminders;
    bool bNoCacheViewer = config.NoCacheViewer();

    config.NoReminders(&bNoConfigReminders);

    CheckDlgButton(m_hwndDlg, IDC_CBX_REMINDERS, !bNoConfigReminders);
    EnableWindow(GetDlgItem(m_hwndDlg, IDC_CBX_REMINDERS), !bNoConfigReminders);

    CheckDlgButton(m_hwndDlg, IDC_CBX_FIRSTPIN_FLDRLNK, BST_UNCHECKED);
    EnableWindow(GetDlgItem(m_hwndDlg, IDC_CBX_FIRSTPIN_FLDRLNK), !bNoCacheViewer);

    return CWizardPage::OnInitDialog(wParam, lParam);
}


//
// PSN_WIZFINISH handler.
//
BOOL 
CWizardPgOffline::OnPSNWizFinish(
    void
    )
{
    //
    // Send PSM_QUERYSIBLINGS to all of the pages with
    // wParam set to WM_WIZARDFINISHED.  This will trigger
    // a call to the virtual function OnWizardFinished()
    // allowing each page to respond to the successful completion
    // of the wizard.
    //
    PropSheet_QuerySiblings(GetParent(m_hwndDlg),
                            CWizardPage::WM_WIZARDFINISHED,
                            0);
    //
    // Now handle for this page.
    //
    OnWizardFinished();
    return CWizardPage::OnPSNWizFinish();
}




//
// PSN_WIZFINISH handler.
//
BOOL 
CWizardPgOffline::OnWizardFinished(
    void
    )
{
    bool bEnableReminders = (BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_REMINDERS));
    DWORD dwValue;
    DWORD dwErr;
    
    dwValue = bEnableReminders ? 0 : 1;

    dwErr = SHSetValue(HKEY_CURRENT_USER, 
                       REGSTR_KEY_OFFLINEFILES,
                       REGSTR_VAL_NOREMINDERS, 
                       REG_DWORD,
                       &dwValue,
                       sizeof(dwValue));

    if (bEnableReminders)
    {
        PostToSystray(PWM_RESET_REMINDERTIMER, 0, 0);
    }

    if (BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, IDC_CBX_FIRSTPIN_FLDRLNK))
    {
        COfflineFilesFolder::CreateLinkOnDesktop(m_hwndDlg);
    }

    return CWizardPage::OnWizardFinished();
}


//
// CFirstPinWizard members ----------------------------------------------------
//
CFirstPinWizard::CFirstPinWizard(
    HINSTANCE hInstance,
    HWND hwndParent
    ) : m_hInstance(hInstance),
        m_hwndParent(hwndParent),
        m_PgWelcome(hInstance,
            IDD_FIRSTPIN_WELCOME, 
            0,
            0,
            PSP_DEFAULT,
            PSWIZB_NEXT),
        m_PgPin(hInstance,
            IDD_FIRSTPIN_PIN, 
            0,
            0,
            PSP_DEFAULT,
            PSWIZB_NEXT | PSWIZB_BACK),
        m_PgOffline(hInstance,
            IDD_FIRSTPIN_OFFLINE,
            0,
            0,
            PSP_DEFAULT,
            PSWIZB_FINISH | PSWIZB_BACK)
{
    //
    // Store pointers to each page in an array.  Makes creating the
    // prop sheet easier in Run().
    //
    m_rgpWizPages[0] = &m_PgWelcome;
    m_rgpWizPages[1] = &m_PgPin;
    m_rgpWizPages[2] = &m_PgOffline;
}


//
// Creates the wizard and runs it.
// The wizard runs modally.
//
// Returns:  
//
//   S_OK    = User completed wizard and pressed "Finish".
//   S_FALSE = User pressed "Cancel" in wizard.
//   Other   = Error creating wizard.
//
HRESULT
CFirstPinWizard::Run(
    void
    )
{
    HRESULT hr = NOERROR;

    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE rghpage[ARRAYSIZE(m_rgpWizPages)];

    ZeroMemory(&psh, sizeof(psh));
    ZeroMemory(rghpage, sizeof(rghpage));

    psh.dwSize         = sizeof(psh);
    psh.dwFlags        = PSH_WIZARD_LITE;
    psh.hwndParent     = m_hwndParent;
    psh.hInstance      = m_hInstance;
    psh.nPages         = ARRAYSIZE(rghpage);
    psh.phpage         = rghpage;

    TCHAR szTitle[MAX_PATH];
    TCHAR szSubTitle[MAX_PATH];

    for (int i = 0; i < ARRAYSIZE(rghpage) && SUCCEEDED(hr); i++)
    {
        CWizardPage *pwp = m_rgpWizPages[i];
        ZeroMemory(&psp, sizeof(psp));

        psp.dwSize        = sizeof(psp);
        psp.dwFlags       |= pwp->GetPageFlags();
        psp.hInstance     = m_hInstance;
        psp.pszTemplate   = MAKEINTRESOURCE(pwp->GetDlgTemplate());
        psp.pfnDlgProc    = pwp->GetDlgProc();
        psp.lParam        = (LPARAM)pwp;

        rghpage[i] = CreatePropertySheetPage(&psp);
        if (NULL == rghpage[i])
        {
            while(0 <= --i)
            {
                DestroyPropertySheetPage(rghpage[i]);
            }
            hr = E_FAIL;
        }
    }
    if (SUCCEEDED(hr))
    {
        switch(PropertySheet(&psh))
        {
            case -1:
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;

            case 0:
                hr = S_FALSE; // User pressed "Cancel".
                break;

            case 1:
                hr = S_OK;    // User pressed "Finish".
                break;
        }
    }
            
    return hr;
}


//
// This is the function you call when you want to run the wizard.
// It merely creates a wizard object and tells it to run.
// If the user finishes the wizard, it records this fact in the
// registry.  Calling FirstPinWizardCompleted() will tell
// you later if the user has finished the wizard.
//
// Returns:
//
//  S_OK    = User completed wizard and pressed "Finish".
//  S_FALSE = User cancelled out of wizard.
//  Other   = Error creating wizard.
//
HRESULT
ShowFirstPinWizard(
    HWND hwndParent
    )
{
    HRESULT hr = NOERROR;
    CFirstPinWizard Wizard(g_hInstance, hwndParent);
    hr = Wizard.Run();
    if (S_OK == hr)
    {
        //
        // Only record "finished" in registry if user
        // pressed "finish".
        //
        RegKey key(HKEY_CURRENT_USER, REGSTR_KEY_OFFLINEFILES);
        if (SUCCEEDED(key.Open(KEY_SET_VALUE, true)))
        {
            key.SetValue(REGSTR_VAL_FIRSTPINWIZARDSHOWN, 1);
        }
    }
    return hr;
}

//
// Has user seen the wizard and pressed "finish"?
//
bool
FirstPinWizardCompleted(
    void
    )
{
    return CConfig::GetSingleton().FirstPinWizardShown();
}
