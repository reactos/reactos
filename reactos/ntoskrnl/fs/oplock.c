/* $Id: oplock.c,v 1.3 2002/08/09 22:57:48 ekohl Exp $
 *
 * reactos/ntoskrnl/fs/oplock.c
 *
 */
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlCheckOplock@20
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
NTSTATUS STDCALL
FsRtlCheckOplock(IN POPLOCK Oplock,
		 IN PIRP Irp,
		 IN PVOID Context,
		 IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
		 IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL)
{
  return(STATUS_NOT_IMPLEMENTED);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlCurrentBatchOplock@4
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
BOOLEAN STDCALL
FsRtlCurrentBatchOplock(IN POPLOCK Oplock)
{
  return(FALSE);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlInitializeOplock@4
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 *	Obsolete function.
 */
VOID STDCALL
FsRtlInitializeOplock(IN OUT POPLOCK Oplock)
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlOplockFsctrl@12
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
NTSTATUS STDCALL
FsRtlOplockFsctrl(IN POPLOCK Oplock,
		  IN PIRP Irp,
		  IN ULONG OpenCount)
{
  return(STATUS_NOT_IMPLEMENTED);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlOplockIsFastIoPossible@4
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
BOOLEAN STDCALL
FsRtlOplockIsFastIoPossible(IN POPLOCK Oplock)
{
  return(FALSE);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlUninitializeOplock@4
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
VOID STDCALL
FsRtlUninitializeOplock(IN POPLOCK Oplock)
{
}

/* EOF */
