/* $Id: exception.c,v 1.2 2000/12/23 02:37:38 dwelch Exp $
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

/* FUNCTIONS ***************************************************************/

VOID STDCALL
KiUserExceptionDispatcher(PEXCEPTION_RECORD ExceptionRecord,
			  PCONTEXT Context)
{
   
}
