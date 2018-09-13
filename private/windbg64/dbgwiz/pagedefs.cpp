#include "precomp.hxx"
#pragma hdrstop

BOOL PAGE_DEF::m_bDisplayingMoreInfo = FALSE;
COMPORT_INFO SELECT_PORT_BAUD_PAGE_DEF::m_rgComPortInfo[100] = {0};
DWORD SELECT_PORT_BAUD_PAGE_DEF::m_dwNumComPortsFound = 0;
DWORD SELECT_PORT_BAUD_PAGE_DEF::m_dwSelectedCommPort = 0;


PAGE_DEF::
PAGE_DEF(
    PAGEID  nPageId,
    int     nDlgRsrcId,
    PAGEID  nMoreInfo_PageId,
    int     nHeaderTitleStrId,
    int     nHeaderSubTitleStrId,
    BOOL    bMoreInfoPage
    )
{
    // If running IE 3, then replace the dlg with the hacked version
    if (g_bRunning_IE_3) {
        switch (nDlgRsrcId) {
            case IDD_ADV_COPY_SYMS:
                nDlgRsrcId = IDD_H_ADV_COPY_SYMS;
                break;

            case IDD_BROWSE_PATH:
                nDlgRsrcId = IDD_H_BROWSE_PATH;
                break;

            case IDD_FOUR_OPT:
                nDlgRsrcId = IDD_H_FOUR_OPT;
                break;

            case IDD_SELECT_PORT:
                nDlgRsrcId = IDD_H_SELECT_PORT;
                break;

            case IDD_SELECT_PORT_BAUD:
                nDlgRsrcId = IDD_H_SELECT_PORT_BAUD;
                break;

            case IDD_SELECT_PORT_BAUD_PIPE:
                nDlgRsrcId = IDD_H_SELECT_PORT_BAUD_PIPE;
                break;

            case IDD_SELECT_PORT_BAUD_PIPE_COMPNAME:
                nDlgRsrcId = IDD_H_SELECT_PORT_BAUD_PIPE_COMPNAME;
                break;

            case IDD_STD_COPY_SYMS:
                nDlgRsrcId = IDD_H_STD_COPY_SYMS;
                break;

            case IDD_SUMMARY:
                nDlgRsrcId = IDD_H_SUMMARY;
                break;

            case IDD_TEXT_ONLY:
                nDlgRsrcId = IDD_H_TEXT_ONLY;
                break;

            case IDD_THREE_OPT:
                nDlgRsrcId = IDD_H_THREE_OPT;
                break;

            case IDD_TWO_OPT:
                nDlgRsrcId = IDD_H_TWO_OPT;
                break;

            case IDD_TWO_OPT_BROWSE_PATH:
                nDlgRsrcId = IDD_H_TWO_OPT_BROWSE_PATH;
                break;

            case IDD_WELCOME:
                nDlgRsrcId = IDD_H_WELCOME;
                break;
        }
    }

    m_hDlg                      = NULL;
    m_nPageId                   = nPageId;
    m_nDlgRsrcId                = nDlgRsrcId;
    m_hpsp                      = NULL;
    m_nMoreInfo_PageId          = nMoreInfo_PageId;
    m_nHeaderTitleStrId         = nHeaderTitleStrId;
    m_nHeaderSubTitleStrId      = nHeaderSubTitleStrId;
    m_bMoreInfoPage             = bMoreInfoPage;
}


PAGEID 
PAGE_DEF::
GetNextPage(
    BOOL bCalcNextPage
    )
{
    if (!bCalcNextPage) {
        Assert(!"Should not happen");
        return NULL_PAGEID;
    } else {
        switch (m_nPageId) {
        default:
            Assert(!"Should not happen");
            return NULL_PAGEID;
            
        case OS_SYM_LOC_SYMCPY_PAGEID:
        case SP_SYM_LOC_SYMCPY_PAGEID:
            // Here we have to figure out where we are and where to go.

            // Can we be certain if the we know the service pack
            // number with any certainty.

            // We cannot be certain if we are trying to install symbols 
            // for the target machine and did not use a cfg file, so we must ask the user.
            if (!g_CfgData.AreWeUsingACfgFile() && 
                (enumREMOTE_USERMODE == g_CfgData.m_DebugMode 
                || enumKERNELMODE == g_CfgData.m_DebugMode 
                || enumDUMP == g_CfgData.m_DebugMode) ) { 

                if (OS_SYM_LOC_SYMCPY_PAGEID == m_nPageId) {
                    return SP_SYM_LOC_SYMCPY_PAGEID;
                } else if (SP_SYM_LOC_SYMCPY_PAGEID == m_nPageId) {
                    return HOTFIX_SYM_LOC_SYMCPY_PAGEID;
                }

                Assert(!"Should not have happened.");
                return NULL_PAGEID;
                  
            } else {
                // The information we have is correct.

                // Do we need to copy Service Pack symbols
                if (OS_SYM_LOC_SYMCPY_PAGEID == m_nPageId
                    && g_CfgData.m_dwServicePack) {

                    return SP_SYM_LOC_SYMCPY_PAGEID;
                }

                // Do we need to copy any HotFix symbols
                if (g_CfgData.m_pszHotFixes && *g_CfgData.m_pszHotFixes) {
                    return HOTFIX_SYM_LOC_SYMCPY_PAGEID;
                }

                // Anything else to copy??
                return ASK_ADDITIONAL_SYM_LOC_SYMCPY_PAGEID;
            }
            break;
        }
    }
}

void 
PAGE_DEF::
CreatePropPage()
{
    PROPSHEETPAGE psp = {0};

    psp.hInstance           = g_hInst;
    psp.pfnDlgProc          = (DLGPROC) WizardStub_DlgProc;
    psp.pszTemplate         = MAKEINTRESOURCE( m_nDlgRsrcId );

    if (g_bRunning_IE_3) {
        psp.dwSize = sizeof(IE_3_PROP_PAGE);
        psp.dwFlags = PSP_DEFAULT;
    } else {
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT;
#if (_WIN32_IE >= 0x0400)
        psp.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle      = MAKEINTRESOURCE( m_nHeaderTitleStrId );
        psp.pszHeaderSubTitle   = MAKEINTRESOURCE( m_nHeaderSubTitleStrId );
#endif
    }

    m_hpsp = CreatePropertySheetPage(&psp);
    Assert(m_hpsp);
}

int
PAGE_DEF::
DlgProc(
    IN HWND     hDlg,   
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    )
{  
    Assert(m_hDlg == hDlg);

    BOOL bStatus = TRUE;

    switch (uMsg) {
    case WM_NCDESTROY:
        m_hDlg = NULL;
        break;

    case WM_INITDIALOG:
        // Fall thru
    case UM_DLG_PAGE_SETACTIVE:
        if (NULL_PAGEID == m_nMoreInfo_PageId) {
            HWND hwndMoreInfoBut = GetDlgItem(hDlg, IDC_BUT_MORE_INFO);
            Assert(hwndMoreInfoBut);
            ShowWindow(hwndMoreInfoBut, SW_HIDE);
        }

        {
            PAGEID pageidNext = GetNextPage(); 
            LPARAM l = 0;

            if (!g_stack.IsEmpty()) {
                l |= PSWIZB_BACK;
            }
            if (NULL_PAGEID != pageidNext || (m_bDisplayingMoreInfo && NULL_PAGEID == pageidNext) ) {
                l |= PSWIZB_NEXT;
            }
            if (THE_END_PAGEID == pageidNext) {
                l |= PSWIZB_FINISH;
            }

            PostMessage(GetParent(hDlg), PSM_SETWIZBUTTONS, 0, l);
        }

        //
        // Special code to handle the case where we are displaying 'More Info'
        //
        if (m_bMoreInfoPage && !m_bDisplayingMoreInfo) {
            // If this is a 'More Info' page, but we are no longer displaying more
            // info, then all of the more info pages must be removed. We accept the activation
            // But we immediately hit the back button.
            PostMessage(GetParent(hDlg), PSM_PRESSBUTTON, PSBTN_BACK, 0);
        }
        break;

    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam); // notification code 
            WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier 
            HWND hwndCtl = (HWND) lParam;      // handle of control 

            if (hwndCtl) {
                switch (wID) {
                case IDC_BUT_MORE_INFO:
                    // This is from the 'More Info" button
                    Assert(FALSE == m_bDisplayingMoreInfo); // More info screen can't have more info
                    m_bDisplayingMoreInfo = TRUE;
                    PostMessage(GetParent(hDlg), PSM_PRESSBUTTON, PSBTN_NEXT, 0);
                    break;

                case IDC_BUT_BROWSE:
                    {
                        PSTR pszInitDir = NULL;
                        PSTR pszFileMask = NULL;
                        char szTmpPath[_MAX_PATH] = {0};

                        switch (m_nPageId) {
                        case USER_EXE_PROCESS_CHOICE_PAGEID:
                            pszFileMask = WKSP_DynaLoadString(g_hInst, IDS_FILE_MASK_EXE);
                            pszInitDir = NT4SafeStrDup("");
                            BrowseForFile(hDlg, pszInitDir, pszFileMask, IDC_EDIT_PATH);
                            break;
    
                        case CRASHDUMP_PAGEID:
                            pszFileMask = WKSP_DynaLoadString(g_hInst, IDS_FILE_MASK_DMP);
                            
                            Assert(GetWindowsDirectory(szTmpPath, sizeof(szTmpPath)));
                            pszInitDir = NT4SafeStrDup(szTmpPath);
                            BrowseForFile(hDlg, pszInitDir, pszFileMask, IDC_EDIT_PATH);
                            break;

                        case OS_SYM_LOC_SYMCPY_PAGEID:
                        case SP_SYM_LOC_SYMCPY_PAGEID:
                        case HOTFIX_SYM_LOC_SYMCPY_PAGEID:
                        case ADDITIONAL_SYM_LOC_SYMCPY_PAGEID:
                        case GET_DEST_DIR_SYMCPY_PAGEID:
                            BrowseForDir(hDlg, IDC_EDIT_PATH);
                            break;

                        default:
                            pszInitDir = NT4SafeStrDup("a:\\");
                            pszFileMask = WKSP_DynaLoadString(g_hInst, IDS_FILE_MASK_INF);
                            if (SAVE_INI_FILE_PAGEID == m_nPageId) {
                                char sz[_MAX_PATH];

                                GetDlgItemText(hDlg, IDC_EDIT_PATH, sz, sizeof(sz));
                                SaveFileDlg(hDlg, sz, sizeof(sz), pszInitDir, pszFileMask, IDC_EDIT_PATH);
                            } else {
                                BrowseForFile(hDlg, pszInitDir, pszFileMask, IDC_EDIT_PATH);
                            }
                        }

                        if (pszInitDir) {
                            free(pszInitDir);
                        }
                        if (pszFileMask) {
                            free(pszFileMask);
                        }

                    }
                    break;
                }
            }
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {
            case PSN_QUERYCANCEL:
                {
                    PSTR pszWarning = WKSP_DynaLoadString(g_hInst, IDS_GEN_WARNING_HDR);
                    BOOL bPreventCancel = IDNO == WKSP_CustMsgBox(
                        MB_ICONHAND | MB_TASKMODAL | MB_YESNO, 
                        pszWarning, IDS_CONFIRM_CANCEL);

                    free(pszWarning);

                    if (!bPreventCancel) {
                        CleanupAfterWizard(hDlg);
                    }

                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, bPreventCancel);
                }
                break;

            case PSN_WIZNEXT:
                {
                    // Really get the next page
                    PAGEID pageidNext = GetNextPage(TRUE);

                    //
                    // Special code to handle the case where we are displaying 'More Info'
                    //
                    if (m_bMoreInfoPage) {
                        Assert(m_bDisplayingMoreInfo);
                    
                        if (NULL_PAGEID == pageidNext) {
                            // End of 'More Info' pages.
                            m_bDisplayingMoreInfo = FALSE;

                            // Do not go forward
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);

                            // Go back
                            PostMessage(GetParent(hDlg), PSM_PRESSBUTTON, PSBTN_BACK, 0);
                        }
                    } else {
                        if (NULL_PAGEID == pageidNext) {
                            // Invalid page, stay here
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        } else if (GOTO_PREV_PAGEID == pageidNext) {
                            // Do not go forward
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            // Go back
                            PostMessage(GetParent(hDlg), PSM_PRESSBUTTON, PSBTN_BACK, 0);
                        } else {
                            extern PAGEID g_nNewPageIdx;

                            Assert(NULL_PAGEID == g_nNewPageIdx);
                            
                            if (!m_bDisplayingMoreInfo) {
                                g_nNewPageIdx = pageidNext;
                            } else {
                                Assert(m_nMoreInfo_PageId);
                                g_nNewPageIdx = m_nMoreInfo_PageId;
                            }
    
                            g_stack.PushEntry(m_nPageId);

                            Assert(FIRST_PAGEID <= g_nNewPageIdx);
                            Assert(g_nNewPageIdx < MAX_NUM_PAGEID);
                            Assert(g_nNewPageIdx == g_rgpPageDefs[g_nNewPageIdx]->m_nPageId);
                            Assert(g_rgpPageDefs[g_nNewPageIdx]->m_hpsp);

                            // Let the prop sheet switch to the next page
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
                            
                            SendMessage(GetParent(hDlg), PSM_ADDPAGE, 0, (LPARAM) g_rgpPageDefs[g_nNewPageIdx]->m_hpsp);
                        }
                    }
                }
                break;            
                
            case PSN_WIZBACK:
                {
                    if (g_stack.IsEmpty()) {
                        // We are at the first page, can't go anywhere
                        Assert(!"Ignorable Error: Back button should have been disabled");
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    } else {
                        // Go ahead and move back
                        PAGEID nIdx = g_stack.PopEntry();
                        Assert(g_rgpPageDefs[nIdx]->m_hDlg);
                        PostMessage(g_rgpPageDefs[nIdx]->m_hDlg, UM_DLG_PAGE_SETACTIVE, 0, 0);

                        if (!g_rgpPageDefs[nIdx]->m_bMoreInfoPage) {
                            m_bDisplayingMoreInfo = FALSE;
                        }

                        PostMessage(GetParent(hDlg), PSM_REMOVEPAGE, -1, (LPARAM) m_hpsp);
                        // The prop sheet destroys that page, we need to create a new one
                        CreatePropPage();

                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);

                    }
                }
                break;

            default:
                bStatus = FALSE;
                break;
            }
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}


int
WELCOME_PAGE_DEF::
DlgProc(
    IN HWND     hDlg,   
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    )
{
    Assert(m_hDlg == hDlg);

    // We don't want the text only functionality
    return PAGE_DEF::DlgProc(hDlg, uMsg, wParam, lParam);
}


WELCOME_PAGE_DEF::
WELCOME_PAGE_DEF(
    PAGEID      nPageId,
    int         nDlgRsrcId,
    PAGEID      nMoreInfo_PageId,
    int         nHeaderTitleStrId,
    int         nHeaderSubTitleStrId,
    int         nTextStrId,
    PAGEID      nNextPageId,
    BOOL        bMoreInfoPage
    )
: TEXT_ONLY_PAGE_DEF(nPageId, nDlgRsrcId, nMoreInfo_PageId, 
    nHeaderTitleStrId, nHeaderSubTitleStrId, 
    nTextStrId, nNextPageId, bMoreInfoPage)
{
}


PAGEID 
TEXT_ONLY_PAGE_DEF::
GetNextPage(
    BOOL bCalcNextPage
    )
{
    if (m_bDisplayingMoreInfo && NULL_PAGEID != m_nMoreInfo_PageId) {
        return m_nMoreInfo_PageId;
    } else {
        return m_nNextPageId;
    }
}

TEXT_ONLY_PAGE_DEF::
TEXT_ONLY_PAGE_DEF(
    PAGEID      nPageId,
    int         nDlgRsrcId,
    PAGEID      nMoreInfo_PageId,
    int         nHeaderTitleStrId,
    int         nHeaderSubTitleStrId,
    int         nTextStrId,
    PAGEID      nNextPageId,
    BOOL        bMoreInfoPage
    )
: PAGE_DEF(nPageId, nDlgRsrcId, nMoreInfo_PageId, 
    nHeaderTitleStrId, nHeaderSubTitleStrId, bMoreInfoPage)
{
    m_nNextPageId = nNextPageId;
    m_nMainTextStrId = nTextStrId;
}

int
TEXT_ONLY_PAGE_DEF::
DlgProc(
    IN HWND     hDlg,   
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    )
{
    int nRes = 0;
    Assert(m_hDlg == hDlg);

    switch (uMsg) {
    case WM_INITDIALOG:
    case UM_DLG_PAGE_SETACTIVE:
        {
            HWND hwndEdit = GetDlgItem(hDlg, IDC_STXT_MAIN);
            Assert(hwndEdit);

            if (WM_INITDIALOG == uMsg) {
                PSTR pszMainText = WKSP_DynaLoadString(g_hInst, m_nMainTextStrId);
                Assert(pszMainText);
                Assert(SetDlgItemText(hDlg, IDC_STXT_MAIN, pszMainText));
                free(pszMainText);
            }
            
            // Remove any text selections
            PostMessage(hwndEdit, EM_SETSEL, -1, 0);
        }
        break;
    }

    return PAGE_DEF::DlgProc(hDlg, uMsg, wParam, lParam);
}

int
DISPLAY_SUMMARY_INFO_PAGE_DEF::
DlgProc(
    IN HWND     hDlg,   
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    )
{
    Assert(m_hDlg == hDlg);

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            DWORD dwServicePack = 0;
            PSTR pszArchitecture = NULL;
            PSTR pszDebugFree = NULL;
            PSTR pszServerOrWorkstation = NULL; // wrkstation or server
            PSTR pszHotFixes = NULL;
            PSTR pszConnection = NULL;
            PSTR pszMainText = NULL;
    
            Assert(enumREMOTE_USERMODE == g_CfgData.m_DebugMode
                || enumKERNELMODE == g_CfgData.m_DebugMode);

            //
            // Get architecture
            if (PROCESSOR_ARCHITECTURE_MIPS == g_CfgData.m_si.wProcessorArchitecture) {
                pszArchitecture = WKSP_DynaLoadString(g_hInst, IDS_OS_DESC_MIPS);
            } else if (PROCESSOR_ARCHITECTURE_INTEL == g_CfgData.m_si.wProcessorArchitecture) {
                pszArchitecture = WKSP_DynaLoadString(g_hInst, IDS_OS_DESC_INTEL);
            } else if (PROCESSOR_ARCHITECTURE_ALPHA == g_CfgData.m_si.wProcessorArchitecture) {
                pszArchitecture = WKSP_DynaLoadString(g_hInst, IDS_OS_DESC_ALPHA);
            } else if (PROCESSOR_ARCHITECTURE_PPC == g_CfgData.m_si.wProcessorArchitecture) {
                pszArchitecture = WKSP_DynaLoadString(g_hInst, IDS_OS_DESC_PPC);
            } else {
                pszArchitecture = WKSP_DynaLoadString(g_hInst, IDS_UNKNOWN);
            }
            Assert(pszArchitecture);

                
            //
            // Get service pack
            sscanf(g_CfgData.m_osi.szCSDVersion, "Service Pack %lu", &dwServicePack);

            //
            // Figure out if it's a workstation, server, enterprise server
            {
                HKEY hkey = NULL;
                char sz[_MAX_PATH] = {0};
                DWORD dwType;
                DWORD dwSize = sizeof(sz);

                Assert(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    "SYSTEM\\CurrentControlSet\\Control\\ProductOptions", 0, KEY_READ, &hkey));

                Assert(ERROR_SUCCESS == RegQueryValueEx(hkey, "ProductType", NULL, &dwType,
                    (PUCHAR) sz, &dwSize));

                if (!_stricmp(sz, "WinNT")) {
                    
                    pszServerOrWorkstation = WKSP_DynaLoadString(g_hInst, IDS_OS_DESC_WORKSTATION);
                
                } else if (!_stricmp(sz, "ServerNT")) {
                    
                    // Its a server, but is the enterprise version.
                    dwSize = sizeof(sz);

                    if (ERROR_SUCCESS == RegQueryValueEx(hkey, "ProductSuite", NULL, &dwType,
                        (PUCHAR) sz, &dwSize)) {
            
                        Assert(REG_MULTI_SZ == dwType);

                        if (!_stricmp(sz, "Enterprise")) {
                            pszServerOrWorkstation = WKSP_DynaLoadString(g_hInst, IDS_OS_DESC_ENTERPRISE);
                        } else {
                            pszServerOrWorkstation = WKSP_DynaLoadString(g_hInst, IDS_UNKNOWN);
                        }
                    } else {
                        pszServerOrWorkstation = WKSP_DynaLoadString(g_hInst, IDS_OS_DESC_SERVER);
                    }
                } else {
                    pszServerOrWorkstation = WKSP_DynaLoadString(g_hInst, IDS_UNKNOWN);
                }

                Assert(ERROR_SUCCESS == RegCloseKey(hkey));
            }

            //
            // Get HotFixes
            if (g_CfgData.m_pszHotFixes) {
                pszHotFixes = NT4SafeStrDup(g_CfgData.m_pszHotFixes);
            } else {
                pszHotFixes = WKSP_DynaLoadString(g_hInst, IDS_NONE);
            }

            //
            // Get OS description
            PSTR pszOs;       
            {
                char szServicePack[20] = {0};
                
                if (dwServicePack) {
                    _ultoa(dwServicePack, szServicePack, 10);
                } else {
                    PSTR pszNone = WKSP_DynaLoadString(g_hInst, IDS_NONE);
                    strcpy(szServicePack, pszNone);
                    free(pszNone);
                }
                
                pszOs = WKSP_DynaLoadStringWithArgs(g_hInst, IDS_OS_SUMMARY,
                    g_CfgData.m_osi.dwMajorVersion, g_CfgData.m_osi.dwMinorVersion, g_CfgData.m_osi.dwBuildNumber, 
                    pszArchitecture, pszServerOrWorkstation, g_CfgData.m_pszFreeCheckedBuild, 
                    szServicePack, pszHotFixes);
            }

            //
            // Build connection string
            if (enumKERNELMODE == g_CfgData.m_DebugMode) {
                Assert(g_CfgData.m_bSerial);

                PSTR pszSerial = WKSP_DynaLoadStringWithArgs(g_hInst, IDS_SERIAL_CONNECTION, 
                    BaudRateBitFlagToText(g_CfgData.m_dwBaudRate));

                pszConnection = WKSP_DynaLoadStringWithArgs(g_hInst, IDS_KERNEL_SUMMARY,
                    pszSerial);

                free(pszSerial);
            } else {
                if (g_CfgData.m_bSerial) {
                    PSTR pszSerial = WKSP_DynaLoadStringWithArgs(g_hInst, IDS_SERIAL_CONNECTION, 
                        BaudRateBitFlagToText(g_CfgData.m_dwBaudRate));

                    pszSerial = WKSP_DynaLoadStringWithArgs(g_hInst, IDS_REMOTE_USER_SUMMARY,
                        pszSerial);

                    free(pszSerial);
                } else {
                    Assert(g_CfgData.m_pszCompName);
                    Assert(g_CfgData.m_pszPipeName);

                    PSTR pszNetwork = WKSP_DynaLoadStringWithArgs(g_hInst, IDS_NETWORK_CONNECTION, 
                        g_CfgData.m_pszCompName, g_CfgData.m_pszPipeName);

                    pszConnection = WKSP_DynaLoadStringWithArgs(g_hInst, IDS_REMOTE_USER_SUMMARY,
                        pszNetwork);

                    free(pszNetwork);
                }
            }

            //
            // Get Main text
            pszMainText = WKSP_DynaLoadStringWithArgs(g_hInst, m_nMainTextStrId,
                pszConnection, pszOs);
            Assert(pszMainText);

            //
            // Display the text
            Assert(SetDlgItemText(hDlg, IDC_EDIT_MAIN, pszMainText));

            free(pszOs);
            free(pszArchitecture);
            free(pszDebugFree);
            free(pszServerOrWorkstation);
            free(pszHotFixes);
            free(pszConnection);
            free(pszMainText);
        }
        // Fall thru

    case UM_DLG_PAGE_SETACTIVE:
        //
        // Make so that the text is not currently selected
        {
            HWND hwnd = GetDlgItem(hDlg, IDC_EDIT_MAIN);
            Assert(hwnd);
            PostMessage(hwnd, EM_SETSEL, (WPARAM) (int) -1, 0);
        }

        // Skip over TEXT_ONLY, because it will try to reload the main
        // text
        return PAGE_DEF::DlgProc(hDlg, uMsg, wParam, lParam);
    }

    return TEXT_ONLY_PAGE_DEF::DlgProc(hDlg, uMsg, wParam, lParam);
}

DISPLAY_SUMMARY_INFO_PAGE_DEF::
DISPLAY_SUMMARY_INFO_PAGE_DEF(
    PAGEID      nPageId,
    int         nDlgRsrcId,
    PAGEID      nMoreInfo_PageId,
    int         nHeaderTitleStrId,
    int         nHeaderSubTitleStrId,
    int         nTextStrId,
    PAGEID      nNextPageId,
    BOOL        bMoreInfoPage
    )
: TEXT_ONLY_PAGE_DEF(nPageId, nDlgRsrcId, nMoreInfo_PageId, 
    nHeaderTitleStrId, nHeaderSubTitleStrId, 
    nTextStrId, nNextPageId, bMoreInfoPage)
{
}

PAGEID 
SELECT_PORT_BAUD_PAGE_DEF::
GetNextPage(
    BOOL bCalcNextPage
    )
{
    switch (m_nPageId) {
    case KERNEL_SELECT_PORT_BAUD_PAGEID:
        if (g_CfgData.m_bRunningOnHost) {
            m_nNextPageId = SYMBOL_FILE_COPY_PAGEID;
        } else {
            m_nNextPageId = SAVE_INI_FILE_PAGEID;
        }
        break;
    }

    return TEXT_ONLY_PAGE_DEF::GetNextPage(bCalcNextPage);
}


SELECT_PORT_BAUD_PAGE_DEF::
SELECT_PORT_BAUD_PAGE_DEF(
    PAGEID      nPageId,
    int         nDlgRsrcId,
    PAGEID      nMoreInfo_PageId,
    int         nHeaderTitleStrId,
    int         nHeaderSubTitleStrId,
    int         nTextStrId,
    PAGEID      nNextPageId,
    BOOL        bMoreInfoPage
    )
: TEXT_ONLY_PAGE_DEF(nPageId, nDlgRsrcId, nMoreInfo_PageId, 
    nHeaderTitleStrId, nHeaderSubTitleStrId, 
    nTextStrId, nNextPageId, bMoreInfoPage)
{
    static BOOL bInitStaticData = TRUE;

    if (bInitStaticData) {
        bInitStaticData = FALSE;
        EnumComPorts(m_rgComPortInfo, sizeof(m_rgComPortInfo) / sizeof(COMPORT_INFO), m_dwNumComPortsFound);
    }

    m_dwSelectedCommPort = 0;
}


int
SELECT_PORT_BAUD_PAGE_DEF::
DlgProc(
    IN HWND     hDlg,   
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    )
{
    Assert(m_hDlg == hDlg);

    HWND hwndPort = GetDlgItem(hDlg, IDC_COMBO_COMM_PORT);
    PSTR pszSeparator = "      ";

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            g_CfgData.m_bSerial = TRUE;

            BOOL bIncludeBaud = m_nDlgRsrcId != IDD_SELECT_PORT;
    
#define ADD_STR_TO_CMBO(bit)                                                    \
    if (m_rgComPortInfo[dw].dwSettableBaud & bit) {                             \
        strcpy(sz, m_rgComPortInfo[dw].szSymName);                              \
        if (bIncludeBaud) {                                                     \
            strcat(sz, pszSeparator);                                           \
            strcat(sz, BaudRateBitFlagToText(bit));                             \
        }                                                                       \
                                                                                \
        LRESULT lRes = SendMessage(hwndPort, CB_ADDSTRING, 0, (LPARAM) sz);     \
        Assert(CB_ERRSPACE != lRes && CB_ERR != lRes);                          \
        SendMessage(hwndPort, CB_SETITEMDATA, lRes, MAKELPARAM(dw, bit));       \
    }                                           

            char sz[256];
            DWORD dw;
            for (dw=0; dw<m_dwNumComPortsFound; dw++) {       
                if (!bIncludeBaud) {                    
                    LRESULT lRes = SendMessage(hwndPort, CB_ADDSTRING, 
                        0, (LPARAM) m_rgComPortInfo[dw].szSymName);
                    Assert(CB_ERRSPACE != lRes && CB_ERR != lRes);
                    SendMessage(hwndPort, CB_SETITEMDATA, lRes, MAKELPARAM(dw, 0));
                } else {
                    ADD_STR_TO_CMBO(BAUD_9600);
                    ADD_STR_TO_CMBO(BAUD_14400);
                    ADD_STR_TO_CMBO(BAUD_19200);
                    ADD_STR_TO_CMBO(BAUD_38400);
                }
            }

#undef ADD_STR_TO_CMBO

            // Select the previously selected item or just select the
            // first one.
            if (0 == m_dwSelectedCommPort) {
                // Just grab the first one
                SendMessage(hwndPort, CB_SETCURSEL, 0, 0);
            } else {
                dw = LOWORD(m_dwSelectedCommPort);
                strcpy(sz, m_rgComPortInfo[dw].szSymName);
                if (bIncludeBaud) {                    
                    strcat(sz, pszSeparator);          
                    strcat(sz, BaudRateBitFlagToText(HIWORD(m_dwSelectedCommPort)));
                }                                      
                LRESULT lCurSel = SendMessage(hwndPort, CB_FINDSTRINGEXACT, -1, (LPARAM) sz);
                Assert(CB_ERR != lCurSel);
                SendMessage(hwndPort, CB_SETCURSEL, lCurSel, 0);
            }
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {
            case PSN_WIZNEXT:
            case PSN_WIZBACK:
                {
                    LRESULT lCurSel = SendMessage(hwndPort, CB_GETCURSEL, 0, 0);
                    Assert(CB_ERR != lCurSel);

                    m_dwSelectedCommPort = (ULONG)SendMessage(hwndPort, CB_GETITEMDATA, lCurSel, 0);
                    Assert(CB_ERR != m_dwSelectedCommPort);

                    g_CfgData.m_dwBaudRate = HIWORD(m_dwSelectedCommPort);

                    if (g_CfgData.m_pszCommPortName) {
                        free(g_CfgData.m_pszCommPortName);
                    }
                    g_CfgData.m_pszCommPortName = NT4SafeStrDup(m_rgComPortInfo[LOWORD(m_dwSelectedCommPort)].szSymName);
                }
                break;
            }
        }
        break;
    }

    return TEXT_ONLY_PAGE_DEF::DlgProc(hDlg, uMsg, wParam, lParam);
}


void 
SELECT_PORT_BAUD_PIPE_COMPNAME_PAGE_DEF::
GrayDlgItems()
{
    Assert(m_hDlg);

    m_bSerial = IsDlgButtonChecked(m_hDlg, IDC_RADIO_SERIAL) == BST_CHECKED;
    

    EnableWindow(GetDlgItem(m_hDlg, IDC_EDIT_MACHINE_NAME), !m_bSerial);
    EnableWindow(GetDlgItem(m_hDlg, IDC_STXT_MACHINE_NAME), !m_bSerial);
    EnableWindow(GetDlgItem(m_hDlg, IDC_EDIT_PIPE_NAME), !m_bSerial);
    EnableWindow(GetDlgItem(m_hDlg, IDC_STXT_PIPE_NAME), !m_bSerial);
    EnableWindow(GetDlgItem(m_hDlg, IDC_STXT_COMM_PORT), m_bSerial);
    EnableWindow(GetDlgItem(m_hDlg, IDC_COMBO_COMM_PORT), m_bSerial);
}

SELECT_PORT_BAUD_PIPE_COMPNAME_PAGE_DEF::
SELECT_PORT_BAUD_PIPE_COMPNAME_PAGE_DEF(
    PAGEID      nPageId,
    int         nDlgRsrcId,
    PAGEID      nMoreInfo_PageId,
    int         nHeaderTitleStrId,
    int         nHeaderSubTitleStrId,
    int         nTextStrId,
    PAGEID      nNextPageId,
    BOOL        bMoreInfoPage
    )
: SELECT_PORT_BAUD_PAGE_DEF(nPageId, nDlgRsrcId, nMoreInfo_PageId, 
    nHeaderTitleStrId, nHeaderSubTitleStrId, 
    nTextStrId, nNextPageId, bMoreInfoPage)
{
    char sz[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD dwSize = sizeof(sz);
    Assert(GetComputerName(sz, &dwSize));

    m_bSerial = FALSE;
    m_pszCompName = NT4SafeStrDup(sz);
    m_pszPipeName = NT4SafeStrDup("windbg");
}



int
SELECT_PORT_BAUD_PIPE_COMPNAME_PAGE_DEF::
DlgProc(
    IN HWND     hDlg,   
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    )
{
    Assert(m_hDlg == hDlg);

    // These may be present
    HWND hwndCompNameEdit = GetDlgItem(hDlg, IDC_EDIT_MACHINE_NAME);
    HWND hwndCompNameStxt = GetDlgItem(hDlg, IDC_STXT_MACHINE_NAME);

    // these should always be present
    HWND hwndPipeEdit = GetDlgItem(hDlg, IDC_EDIT_PIPE_NAME);
    HWND hwndPipeStxt = GetDlgItem(hDlg, IDC_STXT_PIPE_NAME);

    switch (uMsg) {
    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam); // notification code 
            WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier 
            HWND hwndCtl = (HWND) lParam;      // handle of control 

            if (hwndCtl) {
                switch (wNotifyCode) {
                case BN_CLICKED:
                    GrayDlgItems();
                    break;
                }
            }
        }
        break;

    case WM_INITDIALOG:
        {
            Assert(CheckDlgButton(hDlg, IDC_RADIO_NETWORK, !m_bSerial ? BST_CHECKED : BST_UNCHECKED));
            Assert(CheckDlgButton(hDlg, IDC_RADIO_SERIAL, m_bSerial ? BST_CHECKED : BST_UNCHECKED));

            if (hwndCompNameEdit) {
                SendMessage(hwndCompNameEdit, EM_LIMITTEXT, MAX_COMPUTERNAME_LENGTH, 0);

                if (m_pszCompName) {
                    SetWindowText(hwndCompNameEdit, m_pszCompName);
                }
            }

            // See docs (CreateNamedPipe) for max pipe name len
            SendMessage(hwndPipeEdit, EM_LIMITTEXT, 256 - strlen("\\\\.\\pipe\\"), 0);

            if (m_pszPipeName) {
                SetWindowText(hwndPipeEdit, m_pszPipeName);
            }

            GrayDlgItems();
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {
            case PSN_WIZNEXT:
            case PSN_WIZBACK:
                {
                    char sz[1024] = {0};

                    // Get the comp name if present
                    if (hwndCompNameEdit) {
                        GetWindowText(hwndCompNameEdit, sz, sizeof(sz));       
                        if (m_pszCompName) {
                            free(m_pszCompName);
                        }
                        m_pszCompName = NT4SafeStrDup(sz);
                    }

                    // Get the pipe name
                    GetWindowText(hwndPipeEdit, sz, sizeof(sz));
                    if (m_pszPipeName) {
                        free(m_pszPipeName);
                    }
                    m_pszPipeName = NT4SafeStrDup(sz);

                    if (!m_bDisplayingMoreInfo &&!m_bSerial && PSN_WIZNEXT == pnmh->code) {

                        if (!*m_pszPipeName) {
                            // Stay here
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            WKSP_MsgBox(NULL, IDS_ERROR_NEED_PIPE_NAME);
                            return TRUE;
                        }

                        if (hwndCompNameEdit && !*m_pszCompName) {
                            // Stay here
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            WKSP_MsgBox(NULL, IDS_ERROR_NEED_COMP_NAME);
                            return TRUE;
                        }
                    }                   
                    

                    //
                    // Copy the data out to the global data
                    g_CfgData.m_bSerial = m_bSerial;
                    
                    // Copy serial data

                    // Copy Net data
                    if (g_CfgData.m_pszCompName) {
                        free(g_CfgData.m_pszCompName);
                    }
                    g_CfgData.m_pszCompName = NT4SafeStrDup(m_pszCompName);

                    if (g_CfgData.m_pszPipeName) {
                        free(g_CfgData.m_pszPipeName);
                    }
                    g_CfgData.m_pszPipeName = NT4SafeStrDup(m_pszPipeName);
                }
                break;
            }
        }
        break;
    }

    return SELECT_PORT_BAUD_PAGE_DEF::DlgProc(hDlg, uMsg, wParam, lParam);
}


HELP_TEXT_ONLY_PAGE_DEF::
HELP_TEXT_ONLY_PAGE_DEF(
    PAGEID      nPageId,
    int         nHeaderTitleStrId,
    int         nHeaderSubTitleStrId,
    int         nTextStrId,
    PAGEID      nNextPageId
    )
: TEXT_ONLY_PAGE_DEF(nPageId, IDD_TEXT_ONLY, NULL_PAGEID, 
    nHeaderTitleStrId, nHeaderSubTitleStrId, nTextStrId, nNextPageId, TRUE)
{
}


BROWSE_PATH_PAGE_DEF::
BROWSE_PATH_PAGE_DEF(
    PAGEID      nPageId,
    int         nDlgRsrcId,
    PAGEID      nMoreInfo_PageId,
    int         nHeaderTitleStrId,
    int         nHeaderSubTitleStrId,
    int         nTextStrId,
    PAGEID      nNextPageId
    )
: TEXT_ONLY_PAGE_DEF(nPageId, nDlgRsrcId, 
    nMoreInfo_PageId, nHeaderTitleStrId, nHeaderSubTitleStrId,
    nTextStrId, nNextPageId)
{
    m_pszPath = NULL;
}


PAGEID 
BROWSE_PATH_PAGE_DEF::
GetNextPage(
    BOOL bCalcNextPage
    )
{
    if (!bCalcNextPage) {
        return m_nNextPageId;
    } else {
        switch (m_nPageId) {
        default:
            return m_nNextPageId;

        case GET_DEST_DIR_SYMCPY_PAGEID:
            {
                // Make sure that the user is not trying to perform a
                // recursive copy.
                PSTR pszSrc;
                BOOL bRecursive = FALSE;
                char szDest[_MAX_PATH] = {0};

                GetDlgItemText(m_hDlg, IDC_EDIT_PATH, szDest, sizeof(szDest));

                if (!bRecursive && g_CfgData.m_bCopyOS_SymPath) {
                    pszSrc = g_CfgData.m_pszOS_SymPath;
                    if (!_strnicmp(szDest, pszSrc, strlen(pszSrc) ) ) {
                        // Recursive copy
                        bRecursive = TRUE;
                    }
                }

                if (!bRecursive && g_CfgData.m_bCopySP_SymPath) {
                    pszSrc = g_CfgData.m_pszSP_SymPath;
                    if (!_strnicmp(szDest, pszSrc, strlen(pszSrc) ) ) {
                        // Recursive copy
                        bRecursive = TRUE;
                    }
                }

                if (!bRecursive && g_CfgData.m_bCopyHotFix_SymPath) {
                    pszSrc = g_CfgData.m_pszHotFix_SymPath;
                    if (!_strnicmp(szDest, pszSrc, strlen(pszSrc) ) ) {
                        // Recursive copy
                        bRecursive = TRUE;
                    }
                }

                if (!bRecursive && g_CfgData.m_bCopyAdditional_SymPath) {
                        TListEntry<PSTR> * pSymPathEntry = g_CfgData.m_list_pszAdditional_SymPath.FirstEntry();

                        for (; pSymPathEntry != g_CfgData.m_list_pszAdditional_SymPath.Stop(); 
                            pSymPathEntry = pSymPathEntry->Flink) {
        
                            pszSrc = pSymPathEntry->m_tData;
                            if (!_strnicmp(szDest, pszSrc, strlen(pszSrc) ) ) {
                                // Recursive copy
                                bRecursive = TRUE;
                            }
                        }
                }

                if (!bRecursive) {
                    // None of the copies are recursive
                    return m_nNextPageId;
                } else {
                    // User is trying to recursively copy symbol files
                    WKSP_MsgBox(NULL, IDS_ERROR_SYM_COPY_IS_RECURSIVE,
                        szDest, pszSrc);
                    return NULL_PAGEID;
                }
            }
            break;

        case TARGET_CONFIG_FILE_LOCATION_PAGEID:
            {
                // Load ini file. Decide where to go from there
                TARGET_CFG_INFO TargInfo;
                
                if (TargInfo.Read(m_pszPath)) {
                    g_CfgData.UseCfgFile();

                    if (enumREMOTE_USERMODE == g_CfgData.m_DebugMode) {
                        if (g_CfgData.m_bSerial) {
                            return SELECT_PORT_PAGEID;
                        } else {
                            return SYMBOL_FILE_COPY_PAGEID;
                        }
                    } else if (enumKERNELMODE == g_CfgData.m_DebugMode) {
                        return SELECT_PORT_PAGEID;
                    } else {
                        Assert(!"Bad file not caught.");
                        g_CfgData.DoNotUseCfgFile();
                        return NULL_PAGEID;
                    }
                } else {
                    g_CfgData.DoNotUseCfgFile();
                    return NULL_PAGEID;
                }
            }
            break;
        }
    }
}

int
BROWSE_PATH_PAGE_DEF::
DlgProc(
    IN HWND     hDlg,   
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    )
{
    Assert(m_hDlg == hDlg);

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            if (!m_pszPath) {
                if (TARGET_CONFIG_FILE_LOCATION_PAGEID == m_nPageId) {
                    m_pszPath = NT4SafeStrDup("a:\\dbgwiz.inf");
                } else if (m_nPageId == OS_SYM_LOC_SYMCPY_PAGEID) {
                    m_pszPath = NT4SafeStrDup(g_CfgData.m_pszOS_SymPath);
                } else if (m_nPageId == ADDITIONAL_SYM_LOC_SYMCPY_PAGEID) {
                    m_pszPath = NT4SafeStrDup("a:\\");
                } else if (m_nPageId == CRASHDUMP_PAGEID) {
                    m_pszPath = NT4SafeStrDup(g_CfgData.m_pszDumpFileName);
                }
            }

            HWND hwndPath = GetDlgItem(hDlg, IDC_EDIT_PATH);
            Assert(hwndPath);
            PostMessage(hwndPath, EM_LIMITTEXT, _MAX_PATH -1, 0);

            if (m_pszPath) {
                SetDlgItemText(hDlg, IDC_EDIT_PATH, m_pszPath);
            }
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {
            case PSN_WIZNEXT:
            case PSN_WIZBACK:
                {
                    char sz[_MAX_PATH] = {0};

                    GetDlgItemText(hDlg, IDC_EDIT_PATH, sz, sizeof(sz));

                    if (m_pszPath) {
                        free(m_pszPath);
                    }
                    m_pszPath = NT4SafeStrDup(sz);

                    if (!m_bDisplayingMoreInfo && !*sz && PSN_WIZNEXT == pnmh->code) {
                        // Stay here
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        WKSP_MsgBox(NULL, IDS_ERROR_NEED_PATH);
                        return TRUE;
                    }                        
                }
                break;
            }
        }
        break;
    }

    BOOL bRet = TEXT_ONLY_PAGE_DEF::DlgProc(hDlg, uMsg, wParam, lParam);

    // We update this data if we are moving to the next page
    // AND we have data to update
    if (WM_NOTIFY == uMsg && PSN_WIZNEXT == ((LPNMHDR)lParam)->code 
        && m_pszPath) {

        if (-1 == GetWindowLongPtr(hDlg, DWLP_MSGRESULT)) {
            if (ADDITIONAL_SYM_LOC_SYMCPY_PAGEID == m_nPageId) {
                UINT uSize;
                DWORD dwSerial;
                UINT uDriveType;
                PSTR pszTmp = NULL;
                
                g_CfgData.m_bCopyAdditional_SymPath = TRUE;
                
                g_CfgData.m_list_pszAdditional_SymPath.InsertTail(m_pszPath);
                
                //
                // Get Disk info
                GetDiskInfo(m_pszPath, pszTmp, dwSerial, uDriveType);
                
                g_CfgData.m_list_pszAdditional_VolumeLabel_SymPath.InsertTail(pszTmp);                
                g_CfgData.m_list_dwAdditional_SerialNum_SymPath.InsertTail(dwSerial);
                g_CfgData.m_list_uAdditional_DriveType_SymPath.InsertTail(uDriveType);
            }
        } else {            
            // Conditional jumps, and the setting of flags
            switch(m_nPageId) {
            case CRASHDUMP_PAGEID:
                if (g_CfgData.m_pszDumpFileName) {
                    free(g_CfgData.m_pszDumpFileName);
                }
                g_CfgData.m_pszDumpFileName = NT4SafeStrDup(m_pszPath);
                break;
                
            case SHORTCUT_NAME_PAGEID:
                // Cleanup old stuff
                DeleteLinkOnDesktop(hDlg, g_CfgData.m_pszDeskTopShortcut_FullPath);
                if (g_CfgData.m_pszShortCutName) {
                    free(g_CfgData.m_pszShortCutName);
                }
                
                g_CfgData.m_pszShortCutName = NT4SafeStrDup(m_pszPath);
                g_CfgData.m_pszDeskTopShortcut_FullPath = CreateLinkOnDesktop(hDlg, m_pszPath, TRUE);
                break;
                
            case GET_DEST_DIR_SYMCPY_PAGEID:
                if (g_CfgData.m_pszDestinationPath) {
                    free(g_CfgData.m_pszDestinationPath);
                }
                g_CfgData.m_pszDestinationPath = NT4SafeStrDup(m_pszPath);
                break;
            }
        }
    }

    return bRet;
}

PAGEID 
DESKTOP_SHORTCUT_PAGE_DEF::
GetNextPage(
    BOOL bCalcNextPage
    )
{
    if (g_CfgData.m_bDisplaySummaryInfo && !g_CfgData.m_bRunningOnHost) {
        return DISPLAY_SUMMARY_INFO_PAGEID;
    } else {
        return FINISH_PAGEID;
    }
}

DESKTOP_SHORTCUT_PAGE_DEF::
DESKTOP_SHORTCUT_PAGE_DEF(
    PAGEID      nPageId,
    int         nDlgRsrcId,
    PAGEID      nMoreInfo_PageId,
    int         nHeaderTitleStrId,
    int         nHeaderSubTitleStrId,
    int         nTextStrId,
    PAGEID      nNextPageId
    )
: BROWSE_PATH_PAGE_DEF(nPageId, nDlgRsrcId, 
    nMoreInfo_PageId, nHeaderTitleStrId, nHeaderSubTitleStrId,
    nTextStrId, nNextPageId)
{
}

int
DESKTOP_SHORTCUT_PAGE_DEF::
DlgProc(
    IN HWND     hDlg,   
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    )
{
    Assert(m_hDlg == hDlg);

    switch (uMsg) {
    case WM_INITDIALOG:
        ShowWindow(GetDlgItem(hDlg, IDC_BUT_BROWSE), SW_HIDE);
        break;
    }

    return BROWSE_PATH_PAGE_DEF::DlgProc(hDlg, uMsg, wParam, lParam);
}


COPY_SYMS_PAGE_DEF::
COPY_SYMS_PAGE_DEF(
    PAGEID      nPageId,
    int         nDlgRsrcId,
    PAGEID      nMoreInfo_PageId,
    int         nHeaderTitleStrId,
    int         nHeaderSubTitleStrId,
    int         nTextStrId,
    PAGEID      nNextPageId
    )
: TEXT_ONLY_PAGE_DEF(nPageId, nDlgRsrcId,
    nMoreInfo_PageId, nHeaderTitleStrId, nHeaderSubTitleStrId,
    nTextStrId, nNextPageId)
{
    ZeroMemory(m_rgpszMsgs, sizeof(m_rgpszMsgs));
}

COPY_SYMS_PAGE_DEF::
~COPY_SYMS_PAGE_DEF()
{
    for (int i=0; i<nMAX_NUM_MSGS; i++) {
        if (m_rgpszMsgs[i]) {
            free(m_rgpszMsgs[i]);
            m_rgpszMsgs[i] = NULL;
        }
    }
}

BOOL
COPY_SYMS_PAGE_DEF::
Copy(
    HWND hwnd,
    PSTR pszDest,
    PSTR pszSrc,
    PSTR pszVolumeName,
    DWORD dwSerialNumber,
    UINT uDriveType
    )
{
    BOOL bSuccess = FALSE;
    PSTR pszVol_Tmp = NULL;
    DWORD dwSerial_Tmp = 0;
    UINT uDriveType_Tmp = 0;
    COPY_SYMBOLS_DATA_STRUCT CopySyms;
    BOOL bWrongDisk = FALSE;

    do {
        GetDiskInfo(pszSrc, pszVol_Tmp, dwSerial_Tmp, uDriveType_Tmp);
        
        bWrongDisk = dwSerial_Tmp != dwSerialNumber || uDriveType_Tmp != uDriveType
            || NULL == pszVolumeName || NULL == pszVol_Tmp 
            || strcmp(pszVolumeName, pszVol_Tmp);

        if (bWrongDisk) {
            if (IDCANCEL == WKSP_CustMsgBox(
                MB_RETRYCANCEL | MB_ICONSTOP | MB_TASKMODAL,
                NULL, IDS_ERROR_INCORRECT_PATH, pszVolumeName)) {

                goto CLEANUP;
            }
        }

    } while (bWrongDisk);


    if (DRIVE_FIXED == uDriveType) {
        if (!AreAnyOfTheFilesCompressed(TRUE, pszSrc)) {
            // None of the files are compressed and were are referring to
            // a fixed disk.
            PSTR pszInfo = WKSP_DynaLoadString(g_hInst, IDS_INFORMATIONAL);
            WKSP_MsgBox(pszInfo, IDS_WARN_NON_REMOVABLE_MEDIA_ADD_PATH, pszSrc);
            free(pszInfo);
            g_SymPaths.InsertTail(NT4SafeStrDup(pszSrc));
            goto CLEANUP;
        }
    } else if (DRIVE_REMOTE == uDriveType) {
        if (!AreAnyOfTheFilesCompressed(TRUE, pszSrc)) {
            // None of the files are compressed 
            PSTR pszInfo = WKSP_DynaLoadString(g_hInst, IDS_INFORMATIONAL);
            if (IDYES == WKSP_CustMsgBox(MB_YESNO | MB_ICONINFORMATION | MB_TASKMODAL,
                pszInfo, IDS_WARN_WILL_NETWORK_CONNECTION_BE_AVAILABLE, pszSrc)) {

                WKSP_MsgBox(pszInfo, IDS_WARN_FILES_LINKED_INSTEAD_OF_COPIED);
                    
                free(pszInfo);
                g_SymPaths.InsertTail(NT4SafeStrDup(pszSrc));
                goto CLEANUP;
            }
            free(pszInfo);
        }
    } 


    //
    // Copy the files    
    strcpy(CopySyms.m_szSrcPath, pszSrc);
    strcpy(CopySyms.m_szDestPath, pszDest);                            
    
    
    bSuccess = (IDOK == DialogBoxParam(g_hInst, 
        MAKEINTRESOURCE(IDD_COPYING), hwnd, 
        Copying_DlgProc, (LPARAM) &CopySyms));

    if (bSuccess) {        
        g_SymPaths.InsertTail(NT4SafeStrDup(pszDest));
    }


CLEANUP:
    if (pszVol_Tmp) {
        free(pszVol_Tmp);
    }

    return bSuccess;
}


int
COPY_SYMS_PAGE_DEF::
DlgProc(
    IN HWND     hDlg,   
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    )
{
    Assert(m_hDlg == hDlg);

    switch (uMsg) {
    case WM_INITDIALOG:
        PostMessage(hDlg, UM_START_STD_MODE_COPYING, 0, 0);
        break;

    case UM_START_STD_MODE_COPYING:
        {
            BOOL bContinueCopying = TRUE;
            int nIdx;
            int nCurrentlyCopying;
            char sz[1024 * 5];

            ZeroMemory(m_rgpszMsgs, sizeof(m_rgpszMsgs));

            if (g_CfgData.m_bCopyOS_SymPath) {
                m_rgpszMsgs[0] = WKSP_DynaLoadString(g_hInst, IDS_COPYING_OS);
            }

            if (g_CfgData.m_bCopySP_SymPath) {
                m_rgpszMsgs[1] = WKSP_DynaLoadString(g_hInst, IDS_COPYING_SP);
            }

            if (g_CfgData.m_bCopyHotFix_SymPath) {
                m_rgpszMsgs[2] = WKSP_DynaLoadString(g_hInst, IDS_COPYING_HOTFIXES);
            }

            if (g_CfgData.m_bCopyAdditional_SymPath) {
                m_rgpszMsgs[3] = WKSP_DynaLoadString(g_hInst, IDS_COPYING_ADDITIONAL_SYMBOLS);
            }     

            for (nCurrentlyCopying = 0; bContinueCopying && nCurrentlyCopying<nMAX_NUM_MSGS; 
                nCurrentlyCopying++) {

                if (!m_rgpszMsgs[nCurrentlyCopying]) {
                    continue;
                }

                *sz = NULL;
                for (nIdx=0; nIdx<nMAX_NUM_MSGS; nIdx++) {
                    if (m_rgpszMsgs[nIdx]) {
                        if (nCurrentlyCopying == nIdx) {
                            strcat(sz, "\r\n Copying -> ");
                        } else {
                            strcat(sz, "\r\n            ");
                        }
                        strcat(sz, m_rgpszMsgs[nIdx]);
                    }
                }
                SetDlgItemText(hDlg, IDC_STATIC_COPY_STATUS, sz);


                //
                // If the drive is removable we copy the symbols
                // If it a network, we ask if it will be available during
                //  the debug session.
                // If it is a hard disk simply link to it.

                

                //
                // Prompt the user to insert the correct CD
                //
                PSTR pszVolumeName = NULL;
                DWORD dwSerial = 0;
                UINT uDriveType = 0;

                switch(nCurrentlyCopying) {
                case 0:
                    // OS syms
                    bContinueCopying = Copy(hDlg,
                        g_CfgData.m_pszDestinationPath, 
                        g_CfgData.m_pszOS_SymPath, 
                        g_CfgData.m_pszOS_VolumeLabel_SymPath,
                        g_CfgData.m_dwOS_SerialNum_SymPath, 
                        g_CfgData.m_uOS_DriveType_SymPath);
                    break;

                case 1:
                    // SP syms
                    bContinueCopying = Copy(hDlg,
                        g_CfgData.m_pszDestinationPath, 
                        g_CfgData.m_pszSP_SymPath, 
                        g_CfgData.m_pszSP_VolumeLabel_SymPath,
                        g_CfgData.m_dwSP_SerialNum_SymPath, 
                        g_CfgData.m_uSP_DriveType_SymPath);
                    break;

                case 2:
                    // Hotfix syms
                    bContinueCopying = Copy(hDlg,
                        g_CfgData.m_pszDestinationPath, 
                        g_CfgData.m_pszHotFix_SymPath, 
                        g_CfgData.m_pszHotFix_VolumeLabel_SymPath,
                        g_CfgData.m_dwHotFix_SerialNum_SymPath, 
                        g_CfgData.m_uHotFix_DriveType_SymPath);
                    break;

                case 3:
                    // Additional syms
                    {
                        Assert(g_CfgData.m_list_pszAdditional_SymPath.Size()
                               == g_CfgData.m_list_pszAdditional_VolumeLabel_SymPath.Size());
                        Assert(g_CfgData.m_list_dwAdditional_SerialNum_SymPath.Size()
                               == g_CfgData.m_list_pszAdditional_VolumeLabel_SymPath.Size());
                        Assert(g_CfgData.m_list_dwAdditional_SerialNum_SymPath.Size()
                               == g_CfgData.m_list_uAdditional_DriveType_SymPath.Size());

                        
                        TListEntry<PSTR> * pSymPathEntry = g_CfgData.m_list_pszAdditional_SymPath.FirstEntry();
                        TListEntry<PSTR> * pVolLblEntry = g_CfgData.m_list_pszAdditional_VolumeLabel_SymPath.FirstEntry();
                        TListEntry<DWORD> * pSerialNumEntry = g_CfgData.m_list_dwAdditional_SerialNum_SymPath.FirstEntry();
                        TListEntry<UINT> * pDriveTypeEntry = g_CfgData.m_list_uAdditional_DriveType_SymPath.FirstEntry();

                        for (; bContinueCopying && pSymPathEntry != g_CfgData.m_list_pszAdditional_SymPath.Stop(); 
                            pSymPathEntry = pSymPathEntry->Flink, 
                            pVolLblEntry = pVolLblEntry->Flink,
                            pSerialNumEntry = pSerialNumEntry->Flink, 
                            pDriveTypeEntry = pDriveTypeEntry->Flink) {
        
                            bContinueCopying = Copy(hDlg,
                                g_CfgData.m_pszDestinationPath, 
                                pSymPathEntry->m_tData, 
                                pVolLblEntry->m_tData,
                                pSerialNumEntry->m_tData, 
                                pDriveTypeEntry->m_tData);
                        }
                    }
                    break;
                }
            }
             
            for (nIdx=0; nIdx<nMAX_NUM_MSGS; nIdx++) {
                if (m_rgpszMsgs[nIdx]) {
                    free(m_rgpszMsgs[nIdx]);
                    m_rgpszMsgs[nIdx] = NULL;
                }
            }
        }
        return TRUE;
    }

    return TEXT_ONLY_PAGE_DEF::DlgProc(hDlg, uMsg, wParam, lParam);
}

void 
ADV_COPY_SYMS_PAGE_DEF::
GrayDlgItems()
{
    Assert(m_hDlg);

    m_bCopySymbols = IsDlgButtonChecked(m_hDlg, IDC_RADIO_COPY_SYMBOLS) == BST_CHECKED;
    EnableWindow(GetDlgItem(m_hDlg, IDC_STXT_DEST), m_bCopySymbols);
    EnableWindow(GetDlgItem(m_hDlg, IDC_EDIT_DEST), m_bCopySymbols);
    EnableWindow(GetDlgItem(m_hDlg, IDC_BUT_BROWSE_DEST), m_bCopySymbols);
}

int 
ADV_COPY_SYMS_PAGE_DEF::
DlgProc(
    IN HWND     hDlg,   
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    )
{
    Assert(m_hDlg == hDlg);

    switch (uMsg) {
    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam); // notification code 
            WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier 
            HWND hwndCtl = (HWND) lParam;      // handle of control 

            if (hwndCtl && BN_CLICKED == wNotifyCode) {
                // A radio button or the copy button was clicked
                GrayDlgItems();

                switch (wID) {
                case IDC_BUT_BROWSE_SRC:
                    BrowseForDir(hDlg, IDC_EDIT_SRC);
                    break;

                case IDC_BUT_BROWSE_DEST:
                    BrowseForDir(hDlg, IDC_EDIT_DEST);
                    break;

                case IDC_BUT_COPY:
                    {
                        // The copy button was clicked.
                        char szSrc[_MAX_PATH] = {0};
                        char szDest[_MAX_PATH] = {0};
                        
                        GetDlgItemText(hDlg, IDC_EDIT_SRC, szSrc, sizeof(szSrc));
                        GetDlgItemText(hDlg, IDC_EDIT_DEST, szDest, sizeof(szDest));
                        
                        if (!*szSrc) {
                            WKSP_MsgBox(NULL, IDS_ERROR_NEED_SRC_PATH);
                            break;
                        }
                        
                        if (m_bCopySymbols && !*szDest) {
                            WKSP_MsgBox(NULL, IDS_ERROR_NEED_DEST_PATH);
                            break;
                        }
                        
                        if (m_bCopySymbols) {
                            COPY_SYMBOLS_DATA_STRUCT CopySyms;
            
                            strcpy(CopySyms.m_szSrcPath, szSrc);
                            strcpy(CopySyms.m_szDestPath, szDest);                            

                            if (IDOK == DialogBoxParam(g_hInst, 
                                MAKEINTRESOURCE(IDD_COPYING), hDlg, 
                                Copying_DlgProc, (LPARAM) &CopySyms)) {

                                g_SymPaths.InsertTail(NT4SafeStrDup(szDest));
                            }
                        } else {
                            g_SymPaths.InsertTail(NT4SafeStrDup(szSrc));
                        }
                    }
                    break;
                }
            }
        }
        break;

    case WM_INITDIALOG:
        CheckDlgButton(hDlg, IDC_RADIO_LINK_SYMBOLS, m_bCopySymbols ? BST_UNCHECKED : BST_CHECKED);
        CheckDlgButton(hDlg, IDC_RADIO_COPY_SYMBOLS, m_bCopySymbols ? BST_CHECKED : BST_UNCHECKED);

        SetDlgItemText(hDlg, IDC_EDIT_SRC, g_CfgData.m_pszOS_SymPath);

        SendDlgItemMessage(hDlg, IDC_EDIT_SRC, EM_LIMITTEXT, _MAX_PATH -1, 0);
        SendDlgItemMessage(hDlg, IDC_EDIT_DEST, EM_LIMITTEXT, _MAX_PATH -1, 0);
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {
            case PSN_WIZNEXT:
                if (!m_bDisplayingMoreInfo && 0 == g_SymPaths.Size()) {
                    // Proceed without any symbols
                    PSTR pszWarn = WKSP_DynaLoadString(g_hInst, IDS_WARN_ADV_NO_SYMBOLS_SPECIFIED);
                    if (IDNO == WKSP_CustMsgBox(MB_ICONINFORMATION | MB_TASKMODAL | MB_YESNO, 
                        pszWarn, IDS_WARN_ADV_NO_SYMBOLS_SPECIFIED)) {

                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }
                    free(pszWarn);
                }
            case PSN_WIZBACK:
                GrayDlgItems();
                break;
            }
        }
        break;
    }

    return PAGE_DEF::DlgProc(hDlg, uMsg, wParam, lParam);
}

PAGEID 
ADV_COPY_SYMS_PAGE_DEF::
GetNextPage(
    BOOL bCalcNextPage
    )
{
    return m_nNextPageId;
}

ADV_COPY_SYMS_PAGE_DEF::
ADV_COPY_SYMS_PAGE_DEF(
    PAGEID      nPageId,
    int         nDlgRsrcId,
    PAGEID      nMoreInfo_PageId,
    int         nHeaderTitleStrId,
    int         nHeaderSubTitleStrId,
    PAGEID      nNextPageId
    )
: PAGE_DEF(nPageId, nDlgRsrcId, nMoreInfo_PageId, 
    nHeaderTitleStrId, nHeaderSubTitleStrId, FALSE)
{
    m_nNextPageId = nNextPageId;
    m_bCopySymbols = FALSE;
}
