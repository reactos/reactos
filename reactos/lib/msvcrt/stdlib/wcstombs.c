/* Copyright (C) 1991, 1992, 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */
#include <msvcrti.h>


#ifndef EILSEQ
#define EILSEQ EINVAL
#endif


static const wchar_t encoding_mask[] =
{
  ~0x7ff, ~0xffff, ~0x1fffff, ~0x3ffffff
};

static const unsigned char encoding_byte[] =
{
  0xc0, 0xe0, 0xf0, 0xf8, 0xfc
};

/* We don't need the state really because we don't have shift states
   to maintain between calls to this function.  */
static mbstate_t internal;


extern mbstate_t __no_r_state;  /* Defined in mbtowc.c.  */

size_t
__wcsrtombs (char *dst, const wchar_t **src, size_t len, mbstate_t *ps);

/* Convert the `wchar_t' string in PWCS to a multibyte character string
   in S, writing no more than N characters.  Return the number of bytes
   written, or (size_t) -1 if an invalid `wchar_t' was found.

   Attention: this function should NEVER be intentionally used.
   The interface is completely stupid.  The state is shared between
   all conversion functions.  You should use instead the restartable
   version `wcsrtombs'.  */
size_t
wcstombs (char *s, const wchar_t *pwcs, size_t n)
{
  mbstate_t save_shift = __no_r_state;
  size_t written;

  written = __wcsrtombs (s, &pwcs, n, &__no_r_state);

  /* Restore the old shift state.  */
  __no_r_state = save_shift;

  /* Return how many we wrote (or maybe an error).  */
  return written;
}

size_t
__wcsrtombs (char *dst, const wchar_t **src, size_t len, mbstate_t *ps)
{
  size_t written = 0;
  const wchar_t *run = *src;

  if (ps == NULL)
    ps = &internal;

  if (dst == NULL)
    /* The LEN parameter has to be ignored if we don't actually write
       anything.  */
    len = ~0;

  while (written < len)
    {
      wchar_t wc = *run++;

      if (wc < 0 || wc > 0x7fffffff)
        {
          /* This is no correct ISO 10646 character.  */
          __set_errno (EILSEQ);
          return (size_t) -1;
        }

      if (wc == L'\0')
        {
          /* Found the end.  */
          if (dst != NULL)
            *dst = '\0';
          *src = NULL;
          return written;
        }
      else if (wc < 0x80)
        {
          /* It's an one byte sequence.  */
          if (dst != NULL)
            *dst++ = (char) wc;
          ++written;
        }
      else
        {
          size_t step;

          for (step = 2; step < 6; ++step)
            if ((wc & encoding_mask[step - 2]) == 0)
              break;

          if (written + step >= len)
            /* Too long.  */
            break;

          if (dst != NULL)
            {
              size_t cnt = step;

              dst[0] = encoding_byte[cnt - 2];

              --cnt;
              do
                {
                  dst[cnt] = 0x80 | (wc & 0x3f);
                  wc >>= 6;
                }
              while (--cnt > 0);
              dst[0] |= wc;

              dst += step;
            }

          written += step;
        }
    }

  /* Store position of first unprocessed word.  */
  *src = run;

  return written;
}
//weak_alias (__wcsrtombs, wcsrtombs)