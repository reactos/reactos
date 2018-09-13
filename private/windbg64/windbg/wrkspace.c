/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    WrkSpace.c

Abstract:

    This module contains the support for Windbg's workspaces.

Author:

    Ramon J. San Andres (ramonsa)   07-July-1992
    Griffith Wm. Kadnier (v-griffk) 15-Jan-1993
    Carlos Klapp (a-caklap)         15-July-98


Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include "dbugexcp.h"

#include "include\cntxthlp.h"


//
// Variables
//
char SrcFileDirectory[ MAX_PATH ];
char ExeFileDirectory[ MAX_PATH ];
char DocFileDirectory[ MAX_PATH ];
char UserDllDirectory[ MAX_PATH ];


CFinal_Windbg_WKSP          g_Windbg_WkSp;
CGlobalPreferences_WKSP     &g_contGlobalPreferences_WkSp = g_Windbg_WkSp.m_contGlobalPreferences;
CAll_Exceptions_WKSP        &g_contExceptionsMasterList_WkSp = g_contGlobalPreferences_WkSp.m_dynacontAllExceptionsMasterList;
CUser_WM_Messages_WKSP      &g_contUser_WM_Messages_WkSp = g_contGlobalPreferences_WkSp.m_contUser_WM_Messages;
CAll_TLs_WKSP               &g_dynacontAll_TLs_WkSp = g_contGlobalPreferences_WkSp.m_dynacont_All_TLs;

CAll_Debuggees_WKSP         &g_contAllDebuggees_WkSp = g_Windbg_WkSp.m_contAllDebuggees;
CDebuggee_WKSP              &g_contDebuggee_WkSp = g_contAllDebuggees_WkSp.m_contDebuggee;
CWorkSpace_WKSP             &g_contWorkspace_WkSp = g_contDebuggee_WkSp.m_contWorkSpace;

CAll_Exceptions_WKSP        &g_dynacontAllExceptions_WkSp = g_contWorkspace_WkSp.m_dynacontAllExceptions;
CPaths_WKSP                 &g_contPaths_WkSp = g_contWorkspace_WkSp.m_contPaths;
CKernel_Debugging_WKSP      &g_contKernelDbgPreferences_WkSp = g_contWorkspace_WkSp.m_contKernelDbgPreferences;

CAllWindowLayouts_WKSP      &g_contAllWinLayouts_WkSp = g_Windbg_WkSp.m_contAllWinLayouts;
CWinLayout_WKSP             &g_contWinLayout_WkSp = g_contAllWinLayouts_WkSp.m_contWinLayout;

CIndivWinLayout_WKSP        &g_contFrameWindow = g_contWinLayout_WkSp.m_contFrameWindows;
CAllChildWindows_WKSP       &g_dynacontAllChildWindows = g_contWinLayout_WkSp.m_dynacontChildWindows;



BOOL
WKSP_Initialize()
{
    //
    // We initialize it this so that its parent is 'g_Windbg_WkSp'
    // and that it has a name, but we want to be able to load/save all
    // of the debuggees and workspaces separately of from the one the debugger
    // is using.

    // We do it here so that we 
    g_contDebuggee_WkSp.SetRegistryName(CBase_Windbg_WKSP::m_pszNoProgramLoaded);

    g_contWorkspace_WkSp.SetRegistryName(CBase_Windbg_WKSP::m_pszUntitledWorkSpaceName);

    g_contWinLayout_WkSp.SetRegistryName(CBase_Windbg_WKSP::m_pszDefaultWinLayout);

    // Make sure the mirrored flags are correctly set.
    g_Windbg_WkSp.SetMirrorFlagForChildren();

    return TRUE;
}




//
// Function prototypes
//
INT_PTR
CALLBACK
DlgProc_WindowLayout_SaveAs(
    HWND    hDlg,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

INT_PTR
CALLBACK 
DlgProc_WindowLayout_Manage(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
CALLBACK 
DlgProc_WorkSpace_Manage(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
CALLBACK
DlgProc_WorkSpace_SaveAs(
    HWND    hDlg,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    );


//
// Code
//
BOOL
SetTransportLayer(
    PSTR pszTL_Name,
    CAll_TLs_WKSP * pCAll_TLs_WrkSpc
    )
/*++
Routine Description:
    Sets the name of the selected transport and warns user 
    if pipes are not supported on this platform.

Arguments:

    pszTL_Name - Name of the selected transport
--*/
{
    Assert(pszTL_Name);
    Assert(pCAll_TLs_WrkSpc);

    // Make sure that we warn the user that he is using a transport layer
    // that requires named pipes and they are not supported on win9x
    static OSVERSIONINFO osi = {0};
    
    if (0 == osi.dwOSVersionInfoSize) {
        osi.dwOSVersionInfoSize = sizeof(osi);
        Assert(GetVersionEx(&osi));
    }


    TListEntry<CIndiv_TL_WKSP *> * pContEntry =
        pCAll_TLs_WrkSpc->m_listConts.
        Find(pszTL_Name, WKSP_Generic_CmpRegName);
    
    if (!pContEntry) {
        return FALSE;
    }

    CIndiv_TL_WKSP * pIndTl = pContEntry->m_tData;
    AssertType(*pIndTl, CIndiv_TL_WKSP);

    if (VER_PLATFORM_WIN32_NT != osi.dwPlatformId && !_stricmp(pIndTl->m_pszDll, "tlpipe.dll")) {
        InformationBox(ERR_Pipes_Not_Supported_On_Win9x, pIndTl->m_pszDescription, "tlpipe.dll");
    }

    if (g_contWorkspace_WkSp.m_pszSelectedTL) {
        free(g_contWorkspace_WkSp.m_pszSelectedTL);
    }
    g_contWorkspace_WkSp.m_pszSelectedTL = _strdup(pszTL_Name);
    

    return TRUE;
}                                       /* SetDllKey() */



BOOL
ValidateAllTransportLayers(
    HWND    hwndParent,
    CAll_TLs_WKSP * pCAll_TLs_WrkSpc
    )
/*++

Routine Description:

    Verify that all of the TLs are valid.

Arguments:

    hDlg    - Supplies window handle for dialog

Returns:

    TRUE if DLL is valid or user said use it anyway; FALSE otherwise

--*/
{
    HMODULE     hDll;
    CIndiv_TL_WKSP * pIndTl;
    
    TListEntry<CIndiv_TL_WKSP *> * pContEntry = pCAll_TLs_WrkSpc->m_listConts.FirstEntry();
    for (; pContEntry != pCAll_TLs_WrkSpc->m_listConts.Stop(); 
        pContEntry = pContEntry->Flink) {
        
        pIndTl = pContEntry->m_tData;
        AssertType(*pIndTl, CIndiv_TL_WKSP);        
        
        if (hDll = LoadHelperDll(pIndTl->m_pszDll, "TL", FALSE)) {
            FreeLibrary(hDll);
        } else {
            int nRes = VarMsgBox(hwndParent,
                                 DBG_Bad_DLL_YESNO,
                                 MB_YESNO 
                                     | MB_ICONQUESTION 
                                     | (NULL == hwndParent ? MB_TASKMODAL : 0),
                                 pIndTl->m_pszDescription ? pIndTl->m_pszDescription : ""
                                 );
            
            if (nRes == IDNO) {
                return FALSE;
            }
        }
    }
    
    return TRUE;
}





BOOL
SYSetProfileString(
                  LPCTSTR szName,
                  LPCTSTR szValue
                  )
{
    return FALSE;
}

BOOL
SYGetProfileString(
    LPCTSTR szName,
    LPTSTR szValue,
    ULONG cbValue,
    ULONG * pcbRet
    )
{
    ULONG       cb;
    cb = ModListGetSearchPath(NULL, cbValue);
    if (cb <= cbValue) {
        *pcbRet = ModListGetSearchPath(szValue, cbValue);
        return TRUE;
    }
    *pcbRet = cb;
    return FALSE;
}

BOOL
Get_TL_ModuleInfo(
    TCHAR szModuleName[MAX_PATH],
    TCHAR szParams[MAX_PATH]
    )
{
    PCTSTR pszTmp;

    if (!szModuleName && !szParams) {
        return FALSE;
    }

    if (szModuleName) {
        pszTmp = g_Windbg_WkSp.GetSelected_TL_Dll_Name();
        if (!pszTmp) {
            return FALSE;
        }
        _tcscpy(szModuleName, pszTmp);
    }

    if (szParams) {
        pszTmp = g_Windbg_WkSp.GetSelected_TL_Params();
        if (!pszTmp) {
            return FALSE;
        }
        _tcscpy(szParams, pszTmp);
    }

    return TRUE;
}

void
WKSPSetupTL(
    HTL htl
    )
{
    TLSS    tlss;
    
    tlss.fRMAttached = FALSE;
    
    tlss.fLoad = TRUE;
    
    //tlss.lParam = 0;
    tlss.pfnGetModuleInfo = Get_TL_ModuleInfo;
    
    OSDTLSetup(htl, &tlss);
}


BOOL
Get_EM_ModuleInfo(
    TCHAR szModuleName[MAX_PATH],
    TCHAR szParams[MAX_PATH]
    )
{
    if (!szModuleName && !szParams) {
        return FALSE;
    }

    //
    // Get the module name
    //
    if (szModuleName) {
        if (!g_contWorkspace_WkSp.m_bKernelDebugger) {
            _tcscpy(szModuleName, _T("DM.DLL") );
        } else {
            switch (g_contKernelDbgPreferences_WkSp.m_mptPlatform) {
            case mptix86:
                _tcscpy(szModuleName, _T("DMKDX86.DLL ") );
                break;
                
#if 0
            case mptmips:
                _tcscpy(szModuleName, _T("DMKDMIP.DLL ") );
                break;
#endif
                
            case mptdaxp:
                _tcscpy(szModuleName, _T("DMKDALP.DLL ") );
                break;
                
            case mptia64:
                _tcscpy(szModuleName, _T("DMKDI64.DLL ") );
                break;
                
#if 0
            case mptntppc:
                _tcscpy(szModuleName, _T("DMKDPPC.DLL ") );
                break;
#endif
                
            default:
                _tcscpy(szModuleName, _T("") );
                // This should not have happened.
                Assert(0);
            }
        }
    }


    //
    // Get the parameters
    //
    if (szParams) {
        if (!g_contWorkspace_WkSp.m_bKernelDebugger) {
            _tcscpy(szParams, _T("") );
        } else {
            FormatKdParams(szParams);
        }
    }

    return TRUE;
}

void
WKSPSetupEM(
    HEM hem
    )
{
    EMSS    emss;

    emss.fLoad = TRUE;
    emss.fSave = FALSE;
    
    //emss.lParam = NULL;
    emss.pfnGetModuleInfo = Get_EM_ModuleInfo;
    
    OSDEMSetup(hem, &emss);
}






//
//  External variables
//
extern char     DebuggerName[];
extern LPSTR    LpszCommandLine;
extern CXF      CxfIp;
extern LPSTR    LpszCommandLineTransportDll;

//
//  External functions
//
extern HWND     GetLocalHWND(void);
extern HWND     GetFloatHWND(void);
extern HWND     GetWatchHWND(void);
extern HWND     GetCpuHWND(void);
extern HWND     GetCallsHWND(void);
extern LRESULT  SendMessageNZ (HWND,UINT,WPARAM,LPARAM);

//
//  Global variables
//
BOOL                bProgramLoaded   = FALSE;
//char                szCurrentWorkSpaceName[ MAX_PATH ];
//char                szCurrentProgramName[ MAX_PATH ];


EXCEPTION_LIST *DefaultExceptionList = NULL;











//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
// Root for windbg
//


CFinal_Windbg_WKSP::
CFinal_Windbg_WKSP()
{
    m_CurrentExePath = _strdup(m_pszNoProgramLoaded);
}


CFinal_Windbg_WKSP::
~CFinal_Windbg_WKSP()
{
    FREE_STR(m_CurrentExePath);
}

void 
CFinal_Windbg_WKSP::
SetCurrentProgramName(
    PCSTR pszProgPath
    ) 
/*++
Description
    Set the name of an executable and loads the appropriate workspace.
--*/
{
    Assert(pszProgPath);

    TCHAR szExe[_MAX_PATH];


    //
    // Store the complete path of the EXE so that it can be loaded
    //
    m_CurrentExePath = _strdup(pszProgPath);
    
    
    //
    // Get the name of the exe to use as the workspace key
    //
    _tsplitpath(pszProgPath, NULL, NULL, szExe, NULL);

    // string is blank
    if ( !*szExe ) {
        return;
    }

    if ( !_tcsicmp(szExe, NT_ALT_KERNEL_NAME)
        || !_tcsicmp(szExe, NT_ALT_KRNLMP_NAME)
        || !_tcsicmp(szExe, NT_KERNEL_NAME)
        || !_tcsicmp(szExe, NT_KRNLMP_NAME)
        ) {

        g_contWorkspace_WkSp.m_bKernelDebugger = TRUE;
    }

    if ( _tcsicmp(szExe, g_contDebuggee_WkSp.m_pszRegistryName) ) {
        
        g_contDebuggee_WkSp.CloseRegistryKeys();
        g_contDebuggee_WkSp.SetRegistryName(szExe);
    }
}


void 
CFinal_Windbg_WKSP::
SetCurrentWorkSpaceName(
    PCSTR pszWkSpName
    ) 
/*++
Description
    Set the name of an executable and loads the appropriate workspace.
--*/
{
    Assert(pszWkSpName);

    PSTR pszTmp;

    // Skip over white space and remove any trailing white space
    pszWkSpName = CPSkipWhitespace(pszWkSpName);

    pszTmp = _strdup(pszWkSpName);
    CPRemoveTrailingWhitespace(pszTmp);

    // string is blank
    if ( !*pszTmp ) {
        goto CLEANUP;
    }

    if ( _tcsicmp(pszTmp, g_contWorkspace_WkSp.m_pszRegistryName) ) {
        
        g_contWorkspace_WkSp.CloseRegistryKeys();
        g_contWorkspace_WkSp.SetRegistryName(pszTmp);
    }

CLEANUP:
    FREE_STR(pszTmp);
}



const char *
CFinal_Windbg_WKSP::
GetCurrentProgramName(
    BOOL bReturnUntitled
    )
/*++
Routine Description:
    Returns current program name

Arguments:
    bReturnUntitled -
        If a program has been loaded:
            The value does not matter and the name of the program is returned.
        If a program is NOT loaded:
            TRUE - the default name will be returned.
            FALSE - NULL is returned.

Return Value:
    LPSTR  -   Current program Name or NULL.
--*/
{
    if ( _tcsicmp(m_CurrentExePath, m_pszNoProgramLoaded) ) {

        //
        // A program has been loaded
        //
        return m_CurrentExePath;

    } else {
        
        //
        // No program loaded
        //
        if (bReturnUntitled) {
            return m_pszNoProgramLoaded;
        } else {
            return NULL;
        }
    }
}

int
CFinal_Windbg_WKSP::
Save_WkSp_Prompt()
{
    int nRes = IDNO;
    BOOL bSave = FALSE;

    if (!g_contGlobalPreferences_WkSp.m_bAlwaysSaveWorkspace) {
        
        bSave = FALSE;

    } else {
        
        if (g_contGlobalPreferences_WkSp.m_bPromptBeforeSavingWorkspace) {
            
            // Ask user whether to save or not            
            nRes = QuestionBox(DLG_AskToSaveWorkspace, MB_YESNOCANCEL);
            bSave = (IDYES == nRes);

        } else {

            bSave = TRUE;

        }
    }

    if (bSave) {
        g_Windbg_WkSp.Save(FALSE, FALSE);
    } else {
        // Only save out the mirrored information.
        g_Windbg_WkSp.Save(TRUE, FALSE);
    }

    return nRes;
}


void
CFinal_Windbg_WKSP::
SaveAs_WinLayout_Prompt()
{
    INT_PTR nRes = StartDialog( DLG_WINDOW_LAYOUT_SAVEAS, DlgProc_WindowLayout_SaveAs );

    if (-1 == nRes) {
        PTSTR pszErr = WKSP_FormatLastErrorMessage();
        if (!pszErr) {
            pszErr = "Unable to obtain last error code.";
        }
        ErrorBox(ERR_Cannot_Create_Dlg, pszErr);
    }
}


void
CFinal_Windbg_WKSP::
Manage_WinLayout_Prompt()
{
    INT_PTR nRes = StartDialog( IDD_DLG_MNG_WINLAYOUT, DlgProc_WindowLayout_Manage );

    if (-1 == nRes) {
        PTSTR pszErr = WKSP_FormatLastErrorMessage();
        if (!pszErr) {
            pszErr = "Unable to obtain last error code.";
        }
        ErrorBox(ERR_Cannot_Create_Dlg, pszErr);
    }
}

void
CFinal_Windbg_WKSP::
SaveAs_WkSp_Prompt()
{
    INT_PTR nRes = StartDialog( DLG_WRKSPC_SAVEAS, DlgProc_WorkSpace_SaveAs );

    if (-1 == nRes) {
        PTSTR pszErr = WKSP_FormatLastErrorMessage();
        if (!pszErr) {
            pszErr = "Unable to obtain last error code.";
        }
        ErrorBox(ERR_Cannot_Create_Dlg, pszErr);
    }
}

void
CFinal_Windbg_WKSP::
New_WkSp_Prompt(
    BOOL bPrompt
    )
{
    Assert( ExitingDebugger || !(LptdCur && LptdCur->tstate == tsRunning));

    if ( bPrompt ) {
        if ( IDNO == QuestionBox(SYS_New_Workspace_Prompt, MB_YESNO) ) {
            return;
        }
    }

    CFinal_Windbg_WKSP * pTmp = new CFinal_Windbg_WKSP;

    Duplicate(*pTmp);

    delete(pTmp);
}

void
CFinal_Windbg_WKSP::
Manage_WkSp_Prompt()
{
    INT_PTR nRes = StartDialog(IDD_DLG_MNG_WRKSPC, DlgProc_WorkSpace_Manage);

    if (-1 == nRes) {
        PTSTR pszErr = WKSP_FormatLastErrorMessage();
        if (!pszErr) {
            pszErr = "Unable to obtain last error code.";
        }
        ErrorBox(ERR_Cannot_Create_Dlg, pszErr);
    }
}

CDebuggee_WKSP *
CFinal_Windbg_WKSP::
DoesDebuggeeExist(
    LPSTR   pszProgramName
    )
/*++
Routine Description:
    Determines if a workspace exists

Arguments:
    pszProgramName -   Supplies name of program. NULL for debugger default
    WorkSpace   -   Supplies name of workspace. NULL ONLY for the
                    debugger default (NOT for program default)

    NOTE: Either

Return Value:
    BOOL    -   TRUE if default exists
--*/
{
    Assert(pszProgramName);

    // Find executable first
    TListEntry<CDebuggee_WKSP *> * pDebuggee_ListEntry =
        g_contAllDebuggees_WkSp.m_listConts.
        Find(pszProgramName, WKSP_Generic_CmpRegName);
    
    if (pDebuggee_ListEntry) {
        // Found
        Assert(pDebuggee_ListEntry->m_tData);
        return pDebuggee_ListEntry->m_tData;
    } else {
        // Not found
        return NULL;
    } 
}


CWorkSpace_WKSP *
CFinal_Windbg_WKSP::
DoesWorkSpaceExist(
    CDebuggee_WKSP * pCDebuggee,
    LPSTR   pszWorkSpace
    )
/*++
Routine Description:
    Determines if a workspace exists

Arguments:
    pszProgramName -   Supplies name of program. NULL for debugger default
    WorkSpace   -   Supplies name of workspace. NULL ONLY for the
                    debugger default (NOT for program default)

    NOTE: Either

Return Value:
    BOOL    -   TRUE if default exists
--*/
{
    Assert(pCDebuggee);
    Assert(pszWorkSpace);
    Assert(*pszWorkSpace);

    TListEntry<CWorkSpace_WKSP *> * pWorkSpace_ListEntry = pCDebuggee->
        m_listConts.Find(pszWorkSpace, WKSP_Generic_CmpRegName);

    if (pWorkSpace_ListEntry) {
        // Found
        return pWorkSpace_ListEntry->m_tData;
    } else {
        return NULL;
    }
}


void
CFinal_Windbg_WKSP::
Save_File_MRU_List()
/*++
Routine Description:
    Saves the MRU list to the registry.

Arguments:
    Hkey        -   Supplies the registry key
    MruName     -   Supplies the MRU name
    WhatFile    -   Supplies the kind of file (Editor/Project)

Return Value:
    BOOL    - TRUE if MRU list loaded
--*/
{
/*
    char   *s;
    ULONG   i;
    LPSTR   List        = NULL;
    DWORD   ListLength  = 0;
    BOOL    Ok          = TRUE;

    //
    //  Get current MRU list
    //
    for ( i = nbFilesKept[ WhatFile ]; Ok && (i > 0); i-- ) {
        Dbg(s = (LPSTR)GlobalLock( hFileKept[ WhatFile ][ i-1 ] ) );
        Ok = AddToMultiString( &List, &ListLength, s );
        Dbg(GlobalUnlock ( hFileKept[ WhatFile ][ i-1 ] ) == FALSE);
    }

    //
    //  Save the List in the registry
    //
    if ( Ok && List ) {
        if ( RegSetValueEx(Hkey, MruName, 0, REG_MULTI_SZ, (PUCHAR) List, ListLength) != NO_ERROR ) {
            Ok = FALSE;
        }
    }

    if ( List ) {
        DeallocateMultiString( List );
    }
*/
}

void
CFinal_Windbg_WKSP::
Load_File_MRU_List()
/*++
Routine Description:
    Loads an MRU list from the registry.

Arguments:
    Hkey        -   Supplies the registry key
    MruName     -   Supplies the MRU name
    WhatFile    -   Supplies the kind of file (Editor/Project)
    WhatMenu    -   Supplies the kind of menu
    WhatPosition-   Supplies the position

Return Value:
    BOOL    - TRUE if MRU list loaded
--*/
{
/*
    LPSTR   Name;
    LPSTR   List;
    DWORD   DataSize;
    BOOL    Ok       = FALSE;
    DWORD   Next     = 0;

    if ( List = LoadMultiString( Hkey, MruName, &DataSize ) ) {
        while ( Name = GetNextStringFromMultiString(List, DataSize, &Next ) ) {
            //
            //  Add the entries to the MRU list
            //
            InsertKeptFileNames( (WORD)WhatFile, (int)WhatMenu, Name );
        }
        Ok = TRUE;
        DeallocateMultiString( List );
    }
    return Ok;
*/
}



PCSTR 
CFinal_Windbg_WKSP::
GetSelected_TL_Dll_Name()
{
    // Make sure that the TL exists
    if (!g_contWorkspace_WkSp.m_pszSelectedTL) {
        return NULL;
    } else {
        Assert(*g_contWorkspace_WkSp.m_pszSelectedTL);

        // Find the specified TL
        TListEntry<CIndiv_TL_WKSP *> * pContEntry
            = g_dynacontAll_TLs_WkSp.m_listConts.
            Find(g_contWorkspace_WkSp.m_pszSelectedTL, WKSP_Generic_CmpRegName);

        if (!pContEntry) {
            return NULL;
        } else {
            CIndiv_TL_WKSP * pTL = pContEntry->m_tData;

            Assert(pTL);

            return pTL->m_pszDll;
        }
    }
}

PCSTR 
CFinal_Windbg_WKSP::
GetSelected_TL_Params()
{
    // Make sure that the TL exists
    if (!g_contWorkspace_WkSp.m_pszSelectedTL) {
        return NULL;
    } else {
        Assert(*g_contWorkspace_WkSp.m_pszSelectedTL);

        // Find the specified TL
        TListEntry<CIndiv_TL_WKSP *> * pContEntry
            = g_dynacontAll_TLs_WkSp.m_listConts.
            Find(g_contWorkspace_WkSp.m_pszSelectedTL, WKSP_Generic_CmpRegName);

        if (!pContEntry) {
            return NULL;
        } else {
            CIndiv_TL_WKSP * pTL = pContEntry->m_tData;

            Assert(pTL);

            return pTL->m_pszParams;
        }
    }
}

PSTR 
CFinal_Windbg_WKSP::
GetDefaultWorkSpaceForDebuggee(
    PSTR pszDebuggeeName
    )
{
    return NULL;
}


BOOL 
CFinal_Windbg_WKSP::
ShuttingDown()
{
    Assert( ExitingDebugger || !(LptdCur && LptdCur->tstate == tsRunning));

    BOOL    bCancel = TRUE;


    Assert( ExitingDebugger || !(LptdCur && LptdCur->tstate == tsRunning));


    bCancel = IDCANCEL == Save_WkSp_Prompt();

    return bCancel;
}

void
CFinal_Windbg_WKSP::
Restore(
    BOOL bOnlyItems
    )
{
    CBase_Windbg_WKSP::Restore(bOnlyItems);

    LoadAllWindows();

    //
    // Put the wrkspc info into the debugger
    //

    //
    // Set the root name mappings
    //
    SetRootNameMappings(g_contPaths_WkSp.m_pszRootMappingPairs, 
                        WKSP_MultiStrSize(g_contPaths_WkSp.m_pszRootMappingPairs)
                        );

    //
    // Breakpoints
    //
    if (g_contWorkspace_WkSp.m_pszBreakPoints) {

        PTSTR pszBrkPt = g_contWorkspace_WkSp.m_pszBreakPoints;
        for(; *pszBrkPt; pszBrkPt += _tcslen(pszBrkPt) +1) {
            BPSTATUS    bpstatus;
            BOOL        bDisabled;
            HBPT        hBpt;

            Assert(_tcslen(pszBrkPt) >= 3);

            bDisabled = (_T('D') == *pszBrkPt);
            
            // skip over the enabled and instantiated chars
            pszBrkPt += 3;
            
            bpstatus = BPParse( &hBpt, pszBrkPt, NULL, NULL,
                LppdCur ? LppdCur->hpid : 0);
            
            if ((BPNOERROR == bpstatus) && BPNOERROR == BPAddToList(hBpt, -1)) {
                if ( bDisabled ) {
                    BPDisable(hBpt);
                } else if ( DebuggeeActive() ) {
                    BPBindHbpt( hBpt, NULL );
                }
                Dbg(BPCommit() == BPNOERROR);            
            }
        }
    }


    //
    // Exceptions
    //
    {
        EXCEPTION_LIST *pExceptionList = NULL;
        TListEntry<CIndiv_Excep_WKSP *> * pContEntry;

        // Add exceptions from the master list
        pContEntry = g_contExceptionsMasterList_WkSp.m_listConts.FirstEntry();
        for (; pContEntry != g_contExceptionsMasterList_WkSp.m_listConts.Stop(); 
            pContEntry = pContEntry->Flink) {

            if (!g_dynacontAllExceptions_WkSp.m_listConts.
                Find(pContEntry->m_tData->m_pszRegistryName, WKSP_Generic_CmpRegName)) {
                
                // This exception wasn't in this list. Add it.
                CIndiv_Excep_WKSP * pIndExc = new CIndiv_Excep_WKSP;
                pIndExc->Init(&g_dynacontAllExceptions_WkSp, NULL);
                // Duplicate will copy all of the necessary values
                pIndExc->Duplicate(*pContEntry->m_tData);
                pIndExc->m_bMirror = g_dynacontAllExceptions_WkSp.m_bMirror;
            }
        }

        // Add all exception to the list
        pContEntry = g_dynacontAllExceptions_WkSp.m_listConts.FirstEntry();
        for (; pContEntry != g_dynacontAllExceptions_WkSp.m_listConts.Stop(); 
            pContEntry = pContEntry->Flink) {
            
            CIndiv_Excep_WKSP * pIndExc = pContEntry->m_tData;
            
            EXCEPTION_LIST * pException = (EXCEPTION_LIST *)
                calloc( sizeof( EXCEPTION_LIST), 1 );
            
            pException->dwExceptionCode = strtoul(pIndExc->m_pszRegistryName, NULL, 16);
            pException->efd             = (_EXCEPTION_FILTER_DEFAULT) pIndExc->m_dwAction;
            if (pIndExc->m_pszName) {
                pException->lpName = _strdup(pIndExc->m_pszName);
            }
            if (pIndExc->m_pszFirstCmd) {
                pException->lpCmd = _strdup(pIndExc->m_pszFirstCmd);
            }
            if (pIndExc->m_pszSecondCmd) {
                pException->lpCmd2 = _strdup(pIndExc->m_pszSecondCmd);
            }
            
            InsertException( &pExceptionList, pException );
        }            
        
        //
        // If there was an error getting the new list, let's just use
        // the current list. Potential errors can arrive from not being 
        // able to parse the exception string, or bad data in the registry
        //
        if (pExceptionList) {
            
            //
            // Do we have a valid exception?
            if (0 == pExceptionList->dwExceptionCode || 0 == pExceptionList->next) {
                // Bad exception
                free(pExceptionList);
                pExceptionList = NULL;
            } else {
                
                //
                // Now that we have a new list, let's update the default one
                //
                
                // Free current default exception list.
                while( DefaultExceptionList ) {
                    
                    EXCEPTION_LIST *pException = DefaultExceptionList;
                    DefaultExceptionList = DefaultExceptionList->next;
                    
                    FREE_STR( pException->lpName );                    
                    FREE_STR( pException->lpCmd );
                    FREE_STR( pException->lpCmd2 );
                    
                    ZeroMemory(pException, sizeof(EXCEPTION_LIST));
                    
                    free( pException );
                }
                
                DefaultExceptionList = pExceptionList;
                
                // 
                // The exception list may already be in use by process 0. This scenario
                // occurs when you attach to a process, then load a workspace. We have 
                // to reset the exception lists for the all of the processes
                //
                if (DebuggeeActive()) {
                    
                    LPPD lppdTmp = GetLppdHead();
                    Assert(lppdTmp != NULL);
                    
                    for ( ;lppdTmp != NULL; lppdTmp = lppdTmp->lppdNext) {
                        if (lppdTmp->pstate == psDestroyed) {
                            continue;
                        }
                        ClearProcessExceptions(lppdTmp);
                        SetProcessExceptions(lppdTmp);
                    }
                }
            }
        }            
    }
}


void
CFinal_Windbg_WKSP::
Save(
    BOOL bOnlySaveMirror,
    BOOL bOnlyItems
    )
{
    //
    // Breakpoints
    //
    {
        _TCHAR sz[2000]; // This should be big enough to hold any breakpoint
        HBPT hBpt = NULL;

        FREE_STR(g_contWorkspace_WkSp.m_pszBreakPoints);
        
        Dbg(BPNextHbpt(&hBpt, bptNext) == BPNOERROR);
        while (hBpt != NULL) {
            Dbg(BPFormatHbpt( hBpt, sz, sizeof(sz), 
                BPFCF_WNDPROC | BPFCF_WRKSPACE ) == BPNOERROR);
            
            WKSP_AppendStrToMultiStr(g_contWorkspace_WkSp.m_pszBreakPoints, sz);

            Dbg(BPNextHbpt(&hBpt, bptNext) == BPNOERROR);
        }
    }

    //
    // Exceptions
    //
    {
        //
        //  Note that we only save one exception list: The default
        //  exception list. This exception list corresponds to
        //  process 0. If other processes have different exception
        //  lists, they will be lost.
        //
        EXCEPTION_LIST *pExceptionList = DefaultExceptionList;
        //TListEntry<CIndiv_Excep_WKSP *> * pContEntry;

        // Delete the old list of exceptions
        g_dynacontAllExceptions_WkSp.m_listConts.ClearList();


        //
        //  If we don't have an exception list, we must load the EM and
        //  initialize the default exception list.
        //
        if ( !LppdCur && !DefaultExceptionList ) {
            if ( GetDefaultExceptionList() ) {
                pExceptionList = DefaultExceptionList;
            }
        }

        for (; pExceptionList; pExceptionList = pExceptionList->next) {

            // Rebuild the list
            _TCHAR szRegName[9] = {0};
            CIndiv_Excep_WKSP * pIndExc;

            pIndExc = new CIndiv_Excep_WKSP();
            pIndExc->Init(&g_dynacontAllExceptions_WkSp, NULL);

            _ultot(pExceptionList->dwExceptionCode, szRegName, 16);
            Assert(_tcslen(szRegName) * sizeof(_TCHAR) < sizeof(szRegName));
            pIndExc->SetRegistryName(_tcsdup(szRegName));
            
            pIndExc->m_dwAction = pExceptionList->efd;
            
            if (pExceptionList->lpName) {
                pIndExc->m_pszName = _strdup(pExceptionList->lpName);
            }
            if (pExceptionList->lpCmd) {
                pIndExc->m_pszFirstCmd = _strdup(pExceptionList->lpCmd);
            }
            if (pExceptionList->lpCmd2) {
                pIndExc->m_pszSecondCmd = _strdup(pExceptionList->lpCmd2);
            }

            // Make sure the main exception list contains any new exceptions
            if (!g_contExceptionsMasterList_WkSp.m_listConts.
                Find(pIndExc->m_pszRegistryName, WKSP_Generic_CmpRegName)) {
                
                // This exception wasn't in this list. Add it.
                CIndiv_Excep_WKSP * pIndExcCopy = new CIndiv_Excep_WKSP;

                // Add it to the list
                pIndExcCopy->Init(&g_contExceptionsMasterList_WkSp, NULL);
                // Duplicate will copy all of the necessary values
                pIndExcCopy->Duplicate(*pIndExc);
                pIndExcCopy->m_bMirror = g_contExceptionsMasterList_WkSp.m_bMirror;
            }
        }
    }

    SaveAllWindows();

    CBase_Windbg_WKSP::Save(bOnlySaveMirror, bOnlyItems);
}


HTREEITEM 
AddItemToTree(
    HWND    hwndTV, 
    LPSTR   lpszItem, 
    int     nLevel,
    LPARAM  lparam
    )
/*++
Description
    Adds items to a tree view control.     
    
Arguments      
    hwndTV - handle to the tree view control. 
    lpszItem - text of the item to add. 
    nLevel - level at which to add the item. Only levels 1-3 are supported
        1 - root
        2 - level 1
        3 - level 2

Returns
    the handle to the newly added item. 
--*/
{ 
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    static HTREEITEM hPrev = (HTREEITEM) TVI_FIRST; 
    static HTREEITEM hPrevRootItem = NULL; 
    static HTREEITEM hPrevLev2Item = NULL; 
    HTREEITEM hti; 
 
    tvi.mask = TVIF_TEXT | TVIF_PARAM; 
 
    // Set the text of the item. 
    tvi.pszText = lpszItem; 
    tvi.cchTextMax = lstrlen(lpszItem); 
    tvi.lParam = (LPARAM) lparam; 
 
    tvins.item = tvi; 
    tvins.hInsertAfter = hPrev; 
 
    // Set the parent item based on the specified level. 
    if (nLevel == 1) {
        tvins.hParent = TVI_ROOT; 
    } else if (nLevel == 2) {
        tvins.hParent = hPrevRootItem; 
    } else  {
        tvins.hParent = hPrevLev2Item; 
    }
 
    // Add the item to the tree view control. 
    hPrev = (HTREEITEM) SendMessage(hwndTV, 
                                    TVM_INSERTITEM, 
                                    0, 
                                    (LPARAM) (LPTVINSERTSTRUCT) &tvins); 
 
    // Save the handle to the item. 
    if (nLevel == 1) {
        hPrevRootItem = hPrev; 
    } else if (nLevel == 2) {
        hPrevLev2Item = hPrev; 
    }
 
    return hPrev; 
} 
 
void
PopulateWindowLayoutTree(
    HWND hwndTree
    )
{
    HKEY                hkey;
    CRegEntry          *pRegEntry;
    CRegEntry          *pWS_RegEntry; // work space
    TList< CRegEntry* > list;

    // Delete the contents of the tree
    TreeView_DeleteAllItems(hwndTree);

    hkey = g_contAllWinLayouts_WkSp.GetRegistryKey();

    if ( !hkey ) {
        return;
    }

    if ( !WKSP_RegEnumerate(hkey, &list, NULL, TRUE) ) {
        return;
    }

    while ( !list.IsEmpty() ) {
    
        pRegEntry = list.GetHeadData();
        list.RemoveHead();
                    
        // Add item at the root level
        AddItemToTree(hwndTree,
                      pRegEntry->m_pszName, 
                      1,
                      NULL
                      );
       
        // Delete it without cleaning up since we'll do this manually
        pRegEntry->CleanUp();
        delete pRegEntry;
    }
}

void
PopulateExeWorkspaceTree(
    HWND hwndTree
    )
{
    HKEY                hkey;
    CRegEntry          *pExe_RegEntry;
    CRegEntry          *pWS_RegEntry; // work space
    TList< CRegEntry* > listExes;
    TList< CRegEntry* > listWrkSpcs;

    // Delete the contents of the tree
    TreeView_DeleteAllItems(hwndTree);

    hkey = g_contAllDebuggees_WkSp.GetRegistryKey();

    if ( !hkey ) {
        return;
    }

    if ( !WKSP_RegEnumerate(hkey, &listExes, NULL, TRUE) ) {
        return;
    }

    while ( !listExes.IsEmpty() ) {
    
        pExe_RegEntry = listExes.GetHeadData();
        listExes.RemoveHead();
                    
        // Add item at the root level
        AddItemToTree(hwndTree,
                      pExe_RegEntry->m_pszName, 
                      1,
                      NULL
                      );

        //
        // Get the WrkSpc list
        //
        if ( WKSP_RegEnumerate(pExe_RegEntry->m_hkey, &listWrkSpcs, NULL, TRUE) ) {
            
            while ( !listWrkSpcs.IsEmpty() ) {

                pWS_RegEntry = listWrkSpcs.GetHeadData();
                listWrkSpcs.RemoveHead();

                // Add item at the level 2
                AddItemToTree(hwndTree,
                              pWS_RegEntry->m_pszName, 
                              2,
                              NULL
                              );
                
                pWS_RegEntry->CleanUp();
                delete pWS_RegEntry;
            }

        }
        
        
        // Delete it without cleaning up since we'll do this manually
        pExe_RegEntry->CleanUp();
        delete pExe_RegEntry;
    }
}

INT_PTR
CALLBACK 
DlgProc_WorkSpace_Manage(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++
Description:
    Used to manage workspaces. Rename, delete, copy and paste workspaces from to EXE.

    See description for dlg procs.
--*/
{
#if 0 // Not yet enabled
    static DWORD HelpArray[]=
    {
       IDC_LIST1, IDH_TRANSPORT,
       IDC_STEXT_SELECT, IDH_TLSELECT,
       IDC_BUT_SELECT, IDH_TLSELECT,
       IDC_BUT_ADD, IDH_TLADD,
       IDC_BUT_EDIT, IDH_TLEDIT,
       IDC_BUT_DELETE, IDH_TLDEL,
       0, 0
    };

    static DWORD NOEDIT_HelpArray[]=
    {
       IDC_LIST1, IDH_TRANSPORT_DISABLED,
       IDC_STEXT_SELECT, IDH_TRANSPORT_DISABLED,
       IDC_BUT_SELECT, IDH_TRANSPORT_DISABLED,
       IDC_BUT_ADD, IDH_TRANSPORT_DISABLED,
       IDC_BUT_EDIT, IDH_TRANSPORT_DISABLED,
       IDC_BUT_DELETE, IDH_TRANSPORT_DISABLED,
       0, 0
    };
#endif

    HWND hwndTree = GetDlgItem(hDlg, IDC_TREE_WRKSPC);

    switch (uMsg) {
        
    default:
        return FALSE;

#if 0 // Not yet enabled
    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle,
                "windbg.hlp",
                HELP_WM_HELP,
                (ULONG_PTR) pdwHelpArray );
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam,
                "windbg.hlp",
                HELP_CONTEXTMENU,
                (ULONG_PTR) pdwHelpArray );
        return TRUE;
#endif

    case WM_INITDIALOG:
        PopulateExeWorkspaceTree(hwndTree);
        return TRUE;

    case WM_COMMAND:
        {
            WORD    wNotifyCode = HIWORD(wParam);  // notification code 
            WORD    wID = LOWORD(wParam);          // item, control, or accelerator identifier 
            HWND    hwndCtl = (HWND) lParam;       // handle of control 
            
            TCHAR               szExeName[_MAX_PATH];
            TCHAR               szWrkSpcName[_MAX_PATH];
            HTREEITEM           htiSel = NULL;
            HTREEITEM           htiParent = NULL;
            CDebuggee_WKSP      *pTmpDebuggee_WkSp = NULL;
            CWorkSpace_WKSP     *pTmpWorkspace_WkSp = NULL;


            //
            // Whack any garbage
            //
            *szExeName = NULL;
            *szWrkSpcName = NULL;

            //
            // Get the name of the currently selected EXE and set the name of
            // the WORKSPACE to NULL
            //
            // or
            //
            // Get the name of the currently slected WORKSPACE and get the name of 
            // parent EXE the
            //

            htiSel = TreeView_GetSelection(hwndTree) ;
            if ( htiSel ) {

                //
                // Success, but what has been selected? An EXE or a WORKSPACE?
                //
                TVITEM tvi;

                //
                // Assume we are retrieving the name of a workspace
                //
                tvi.mask = TVIF_HANDLE | TVIF_TEXT;
                tvi.hItem = htiSel;
                tvi.pszText = szWrkSpcName;
                tvi.cchTextMax = _tsizeof(szWrkSpcName);

                TreeView_GetItem(hwndTree, &tvi);

                //
                // Get it's parent if it has one
                //
                htiParent = TreeView_GetParent(hwndTree, htiSel);
                if ( !htiParent ) {

                    //
                    // No parent then we have an EXE
                    //                    
                    _tcscpy(szExeName, szWrkSpcName);
                    *szWrkSpcName = NULL;

                } else {

                    //
                    // If it has a parent then it's a WORKSPACE, get the EXE name
                    //
                    tvi.mask = TVIF_HANDLE | TVIF_TEXT;
                    tvi.hItem = htiParent;
                    tvi.pszText = szExeName;
                    tvi.cchTextMax = _tsizeof(szExeName);

                    TreeView_GetItem(hwndTree, &tvi);

                }
            }

            switch (wID) {
                
            default:
                return FALSE;
            
            case IDOK:
                //
                // Open a workspace
                //
                if ( !*szWrkSpcName || !*szExeName ) {
                    // Must have a WORKSPACE & EXE selected
                    return TRUE;
                }

                g_contWorkspace_WkSp.CloseRegistryKeys();

                if ( !_tcsicmp(szExeName, g_contDebuggee_WkSp.m_pszRegistryName) ) {

                    g_contWorkspace_WkSp.SetRegistryName(szWrkSpcName);
                    g_contWorkspace_WkSp.Restore(FALSE); 

                } else {

                    pTmpDebuggee_WkSp = new CDebuggee_WKSP;
                    pTmpDebuggee_WkSp->SetRegistryName(szExeName);
                    pTmpDebuggee_WkSp->SetParent( &g_contAllDebuggees_WkSp );

                    pTmpWorkspace_WkSp = &pTmpDebuggee_WkSp->m_contWorkSpace;
                    pTmpWorkspace_WkSp->SetRegistryName(szWrkSpcName);

                    pTmpWorkspace_WkSp->Restore(TRUE);

                    g_contWorkspace_WkSp.Duplicate( *pTmpWorkspace_WkSp );

                    delete pTmpDebuggee_WkSp;
                
                }
                //
                // Fall through
                //
                

            case IDCANCEL:
                //
                // Resave the mirrored info since we have overwritten by the changes we've made
                //
                {
                    HCURSOR hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    g_Windbg_WkSp.Save(TRUE, FALSE);
                    SetCursor(hcurOld);
                }
                
                EndDialog(hDlg, wID); // Return whatever the user pressed.
                return TRUE;

            case IDC_BUT_COPY:
                InformationBox(ERR_Not_YetImplemented);
                
                if ( !*szWrkSpcName && !*szExeName ) {
                    // Must have a EXE or WORKSPACE or EXE selected
                    return TRUE;
                }
                return TRUE;

            case IDC_BUT_PASTE:
                InformationBox(ERR_Not_YetImplemented);
                return TRUE;

            case IDC_BUT_RENAME:
                InformationBox(ERR_Not_YetImplemented);
                return TRUE;

            case IDC_BUT_DELETE:
                {
                    HKEY hkeyParent;
                    HKEY hkeyExe;
                    HKEY hkeyWS;

                    if ( !*szWrkSpcName && !*szExeName ) {
                        // Must have a WORKSPACE or EXE selected
                        return TRUE;
                    }

                    hkeyParent = g_contAllDebuggees_WkSp.GetRegistryKey();                    
                    if ( !hkeyParent ) {
                        return TRUE;
                    }

                    //
                    // Close all registry keys just to be on the safe side
                    //
                    g_contDebuggee_WkSp.CloseRegistryKeys();

                    if ( !*szWrkSpcName ) {
                        
                        //
                        // Delete the EXE
                        //
                        if ( !WKSP_RegDeleteKey(hkeyParent, szExeName) 
                            || !TreeView_DeleteItem(hwndTree, htiSel) ) {

                            ErrorBox(ERR_Deleting_Registry_Key);
                            // All bets are off, repopulate the tree
                            PopulateExeWorkspaceTree(hwndTree);

                        }

                    } else {
                        
                        //
                        // Delete the workspace
                        //
                        LONG lRes;
                        
                        //
                        // Open the EXE
                        //
                        lRes = RegOpenKeyEx(hkeyParent, 
                                            szExeName,
                                            0, 
                                            KEY_ALL_ACCESS, 
                                            &hkeyExe
                                            );
                        if (ERROR_SUCCESS != lRes) {
                            // All bets are off, repopulate the tree
                            PopulateExeWorkspaceTree(hwndTree);
                            return FALSE;
                        }
                    
                        //
                        // Delete the workspace
                        //
                        if ( !WKSP_RegDeleteKey(hkeyExe, szWrkSpcName) 
                            || !TreeView_DeleteItem(hwndTree, htiSel) ) {

                            ErrorBox(ERR_Deleting_Registry_Key);
                            // All bets are off, repopulate the tree
                            PopulateExeWorkspaceTree(hwndTree);
                        }

                        RegCloseKey(hkeyExe);
                    }
                }
                return TRUE;

            case IDC_BUT_MAKE_DEFAULT:
                //
                // Make the currently selected workspace the default one for this EXE
                //
                if ( !*szWrkSpcName || !*szExeName ) {
                    // Must have a WORKSPACE & EXE selected
                    return TRUE;
                }
                
                // Is it for this EXE?
                if ( !_tcsicmp(szExeName, g_contDebuggee_WkSp.m_pszRegistryName) ) {
                    
                    FREE_STR(g_contDebuggee_WkSp.m_pszNameOfPreferredWorkSpace);
                    g_contDebuggee_WkSp.m_pszNameOfPreferredWorkSpace = _tcsdup(szExeName);
                    //
                    // Save change, also save to the mirror, but only the data items
                    //
                    g_contDebuggee_WkSp.Save(FALSE, TRUE); 

                } else {

                    pTmpDebuggee_WkSp = new CDebuggee_WKSP;
                    pTmpDebuggee_WkSp->SetRegistryName(szExeName);
                    pTmpDebuggee_WkSp->SetParent( &g_contAllDebuggees_WkSp );
                    pTmpDebuggee_WkSp->Restore(TRUE);

                    FREE_STR(pTmpDebuggee_WkSp->m_pszNameOfPreferredWorkSpace);
                    pTmpDebuggee_WkSp->m_pszNameOfPreferredWorkSpace = _tcsdup(szExeName);
                    //
                    // Save change, also save to the mirror.
                    // but only save the data items
                    //
                    pTmpDebuggee_WkSp->Save(FALSE, TRUE); 

                    // Let's reset the mirror to our stuff.
                    g_contDebuggee_WkSp.Save(FALSE, TRUE); 
                }

                return TRUE;

            case IDC_BUT_NEW:
                InformationBox(ERR_Not_YetImplemented);
                return TRUE;
            }
        }
    }
}





INT_PTR
CALLBACK 
DlgProc_WindowLayout_Manage(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++
Description:
    Used to manage workspaces. Rename, delete, copy and paste workspaces from to EXE.

    See description for dlg procs.
--*/
{
#if 0 // Not yet enabled
    static DWORD HelpArray[]=
    {
       IDC_LIST1, IDH_TRANSPORT,
       IDC_STEXT_SELECT, IDH_TLSELECT,
       IDC_BUT_SELECT, IDH_TLSELECT,
       IDC_BUT_ADD, IDH_TLADD,
       IDC_BUT_EDIT, IDH_TLEDIT,
       IDC_BUT_DELETE, IDH_TLDEL,
       0, 0
    };

    static DWORD NOEDIT_HelpArray[]=
    {
       IDC_LIST1, IDH_TRANSPORT_DISABLED,
       IDC_STEXT_SELECT, IDH_TRANSPORT_DISABLED,
       IDC_BUT_SELECT, IDH_TRANSPORT_DISABLED,
       IDC_BUT_ADD, IDH_TRANSPORT_DISABLED,
       IDC_BUT_EDIT, IDH_TRANSPORT_DISABLED,
       IDC_BUT_DELETE, IDH_TRANSPORT_DISABLED,
       0, 0
    };
#endif

    HWND hwndTree = GetDlgItem(hDlg, IDC_TREE_WRKSPC);

    switch (uMsg) {
        
    default:
        return FALSE;

#if 0 // Not yet enabled
    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle,
                "windbg.hlp",
                HELP_WM_HELP,
                (ULONG_PTR) pdwHelpArray );
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam,
                "windbg.hlp",
                HELP_CONTEXTMENU,
                (ULONG_PTR) pdwHelpArray );
        return TRUE;
#endif

    case WM_INITDIALOG:
        //
        // Set the extended style
        //
        PopulateWindowLayoutTree(hwndTree);
        return TRUE;

    case WM_COMMAND:
        {
            WORD    wNotifyCode = HIWORD(wParam);  // notification code 
            WORD    wID = LOWORD(wParam);          // item, control, or accelerator identifier 
            HWND    hwndCtl = (HWND) lParam;       // handle of control 
            
            TCHAR               szLayoutName[_MAX_PATH];
            CWinLayout_WKSP     *pTmpWinLayout = NULL;
            HTREEITEM           htiSel;


            //
            // Whack any garbage
            //
            *szLayoutName = NULL;

            //
            // Get the name of the currently selected EXE and set the name of
            // the WORKSPACE to NULL
            //
            // or
            //
            // Get the name of the currently slected WORKSPACE and get the name of 
            // parent EXE the
            //

            htiSel = TreeView_GetSelection(hwndTree) ;
            if ( htiSel ) {

                //
                // Success, but what has been selected? An EXE or a WORKSPACE?
                //
                TVITEM tvi;

                //
                // Assume we are retrieving the name of a workspace
                //
                tvi.mask = TVIF_HANDLE | TVIF_TEXT;
                tvi.hItem = htiSel;
                tvi.pszText = szLayoutName;
                tvi.cchTextMax = _tsizeof(szLayoutName);

                TreeView_GetItem(hwndTree, &tvi);
            }

            switch (wID) {
                
            default:
                return FALSE;
            
            case IDOK:
                //
                // Open a window layout
                //

                g_contWinLayout_WkSp.CloseRegistryKeys();

                if ( !_tcsicmp(szLayoutName, g_contWinLayout_WkSp.m_pszRegistryName) ) {

                    g_contWinLayout_WkSp.Restore(FALSE); 

                    LoadAllWindows();

                } else {

                    g_contWinLayout_WkSp.SetRegistryName(szLayoutName);
                    g_contWinLayout_WkSp.Restore(FALSE);
                
                    LoadAllWindows();
                }
                //
                // Fall through
                //
                

            case IDCANCEL:
                EndDialog(hDlg, wID); // Return whatever the user pressed.
                return TRUE;

            case IDC_BUT_COPY:
                InformationBox(ERR_Not_YetImplemented);
                return TRUE;

            case IDC_BUT_PASTE:
                InformationBox(ERR_Not_YetImplemented);
                return TRUE;

            case IDC_BUT_RENAME:
                InformationBox(ERR_Not_YetImplemented);
                return TRUE;

            case IDC_BUT_DELETE:
                if ( *szLayoutName ) {
                    HKEY hkeyParent;
                    HKEY hkeyExe;
                    HKEY hkeyWS;


                    hkeyParent = g_contAllWinLayouts_WkSp.GetRegistryKey();                    
                    if ( !hkeyParent ) {
                        return TRUE;
                    }

                    //
                    // Close all registry keys just to be on the safe side
                    //
                    g_contWinLayout_WkSp.CloseRegistryKeys();

                    if ( !*szLayoutName ) {
                        
                        //
                        // Delete the EXE
                        //
                        if ( !WKSP_RegDeleteKey(hkeyParent, szLayoutName) 
                            || !TreeView_DeleteItem(hwndTree, htiSel) ) {

                            ErrorBox(ERR_Deleting_Registry_Key);
                            // All bets are off, repopulate the tree
                            PopulateWindowLayoutTree(hwndTree);

                        }

                    }
                }
                return TRUE;

            case IDC_BUT_MAKE_DEFAULT:
                //
                // Make the currently selected workspace the default one for this EXE
                //
                InformationBox(ERR_Not_YetImplemented);
                return TRUE;

                if ( !*szLayoutName ) {
                    // Must have a WORKSPACE & EXE selected
                    return TRUE;
                }
                
                /*
                // Is it for this EXE?
                if ( !_tcsicmp(szLayoutName, g_contWinLayout_WkSp.m_pszRegistryName) ) {
                    
                    FREE_STR(m_pszLastUsedWinLayout.m_pszNameOfPreferredWorkSpace);
                    g_contWinLayout_WkSp.m_pszNameOfPreferredWorkSpace = _tcsdup(szLayoutName);
                    //
                    // Save change, also save to the mirror, but only the data items
                    //
                    g_contWinLayout_WkSp.Save(FALSE, TRUE); 

                } else {

                    pTmpDebuggee_WkSp = new CWinLayout_WKSP;
                    pTmpDebuggee_WkSp->SetRegistryName(szLayoutName);
                    pTmpDebuggee_WkSp->SetParent( &g_contAllDebuggees_WkSp );
                    pTmpDebuggee_WkSp->Restore(TRUE);

                    FREE_STR(pTmpDebuggee_WkSp->m_pszNameOfPreferredWorkSpace);
                    pTmpDebuggee_WkSp->m_pszNameOfPreferredWorkSpace = _tcsdup(szLayoutName);
                    //
                    // Save change, also save to the mirror.
                    // but only save the data items
                    //
                    pTmpDebuggee_WkSp->Save(FALSE, TRUE); 

                    // Let's reset the mirror to our stuff.
                    g_contWinLayout_WkSp.Save(FALSE, TRUE); 
                }
                */
                return TRUE;

            case IDC_BUT_NEW:
                InformationBox(ERR_Not_YetImplemented);
                return TRUE;
            }
        }
    }
}





INT_PTR
CALLBACK
DlgProc_WorkSpace_SaveAs(
    HWND    hDlg,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
/*++

Routine Description:

    Code for the Program.Load Work Space dialog

Arguments:

    Std. dialog args.

Return Value:

    None

--*/
{
    TCHAR szBuf[1024];

    static DWORD HelpArray[]=
    {
       ID_SAVEAS_NAME, IDH_WKSPNAME,
       ID_SAVEAS_MAKEDEFAULT, IDH_DEFWKSP,
       0, 0
    };

    Unreferenced( lParam );

    switch( msg ) {

    case WM_INITDIALOG:

        SetDlgItemText(hDlg, 
                       ID_SAVEAS_NAME, 
                       g_contWorkspace_WkSp.m_pszRegistryName
                       );

        SendDlgItemMessage(hDlg, 
                           ID_SAVEAS_NAME,
                           EM_SETLIMITTEXT,
                           _tsizeof(szBuf) -1, // leave room for the terminating zero
                           0
                           );

        CheckDlgButton(hDlg, 
                       ID_SAVEAS_MAKEDEFAULT,
                       BM_SETCHECK
                       );

        return TRUE;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
           (LPARAM)(LPVOID) HelpArray );
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
           (LPARAM)(LPVOID) HelpArray );
        return TRUE;

    case WM_COMMAND:
        {
            WORD    wNotifyCode = HIWORD(wParam);  // notification code 
            WORD    wID = LOWORD(wParam);          // item, control, or accelerator identifier 
            HWND    hwndCtl = (HWND) lParam;       // handle of control 

            switch( wID ) {

            case IDOK:
                {
                    PTSTR   psz;
                    BOOL    bMakeDefault;

                    GetDlgItemText(hDlg, 
                                   ID_SAVEAS_NAME, 
                                   szBuf,
                                   _tsizeof(szBuf)
                                   );
                    szBuf[_tsizeof(szBuf) -1] = 0;

                    psz = CPSkipWhitespace(szBuf);
                    if ( !*psz ) {
                        ErrorBox(ERR_Must_Supply_WrkSpc_Name);
                        return TRUE;
                    }

                    g_contWorkspace_WkSp.SetRegistryName(psz);
                    //
                    // Close existing handles to the registry and 
                    // create new ones since we now have a new name.
                    //
                    g_contWorkspace_WkSp.CloseRegistryKeys();

                    bMakeDefault = BM_SETCHECK == IsDlgButtonChecked(hDlg, ID_SAVEAS_MAKEDEFAULT );

                    if (bMakeDefault) {
                        FREE_STR(g_contDebuggee_WkSp.m_pszNameOfPreferredWorkSpace);

                        g_contDebuggee_WkSp.m_pszNameOfPreferredWorkSpace = 
                            _strdup(g_contWorkspace_WkSp.m_pszRegistryName);
                    }
                }
                g_contWorkspace_WkSp.Save(FALSE, FALSE);

                //
                // Fall thru
                //

            case IDCANCEL:
                EndDialog(hDlg, wID); // Return whatever the user pressed.
                return TRUE;
            }
        }

        break;
    }

    return FALSE;
}


INT_PTR
CALLBACK
DlgProc_WindowLayout_SaveAs(
    HWND    hDlg,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
/*++

Routine Description:

    Code for the Program.Load Work Space dialog

Arguments:

    Std. dialog args.

Return Value:

    None

--*/
{
    TCHAR szBuf[1024];

    /*
    static DWORD HelpArray[]=
    {
       ID_SAVEAS_NAME, IDH_WKSPNAME,
       ID_SAVEAS_MAKEDEFAULT, IDH_DEFWKSP,
       0, 0
    };
    */

    Unreferenced( lParam );

    switch( msg ) {

    case WM_INITDIALOG:

        SetDlgItemText(hDlg, 
                       ID_SAVEAS_NAME, 
                       g_contWinLayout_WkSp.m_pszRegistryName
                       );

        SendDlgItemMessage(hDlg, 
                           ID_SAVEAS_NAME,
                           EM_SETLIMITTEXT,
                           _tsizeof(szBuf) -1, // leave room for the terminating zero
                           0
                           );

        CheckDlgButton(hDlg, 
                       ID_SAVEAS_MAKEDEFAULT,
                       BM_SETCHECK
                       );

        return TRUE;

    /*
    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
           (DWORD)(LPVOID) HelpArray );
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
           (DWORD)(LPVOID) HelpArray );
        return TRUE;
    */

    case WM_COMMAND:

        {
            WORD    wNotifyCode = HIWORD(wParam);  // notification code 
            WORD    wID = LOWORD(wParam);          // item, control, or accelerator identifier 
            HWND    hwndCtl = (HWND) lParam;       // handle of control 

            switch( wID ) {

            case IDOK:
                {
                    PTSTR   psz;
                    BOOL    bMakeDefault;

                    GetDlgItemText(hDlg, 
                                   ID_SAVEAS_NAME, 
                                   szBuf,
                                   _tsizeof(szBuf)
                                   );
                    szBuf[_tsizeof(szBuf) -1] = 0;

                    psz = CPSkipWhitespace(szBuf);
                    if ( !*psz ) {
                        ErrorBox(ERR_Must_Supply_WrkSpc_Name);
                        return TRUE;
                    }

                    g_contWinLayout_WkSp.SetRegistryName(psz);
                    //
                    // Close existing handles to the registry and 
                    // create new ones since we now have a new name.
                    //
                    g_contWinLayout_WkSp.CloseRegistryKeys();

                    bMakeDefault = BM_SETCHECK == IsDlgButtonChecked(hDlg, ID_SAVEAS_MAKEDEFAULT );

                    /*if (bMakeDefault) {
                        FREE_STR(g_contWinLayout_WkSp.m_pszNameOfPreferredWorkSpace);

                        g_contWinLayout_WkSp.m_pszNameOfPreferredWorkSpace = 
                            _strdup(g_contWorkspace_WkSp.m_pszRegistryName);
                    }*/
                }
                SaveAllWindows();
                g_contWinLayout_WkSp.Save(FALSE, FALSE);

                //
                // Fall thru
                //

            case IDCANCEL:
                EndDialog( hDlg, wID );
                break;
            }
        }
        break;
    }

    return FALSE;
}



