/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/napi.c
 * PURPOSE:         Native API support routines
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wstring.h>
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

#include <ntdll/service.h>
#include <ntdll/napi.h>

/* FUNCTIONS *****************************************************************/
			 
