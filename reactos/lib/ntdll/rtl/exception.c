/* $Id: exception.c,v 1.1 2000/05/13 13:50:57 dwelch Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode exception support
 * FILE:              lib/ntdll/rtl/exception.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

VOID KiUserExceptionDispatcher(PEXCEPTION_RECORD ExceptionRecord,
			       PCONTEXT Context)
{
   
}
