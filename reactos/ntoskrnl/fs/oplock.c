/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/fs/oplock.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     Emanuele Aliberti
 *                  Eric Kohl
 */

#include <ntoskrnl.h>


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
 * @unimplemented
 */
NTSTATUS
NTAPI
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
 * @unimplemented
 */
BOOLEAN
NTAPI
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
 *
 * @implemented
 */
VOID
NTAPI
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
 * @unimplemented
 */
NTSTATUS
NTAPI
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
 * @unimplemented
 */
BOOLEAN
NTAPI
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
 * @implemented
 */
VOID
NTAPI
FsRtlUninitializeOplock(IN POPLOCK Oplock)
{
}

/* EOF */
