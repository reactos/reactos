/*
 * COPYRIGHT:   See COPYING in the top level directory
 * 		Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details 
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/atol.c
 * PURPOSE:     Ungets a character from stdin
 * PROGRAMER:   DJ Delorie
		Boudewijn Dekker [ Adapted from djgpp libc ]
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>

long
atol(const char *str)
{
  return strtol(str, 0, 10);
}
