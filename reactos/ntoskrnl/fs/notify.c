/* $Id: notify.c,v 1.1 2000/03/11 00:51:36 ea Exp $
 *
 * reactos/ntoskrnl/fs/notify.c
 *
 */
#include <ntos.h>
#include <ddk/ntifs.h>


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyChangeDirectory@28
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
FsRtlNotifyChangeDirectory (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6
	)
{
	FsRtlNotifyFullChangeDirectory (
		Unknown0,
		Unknown3,
		Unknown1,
		Unknown2,
		Unknown4,
		1,
		Unknown5,
		Unknown6,
		0,
		0
		);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyCleanup@12
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
FsRtlNotifyCleanup (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyFullChangeDirectory@40
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
FsRtlNotifyFullChangeDirectory (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6,
	DWORD	Unknown7,
	DWORD	Unknown8,
	DWORD	Unknown9
	)
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyFullReportChange@36
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
FsRtlNotifyFullReportChange (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6,
	DWORD	Unknown7,
	DWORD	Unknown8
	)
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyInitializeSync@4
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
FsRtlNotifyInitializeSync (
	IN OUT	PVOID	* Unknown0
	)
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyReportChange@20
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
FsRtlNotifyReportChange (
	DWORD	Unknown0,
	DWORD	Unknown1,
	PVOID	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	/*
	 * It should probably call
	 * FsRtlNotifyFullReportChange.
	 */
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNotifyUninitializeSync@4
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
FsRtlNotifyUninitializeSync (
	IN OUT	PVOID	* Unknown0
	)
{
}


/* EOF */
