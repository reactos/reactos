//
// Transport Layer:
//
#ifdef CINDIV_TL_DEFINE
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP, m_pszDescription, NULL);
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP, m_pszDll,         NULL);
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP, m_pszParams,      NULL);
#endif


//
// Paths
//
#ifdef CPATHS_DEFINE
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,          m_pszSourceCodeSearchPath,   NULL);
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,          m_pszSymPath,                _strdup("%SystemRoot%\\symbols"));
VAR_WRKSPC(PSTR,    CMULTI_SZ_ITEM_WKSP,    m_pszRootMappingPairs,       NULL);
#endif


//
// Individual Exception:
//
#ifdef CINDIV_EXCEP_DEFINE
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,      m_pszName,          NULL);
VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,   m_dwAction,         efdNotify);
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,      m_pszFirstCmd,      NULL);
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,      m_pszSecondCmd,     NULL);
#endif


//
// Kernel debugger params
//
#ifdef CKERNEL_DBG_DEFINE
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,     m_bInitialBp,       TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,     m_bUseModem,        FALSE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,     m_bGoExit,          FALSE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,     m_bUseCrashDump,    FALSE);

VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,   m_dwBaudRate,       19200);
VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,   m_dwPort,           2);
VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,   m_dwCache,          102400);


// What platform are we running on?
#if defined(HOST_MIPS)

VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,   m_mptPlatform,      mptmips);

#elif defined(HOST_i386)

VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,   m_mptPlatform,      mptix86);

#elif defined(HOST_ALPHA)

VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,   m_mptPlatform,      mptdaxp);

#elif defined(HOST_PPC)

VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,   m_mptPlatform,      mptntppc);

#elif defined(HOST_IA64)

VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,   m_mptPlatform,      mptia64);

#else

VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,   m_mptPlatform,      mptUnknown);

#endif

// These variables are not stored in the registry
STD_VAR_WRKSPC(PSTR,    m_pszCrashDump,     NULL);
#endif

    
//
// Individual Workspace:
//
#ifdef CWORKSPACE_DEFINE
// Automatically reload source files changed outside of the debugger
//  TRUE - Automatically reload changed source files without prompting
//  FALSE - Ask the user whether he wants to reload the source file
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bAutoReloadSrcFiles,			FALSE);

// Attempt to evaluate all UINT pointers as unicode strings
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bUnicodeIsDefault,			FALSE);

VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bSrcMode,                     TRUE);

VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,          m_pszSelectedTL,                _strdup(szWDBG_DEFAULT_TL_NAME));
VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,          m_pszWindowTitle,               NULL);

VAR_WRKSPC(PSTR,    CMULTI_SZ_ITEM_WKSP,    m_pszRootMappingPairs,          NULL);

VAR_WRKSPC(PSTR,    CMULTI_SZ_ITEM_WKSP,    m_pszBreakPoints,               NULL);


VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,          m_pszCmdLine,                   NULL);

// children of the new process will be debugged.
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bDebugChildren,               TRUE);

// When attaching to process, indicates whether the process
// should be stopped or left running
// Help text: [Don't break into newly attached processes]
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bAttachGo,                    FALSE);

// Do/Don't break into new child processes
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bChildGo,                     TRUE);

// Indicates whether the process should be stopped when a thread terminates.
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bGoOnThreadTerm,              TRUE);

// Indicates whether to continue or stop when an image is loaded
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bGoOnModLoad,					TRUE);

// Toggle: notification when a thread is terminated.
// FALSE implies GoOnThreadTerm == TRUE
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bNotifyThreadTerm,            TRUE); 

// Toggle: notification of when a thread is created.
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bNotifyThreadCreate,          TRUE);


// Debuggee inherits handles - for debugging debuggers
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bInheritHandles,              FALSE);

// First step goes to entry point, not main
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bEPIsFirstStep,               FALSE);

// Enable Kernel debugger
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bKernelDebugger,              FALSE);

// For 16 bit processes, documented in help file and accesible from the UI.
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bWowVdm,                      FALSE);

// When disconnecting from the remote, should a "simple disconnect" be performed,
// or should the remote be shutdown.
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bDisconnectOnExit,            TRUE);

// Slow stepping
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bAlternateSS,                 FALSE);

// Ignore all symbol errors when loading symbols
// TRUE - allows the debugger to load symbols that have mismatched timestaps, checksums,
//          and when the image header could not be read because the header has been paged out.
// FALSE - the debugger will not load symbols that experience errors while loading
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bIgnoreAllSymbolErrors,       TRUE);

// Ask for user intervention if symbols could not be loaded without errors
// TRUE - ask for user intervention
// FALSE - do not ask for user intervention
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bBrowseForSymsOnSymLoadErrors,	TRUE);

//
//  m_bIgnoreAllSymbolErrors == TRUE (fBrowseForSymsOnSymLoadErrors is ignored)
//          A symbol file will be loaded regardless of any errors such as mismatched
//          timestamps, checksum errors, etc.
//  m_bIgnoreAllSymbolErrors == FALSE
//      BrowseForSymsOnSymLoadErrors == FALSE
//          Any error during symbol loading will prevent the symbol for that file to
//          be loaded. The user will NOT be prompted for any action.
//
//      BrowseForSymsOnSymLoadErrors == TRUE
//          If any errors during symbol loading occur, the symbol will not be loaded.
//          However, the user will be asked to intervene by specifying a symbol file
//          for image in question or choose not to load a symbols file.
//



// Make WinDbg noisier! Do a grep for more details.
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bVerbose,                     TRUE);

// Toggle between long & short context notation
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bShortContext,                TRUE);

// Toggles between the masm evaluator and the C evaluator.
// See command line ".opt masmeval" for the same amount of info,
// better yet, see the help file.
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bMasmEval,                    FALSE);

// Background symbol loading
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bShBackground,                FALSE);

// Ignore version checking on DLLs
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bNoVersion,                   FALSE);

// Indicates whether a user mode crash dump is being debugged.
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bUserCrashDump,               FALSE);

VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bRegModeExt,                  TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bRegModeMMU,                  FALSE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bShowSegVal,                  FALSE);


VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bLfOptAppend,                 FALSE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bLfOptAuto,                   FALSE);

VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,       m_dopDisAsmOpts,                
    dopSym | dopFlatAddr | dopNeverOpenAutomatically | dopRaw);


VAR_WRKSPC(PSTR,    CSZ_ITEM_WKSP,          m_pszLogFileName,               _strdup("windbg.log"));


// workspaces
D_CONT_WRKSPC(CAll_Exceptions_WKSP,             m_dynacontAllExceptions);
CONT_WRKSPC(CPaths_WKSP,                        m_contPaths);
CONT_WRKSPC(CKernel_Debugging_WKSP,             m_contKernelDbgPreferences);

// These variables are not stored in the registry
STD_VAR_WRKSPC(PSTR,    m_pszRemotePipe,        NULL);
STD_VAR_WRKSPC(PSTR,    m_pszUserCrashDump,     NULL);
#endif

/*
//
// All the workspaces
//
#ifdef CALL_WORKSPACES_DEFINE
#endif
*/

//
// Debuggee:
//
#ifdef CDEBUGGEE_DEFINE
VAR_WRKSPC(PSTR, 
           CSZ_ITEM_WKSP, 
           m_pszNameOfPreferredWorkSpace, 
           _strdup(CBase_Windbg_WKSP::m_pszUntitledWorkSpaceName)
           );

M_CONT_WRKSPC(CWorkSpace_WKSP,                m_contWorkSpace);
#endif


//
// All Debuggees
//
#ifdef CALL_DEBUGGEES_DEFINE
CONT_WRKSPC(CDebuggee_WKSP,                 m_contDebuggee);
#endif


//
// User preferences:
//
#ifdef CPREFERENCES_DEFINE
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bShowToolbar,                     TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bShowStatusBar,                   TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bAlwaysSaveWorkspace,             TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bPromptBeforeSavingWorkspace,     TRUE);
VAR_WRKSPC(PSTR,    CMULTI_SZ_ITEM_WKSP,    m_pszFilesMRUList,                  NULL);

VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bKeepTabs,                        TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_nTabSize,                         4);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bVertScrollBars,                  TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bHorzScrollBars,                  TRUE);

// In bytes
VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,       m_dwUndoResize,                     4096);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bSrchSysPathForExe,               TRUE);

// Toggles: enter key at command line acts as a repeat of the previous command.
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bCommandRepeat,                   FALSE);


// Used by call stack window
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bFrameptr,                    TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bRetAddr,                     TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bFuncName,                    TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bDisplacement,                TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bParams,                      TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bStack,                       FALSE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bSource,                      FALSE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bModule,                      TRUE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bRtf,                         FALSE);
VAR_WRKSPC(BOOL,    CINT_ITEM_WKSP,         m_bFrameNum,                    FALSE);
VAR_WRKSPC(DWORD,   CDWORD_ITEM_WKSP,       m_dwMaxFrames,                  500);



D_CONT_WRKSPC(CAll_TLs_WKSP,                m_dynacont_All_TLs);
CONT_WRKSPC(CUser_WM_Messages_WKSP,         m_contUser_WM_Messages);
D_CONT_WRKSPC(CAll_Exceptions_WKSP,         m_dynacontAllExceptionsMasterList);
#endif


//
// Individual window layout information
//
#ifdef CINDIV_WIN_LAYOUT_DEFINE
// Common to all windows
VAR_WRKSPC(int,             CINT_ITEM_WKSP,         m_nType,            -1);
VAR_WRKSPC(int,             CINT_ITEM_WKSP,         m_nOrder,           -1);

VAR_WRKSPC(WINDOW_STATE,    CWINDOW_STATE_ITEM_WKSP, m_nWindowState,    WSTATE_NORMAL);

VAR_WRKSPC(int,             CINT_ITEM_WKSP,         m_nX,               CW_USEDEFAULT);
VAR_WRKSPC(int,             CINT_ITEM_WKSP,         m_nY,               CW_USEDEFAULT);
VAR_WRKSPC(int,             CINT_ITEM_WKSP,         m_nWidth,           CW_USEDEFAULT);
VAR_WRKSPC(int,             CINT_ITEM_WKSP,         m_nHeight,          CW_USEDEFAULT);

BIN_VAR_WRKSPC(LOGFONT,     CLOGFONT_ITEM_WKSP,     m_logfont );


// Used by document window
VAR_WRKSPC(PSTR,            CSZ_ITEM_WKSP,          m_pszFileName,      NULL);
VAR_WRKSPC(BOOL,            CINT_ITEM_WKSP,         m_bReadOnly,        TRUE);
VAR_WRKSPC(int,             CINT_ITEM_WKSP,         m_nColumn,          0);
VAR_WRKSPC(int,             CINT_ITEM_WKSP,         m_nLine,            0);


// Used by Paned windows 
VAR_WRKSPC(DWORD,           CDWORD_ITEM_WKSP,       m_dwPaneFlags,          0);
VAR_WRKSPC(DWORD,           CDWORD_ITEM_WKSP,       m_dwPerCent,            0);


// Used by memory window
VAR_WRKSPC(BOOL,            CINT_ITEM_WKSP,         m_nFormat,          0);
VAR_WRKSPC(BOOL,            CINT_ITEM_WKSP,         m_bLive,            FALSE);


// Used by memory window & paned window
VAR_WRKSPC(PSTR,            CSZ_ITEM_WKSP,          m_pszExpression,    NULL);

#endif


//
// An window layout
//
#ifdef CWIN_LAYOUT_DEFINE
// All other windows
CONT_WRKSPC(CIndivWinLayout_WKSP,                   m_contFrameWindows);

D_CONT_WRKSPC(CAllChildWindows_WKSP,                m_dynacontChildWindows);
#endif

//
// All window layouts
//
#ifdef CALL_WIN_LAYOUTS_DEFINE
VAR_WRKSPC(PSTR,            CSZ_ITEM_WKSP,          m_pszLastUsedWinLayout,     _strdup("default"));

M_CONT_WRKSPC(CWinLayout_WKSP,                      m_contWinLayout);
#endif


//
// Root for windbg workspaces
//
#ifdef CWINDBG_DEFINE
// List of all of the debuggees
CONT_WRKSPC(CAll_Debuggees_WKSP,                    m_contAllDebuggees);

CONT_WRKSPC(CGlobalPreferences_WKSP,                m_contGlobalPreferences);

// Window Layout
CONT_WRKSPC(CAllWindowLayouts_WKSP,                 m_contAllWinLayouts);

// These variables are not stored in the registry
//STD_VAR_WRKSPC(PSTR,    m_pszProgramName,           _strdup(m_pszNoProgramLoaded));
//STD_VAR_WRKSPC(PSTR,    m_pszWorkSpaceName,         _strdup(m_pszUntitledWorkSpaceName));
#endif




