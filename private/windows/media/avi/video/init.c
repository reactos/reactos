/*
    init.c initialisation for MSVIDEO.DLL

    Copyright (c) Microsoft Corporation 1992. All rights reserved

*/

#include <windows.h>
#include <win32.h>
#include <verinfo.h>           // to get rup and MMVERSION
#include "mmsystem.h"
#include "msviddrv.h"
#include <vfw.h>
#include "msvideoi.h"
#ifdef _WIN32
#include "profile.h"
#endif

#include "debug.h"

/*
 * we have to allow the compman dll to perform load and unload
 * processing - among other things, it has a critsec that needs to
 * be initialised and freed
 */
#ifdef _WIN32
extern BOOL     WINAPI ICDllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
#else
extern BOOL FAR PASCAL ICDllEntryPoint(DWORD dwReason, HINSTANCE hinstDLL, WORD	wDS, WORD wHeapSize, DWORD dwReserved1, WORD wReserved2);
#endif

//
//
//
#ifndef _WIN32
extern void FAR PASCAL videoCleanup(HTASK hTask);
#else
    #define videoCleanup(hTask) // Nothing to do for 32 bit code
#endif
extern void FAR PASCAL DrawDibCleanup(HTASK hTask);
extern void FAR PASCAL ICCleanup(HTASK hTask);

//--------------------------------------------------------------------------;
//
//
//  -- ==  DLL Initialization entry points  == --
//
//
//--------------------------------------------------------------------------;

/*****************************************************************************
 * Variables
 *
 ****************************************************************************/

HINSTANCE ghInst;                         // our module handle
BOOL gfIsRTL;

// dont change this without changing DRAWDIB\PROFDISP.C & MSVIDEO.RC
#define IDS_ISRTL 4003

#ifdef _WIN32
/*****************************************************************************
 * @doc INTERNAL VIDEO
 *
 * DLLEntryPoint - Standard 32-bit DLL entry point.
 *
 ****************************************************************************/

BOOL WINAPI DLLEntryPoint (
   HINSTANCE hInstance,
   ULONG Reason,
   LPVOID pv)
{
    BOOL fReturn = TRUE;

    switch (Reason)
    {
        TCHAR    ach[2];

	case DLL_PROCESS_ATTACH:
	    DbgInitialize(TRUE);

	    ghInst = hInstance;
	    LoadString(ghInst, IDS_ISRTL, ach, sizeof(ach)/sizeof(TCHAR));
	    gfIsRTL = ach[0] == TEXT('1');

	    DisableThreadLibraryCalls(hInstance);

	    fReturn = ICDllEntryPoint(hInstance, Reason, pv);

            break;

        case DLL_PROCESS_DETACH:
	    DrawDibCleanup(NULL);

	    ICCleanup(NULL);

	    ICDllEntryPoint(hInstance, Reason, pv);

	    videoCleanup(NULL);

	    CloseKeys();

	    break;

        //case DLL_THREAD_DETACH:
        //    break;

        //case DLL_THREAD_ATTACH:
        //    break;
    }

    return TRUE;
}

#else
//--------------------------------------------------------------------------;
//
//  BOOL DllEntryPoint
//
//  Description:
//	This is a special 16-bit entry point called by the Chicago kernel
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
//	DAYTONA 16-bit builds (ie, WOW):
//	    We call this function from LibEntry.asm.  Daytona WOW does not
//	    call this function directly.  Since we only call it from
//	    LibEntry and WEP, cUsage just bounces between 0 and 1.
//
//	CHICAGO 16-bit builds:
//	    The Chicago kernel calls this directly for every usage increment
//	    and decrement.  cUsage will track the usages and init or terminate
//	    appropriately.
//
//  History:
//      07/07/94    [frankye]
//
//--------------------------------------------------------------------------;

BOOL FAR PASCAL _export DllEntryPoint
(
 DWORD	    dwReason,
 HINSTANCE  hInstance,
 WORD	    wDS,
 WORD	    wHeapSize,
 DWORD	    dwReserved1,
 WORD	    wReserved2
)
{
    static UINT cUsage = 0;

    switch (dwReason)
    {
	case 1:
	{
	    //
	    //	Usage increment
	    //
	    cUsage++;

	    ASSERT( 0 != cUsage );
	
	    if (1 == cUsage)
	    {
		TCHAR ach[2];
		DbgInitialize(TRUE);
		ghInst = hInstance;
		LoadString(ghInst, IDS_ISRTL, ach, sizeof(ach)/sizeof(TCHAR));
		gfIsRTL = ach[0] == TEXT('1');
	    }
	
	    //
	    //	Call ICProcessAttach on _every_ usage increment.  On Chicago,
	    //	the ICM stuff needs to be aware of all processes that load
	    //	and free this dll.  The only way to do this is to allow it to
	    //	look at stuff on every usage delta.
	    //
	    ICProcessAttach();

	    return TRUE;
	}
	
	case 0:
	{
	    //
	    //	Usage decrement
	    //
	    ASSERT( 0 != cUsage );
	
	    cUsage--;

	    if (0 == cUsage)
	    {
		DrawDibCleanup(NULL);
		ICCleanup(NULL);
	    }
	
	    //
	    //	Call ICProcessDetach on _every_ usage increment.  On Chicago,
	    //	the ICM stuff needs to be aware of all processes that load
	    //	and free this dll.  The only way to do this is to allow it to
	    //	look at stuff on every usage delta.
	    //
            ICProcessDetach();
	
	    if (0 == cUsage)
	    {
		videoCleanup(NULL);
	    }

	    return TRUE;
	}

    }
    return TRUE;
}

#endif

//--------------------------------------------------------------------------;
//
//
//
//
//--------------------------------------------------------------------------;

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | VideoForWindowsVersion | This function returns the version
 *   of the Microsoft Video for Windows software.
 *
 * @rdesc Returns a DWORD version, the hiword is the product version the
 *  loword is the minor revision.
 *
 * @comm currently returns 0x010A00## (1.10.00.##) ## is the internal build
 *      number.
 *
 ****************************************************************************/
#if 0
#ifdef rup
    #define MSVIDEO_VERSION     (0x01000000l+rup)       // 1.00.00.##
#else
    #define MSVIDEO_VERSION     (0x01000000l)           // 1.00.00.00
#endif
#else
    #define MSVIDEO_VERSION     (0x0L+(((DWORD)MMVERSION)<<24)+(((DWORD)MMREVISION)<<16)+((DWORD)MMRELEASE))
#endif

DWORD FAR PASCAL VideoForWindowsVersion(void)
{
    return MSVIDEO_VERSION;
}
