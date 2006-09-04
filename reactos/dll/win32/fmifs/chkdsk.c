/* $Id$
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
#include "precomp.h"

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

/* EOF */
