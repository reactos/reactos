/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/ismblead.c
 * PURPOSE:     Checks for a lead byte 
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <crtdll/mbstring.h>

int _ismbblead(unsigned int byte)
{

	return (int)IsDBCSLeadByte(byte) 
}

int _ismbslead( const unsigned char *str, const unsigned char *t)
{
	char *s = str;
	while(*s != 0 && s != t) 
	{
		
		s+= mblen(*s);
	}		
	return ismbblead( *s)
}

