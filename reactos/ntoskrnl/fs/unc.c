/* $Id: unc.c,v 1.3 2002/09/07 15:12:50 chorns Exp $
 *
 * reactos/ntoskrnl/fs/unc.c
 *
 */
#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


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
