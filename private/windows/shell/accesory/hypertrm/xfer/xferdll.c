/*	File: D:\WACKER\xfer\xferdll.c (Created: 08-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>

BOOL WINAPI XferEntry(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);
BOOL WINAPI _CRT_INIT(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	XferEntry
 *
 * DESCRIPTION:
 *	Currently, just initializes the C-Runtime library but may be used
 *	for other things later.
 *
 * ARGUMENTS:
 *	hInstDll	- Instance of this DLL
 *	fdwReason	- Why this entry point is called
 *	lpReserved	- reserved
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL WINAPI XferEntry(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
	{
	return _CRT_INIT(hInstDll, fdwReason, lpReserved);
	}
