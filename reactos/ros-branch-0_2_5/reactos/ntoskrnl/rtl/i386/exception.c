/* $Id: exception.c,v 1.11 2004/07/01 02:40:23 hyperion Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Kernel-mode exception support for IA-32
 * FILE:              ntoskrnl/rtl/i386/exception.c
 * PROGRAMER:         Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include <ntos.h>
#include <internal/ke.h>
#include <internal/ps.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

#if 1
VOID STDCALL
MsvcrtDebug(ULONG Value)
{
  DbgPrint("KernelDebug 0x%.08x\n", Value);
}
#endif

#if !defined(_MSC_VER)
/*
 * When compiling this file with MSVC itself, don't compile these functions.
 * They are replacements for MS compiler and/or C runtime library functions,
 * which are already provided by the MSVC compiler and C runtime library.
 */

/*
 * @implemented
 */
int
_abnormal_termination(void)
{
   DbgPrint("Abnormal Termination\n");
   return 0;
}

struct _CONTEXT;

/*
 * @implemented
 */
EXCEPTION_DISPOSITION
_except_handler2(
   struct _EXCEPTION_RECORD *ExceptionRecord,
   void *RegistrationFrame,
   struct _CONTEXT *ContextRecord,
   void *DispatcherContext)
{
	DbgPrint("_except_handler2()\n");
	return (EXCEPTION_DISPOSITION)0;
}

/*
 * @implemented
 */
void __cdecl
_global_unwind2(PEXCEPTION_REGISTRATION RegistrationFrame)
{
   RtlUnwind(RegistrationFrame, &&__ret_label, NULL, 0);
__ret_label:
   // return is important
   return;
}

#endif	/* _MSC_VER */

/* EOF */
