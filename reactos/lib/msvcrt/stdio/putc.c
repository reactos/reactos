/* $Id: putc.c,v 1.8 2003/07/11 21:58:09 royce Exp $
 *
 *  ReactOS msvcrt library
 *
 *  putc.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  Based on original work Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/wchar.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>

// putc can be a macro
#undef putc
#undef putwc

#ifndef MB_CUR_MAX_CONST
#define MB_CUR_MAX_CONST 10
#endif

int putc(int c, FILE* fp)
{
    // valid stream macro should check that fp is dword aligned
    if (!__validfp(fp)) {
        __set_errno(EINVAL);
        return EOF;
    }
    // check for write access on fp
    if (!OPEN4WRITING(fp)) {
        __set_errno(EINVAL);
        return EOF;
    }
    fp->_flag |= _IODIRTY;
    if (fp->_cnt > 0) {
        fp->_cnt--;
        *(fp)->_ptr++ = (unsigned char)c;
        return (int)(unsigned char)c; 
    } else {
        return _flsbuf((unsigned char)c, fp);
    }
    return EOF;
}

//wint_t putwc(wint_t c, FILE* fp)
/*
 * @implemented
 */
int putwc(wint_t c, FILE* fp)
{
    // valid stream macro should check that fp is dword aligned
    if (!__validfp(fp)) {
        __set_errno(EINVAL);
        return WEOF;
    }
    // check for write access on fp
    if (!OPEN4WRITING(fp)) {
        __set_errno(EINVAL);
        return WEOF;
    }
    // might check on multi bytes if text mode
    if (fp->_flag & _IOBINARY) {
    //if (1) {

        fp->_flag |= _IODIRTY;
        if (fp->_cnt > 0) {
            fp->_cnt -= sizeof(wchar_t);
            //*((wchar_t*)(fp->_ptr))++ = c;
            *((wchar_t*)(fp->_ptr)) = c;
            ++((wchar_t*)(fp->_ptr));
            return (wint_t)c;
        } else {
#if 1
            wint_t result;
            result = _flsbuf(c, fp);
            if (result == EOF)
                return WEOF;
            result = _flsbuf((int)(c >> 8), fp);
            if (result == EOF)
                return WEOF;
            return result;
#else
            return _flswbuf(c, fp);
#endif
        }

    } else {
#if 0
        int i;
        int mb_cnt;
        char mbchar[MB_CUR_MAX_CONST];

        // Convert wide character to the corresponding multibyte character.
        mb_cnt = wctomb(mbchar, (wchar_t)c);
        if (mb_cnt == -1) {
            fp->_flag |= _IOERR;
            return WEOF;
        }
        if (mb_cnt > MB_CUR_MAX_CONST) {
            // BARF();
        }
        for (i = 0; i < mb_cnt; i++) {
            fp->_flag |= _IODIRTY;
            if (fp->_cnt > 0) {
                fp->_cnt--;
                *(fp)->_ptr++ = (unsigned char)mbchar[i];
            } else {
                if (_flsbuf((unsigned char)mbchar[i], fp) == EOF) {
                    return WEOF;
                }
            }
        }
#else
        fp->_flag |= _IODIRTY;
        if (fp->_cnt > 0) {
            fp->_cnt--;
            *(fp)->_ptr++ = (unsigned char)c;
        } else {
            if (_flsbuf(c, fp) == EOF) {
                return WEOF;
            }
        }
#endif
        return c; 
    }
    return WEOF;
}
