/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/ex/init.c
 * PURPOSE:         executive initalization
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  Created 11/09/99
 */

#include <ddk/ntddk.h>
#include <internal/ex.h>

/* DATA **********************************************************************/

POBJECT_TYPE EXPORTED ExDesktopObjectType = NULL;
POBJECT_TYPE EXPORTED ExWindowStationObjectType = NULL;


/* FUNCTIONS ****************************************************************/

VOID ExInit (VOID)
{
  ExInitTimeZoneInfo();
}


BOOLEAN
STDCALL
ExIsProcessorFeaturePresent (
	IN	ULONG	ProcessorFeature
	)
{
	if (ProcessorFeature >= 32)
		return FALSE;

	return FALSE;
}


VOID
STDCALL
ExPostSystemEvent (
	ULONG	Unknown1,
	ULONG	Unknown2,
	ULONG	Unknown3
	)
{
	/* doesn't do anything */
}

/* EOF */
