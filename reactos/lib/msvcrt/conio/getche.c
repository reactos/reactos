/*
 * COPYRIGHT:   See COPYING in the top level directory
 * 		Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details 
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/getche.c
 * PURPOSE:     Reads a character from stdin
 * PROGRAMER:   DJ Delorie
				Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <msvcrti.h>


int _getche(void)
{
  if (char_avail)
    /*
     * We don't know, wether the ungot char was already echoed
     * we assume yes (for example in cscanf, probably the only
     * place where ungetch is ever called.
     * There is no way to check for this really, because
     * ungetch could have been called with a character that
     * hasn't been got by a conio function.
     * We don't echo again.
     */ 
    return(_getch());
  return (_putch(_getch()));
}
