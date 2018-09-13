/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    WrkSpace.h

Abstract:

    Header file for WinDbg WorkSpaces.

Author:

    Ramon J. San Andres (ramonsa)   07-July-1992
    Carlos Klapp (a-caklap)         15-July-1998

Environment:

    Win32, User Mode

--*/


//
//  these reserved image names are used to magically identify
//  kernel debugger workspaces
//
#define KD_PGM_NAME1 NT_KERNEL_NAME
#define KD_PGM_NAME2 "osloader.exe"



BOOL    LoadAllWindows(VOID);
BOOL    SaveAllWindows(VOID);


/*
LONG 
WKSPGetSetProfileProc(
    LPTSTR          KeyName,    // SubKey name (must be relative)
    LPTSTR          ValueName,  // value name
    LPDWORD         lpType,     // type of data (valid only in the case of Set)
    LPBYTE          Data,       // pointer to data
    LPDWORD         lpcbData,   // size of data (in bytes)
    BOOL            fSet,       // TRUE = setting, FALSE = getting
    LPARAM          lParam      // instance data from shell
    );

HKEY
GetCurrentWorkSpaceOptionSubkey(
    LPCTSTR SubKeyName,
    BOOL    Create
    );
*/

void WKSPSetupTL(HTL htl);
void WKSPSetupEM(HEM hem);


BOOL SetTransportLayer(PSTR pszTL_Name, CAll_TLs_WKSP * pCAll_TLs_WrkSpc);
BOOL ValidateAllTransportLayers(HWND hwndParent, CAll_TLs_WKSP * pCAll_TLs_WrkSpc);



BOOL
WKSP_Initialize();



// Following class contains variables that are not written out to the
// registry but belong to the workspace class.
class CFinal_Windbg_WKSP
: public CBase_Windbg_WKSP 
{
public:
    CFinal_Windbg_WKSP();
    virtual ~CFinal_Windbg_WKSP();


// Overriden methods
public:
    virtual void Restore(BOOL bOnlyItems);
    virtual void Save(BOOL bOnlySaveMirror, BOOL bOnlyItems);


// Flag indicating whether a save is required.
public:
    // Flag indicating that the debugger state has changed since the last save.
    // True - state has changed since the last save.
    BOOL HasStateChanged() const { return TRUE; }

public:
    void SetCurrentProgramName(PCSTR pszProgName);
    const char * GetCurrentProgramName(BOOL fReturnUntitled);

    void SetCurrentWorkSpaceName(PCSTR pszProgName);

public:
    BOOL IsProgramLoaded() { return TRUE; };





// New helper functions
public:
    virtual int Save_WkSp_Prompt();
    virtual void SaveAs_WkSp_Prompt();
    virtual void New_WkSp_Prompt(BOOL bPrompt);
    virtual void Manage_WkSp_Prompt();

    virtual void SaveAs_WinLayout_Prompt();
    virtual void Manage_WinLayout_Prompt();

    CDebuggee_WKSP * DoesDebuggeeExist(LPSTR pszProgramName);
    CWorkSpace_WKSP * DoesWorkSpaceExist(CDebuggee_WKSP * pCDebuggee, PSTR pszWorkSpace);

    void Save_File_MRU_List();
    void Load_File_MRU_List();
    

    PCSTR GetSelected_TL_Dll_Name();
    PCSTR GetSelected_TL_Params();


    PSTR GetDefaultWorkSpaceForDebuggee(PSTR pszDebuggeeName);
    PSTR GetNameOfCurrentWorkSpace();



    BOOL ShuttingDown();


    //
    // Data
    //
    PSTR m_CurrentExePath;
};


extern CFinal_Windbg_WKSP          g_Windbg_WkSp;
extern CGlobalPreferences_WKSP     &g_contGlobalPreferences_WkSp;
extern CAll_Exceptions_WKSP        &g_contExceptionsMasterList_WkSp;
extern CUser_WM_Messages_WKSP      &g_contUser_WM_Messages_WkSp;
extern CAll_TLs_WKSP               &g_dynacontAll_TLs_WkSp;

extern CAll_Debuggees_WKSP         &g_contAllDebuggees_WkSp;
extern CDebuggee_WKSP              &g_contDebuggee_WkSp;
extern CWorkSpace_WKSP             &g_contWorkspace_WkSp;

extern CAll_Exceptions_WKSP        &g_dynacontAllExceptions_WkSp;
extern CPaths_WKSP                 &g_contPaths_WkSp;
extern CKernel_Debugging_WKSP      &g_contKernelDbgPreferences_WkSp;

extern CIndivWinLayout_WKSP        &g_contFrameWindow;
extern CAllChildWindows_WKSP       &g_dynacontAllChildWindows;
extern CWinLayout_WKSP             &g_contWinLayout_WkSp;
extern CAllWindowLayouts_WKSP      &g_contAllWinLayouts_WkSp;


