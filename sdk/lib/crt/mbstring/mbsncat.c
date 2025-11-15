/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/mbstring/mbsncat.c
 * PURPOSE:     Concatenate two multi byte string to maximum of n characters or bytes
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              12/04/99: Created
 *              11/10/2025 Somewhat synced with Wine 10.0 by Doug Lyons
 */

#include <precomp.h>
#include <mbstring.h>
#include <string.h>

static inline unsigned char* u_strncat( unsigned char* dst, const unsigned char* src, size_t len )
{
  return (unsigned char*)strncat( (char*)dst, (const char*)src, len);
}

size_t _mbclen2(const unsigned int s);
unsigned char *_mbset (unsigned char *string, int c);

/*
 * @implemented
 */
unsigned char *_mbsncat (unsigned char *dst, const unsigned char *src, size_t n)
{
    MSVCRT_pthreadmbcinfo mbcinfo = get_mbcinfo();

    if (!n)
        return dst;

    if (!dst || !src) ERR("Bad Parameter\n");

    if (mbcinfo->ismbcodepage)
    {
        unsigned char *res = dst;
        while (*dst)
        {
            if (_ismbblead(*dst++))
                dst++;
        }
        while (*src && n--)
        {
            *dst++ = *src;
            if (_ismbblead(*src++))
                *dst++ = *src++;
        }
        *dst = '\0';
        return res;
    }
    return u_strncat(dst, src, n); /* ASCII CP */
}

/*
 * @implemented
 */
unsigned char * _mbsnbcat(unsigned char *dst, const unsigned char *src, size_t n)
{
    MSVCRT_pthreadmbcinfo mbcinfo = get_mbcinfo();
    unsigned char *s;

    /* replace TRACE with ERR for debug output */
    TRACE("Src %s\n", wine_dbgstr_an((const char*)src, n));

    if (!dst || !src) ERR("Bad Parameter\n");

    if (!src && !dst && !n && !MSVCRT_CHECK_PMT(dst && src))
        return NULL;

    if (mbcinfo->ismbcodepage)
    {
        unsigned char *res = dst;
        while (*dst)
        {
            if (_ismbblead(*dst++))
            {
                if (*dst)
                {
                    dst++;
                }
                else
                {
                    /* as per msdn overwrite the lead byte in front of '\0' */
                    dst--;
                    break;
                }
            }
        }
        while (*src && n--) *dst++ = *src++;
        *dst = '\0';
        return res;
    }
    s = u_strncat(dst, src, n); /* ASCII CP */
    /* replace TRACE with ERR for debug output */
    TRACE("Dst %s\n", wine_dbgstr_an((const char*)dst, _mbslen(dst)));
    return s;
}
