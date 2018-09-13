enum DEBUG_MODE { 
    // User mode. Remote or local debugging is allowed.
    enumLOCAL_USERMODE, 
    
    // A debugging situation where user mode remote debugging is the 
    // only allowed scenario.
    enumREMOTE_USERMODE,

    enumKERNELMODE, 
    enumDUMP, 

    // A situation that cannot be done easily by WinDbg, and should be
    // done using KD & NTSD.
    enumNOT_SUPPORTED
};



// Privat app messages
#define UM_DLG_PAGE_SETACTIVE       (WM_USER + 200)
#define UM_START_STD_MODE_COPYING   (WM_USER + 201)
#define UM_STARTCOPYING             (WM_USER + 202)
#define UM_FINISHED_ENDDIALOG       (WM_USER + 203)



enum PAGEID {
    NULL_PAGEID = -4,

    GOTO_PREV_PAGEID = -3,

    // Flag to indicate that we are finished
    THE_END_PAGEID = -2,

    // Determine at runtime which page to jump to
    DET_RUNTIME_PAGEID = -1,

    FIRST_PAGEID = 0,

    WELCOME_PAGEID = 0,
    
    RUN_WIZARD_ON_TARGET_PAGEID,

    SPECIFY_INI_FILE_PAGEID,
    CONNECTION_SELECTION_PAGEID,
    SYMBOL_FILE_COPY_PAGEID,
    TARGET_CONFIG_FILE_LOCATION_PAGEID,

    SELECT_PORT_PAGEID,
    SHORTCUT_NAME_PAGEID,
    IS_THIS_KERNEL_MODE_PAGEID,
    SELECT_PORT_BAUD_PAGEID,
    IS_TARGET_RESPONSIVE_PAGEID,
    TOAST_MESSAGE_PAGEID,
    DEBUG_APP_PAGEID,
    CRASHDUMP_PAGEID,
    HOST_OR_TARGET_PAGEID,
    REMOTE_CONNECTION_TYPE_PAGEID,
    REMOTE_TARGET_APP_PAGEID,
    KERNEL_MACHINE_ROLE_PAGEID,
    EXPERT_DEBUGGING_CHOICE_PAGEID,
    SELECT_HANDDOLD_INI_PAGEID,
    USER_EXE_PROCESS_CHOICE_PAGEID,
    RM_USER_HOST_TARGET_CHOICE_PAGEID,
    KERNEL_HOST_TARGET_CHOICE_PAGEID,

    IS_THIS_AN_APPLICATION_PAGEID,
    IS_THIS_A_DUMP_PAGEID,
    TOAST_UNKNOWN_DEBUGGEE_PAGEID,

    KERNEL_SELECT_PORT_BAUD_PAGEID,
    FINISH_PAGEID,
    DISPLAY_SUMMARY_INFO_PAGEID,

    //START_HAVE_INI_FILE_PAGE_ID,

    //ADV_OR_STD_WIZ_PAGEID,


    SAVE_INI_FILE_PAGEID,


    //ADV_OR_STD_SYMCPY_PAGEID,
    //ADV_SYMCPY_PAGEID,
    OS_SYM_LOC_SYMCPY_PAGEID,
    SP_SYM_LOC_SYMCPY_PAGEID,
    HOTFIX_SYM_LOC_SYMCPY_PAGEID,
    ASK_ADDITIONAL_SYM_LOC_SYMCPY_PAGEID,
    ADDITIONAL_SYM_LOC_SYMCPY_PAGEID,

    GET_DEST_DIR_SYMCPY_PAGEID,

    WARN_THAT_COPYING_IS_ABOUT_TO_BEGIN,

    ADV_SYMBOL_FILE_COPY_PAGEID,
    STD_COPY_SYMCPY_PAGEID,

    //
    // Help Dlgs
    //
    MI_01_SYMBOL_FILE_COPY_PAGEID,
    MI_01_IS_TARGET_RESPONSIVE_PAGEID,
    MI_01_BLUE_SCREEN_PAGEID,
    MI_01_SAVE_LOAD_INI_PAGEID,
    MI_01_HOST_TARGET_PAGEID,
    MI_01_NULL_MODEM_CABLE_PAGEID,
    MI_01_WHY_RUN_ON_TARGET_FIRST_PAGEID,
    MI_01_SYMBOLS_PAGEID,
    MI_01_SYMBOL_COPY_METHOD_PAGEID,
    MI_01_SERVICE_PACK_PAGEID,
    MI_01_ADDITIONAL_SYMBOLS_PAGEID,
    MI_01_OEM_PAGEID,
    MI_01_LAUNCH_ATTACH_PAGEID,
    MI_01_DUMP_FILE_PAGEID,
    MI_01_APPLICATION_PAGEID,
    





    //
    // Generic dlgs
    //
    // The code will mess with this class' data
    //  at runtime.
    GEN_ERROR_PAGEID,
    GEN_WARNING_PAGEID,
    //GEN_TWO_OPT_PAGEID,

    



    //ADV_SYM_CPY_PAGEID,
    MAX_NUM_PAGEID
};

typedef struct _COMPORTINFO {
    CHAR    szSymName[16];
    DWORD   dwNum;
    DWORD   dwSettableBaud;
} COMPORT_INFO, * PCOMPORT_INFO;


class CFGDATA;
extern CFGDATA g_CfgData;

class PAGE_DEF;
extern PAGE_DEF * g_rgpPageDefs[MAX_NUM_PAGEID];


extern HFONT g_hBigBoldFont;
extern HFONT g_hBoldFont;

extern HINSTANCE g_hInst;

extern TSingle_List<PAGEID> g_stack;
extern TList<PSTR> g_SymPaths;


extern CFGDATA g_CfgData;

extern g_bRunning_IE_3;


int CALLBACK WizardStub_DlgProc(HWND, UINT, WPARAM, LPARAM);

BOOL InitPageDefs();

DWORD GetNumPropSheetPages(HWND hwndPropSheet);

PSTR BaudRateBitFlagToText(DWORD dwBaudRate);

DWORD BaudRateTextToBitFlag(PSTR pszBaudRate);





//
//
//
struct IE_3_PROP_PAGE
{
    DWORD           dwSize;
    DWORD           dwFlags;
    HINSTANCE       hInstance;
    union {
        LPCSTR          pszTemplate;
#ifdef _WIN32
        LPCDLGTEMPLATE  pResource;
#else
        const VOID FAR *pResource;
#endif
    } DUMMYUNIONNAME;
    union {
        HICON       hIcon;
        LPCSTR      pszIcon;
    } DUMMYUNIONNAME2;
    LPCSTR          pszTitle;
    DLGPROC         pfnDlgProc;
    LPARAM          lParam;
    LPFNPSPCALLBACKA pfnCallback;
    UINT FAR * pcRefParent;
};


struct IE_3_PROP_SHEET 
{
    DWORD           dwSize;
    DWORD           dwFlags;
    HWND            hwndParent;
    HINSTANCE       hInstance;
    union {
        HICON       hIcon;
        LPCSTR      pszIcon;
    }DUMMYUNIONNAME;
    LPCSTR          pszCaption;
    
    UINT            nPages;
    union {
        UINT        nStartPage;
        LPCSTR      pStartPage;
    }DUMMYUNIONNAME2;
    union {
        LPCPROPSHEETPAGEA ppsp;
        HPROPSHEETPAGE FAR *phpage;
    }DUMMYUNIONNAME3;
    PFNPROPSHEETCALLBACK pfnCallback;
};


struct tagIE_3_OPENFILENAMEA {
   DWORD        lStructSize;
   HWND         hwndOwner;
   HINSTANCE    hInstance;
   LPCSTR       lpstrFilter;
   LPSTR        lpstrCustomFilter;
   DWORD        nMaxCustFilter;
   DWORD        nFilterIndex;
   LPSTR        lpstrFile;
   DWORD        nMaxFile;
   LPSTR        lpstrFileTitle;
   DWORD        nMaxFileTitle;
   LPCSTR       lpstrInitialDir;
   LPCSTR       lpstrTitle;
   DWORD        Flags;
   WORD         nFileOffset;
   WORD         nFileExtension;
   LPCSTR       lpstrDefExt;
   LPARAM       lCustData;
   LPOFNHOOKPROC lpfnHook;
   LPCSTR       lpTemplateName;
};
