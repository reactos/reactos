/* $Id: callback.c,v 1.5 2000/12/23 02:37:39 dwelch Exp $
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

/* EOF */
