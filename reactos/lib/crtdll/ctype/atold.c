/*
 * COPYRIGHT:   See COPYING in the top level directory
 * 		Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details 
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/ungetch.c
 * PURPOSE:     Ungets a character from stdin
 * PROGRAMER:   DJ Delorie
		Boudewijn Dekker [ Adapted from djgpp libc ]
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdlib.h>

long double
_atold(const char *ascii)
{
  return _strtold(ascii, 0);
}
