/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/ischira.c
 * PURPOSE:
 * PROGRAMER:
 * UPDATE HISTORY:
 *              12/04/99: Ariadne Created
 *              05/30/08: Samuel Serapion adapted  from PROJECT C Library
 *
 */

#include <mbctype.h>


/*
 * @implemented
 */
int _ismbchira( unsigned int c )
{
  return ((c>=0x829F) && (c<=0x82F1));
}

/*
 * @implemented
 */
int _ismbckata( unsigned int c )
{
  return ((c>=0x8340) && (c<=0x8396));
}

/*
 * @implemented
 */
unsigned int _mbctohira( unsigned int c )
{
    if (c >= 0x8340 && c <= 0x837e)
	return c - 0xa1;
    else if (c >= 0x8380 && c <= 0x8396)
	return c - 0xa2;
    else
	return c;
}

/*
 * @implemented
 */
unsigned int _mbctokata( unsigned int c )
{
    if (c >= 0x829f && c <= 0x82dd)
	return c + 0xa1;
    else if (c >= 0x82de && c <= 0x82f1)
	return c + 0xa2;
    else
	return c;
}


