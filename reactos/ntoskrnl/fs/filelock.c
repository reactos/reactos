/* $Id: filelock.c,v 1.1 2000/02/26 16:22:27 ea Exp $
 *
 * reactos/ntoskrnl/fs/filelock.c
 *
 */
#include <ntos.h>
#include <ddk/ntifs.h>


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlCheckLockForReadAccess@8
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
BOOLEAN
STDCALL
FsRtlCheckLockForReadAccess (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlCheckLockForWriteAccess@8
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
BOOLEAN
STDCALL
FsRtlCheckLockForWriteAccess (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastCheckLockForRead@24
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
BOOLEAN
STDCALL
FsRtlFastCheckLockForRead (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastCheckLockForWrite@24
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
BOOLEAN
STDCALL
FsRtlFastCheckLockForWrite (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastUnlockAll@16
 *	FsRtlFastUnlockAllByKey@20
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
static
NTSTATUS
STDCALL
_FsRtlFastUnlockAllByKey (
	IN	DWORD	Unknown0,
	IN	DWORD	Unknown1,
	IN	DWORD	Unknown2,
	IN	DWORD	Unknown3,
	IN	BOOLEAN	UseKey,	/* FIXME: guess */
	IN	DWORD	Key	/* FIXME: guess */
	)
{
	/* FIXME: */
	return (STATUS_RANGE_NOT_LOCKED);
}


NTSTATUS
STDCALL
FsRtlFastUnlockAll (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	return _FsRtlFastUnlockAllByKey (
			Unknown0,
			Unknown1,
			Unknown2,
			Unknown3,
			FALSE, /* DO NOT USE KEY? */
			0
			);
}


NTSTATUS
STDCALL
FsRtlFastUnlockAllByKey (
	IN	DWORD	Unknown0,
	IN	DWORD	Unknown1,
	IN	DWORD	Unknown2,
	IN	DWORD	Unknown3,
	IN	DWORD	Key
	)
{
	return _FsRtlFastUnlockAllByKey (
			Unknown0,
			Unknown1,
			Unknown2,
			Unknown3,
			TRUE, /* USE KEY? */
			Key
			);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastUnlockSingle@32
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
FsRtlFastUnlockSingle (
	IN	DWORD	Unknown0,
	IN	DWORD	Unknown1,
	IN	DWORD	Unknown2,
	IN	DWORD	Unknown3,
	IN	DWORD	Unknown4,
	IN	DWORD	Unknown5,
	IN	DWORD	Unknown6,
	IN	DWORD	Unknown7
	)
{
	return (STATUS_RANGE_NOT_LOCKED);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlGetNextFileLock@8
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
FsRtlGetNextFileLock (
	IN	DWORD	Unknown0,
	IN OUT	PVOID	Unknown1
	)
{
	return (STATUS_NOT_IMPLEMENTED);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlInitializeFileLock@12
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
FsRtlInitializeFileLock (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlPrivateLock@48
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
BOOLEAN
STDCALL
FsRtlPrivateLock (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6,
	DWORD	Unknown7,
	DWORD	Unknown8,
	DWORD	Unknown9,
	DWORD	Unknown10,
	DWORD	Unknown11
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlProcessFileLock@12
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
FsRtlProcessFileLock (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	return (STATUS_NOT_IMPLEMENTED);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlUninitializeFileLock@4
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
FsRtlUninitializeFileLock (
	IN OUT	PVOID	lpUnknown0
	)
{
}


/* EOF */
