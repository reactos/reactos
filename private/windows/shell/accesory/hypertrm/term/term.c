/*	File: D:\WACKER\term.c (Created: 23-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 5/25/99 8:56a $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\tdll.h>

#if !defined(NDEBUG)
//#include <nih\smrtheap.h>

// 'MemDefaultPoolFlags = MEM_POOL_SERIALIZE' is required by Smartheap
// if app is multithreaded.
//
#if !defined(NO_SMARTHEAP)
unsigned MemDefaultPoolFlags = MEM_POOL_SERIALIZE;
#endif

#endif

#if defined(MSVS6_DEBUG)
#if defined(_DEBUG)
//
// If compiling a debug build with VC6, then turn on the
// new heap debugging tools.  To enable this, add the
// define of MSVS6_DEBUG in your personal.cfg.
//
#include <crtdbg.h>
#endif // _DEBUG
#endif // MSVS6_DEBUG


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	WinMain
 *
 * DESCRIPTION:
 *	Entry point for wacker
 *
 */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
	{
	int i;

#if defined(MSVS6_DEBUG)
#if defined(_DEBUG)

	//
	// If compiling a debug build with VC6, then turn on the
	// new heap debugging tools.  To enable this, add the
	// define of MSVS6_DEBUG in your personal.cfg.
	//
	// Get the current state of the flag
	// and store it in a temporary variable
	//
	int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

	//
	// Turn On (OR) - Keep freed memory blocks in the
	// heap’s linked list and mark them as freed
	//
	tmpFlag |= _CRTDBG_DELAY_FREE_MEM_DF;

	//
	// Turn On (OR) - Enable debug heap allocations
	// and use of memory block type identifiers,
	// such as _CLIENT_BLOCK.
	//
	tmpFlag |= _CRTDBG_ALLOC_MEM_DF;

	//
	// Turn On (OR) - Enable debug heap memory leak check
	// at program exit.
	//
	tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

	//
	// Turn Off (AND) - prevent _CrtCheckMemory from
	// being called at every allocation request
	//
	tmpFlag &= ~_CRTDBG_CHECK_ALWAYS_DF;

	//
	// Set the new state for the flag
	//
	_CrtSetDbgFlag( tmpFlag );

#endif // _DEBUG
#endif // MSVS6_DEBUG

	if (hPrevInst)
		return FALSE;

	/* --- Initialize Smartheap memory manager for debug version only. --- */

	#if !defined(NDEBUG)
    #if !defined(NO_SMARTHEAP)
	MemRegisterTask();
	#endif
    #endif

	/* --- Initialize this instance of the program --- */

	if (!InitInstance(hInst, (LPTSTR)lpCmdLine, nCmdShow))
		return FALSE;

	/* --- Process messages until the end --- */

	i = MessageLoop();

	/* --- Report any memory leaks in debug version only. --- */

	#if !defined(NDEBUG)
    #if !defined(NO_SMARTHEAP)
	dbgMemReportLeakage(MemDefaultPool, 1, 1); //lint !e522
    #endif
	#endif

	return i;
	}
