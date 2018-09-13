/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    nothunk.c

Abstract:

    Code to handle routines which are being thunked down to 16 bits or
    exported from the Windows 95 kernel.

Author:

    Bob Day   (bobday)

Revision History:

    Sunil Pai (sunilp) 28-Oct-1994. Removed SHRestartWindows,
                                    SHGetAboutInformation to start.c, deleted
                                    Shl1632_Thunk* and Shl3216_Thunk* routines,
                                    Removed pifmgr routines to pifmgr.c.

--*/


#include "shellprv.h"
#pragma  hdrstop

//---------------------------------------------------------------------------
//
// PURPOSE:  This is to support 16 bit control panel applets.
//
// EXPORTED: Internal export.
//
// DEFINED:  \win\core\library\cpl16.c.
//
// USED:     By display and driver applets.
//
// DETAILS:  The implementation of this thunks the cplinfo returned from the
//           16 bit applet to the 32 bit cplinfo.
//
//           NT doesn't support 16 bit applets.  We need to decide if we do
//           need to support this, accordingly this can be just a stub or
//           we need to import this.  Also no 32 corresponding 32 bit API
//           exposed.
//
// HISTORY:  BUGBUG - SUNILP
//
//----------------------------------------------------------------------------

LRESULT WINAPI CallCPLEntry16(
    HINSTANCE hinst,
    FARPROC16 lpfnEntry,
    HWND hwndCPL,
    UINT msg,
    LPARAM lParam1,
    LPARAM lParam2
) {
    return 0L;      // BUGBUG - 0L appears to mean un-handled, is this true?
}

//---------------------------------------------------------------------------
//
// PURPOSE:
//
//---------------------------------------------------------------------------

void RunDll_CallEntry16(
    RUNDLLPROC pfn,
    HWND hwndStub,
    HINSTANCE hinst,
    LPSTR pszParam,
    int nCmdShow)
{
    return;
}

VOID WINAPI UninitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
    DeleteCriticalSection(lpCriticalSection);
}

// BUGBUG Do we want to handle this on NT? (\\tal\msdos\win\core\shell\library\cbthook.c)

BOOL CheckResourcesBeforeExec(void)
{
    return TRUE;
}

// BUGBUG See SanfordS about the need to provide this

void SHGlobalDefect(DWORD lpVoid) {};

//---------------------------------------------------------------------------
//
// PURPOSE:  This is to add property pages from a 16 bit dll.
//
// EXPORTED: Not exported
//
// DEFINED:  \win\core\library\prt16.c
//
// USED:     internally by the shelldll.
//
// DETAILS:  The implements adding of prop pages from a 16 bit dll to a
//           property page array. The function passed in is a 16 bit fn.
//
//           We need to find out what to do about these.  We probably won't
//           support any 16 bit prop pages..
//
// HISTORY:  BUGBUG - SUNILP
//
//----------------------------------------------------------------------------

VOID WINAPI CallAddPropSheetPages16(
    LPFNADDPROPSHEETPAGES lpfn16,
    LPVOID hdrop,
    LPPAGEARRAY papg
) {
    // Can't return error condition...
}

//---------------------------------------------------------------------------
// add the pages for a given 16bit dll specified
//
// hDrop        list of files to add pages for
// pszDllEntry  DLLNAME,EntryPoint string
// lpfnAddPage  32bit add page callback
// lParam       data for 32bit page callback
//

UINT WINAPI SHAddPages16(HGLOBAL hGlobal, LPCTSTR pszDllEntry, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    return 0;
}


// BUGBUG - BobDay -
//
HMODULE WINAPI NoThkGetModuleHandle16(
    LPCTSTR lpModuleName
    )
{
    return 0;           // Return an error condition for now
}



//---------------------------------------------------------------------------
//
// PURPOSE:  To get the filename from and hinst for a 16 bit module.
//
// EXPORTED: Not Exported
//
// DEFINED:  \win\core\library\cpl16.c
//
// USED:     internally by the shelldll.
//
// DETAILS:  This cannot be supported by the 32 bit GetModuleFileName call.
//           This is being used by the shell to support:
//           1) 16 bit cpls (in control1.c)  [We may not support 16 bit cpls]
//           2) In ShellExecuteNormal (in shlexec.c) a new Win32 error may be
//              returned from CreateProcess (ERROR_SINGLE_INSTANCE_APP). For
//              this error ShellExecuteNormal examines all top level windows
//              , gets the instance handle for the window and uses this
//              function to get the exe name and sees if it matches.  Once
//              the top level window which refers to the same exe is found,
//              it switches this window to be the foreground window and
//              returns the handle of this.
//
//           Investigation Needed:  - a) When is an app a single instance app,
//           how does CreateProcess know.  b) Can CreateProcess on NT return
//           the same behaviour. c) Is this important for us to support, why
//           is this needed.
//
// HISTORY:  BUGBUG - SUNILP
//
//----------------------------------------------------------------------------

 // BUGBUG - BobDay - Don't know what this is for or why, needs to be removed
int WINAPI GetModuleFileName16(
    HINSTANCE hinst,
    LPTSTR szFileName,
    int cchMax
) {
    return 0;           // Return an error condition for now
}

// BUGBUG - BobDay - I think this comes from KERNEL32, its prototype is in
// krnlcmn.h
DWORD WINAPI GetProcessDword( DWORD idProcess, LONG iIndex )
{
    //
    // We only ever use - GPD_HINST, GPD_FLAGS (for WPF_WIN16_PROCESS),
    // GPD_EXP_WINVER
    //
    // In IsWin16Process (GPD_FLAGS) we could look at the WOWWORDs via
    // GetWindowLong, or we could look at the GWL_HINST and see if the
    // highword is zero.
    //
    // Window_GetInstance (GPD_HINST) couldn't we just call GetWindowLong
    // with GWL_HINST?
    //
    // ShellExecuteNormal (GPD_HINST) I don't think we can fix this easily.
    // can we get the pei to just have a process id instead?
    //
    // Window_IsLFNAware (GPD_FLAGS & GPD_EXPWINVER):  Should use
    // IsWin16Process technique for GPD_FLAGS; should use GetProcessVersion
    // instead of GPD_EXP_WINVER.
    //
    ASSERTMSG(iIndex != GPD_EXP_WINVER,
              "Use GetProcessVersion instead of GPD_EXP_WINVER");

    return 0;
}

// BUGBUG - BobDay - I think this comes from KERNEL32, its prototype is in
// krnlcmn.h
BOOL WINAPI SetProcessDword( DWORD idProcess, LONG iIndex, DWORD dwValue )
{
    return 0;
}

// BUGBUG - BobDay - This function needs to be added to KERNEL32. NOPE,
// according to markl we only need this because the critical section
// was located in shared memory.  Possible solution here might be to create
// a named event or mutex and synchronize via it. Another possible solution
// might be to move each of the objects for which there is a critical section
// out of the shared memory segment and maintain a per-process data structure.

// normally: ReinitializeCriticalSection
VOID WINAPI NoThkReinitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
) {
    InitializeCriticalSection( lpCriticalSection );
}

// BUGBUG - BobDay - Some hidden function in KERNEL (16 or 32, I can't determine)
int WINAPI PK16FNF(
    char *szBuffer
) {
    return 0;
}


// BUGBUG - BobDay - One of the 32-bit control panel applets needs to get
// the list of drivers.  This was a thunk down into the 16-bit world to call
// the GetNextDriver function.  We need to do the right 32-bit thing.
HANDLE WINAPI ShellGetNextDriverName(
    HANDLE hdrv,
    LPSTR pszName,
    int cbName
) {
    return 0;
}



//---------------------------------------------------------------------------
//
// PURPOSE:  This is to load 16 bit dlls
//
// EXPORTED: Not implemented in shelldll
//
// DEFINED:  \win\core\win32\kernel\krnlutil.asm
//
// USED:     Shell, applets, printer..
//
// DETAILS:  This is used to support:
//
//           1) 16 bit cpls. (control1.c, cpls)
//           2) 16 bit dlls with property sheet extensions. (binder.c)
//           3) 16 bit printer drivers.
//
//           We most probably won't support 16 bit cpls and 16 bit printer
//           drivers.  Need to investigate 16 bit property sheet extensions.
//
// HISTORY:  BUGBUG - SUNILP
//
//----------------------------------------------------------------------------
// BUGBUG - BobDay - This function is probably used for something we don't want.

// normally LoadLibrary16
HINSTANCE WINAPI NoThkLoadLibrary16(
    LPCTSTR lpLibFileName
) {
    return 0;
}

//---------------------------------------------------------------------------
//
// PURPOSE:  This is to free the loaded 16 bit dlls
//
// EXPORTED: Not implemented in shelldll
//
// DEFINED:  \win\core\win32\kernel\krnlutil.asm
//
// USED:     Shell, applets, printer..
//
// DETAILS:  This is used to support:
//
//           1) 16 bit cpls. (control1.c, cpls)
//           2) 16 bit dlls with property sheet extensions. (binder.c)
//           3) 16 bit printer drivers.
//
//           We most probably won't support 16 bit cpls and 16 bit printer
//           drivers.  Need to investigate 16 bit property sheet extensions.
//
// HISTORY:  BUGBUG - SUNILP
//
//----------------------------------------------------------------------------
// BUGBUG - BobDay - This function is probably used for something we don't want.

//normally: FreeLibrary16
BOOL WINAPI NoThkFreeLibrary16(
    HINSTANCE hLibModule
) {
    return FALSE;
}

//---------------------------------------------------------------------------
//
// PURPOSE:  To get addresses of procedures in 16 bit dlls
//
// EXPORTED: Not implemented in shelldll
//
// DEFINED:  \win\core\win32\kernel\krnlutil.asm
//
// USED:     Shell, applets, printer..
//
// DETAILS:  This is used to support:
//
//           1) 16 bit cpls. (control1.c, cpls)
//           2) 16 bit dlls with property sheet extensions. (binder.c)
//           3) 16 bit printer drivers.
//
//           We most probably won't support 16 bit cpls and 16 bit printer
//           drivers.  Need to investigate 16 bit property sheet extensions.
//
//
// HISTORY:  BUGBUG - SUNILP
//
//----------------------------------------------------------------------------
// BUGBUG - BobDay - This function is probably used for something we don't want.
// normally: GetProcAddress16
FARPROC WINAPI NoThkGetProcAddress16(
    HINSTANCE hModule,
    LPCSTR lpProcName
) {
    return NULL;
}
