#include "precomp.hxx"
#pragma hdrstop


BOOL
InitPageDefs()
{
    //
    // 0
    // LEEFI
    //
    g_rgpPageDefs[WELCOME_PAGEID] = new WELCOME_PAGE_DEF(
        WELCOME_PAGEID, // Page ID
        IDD_WELCOME, // DLG resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_WELCOME_HDR, // String resource ID
        IDS_WELCOME_HDR_SUB, // String resource ID
        IDS_WELCOME, // String resource ID
        SELECT_HANDDOLD_INI_PAGEID // Goto this page on NEXT
        );

    //
    // Y_1
    // SELECT_HANDDOLD_INI_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 3;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_01_SELECT_HANDDOLD_INI, // string ID for opt 1
            IDS_OPT_02_SELECT_HANDDOLD_INI, // string ID for opt 2
            IDS_OPT_03_SELECT_HANDDOLD_INI // string ID for opt 3
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            IS_THIS_KERNEL_MODE_PAGEID, // Option 1, goto this page
            EXPERT_DEBUGGING_CHOICE_PAGEID, // Option 2, goto this page
            TARGET_CONFIG_FILE_LOCATION_PAGEID // Option 3, goto this page
        };

        g_rgpPageDefs[SELECT_HANDDOLD_INI_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            SELECT_HANDDOLD_INI_PAGEID, // Page ID
            IDD_THREE_OPT, // Dlg resource ID
            NULL_PAGEID, // ID of 'more info' page
            IDS_SELECT_HANDDOLD_INI_HDR, // String resource ID
            IDS_SELECT_HANDDOLD_INI_HDR_SUB, // String resource ID
            IDS_SELECT_HANDDOLD_INI, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // Y_2
    // EXPERT_DEBUGGING_CHOICE_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 4;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_01_EXPERT_DEBUGGING_CHOICE, // string ID for opt 1
            IDS_OPT_02_EXPERT_DEBUGGING_CHOICE, // string ID for opt 2
            IDS_OPT_03_EXPERT_DEBUGGING_CHOICE, // string ID for opt 3
            IDS_OPT_04_EXPERT_DEBUGGING_CHOICE // string ID for opt 4
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            USER_EXE_PROCESS_CHOICE_PAGEID, // Option 1, goto this page
            RM_USER_HOST_TARGET_CHOICE_PAGEID, // Option 2, goto this page
            KERNEL_HOST_TARGET_CHOICE_PAGEID, // Option 3, goto this page
            CRASHDUMP_PAGEID // Option 4, goto this page
        };

        g_rgpPageDefs[EXPERT_DEBUGGING_CHOICE_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            EXPERT_DEBUGGING_CHOICE_PAGEID, // Page ID
            IDD_FOUR_OPT, // Dlg resource ID
            NULL_PAGEID, // ID of 'more info' page
            IDS_EXPERT_DEBUGGING_CHOICE_HDR, // String resource ID
            IDS_EXPERT_DEBUGGING_CHOICE_HDR_SUB, // String resource ID
            IDS_EXPERT_DEBUGGING_CHOICE, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // Y_3
    // KERNEL_HOST_TARGET_CHOICE_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_HOST, // string ID for opt 1
            IDS_TARGET // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            RUN_WIZARD_ON_TARGET_PAGEID, // Option 1, goto this page
            KERNEL_SELECT_PORT_BAUD_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[KERNEL_HOST_TARGET_CHOICE_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            KERNEL_HOST_TARGET_CHOICE_PAGEID, // Page ID
            IDD_TWO_OPT, // Dlg resource ID
            NULL_PAGEID, // ID of 'more info' page
            IDS_KERNEL_HOST_TARGET_CHOICE_HDR, // String resource ID
            IDS_KERNEL_HOST_TARGET_CHOICE_HDR_SUB, // String resource ID
            IDS_KERNEL_HOST_TARGET_CHOICE, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // Y_4
    // KERNEL_SELECT_PORT_BAUD_PAGEID
    //
    g_rgpPageDefs[KERNEL_SELECT_PORT_BAUD_PAGEID] = new SELECT_PORT_BAUD_PAGE_DEF(
        KERNEL_SELECT_PORT_BAUD_PAGEID, // Page ID
        IDD_SELECT_PORT_BAUD, // DLG resource ID
        MI_01_NULL_MODEM_CABLE_PAGEID, // ID of 'more info' page
        IDS_KERNEL_SELECT_PORT_BAUD_HDR, // String resource ID
        IDS_KERNEL_SELECT_PORT_BAUD_HDR_SUB, // String resource ID
        IDS_KERNEL_SELECT_PORT_BAUD, // String resource ID
        DET_RUNTIME_PAGEID // Goto this page on NEXT
        );

    //
    // Y_5
    // SAVE_INI_FILE_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_01_SAVE_INI_FILE, // string ID for opt 1
            IDS_OPT_02_SAVE_INI_FILE // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            SHORTCUT_NAME_PAGEID,
            SHORTCUT_NAME_PAGEID
            //FINISH_PAGEID, // Option 1, goto this page
            //DISPLAY_SUMMARY_INFO_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[SAVE_INI_FILE_PAGEID] = new MULTI_OPT_BROWSE_PATH_PAGE_DEF<dwMAX_OPTIONS>(
            SAVE_INI_FILE_PAGEID, // Page ID
            IDD_TWO_OPT_BROWSE_PATH, // Dlg resource ID
            MI_01_SAVE_LOAD_INI_PAGEID, // ID of 'more info' page
            IDS_SAVE_INI_FILE_HDR, // String resource ID
            IDS_SAVE_INI_FILE_HDR_SUB, // String resource ID
            IDS_SAVE_INI_FILE, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // Y_6
    // DISPLAY_SUMMARY_INFO_PAGEID
    //
    g_rgpPageDefs[DISPLAY_SUMMARY_INFO_PAGEID] = new DISPLAY_SUMMARY_INFO_PAGE_DEF(
        DISPLAY_SUMMARY_INFO_PAGEID, // Page ID
        IDD_SUMMARY, // DLG resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_DISPLAY_SUMMARY_INFO_HDR, // String resource ID
        IDS_DISPLAY_SUMMARY_INFO_HDR_SUB, // String resource ID
        IDS_DISPLAY_SUMMARY_INFO, // String resource ID
        FINISH_PAGEID // Goto this page on NEXT
        );

    //
    // Y_7
    // FINISH_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_FINISH_LAUNCH_DEBUGGER, // string ID for opt 1
            IDS_FINISH_DO_NOT_LAUNCH_DEBUGGER // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            THE_END_PAGEID, // Option 1, goto this page
            THE_END_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[FINISH_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            FINISH_PAGEID, // Page ID
            IDD_TWO_OPT, // Dlg resource ID
            NULL_PAGEID, // ID of 'more info' page
            IDS_FINISH_HDR, // String resource ID
            IDS_FINISH_HDR_SUB, // String resource ID
            IDS_FINISH, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // Y_12
    // RM_USER_HOST_TARGET_CHOICE_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_TARGET, // string ID for opt 1
            IDS_HOST // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            REMOTE_CONNECTION_TYPE_PAGEID, // Option 1, goto this page
            RUN_WIZARD_ON_TARGET_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[RM_USER_HOST_TARGET_CHOICE_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            RM_USER_HOST_TARGET_CHOICE_PAGEID, // Page ID
            IDD_TWO_OPT, // Dlg resource ID
            NULL_PAGEID, // ID of 'more info' page
            IDS_RM_USER_HOST_TARGET_CHOICE_HDR, // String resource ID
            IDS_RM_USER_HOST_TARGET_CHOICE_HDR_SUB, // String resource ID
            IDS_RM_USER_HOST_TARGET_CHOICE, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // Y_14
    // USER_EXE_PROCESS_CHOICE_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_01_USER_EXE_PROCESS_CHOICE, // string ID for opt 1
            IDS_OPT_02_USER_EXE_PROCESS_CHOICE // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            DET_RUNTIME_PAGEID, // Option 1, goto this page
            DET_RUNTIME_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[USER_EXE_PROCESS_CHOICE_PAGEID] = new MULTI_OPT_BROWSE_PATH_PAGE_DEF<dwMAX_OPTIONS>(
            USER_EXE_PROCESS_CHOICE_PAGEID, // Page ID
            IDD_TWO_OPT_BROWSE_PATH, // Dlg resource ID
            MI_01_LAUNCH_ATTACH_PAGEID, // ID of 'more info' page
            IDS_USER_EXE_PROCESS_CHOICE_HDR, // String resource ID
            IDS_USER_EXE_PROCESS_CHOICE_HDR_SUB, // String resource ID
            IDS_USER_EXE_PROCESS_CHOICE, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // ADV_SYMBOL_FILE_COPY_PAGEID
    //
    g_rgpPageDefs[ADV_SYMBOL_FILE_COPY_PAGEID] = new ADV_COPY_SYMS_PAGE_DEF(
        ADV_SYMBOL_FILE_COPY_PAGEID, // Page ID
        IDD_ADV_COPY_SYMS, // Dlg resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_ADV_SYMBOL_FILE_COPY_HDR, // String resource ID
        IDS_ADV_SYMBOL_FILE_COPY_HDR_SUB, // String resource ID
        SHORTCUT_NAME_PAGEID //  Next page
        );


    //
    // Z_30 & Y_8
    // RUN_WIZARD_ON_TARGET_PAGEID
    //
    g_rgpPageDefs[RUN_WIZARD_ON_TARGET_PAGEID] = new TEXT_ONLY_PAGE_DEF(
        RUN_WIZARD_ON_TARGET_PAGEID, // Page ID
        IDD_TEXT_ONLY, // DLG resource ID
        MI_01_WHY_RUN_ON_TARGET_FIRST_PAGEID, // ID of 'more info' page
        IDS_RUN_WIZARD_ON_TARGET_HDR, // String resource ID
        IDS_RUN_WIZARD_ON_TARGET_HDR_SUB, // String resource ID
        IDS_RUN_WIZARD_ON_TARGET, // String resource ID
        SPECIFY_INI_FILE_PAGEID // Goto this page on NEXT
        );

    //
    // Z_31
    // SPECIFY_INI_FILE_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_01_SPECIFY_INI_FILE, // string ID for opt 1
            IDS_OPT_02_SPECIFY_INI_FILE // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            DET_RUNTIME_PAGEID, // Option 1, goto this page
            DET_RUNTIME_PAGEID
            //ADV_SYM_CPY_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[SPECIFY_INI_FILE_PAGEID] = new MULTI_OPT_BROWSE_PATH_PAGE_DEF<dwMAX_OPTIONS>(
            SPECIFY_INI_FILE_PAGEID, // Page ID
            IDD_TWO_OPT_BROWSE_PATH, // Dlg resource ID
            MI_01_SAVE_LOAD_INI_PAGEID, // ID of 'more info' page
            IDS_SPECIFY_INI_FILE_HDR, // String resource ID
            IDS_SPECIFY_INI_FILE_HDR_SUB, // String resource ID
            IDS_SPECIFY_INI_FILE, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // Z_33
    // CONNECTION_SELECTION_PAGEID
    //
    g_rgpPageDefs[CONNECTION_SELECTION_PAGEID] = new SELECT_PORT_BAUD_PIPE_COMPNAME_PAGE_DEF(
        CONNECTION_SELECTION_PAGEID, // Page ID
        IDD_SELECT_PORT_BAUD_PIPE_COMPNAME, // DLG resource ID
        MI_01_NULL_MODEM_CABLE_PAGEID, // ID of 'more info' page
        IDS_CONNECTION_SELECTION_HDR, // String resource ID
        IDS_CONNECTION_SELECTION_HDR_SUB, // String resource ID
        IDS_CONNECTION_SELECTION, // String resource ID
        USER_EXE_PROCESS_CHOICE_PAGEID // Goto this page on NEXT
        );

    //
    // *_2
    // TARGET_CONFIG_FILE_LOCATION_PAGEID
    //
    g_rgpPageDefs[TARGET_CONFIG_FILE_LOCATION_PAGEID] = new BROWSE_PATH_PAGE_DEF(
        TARGET_CONFIG_FILE_LOCATION_PAGEID, // Page ID
        IDD_BROWSE_PATH, // Dlg resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_TARGET_CONFIG_FILE_LOCATION_HDR, // String resource ID
        IDS_TARGET_CONFIG_FILE_LOCATION_HDR_SUB, // String resource ID
        IDS_TARGET_CONFIG_FILE_LOCATION, // Text for the dlg
        DET_RUNTIME_PAGEID //  Next page
        );

    //
    // (Y_10)
    // SELECT_PORT_PAGEID
    //
    g_rgpPageDefs[SELECT_PORT_PAGEID] = new SELECT_PORT_BAUD_PAGE_DEF(
        SELECT_PORT_PAGEID, // Page ID
        IDD_SELECT_PORT, // DLG resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_SELECT_COM_PORT_HDR, // String resource ID
        IDS_SELECT_COM_PORT_HDR_SUB, // String resource ID
        IDS_SELECT_COM_PORT, // String resource ID
        SYMBOL_FILE_COPY_PAGEID // Goto this page on NEXT
        );

    //
    // (Y_19)
    // SHORTCUT_NAME_PAGEID
    //
    g_rgpPageDefs[SHORTCUT_NAME_PAGEID] = new DESKTOP_SHORTCUT_PAGE_DEF(
        SHORTCUT_NAME_PAGEID, // Page ID
        IDD_BROWSE_PATH, // Dlg resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_SHORTCUT_NAME_HDR, // String resource ID
        IDS_SHORTCUT_NAME_HDR_SUB, // String resource ID
        IDS_SHORTCUT_NAME, // Text for the dlg
        DET_RUNTIME_PAGEID //  Next page
        );

    //
    // (B_1)
    // IS_THIS_KERNEL_MODE_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_YES, // string ID for opt 1
            IDS_NO // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            KERNEL_MACHINE_ROLE_PAGEID, // Option 1, goto this page
            IS_THIS_AN_APPLICATION_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[IS_THIS_KERNEL_MODE_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            IS_THIS_KERNEL_MODE_PAGEID, // Page ID
            IDD_TWO_OPT, // Dlg resource ID
            MI_01_BLUE_SCREEN_PAGEID, // ID of 'more info' page
            IDS_IS_THIS_KERNEL_MODE_HDR, // String resource ID
            IDS_IS_THIS_KERNEL_MODE_HDR_SUB, // String resource ID
            IDS_IS_THIS_KERNEL_MODE, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // SELECT_PORT_BAUD_PAGEID
    //
    g_rgpPageDefs[SELECT_PORT_BAUD_PAGEID] = new SELECT_PORT_BAUD_PAGE_DEF(
        SELECT_PORT_BAUD_PAGEID, // Page ID
        IDD_SELECT_PORT_BAUD_PIPE, // DLG resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_SELECT_PORT_BAUD_HDR, // String resource ID
        IDS_SELECT_PORT_BAUD_HDR_SUB, // String resource ID
        IDS_SELECT_PORT_BAUD, // String resource ID
        DET_RUNTIME_PAGEID // Goto this page on NEXT
        );

    //
    // (B_2)
    // KERNEL_MACHINE_ROLE_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_1_KERNEL_MACHINE_ROLE, // string ID for opt 1
            IDS_OPT_2_KERNEL_MACHINE_ROLE // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            KERNEL_SELECT_PORT_BAUD_PAGEID, // Option 1, goto this page
            IS_TARGET_RESPONSIVE_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[KERNEL_MACHINE_ROLE_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            KERNEL_MACHINE_ROLE_PAGEID, // Page ID
            IDD_TWO_OPT, // Dlg resource ID
            MI_01_HOST_TARGET_PAGEID, // ID of 'more info' page
            IDS_KERNEL_MACHINE_ROLE_HDR, // String resource ID
            IDS_KERNEL_MACHINE_ROLE_HDR_SUB, // String resource ID
            IDS_KERNEL_MACHINE_ROLE, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // (B_4)
    // IS_TARGET_RESPONSIVE_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_1_IS_TARGET_RESPONSIVE, // string ID for opt 1
            IDS_OPT_2_IS_TARGET_RESPONSIVE // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            RUN_WIZARD_ON_TARGET_PAGEID, // Option 1, goto this page
            TOAST_MESSAGE_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[IS_TARGET_RESPONSIVE_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            IS_TARGET_RESPONSIVE_PAGEID, // Page ID
            IDD_TWO_OPT, // Dlg resource ID
            MI_01_IS_TARGET_RESPONSIVE_PAGEID, // ID of 'more info' page
            IDS_IS_TARGET_RESPONSIVE_HDR, // String resource ID
            IDS_IS_TARGET_RESPONSIVE_HDR_SUB, // String resource ID
            IDS_IS_TARGET_RESPONSIVE, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // TOAST_MESSAGE_PAGEID
    //
    g_rgpPageDefs[TOAST_MESSAGE_PAGEID] = new TEXT_ONLY_PAGE_DEF(
        TOAST_MESSAGE_PAGEID, // Page ID
        IDD_TEXT_ONLY, // DLG resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_TOAST_MESSAGE_HDR, // String resource ID
        IDS_TOAST_MESSAGE_HDR_SUB, // String resource ID
        IDS_TOAST_MESSAGE, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );

    //
    // (B_9)
    // IS_THIS_AN_APPLICATION_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_YES, // string ID for opt 1
            IDS_NO // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            DEBUG_APP_PAGEID, // Option 1, goto this page
            IS_THIS_A_DUMP_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[IS_THIS_AN_APPLICATION_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            IS_THIS_AN_APPLICATION_PAGEID, // Page ID
            IDD_TWO_OPT, // Dlg resource ID
            MI_01_APPLICATION_PAGEID, // ID of 'more info' page
            IDS_IS_THIS_AN_APPLICATION_HDR, // String resource ID
            IDS_IS_THIS_AN_APPLICATION_HDR_SUB, // String resource ID
            IDS_IS_THIS_AN_APPLICATION, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // (B_9_1)
    // IS_THIS_A_DUMP_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_YES, // string ID for opt 1
            IDS_NO // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            CRASHDUMP_PAGEID, // Option 1, goto this page
            TOAST_UNKNOWN_DEBUGGEE_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[IS_THIS_A_DUMP_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            IS_THIS_A_DUMP_PAGEID, // Page ID
            IDD_TWO_OPT, // Dlg resource ID
            MI_01_DUMP_FILE_PAGEID, // ID of 'more info' page
            IDS_IS_THIS_A_DUMP_HDR, // String resource ID
            IDS_IS_THIS_A_DUMP_HDR_SUB, // String resource ID
            IDS_IS_THIS_A_DUMP, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // (B_9_2)
    // TOAST_UNKNOWN_DEBUGGEE_PAGEID
    //
    g_rgpPageDefs[TOAST_UNKNOWN_DEBUGGEE_PAGEID] = new TEXT_ONLY_PAGE_DEF(
        TOAST_UNKNOWN_DEBUGGEE_PAGEID, // Page ID
        IDD_TEXT_ONLY, // DLG resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_UNKNOWN_DEBUGGEE_HDR, // String resource ID
        IDS_UNKNOWN_DEBUGGEE_HDR_SUB, // String resource ID
        IDS_UNKNOWN_DEBUGGEE, // String resource ID
        EXPERT_DEBUGGING_CHOICE_PAGEID // Goto this page on NEXT
        );

    //
    // (B_10)
    // DEBUG_APP_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_01_DEBUG_APP, // string ID for opt 1
            IDS_OPT_02_DEBUG_APP // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            HOST_OR_TARGET_PAGEID, // Option 1, goto this page
            USER_EXE_PROCESS_CHOICE_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[DEBUG_APP_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            DEBUG_APP_PAGEID, // Page ID
            IDD_TWO_OPT, // Dlg resource ID
            NULL_PAGEID, // ID of 'more info' page
            IDS_DEBUG_APP_HDR, // String resource ID
            IDS_DEBUG_APP_HDR_SUB, // String resource ID
            IDS_DEBUG_APP, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // (B_12)
    // HOST_OR_TARGET_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_01_HOST_OR_TARGET, // string ID for opt 1
            IDS_OPT_02_HOST_OR_TARGET // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            RUN_WIZARD_ON_TARGET_PAGEID, // Option 1, goto this page
            REMOTE_CONNECTION_TYPE_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[HOST_OR_TARGET_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            HOST_OR_TARGET_PAGEID, // Page ID
            IDD_TWO_OPT, // Dlg resource ID
            NULL_PAGEID, // ID of 'more info' page
            IDS_HOST_OR_TARGET_HDR, // String resource ID
            IDS_HOST_OR_TARGET_HDR_SUB, // String resource ID
            IDS_HOST_OR_TARGET, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // B_14
    // REMOTE_CONNECTION_TYPE_PAGEID
    //
    g_rgpPageDefs[REMOTE_CONNECTION_TYPE_PAGEID] = new SELECT_PORT_BAUD_PIPE_COMPNAME_PAGE_DEF(
        REMOTE_CONNECTION_TYPE_PAGEID, // Page ID
        IDD_SELECT_PORT_BAUD_PIPE, // DLG resource ID
        MI_01_NULL_MODEM_CABLE_PAGEID, // ID of 'more info' page
        IDS_REMOTE_CONNECTION_TYPE_HDR, // String resource ID
        IDS_REMOTE_CONNECTION_TYPE_HDR_SUB, // String resource ID
        IDS_REMOTE_CONNECTION_TYPE, // String resource ID
        USER_EXE_PROCESS_CHOICE_PAGEID // Goto this page on NEXT
        );

    //
    // (B_11)
    // CRASHDUMP_PAGEID
    //
    g_rgpPageDefs[CRASHDUMP_PAGEID] = new BROWSE_PATH_PAGE_DEF(
        CRASHDUMP_PAGEID, // Page ID
        IDD_BROWSE_PATH, // Dlg resource ID
        MI_01_DUMP_FILE_PAGEID, // ID of 'more info' page
        IDS_CRASHDUMP_HDR, // String resource ID
        IDS_CRASHDUMP_HDR_SUB, // String resource ID
        IDS_CRASHDUMP, // Text for the dlg
        SYMBOL_FILE_COPY_PAGEID //  Next page
        );

    //
    // (B_13)
    // REMOTE_TARGET_APP_PAGEID
    //
    g_rgpPageDefs[REMOTE_TARGET_APP_PAGEID] = new TEXT_ONLY_PAGE_DEF(
        REMOTE_TARGET_APP_PAGEID, // Page ID
        IDD_TEXT_ONLY, // DLG resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_REMOTE_TARGET_APP_HDR, // String resource ID
        IDS_REMOTE_TARGET_APP_HDR_SUB, // String resource ID
        IDS_REMOTE_TARGET_APP, // String resource ID
        SYMBOL_FILE_COPY_PAGEID // Goto this page on NEXT
        );

    //
    // (K_1)
    // SYMBOL_FILE_COPY_PAGEID
    //
    // intro to standard symbol copying
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_1_SYMBOL_FILE_COPY, // string ID for opt 1
            IDS_OPT_2_SYMBOL_FILE_COPY // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            OS_SYM_LOC_SYMCPY_PAGEID, // Option 1, goto this page
            ADV_SYMBOL_FILE_COPY_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[SYMBOL_FILE_COPY_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            SYMBOL_FILE_COPY_PAGEID, // Page ID
            IDD_TWO_OPT, // Dlg resource ID
            MI_01_SYMBOL_COPY_METHOD_PAGEID, // ID of 'more info' page
            IDS_SYMBOL_FILE_COPY_HDR, // String resource ID
            IDS_SYMBOL_FILE_COPY_HDR_SUB, // String resource ID
            IDS_SYMBOL_FILE_COPY, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // More info for
    // SYMBOL_FILE_COPY_PAGEID
    //
    g_rgpPageDefs[MI_01_SYMBOL_FILE_COPY_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_SYMBOL_FILE_COPY_PAGEID, // Page ID
        IDS_MI_01_SYMBOL_FILE_COPY_HDR, // String resource ID
        IDS_MI_01_SYMBOL_FILE_COPY_HDR_SUB, // String resource ID
        IDS_MI_01_SYMBOL_FILE_COPY, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );

    //
    // copy OS symbols
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_01_OS_SYM_LOC_SYMCPY, // string ID for opt 1
            IDS_OPT_02_OS_SYM_LOC_SYMCPY // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            DET_RUNTIME_PAGEID, // Option 1, goto this page
            DET_RUNTIME_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[OS_SYM_LOC_SYMCPY_PAGEID] = new MULTI_OPT_BROWSE_PATH_PAGE_DEF<dwMAX_OPTIONS>(
            OS_SYM_LOC_SYMCPY_PAGEID, // Page ID
            IDD_TWO_OPT_BROWSE_PATH, // Dlg resource ID
            MI_01_SYMBOLS_PAGEID, // ID of 'more info' page
            IDS_OS_SYM_LOC_SYMCPY_HDR, // String resource ID
            IDS_OS_SYM_LOC_SYMCPY_HDR_SUB, // String resource ID
            IDS_OS_SYM_LOC_SYMCPY, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // copy Service Pack symbols
    // SP_SYM_LOC_SYMCPY_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_01_SP_SYM_LOC_SYMCPY, // string ID for opt 1
            IDS_OPT_02_SP_SYM_LOC_SYMCPY // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            HOTFIX_SYM_LOC_SYMCPY_PAGEID, // Option 1, goto this page
            HOTFIX_SYM_LOC_SYMCPY_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[SP_SYM_LOC_SYMCPY_PAGEID] = new MULTI_OPT_BROWSE_PATH_PAGE_DEF<dwMAX_OPTIONS>(
            SP_SYM_LOC_SYMCPY_PAGEID, // Page ID
            IDD_TWO_OPT_BROWSE_PATH, // Dlg resource ID
            MI_01_SYMBOLS_PAGEID, // ID of 'more info' page
            IDS_SP_SYM_LOC_SYMCPY_HDR, // String resource ID
            IDS_SP_SYM_LOC_SYMCPY_HDR_SUB, // String resource ID
            IDS_SP_SYM_LOC_SYMCPY, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // copy hotfixes
    // HOTFIX_SYM_LOC_SYMCPY_PAGEID
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_01_HOTFIX_SYM_LOC_SYMCPY, // string ID for opt 1
            IDS_OPT_02_HOTFIX_SYM_LOC_SYMCPY // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            ASK_ADDITIONAL_SYM_LOC_SYMCPY_PAGEID, // Option 1, goto this page
            ASK_ADDITIONAL_SYM_LOC_SYMCPY_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[HOTFIX_SYM_LOC_SYMCPY_PAGEID] = new MULTI_OPT_BROWSE_PATH_PAGE_DEF<dwMAX_OPTIONS>(
            HOTFIX_SYM_LOC_SYMCPY_PAGEID, // Page ID
            IDD_TWO_OPT_BROWSE_PATH, // Dlg resource ID
            MI_01_SYMBOLS_PAGEID, // ID of 'more info' page
            IDS_HOTFIX_SYM_LOC_SYMCPY_HDR, // String resource ID
            IDS_HOTFIX_SYM_LOC_SYMCPY_HDR_SUB, // String resource ID
            IDS_HOTFIX_SYM_LOC_SYMCPY, // Text for the dlg
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // Ask if they have additional symbols to copy
    //
    {
        const DWORD dwMAX_OPTIONS = 2;
        int rgnStrIds[dwMAX_OPTIONS] = {
            IDS_OPT_1_ASK_ADDITIONAL_SYM_LOC_SYMCPY, // string ID for opt 1
            IDS_OPT_2_ASK_ADDITIONAL_SYM_LOC_SYMCPY // string ID for opt 2
        };
        PAGEID rgnPageIds[dwMAX_OPTIONS] = {
            // Go no where, stay on this page
            DET_RUNTIME_PAGEID, // Option 1, goto this page
            DET_RUNTIME_PAGEID // Option 2, goto this page
        };

        g_rgpPageDefs[ASK_ADDITIONAL_SYM_LOC_SYMCPY_PAGEID] = new MULTI_OPT_PAGE_DEF<dwMAX_OPTIONS>(
            ASK_ADDITIONAL_SYM_LOC_SYMCPY_PAGEID, // Page ID
            IDD_TWO_OPT, // Dlg resource ID
            MI_01_ADDITIONAL_SYMBOLS_PAGEID, // ID of 'more info' page
            IDS_ASK_ADDITIONAL_SYM_LOC_SYMCPY_HDR, // String resource ID
            IDS_ASK_ADDITIONAL_SYM_LOC_SYMCPY_HDR_SUB, // String resource ID
            IDS_ASK_ADDITIONAL_SYM_LOC_SYMCPY, // String resource ID
            0, // Default option
            rgnStrIds, sizeof(rgnStrIds),
            rgnPageIds, sizeof(rgnPageIds)
            );
    }

    //
    // Specify additional location of where to copy things to
    //
    g_rgpPageDefs[ADDITIONAL_SYM_LOC_SYMCPY_PAGEID] = new BROWSE_PATH_PAGE_DEF(
        ADDITIONAL_SYM_LOC_SYMCPY_PAGEID, // Page ID
        IDD_BROWSE_PATH, // Dlg resource ID
        MI_01_SYMBOLS_PAGEID, // ID of 'more info' page
        IDS_ADDITIONAL_SYM_LOC_SYMCPY_HDR, // String resource ID
        IDS_ADDITIONAL_SYM_LOC_SYMCPY_HDR_SUB, // String resource ID
        IDS_ADDITIONAL_SYM_LOC_SYMCPY, // Text for the dlg
        GOTO_PREV_PAGEID //  Next page
        );

    //
    // Get the destination directory for the symbols
    // GET_DEST_DIR_SYMCPY_PAGEID
    //
    g_rgpPageDefs[GET_DEST_DIR_SYMCPY_PAGEID] = new BROWSE_PATH_PAGE_DEF(
        GET_DEST_DIR_SYMCPY_PAGEID, // Page ID
        IDD_BROWSE_PATH, // Dlg resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_GET_DEST_DIR_SYMCPY_HDR, // String resource ID
        IDS_GET_DEST_DIR_SYMCPY_HDR_SUB, // String resource ID
        IDS_GET_DEST_DIR_SYMCPY, // Text for the dlg
        WARN_THAT_COPYING_IS_ABOUT_TO_BEGIN //  Next page
        );

    //
    // WARN_THAT_COPYING_IS_ABOUT_TO_BEGIN
    //
    g_rgpPageDefs[WARN_THAT_COPYING_IS_ABOUT_TO_BEGIN] = new TEXT_ONLY_PAGE_DEF(
        WARN_THAT_COPYING_IS_ABOUT_TO_BEGIN, // Page ID
        IDD_TEXT_ONLY, // DLG resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_WARN_THAT_COPYING_IS_ABOUT_TO_BEGIN_HDR, // String resource ID
        IDS_WARN_THAT_COPYING_IS_ABOUT_TO_BEGIN_HDR_SUB, // String resource ID
        IDS_WARN_THAT_COPYING_IS_ABOUT_TO_BEGIN, // String resource ID
        STD_COPY_SYMCPY_PAGEID // Goto this page on NEXT
        );

    //
    // standard sym copying (display progress and do actual copying)
    //
    g_rgpPageDefs[STD_COPY_SYMCPY_PAGEID] = new COPY_SYMS_PAGE_DEF(
        STD_COPY_SYMCPY_PAGEID, // Page ID
        IDD_STD_COPY_SYMS, // Dlg resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_SYMBOL_FILE_COPY_HDR, // String resource ID
        IDS_SYMBOL_FILE_COPY_HDR_SUB, // String resource ID
        IDS_SYMBOL_FILE_COPY, // Text for the dlg
        SHORTCUT_NAME_PAGEID //  Next page
        );

    //
    // Display a generic error
    //
    g_rgpPageDefs[GEN_ERROR_PAGEID] = new TEXT_ONLY_PAGE_DEF(
        GEN_ERROR_PAGEID, // Page ID
        IDD_TEXT_ONLY, // DLG resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_GEN_ERROR_HDR, // String resource ID
        0, // String resource ID
        0, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );

    //
    // Diplay a generic warning
    //
    g_rgpPageDefs[GEN_WARNING_PAGEID] = new TEXT_ONLY_PAGE_DEF(
        GEN_WARNING_PAGEID, // Page ID
        IDD_TEXT_ONLY, // DLG resource ID
        NULL_PAGEID, // ID of 'more info' page
        IDS_GEN_WARNING_HDR, // String resource ID
        0, // String resource ID
        0, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );


    //
    // Help text
    //

    g_rgpPageDefs[MI_01_IS_TARGET_RESPONSIVE_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_IS_TARGET_RESPONSIVE_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_IS_TARGET_RESPONSIVE, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_BLUE_SCREEN_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_BLUE_SCREEN_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_BLUE_SCREEN, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_HOST_TARGET_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_HOST_TARGET_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_HOST_TARGET, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_NULL_MODEM_CABLE_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_NULL_MODEM_CABLE_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_NULL_MODEM_CABLE, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_SAVE_LOAD_INI_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_SAVE_LOAD_INI_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_SAVE_LOAD_INI, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_WHY_RUN_ON_TARGET_FIRST_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_WHY_RUN_ON_TARGET_FIRST_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_WHY_RUN_ON_TARGET_FIRST, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_SYMBOLS_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_SYMBOLS_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_SYMBOLS, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_SYMBOL_COPY_METHOD_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_SYMBOL_COPY_METHOD_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_SYMBOL_COPY_METHOD, // String resource ID
        MI_01_SYMBOLS_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_SERVICE_PACK_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_SERVICE_PACK_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_SERVICE_PACK, // String resource ID
        MI_01_SYMBOLS_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_ADDITIONAL_SYMBOLS_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_ADDITIONAL_SYMBOLS_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_ADDITIONAL_SYMBOLS, // String resource ID
        MI_01_OEM_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_OEM_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_OEM_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_OEM, // String resource ID
        MI_01_SYMBOLS_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_LAUNCH_ATTACH_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_LAUNCH_ATTACH_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_LAUNCH_ATTACH, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_DUMP_FILE_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_DUMP_FILE_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_DUMP_FILE, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );

    g_rgpPageDefs[MI_01_APPLICATION_PAGEID] = new HELP_TEXT_ONLY_PAGE_DEF(
        MI_01_APPLICATION_PAGEID, // Page ID
        IDS_MORE_INFO_HDR, // String resource ID
        NULL, // String resource ID
        IDS_MI_01_APPLICATION, // String resource ID
        NULL_PAGEID // Goto this page on NEXT
        );




    return TRUE;
};
