/* $Id: fiber.c,v 1.2 2002/09/07 15:12:28 chorns Exp $
 *
 * FILE: lib/kernel32/thread/fiber.c
 *
 * ReactOS Kernel32.dll
 *
 */
#include <kernel32.h>


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
InternalGetCurrentFiber(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return NULL;
}


/**********************************************************************
 *	GetFiberData
 */
PVOID
STDCALL
InternalGetFiberData(VOID)
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
