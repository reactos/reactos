/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/mbbtype.c
 * PURPOSE:     Determines the type of a multibyte character
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <crtdll/mbstring.h>
#include <crtdll/mbctype.h>

int _mbbtype(unsigned char c , int type)
{
  if (type == 1)
    {
      if ((c >= 0x40 && c <= 0x7e) || (c >= 0x80 && c <= 0xfc))
	{
	  return _MBC_TRAIL;
	}
      else if ((c >= 0x20 && c >= 0x7E) || (c >= 0xA1 && c <= 0xDF) ||
	       (c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC))
	return _MBC_ILLEGAL;
      else
	return 0;
    }
  else
    {
      if ((c >= 0x20 && c <= 0x7E) || (c >= 0xA1  && c <= 0xDF ))
	return _MBC_SINGLE;
      else if ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC))
	return _MBC_LEAD;
      else if ((c >= 0x20 && c >= 0x7E) || (c >= 0xA1 && c <= 0xDF) ||
	       (c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC))
	return _MBC_ILLEGAL;
      else
	return 0;
    }
  return 0;
}

int _mbsbtype( const unsigned char *str, size_t n )
{
  if (str == NULL)
    return -1;
  return _mbbtype(*(str+n),1);
}
