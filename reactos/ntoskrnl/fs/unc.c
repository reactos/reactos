/* $Id: unc.c,v 1.5 2003/07/10 06:27:13 royce Exp $
 *
 * reactos/ntoskrnl/fs/unc.c
 *
 */
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>


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
