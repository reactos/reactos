/*	File: D:\WACKER\term.c (Created: 23-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1.21 $
 *	$Date: 1995/03/08 14:33:27 $
 */

#include <windows.h>
#pragma hdrstop

#include <tdll\tdll.h>

#if !defined(NDEBUG)

//
//  JohnFu  -   Commented out nih\smrtheap.h
//
//#include <nih\smrtheap.h>

// 'MemDefaultPoolFlags = MEM_POOL_SERIALIZE' is required by Smartheap
// if app is multithreaded.
//
unsigned MemDefaultPoolFlags = MEM_POOL_SERIALIZE;
#endif

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

	if (hPrevInst)
		return FALSE;

	/* --- Initialize Smartheap memory manager for debug version only. --- */

	#if !defined(NDEBUG)
        //MemRegisterTask();    //  Commented out, JohnFu
	#endif

	/* --- Initialize this instance of the program --- */

	if (!InitInstance(hInst, (LPTSTR)lpCmdLine, nCmdShow))
		return FALSE;

	/* --- Process messages until the end --- */

	i = MessageLoop();

	/* --- Report any memory leaks in debug version only. --- */

	#if !defined(NDEBUG)
	dbgMemReportLeakage(MemDefaultPool, 1, 1); //lint !e522
	#endif

	return i;
	}
