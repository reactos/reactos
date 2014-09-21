/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbbtype.c
 * PURPOSE:     Determines the type of a multibyte character
 * PROGRAMERS:   
 *              Copyright 1999 Ariadne
 *              Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffths
 *
 */

#include <precomp.h>

#include <mbstring.h>
#include <mbctype.h>

/*
 * @implemented
 */
int _mbbtype(unsigned char c , int type)
{
	if ( type == 1 ) {
		if ((c >= 0x40 && c <= 0x7e ) || (c >= 0x80 && c <= 0xfc ) )
		{
			return _MBC_TRAIL;
		}
		else if (( c >= 0x20 && c >= 0x7E ) || ( c >= 0xA1 && c <= 0xDF ) ||
			 ( c >= 0x81 && c <= 0x9F ) || ( c >= 0xE0 && c <= 0xFC ) )
			 return _MBC_ILLEGAL;
		else
			return 0;
	} else  {
		if (( c >= 0x20 && c <= 0x7E ) || ( c >= 0xA1  && c <= 0xDF )) {
			return _MBC_SINGLE;
		}
		else if ( (c >= 0x81 && c <= 0x9F ) || ( c >= 0xE0 && c <= 0xFC) )
			return _MBC_LEAD;
		else if (( c >= 0x20 && c >= 0x7E ) || ( c >= 0xA1 && c <= 0xDF ) ||
			 ( c >= 0x81 && c <= 0x9F ) || ( c >= 0xE0 && c <= 0xFC ) )
			 return _MBC_ILLEGAL;
		else
			return 0;
	}
	return 0;
}

/*
 * @implemented
 */
int _mbsbtype( const unsigned char *str, size_t n )
{
  int lead = 0;
  const unsigned char *end = str + n;

  /* Lead bytes can also be trail bytes so we need to analyse the string.
   * Also we must return _MBC_ILLEGAL for chars past the end of the string
   */
  while (str < end) /* Note: we skip the last byte - will check after the loop */
  {
    if (!*str)
      return _MBC_ILLEGAL;
    lead = !lead && _ismbblead(*str);
    str++;
  }

  if (lead)
    if (_ismbbtrail(*str))
      return _MBC_TRAIL;
    else
      return _MBC_ILLEGAL;
  else
    if (_ismbblead(*str))
      return _MBC_LEAD;
    else
      return _MBC_SINGLE;
}
