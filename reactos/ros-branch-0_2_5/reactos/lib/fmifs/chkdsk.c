/* $Id: chkdsk.c,v 1.2 2004/02/23 11:55:12 ekohl Exp $
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
VOID STDCALL
Chkdsk (PWCHAR		DriveRoot,
	PWCHAR		Format,
	BOOLEAN		CorrectErrors,
	BOOLEAN		Verbose,
	BOOLEAN		CheckOnlyIfDirty,
	BOOLEAN		ScanDrive,
	PVOID		Unused2,
	PVOID		Unused3,
	PFMIFSCALLBACK	Callback)
{
  BOOLEAN	Argument = FALSE;

  /* FAIL immediately */
  Callback (DONE,	/* Command */
	    0,		/* DWORD Modifier */
	    &Argument);	/* Argument */
}


/* FMIFS.2 (SP4 only?) */
VOID STDCALL
ChkdskEx (VOID)
{
}

/* EOF */
