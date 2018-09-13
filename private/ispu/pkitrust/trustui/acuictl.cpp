//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       acuictl.cpp
//
//  Contents:   Authenticode Default UI controls
//
//  History:    12-May-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <stdpch.h>

#include <richedit.h>

#include "secauth.h"

IACUIControl::IACUIControl(CInvokeInfoHelper& riih) : m_riih( riih ),
                                    m_hrInvokeResult( TRUST_E_SUBJECT_NOT_TRUSTED )
{
    m_hrInvokeResult    = TRUST_E_SUBJECT_NOT_TRUSTED;


    m_pszCopyActionText             = NULL;
    m_pszCopyActionTextNoTS         = NULL;
    m_pszCopyActionTextNotSigned    = NULL;

    if ((riih.ProviderData()) &&
        (riih.ProviderData()->psPfns) &&
        (riih.ProviderData()->psPfns->psUIpfns) &&
        (riih.ProviderData()->psPfns->psUIpfns->psUIData))
    {
        if (_ISINSTRUCT(CRYPT_PROVUI_DATA,
                         riih.ProviderData()->psPfns->psUIpfns->psUIData->cbStruct,
                         pCopyActionTextNotSigned))
        {
            this->LoadActionText(&m_pszCopyActionText,
                           riih.ProviderData()->psPfns->psUIpfns->psUIData->pCopyActionText, IDS_ACTIONSIGNED);
            this->LoadActionText(&m_pszCopyActionTextNoTS,
                           riih.ProviderData()->psPfns->psUIpfns->psUIData->pCopyActionTextNoTS, IDS_ACTIONSIGNED_NODATE);
            this->LoadActionText(&m_pszCopyActionTextNotSigned,
                           riih.ProviderData()->psPfns->psUIpfns->psUIData->pCopyActionTextNotSigned, IDS_ACTIONNOTSIGNED);
        }
    }

    if (!(m_pszCopyActionText))
    {
        this->LoadActionText(&m_pszCopyActionText, NULL, IDS_ACTIONSIGNED);
    }

    if (!(m_pszCopyActionTextNoTS))
    {
        this->LoadActionText(&m_pszCopyActionTextNoTS, NULL, IDS_ACTIONSIGNED_NODATE);
    }

    if (!(m_pszCopyActionTextNotSigned))
    {
        this->LoadActionText(&m_pszCopyActionTextNotSigned, NULL, IDS_ACTIONNOTSIGNED);
    }
}

void IACUIControl::LoadActionText(WCHAR **ppszRet, WCHAR *pwszIn, DWORD dwDefId)
{
    WCHAR    sz[MAX_PATH];

    *ppszRet    = NULL;
    sz[0]       = NULL;

    if ((pwszIn) && (*pwszIn))
    {
        sz[0] = NULL;
        if (wcslen(pwszIn) < MAX_PATH)
        {
            wcscpy(&sz[0], pwszIn);
        }

        if (sz[0])
        {
            if (*ppszRet = new WCHAR[wcslen(&sz[0]) + 1])
            {
                wcscpy(*ppszRet, &sz[0]);
            }
        }

    }

    if (!(sz[0]))
    {
        sz[0] = NULL;
        LoadStringU(g_hModule, dwDefId, &sz[0], MAX_PATH);

        if (sz[0])
        {
            if (*ppszRet = new WCHAR[wcslen(&sz[0]) + 1])
            {
                wcscpy(*ppszRet, &sz[0]);
            }
        }
    }
}

IACUIControl::~IACUIControl ()
{
    DELETE_OBJECT(m_pszCopyActionText);
    DELETE_OBJECT(m_pszCopyActionTextNoTS);
    DELETE_OBJECT(m_pszCopyActionTextNotSigned);
}

void IACUIControl::SetupButtons(HWND hWnd)
{
    char    sz[MAX_PATH];

    if ((m_riih.ProviderData()) &&
        (m_riih.ProviderData()->psPfns) &&
        (m_riih.ProviderData()->psPfns->psUIpfns) &&
        (m_riih.ProviderData()->psPfns->psUIpfns->psUIData))
    {
        if (m_riih.ProviderData()->psPfns->psUIpfns->psUIData->pYesButtonText)
        {
            if (!(m_riih.ProviderData()->psPfns->psUIpfns->psUIData->pYesButtonText[0]))
            {
                ShowWindow(GetDlgItem(hWnd, IDYES), SW_HIDE);
            }
            else
            {
                SetWindowTextU(GetDlgItem(hWnd, IDYES), m_riih.ProviderData()->psPfns->psUIpfns->psUIData->pYesButtonText);
            }
        }

        if (m_riih.ProviderData()->psPfns->psUIpfns->psUIData->pNoButtonText)
        {
            if (!(m_riih.ProviderData()->psPfns->psUIpfns->psUIData->pNoButtonText[0]))
            {
                ShowWindow(GetDlgItem(hWnd, IDNO), SW_HIDE);
            }
            else
            {
                SetWindowTextU(GetDlgItem(hWnd, IDNO), m_riih.ProviderData()->psPfns->psUIpfns->psUIData->pNoButtonText);
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     IACUIControl::OnUIMessage, public
//
//  Synopsis:   responds to UI messages
//
//  Arguments:  [hwnd]   -- window
//              [uMsg]   -- message id
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if message processing should continue, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
IACUIControl::OnUIMessage (
                  HWND   hwnd,
                  UINT   uMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  )
{
    switch ( uMsg )
    {
    case WM_INITDIALOG:
        {
            BOOL fReturn;
            HICON   hIcon;

            fReturn = OnInitDialog(hwnd, wParam, lParam);

            ACUICenterWindow(hwnd);

 //           hIcon = LoadIcon((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), MAKEINTRESOURCE(IDI_LOCK));

 //           dwOrigIcon = SetClassLongPtr(hwnd, GCLP_HICON,
 //                                     (LONG_PTR)LoadIcon((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
 //                                                    MAKEINTRESOURCE(IDI_LOCK)));

            // PostMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            // PostMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

            return( fReturn );
        }
        break;

    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam);
            WORD wId = LOWORD(wParam);
            HWND hwndControl = (HWND)lParam;

            if ( wNotifyCode == BN_CLICKED )
            {
                if ( wId == IDYES )
                {
                    return( OnYes(hwnd) );
                }
                else if ( wId == IDNO )
                {
                    return( OnNo(hwnd) );
                }
                else if ( wId == IDMORE )
                {
                    return( OnMore(hwnd) );
                }
            }

            return( FALSE );
        }
        break;

    case WM_CLOSE:
        return( OnNo(hwnd) );
        break;

    default:
        return( FALSE );
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CVerifiedTrustUI::CVerifiedTrustUI, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [riih] -- invoke info helper reference
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CVerifiedTrustUI::CVerifiedTrustUI (CInvokeInfoHelper& riih, HRESULT& rhr)
                 : IACUIControl( riih ),
                   m_pszInstallAndRun( NULL ),
                   m_pszAuthenticity( NULL ),
                   m_pszCaution( NULL ),
                   m_pszPersonalTrust( NULL )
{
    DWORD_PTR aMessageArgument[3];

    //
    // Initialize the hot-link subclass data
    //

    m_lsdPublisher.uId          = IDC_PUBLISHER;
    m_lsdPublisher.hwndParent   = NULL;
    m_lsdPublisher.wpPrev       = (WNDPROC)NULL;
    m_lsdPublisher.pvData       = (LPVOID)&riih;
    m_lsdPublisher.uToolTipText = IDS_CLICKHEREFORCERT;

    m_lsdOpusInfo.uId           = IDC_INSTALLANDRUN;
    m_lsdOpusInfo.hwndParent    = NULL;
    m_lsdOpusInfo.wpPrev        = (WNDPROC)NULL;
    m_lsdOpusInfo.pvData        = &riih;
    m_lsdOpusInfo.uToolTipText  = (DWORD_PTR)riih.ControlWebPage();

    m_lsdCA.uId                 = IDC_AUTHENTICITY;
    m_lsdCA.hwndParent          = NULL;
    m_lsdCA.wpPrev              = (WNDPROC)NULL;
    m_lsdCA.pvData              = &riih;
    m_lsdCA.uToolTipText        = (DWORD_PTR)riih.CAWebPage(); // IDS_CLICKHEREFORCAINFO;

    m_lsdAdvanced.uId           = IDC_ADVANCED;
    m_lsdAdvanced.hwndParent    = NULL;
    m_lsdAdvanced.wpPrev        = (WNDPROC)NULL;
    m_lsdAdvanced.pvData        = &riih;
    m_lsdAdvanced.uToolTipText  = IDS_CLICKHEREFORADVANCED;


    //
    // Format the install and run string
    //

    aMessageArgument[2] = NULL;

    if (m_riih.CertTimestamp())
    {
        aMessageArgument[0] = (DWORD_PTR)m_pszCopyActionText;
        aMessageArgument[1] = (DWORD_PTR)m_riih.Subject();
        aMessageArgument[2] = (DWORD_PTR)m_riih.CertTimestamp();
    }
    else
    {
        aMessageArgument[0] = (DWORD_PTR)m_pszCopyActionTextNoTS;
        aMessageArgument[1] = (DWORD_PTR)m_riih.Subject();
        aMessageArgument[2] = NULL;
    }

    rhr = FormatACUIResourceString(0, aMessageArgument, &m_pszInstallAndRun);

    //
    // Format the authenticity string
    //

    if ( rhr == S_OK )
    {
        aMessageArgument[0] = (DWORD_PTR)m_riih.PublisherCertIssuer();

        rhr = FormatACUIResourceString(
                        IDS_AUTHENTICITY,
                        aMessageArgument,
                        &m_pszAuthenticity
                        );
    }

    //
    // Get the publisher as a message argument
    //

    aMessageArgument[0] = (DWORD_PTR)m_riih.Publisher();

    //
    // Format the caution string
    //

    if ( rhr == S_OK )
    {
        rhr = FormatACUIResourceString(
                        IDS_CAUTION,
                        aMessageArgument,
                        &m_pszCaution
                        );
    }

    //
    // Format the personal trust string
    //

    if ( rhr == S_OK )
    {
        rhr = FormatACUIResourceString(
                        IDS_PERSONALTRUST,
                        aMessageArgument,
                        &m_pszPersonalTrust
                        );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CVerifiedTrustUI::~CVerifiedTrustUI, public
//
//  Synopsis:   Destructor
//
//  Arguments:  (none)
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CVerifiedTrustUI::~CVerifiedTrustUI ()
{
    DELETE_OBJECT(m_pszInstallAndRun);
    DELETE_OBJECT(m_pszAuthenticity);
    DELETE_OBJECT(m_pszCaution);
    DELETE_OBJECT(m_pszPersonalTrust);
}

//+---------------------------------------------------------------------------
//
//  Member:     CVerifiedTrustUI::InvokeUI, public
//
//  Synopsis:   invoke the UI
//
//  Arguments:  [hDisplay] -- parent window
//
//  Returns:    S_OK, user trusts the subject
//              TRUST_E_SUBJECT_NOT_TRUSTED, user does NOT trust the subject
//              Any other valid HRESULT
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CVerifiedTrustUI::InvokeUI (HWND hDisplay)
{
    //
    // Bring up the dialog
    //

    if ( DialogBoxParamU(
               g_hModule,
               (LPWSTR) MAKEINTRESOURCE(IDD_DIALOG1_VERIFIED),
               hDisplay,
               ACUIMessageProc,
               (LPARAM)this
               ) == -1 )
    {
        return( HRESULT_FROM_WIN32(GetLastError()) );
    }


    //
    // The result has been stored as a member
    //

    return( m_hrInvokeResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CVerifiedTrustUI::OnInitDialog, public
//
//  Synopsis:   dialog initialization
//
//  Arguments:  [hwnd]   -- dialog window
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if successful init, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CVerifiedTrustUI::OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    WCHAR psz[MAX_LOADSTRING_BUFFER];
    HWND hControl;
    int  deltavpos = 0;
    int  deltaheight;
    int  bmptosep;
    int  septodlg;
    int  savevpos;
    int  hkcharpos;
    RECT rect;

    //
    // Setup the publisher link subclass data parent window
    //

    m_lsdPublisher.hwndParent   = hwnd;
    m_lsdOpusInfo.hwndParent    = hwnd;
    m_lsdCA.hwndParent          = hwnd;
    m_lsdAdvanced.hwndParent    = hwnd;

    //
    // Render the install and run string
    //


    deltavpos = RenderACUIStringToEditControl(
                                 hwnd,
                                 IDC_INSTALLANDRUN,
                                 IDC_PUBLISHER,
                                 m_pszInstallAndRun,
                                 deltavpos,
                                 (m_riih.ControlWebPage()) ? TRUE : FALSE,
                                 (WNDPROC)ACUILinkSubclass,
                                 &m_lsdOpusInfo,
                                 0,
                                 m_riih.Subject());


    //
    // Render the publisher, give it a "link" look and feel if it is a known
    // publisher
    //

        //
        // if there was a test cert in the chain, add it to the text...
        //
    if (m_riih.TestCertInChain())
    {
        WCHAR    *pszCombine;

        pszCombine = new WCHAR[wcslen(m_riih.Publisher()) + wcslen(m_riih.TestCertInChain()) + 3];

        if (pszCombine != NULL)
        {
            wcscpy(pszCombine, m_riih.Publisher());
            wcscat(pszCombine, L"\r\n");
            wcscat(pszCombine, m_riih.TestCertInChain());

            deltavpos = RenderACUIStringToEditControl(
                                         hwnd,
                                         IDC_PUBLISHER,
                                         IDC_AUTHENTICITY,
                                         pszCombine,
                                         deltavpos,
                                         m_riih.IsKnownPublisher() &&
                                         m_riih.IsCertViewPropertiesAvailable(),
                                         (WNDPROC)ACUILinkSubclass,
                                         &m_lsdPublisher,
                                         0,
                                         NULL
                                         );

            delete[] pszCombine;
        }

        if (LoadStringU(g_hModule, IDS_TESTCERTTITLE, psz, MAX_LOADSTRING_BUFFER) != 0)
        {
            int wtlen;

            wtlen = wcslen(psz) + GetWindowTextLength(hwnd);
            pszCombine = new WCHAR[wtlen + 1];

            if (pszCombine != NULL)
            {
                GetWindowTextU(hwnd, pszCombine, wtlen + 1);
                wcscat(pszCombine, psz);
                SetWindowTextU(hwnd, pszCombine);

                delete[] pszCombine;
            }
        }
    }
    else
    {
        deltavpos = RenderACUIStringToEditControl(
                                     hwnd,
                                     IDC_PUBLISHER,
                                     IDC_AUTHENTICITY,
                                     m_riih.Publisher(),
                                     deltavpos,
                                     m_riih.IsKnownPublisher() &&
                                     m_riih.IsCertViewPropertiesAvailable(),
                                     (WNDPROC)ACUILinkSubclass,
                                     &m_lsdPublisher,
                                     0,
                                     NULL
                                     );
    }

    //
    // Render the authenticity statement
    //
    deltavpos = RenderACUIStringToEditControl(
                                 hwnd,
                                 IDC_AUTHENTICITY,
                                 IDC_CAUTION,
                                 m_pszAuthenticity,
                                 deltavpos,
                                 (m_riih.CAWebPage()) ? TRUE : FALSE,
                                 (WNDPROC)ACUILinkSubclass,
                                 &m_lsdCA,
                                 0,
                                 m_riih.PublisherCertIssuer());


    //
    // Render the caution statement
    //

    deltavpos = RenderACUIStringToEditControl(
                                 hwnd,
                                 IDC_CAUTION,
                                 IDC_ADVANCED,
                                 m_pszCaution,
                                 deltavpos,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL
                                 );

    //
    // Render the advanced string
    //
    if ((m_riih.AdvancedLink()) &&
         (m_riih.ProviderData()->psPfns->psUIpfns->pfnOnAdvancedClick))
    {
        deltavpos = RenderACUIStringToEditControl(
                                     hwnd,
                                     IDC_ADVANCED,
                                     IDC_PERSONALTRUST,
                                     m_riih.AdvancedLink(),
                                     deltavpos,
                                     TRUE,
                                     (WNDPROC)ACUILinkSubclass,
                                     &m_lsdAdvanced,
                                     0,
                                     NULL
                                     );
    }
    else
    {
        ShowWindow(GetDlgItem(hwnd, IDC_ADVANCED), SW_HIDE);
    }

    //
    // Calculate the distances from the bottom of the bitmap to the top
    // of the separator and from the bottom of the separator to the bottom
    // of the dialog
    //

    bmptosep = CalculateControlVerticalDistance(
                               hwnd,
                               IDC_VERBMP,
                               IDC_SEPARATORLINE
                               );

    septodlg = CalculateControlVerticalDistanceFromDlgBottom(
                                                   hwnd,
                                                   IDC_SEPARATORLINE
                                                   );

    //
    // Rebase the check box and render the personal trust statement or hide
    // them the publisher is not known
    //

    if ( m_riih.IsKnownPublisher() == TRUE )
    {
        hControl = GetDlgItem(hwnd, IDC_PTCHECK);

        RebaseControlVertical(
                     hwnd,
                     hControl,
                     NULL,
                     FALSE,
                     deltavpos,
                     0,
                     bmptosep,
                     &deltaheight
                     );

        assert( deltaheight == 0 );

        //
        // Find the hotkey character position for the personal trust
        // check box
        //

        hkcharpos = GetHotKeyCharPosition(GetDlgItem(hwnd, IDC_PTCHECK));

        deltavpos = RenderACUIStringToEditControl(
                                    hwnd,
                                    IDC_PERSONALTRUST,
                                    IDC_SEPARATORLINE,
                                    m_pszPersonalTrust,
                                    deltavpos,
                                    FALSE,
                                    NULL,
                                    NULL,
                                    bmptosep,
                                    NULL
                                    );

        if ( hkcharpos != 0 )
        {
            FormatHotKeyOnEditControl(
                          GetDlgItem(hwnd, IDC_PERSONALTRUST),
                          hkcharpos
                          );
        }
    }
    else
    {
        ShowWindow(GetDlgItem(hwnd, IDC_PTCHECK), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_PERSONALTRUST), SW_HIDE);
    }


    //
    // Rebase the static line
    //

    hControl = GetDlgItem(hwnd, IDC_SEPARATORLINE);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    assert( deltaheight == 0 );

    //
    // Rebase the buttons
    //

    hControl = GetDlgItem(hwnd, IDYES);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    assert( deltaheight == 0 );

    hControl = GetDlgItem(hwnd, IDNO);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    assert( deltaheight == 0 );

    hControl = GetDlgItem(hwnd, IDMORE);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    assert( deltaheight == 0 );

    //
    // Resize the bitmap and the dialog rectangle if necessary
    //

    if ( deltavpos > 0 )
    {
        int cyupd;

        hControl = GetDlgItem(hwnd, IDC_VERBMP);
        GetWindowRect(hControl, &rect);

        cyupd = CalculateControlVerticalDistance(
                                hwnd,
                                IDC_VERBMP,
                                IDC_SEPARATORLINE
                                );

        cyupd -= bmptosep;

        SetWindowPos(
           hControl,
           NULL,
           0,
           0,
           rect.right - rect.left,
           (rect.bottom - rect.top) + cyupd,
           SWP_NOZORDER | SWP_NOMOVE
           );

        GetWindowRect(hwnd, &rect);

        cyupd = CalculateControlVerticalDistanceFromDlgBottom(
                                hwnd,
                                IDC_SEPARATORLINE
                                );

        cyupd = septodlg - cyupd;

        SetWindowPos(
           hwnd,
           NULL,
           0,
           0,
           rect.right - rect.left,
           (rect.bottom - rect.top) + cyupd,
           SWP_NOZORDER | SWP_NOMOVE
           );
    }

    //
    //  check for overridden button texts
    //
    this->SetupButtons(hwnd);

    //
    // Set focus to appropriate control
    //

    hControl = GetDlgItem(hwnd, IDNO);
    ::PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM) hControl, (LPARAM) MAKEWORD(TRUE, 0));
    
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CVerifiedTrustUI::OnYes, public
//
//  Synopsis:   process IDYES button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CVerifiedTrustUI::OnYes (HWND hwnd)
{
    //
    // Set the invoke result
    //

    m_hrInvokeResult = S_OK;

    //
    // Add the publisher to the trust database
    //
    // BUGBUG: What if this fails??
    //

    if ( SendDlgItemMessage(
             hwnd,
             IDC_PTCHECK,
             BM_GETCHECK,
             0,
             0
             ) == BST_CHECKED )
    {
        m_riih.AddPublisherToPersonalTrust();
    }

    //
    // End the dialog processing
    //

    EndDialog(hwnd, (int)m_hrInvokeResult);
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CVerifiedTrustUI::OnNo, public
//
//  Synopsis:   process IDNO button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CVerifiedTrustUI::OnNo (HWND hwnd)
{
    m_hrInvokeResult = TRUST_E_SUBJECT_NOT_TRUSTED;

    EndDialog(hwnd, (int)m_hrInvokeResult);
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CVerifiedTrustUI::OnMore, public
//
//  Synopsis:   process the IDMORE button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CVerifiedTrustUI::OnMore (HWND hwnd)
{
    WinHelp(hwnd, "SECAUTH.HLP", HELP_CONTEXT, IDH_SECAUTH_SIGNED);

        // ACUIViewHTMLHelpTopic(hwnd, "sec_signed.htm");

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::CUnverifiedTrustUI, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [riih] -- invoke info helper reference
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CUnverifiedTrustUI::CUnverifiedTrustUI (CInvokeInfoHelper& riih, HRESULT& rhr)
                 : IACUIControl( riih ),
                   m_pszNoAuthenticity( NULL ),
                   m_pszProblemsBelow( NULL ),
                   m_pszInstallAndRun3( NULL )
{
    DWORD_PTR aMessageArgument[3];

    //
    // Initialize the publisher link subclass data
    //

    m_lsdPublisher.uId          = IDC_PUBLISHER;
    m_lsdPublisher.hwndParent   = NULL;
    m_lsdPublisher.wpPrev       = (WNDPROC)NULL;
    m_lsdPublisher.pvData       = (LPVOID)&riih;
    m_lsdPublisher.uToolTipText = IDS_CLICKHEREFORCERT;

    m_lsdOpusInfo.uId           = IDC_INSTALLANDRUN;
    m_lsdOpusInfo.hwndParent    = NULL;
    m_lsdOpusInfo.wpPrev        = (WNDPROC)NULL;
    m_lsdOpusInfo.pvData        = &riih;
    m_lsdOpusInfo.uToolTipText  = (DWORD_PTR)riih.ControlWebPage(); // IDS_CLICKHEREFOROPUSINFO;

    m_lsdAdvanced.uId           = IDC_ADVANCED;
    m_lsdAdvanced.hwndParent    = NULL;
    m_lsdAdvanced.wpPrev        = (WNDPROC)NULL;
    m_lsdAdvanced.pvData        = &riih;
    m_lsdAdvanced.uToolTipText  = IDS_CLICKHEREFORADVANCED;


    //
    // Format the no authenticity string
    //

    rhr = FormatACUIResourceString(
                    IDS_NOAUTHENTICITY,
                    NULL,
                    &m_pszNoAuthenticity
                    );

    //
    // Format the problems below string
    //

    if ( rhr == S_OK )
    {
        aMessageArgument[0] = (DWORD_PTR)m_riih.ErrorStatement();

        rhr = FormatACUIResourceString(
                    IDS_PROBLEMSBELOW,
                    aMessageArgument,
                    &m_pszProblemsBelow
                    );
    }

    //
    // Format the install and run string
    //

    if ( rhr == S_OK )
    {
        if (m_riih.CertTimestamp())
        {
            aMessageArgument[0] = (DWORD_PTR)m_pszCopyActionText;
            aMessageArgument[1] = (DWORD_PTR)m_riih.Subject();
            aMessageArgument[2] = (DWORD_PTR)m_riih.CertTimestamp();
        }
        else
        {
            aMessageArgument[0] = (DWORD_PTR)m_pszCopyActionTextNoTS;
            aMessageArgument[1] = (DWORD_PTR)m_riih.Subject();
            aMessageArgument[2] = NULL;
        }

        rhr = FormatACUIResourceString(0, aMessageArgument, &m_pszInstallAndRun3);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::~CUnverifiedTrustUI, public
//
//  Synopsis:   Destructor
//
//  Arguments:  (none)
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CUnverifiedTrustUI::~CUnverifiedTrustUI ()
{
    DELETE_OBJECT(m_pszNoAuthenticity);
    DELETE_OBJECT(m_pszProblemsBelow);
    DELETE_OBJECT(m_pszInstallAndRun3);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::InvokeUI, public
//
//  Synopsis:   invoke the UI
//
//  Arguments:  [hDisplay] -- parent window
//
//  Returns:    S_OK, user trusts the subject
//              TRUST_E_SUBJECT_NOT_TRUSTED, user does NOT trust the subject
//              Any other valid HRESULT
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CUnverifiedTrustUI::InvokeUI (HWND hDisplay)
{
    HRESULT hr = S_OK;

    //
    // Bring up the dialog
    //

    if ( DialogBoxParamU(
               g_hModule,
               (LPWSTR) MAKEINTRESOURCE(IDD_DIALOG2_UNVERIFIED),
               hDisplay,
               ACUIMessageProc,
               (LPARAM)this
               ) == -1 )
    {
        return( HRESULT_FROM_WIN32(GetLastError()) );
    }

    //
    // The result has been stored as a member
    //

    return( m_hrInvokeResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::OnInitDialog, public
//
//  Synopsis:   dialog initialization
//
//  Arguments:  [hwnd]   -- dialog window
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if successful init, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CUnverifiedTrustUI::OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HWND hControl;
    int  deltavpos = 0;
    int  deltaheight;
    int  bmptosep;
    int  septodlg;
    RECT rect;

    //
    // Setup the publisher link subclass data parent window
    //

    m_lsdPublisher.hwndParent   = hwnd;
    m_lsdOpusInfo.hwndParent    = hwnd;
    m_lsdAdvanced.hwndParent    = hwnd;


    //
    // Render the no authenticity statement
    //

    deltavpos = RenderACUIStringToEditControl(
                                 hwnd,
                                 IDC_NOAUTHENTICITY,
                                 IDC_PROBLEMSBELOW,
                                 m_pszNoAuthenticity,
                                 deltavpos,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL
                                 );

    //
    // Render the problems below string
    //

    deltavpos = RenderACUIStringToEditControl(
                                 hwnd,
                                 IDC_PROBLEMSBELOW,
                                 IDC_INSTALLANDRUN3,
                                 m_pszProblemsBelow,
                                 deltavpos,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL
                                 );

    //
    // Render the install and run string
    //

    deltavpos = RenderACUIStringToEditControl(
                                 hwnd,
                                 IDC_INSTALLANDRUN3,
                                 IDC_PUBLISHER2,
                                 m_pszInstallAndRun3,
                                 deltavpos,
                                 (m_riih.ControlWebPage()) ? TRUE : FALSE,
                                 (WNDPROC)ACUILinkSubclass,
                                 &m_lsdOpusInfo,
                                 0,
                                 m_riih.Subject());


    //
    // Calculate the distances from the bottom of the bitmap to the top
    // of the separator and from the bottom of the separator to the bottom
    // of the dialog
    //

    bmptosep = CalculateControlVerticalDistance(
                               hwnd,
                               IDC_NOVERBMP2,
                               IDC_SEPARATORLINE
                               );

    septodlg = CalculateControlVerticalDistanceFromDlgBottom(
                                                   hwnd,
                                                   IDC_SEPARATORLINE
                                                   );

    //
    // Render the publisher, give it a "link" look and feel
    //

    deltavpos = RenderACUIStringToEditControl(
                                 hwnd,
                                 IDC_PUBLISHER2,
                                 IDC_ADVANCED,
                                 m_riih.Publisher(),
                                 deltavpos,
                                 m_riih.IsKnownPublisher() &&
                                 m_riih.IsCertViewPropertiesAvailable(),
                                 (WNDPROC)ACUILinkSubclass,
                                 &m_lsdPublisher,
                                 bmptosep,
                                 NULL
                                 );

    if ((m_riih.AdvancedLink()) &&
         (m_riih.ProviderData()->psPfns->psUIpfns->pfnOnAdvancedClick))
    {
        deltavpos = RenderACUIStringToEditControl(
                                     hwnd,
                                     IDC_ADVANCED,
                                     IDC_SEPARATORLINE,
                                     m_riih.AdvancedLink(),
                                     deltavpos,
                                     TRUE,
                                     (WNDPROC)ACUILinkSubclass,
                                     &m_lsdAdvanced,
                                     0,
                                     NULL
                                     );
    }
    else
    {
        ShowWindow(GetDlgItem(hwnd, IDC_ADVANCED), SW_HIDE);
    }

    //
    // Rebase the static line
    //

    hControl = GetDlgItem(hwnd, IDC_SEPARATORLINE);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    assert( deltaheight == 0 );

    //
    // Rebase the buttons
    //

    hControl = GetDlgItem(hwnd, IDYES);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    assert( deltaheight == 0 );

    hControl = GetDlgItem(hwnd, IDNO);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    assert( deltaheight == 0 );

    hControl = GetDlgItem(hwnd, IDMORE);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    assert( deltaheight == 0 );

    //
    // Resize the bitmap and the dialog rectangle if necessary
    //

    if ( deltavpos > 0 )
    {
        int cyupd;

        hControl = GetDlgItem(hwnd, IDC_NOVERBMP2);
        GetWindowRect(hControl, &rect);

        cyupd = CalculateControlVerticalDistance(
                                hwnd,
                                IDC_NOVERBMP2,
                                IDC_SEPARATORLINE
                                );

        cyupd -= bmptosep;

        SetWindowPos(
                 hControl,
                 NULL,
                 0,
                 0,
                 rect.right - rect.left,
                 (rect.bottom - rect.top) + cyupd,
                 SWP_NOZORDER | SWP_NOMOVE
                 );

        GetWindowRect(hwnd, &rect);

        cyupd = CalculateControlVerticalDistanceFromDlgBottom(
                                hwnd,
                                IDC_SEPARATORLINE
                                );

        cyupd = septodlg - cyupd;

        SetWindowPos(
                 hwnd,
                 NULL,
                 0,
                 0,
                 rect.right - rect.left,
                 (rect.bottom - rect.top) + cyupd,
                 SWP_NOZORDER | SWP_NOMOVE
                 );
    }

    //
    //  check for overridden button texts
    //
    this->SetupButtons(hwnd);

    //
    // Set focus to appropriate control
    //

    hControl = GetDlgItem(hwnd, IDNO);
    ::PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM) hControl, (LPARAM) MAKEWORD(TRUE, 0));

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::OnYes, public
//
//  Synopsis:   process IDYES button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CUnverifiedTrustUI::OnYes (HWND hwnd)
{
    m_hrInvokeResult = S_OK;

    EndDialog(hwnd, (int)m_hrInvokeResult);
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::OnNo, public
//
//  Synopsis:   process IDNO button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CUnverifiedTrustUI::OnNo (HWND hwnd)
{
    m_hrInvokeResult = TRUST_E_SUBJECT_NOT_TRUSTED;

    EndDialog(hwnd, (int)m_hrInvokeResult);
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnverifiedTrustUI::OnMore, public
//
//  Synopsis:   process the IDMORE button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CUnverifiedTrustUI::OnMore (HWND hwnd)
{
    WinHelp(hwnd, "SECAUTH.HLP", HELP_CONTEXT, IDH_SECAUTH_SIGNED_N_INVALID);

        // ACUIViewHTMLHelpTopic(hwnd, "sec_signed_n_invalid.htm");

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNoSignatureUI::CNoSignatureUI, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [riih] -- invoke info helper
//              [rhr]  -- result code reference
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CNoSignatureUI::CNoSignatureUI (CInvokeInfoHelper& riih, HRESULT& rhr)
               : IACUIControl( riih ),
                 m_pszInstallAndRun2( NULL ),
                 m_pszNoPublisherFound( NULL )
{
    DWORD_PTR aMessageArgument[2];

    //
    // Format the install and run string
    //

    aMessageArgument[0] = (DWORD_PTR)m_pszCopyActionTextNotSigned;
    aMessageArgument[1] = (DWORD_PTR)m_riih.Subject();

    rhr = FormatACUIResourceString(0, aMessageArgument, &m_pszInstallAndRun2);

    //
    // Format the no publisher found string
    //

    if ( rhr == S_OK )
    {
        aMessageArgument[0] = (DWORD_PTR)m_riih.ErrorStatement();

        rhr = FormatACUIResourceString(
                    IDS_NOPUBLISHERFOUND,
                    aMessageArgument,
                    &m_pszNoPublisherFound
                    );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CNoSignatureUI::~CNoSignatureUI, public
//
//  Synopsis:   Destructor
//
//  Arguments:  (none)
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CNoSignatureUI::~CNoSignatureUI ()
{
    DELETE_OBJECT(m_pszInstallAndRun2);
    DELETE_OBJECT(m_pszNoPublisherFound);
}

//+---------------------------------------------------------------------------
//
//  Member:     CNoSignatureUI::InvokeUI, public
//
//  Synopsis:   invoke the UI
//
//  Arguments:  [hDisplay] -- parent window
//
//  Returns:    S_OK, user trusts the subject
//              TRUST_E_SUBJECT_NOT_TRUSTED, user does NOT trust the subject
//              Any other valid HRESULT
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CNoSignatureUI::InvokeUI (HWND hDisplay)
{
    HRESULT hr = S_OK;

    //
    // Bring up the dialog
    //

    if ( DialogBoxParamU(
               g_hModule,
               (LPWSTR) MAKEINTRESOURCE(IDD_DIALOG3_NOSIGNATURE),
               hDisplay,
               ACUIMessageProc,
               (LPARAM)this
               ) == -1 )
    {
        return( HRESULT_FROM_WIN32(GetLastError()) );
    }

    //
    // The result has been stored as a member
    //

    return( m_hrInvokeResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNoSignatureUI::OnInitDialog, public
//
//  Synopsis:   dialog initialization
//
//  Arguments:  [hwnd]   -- dialog window
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if successful init, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CNoSignatureUI::OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HWND hControl;
    int  deltavpos = 0;
    int  deltaheight;
    int  bmptosep;
    int  septodlg;
    RECT rect;

    //
    // Render the install and run string
    //

    deltavpos = RenderACUIStringToEditControl(
                                 hwnd,
                                 IDC_INSTALLANDRUN2,
                                 IDC_NOPUBLISHERFOUND,
                                 m_pszInstallAndRun2,
                                 deltavpos,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL
                                 );

    //
    // Calculate the distances from the bottom of the bitmap to the top
    // of the separator and from the bottom of the separator to the bottom
    // of the dialog
    //

    bmptosep = CalculateControlVerticalDistance(
                               hwnd,
                               IDC_NOVERBMP,
                               IDC_SEPARATORLINE
                               );

    septodlg = CalculateControlVerticalDistanceFromDlgBottom(
                                                   hwnd,
                                                   IDC_SEPARATORLINE
                                                   );

    //
    // Render the no publisher found statement
    //

    deltavpos = RenderACUIStringToEditControl(
                                 hwnd,
                                 IDC_NOPUBLISHERFOUND,
                                 IDC_SEPARATORLINE,
                                 m_pszNoPublisherFound,
                                 deltavpos,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 bmptosep,
                                 NULL
                                 );

    //
    // Rebase the static line
    //

    hControl = GetDlgItem(hwnd, IDC_SEPARATORLINE);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    assert( deltaheight == 0 );

    //
    // Rebase the buttons
    //

    hControl = GetDlgItem(hwnd, IDYES);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    assert( deltaheight == 0 );

    hControl = GetDlgItem(hwnd, IDNO);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    assert( deltaheight == 0 );

    hControl = GetDlgItem(hwnd, IDMORE);
    RebaseControlVertical(hwnd, hControl, NULL, FALSE, deltavpos, 0, 0, &deltaheight);
    assert( deltaheight == 0 );

    //
    // Resize the bitmap and the dialog rectangle if necessary
    //

    if ( deltavpos > 0 )
    {
        int cyupd;

        hControl = GetDlgItem(hwnd, IDC_NOVERBMP);
        GetWindowRect(hControl, &rect);

        cyupd = CalculateControlVerticalDistance(
                                hwnd,
                                IDC_NOVERBMP,
                                IDC_SEPARATORLINE
                                );

        cyupd -= bmptosep;

        SetWindowPos(
                 hControl,
                 NULL,
                 0,
                 0,
                 rect.right - rect.left,
                 (rect.bottom - rect.top) + cyupd,
                 SWP_NOZORDER | SWP_NOMOVE
                 );

        GetWindowRect(hwnd, &rect);

        cyupd = CalculateControlVerticalDistanceFromDlgBottom(
                                hwnd,
                                IDC_SEPARATORLINE
                                );

        cyupd = septodlg - cyupd;

        SetWindowPos(
                 hwnd,
                 NULL,
                 0,
                 0,
                 rect.right - rect.left,
                 (rect.bottom - rect.top) + cyupd,
                 SWP_NOZORDER | SWP_NOMOVE
                 );
    }

    //
    //  check for overridden button texts
    //
    this->SetupButtons(hwnd);

    //
    // Set focus to appropriate control
    //

    hControl = GetDlgItem(hwnd, IDNO);
    ::PostMessage(hwnd, WM_NEXTDLGCTL, (WPARAM) hControl, (LPARAM) MAKEWORD(TRUE, 0));

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNoSignatureUI::OnYes, public
//
//  Synopsis:   process IDYES button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CNoSignatureUI::OnYes (HWND hwnd)
{
    m_hrInvokeResult = S_OK;

    EndDialog(hwnd, (int)m_hrInvokeResult);
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNoSignatureUI::OnNo, public
//
//  Synopsis:   process IDNO button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CNoSignatureUI::OnNo (HWND hwnd)
{
    m_hrInvokeResult = TRUST_E_SUBJECT_NOT_TRUSTED;

    EndDialog(hwnd, (int)m_hrInvokeResult);
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNoSignatureUI::OnMore, public
//
//  Synopsis:   process the IDMORE button click
//
//  Arguments:  [hwnd] -- window handle
//
//  Returns:    TRUE
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
CNoSignatureUI::OnMore (HWND hwnd)
{
    WinHelp(hwnd, "SECAUTH.HLP", HELP_CONTEXT, IDH_SECAUTH_UNSIGNED);

        // ACUIViewHTMLHelpTopic(hwnd, "sec_unsigned.htm");

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   ACUIMessageProc
//
//  Synopsis:   message proc to process UI messages
//
//  Arguments:  [hwnd]   -- window
//              [uMsg]   -- message id
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if message processing should continue, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK ACUIMessageProc (
                  HWND   hwnd,
                  UINT   uMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  )
{
    IACUIControl* pUI = NULL;

    //
    // Get the control
    //

    if (uMsg == WM_INITDIALOG)
    {
        pUI = (IACUIControl *)lParam;
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
    }
    else
    {
        pUI = (IACUIControl *)GetWindowLongPtr(hwnd, DWLP_USER);
    }

    //
    // If we couldn't find it, we must not have set it yet, so ignore this
    // message
    //

    if ( pUI == NULL )
    {
        return( FALSE );
    }

    //
    // Pass the message on to the control
    //

    return( pUI->OnUIMessage(hwnd, uMsg, wParam, lParam) );
}


int GetRichEditControlLineHeight(HWND  hwnd)
{
    RECT        rect;
    POINT       pointInFirstRow;
    POINT       pointInSecondRow;
    int         secondLineCharIndex;
    int         i;
    RECT        originalRect;

    GetWindowRect(hwnd, &originalRect);

    //
    // HACK ALERT, believe it or not there is no way to get the height of the current
    // font in the edit control, so get the position a character in the first row and the position
    // of a character in the second row, and do the subtraction to get the
    // height of the font
    //
    SendMessageA(hwnd, EM_POSFROMCHAR, (WPARAM) &pointInFirstRow, (LPARAM) 0);

    //
    // HACK ON TOP OF HACK ALERT,
    // since there may not be a second row in the edit box, keep reducing the width
    // by half until the first row falls over into the second row, then get the position
    // of the first char in the second row and finally reset the edit box size back to
    // it's original size
    //
    secondLineCharIndex = (int)SendMessageA(hwnd, EM_LINEINDEX, (WPARAM) 1, (LPARAM) 0);
    if (secondLineCharIndex == -1)
    {
        for (i=0; i<20; i++)
        {
            GetWindowRect(hwnd, &rect);
            SetWindowPos(   hwnd,
                            NULL,
                            0,
                            0,
                            (rect.right-rect.left)/2,
                            rect.bottom-rect.top,
                            SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
            secondLineCharIndex = (int)SendMessageA(hwnd, EM_LINEINDEX, (WPARAM) 1, (LPARAM) 0);
            if (secondLineCharIndex != -1)
            {
                break;
            }
        }

        if (secondLineCharIndex == -1)
        {
            // if we failed after twenty tries just reset the control to its original size
            // and get the heck outa here!!
            SetWindowPos(hwnd,
                    NULL,
                    0,
                    0,
                    originalRect.right-originalRect.left,
                    originalRect.bottom-originalRect.top,
                    SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

            return 0;
        }

        SendMessageA(hwnd, EM_POSFROMCHAR, (WPARAM) &pointInSecondRow, (LPARAM) secondLineCharIndex);

        SetWindowPos(hwnd,
                    NULL,
                    0,
                    0,
                    originalRect.right-originalRect.left,
                    originalRect.bottom-originalRect.top,
                    SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
    else
    {
        SendMessageA(hwnd, EM_POSFROMCHAR, (WPARAM) &pointInSecondRow, (LPARAM) secondLineCharIndex);
    }
    
    return (pointInSecondRow.y - pointInFirstRow.y);
}

//+---------------------------------------------------------------------------
//
//  Function:   RebaseControlVertical
//
//  Synopsis:   Take the window control, if it has to be resized for text, do
//              so.  Reposition it adjusted for delta pos and return any
//              height difference for the text resizing
//
//  Arguments:  [hwndDlg]        -- host dialog
//              [hwnd]           -- control
//              [hwndNext]       -- next control
//              [fResizeForText] -- resize for text flag
//              [deltavpos]      -- delta vertical position
//              [oline]          -- original number of lines
//              [minsep]         -- minimum separator
//              [pdeltaheight]   -- delta in control height
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID RebaseControlVertical (
                  HWND  hwndDlg,
                  HWND  hwnd,
                  HWND  hwndNext,
                  BOOL  fResizeForText,
                  int   deltavpos,
                  int   oline,
                  int   minsep,
                  int*  pdeltaheight
                  )
{
    int        x = 0;
    int        y = 0;
    int        odn = 0;
    int         orig_w;
    RECT       rect;
    RECT       rectNext;
    RECT       rectDlg;
    TEXTMETRIC tm;

    //
    // Set the delta height to zero for now.  If we resize the text
    // a new one will be calculated
    //

    *pdeltaheight = 0;

    //
    // Get the control window rectangle
    //

    GetWindowRect(hwnd, &rect);
    GetWindowRect(hwndNext, &rectNext);

    odn     = rectNext.top - rect.bottom;

    orig_w  = rect.right - rect.left;

    MapWindowPoints(NULL, hwndDlg, (LPPOINT) &rect, 2);

    //
    // If we have to resize the control due to text, find out what font
    // is being used and the number of lines of text.  From that we'll
    // calculate what the new height for the control is and set it up
    //

    if ( fResizeForText == TRUE )
    {
        HDC        hdc;
        HFONT      hfont;
        HFONT      hfontOld;
        int        cline;
        int        h;
        int        w;
        int        dh;
        int        lineHeight;
        
        //
        // Get the metrics of the current control font
        //

        hdc = GetDC(hwnd);
        if (hdc == NULL)
        {
            hdc = GetDC(NULL);
            if (hdc == NULL)
            {
                return;
            }
        }

        hfont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
        if ( hfont == NULL )
        {
            hfont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0);
        }

        hfontOld = (HFONT)SelectObject(hdc, hfont);
        GetTextMetrics(hdc, &tm);

        lineHeight = GetRichEditControlLineHeight(hwnd);
        if (lineHeight == 0)
        {
            lineHeight = tm.tmHeight;
        }
        
        //
        // Set the minimum separation value
        //

        if ( minsep == 0 )
        {
            minsep = lineHeight;
        }

        //
        // Calculate the width and the new height needed
        //

        cline = (int)SendMessage(hwnd, EM_GETLINECOUNT, 0, 0);

        h = cline * lineHeight;

        w = GetEditControlMaxLineWidth(hwnd, hdc, cline);
        w += 3; // a little bump to make sure string will fit

        if (w > orig_w)
        {
            w = orig_w;
        }

        SelectObject(hdc, hfontOld);
        ReleaseDC(hwnd, hdc);

        //
        // Calculate an addition to height by checking how much space was
        // left when there were the original # of lines and making sure that
        // that amount is  still left when we do any adjustments
        //

        h += ( ( rect.bottom - rect.top ) - ( oline * lineHeight ) );
        dh = h - ( rect.bottom - rect.top );

        //
        // If the current height is too small, adjust for it, otherwise
        // leave the current height and just adjust for the width
        //

        if ( dh > 0 )
        {
            SetWindowPos(hwnd, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
        }
        else
        {
            SetWindowPos(
               hwnd,
               NULL,
               0,
               0,
               w,
               ( rect.bottom - rect.top ),
               SWP_NOZORDER | SWP_NOMOVE
               );
        }

        if ( cline < SendMessage(hwnd, EM_GETLINECOUNT, 0, 0) )
        {
            AdjustEditControlWidthToLineCount(hwnd, cline, &tm);
        }
    }

    //
    // If we have to use deltavpos then calculate the X and the new Y
    // and set the window position appropriately
    //

    if ( deltavpos != 0 )
    {
        GetWindowRect(hwndDlg, &rectDlg);

        MapWindowPoints(NULL, hwndDlg, (LPPOINT) &rectDlg, 2);

        x = rect.left - rectDlg.left - GetSystemMetrics(SM_CXEDGE);
        y = rect.top - rectDlg.top - GetSystemMetrics(SM_CYCAPTION) + deltavpos;

        SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }

    //
    // Get the window rect for the next control and see what the distance
    // is between the current control and it.  With that we must now
    // adjust our deltaheight, if the distance to the next control is less
    // than a line height then make it a line height, otherwise just let it
    // be
    //

    if ( hwndNext != NULL )
    {
        int dn;

        GetWindowRect(hwnd, &rect);
        GetWindowRect(hwndNext, &rectNext);

        dn = rectNext.top - rect.bottom;

        if ( odn > minsep )
        {
            if ( dn < minsep )
            {
                *pdeltaheight = minsep - dn;
            }
        }
        else
        {
            if ( dn < odn )
            {
                *pdeltaheight = odn - dn;
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ACUISetArrowCursorSubclass
//
//  Synopsis:   subclass routine for setting the arrow cursor.  This can be
//              set on multiline edit routines used in the dialog UIs for
//              the default Authenticode provider
//
//  Arguments:  [hwnd]   -- window handle
//              [uMsg]   -- message id
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if message handled, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
LRESULT CALLBACK ACUISetArrowCursorSubclass (
                  HWND   hwnd,
                  UINT   uMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  )
{
    HDC         hdc;
    WNDPROC     wndproc;
    PAINTSTRUCT ps;

    wndproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch ( uMsg )
    {
    case WM_SETCURSOR:

        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return( TRUE );

        break;

    case WM_CHAR:

        if ( wParam != (WPARAM)' ' )
        {
            break;
        }

    case WM_LBUTTONDOWN:

        if ( hwnd == GetDlgItem(GetParent(hwnd), IDC_PERSONALTRUST) )
        {
            int  check;
            HWND hwndCheck;

            //
            // Toggle the check state of the PTCHECK control if the
            // personal trust statement is clicked on
            //

            hwndCheck = GetDlgItem(GetParent(hwnd), IDC_PTCHECK);
            check = (int)SendMessage(hwndCheck, BM_GETCHECK, 0, 0);

            if ( check == BST_CHECKED )
            {
                check = BST_UNCHECKED;
            }
            else if ( check == BST_UNCHECKED )
            {
                check = BST_CHECKED;
            }
            else
            {
                check = BST_UNCHECKED;
            }

            SendMessage(hwndCheck, BM_SETCHECK, (WPARAM)check, 0);
            SetFocus(hwnd);
            return( TRUE );
        }

        return(TRUE);

    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:

        return( TRUE );

        break;

    case EM_SETSEL:

        return( TRUE );

        break;

    case WM_PAINT:

        CallWindowProc(wndproc, hwnd, uMsg, wParam, lParam);
        if ( hwnd == GetFocus() )
        {
            DrawFocusRectangle(hwnd, NULL);
        }
        return( TRUE );

        break;

    case WM_SETFOCUS:

        if ( hwnd != GetDlgItem(GetParent(hwnd), IDC_PERSONALTRUST) )
        {
            SetFocus(GetNextDlgTabItem(GetParent(hwnd), hwnd, FALSE));
            return( TRUE );
        }
        else
        {
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return( TRUE );
        }

        break;

    case WM_KILLFOCUS:

        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        return( TRUE );

    }

    return(CallWindowProc(wndproc, hwnd, uMsg, wParam, lParam));
}

//+---------------------------------------------------------------------------
//
//  Function:   SubclassEditControlForArrowCursor
//
//  Synopsis:   subclasses edit control so that the arrow cursor can replace
//              the edit bar
//
//  Arguments:  [hwndEdit] -- edit control
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID SubclassEditControlForArrowCursor (HWND hwndEdit)
{
    LONG_PTR PrevWndProc;

    PrevWndProc = GetWindowLongPtr(hwndEdit, GWLP_WNDPROC);
    SetWindowLongPtr(hwndEdit, GWLP_USERDATA, (LONG_PTR)PrevWndProc);
    SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)ACUISetArrowCursorSubclass);
}

//+---------------------------------------------------------------------------
//
//  Function:   SubclassEditControlForLink
//
//  Synopsis:   subclasses the edit control for a link using the link subclass
//              data
//
//  Arguments:  [hwndDlg]  -- dialog
//              [hwndEdit] -- edit control
//              [wndproc]  -- window proc to subclass with
//              [plsd]     -- data to pass on to window proc
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID SubclassEditControlForLink (
                 HWND                       hwndDlg,
                 HWND                       hwndEdit,
                 WNDPROC                    wndproc,
                 PTUI_LINK_SUBCLASS_DATA    plsd
                 )
{
    HWND hwndTip;

    plsd->hwndTip = CreateWindowA(
                          TOOLTIPS_CLASSA,
                          (LPSTR)NULL,
                          WS_POPUP | TTS_ALWAYSTIP,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          hwndDlg,
                          (HMENU)NULL,
                          g_hModule,
                          NULL
                          );

    if ( plsd->hwndTip != NULL )
    {
        TOOLINFOA   tia;
        DWORD       cb;
        LPSTR       psz;

        memset(&tia, 0, sizeof(TOOLINFOA));
        tia.cbSize = sizeof(TOOLINFOA);
        tia.hwnd = hwndEdit;
        tia.uId = 1;
        tia.hinst = g_hModule;
        //GetClientRect(hwndEdit, &tia.rect);
        SendMessage(hwndEdit, EM_GETRECT, 0, (LPARAM)&tia.rect);

        //
        // if plsd->uToolTipText is a string then convert it
        //
        if (plsd->uToolTipText &0xffff0000)
        {
            cb = WideCharToMultiByte(
                        0, 
                        0, 
                        (LPWSTR)plsd->uToolTipText, 
                        -1,
                        NULL, 
                        0, 
                        NULL, 
                        NULL);

            if (NULL == (psz = new char[cb]))
            {
                return;
            }

            WideCharToMultiByte(
                        0, 
                        0, 
                        (LPWSTR)plsd->uToolTipText, 
                        -1,
                        psz, 
                        cb, 
                        NULL, 
                        NULL);
            
            tia.lpszText = psz;
        }
        else
        {
            tia.lpszText = (LPSTR)plsd->uToolTipText;
        }

        SendMessage(plsd->hwndTip, TTM_ADDTOOL, 0, (LPARAM)&tia);

        if (plsd->uToolTipText &0xffff0000)
        {
            delete[] psz;
        }
    }

    plsd->fMouseCaptured = FALSE;
    plsd->wpPrev = (WNDPROC)GetWindowLongPtr(hwndEdit, GWLP_WNDPROC);
    SetWindowLongPtr(hwndEdit, GWLP_USERDATA, (LONG_PTR)plsd);
    SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)wndproc);
}

//+---------------------------------------------------------------------------
//
//  Function:   ACUILinkSubclass
//
//  Synopsis:   subclass for the publisher link
//
//  Arguments:  [hwnd]   -- window handle
//              [uMsg]   -- message id
//              [wParam] -- parameter 1
//              [lParam] -- parameter 2
//
//  Returns:    TRUE if message handled, FALSE otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
LRESULT CALLBACK ACUILinkSubclass (
                  HWND   hwnd,
                  UINT   uMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  )
{
    PTUI_LINK_SUBCLASS_DATA plsd;
    CInvokeInfoHelper*      piih;

    plsd = (PTUI_LINK_SUBCLASS_DATA)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    piih = (CInvokeInfoHelper *)plsd->pvData;

    switch ( uMsg )
    {
    case WM_SETCURSOR:

        if (!plsd->fMouseCaptured)
        {
            SetCapture(hwnd);
            plsd->fMouseCaptured = TRUE;
        }

        SetCursor(LoadCursor((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                            MAKEINTRESOURCE(IDC_TUIHAND)));
        return( TRUE );

        break;

    case WM_CHAR:

        if ( wParam != (WPARAM)' ')
        {
            break;
        }

        // fall through to wm_lbuttondown....

    case WM_LBUTTONDOWN:

        SetFocus(hwnd);

        switch(plsd->uId)
        {
            case IDC_PUBLISHER:
                piih->CallCertViewProperties(plsd->hwndParent);
                break;

            case IDC_INSTALLANDRUN:
                piih->CallWebLink(plsd->hwndParent, (WCHAR *)piih->ControlWebPage());
                break;

            case IDC_AUTHENTICITY:
                piih->CallWebLink(plsd->hwndParent, (WCHAR *)piih->CAWebPage());
                break;


            case IDC_ADVANCED:
                piih->CallAdvancedLink(plsd->hwndParent);
                break;
        }

        return( TRUE );

    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:

        return( TRUE );

    case EM_SETSEL:

        return( TRUE );

    case WM_PAINT:

        CallWindowProc(plsd->wpPrev, hwnd, uMsg, wParam, lParam);
        if ( hwnd == GetFocus() )
        {
            DrawFocusRectangle(hwnd, NULL);
        }
        return( TRUE );

    case WM_SETFOCUS:

        if ( hwnd == GetFocus() )
        {
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            SetCursor(LoadCursor((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                                MAKEINTRESOURCE(IDC_TUIHAND)));
            return( TRUE );
        }
        break;

    case WM_KILLFOCUS:

        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        SetCursor(LoadCursor(NULL, IDC_ARROW));

        return( TRUE );

    case WM_MOUSEMOVE:

        MSG                 msg;
        DWORD               dwCharLine;
        CHARFORMAT          sCharFmt;
        RECT                rect;
        int                 xPos, yPos;

        memset(&msg, 0, sizeof(MSG));
        msg.hwnd    = hwnd;
        msg.message = uMsg;
        msg.wParam  = wParam;
        msg.lParam  = lParam;

        SendMessage(plsd->hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);

        // check to see if the mouse is in this windows rect, if not, then reset
        // the cursor to an arrow and release the mouse
        GetClientRect(hwnd, &rect);
        xPos = LOWORD(lParam);
        yPos = HIWORD(lParam);
        if ((xPos < 0) ||
            (yPos < 0) ||
            (xPos > (rect.right - rect.left)) ||
            (yPos > (rect.bottom - rect.top)))
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            ReleaseCapture();
            plsd->fMouseCaptured = FALSE;
        }

        /*
            warning!
                    EM_CHARFROMPOS gets an access violation!

        dwCharLine = SendMessage(hwnd, EM_CHARFROMPOS, 0, lParam);

        if (dwCharLine == (-1))
        {
            return(TRUE);
        }

        SendMessage(hwnd, EM_SETSEL, (WPARAM)LOWORD(dwCharLine), (LPARAM)(LOWORD(dwCharLine) + 1));

        memset(&sCharFmt, 0x00, sizeof(CHARFORMAT));
        sCharFmt.cbSize = sizeof(CHARFORMAT);

        SendMessage(hwnd, EM_GETCHARFORMAT, TRUE, (LPARAM)&sCharFmt);

        if (sCharFmt.dwEffects & CFE_UNDERLINE)
        {
            SetCursor(LoadCursor((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                                MAKEINTRESOURCE(IDC_TUIHAND)));
        }
        else
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }

  */
        return( TRUE );
    }

    return(CallWindowProc(plsd->wpPrev, hwnd, uMsg, wParam, lParam));
}

//+---------------------------------------------------------------------------
//
//  Function:   FormatACUIResourceString
//
//  Synopsis:   formats a string given a resource id and message arguments
//
//  Arguments:  [StringResourceId] -- resource id
//              [aMessageArgument] -- message arguments
//              [ppszFormatted]    -- formatted string goes here
//
//  Returns:    S_OK if successful, any valid HRESULT otherwise
//
//----------------------------------------------------------------------------
HRESULT FormatACUIResourceString (
                  UINT   StringResourceId,
                  DWORD_PTR* aMessageArgument,
                  LPWSTR* ppszFormatted
                  )
{
    HRESULT hr = S_OK;
    WCHAR   sz[MAX_LOADSTRING_BUFFER];
    LPVOID  pvMsg;

    pvMsg = NULL;
    sz[0] = NULL;

    //
    // Load the string resource and format the message with that string and
    // the message arguments
    //

    if (StringResourceId != 0)
    {
        if ( LoadStringU(g_hModule, StringResourceId, sz, MAX_LOADSTRING_BUFFER) == 0 )
        {
            return(HRESULT_FROM_WIN32(GetLastError()));
        }

        if ( FormatMessageU(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY, sz, 0, 0, (LPWSTR)&pvMsg, 0,
                            (va_list *)aMessageArgument) == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        if ( FormatMessageU(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY, (char *)aMessageArgument[0], 0, 0,
                            (LPWSTR)&pvMsg, 0, (va_list *)&aMessageArgument[1]) == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (pvMsg)
    {
        *ppszFormatted = new WCHAR[wcslen((WCHAR *)pvMsg) + 1];

        if (*ppszFormatted)
        {
            wcscpy(*ppszFormatted, (WCHAR *)pvMsg);
        }

        LocalFree(pvMsg);
    }

    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Function:   RenderACUIStringToEditControl
//
//  Synopsis:   renders a string to the control given and if requested, gives
//              it a link look and feel, subclassed to the wndproc and plsd
//              given
//
//  Arguments:  [hwndDlg]       -- dialog window handle
//              [ControlId]     -- control id
//              [NextControlId] -- next control id
//              [psz]           -- string
//              [deltavpos]     -- delta vertical position
//              [fLink]         -- a link?
//              [wndproc]       -- optional wndproc, valid if fLink == TRUE
//              [plsd]          -- optional plsd, valid if fLink === TRUE
//              [minsep]        -- minimum separation
//              [pszThisTextOnlyInLink -- only change this text.
//
//  Returns:    delta in height of the control
//
//  Notes:
//
//----------------------------------------------------------------------------
int RenderACUIStringToEditControl (
                  HWND                      hwndDlg,
                  UINT                      ControlId,
                  UINT                      NextControlId,
                  LPCWSTR                   psz,
                  int                       deltavpos,
                  BOOL                      fLink,
                  WNDPROC                   wndproc,
                  PTUI_LINK_SUBCLASS_DATA   plsd,
                  int                       minsep,
                  LPCWSTR                   pszThisTextOnlyInLink
                  )
{
    HWND hControl;
    int  deltaheight = 0;
    int  oline = 0;
    int  hkcharpos;

    //
    // Get the control and set the text on it, make sure the background
    // is right if it is a rich edit control
    //

    hControl = GetDlgItem(hwndDlg, ControlId);
    oline = (int)SendMessage(hControl, EM_GETLINECOUNT, 0, 0);
    CryptUISetRicheditTextW(hwndDlg, ControlId, L"");
    CryptUISetRicheditTextW(hwndDlg, ControlId, psz); //SetWindowTextU(hControl, psz);

    //
    // If there is a '&' in the string, then get rid of it
    //
    hkcharpos = GetHotKeyCharPosition(hControl);
    if (hkcharpos != 0)
    {
        CHARRANGE  cr;
        CHARFORMAT cf;

        cr.cpMin = hkcharpos - 1;
        cr.cpMax = hkcharpos;

        SendMessage(hControl, EM_EXSETSEL, 0, (LPARAM) &cr);
        SendMessage(hControl, EM_REPLACESEL, FALSE, (LPARAM) "");

        cr.cpMin = -1;
        cr.cpMax = 0;
        SendMessage(hControl, EM_EXSETSEL, 0, (LPARAM) &cr);
    }

    SendMessage(
        hControl,
        EM_SETBKGNDCOLOR,
        0,
        (LPARAM)GetSysColor(COLOR_3DFACE)
        );

    //
    // If we have a link then update for the link look
    //

    if ( fLink == TRUE )
    {
        CHARFORMAT cf;

        memset(&cf, 0, sizeof(CHARFORMAT));
        cf.cbSize = sizeof(CHARFORMAT);
        cf.dwMask = CFM_COLOR | CFM_UNDERLINE;

        cf.crTextColor = RGB(0, 0, 255);
        cf.dwEffects |= CFM_UNDERLINE;

        if (pszThisTextOnlyInLink)
        {
            FINDTEXTEX  ft;
            DWORD       pos;
            char        *pszOnlyThis;
            DWORD       cb;

            cb = WideCharToMultiByte(
                        0, 
                        0, 
                        pszThisTextOnlyInLink, 
                        -1,
                        NULL, 
                        0, 
                        NULL, 
                        NULL);

            if (NULL == (pszOnlyThis = new char[cb]))
            {
                return 0;
            }

            WideCharToMultiByte(
                        0, 
                        0, 
                        pszThisTextOnlyInLink, 
                        -1,
                        pszOnlyThis, 
                        cb, 
                        NULL, 
                        NULL);


            memset(&ft, 0x00, sizeof(FINDTEXTEX));
            ft.chrg.cpMin   = 0;
            ft.chrg.cpMax   = (-1);
            ft.lpstrText    = (char *)pszOnlyThis;

            if ((pos = (DWORD)SendMessage(hControl, EM_FINDTEXTEX, 0, (LPARAM)&ft)) != (-1))
            {
                SendMessage(hControl, EM_EXSETSEL, 0, (LPARAM)&ft.chrgText);
                SendMessage(hControl, EM_SETCHARFORMAT, SCF_WORD | SCF_SELECTION, (LPARAM)&cf);
                ft.chrgText.cpMin   = 0;
                ft.chrgText.cpMax   = 0;
                SendMessage(hControl, EM_EXSETSEL, 0, (LPARAM)&ft.chrgText);
            }

            delete[] pszOnlyThis;
        }
        else
        {
            SendMessage(hControl, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
        }
    }

    //
    // Rebase the control
    //

    RebaseControlVertical(
                 hwndDlg,
                 hControl,
                 GetDlgItem(hwndDlg, NextControlId),
                 TRUE,
                 deltavpos,
                 oline,
                 minsep,
                 &deltaheight
                 );

    //
    // If we have the link look then we must subclass for the appropriate
    // link feel, otherwise we subclass for a static text control feel
    //

    if ( fLink == TRUE )
    {
        SubclassEditControlForLink(hwndDlg, hControl, wndproc, plsd);
    }
    else
    {
        SubclassEditControlForArrowCursor(hControl);
    }

    return( deltaheight );
}

//+---------------------------------------------------------------------------
//
//  Function:   CalculateControlVerticalDistance
//
//  Synopsis:   calculates the vertical distance from the bottom of Control1
//              to the top of Control2
//
//  Arguments:  [hwnd]     -- parent dialog
//              [Control1] -- first control
//              [Control2] -- second control
//
//  Returns:    the distance in pixels
//
//  Notes:      assumes control1 is above control2
//
//----------------------------------------------------------------------------
int CalculateControlVerticalDistance (HWND hwnd, UINT Control1, UINT Control2)
{
    RECT rect1;
    RECT rect2;

    GetWindowRect(GetDlgItem(hwnd, Control1), &rect1);
    GetWindowRect(GetDlgItem(hwnd, Control2), &rect2);

    return( rect2.top - rect1.bottom );
}

//+---------------------------------------------------------------------------
//
//  Function:   CalculateControlVerticalDistanceFromDlgBottom
//
//  Synopsis:   calculates the distance from the bottom of the control to
//              the bottom of the dialog
//
//  Arguments:  [hwnd]    -- dialog
//              [Control] -- control
//
//  Returns:    the distance in pixels
//
//  Notes:
//
//----------------------------------------------------------------------------
int CalculateControlVerticalDistanceFromDlgBottom (HWND hwnd, UINT Control)
{
    RECT rect;
    RECT rectControl;

    GetClientRect(hwnd, &rect);
    GetWindowRect(GetDlgItem(hwnd, Control), &rectControl);

    return( rect.bottom - rectControl.bottom );
}

//+---------------------------------------------------------------------------
//
//  Function:   ACUICenterWindow
//
//  Synopsis:   centers the given window
//
//  Arguments:  [hWndToCenter] -- window handle
//
//  Returns:    (none)
//
//  Notes:      This code was stolen from ATL and hacked upon madly :-)
//
//----------------------------------------------------------------------------
VOID ACUICenterWindow (HWND hWndToCenter)
{
    HWND  hWndCenter;

	// determine owner window to center against
	DWORD dwStyle = (DWORD)GetWindowLong(hWndToCenter, GWL_STYLE);

  	if(dwStyle & WS_CHILD)
  		hWndCenter = ::GetParent(hWndToCenter);
  	else
  		hWndCenter = ::GetWindow(hWndToCenter, GW_OWNER);

    if (hWndCenter == NULL)
    {
        return;
    }

	// get coordinates of the window relative to its parent
	RECT rcDlg;
	::GetWindowRect(hWndToCenter, &rcDlg);
	RECT rcArea;
	RECT rcCenter;
	HWND hWndParent;
	if(!(dwStyle & WS_CHILD))
	{
		// don't center against invisible or minimized windows
		if(hWndCenter != NULL)
		{
			DWORD dwStyle = ::GetWindowLong(hWndCenter, GWL_STYLE);
			if(!(dwStyle & WS_VISIBLE) || (dwStyle & WS_MINIMIZE))
				hWndCenter = NULL;
		}

		// center within screen coordinates
		::SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcArea, NULL);

		if(hWndCenter == NULL)
			rcCenter = rcArea;
		else
			::GetWindowRect(hWndCenter, &rcCenter);
	}
	else
	{
		// center within parent client coordinates
		hWndParent = ::GetParent(hWndToCenter);

		::GetClientRect(hWndParent, &rcArea);
		::GetClientRect(hWndCenter, &rcCenter);
		::MapWindowPoints(hWndCenter, hWndParent, (POINT*)&rcCenter, 2);
	}

	int DlgWidth = rcDlg.right - rcDlg.left;
	int DlgHeight = rcDlg.bottom - rcDlg.top;

	// find dialog's upper left based on rcCenter
	int xLeft = (rcCenter.left + rcCenter.right) / 2 - DlgWidth / 2;
	int yTop = (rcCenter.top + rcCenter.bottom) / 2 - DlgHeight / 2;

	// if the dialog is outside the screen, move it inside
	if(xLeft < rcArea.left)
		xLeft = rcArea.left;
	else if(xLeft + DlgWidth > rcArea.right)
		xLeft = rcArea.right - DlgWidth;

	if(yTop < rcArea.top)
		yTop = rcArea.top;
	else if(yTop + DlgHeight > rcArea.bottom)
		yTop = rcArea.bottom - DlgHeight;

	// map screen coordinates to child coordinates
	::SetWindowPos(
         hWndToCenter,
         HWND_TOPMOST,
         xLeft,
         yTop,
         -1,
         -1,
         SWP_NOSIZE | SWP_NOACTIVATE
         );
}

//+---------------------------------------------------------------------------
//
//  Function:   ACUIViewHTMLHelpTopic
//
//  Synopsis:   html help viewer
//
//  Arguments:  [hwnd]     -- caller window
//              [pszTopic] -- topic
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID ACUIViewHTMLHelpTopic (HWND hwnd, LPSTR pszTopic)
{
//    HtmlHelpA(
//        hwnd,
//        "%SYSTEMROOT%\\help\\iexplore.chm>large_context",
//        HH_DISPLAY_TOPIC,
//        (DWORD)pszTopic
//        );
}

//+---------------------------------------------------------------------------
//
//  Function:   GetEditControlMaxLineWidth
//
//  Synopsis:   gets the maximum line width of the edit control
//
//----------------------------------------------------------------------------
int GetEditControlMaxLineWidth (HWND hwndEdit, HDC hdc, int cline)
{
    int        index;
    int        line;
    int        charwidth;
    int        maxwidth = 0;
    CHAR       szMaxBuffer[1024];
    WCHAR      wsz[1024];
    TEXTRANGEA tr;
    SIZE       size;

    tr.lpstrText = szMaxBuffer;

    for ( line = 0; line < cline; line++ )
    {
        index = (int)SendMessage(hwndEdit, EM_LINEINDEX, (WPARAM)line, 0);
        charwidth = (int)SendMessage(hwndEdit, EM_LINELENGTH, (WPARAM)index, 0);

        tr.chrg.cpMin = index;
        tr.chrg.cpMax = index + charwidth;
        SendMessage(hwndEdit, EM_GETTEXTRANGE, 0, (LPARAM)&tr);

        wsz[0] = NULL;

        MultiByteToWideChar(0, 0, (const char *)tr.lpstrText, -1, &wsz[0], 1024);

        if (wsz[0])
        {
            GetTextExtentPoint32W(hdc, &wsz[0], charwidth, &size);

            if ( size.cx > maxwidth )
            {
                maxwidth = size.cx;
            }
        }
    }

    return( maxwidth );
}

//+---------------------------------------------------------------------------
//
//  Function:   DrawFocusRectangle
//
//  Synopsis:   draws the focus rectangle for the edit control
//
//----------------------------------------------------------------------------
void DrawFocusRectangle (HWND hwnd, HDC hdc)
{
    RECT        rect;
    PAINTSTRUCT ps;
    BOOL        fReleaseDC = FALSE;

    if ( hdc == NULL )
    {
        hdc = GetDC(hwnd);
        if ( hdc == NULL )
        {
            return;
        }
        fReleaseDC = TRUE;
    }

    GetClientRect(hwnd, &rect);
    DrawFocusRect(hdc, &rect);

    if ( fReleaseDC == TRUE )
    {
        ReleaseDC(hwnd, hdc);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetHotKeyCharPosition
//
//  Synopsis:   gets the character position for the hotkey, zero means
//              no-hotkey
//
//----------------------------------------------------------------------------
int GetHotKeyCharPosition (HWND hwnd)
{
    WCHAR   szText[MAX_LOADSTRING_BUFFER];
    LPWSTR  psz;
    
    GetWindowTextU(hwnd, szText, MAX_LOADSTRING_BUFFER);

    psz = szText;
    while ( ( psz = wcschr(psz, L'&') ) != NULL )
    {
        psz++;
        if ( *psz != L'&' )
        {
            break;
        }
    }

    if ( psz == NULL )
    {
        return( 0 );
    }

    return (int)(( psz - szText ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   FormatHotKeyOnEditControl
//
//  Synopsis:   formats the hot key on an edit control by making it underlined
//
//----------------------------------------------------------------------------
VOID FormatHotKeyOnEditControl (HWND hwnd, int hkcharpos)
{
    CHARRANGE  cr;
    CHARFORMAT cf;

    assert( hkcharpos != 0 );

    cr.cpMin = hkcharpos - 1;
    cr.cpMax = hkcharpos;

    SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM)&cr);

    memset(&cf, 0, sizeof(CHARFORMAT));
    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_UNDERLINE;
    cf.dwEffects |= CFM_UNDERLINE;

    SendMessage(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    cr.cpMin = -1;
    cr.cpMax = 0;
    SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM)&cr);
}

//+---------------------------------------------------------------------------
//
//  Function:   AdjustEditControlWidthToLineCount
//
//  Synopsis:   adjust edit control width to the given line count
//
//----------------------------------------------------------------------------
void AdjustEditControlWidthToLineCount(HWND hwnd, int cline, TEXTMETRIC* ptm)
{
    RECT rect;
    int  w;
    int  h;

    GetWindowRect(hwnd, &rect);
    h = rect.bottom - rect.top;
    w = rect.right - rect.left;

    while ( cline < SendMessage(hwnd, EM_GETLINECOUNT, 0, 0) )
    {
        w += ptm->tmMaxCharWidth;
        SetWindowPos(hwnd, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
        printf(
            "Line count adjusted to = %d\n",
            (DWORD) SendMessage(hwnd, EM_GETLINECOUNT, 0, 0)
            );
    }
}
