/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/mbscoll.c
 * PURPOSE:     
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <crtdll/mbstring.h>

int _mbscoll(const unsigned char *str1, const unsigned char *str2)
{
	return strcoll(str1,str2);
}