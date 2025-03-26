/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbslwr.c
 * PURPOSE:     Multibyte lowercase functions
 * PROGRAMER:   Eric Kohl
 *              Samuel Serapion, adapted from PROJECT C Library
 */

#include <precomp.h>
#include <mbstring.h>
#include <ctype.h>

unsigned int _mbbtolower(unsigned int c)
{
	if (!_ismbblead(c) )
		return tolower(c);
	return c;
}

/*
 * @implemented
 */
unsigned int _mbctolower(unsigned int c)
{
    return _ismbcupper (c) ? c + 0x21 : c;
}

/*
 * @implemented
 */
unsigned char * _mbslwr(unsigned char *x)
{
    unsigned char *y=x;

    if (x == NULL)
    {
        return NULL;
    }

    while (*y)
    {
        if (!_ismbblead(*y))
        {
            *y = tolower(*y);
            y++;
        }
        else
        {
            *y = _mbctolower(*(unsigned short *)y);
            y++;
        }
    }
    return x;
}
