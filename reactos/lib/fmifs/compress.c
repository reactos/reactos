/* $Id: compress.c,v 1.1 1999/05/11 21:19:41 ea Exp $
 *
 * COPYING:	See the top level directory
 * PROJECT:	ReactOS 
 * FILE:	reactos/lib/fmifs/compress.c
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


/* FMIFS.4 */
BOOL
__stdcall
EnableVolumeCompression(
	PWCHAR	DriveRoot,
	BOOL	Enable
	)
{
	return FALSE;
}


/* EOF */
