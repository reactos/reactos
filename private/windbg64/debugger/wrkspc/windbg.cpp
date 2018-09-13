#include "precomp.hxx"
#pragma hdrstop


#include "cppmacro.h"


//
// WINDBG
//


//
// Transport Layer:
//
CAll_TLs_WKSP::
CAll_TLs_WKSP()
{
    const int nRows = 2;
    const int nCols = 4;

    static LPSTR rgsz[nRows][nCols] = {
        { szWDBG_DEFAULT_TL_NAME,  "Debugging on same machine",    "tlloc.dll",    "" },
        { "Pipes",  "Named pipe on same machine",   "tlpipe.dll",   ". windbg" }
    };

    // Dynamically create the default values

    // First create the specific values used by windbg
    for (int nR = 0; nR<nRows; nR++) {
        CIndiv_TL_WKSP * pCont = new CIndiv_TL_WKSP();
        Assert(pCont);
        pCont->Init(this, rgsz[nR][0], FALSE, FALSE);

        pCont->m_pszDescription = _strdup(rgsz[nR][1]);
        pCont->m_pszDll = _strdup(rgsz[nR][2]);
        pCont->m_pszParams = _strdup(rgsz[nR][3]);
    }

    // Common values used by windbg & windbgrm
    for (nR = 0; nR<ROWS_SERIAL_TRANSPORT_LAYERS; nR++) {
        CIndiv_TL_WKSP * pCont = new CIndiv_TL_WKSP();
        Assert(pCont);
        pCont->Init(this, rgszSerialTransportLayers[nR][0], FALSE, FALSE);

        pCont->m_pszDescription = _strdup(rgszSerialTransportLayers[nR][1]);
        pCont->m_pszDll = _strdup(rgszSerialTransportLayers[nR][2]);
        pCont->m_pszParams = _strdup(rgszSerialTransportLayers[nR][3]);
    }
}


//
// Transport Layer:
//
CIndiv_TL_WKSP::
CIndiv_TL_WKSP()
{
#define CINDIV_TL_DEFINE
#include "windbg.hxx"
#undef CINDIV_TL_DEFINE
}


//
// Paths:
//
CPaths_WKSP::
CPaths_WKSP()
{
#define CPATHS_DEFINE
#include "windbg.hxx"
#undef CPATHS_DEFINE
}


//
// Individual Exception:
//
CIndiv_Excep_WKSP::
CIndiv_Excep_WKSP()
{
#define CINDIV_EXCEP_DEFINE
#include "windbg.hxx"
#undef CINDIV_EXCEP_DEFINE
}


//
// All Exceptions:
//
CAll_Exceptions_WKSP::
CAll_Exceptions_WKSP()
{
}


//
// Kernel debugger preferences:
//
CKernel_Debugging_WKSP::
CKernel_Debugging_WKSP()
{
#define CKERNEL_DBG_DEFINE
#include "windbg.hxx"
#undef CKERNEL_DBG_DEFINE
}


//
// Individual Workspace:
//
CWorkSpace_WKSP::
CWorkSpace_WKSP()
{
#define CWORKSPACE_DEFINE
#include "windbg.hxx"
#undef CWORKSPACE_DEFINE
}

HKEY 
CWorkSpace_WKSP::
GetMirrorKey(
    PBOOL pbRegKeyCreated,
    PSTR pszSubstituteRegistryName
    )
{

    //
    // Override this function so that we will always create a mirrored
    // workspace in the root of windbg with a known name.
    //

    return CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP>::
        GetMirrorKey(pbRegKeyCreated, "WorkSpaceTemplate");
}


/*
//
// All Workspaces:
//
CAll_WorkSpaces_WKSP::
CAll_WorkSpaces_WKSP()
{
#define CALL_WORKSPACES_DEFINE
#include "windbg.hxx"
#undef CALL_WORKSPACES_DEFINE
}
*/


//
// Debuggee:
//
CDebuggee_WKSP::
CDebuggee_WKSP()
{
#define CDEBUGGEE_DEFINE
#include "windbg.hxx"
#undef CDEBUGGEE_DEFINE
}


//
// List of All the Debuggees:
//
CAll_Debuggees_WKSP::
CAll_Debuggees_WKSP()
{
#define CALL_DEBUGGEES_DEFINE
#include "windbg.hxx"
#undef CALL_DEBUGGEES_DEFINE
}


//
// User preferences:
//
CGlobalPreferences_WKSP::
CGlobalPreferences_WKSP()
{
#define CPREFERENCES_DEFINE
#include "windbg.hxx"
#undef CPREFERENCES_DEFINE
}


//
// User defined WM_ messages
//
CUser_WM_Messages_WKSP::
CUser_WM_Messages_WKSP()
{
}


//
// A child window
//
CIndivWinLayout_WKSP::
CIndivWinLayout_WKSP()
{
#define CINDIV_WIN_LAYOUT_DEFINE
#include "windbg.hxx"
#undef CINDIV_WIN_LAYOUT_DEFINE
}


//
// All child windows
//
CAllChildWindows_WKSP::
CAllChildWindows_WKSP()
{
}


//
// A window layout
//
CWinLayout_WKSP::
CWinLayout_WKSP()
{
#define CWIN_LAYOUT_DEFINE
#include "windbg.hxx"
#undef CWIN_LAYOUT_DEFINE
}


HKEY 
CWinLayout_WKSP::
GetMirrorKey(
    PBOOL pbRegKeyCreated,
    PSTR pszSubstituteRegistryName
    )
{

    //
    // Override this function so that we will always create a mirrored
    // workspace in the root of windbg with a known name.
    //

    return CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP>::
        GetMirrorKey(pbRegKeyCreated, "LastUsedWinLayout");
}


//
// All window layouts
//
CAllWindowLayouts_WKSP::
CAllWindowLayouts_WKSP()
{
#define CALL_WIN_LAYOUTS_DEFINE
#include "windbg.hxx"
#undef CALL_WIN_LAYOUTS_DEFINE
}
    

//
// Root for windbg
//
CBase_Windbg_WKSP::
CBase_Windbg_WKSP()
{
#define CWINDBG_DEFINE
#include "windbg.hxx"
#undef CWINDBG_DEFINE

    SetRegistryName("Software\\Microsoft\\windbg\\" WORKSPACE_REVISION_NUMBER );
}

HKEY
CBase_Windbg_WKSP::
GetRegistryKey(
    PBOOL pbRegKeyCreated
    )
{
    if (m_hkeyRegistry) {
        return m_hkeyRegistry;
    }

    return m_hkeyRegistry = WKSP_RegKeyOpenCreate(HKEY_CURRENT_USER, 
                                                  m_pszRegistryName,
                                                  pbRegKeyCreated
                                                  );
}


const char * const  CBase_Windbg_WKSP::m_pszNoProgramLoaded         = "<No Program Loaded>";
const char * const  CBase_Windbg_WKSP::m_pszAttachedProgramName     = "<Attached Program>";
const char * const  CBase_Windbg_WKSP::m_pszUntitledWorkSpaceName   = "New Workspace";
const char * const  CBase_Windbg_WKSP::m_pszDefaultWinLayout        = "Default Windows Layout";
