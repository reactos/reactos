/* $Id: misc.c,v 1.1 2004/01/16 15:31:53 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/misc.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include <ntos.h>
#include <windows.h>
#include <string.h>

#include "internal.h"


/* FUNCTIONS ***************************************************************/

LPWSTR
AppendBackslash (LPWSTR String)
{
  ULONG Length;

  Length = lstrlenW (String);
  if (String[Length - 1] != L'\\')
    {
      String[Length] = L'\\';
      Length++;
      String[Length] = (WCHAR)0;
    }

  return &String[Length];
}

/* EOF */
