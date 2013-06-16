/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/media.c
 * PURPOSE:         Media
 *
 * PROGRAMMERS:     (none)
 */

#include "precomp.h"

/* FMIFS.9 */
BOOL NTAPI
QuerySupportedMedia(
	IN PWCHAR DriveRoot,
	OUT FMIFS_MEDIA_FLAG *CurrentMedia OPTIONAL,
	IN ULONG Unknown3,
	OUT PULONG Unknown4)
{
	return FALSE;
}

/* EOF */
