/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/ctype/iscsym.c
 * PURPOSE:     Check for a valid characters in a c symbol
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <msvcrt/ctype.h>


/*
 * @implemented
 */
int __iscsymf(int c)
{
	return (isalpha(c) || ( c == '_' )) ;
}

/*
 * @implemented
 */
int __iscsym(int c)
{
	return (isalnum(c) || ( c == '_' )) ;
}
