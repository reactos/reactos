/* $Id: fiber.c,v 1.1 2000/08/14 14:34:12 ea Exp $
 *
 * FILE: lib/kernel32/thread/fiber.c
 *
 * ReactOS Kernel32.dll
 *
 */
#include <windows.h>


/**********************************************************************
 *	ConvertThreadToFiber
 */
LPVOID
STDCALL
ConvertThreadToFiber(
	LPVOID lpArgument
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


/**********************************************************************
 *	CreateFiber
 */
LPVOID
STDCALL
CreateFiber(
	DWORD			dwStackSize,
	LPFIBER_START_ROUTINE	lpStartAddress,
	LPVOID			lpArgument
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


/**********************************************************************
 *	DeleteFiber
 */
VOID
STDCALL
DeleteFiber(
	LPVOID	lpFiber
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return;
}


/**********************************************************************
 *	GetCurrentFiber
 */
PVOID
STDCALL
GetCurrentFiber(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


/**********************************************************************
 *	GetFiberData
 */
PVOID
STDCALL
GetFiberData(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


/**********************************************************************
 *	SwitchToFiber
 */
VOID
STDCALL
SwitchToFiber(
	LPVOID	lpFiber
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return;
}


/* EOF */
