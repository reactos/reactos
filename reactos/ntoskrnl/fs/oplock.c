/* $Id: oplock.c,v 1.1 2000/03/10 22:09:17 ea Exp $
 *
 * reactos/ntoskrnl/fs/oplock.c
 *
 */
#include <ntos.h>


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
DWORD
STDCALL
FsRtlCheckOplock (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	return 0;
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
BOOLEAN
STDCALL
FsRtlCurrentBatchOplock (
	DWORD	Unknown0
	)
{
	return FALSE;
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
VOID
STDCALL
FsRtlInitializeOplock (
	DWORD	Unknown0
	)
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
NTSTATUS
STDCALL
FsRtlOplockFsctrl (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	return 0;
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
BOOLEAN
STDCALL
FsRtlOplockIsFastIoPossible (
	DWORD	Unknown0
	)
{
	return FALSE;
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
DWORD
STDCALL
FsRtlUninitializeOplock (
	DWORD	Unknown0
	)
{
	return 0;
}


/* EOF */
