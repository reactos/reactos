template<const DWORD m_dwNumOptions>
PAGEID 
MULTI_OPT_PAGE_DEF<m_dwNumOptions>::
GetNextPage(
    BOOL bCalcNextPage
    )
{
    int nOptIdx;

    // Conditional jumps, and the setting of flags
    switch(m_nPageId) {

    case ASK_ADDITIONAL_SYM_LOC_SYMCPY_PAGEID:
        if (0 == m_nOptDefault) {
            
            return ADDITIONAL_SYM_LOC_SYMCPY_PAGEID;

        } else if (g_CfgData.m_bCopyOS_SymPath 
            || g_CfgData.m_bCopySP_SymPath 
            || g_CfgData.m_bCopyHotFix_SymPath 
            || g_CfgData.m_bCopyAdditional_SymPath) {

            // The user specified symbols, prepare to copy them
            return GET_DEST_DIR_SYMCPY_PAGEID;

        } else {

            // The user didn't specify any symbols. Set the warning text
            TEXT_ONLY_PAGE_DEF * pTextPage = (TEXT_ONLY_PAGE_DEF *) g_rgpPageDefs[GEN_ERROR_PAGEID];
            AssertType(*pTextPage, TEXT_ONLY_PAGE_DEF);

            pTextPage->m_nHeaderSubTitleStrId = IDS_ERROR_YOU_MUST_COPY_SOME_SYMBOLS_HDR_SUB;
            pTextPage->m_nMainTextStrId = IDS_ERROR_YOU_MUST_COPY_SOME_SYMBOLS;

            return GEN_ERROR_PAGEID;
        }
        break;

    case SELECT_HANDDOLD_INI_PAGEID:
        g_CfgData.m_bAdvancedInterface = FALSE;
        if (IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[1]) == BST_CHECKED) {
            // Adv interface
            g_CfgData.m_bAdvancedInterface = TRUE;
        }
        break;

    case DEBUG_APP_PAGEID:
        if (IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED) {
            g_CfgData.m_DebugMode = enumREMOTE_USERMODE;
        } else {
            g_CfgData.m_bRunningOnHost = TRUE;
            g_CfgData.m_DebugMode = enumLOCAL_USERMODE;
        }
        break;

    case RM_USER_HOST_TARGET_CHOICE_PAGEID:
        g_CfgData.m_bRunningOnHost = IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[1]) == BST_CHECKED;
        break;

    case HOST_OR_TARGET_PAGEID:
        g_CfgData.m_bRunningOnHost = IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED;
        break;

    case IS_THIS_KERNEL_MODE_PAGEID:
        if (IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED) {
            g_CfgData.m_DebugMode = enumKERNELMODE;
        }
        break;

    case IS_THIS_AN_APPLICATION_PAGEID:
        if (IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED) {
            g_CfgData.m_bRunningOnHost = TRUE;
            g_CfgData.m_DebugMode = enumLOCAL_USERMODE;
        }
        break;

    case IS_THIS_A_DUMP_PAGEID:
        if (IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED) {
            g_CfgData.m_bRunningOnHost = TRUE;
            g_CfgData.m_DebugMode = enumDUMP;
        }
        break;

    case KERNEL_HOST_TARGET_CHOICE_PAGEID:
        g_CfgData.m_bRunningOnHost = IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED;
        break;
    
    case KERNEL_MACHINE_ROLE_PAGEID:
        g_CfgData.m_bRunningOnHost = IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[1]) == BST_CHECKED;
        break;

    case EXPERT_DEBUGGING_CHOICE_PAGEID:
        for (nOptIdx=0; nOptIdx < m_dwNumOptions; nOptIdx++) {
            if (IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[nOptIdx]) == BST_CHECKED) {
                break;
            }
        }
        
        if (nOptIdx == m_dwNumOptions) {
            nOptIdx = m_nOptDefault;
        }

        // Make sure something was selected
        switch (m_rgnOpts_PageId[nOptIdx]) {
        default:
            Assert(!"Match not found");
            break;

        case USER_EXE_PROCESS_CHOICE_PAGEID:
            g_CfgData.m_bRunningOnHost = TRUE;
            g_CfgData.m_DebugMode = enumLOCAL_USERMODE;
            break;

        case RM_USER_HOST_TARGET_CHOICE_PAGEID:
            g_CfgData.m_DebugMode = enumREMOTE_USERMODE;
            break;

        case KERNEL_HOST_TARGET_CHOICE_PAGEID:
            g_CfgData.m_DebugMode = enumKERNELMODE;
            break;

        case CRASHDUMP_PAGEID:
            g_CfgData.m_bRunningOnHost = TRUE;
            g_CfgData.m_DebugMode = enumDUMP;
            break;
        }
        break;

    }

    //
    // Determine which button is selected and go to that page
    //
    for (nOptIdx=0; nOptIdx < m_dwNumOptions; nOptIdx++) {
        if (IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[nOptIdx]) == BST_CHECKED) {
            return m_rgnOpts_PageId[nOptIdx];
        }
    }

    Assert(!"Error");
    return NULL_PAGEID;
}

template<const DWORD m_dwNumOptions>
MULTI_OPT_PAGE_DEF<m_dwNumOptions>::
MULTI_OPT_PAGE_DEF(
    PAGEID      nPageId,
    int         nDlgRsrcId,
    PAGEID      nMoreInfo_PageId,
    int         nHeaderTitleStrId,
    int         nHeaderSubTitleStrId,

    int         nMainTextStrId,

    int         nOptDefault,

    int         rgnOpt_StrId[],
    UINT        uStrIdSize,
    PAGEID      rgnOpt_PageId[],
    UINT        uPageIdSize
    )
: PAGE_DEF(nPageId, nDlgRsrcId, nMoreInfo_PageId, 
    nHeaderTitleStrId, nHeaderSubTitleStrId)
{
    Assert(nOptDefault < m_dwNumOptions);
    Assert(rgnOpt_StrId);
    Assert(rgnOpt_PageId);
    Assert(sizeof(int) * m_dwNumOptions == uStrIdSize);
    Assert(sizeof(PAGEID) * m_dwNumOptions == uPageIdSize);

    m_nMainTextStrId = nMainTextStrId;

    m_nOptDefault = nOptDefault;
    
    memcpy(m_rgnOpts_StrId, rgnOpt_StrId, sizeof(m_rgnOpts_StrId));
    memcpy(m_rgnOpts_PageId, rgnOpt_PageId, sizeof(m_rgnOpts_PageId));

    // Intentionally designed to fall through
    switch (m_dwNumOptions) {
    case 4:
        m_rgnOpts_CtrlId[3] = IDC_RADIO_OPT_4;

    case 3:
        m_rgnOpts_CtrlId[2] = IDC_RADIO_OPT_3;

    case 2:
        m_rgnOpts_CtrlId[1] = IDC_RADIO_OPT_2;

    case 1:
        m_rgnOpts_CtrlId[0] = IDC_RADIO_OPT_1;
    }
}



template<const DWORD m_dwNumOptions>
int
MULTI_OPT_PAGE_DEF<m_dwNumOptions>::
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
            //
            // If we are on the finish page and in kernel mode and not on the host,
            // we should hide the launch debugger now options.
            //
            // We also need to hide these buttons if a shorcut wan't created.
            //
            if (FINISH_PAGEID == m_nPageId && 
                ((enumKERNELMODE == g_CfgData.m_DebugMode && !g_CfgData.m_bRunningOnHost)
                || NULL == g_CfgData.m_pszDeskTopShortcut_FullPath)
                ) {


                for (int nIdx=0; nIdx < sizeof(m_rgnOpts_CtrlId)/sizeof(int); nIdx++) {
                    HWND hwnd = GetDlgItem(hDlg, m_rgnOpts_CtrlId[nIdx]);
                    ShowWindow(hwnd, SW_HIDE);
                }
            }

            for (int nIdx=0; nIdx < sizeof(m_rgnOpts_CtrlId)/sizeof(int); nIdx++) {

                PSTR psz = WKSP_DynaLoadString(g_hInst, m_rgnOpts_StrId[nIdx]);
                Assert(psz);
                Assert(SetDlgItemText(hDlg, m_rgnOpts_CtrlId[nIdx], psz));
                free(psz);
            }

            Assert(CheckDlgButton(hDlg, m_rgnOpts_CtrlId[m_nOptDefault], BST_CHECKED ));
        }
        // Fall thru

    case UM_DLG_PAGE_SETACTIVE:
        {
            PSTR pszMainText = NULL;
         
            if (HOTFIX_SYM_LOC_SYMCPY_PAGEID == m_nPageId) {
                if (g_CfgData.m_pszHotFixes) {
                    pszMainText = WKSP_DynaLoadStringWithArgs(g_hInst, m_nMainTextStrId,
                        g_CfgData.m_pszHotFixes);
                } else {
                    PSTR psz = WKSP_DynaLoadString(g_hInst, IDS_UNKNOWN);
                    pszMainText = WKSP_DynaLoadStringWithArgs(g_hInst, m_nMainTextStrId, psz);
                    free(psz);
                }
            } else if (SP_SYM_LOC_SYMCPY_PAGEID == m_nPageId) {
                if (g_CfgData.m_dwServicePack) {
                    char sz[20] = {0};

                    _ultoa(g_CfgData.m_dwServicePack, sz, 10);
                    pszMainText = WKSP_DynaLoadStringWithArgs(g_hInst, m_nMainTextStrId, sz);
                } else {
                    PSTR psz = WKSP_DynaLoadString(g_hInst, IDS_UNKNOWN);
                    pszMainText = WKSP_DynaLoadStringWithArgs(g_hInst, m_nMainTextStrId, psz);
                    free(psz);
                } 
            } else {
                pszMainText = WKSP_DynaLoadString(g_hInst, m_nMainTextStrId);
            }

            Assert(pszMainText);
            Assert(SetDlgItemText(hDlg, IDC_STXT_MAIN, pszMainText));
            free(pszMainText);
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {
            case PSN_WIZFINISH:
                {
                    Assert(FINISH_PAGEID == m_nPageId);
                    if (IsDlgButtonChecked(hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED) {
                        if (enumKERNELMODE == g_CfgData.m_DebugMode && !g_CfgData.m_bRunningOnHost) {
                            
                            SpawnMunger();

                        } else {
                            
                            PSTR pszShortcutPath = (PSTR) malloc(strlen(g_CfgData.m_pszDeskTopShortcut_FullPath) +3);
                            strcpy(pszShortcutPath, "\"");
                            strcat(pszShortcutPath, g_CfgData.m_pszDeskTopShortcut_FullPath);
                            strcat(pszShortcutPath, "\"");              
                            
                            if (PtrToUlong(ShellExecute(hDlg, NULL, pszShortcutPath,
                                NULL, NULL, SW_SHOWNORMAL)) <= 32) {
                                
                                PSTR pszLastError = WKSP_FormatLastErrorMessage();
                                WKSP_MsgBox(NULL, IDS_ERROR_LAUNCHING_DEBUGGER, pszLastError);
                                free(pszLastError);
                            }
                            
                            free(pszShortcutPath);
                        }
                    }
                }
                break;

            case PSN_WIZNEXT:
                for (int i=0; i < m_dwNumOptions; i++) {
                    if (IsDlgButtonChecked(hDlg, m_rgnOpts_CtrlId[i]) == BST_CHECKED) {
                        m_nOptDefault = i;
                        break;
                    }
                }
                break;
            }
        }
    }

    return PAGE_DEF::DlgProc(hDlg, uMsg, wParam, lParam);
}


template<const DWORD m_dwNumOptions>
PAGEID 
MULTI_OPT_BROWSE_PATH_PAGE_DEF<m_dwNumOptions>::
GetNextPage(
    BOOL bCalcNextPage
    )
{
    int nOptIdx;

    // Conditional jumps, and the setting of flags
    switch(m_nPageId) {
        
    case OS_SYM_LOC_SYMCPY_PAGEID:
        return GetDriveInfo(bCalcNextPage, g_CfgData.m_pszOS_SymPath,
            g_CfgData.m_pszOS_VolumeLabel_SymPath, g_CfgData.m_dwOS_SerialNum_SymPath, 
            g_CfgData.m_uOS_DriveType_SymPath);

    case SP_SYM_LOC_SYMCPY_PAGEID:
        return GetDriveInfo(bCalcNextPage, g_CfgData.m_pszSP_SymPath,
            g_CfgData.m_pszSP_VolumeLabel_SymPath, g_CfgData.m_dwSP_SerialNum_SymPath, 
            g_CfgData.m_uSP_DriveType_SymPath);
        
    case HOTFIX_SYM_LOC_SYMCPY_PAGEID:
        return GetDriveInfo(bCalcNextPage, g_CfgData.m_pszHotFix_SymPath,
            g_CfgData.m_pszHotFix_VolumeLabel_SymPath, g_CfgData.m_dwHotFix_SerialNum_SymPath, 
            g_CfgData.m_uHotFix_DriveType_SymPath);
        
    case USER_EXE_PROCESS_CHOICE_PAGEID:
        g_CfgData.m_bLaunchNewProcess = IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED;
        
        if (g_CfgData.m_pszExecutableName) {
            free(g_CfgData.m_pszExecutableName);
        }
        g_CfgData.m_pszExecutableName = NT4SafeStrDup(m_pszPath);
        
        if (enumLOCAL_USERMODE == g_CfgData.m_DebugMode) {
            return SYMBOL_FILE_COPY_PAGEID;
        } else {
            if (g_CfgData.m_bRunningOnHost) {
                return SYMBOL_FILE_COPY_PAGEID;
            } else {
                return SAVE_INI_FILE_PAGEID;
            }
        }
        break;
        
    case SAVE_INI_FILE_PAGEID:
        // If they want to enter the info manually, then we need to display summary info
        g_CfgData.m_bDisplaySummaryInfo = IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[1]) == BST_CHECKED;
        if (bCalcNextPage) {
            if (!g_CfgData.m_bDisplaySummaryInfo) {
                // We are actaully moving to the next page and we need to
                // save the ini file
                TARGET_CFG_INFO TargInfo;
                
                if (!TargInfo.Write(m_pszPath)) {
                    // Stay where we are
                    return NULL_PAGEID;
                }
            }

            // What is the next page we are moving to?
            if (!g_CfgData.m_bRunningOnHost && enumKERNELMODE == g_CfgData.m_DebugMode) {
                if (g_CfgData.m_bDisplaySummaryInfo) {
                    return DISPLAY_SUMMARY_INFO_PAGEID;
                } else {
                    return FINISH_PAGEID;
                }
            }
        }
        break;
        
    case SPECIFY_INI_FILE_PAGEID:
        if (!bCalcNextPage) {
            return DET_RUNTIME_PAGEID;
        } else {
            if (IsDlgButtonChecked(m_hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED) {
                if (!(m_pszPath && *m_pszPath)) {
                    return DET_RUNTIME_PAGEID;
                } else {
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
            } else {
                g_CfgData.DoNotUseCfgFile();
                
                if (enumKERNELMODE == g_CfgData.m_DebugMode) {
                    return KERNEL_SELECT_PORT_BAUD_PAGEID;
                } else {
                    return CONNECTION_SELECTION_PAGEID;
                }
            }
        }
        break;
    }

    return MULTI_OPT_PAGE_DEF<m_dwNumOptions>::GetNextPage(bCalcNextPage);
}


template<const DWORD m_dwNumOptions>
void 
MULTI_OPT_BROWSE_PATH_PAGE_DEF<m_dwNumOptions>::
GrayDlgItems()
{
    Assert(m_hDlg);

    if (IDD_TWO_OPT_BROWSE_PATH == m_nDlgRsrcId) {

        m_nOptDefault = 1; 
        if (IsDlgButtonChecked(m_hDlg, IDC_RADIO_OPT_1) == BST_CHECKED) {
            m_nOptDefault = 0; 
        }

        EnableWindow(GetDlgItem(m_hDlg, IDC_EDIT_PATH), 0 == m_nOptDefault);
        EnableWindow(GetDlgItem(m_hDlg, IDC_BUT_BROWSE), 0 == m_nOptDefault);
    }
}


template<const DWORD m_dwNumOptions>
PAGEID 
MULTI_OPT_BROWSE_PATH_PAGE_DEF<m_dwNumOptions>::
GetDriveInfo(
    BOOL bCalcNextPage,
    PSTR & pszPath,
    PSTR & pszVolumeLabel,
    DWORD & dwSerialNumber,
    UINT & uDriveType
    )
{
    if (!bCalcNextPage) {
        return MULTI_OPT_PAGE_DEF<m_dwNumOptions>::GetNextPage(bCalcNextPage);
    }    

    if (0 != m_nOptDefault) {
        // We do not wish to copy symbols
        goto CALC_NEXT_PAGE;
    }

    if (pszPath) {
        free(pszPath);
    }
    pszPath = NT4SafeStrDup(m_pszPath);
    
    if (!GetDiskInfo(m_pszPath, pszVolumeLabel, dwSerialNumber, uDriveType)) {

        // Error getting info for the path specified stay on this page.
        PSTR pszErr = WKSP_FormatLastErrorMessage();
        
        int nRes = WKSP_CustMsgBox(MB_YESNO | MB_ICONINFORMATION | MB_TASKMODAL,
            NULL, IDS_ERROR_GETTING_DRIVE_INFO, pszPath, pszErr);
        
        free(pszErr);

        if (IDYES == nRes) {
            // User wants to continue regardless of the error
            goto CALC_NEXT_PAGE;
        } else {
            // Error detected and user want to stay on the same page
            return NULL_PAGEID;
        }
    }


CALC_NEXT_PAGE:
    switch (m_nPageId) {
    default:
        return MULTI_OPT_PAGE_DEF<m_dwNumOptions>::GetNextPage(bCalcNextPage);
        
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
    }

    // 64 bit compiler workaround
    return NULL_PAGEID;
}

template<const DWORD m_dwNumOptions>
int
MULTI_OPT_BROWSE_PATH_PAGE_DEF<m_dwNumOptions>::
DlgProc(
    IN HWND     hDlg,   
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    )
{
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
            if (!m_pszPath) {
                if (m_nPageId == SAVE_INI_FILE_PAGEID 
                    || m_nPageId == SPECIFY_INI_FILE_PAGEID) {

                    m_pszPath = NT4SafeStrDup("a:\\dbgwiz.inf");

                } else if (m_nPageId == OS_SYM_LOC_SYMCPY_PAGEID) {
                    
                    m_pszPath = NT4SafeStrDup(g_CfgData.m_pszOS_SymPath);

                } else if (m_nPageId == SP_SYM_LOC_SYMCPY_PAGEID
                    || m_nPageId == HOTFIX_SYM_LOC_SYMCPY_PAGEID) {
                    
                    m_pszPath = NT4SafeStrDup(g_CfgData.m_pszSP_SymPath);
                }
            }


            CheckDlgButton(hDlg, IDC_RADIO_OPT_1, 0 == m_nOptDefault ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_RADIO_OPT_2, 1 == m_nOptDefault ? BST_CHECKED : BST_UNCHECKED);            

            HWND hwndPath = GetDlgItem(hDlg, IDC_EDIT_PATH);
            Assert(hwndPath);
            PostMessage(hwndPath, EM_LIMITTEXT, _MAX_PATH -1, 0);

            if (m_pszPath) {
                SetDlgItemText(hDlg, IDC_EDIT_PATH, m_pszPath);
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
                    char sz[_MAX_PATH] = {0};

                    GetDlgItemText(hDlg, IDC_EDIT_PATH, sz, sizeof(sz));

                    if (m_pszPath) {
                        free(m_pszPath);
                    }
                    m_pszPath = NT4SafeStrDup(sz);

                    if (PSN_WIZNEXT == pnmh->code) {
                        if (!m_bDisplayingMoreInfo && IsDlgButtonChecked(hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED
                            && !*sz) {

                            // Stay here
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            WKSP_MsgBox(NULL, IDS_ERROR_NEED_PATH);
                            return TRUE;
                        }
                    } else {
                        // PSN_WIZBACK
                        if (SPECIFY_INI_FILE_PAGEID == m_nPageId) {
                            g_CfgData.DoNotUseCfgFile();
                        }
                    }
                }
                break;
            }
        }
        break;
    }

    BOOL bRet = MULTI_OPT_PAGE_DEF<m_dwNumOptions>::DlgProc(hDlg, uMsg, wParam, lParam);

    // We update this data if we are moving to the next page
    // AND we have data to update
    if (WM_NOTIFY == uMsg && PSN_WIZNEXT == ((LPNMHDR)lParam)->code 
        && -1 != GetWindowLongPtr(hDlg, DWLP_MSGRESULT) && m_pszPath) {

        // Conditional jumps, and the setting of flags
        switch(m_nPageId) {
        case OS_SYM_LOC_SYMCPY_PAGEID:
            if (g_CfgData.m_pszOS_SymPath) {
                free(g_CfgData.m_pszOS_SymPath);
            }
            g_CfgData.m_pszOS_SymPath = NT4SafeStrDup(m_pszPath);
            
            g_CfgData.m_bCopyOS_SymPath = IsDlgButtonChecked(hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED;
            break;            

        case SP_SYM_LOC_SYMCPY_PAGEID:
            if (g_CfgData.m_pszSP_SymPath) {
                free(g_CfgData.m_pszSP_SymPath);
            }
            g_CfgData.m_pszSP_SymPath = NT4SafeStrDup(m_pszPath);

            g_CfgData.m_bCopySP_SymPath = IsDlgButtonChecked(hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED;
            break;            

        case HOTFIX_SYM_LOC_SYMCPY_PAGEID:
            if (g_CfgData.m_pszHotFix_SymPath) {
                free(g_CfgData.m_pszHotFix_SymPath);
            }
            g_CfgData.m_pszHotFix_SymPath = NT4SafeStrDup(m_pszPath);
            
            g_CfgData.m_bCopyHotFix_SymPath = IsDlgButtonChecked(hDlg, m_rgnOpts_CtrlId[0]) == BST_CHECKED;
            break;
        }
    }

    return bRet;       
}

template<const DWORD m_dwNumOptions>
MULTI_OPT_BROWSE_PATH_PAGE_DEF<m_dwNumOptions>::
MULTI_OPT_BROWSE_PATH_PAGE_DEF(
    PAGEID      nPageId,
    int         nDlgRsrcId,
    PAGEID      nMoreInfo_PageId,
    int         nHeaderTitleStrId,
    int         nHeaderSubTitleStrId,
    
    int         nMainTextStrId,

    int         nOptDefault,

    int         rgnOpt_StrId[],
    UINT        uStrIdSize,
    PAGEID      rgnOpt_PageId[],
    UINT        uPageIdSize
    )
: MULTI_OPT_PAGE_DEF<m_dwNumOptions>(nPageId, nDlgRsrcId, 
    nMoreInfo_PageId, nHeaderTitleStrId, nHeaderSubTitleStrId, 
    nMainTextStrId, nOptDefault, rgnOpt_StrId, uStrIdSize, 
    rgnOpt_PageId, uPageIdSize)
{
    m_pszPath = NULL;
}
