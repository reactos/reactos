//
// (windbgrm) Each Transport Layer contains Kernel Debugger Settings:
//
#ifdef CKD_RM_DEFINE
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,      m_bEnable,     FALSE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,      m_bVerbose,    FALSE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,      m_bInitialBp,  FALSE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,      m_bDefer,      TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,      m_bUseModem,   FALSE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,      m_bGoExit,     FALSE);
VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,    m_dwBaudRate,  19200);
VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,    m_dwPort,      2);
VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,    m_dwCache,     102400);
VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,    m_dwPlatform,  0);
#endif


//
// (windbgrm) Transport Layer:
//
#ifdef CINDIV_TL_RM_DEFINE
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,      m_pszDescription, NULL);
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,      m_pszDll,         NULL);
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,      m_pszParams,      NULL);

CONT_WRKSPC(CKD_RM_WKSP,                m_contKdSettings);
#endif


//
// (windbgrm) List of all Transport Layers:
//
#ifdef CRM_ALL_TLS_DEFINE
#endif


//
// Root for windbgrm
//
#ifdef CWINDBG_RM_DEFINE
// The variable must be declared first because it is used by container.
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,      m_pszSelectedTL,    _strdup(szRM_DEFAULT_TL_NAME));

// Dynamic list of all the transport layers
D_CONT_WRKSPC(CAll_TLs_RM_WKSP,         m_dynacont_All_TLs);
#endif

