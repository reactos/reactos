/* $Id$
 *
 * reactos/ntoskrnl/fs/unc.c
 *
 */

#include <ntoskrnl.h>


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlDeregisterUncProvider@4
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
VOID STDCALL
FsRtlDeregisterUncProvider(IN HANDLE Handle)
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlRegisterUncProvider@12
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
NTSTATUS STDCALL
FsRtlRegisterUncProvider(IN OUT PHANDLE Handle,
			 IN PUNICODE_STRING RedirectorDeviceName,
			 IN BOOLEAN MailslotsSupported)
{
  return(STATUS_NOT_IMPLEMENTED);
}


/* EOF */
