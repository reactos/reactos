/* $Id: util.c,v 1.9 2002/01/17 23:17:39 ea Exp $
 *
 * reactos/ntoskrnl/fs/util.c
 *
 */
#include <ntos.h>
#include <ddk/ntifs.h>


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlIsTotalDeviceFailure@4
 *
 * DESCRIPTION
 *	Check if an NTSTATUS error code represents a
 *	disk hardware failure.
 *	
 * ARGUMENTS
 *	NtStatus
 *		NTSTATUS to test.
 *
 * RETURN VALUE
 *	FALSE if either (NtStatus >= STATUS_SUCCESS), 
 *	STATUS_CRC_ERROR, STATUS_DEVICE_DATA_ERROR;
 *	TRUE otherwise.
 *
 */
BOOLEAN
STDCALL
FsRtlIsTotalDeviceFailure (
	IN	NTSTATUS	NtStatus
	)
{
	return (
		(NT_SUCCESS(NtStatus))
		|| (STATUS_CRC_ERROR == NtStatus)
		|| (STATUS_DEVICE_DATA_ERROR == NtStatus)
		? FALSE
		: TRUE
		);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlIsNtstatusExpected/1
 *	stack32 = 4
 *
 * DESCRIPTION
 *	Check an NTSTATUS value is expected by the FS kernel
 *	subsystem.
 *
 * ARGUMENTS
 *	NtStatus
 *		NTSTATUS to test.
 *
 * RETURN VALUE
 *	TRUE if NtStatus is NOT one out of:
 *	- STATUS_ACCESS_VIOLATION
 *	- STATUS_ILLEGAL_INSTRUCTION
 *	- STATUS_DATATYPE_MISALIGNMENT
 *	- STATUS_INSTRUCTION_MISALIGNMENT
 *	which are the forbidden return stati in the FsRtl
 *	subsystem; FALSE otherwise.
 *
 * REVISIONS
 *	2002-01-17 Fixed a bad bug reported by Bo Brantén.
 *	Up to version 1.8, this function's semantics was
 *	exactly the opposite! Thank you Bo.
 */
BOOLEAN
STDCALL
FsRtlIsNtstatusExpected (
	IN	NTSTATUS	NtStatus
	)
{
	return (
		(STATUS_DATATYPE_MISALIGNMENT == NtStatus)
		|| (STATUS_ACCESS_VIOLATION == NtStatus)
		|| (STATUS_ILLEGAL_INSTRUCTION == NtStatus)
		|| (STATUS_INSTRUCTION_MISALIGNMENT == NtStatus)
		)
		? FALSE
		: TRUE;
}
	

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNormalizeNtstatus@8
 *
 * DESCRIPTION
 *	Normalize an NTSTATUS value for using in the FS subsystem.
 *
 * ARGUMENTS
 *	NtStatusToNormalize
 *		NTSTATUS to normalize.
 *	NormalizedNtStatus
 *		NTSTATUS to return if the NtStatusToNormalize
 *		value is unexpected by the FS kernel subsystem.
 *
 * RETURN VALUE
 * 	NtStatusToNormalize if it is an expected value,
 * 	otherwise NormalizedNtStatus.
 */
NTSTATUS
STDCALL
FsRtlNormalizeNtstatus (
	IN	NTSTATUS	NtStatusToNormalize,
	IN	NTSTATUS	NormalizedNtStatus
	)
{
	return
		(TRUE == FsRtlIsNtstatusExpected(NtStatusToNormalize))
		? NtStatusToNormalize
		: NormalizedNtStatus;
}


/**********************************************************************
 *	Miscellanea (they may fit somewhere else)
 *********************************************************************/


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlAllocateResource@0
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 
 */
DWORD
STDCALL
FsRtlAllocateResource (VOID)
{
	return 0;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlBalanceReads@4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 
 */
DWORD
STDCALL
FsRtlBalanceReads (
	DWORD	Unknown0
	)
{
	return 0;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlCopyRead@32
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 * 	From Bo Branten's ntifs.h v12.
 * 
 */
BOOLEAN
STDCALL
FsRtlCopyRead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Wait,
	IN	ULONG			LockKey,
	OUT	PVOID			Buffer,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlCopyWrite@32
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 
 * NOTE
 * 	From Bo Branten's ntifs.h v12.
 */
BOOLEAN
STDCALL
FsRtlCopyWrite (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Wait,
	IN	ULONG			LockKey,
	IN	PVOID			Buffer,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlGetFileSize@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 
 */
NTSTATUS
STDCALL
FsRtlGetFileSize (
    IN PFILE_OBJECT         FileObject,
    IN OUT PLARGE_INTEGER   FileSize
    )
{
	return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlPostPagingFileStackOverflow@12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 
 */
VOID
STDCALL
FsRtlPostPagingFileStackOverflow (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlPostStackOverflow@12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 
 */
VOID
STDCALL
FsRtlPostStackOverflow (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlSyncVolumes@12
 *
 * DESCRIPTION
 *	Obsolete function.
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *	It always returns STATUS_SUCCESS.
 */
NTSTATUS
STDCALL
FsRtlSyncVolumes (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	return STATUS_SUCCESS;
}


/* EOF */

