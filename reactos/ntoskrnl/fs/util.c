/* $Id: util.c,v 1.1 1999/08/20 16:29:22 ea Exp $
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
 *	STATUS_CRC_ERROR, 0xC000009C; TRUE otherwise.
 *
 * NOTES
 *	By calling the function with all possible values,
 *	one unknown NTSTATUS value makes the function
 *	return TRUE (0xC000009C).
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
		|| (STATUS_UNKNOWN_C000009C == NtStatus) /* FIXME */
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
 *	STATUS_ILLEGAL_INSTRUCTION, 0x80000002, 0xC00000AA;
 *	FALSE otherwise.
 *
 * NOTES
 *	By calling the function with all possible values,
 *	two unknown NTSTATUS values make the function
 *	return 0x00 (0x80000002, 0xC00000AA).
 */
BOOLEAN
STDCALL
FsRtlIsNtstatusExpected (
	NTSTATUS	NtStatus
	)
{
	return (
		(STATUS_UNKNOWN_80000002 == NtStatus)  /* FIXME */
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

