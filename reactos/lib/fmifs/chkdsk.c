/* $Id: chkdsk.c,v 1.1 1999/05/11 21:19:41 ea Exp $
 *
 * COPYING:	See the top level directory
 * PROJECT:	ReactOS 
 * FILE:	reactos/lib/fmifs/chkdsk.c
 * DESCRIPTION:	File management IFS utility functions
 * PROGRAMMER:	Emanuele Aliberti
 * UPDATED
 * 	1999-02-16 (Emanuele Aliberti)
 * 		Entry points added.
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <fmifs.h>


/* FMIFS.1 */
VOID
__stdcall
Chkdsk(
	PWCHAR		DriveRoot, 
	PWCHAR		Format,
	BOOL		CorrectErrors, 
	BOOL		Verbose, 
	BOOL		CheckOnlyIfDirty,
	BOOL		ScanDrive, 
	PVOID		Unused2, 
	PVOID		Unused3,
	PFMIFSCALLBACK	Callback
	)
{
	BOOL	Argument = FALSE;

	/* FAIL immediately */
	Callback(
		DONE,		/* Command */
		0,		/* DWORD Modifier */
		& Argument	/* Argument */
		);
}


/* FMIFS.2 (SP4 only?) */
VOID
__stdcall
ChkdskEx(VOID)
{
}


/* EOF */
