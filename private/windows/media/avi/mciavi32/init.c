//==========================================================================;
//
//  init.c
//
//  Copyright (c) 1991-1993 Microsoft Corporation.  All Rights Reserved.
//
//  Description:
//
//
//  History:
//      11/15/92    cjp     [curtisp]
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <memory.h>
#include <process.h>

#ifdef WIN4
//
//  WIN4 thunk connect function protos
//
#ifdef _WIN32
BOOL PASCAL mciup_ThunkConnect32(LPCSTR pszDll16, LPCSTR pszDll32, HINSTANCE hinst, DWORD dwReason);
#else
BOOL FAR PASCAL mciup_ThunkConnect16(LPCSTR pszDll16, LPCSTR pszDll32, HINSTANCE hinst, DWORD dwReason);
#endif
#endif

//
//
//
//
#ifdef WIN4
char    gmbszMCIAVI[]	      = "mciavi.drv";
char    gmbszMCIAVI32[]	      = "mciavi32.dll";
#endif


//==========================================================================;
//
//  WIN 16 SPECIFIC SUPPORT
//
//==========================================================================;

#ifndef _WIN32

#ifdef WIN4
//--------------------------------------------------------------------------;
//
//
//
//
//
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
//  BOOL DllEntryPoint
//
//  Description:
//	This is a special 16-bit entry point called by the WIN4 kernel
//	for thunk initialization and cleanup.  It is called on each usage
//	increment or decrement.  Do not call GetModuleUsage within this
//	function as it is undefined whether the usage is updated before
//	or after this DllEntryPoint is called.
//
//  Arguments:
//	DWORD dwReason:
//		1 - attach (usage increment)
//		0 - detach (usage decrement)
//
//	HINSTANCE hinst:
//
//	WORD wDS:
//
//	WORD wHeapSize:
//
//	DWORD dwReserved1:
//
//	WORD wReserved2:
//
//  Return (BOOL):
//
//  Notes:
//	cEntered tracks reentry into DllEntryPoint.  This may happen due to
//	the thunk connections
//
//  History:
//      02/02/94    [frankye]
//
//--------------------------------------------------------------------------;
#pragma message ("--- Remove secret MSACM.INI AllowThunks ini switch")

BOOL FAR PASCAL _export DllEntryPoint
(
 DWORD	    dwReason,
 HINSTANCE  hinst,
 WORD	    wDS,
 WORD	    wHeapSize,
 DWORD	    dwReserved1,
 WORD	    wReserved2
)
{
    static UINT cEntered    = 0;
    BOOL f	    = TRUE;


    //
    //	Track reentry
    //
    //
    cEntered++;


    switch (dwReason)
    {
	case 0:
	case 1:
	    f = (0 != mciup_ThunkConnect16(gmbszMCIAVI, gmbszMCIAVI32,
					   hinst, dwReason));
	    break;

	default:
	    f = TRUE;
	    break;
    }

    //
    //	Track reentry
    //
    //
    cEntered--;

    return (f);
}
#endif

#else // _WIN32

//==========================================================================;
//
//  WIN 32 SPECIFIC SUPPORT
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL DllEntryPoint
//
//  Description:
//      This is the standard DLL entry point for Win 32.
//
//  Arguments:
//      HINSTANCE hinst: Our instance handle.
//
//      DWORD dwReason: The reason we've been called--process/thread attach
//      and detach.
//
//      LPVOID lpReserved: Reserved. Should be NULL--so ignore it.
//
//  Return (BOOL):
//      Returns non-zero if the initialization was successful and 0 otherwise.
//
//  History:
//      11/15/92    cjp     [curtisp]
//	    initial
//	04/18/94    fdy	    [frankye]
//	    major mods for WIN4.  Yes, it looks real ugly now cuz of all
//	    the conditional compilation for WIN4, daytona, etc.  Don't
//	    have time to think of good way to structure all this right now.
//
//--------------------------------------------------------------------------;

BOOL CALLBACK DllEntryPoint
(
    HINSTANCE               hinst,
    DWORD                   dwReason,
    LPVOID                  lpReserved
)
{
    BOOL		f = TRUE;

    //
    //
    //
    if (DLL_PROCESS_ATTACH == dwReason)
    {
	DisableThreadLibraryCalls(hinst);

	//
	//  thunk connect
	//
	mciup_ThunkConnect32(gmbszMCIAVI, gmbszMCIAVI32, hinst, dwReason);
    }


    //
    //
    //
    if (DLL_PROCESS_DETACH == dwReason)
    {
	//
	//  thunk disconnect
	//
	mciup_ThunkConnect32(gmbszMCIAVI, gmbszMCIAVI32, hinst, dwReason);

    }
	

    return (f);
} // DllEntryPoint()

#endif
