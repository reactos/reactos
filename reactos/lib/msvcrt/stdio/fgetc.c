/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/stdio/fgetc.c
 * PURPOSE:     Get a character string from stdin
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Appropriated for Reactos
		25/02/99: Added fgetwc
 */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


int fgetc(FILE *f)
{
  return getc(f);
}

wint_t fgetwc(FILE *f)
{
  return getwc(f);
}
