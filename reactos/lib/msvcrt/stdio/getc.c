/* $Id: getc.c,v 1.6 2002/12/05 15:30:44 robd Exp $
 *
 *  ReactOS msvcrt library
 *
 *  getc.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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

#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/wchar.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>

//getc can be a macro
#undef getc
#undef getwc

#ifndef MB_CUR_MAX
#define MB_CUR_MAX 10
#endif

int getc(FILE *fp)
{
    int c = -1;

    // check for invalid stream
	if ( !__validfp (fp) ) {
		__set_errno(EINVAL);
		return EOF;
	}
    // check for read access on stream
	if ( !OPEN4READING(fp) ) {
		__set_errno(EINVAL);
		return EOF;
	}
	if(fp->_cnt > 0) {
		fp->_cnt--;
		c =  (int)*fp->_ptr++;
	} else {
		c =  _filbuf(fp);
	}
	return c;
}

wint_t getwc(FILE *fp)
{
    wint_t c = -1;

    // check for invalid stream
    if (!__validfp(fp)) {
        __set_errno(EINVAL);
        return WEOF;
    }
    // check for read access on stream
//#define OPEN4READING(f) ((((f)->_flag & _IOREAD) == _IOREAD ) )
    if (!OPEN4READING(fp)) {
        __set_errno(EINVAL);
        return WEOF;
    }
    // might check on multi bytes if text mode
    if (fp->_flag & _IOBINARY) {
        if (fp->_cnt > 1) {
            fp->_cnt -= sizeof(wchar_t);
            c = (wint_t)*((wchar_t*)(fp->_ptr))++;
        } else {
            c = _filwbuf(fp);
        }
    } else {
#if 0
        BOOL get_bytes = 0;
        int mb_cnt = 0;
        int found_cr = 0;
        //int count;
        char mbchar[MB_CUR_MAX];

        do {
            if (fp->_cnt > 0) {
                fp->_cnt--;
                mbchar[mb_cnt] = *fp->_ptr++;
            } else {
                mbchar[mb_cnt] = _filbuf(fp);
            }
            if (isleadbyte(mbchar[mb_cnt])) {
                get_bytes = 1;
            } else {
                get_bytes = 0;
            }
            if (_ismbblead(mbchar[mb_cnt])) {
            }
            ++mb_cnt;
        //}
        } while (get_bytes);

        // Convert a multibyte character to a corresponding wide character.
        mb_cnt = mbtowc(&c, mbchar, mb_cnt);
        if (mb_cnt == -1) {
            fp->_flag |= _IOERR;
            return WEOF;
        }
#else
        if (fp->_cnt > 0) {
            fp->_cnt--;
            c = *fp->_ptr++;
        } else {
            c = _filbuf(fp);
        }
#endif
    }
    return c;
}

