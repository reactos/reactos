/* $Id: util.c,v 1.2 1999/12/26 16:36:45 ea Exp $
 *
 * reactos/ntoskrnl/fs/util.c
 *
 */
#include <ntos.h>


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
	NTSTATUS	NtStatus
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
 *	FsRtlIsNtstatusExpected@4
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
 *	TRUE if NtStatus is either STATUS_ACCESS_VIOLATION,
 *	STATUS_ILLEGAL_INSTRUCTION, STATUS_DATATYPE_MISALIGNMENT,
 *	0xC00000AA; FALSE otherwise.
 *
 * NOTES
 *	By calling the function with all possible values,
 *	one unknown NTSTATUS value makes the function
 *	return 0x00 (0xC00000AA).
 */
BOOLEAN
STDCALL
FsRtlIsNtstatusExpected (
	NTSTATUS	NtStatus
	)
{
	return (
		(STATUS_DATATYPE_MISALIGNMENT == NtStatus)
		|| (STATUS_ACCESS_VIOLATION == NtStatus)
		|| (STATUS_ILLEGAL_INSTRUCTION == NtStatus)
		|| (STATUS_UNKNOWN_C00000AA == NtStatus)  /* FIXME */
		)
		? TRUE
		: FALSE;
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
	NTSTATUS	NtStatusToNormalize,
	NTSTATUS	NormalizedNtStatus
	)
{
	return
		(TRUE == FsRtlIsNtstatusExpected(NtStatusToNormalize))
		? NtStatusToNormalize
		: NormalizedNtStatus;
}


/* EOF */

