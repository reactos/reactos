#ifndef __WINDBG_H__
#define __WINDBG_H__


#include "hmacro.h"


//
// WINDBG registry classes
//

//
//  + Reg key
//  - Reg value
//  > Reg key that is mirrored
//
//
//  HKEY_CURRENT_USER\\Software\\Microsoft\\windbg\\ WORKSPACE_REVISION_NUMBER
//    > Kernel Debugger Preferences
//    + Preferences
//    + User defined window messages
//    + CAll_TLs_WKSP (Transport Layer)
//      + CIndiv_TL_WKSP (individual transport, repeated as necessary)
//        - Description
//        - DLL
//        - Params
//      + CIndiv_TL_WKSP (repeat...)
//        - ...
//    > CPaths_WKSP (Paths)
//      - Source Path
//      - SymPath
//      - Root mapping pair (stored as a multi-string)
//    > Window layout (some of windows are saved)
//    > Exceptions
//    + All Debuggees (list everything debuggable)
//      + Debuggee (individual debuggee) (repeat...)
//        - Preferred workspace name
//        + Individual Workspace (repeat...)
//          > Kernel Debugger Preferences
//          - Name of selected remote pipe
//          - Name of selected Transport Layer
//          - Program args
//          - Title bar
//          + Breakpoints
//          > Exceptions
//          > CPaths_WKSP (Paths)
//          > Window layout (all windows are saved)
//
//





//
// Transport Layer:
//
class CIndiv_TL_WKSP 
: public CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP> 
{
public:

#define CINDIV_TL_DEFINE
#include "windbg.hxx"
#undef CINDIV_TL_DEFINE

public:
    CIndiv_TL_WKSP();
};


//
// Transport Layer:
//
class CAll_TLs_WKSP 
: public CGen_WKSP<CIndiv_TL_WKSP, CItemInterface_WKSP>
{
public:
    CAll_TLs_WKSP();
};


//
// Paths:
//
class CPaths_WKSP
: public CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP>
{
public:

#define CPATHS_DEFINE
#include "windbg.hxx"
#undef CPATHS_DEFINE

    CPaths_WKSP();
};


//
// Individual Exception:
//
class CIndiv_Excep_WKSP
: public CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP>
{
public:

#define CINDIV_EXCEP_DEFINE
#include "windbg.hxx"
#undef CINDIV_EXCEP_DEFINE

    CIndiv_Excep_WKSP();
};


//
// All Exceptions:
//
class CAll_Exceptions_WKSP
: public CGen_WKSP<CIndiv_Excep_WKSP, CItemInterface_WKSP>
{
public:
    CAll_Exceptions_WKSP();
};


//
// Kernel debugger preferences:
//
class CKernel_Debugging_WKSP
: public CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP>
{
public:

#define CKERNEL_DBG_DEFINE
#include "windbg.hxx"
#undef CKERNEL_DBG_DEFINE

    CKernel_Debugging_WKSP();
};


//
// Individual Workspace:
//
class CWorkSpace_WKSP
: public CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP>
{
public:

#define CWORKSPACE_DEFINE
#include "windbg.hxx"
#undef CWORKSPACE_DEFINE

    CWorkSpace_WKSP();

    virtual HKEY GetMirrorKey(PBOOL pbRegKeyCreated = NULL, 
                              PSTR pszSubstituteRegistryName = NULL
                              );
};


/*
//
// All Workspaces:
//
class CAll_WorkSpaces_WKSP
: public CGen_WKSP<CWorkSpace_WKSP, CItemInterface_WKSP>
{
public:

#define CALL_WORKSPACES_DEFINE
#include "windbg.hxx"
#undef CALL_WORKSPACES_DEFINE

    CAll_WorkSpaces_WKSP();
};
*/

//
// Debuggee:
//
class CDebuggee_WKSP
//: public CGen_WKSP<CAll_WorkSpaces_WKSP, CItemInterface_WKSP>
: public CGen_WKSP<CWorkSpace_WKSP, CItemInterface_WKSP>
{
public:

#define CDEBUGGEE_DEFINE
#include "windbg.hxx"
#undef CDEBUGGEE_DEFINE

    CDebuggee_WKSP();
};


//
// List of All the Debuggees:
//
class CAll_Debuggees_WKSP
: public CGen_WKSP<CDebuggee_WKSP, CItemInterface_WKSP>
{
public:

#define CALL_DEBUGGEES_DEFINE
#include "windbg.hxx"
#undef CALL_DEBUGGEES_DEFINE

    CAll_Debuggees_WKSP();
};


//
// User defined WM_ messages
//
class CUser_WM_Messages_WKSP
: public CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP>
{
public:
    // TODO - kcarlos
    CUser_WM_Messages_WKSP();
};


//
// User preferences:
//
class CGlobalPreferences_WKSP
: public CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP>
{
public:

#define CPREFERENCES_DEFINE
#include "windbg.hxx"
#undef CPREFERENCES_DEFINE

    CGlobalPreferences_WKSP();
};


//
// A window layout
//
class CIndivWinLayout_WKSP
: public CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP>
{
public:

#define CINDIV_WIN_LAYOUT_DEFINE
#include "windbg.hxx"
#undef CINDIV_WIN_LAYOUT_DEFINE

    CIndivWinLayout_WKSP();
};


//
// All child windows
//
class CAllChildWindows_WKSP
: public CGen_WKSP<CIndivWinLayout_WKSP, CItemInterface_WKSP>
{
public:

    CAllChildWindows_WKSP();

};


//
// A window layout
//
class CWinLayout_WKSP
: public CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP>
{
public:

#define CWIN_LAYOUT_DEFINE
#include "windbg.hxx"
#undef CWIN_LAYOUT_DEFINE

    CWinLayout_WKSP();

    virtual HKEY GetMirrorKey(PBOOL pbRegKeyCreated = NULL, 
                              PSTR pszSubstituteRegistryName = NULL
                              );
};


//
// All window layouts
//
class CAllWindowLayouts_WKSP
: public CGen_WKSP<CWinLayout_WKSP, CItemInterface_WKSP>
{
public:

#define CALL_WIN_LAYOUTS_DEFINE
#include "windbg.hxx"
#undef CALL_WIN_LAYOUTS_DEFINE

    CAllWindowLayouts_WKSP();
};


//
// Root for windbg
//
class CBase_Windbg_WKSP
: public CGen_WKSP<CDoNothingStub_WKSP, CItemInterface_WKSP>
{
public:
    static const char * const m_pszNoProgramLoaded;
    static const char * const m_pszAttachedProgramName;

    static const char * const m_pszUntitledWorkSpaceName;

    static const char * const m_pszDefaultWinLayout;


#define CWINDBG_DEFINE
#include "windbg.hxx"
#undef CWINDBG_DEFINE

    CBase_Windbg_WKSP();

    virtual HKEY GetRegistryKey(PBOOL pbRegKeyCreated = NULL);
};



#endif


