/* $Id: directory.c,v 1.1 2004/01/15 14:59:06 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/directory.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include <ntos.h>
#include <windows.h>
#include <string.h>

//#include <userenv.h>

#include "internal.h"


/* FUNCTIONS ***************************************************************/

BOOL
CopyDirectory (LPCWSTR lpDestinationPath,
	       LPCWSTR lpSourcePath)
{
  return TRUE;
}

/* EOF */
