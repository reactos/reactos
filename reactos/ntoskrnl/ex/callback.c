/* $Id: callback.c,v 1.4 2000/07/04 01:27:58 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/callback.c
 * PURPOSE:         Executive callbacks
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/*
 * NOTE:
 *	These funtions are not implemented in NT4.
 *	They are implemented in Win2k.
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
ExCreateCallback (
	OUT	PCALLBACK_OBJECT	* CallbackObject,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	BOOLEAN			Create,
	IN	BOOLEAN			AllowMultipleCallbacks
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

VOID
STDCALL
ExNotifyCallback (
	IN	PVOID	CallbackObject,
	IN	PVOID	Argument1,
	IN	PVOID	Argument2
	)
{
	return;
}

PVOID
STDCALL
ExRegisterCallback (
	IN	PCALLBACK_OBJECT	CallbackObject,
	IN	PCALLBACK_FUNCTION	CallbackFunction,
	IN	PVOID			CallbackContext
	)
{
	return NULL;
}

VOID
STDCALL
ExUnregisterCallback (
	IN	PVOID	CallbackRegistration
	)
{
	return;
}

/*
 * FIXME:
 *	The following functions don't belong here.
 *	Move them somewhere else.
 */

#if 0
VOID ExCallUserCallBack(PVOID fn)
/*
 * FUNCTION: Transfer control to a user callback
 */
{
   UNIMPLEMENTED;
}
#endif

NTSTATUS
STDCALL
NtCallbackReturn (
	PVOID		Result,
	ULONG		ResultLength,
	NTSTATUS	Status
	)
{
   UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtW32Call (
	VOID
	)
{
   UNIMPLEMENTED;
}

/* EOF */
