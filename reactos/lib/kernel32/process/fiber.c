/* $Id: fiber.c,v 1.1 1999/05/21 05:42:08 ea Exp $
 *
 * fiber.c
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
	LPVOID lpParameter
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
	LPVOID			lpParameter
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
