
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/ctype/iscsym.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <ctype.h>

#undef iscsym
int
iscsym (int c)
{
	return __iscsym(c);
}

int
__iscsym (int c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || ( c == '_' );
}

#undef iscsymf
int
iscsymf (int c)
{
	return __iscsymf(c);
}

int
__iscsymf (int c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || ( c == '_' );	
}


