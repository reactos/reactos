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

#include <windows.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/ctype.h>
#include <msvcrt/wchar.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>


int
STDCALL
WideCharToMultiByte(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCWSTR  lpWideCharStr,
    int      cchWideChar,
    LPSTR    lpMultiByteStr,
    int      cchMultiByte,
    LPCSTR   lpDefaultChar,
    LPBOOL   lpUsedDefaultChar);


/*
 * @unimplemented
 */
int wctomb(char* dst, wchar_t ch)
{
#if 0
    return WideCharToMultiByte(CP_ACP, 0, &ch, 1, dst, 6, NULL, NULL);
#else
    if (dst == NULL) {
        return 1;
    }
    *dst = ch;
    return 1;
#endif
}


#if 0

#ifndef EILSEQ
#define EILSEQ EINVAL
#endif

static const wchar_t encoding_mask[] =
{
    /* This reflects the sources *nix origin where type wchar_t 
       was 32 bits wide. Since our type wchar_t is only 16 bits
       wide all this module will need to be reviewed.
       Simplest option may well be to forward this modules work 
       on to the kernel which already has support for this.
      */
    ~0x7ff, ~0xffff, ~0x1fffff, ~0x3ffffff
    //~0x0000-07ff, ~0x0000-ffff, ~0x001f-ffff, ~0x03ff-ffff
};

static const unsigned char encoding_byte[] =
{
    0xc0, 0xe0, 0xf0, 0xf8, 0xfc
};

/* The state is for this UTF8 encoding not used.  */
//static mbstate_t internal;
//extern mbstate_t __no_r_state;  /* Defined in mbtowc.c.  */

size_t __wcrtomb(char *s, wchar_t wc);

/* Convert WCHAR into its multibyte character representation,
   putting this in S and returning its length.

   Attention: this function should NEVER be intentionally used.
   The interface is completely stupid.  The state is shared between
   all conversion functions.  You should use instead the restartable
   version `wcrtomb'.  */

int wctomb(char *s, wchar_t wchar)
{
  /* If S is NULL the function has to return null or not null
     depending on the encoding having a state depending encoding or
     not.  This is nonsense because any multibyte encoding has a
     state.  The ISO C amendment 1 corrects this while introducing the
     restartable functions.  We simply say here all encodings have a
     state.  */
    if (s == NULL) {
        return 1;
    }
    return __wcrtomb(s, wchar);
}

size_t __wcrtomb(char *s, wchar_t wc)
{
    char fake[1];
    size_t written = 0;

    if (s == NULL) {
        s = fake;
        wc = L'\0';
    }
    /* Store the UTF8 representation of WC.  */
    //if (wc < 0 || wc > 0x7fffffff) {
    if (wc < 0 || wc > 0x7fff) {
        /* This is no correct ISO 10646 character.  */
        __set_errno (EILSEQ);
        return (size_t) -1;
    }
    if (wc < 0x80) {
        /* It's a one byte sequence.  */
        if (s != NULL) {
            *s = (char)wc;
        }
        return 1;
    }
    for (written = 2; written < 6; ++written) {
        if ((wc & encoding_mask[written - 2]) == 0) {
            break;
        }
    }
    if (s != NULL) {
        size_t cnt = written;
        s[0] = encoding_byte[cnt - 2];
        --cnt;
        do {
            s[cnt] = 0x80 | (wc & 0x3f);
            wc >>= 6;
        } while (--cnt > 0);
        s[0] |= wc;
    }
    return written;
}

#endif
