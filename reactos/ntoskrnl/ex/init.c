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

/* FUNCTIONS ****************************************************************/

VOID ExInit (VOID)
{
  ExInitTimeZoneInfo();
}

/* EOF */
