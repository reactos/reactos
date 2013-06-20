/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbclen.c
 * PURPOSE:      Determines the length of a multi byte character
 * PROGRAMERS:
 *              Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffths
 *
 */

#include <precomp.h>
#include <mbstring.h>

/*
 * @implemented
 */
size_t _mbclen(const unsigned char *s)
{
  return _ismbblead(*s) ? 2 : 1;
}

size_t _mbclen2(const unsigned int s)
{
  return (_ismbblead(s>>8) && _ismbbtrail(s&0x00FF)) ? 2 : 1;
}

/*
 * assume MB_CUR_MAX == 2
 *
 * @implemented
 */
int mblen( const char *str, size_t size )
{
  if (str && *str && size)
  {
    return !isleadbyte((unsigned char)*str) ? 1 : (size>1 ? 2 : -1);
  }
  return 0;
}

size_t __cdecl mbrlen(const char *str, size_t len, mbstate_t *state)
{
    mbstate_t s = (state ? *state : 0);
    size_t ret;

    if(!len || !str || !*str)
        return 0;

    if(get_locinfo()->mb_cur_max == 1) {
        return 1;
    }else if(!s && isleadbyte((unsigned char)*str)) {
        if(len == 1) {
            s = (unsigned char)*str;
            ret = -2;
        }else {
            ret = 2;
        }
    }else if(!s) {
        ret = 1;
    }else {
        s = 0;
        ret = 2;
    }

    if(state)
        *state = s;
    return ret;
}

