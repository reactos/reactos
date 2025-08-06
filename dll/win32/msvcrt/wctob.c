/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT
 * PURPOSE:     Implementation of wctob and btowc
 * COPYRIGHT:   Copyright 2024 ReactOS Project
 */

#include <precomp.h>
#include <wchar.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

/*
 * @implemented
 * wctob - Convert wide character to single byte character
 */
int __cdecl wctob(wint_t wc)
{
    unsigned char buf[MB_LEN_MAX];
    
    /* Check if it's a valid wide character */
    if (wc == WEOF)
        return EOF;
    
    /* Try to convert to multibyte */
    if (wctomb((char *)buf, wc) == 1)
        return buf[0];
    
    return EOF;
}

/*
 * @implemented
 * btowc - Convert single byte to wide character
 */
wint_t __cdecl btowc(int c)
{
    unsigned char ch;
    wchar_t wc;
    
    /* Check for EOF */
    if (c == EOF)
        return WEOF;
    
    /* Convert to unsigned char */
    ch = (unsigned char)c;
    
    /* Convert to wide character */
    if (mbtowc(&wc, (const char *)&ch, 1) == 1)
        return wc;
    
    return WEOF;
}