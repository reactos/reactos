/* $Id: unc.c,v 1.2 2002/08/09 22:57:48 ekohl Exp $
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
 */
NTSTATUS STDCALL
FsRtlRegisterUncProvider(IN OUT PHANDLE Handle,
			 IN PUNICODE_STRING RedirectorDeviceName,
			 IN BOOLEAN MailslotsSupported)
{
  return(STATUS_NOT_IMPLEMENTED);
}


/* EOF */
